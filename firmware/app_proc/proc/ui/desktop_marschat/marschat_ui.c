/************************************************************************************
**                                                                                 **
**                                 mcHF QRP Transceiver                            **
**                         Krassi Atanassov - M0NKA, 2013-2026                     **
**                                                                                 **
**---------------------------------------------------------------------------------**
**                                                                                 **
**  File name:		marschat_ui.c                                                  **
**  Description:	MarsChat chat dialog - atlas themed, history + compose +       **
**					slot telemetry                                                 **
**  Licence:		https://github.com/m0nka/mcHF/blob/main/LICENSE                **
************************************************************************************/
//
// Everything on this screen is drawn by hand (theme/atlas_draw.c) - emWin
// widgets are used only where they earn their keep, i.e. the keys and the
// action row, whose painting is taken over by a skin callback while the
// framework keeps the hit testing.
//
// Repaints are scoped: the clock and the slot countdown tick twice a
// second and live in their own child windows, so the panels below them
// are only redrawn when the data behind them actually changes.
//
// This is a top level screen (MODE_DESKTOP_MARSCHAT), built the same way
// as the FT8 desktop: the dialog is a child of the desktop window with
// nothing else on screen, so no other shell repaints over it
//
#include "mchf_pro_board.h"
#include "main.h"

#include "ui_proc.h"
#include "gui.h"
#include "dialog.h"
#include "desktop\ui_controls_layout.h"

#include "rtc.h"

#include "atlas_draw.h"

#include "mc_frame.h"
#include "marschat_proc.h"

#include "marschat_ui.h"

// UI driver public state
extern struct	UI_DRIVER_STATE			ui_s;
extern struct	PROC_STATE				ps;

static WM_HWIN	hMcDialog;

// Free-text compose buffer - typed via the A-P/space/backspace keys,
// split into MC_PAYLOAD_CHARS chunks and queued on SEND (marschat_proc.c
// owns the tx queue and the slot timing; this is just the local draft)
#define MC_UI_COMPOSE_MAX	(MARSCHAT_TX_QUEUE_LEN * MC_PAYLOAD_CHARS)

static WM_HTIMER	hMcTimer;
static WM_HWIN		hMcTitle;					// title strip, owns the clock
static WM_HWIN		hMcSlot;					// slot bar, owns the countdown

static char			mc_ui_compose[MC_UI_COMPOSE_MAX + 1];
static int			mc_ui_compose_len = 0;
static uint8_t		mc_ui_sending = 0;			// input locked, message in flight

// Slot role this station uses when a session starts. The station that
// opens the chat is the caller (even slot indices) and just presses
// SEND; the one answering picks PEER first, or both would key in the
// same slots and never hear each other
static uint8_t		mc_ui_role = MC_ROLE_CALLER;

// ---------------------------------------------------------------------
// Message list. Custom drawn rather than a LISTBOX: the rows carry a
// direction chip, a timestamp and an SNR column, none of which a listbox
// can style

#define MC_UI_LINE_CAP		16

// Our own outgoing line kind, alongside the MC_UI_LINE_RX / _INFO the
// marschat task queues
#define MC_UI_LINE_TX		2

typedef struct
{
	uint8_t	kind;								// MC_UI_LINE_xxx or MC_UI_LINE_TX
	char	time[8];							// "21:03"
	char	text[26];
	char	meta[12];							// "-7 dB" for rx, "seq 3" for tx
} MC_UI_LINE;

static MC_UI_LINE	mc_ui_lines[MC_UI_LINE_CAP];
static uint8_t		mc_ui_line_count = 0;		// total ever added
static int8_t		mc_ui_last_snr = 0;
static int16_t		mc_ui_last_dt = 0;
static uint8_t		mc_ui_have_rx = 0;

// Snapshot of everything the panels below the clock draw from - when it
// has not moved, they are not repainted
typedef struct
{
	uint8_t		lines;
	uint8_t		state;
	uint8_t		role;
	uint8_t		pending;
	uint8_t		retries;
	uint8_t		tx_seq;
	uint8_t		last_rx_seq;
	uint16_t	queued;
	int			compose_len;
	uint8_t		sending;
} MC_UI_MODEL;

static MC_UI_MODEL	mc_ui_model;

static const GUI_WIDGET_CREATE_INFO _aDialog[] =
{
	// -----------------------------------------------------------------------------------------------------------------------------
	//							name						id						x		y		xsize	ysize
	// -----------------------------------------------------------------------------------------------------------------------------
	{ WINDOW_CreateIndirect,	"", 						ID_WINDOW_0,			0,		0,		MC_UI_W,	MC_UI_H,	0,	0x64,	0 },

	// Character keys, two rows of eight (A-H, I-P)
	{ BUTTON_CreateIndirect,	"",							ID_BUTTON_CHAR_0 + 0,	MC_KEY_X + 0 * (MC_KEY_W + MC_KEY_GAP),	MC_KEY_Y1,	MC_KEY_W,	MC_KEY_H,	0,	0x0,	0 },
	{ BUTTON_CreateIndirect,	"",							ID_BUTTON_CHAR_0 + 1,	MC_KEY_X + 1 * (MC_KEY_W + MC_KEY_GAP),	MC_KEY_Y1,	MC_KEY_W,	MC_KEY_H,	0,	0x0,	0 },
	{ BUTTON_CreateIndirect,	"",							ID_BUTTON_CHAR_0 + 2,	MC_KEY_X + 2 * (MC_KEY_W + MC_KEY_GAP),	MC_KEY_Y1,	MC_KEY_W,	MC_KEY_H,	0,	0x0,	0 },
	{ BUTTON_CreateIndirect,	"",							ID_BUTTON_CHAR_0 + 3,	MC_KEY_X + 3 * (MC_KEY_W + MC_KEY_GAP),	MC_KEY_Y1,	MC_KEY_W,	MC_KEY_H,	0,	0x0,	0 },
	{ BUTTON_CreateIndirect,	"",							ID_BUTTON_CHAR_0 + 4,	MC_KEY_X + 4 * (MC_KEY_W + MC_KEY_GAP),	MC_KEY_Y1,	MC_KEY_W,	MC_KEY_H,	0,	0x0,	0 },
	{ BUTTON_CreateIndirect,	"",							ID_BUTTON_CHAR_0 + 5,	MC_KEY_X + 5 * (MC_KEY_W + MC_KEY_GAP),	MC_KEY_Y1,	MC_KEY_W,	MC_KEY_H,	0,	0x0,	0 },
	{ BUTTON_CreateIndirect,	"",							ID_BUTTON_CHAR_0 + 6,	MC_KEY_X + 6 * (MC_KEY_W + MC_KEY_GAP),	MC_KEY_Y1,	MC_KEY_W,	MC_KEY_H,	0,	0x0,	0 },
	{ BUTTON_CreateIndirect,	"",							ID_BUTTON_CHAR_0 + 7,	MC_KEY_X + 7 * (MC_KEY_W + MC_KEY_GAP),	MC_KEY_Y1,	MC_KEY_W,	MC_KEY_H,	0,	0x0,	0 },
	{ BUTTON_CreateIndirect,	"",							ID_BUTTON_CHAR_0 + 8,	MC_KEY_X + 0 * (MC_KEY_W + MC_KEY_GAP),	MC_KEY_Y2,	MC_KEY_W,	MC_KEY_H,	0,	0x0,	0 },
	{ BUTTON_CreateIndirect,	"",							ID_BUTTON_CHAR_0 + 9,	MC_KEY_X + 1 * (MC_KEY_W + MC_KEY_GAP),	MC_KEY_Y2,	MC_KEY_W,	MC_KEY_H,	0,	0x0,	0 },
	{ BUTTON_CreateIndirect,	"",							ID_BUTTON_CHAR_0 + 10,	MC_KEY_X + 2 * (MC_KEY_W + MC_KEY_GAP),	MC_KEY_Y2,	MC_KEY_W,	MC_KEY_H,	0,	0x0,	0 },
	{ BUTTON_CreateIndirect,	"",							ID_BUTTON_CHAR_0 + 11,	MC_KEY_X + 3 * (MC_KEY_W + MC_KEY_GAP),	MC_KEY_Y2,	MC_KEY_W,	MC_KEY_H,	0,	0x0,	0 },
	{ BUTTON_CreateIndirect,	"",							ID_BUTTON_CHAR_0 + 12,	MC_KEY_X + 4 * (MC_KEY_W + MC_KEY_GAP),	MC_KEY_Y2,	MC_KEY_W,	MC_KEY_H,	0,	0x0,	0 },
	{ BUTTON_CreateIndirect,	"",							ID_BUTTON_CHAR_0 + 13,	MC_KEY_X + 5 * (MC_KEY_W + MC_KEY_GAP),	MC_KEY_Y2,	MC_KEY_W,	MC_KEY_H,	0,	0x0,	0 },
	{ BUTTON_CreateIndirect,	"",							ID_BUTTON_CHAR_0 + 14,	MC_KEY_X + 6 * (MC_KEY_W + MC_KEY_GAP),	MC_KEY_Y2,	MC_KEY_W,	MC_KEY_H,	0,	0x0,	0 },
	{ BUTTON_CreateIndirect,	"",							ID_BUTTON_CHAR_0 + 15,	MC_KEY_X + 7 * (MC_KEY_W + MC_KEY_GAP),	MC_KEY_Y2,	MC_KEY_W,	MC_KEY_H,	0,	0x0,	0 },

	// Action row - SPACE, DEL, CLEAR, SEND, and the two role buttons
	{ BUTTON_CreateIndirect,	"SPACE",					ID_BUTTON_SPACE,		10,		MC_ACT_Y,	170,	MC_ACT_H,	0,	0x0,	0 },
	{ BUTTON_CreateIndirect,	"DEL",						ID_BUTTON_BACKSPACE,	184,	MC_ACT_Y,	110,	MC_ACT_H,	0,	0x0,	0 },
	{ BUTTON_CreateIndirect,	"CLEAR",					ID_BUTTON_CLEAR,		298,	MC_ACT_Y,	110,	MC_ACT_H,	0,	0x0,	0 },
	{ BUTTON_CreateIndirect,	"SEND",						ID_BUTTON_SEND,			412,	MC_ACT_Y,	140,	MC_ACT_H,	0,	0x0,	0 },
	{ BUTTON_CreateIndirect,	"CALLER",					ID_BUTTON_CALLER,		556,	MC_ACT_Y,	115,	MC_ACT_H,	0,	0x0,	0 },
	{ BUTTON_CreateIndirect,	"PEER",						ID_BUTTON_PEER,			675,	MC_ACT_Y,	115,	MC_ACT_H,	0,	0x0,	0 },
};

//*----------------------------------------------------------------------------
//* Function Name       : mc_ui_add_line
//* Object              : append a row to the message list, dropping the
//*						: oldest once the ring is full
//* Context    			: CONTEXT_VIDEO (gui task)
//*----------------------------------------------------------------------------
static void mc_ui_add_line(uint8_t kind, const char *text, const char *meta)
{
	RTC_TimeTypeDef	tm = {0};
	RTC_DateTypeDef	dt = {0};
	MC_UI_LINE		*l;
	int				i;

	k_GetTime(&tm);
	k_GetDate(&dt);

	if(mc_ui_line_count >= MC_UI_LINE_CAP)
	{
		for(i = 0; i < (MC_UI_LINE_CAP - 1); i++)
			mc_ui_lines[i] = mc_ui_lines[i + 1];

		mc_ui_line_count = MC_UI_LINE_CAP - 1;
	}

	l = &mc_ui_lines[mc_ui_line_count++];

	memset(l, 0, sizeof(MC_UI_LINE));
	l->kind = kind;

	snprintf(l->time, sizeof(l->time), "%02d:%02d", tm.Hours, tm.Minutes);
	strncpy(l->text, text, sizeof(l->text) - 1);

	if(meta != NULL)
		strncpy(l->meta, meta, sizeof(l->meta) - 1);
}

//*----------------------------------------------------------------------------
//* Function Name       : mc_ui_drain_rx
//* Object              : pull decoded frames and session notices queued by
//*						: the marschat task into the message list
//* Notes    			: returns nonzero when something was added
//* Context    			: CONTEXT_VIDEO (gui task, WM_TIMER poll)
//*----------------------------------------------------------------------------
static int mc_ui_drain_rx(void)
{
	xQueueHandle	q = marschat_rx_queue();
	MC_UI_RX_MSG	m;
	char			meta[12];
	int				got = 0;

	if(q == NULL)
		return 0;

	while(xQueueReceive(q, &m, 0) == pdPASS)
	{
		got = 1;

		if(m.kind == MC_UI_LINE_INFO)
		{
			mc_ui_add_line(MC_UI_LINE_INFO, m.text, NULL);
			continue;
		}

		mc_ui_last_snr	= m.snr;
		mc_ui_last_dt	= m.dt_cs;
		mc_ui_have_rx	= 1;

		snprintf(meta, sizeof(meta), "%d dB", m.snr);
		mc_ui_add_line(MC_UI_LINE_RX, m.text, meta);
	}

	return got;
}

//*----------------------------------------------------------------------------
//* Function Name       : mc_ui_paint_title
//* Object              : title strip - name, dial frequency, wall clock
//* Notes    			: the clock ticks, so this is its own child window
//*						: and the panels below it stay untouched
//* Context    			: CONTEXT_VIDEO (gui task, WM_PAINT)
//*----------------------------------------------------------------------------
static void mc_ui_paint_title(void)
{
	MC_UI_STATUS	st;
	RTC_TimeTypeDef	tm = {0};
	RTC_DateTypeDef	dt = {0};
	char			buf[32];
	int				x;

	marschat_get_status(&st);

	k_GetTime(&tm);
	k_GetDate(&dt);

	atlas_background(0, 0, MC_UI_W, MC_TITLE_H);

	GUI_SetTextMode(GUI_TM_TRANS);

	// Lime chevron, as in the reference's corner marks
	GUI_SetColor(ATLAS_LIME);
	GUI_FillRect(10, 8, 12, 22);
	GUI_FillRect(10, 20, 22, 22);

	GUI_SetFont(&GUI_Font24B_1);
	GUI_SetColor(GUI_WHITE);
	x = atlas_text(30, 4, "MARSCHAT", 6);

	atlas_ticks(x + 16, 12, 140, 6, 9, 2, ATLAS_CYAN_DEEP);
	atlas_ticks(x + 16, 22, 110, 3, 14, 2, ATLAS_AMBER_DEEP);

	// Clock right, dial frequency just left of it
	snprintf(buf, sizeof(buf), "%02d:%02d:%02d", tm.Hours, tm.Minutes, tm.Seconds);
	GUI_SetColor(ATLAS_AMBER);
	atlas_text_right(MC_UI_W - 10, 4, buf, 2);

	snprintf(buf, sizeof(buf), "%u.%06u MHZ", (unsigned)(st.dial_hz / 1000000),
			(unsigned)(st.dial_hz % 1000000));
	GUI_SetFont(&GUI_Font16B_1);
	GUI_SetColor(ATLAS_CYAN);
	atlas_text_right(MC_UI_W - 150, 8, buf, 1);

	GUI_SetColor(ATLAS_LINE);
	GUI_DrawHLine(MC_TITLE_H - 1, 0, MC_UI_W - 1);
}

//*----------------------------------------------------------------------------
//* Function Name       : mc_ui_paint_slot
//* Object              : the slot bar - role, what this slot is doing and
//*						: the countdown to the next one, plus the burst
//*						: progress hairline underneath
//* Context    			: CONTEXT_VIDEO (gui task, WM_PAINT)
//*----------------------------------------------------------------------------
static void mc_ui_paint_slot(void)
{
	MC_UI_STATUS	st;
	char			buf[48];
	const char		*doing;
	GUI_COLOR		c0, c1;

	marschat_get_status(&st);

	atlas_background(0, 0, MC_SLOT_W, MC_SLOT_WIN_H);

	// Left cap, role block, state block, end cap
	GUI_SetColor(ATLAS_LIME);
	GUI_FillRect(0, 0, 6, MC_SLOT_H - 1);

	if(st.state == MC_SESS_OFF)
	{
		atlas_bar(10, 0, 250, MC_SLOT_H, ATLAS_LINE, ATLAS_CYAN_DEEP);
		atlas_bar(263, 0, 508, MC_SLOT_H, ATLAS_PANEL, ATLAS_PANEL);

		GUI_SetTextMode(GUI_TM_TRANS);
		GUI_SetFont(&GUI_Font20B_1);
		GUI_SetColor(ATLAS_TEXT);
		atlas_text(24, 9, "NO SESSION", 3);

		GUI_SetColor(ATLAS_DIM);
		atlas_text_right(MC_SLOT_W - 24, 9, "SEND STARTS ONE", 3);

		GUI_SetColor(ATLAS_LINE);
		GUI_FillRect(MC_SLOT_W - 8, 0, MC_SLOT_W - 1, MC_SLOT_H - 1);
		return;
	}

	if(st.state == MC_SESS_LOST)
	{
		atlas_bar(10, 0, 250, MC_SLOT_H, ATLAS_AMBER_DEEP, ATLAS_AMBER_DEEP);
		atlas_bar(263, 0, 508, MC_SLOT_H, ATLAS_PANEL, ATLAS_PANEL);

		GUI_SetTextMode(GUI_TM_TRANS);
		GUI_SetFont(&GUI_Font20B_1);
		GUI_SetColor(ATLAS_TEXT);
		atlas_text(24, 9, "LINK LOST", 3);

		GUI_SetColor(ATLAS_AMBER);
		atlas_text_right(MC_SLOT_W - 24, 9, "PEER SILENT - SEND TO RETRY", 3);

		GUI_SetColor(ATLAS_AMBER);
		GUI_FillRect(MC_SLOT_W - 8, 0, MC_SLOT_W - 1, MC_SLOT_H - 1);
		return;
	}

	// Active session: role on the cyan cap, slot state on the right
	atlas_bar(10, 0, 250, MC_SLOT_H, ATLAS_CYAN_HI, ATLAS_CYAN);

	if(st.tx_busy)
	{
		doing = "TX RUNNING";
		c0    = ATLAS_AMBER;
		c1    = ATLAS_AMBER_DEEP;
	}
	else if(st.rx_active)
	{
		doing = "RX CAPTURE";
		c0    = ATLAS_CYAN;
		c1    = ATLAS_CYAN_DEEP;
	}
	else
	{
		doing = st.now_is_ours ? "OUR SLOT" : "PEER SLOT";
		c0    = ATLAS_CYAN_DEEP;
		c1    = ATLAS_PANEL;
	}

	atlas_bar(263, 0, 508, MC_SLOT_H, c0, c1);

	GUI_SetTextMode(GUI_TM_TRANS);
	GUI_SetFont(&GUI_Font20B_1);

	GUI_SetColor(ATLAS_INK);
	atlas_text(24, 9, (st.role == MC_ROLE_CALLER) ? "CALLER" : "PEER", 4);

	GUI_SetColor(st.tx_busy ? ATLAS_INK : ATLAS_TEXT);
	snprintf(buf, sizeof(buf), "%s   %s IN %ds", doing,
			st.next_is_ours ? "TX" : "RX", st.secs_to_slot);
	atlas_text_right(MC_SLOT_W - 24, 9, buf, 3);

	GUI_SetColor(ATLAS_LIME);
	GUI_FillRect(MC_SLOT_W - 8, 0, MC_SLOT_W - 1, MC_SLOT_H - 1);

	// Burst progress - only while the exciter is keyed
	if(st.tx_busy)
		atlas_track(0, MC_BURST_Y, MC_SLOT_W, MC_BURST_H,
					(MC_SLOT_W * st.tx_progress) / 100, ATLAS_LINE_OFF, ATLAS_AMBER);
}

//*----------------------------------------------------------------------------
//* Function Name       : mc_ui_paint_history
//* Object              : bracketed message list
//* Context    			: CONTEXT_VIDEO (gui task, WM_PAINT)
//*----------------------------------------------------------------------------
static void mc_ui_paint_history(void)
{
	int	first, i, y;

	atlas_panel(MC_HIST_X, MC_HIST_Y, MC_HIST_W, MC_HIST_H, 1);

	GUI_SetTextMode(GUI_TM_TRANS);

	first = (int)mc_ui_line_count - MC_HIST_ROWS;
	if(first < 0)
		first = 0;

	for(i = first; i < (int)mc_ui_line_count; i++)
	{
		MC_UI_LINE	*l = &mc_ui_lines[i];
		GUI_COLOR	col;
		const char	*tag;
		int			row = i - first;

		y = MC_HIST_Y + 4 + row * MC_HIST_ROW_H;

		if(row & 1)
		{
			GUI_SetColor(ATLAS_ROW);
			GUI_FillRect(MC_HIST_X + 4, y, MC_HIST_X + MC_HIST_W - 5, y + MC_HIST_ROW_H - 2);
		}

		// Timestamp
		GUI_SetFont(&GUI_Font13B_1);
		GUI_SetColor(ATLAS_DIM);
		atlas_text(MC_HIST_X + 10, y + 5, l->time, 0);

		// Direction chip
		if(l->kind == MC_UI_LINE_RX)
		{
			col = ATLAS_CYAN;
			tag = "RX";
		}
		else if(l->kind == MC_UI_LINE_INFO)
		{
			col = ATLAS_DIM;
			tag = "--";
		}
		else
		{
			col = ATLAS_AMBER;
			tag = "TX";
		}

		atlas_chip(MC_HIST_X + 52, y + 3, 30, 16, tag, col, (l->kind == MC_UI_LINE_RX));

		// Text and the right hand column
		GUI_SetFont(&GUI_Font16B_1);
		GUI_SetColor((l->kind == MC_UI_LINE_INFO) ? ATLAS_DIM : ATLAS_TEXT);
		atlas_text(MC_HIST_X + 92, y + 3, l->text, 1);

		if(l->meta[0] != 0)
		{
			GUI_SetFont(&GUI_Font13B_1);
			GUI_SetColor(ATLAS_DIM);
			atlas_text_right(MC_HIST_X + MC_HIST_W - 12, y + 5, l->meta, 0);
		}
	}
}

//*----------------------------------------------------------------------------
//* Function Name       : mc_ui_telem_cell
//* Object              : one label/value pair of the telemetry grid
//* Context    			: CONTEXT_VIDEO (gui task, WM_PAINT)
//*----------------------------------------------------------------------------
static void mc_ui_telem_cell(int x, int y, const char *label, const char *value, GUI_COLOR col)
{
	GUI_SetFont(&GUI_Font13B_1);
	GUI_SetColor(ATLAS_DIM);
	atlas_text(x, y, label, 2);

	GUI_SetFont(&GUI_Font20B_1);
	GUI_SetColor(col);
	atlas_text(x, y + 15, value, 1);
}

//*----------------------------------------------------------------------------
//* Function Name       : mc_ui_paint_telemetry
//* Object              : sequence/ack/queue state, and the slot map with
//*						: the outcome of the last peer slots
//* Context    			: CONTEXT_VIDEO (gui task, WM_PAINT)
//*----------------------------------------------------------------------------
static void mc_ui_paint_telemetry(void)
{
	MC_UI_STATUS	st;
	char			buf[16];
	int				x0 = MC_TEL_X + 12;
	int				x1 = MC_TEL_X + 160;
	int				y  = MC_TEL_Y + 8;

	marschat_get_status(&st);

	atlas_panel(MC_TEL_X, MC_TEL_Y, MC_TEL_W, MC_TEL_H, 1);

	GUI_SetTextMode(GUI_TM_TRANS);

	snprintf(buf, sizeof(buf), "%d", st.tx_seq);
	mc_ui_telem_cell(x0, y, "TX SEQ", buf, ATLAS_AMBER);

	snprintf(buf, sizeof(buf), "%d", st.last_rx_seq);
	mc_ui_telem_cell(x1, y, "ACK SENT", buf, ATLAS_AMBER);

	y += 42;

	if(st.pending)
		snprintf(buf, sizeof(buf), "%d / %d", st.retries, MC_ARQ_RETRIES);
	else
		snprintf(buf, sizeof(buf), "-");

	mc_ui_telem_cell(x0, y, "TRIES", buf, st.pending ? ATLAS_AMBER : ATLAS_DIM);

	snprintf(buf, sizeof(buf), "%d", st.queued_chunks);
	mc_ui_telem_cell(x1, y, "QUEUED", buf, st.queued_chunks ? ATLAS_CYAN : ATLAS_DIM);

	y += 42;

	if(mc_ui_have_rx)
		snprintf(buf, sizeof(buf), "%d dB", mc_ui_last_snr);
	else
		snprintf(buf, sizeof(buf), "-");

	mc_ui_telem_cell(x0, y, "LAST SNR", buf, mc_ui_have_rx ? ATLAS_CYAN : ATLAS_DIM);

	if(mc_ui_have_rx)
	{
		int	frac = mc_ui_last_dt % 100;

		if(frac < 0)
			frac = -frac;

		snprintf(buf, sizeof(buf), "%d.%02d s", mc_ui_last_dt / 100, frac);
	}
	else
		snprintf(buf, sizeof(buf), "-");

	mc_ui_telem_cell(x1, y, "LAST DT", buf, mc_ui_have_rx ? ATLAS_CYAN : ATLAS_DIM);

	// Slot map: the 120 s slot, burst against the gap that carries the
	// decode and the CW id
	y = MC_TEL_Y + MC_TEL_H - 40;

	GUI_SetFont(&GUI_Font13B_1);
	GUI_SetColor(ATLAS_DIM);
	atlas_text(x0, y - 16, "SLOT 120 s", 2);

	GUI_SetColor(ATLAS_LINE_OFF);
	GUI_FillRect(x0, y, MC_TEL_X + MC_TEL_W - 13, y + 9);

	GUI_SetColor(ATLAS_CYAN_DEEP);
	GUI_FillRect(x0, y, x0 + (((MC_TEL_W - 25) * 111) / 120), y + 9);

	GUI_SetColor(ATLAS_LIME);
	GUI_FillRect(x0 + (((MC_TEL_W - 25) * 111) / 120) + 1, y, MC_TEL_X + MC_TEL_W - 13, y + 9);
}

//*----------------------------------------------------------------------------
//* Function Name       : mc_ui_paint_compose
//* Object              : the draft, split into the part already handed to
//*						: the tx queue and the part still waiting
//* Context    			: CONTEXT_VIDEO (gui task, WM_PAINT)
//*----------------------------------------------------------------------------
static void mc_ui_paint_compose(void)
{
	MC_UI_STATUS	st;
	char			buf[MC_UI_COMPOSE_MAX + 1];
	char			meta[64];
	int				committed, x;

	marschat_get_status(&st);

	GUI_SetColor(ATLAS_PANEL);
	GUI_FillRect(MC_COMP_X, MC_COMP_Y, MC_COMP_X + MC_COMP_W - 1, MC_COMP_Y + MC_COMP_H - 1);

	GUI_SetColor(ATLAS_CYAN);
	GUI_FillRect(MC_COMP_X, MC_COMP_Y, MC_COMP_X + 2, MC_COMP_Y + MC_COMP_H - 1);

	GUI_SetTextMode(GUI_TM_TRANS);

	GUI_SetFont(&GUI_Font13B_1);
	GUI_SetColor(ATLAS_DIM);
	atlas_text(MC_COMP_X + 12, MC_COMP_Y + 9, "TX", 3);

	committed = mc_ui_compose_len - (int)st.queued_chunks * MC_PAYLOAD_CHARS;
	if(committed < 0)
		committed = 0;
	if(committed > mc_ui_compose_len)
		committed = mc_ui_compose_len;

	GUI_SetFont(&GUI_Font20B_1);

	memcpy(buf, mc_ui_compose, (size_t)committed);
	buf[committed] = 0;

	GUI_SetColor(ATLAS_TEXT);
	x = atlas_text(MC_COMP_X + 44, MC_COMP_Y + 5, buf, 2);

	strncpy(buf, mc_ui_compose + committed, sizeof(buf) - 1);
	buf[sizeof(buf) - 1] = 0;

	GUI_SetColor(ATLAS_DIM);
	atlas_text(x, MC_COMP_Y + 5, buf, 2);

	GUI_SetFont(&GUI_Font13B_1);
	GUI_SetColor(ATLAS_DIM);

	if(mc_ui_sending)
		snprintf(meta, sizeof(meta), "SENDING - %d CHUNKS LEFT", st.queued_chunks + (st.pending ? 1 : 0));
	else
		snprintf(meta, sizeof(meta), "%d CHARS - %d CHUNKS - %d MIN",
				mc_ui_compose_len,
				(mc_ui_compose_len + MC_PAYLOAD_CHARS - 1) / MC_PAYLOAD_CHARS,
				((mc_ui_compose_len + MC_PAYLOAD_CHARS - 1) / MC_PAYLOAD_CHARS) * 4);

	atlas_text_right(MC_COMP_X + MC_COMP_W - 12, MC_COMP_Y + 9, meta, 2);
}

//*----------------------------------------------------------------------------
//* Function Name       : mc_ui_button_skin
//* Object              : atlas paint for every button on this screen -
//*						: emWin keeps the hit testing, we take the pixels
//* Context    			: CONTEXT_VIDEO (gui task, widget paint)
//*----------------------------------------------------------------------------
static int mc_ui_button_skin(const WIDGET_ITEM_DRAW_INFO *pDrawItemInfo)
{
	WM_HWIN			hObj = pDrawItemInfo->hWin;
	MC_UI_STATUS	st;
	char			text[16];
	GUI_COLOR		face, edge, ink;
	int				id, w, h, tw;
	int				pressed, enabled, active = 0;

	if(pDrawItemInfo->Cmd != WIDGET_ITEM_DRAW_BACKGROUND)
		return 0;									// text and focus are ours too, drawn below

	id      = WM_GetId(hObj);
	w       = WM_GetWindowSizeX(hObj);
	h       = WM_GetWindowSizeY(hObj);
	pressed = (int)BUTTON_IsPressed(hObj);
	enabled = WM_IsEnabled(hObj);

	BUTTON_GetText(hObj, text, sizeof(text));

	marschat_get_status(&st);

	// Which role button is currently the live one
	if((id == ID_BUTTON_CALLER) || (id == ID_BUTTON_PEER))
	{
		uint8_t want = (id == ID_BUTTON_CALLER) ? MC_ROLE_CALLER : MC_ROLE_PEER;

		active = ((st.state == MC_SESS_ACTIVE) && (st.role == want)) ? 1 : 0;
	}

	// Face colours per button family
	if(id == ID_BUTTON_SEND)
	{
		face = ATLAS_CYAN;
		edge = ATLAS_CYAN_HI;
		ink  = ATLAS_INK;
	}
	else if(active)
	{
		face = ATLAS_AMBER;
		edge = ATLAS_AMBER;
		ink  = ATLAS_INK;
	}
	else if((id == ID_BUTTON_CALLER) || (id == ID_BUTTON_PEER))
	{
		face = ATLAS_PANEL;
		edge = ATLAS_AMBER;
		ink  = ATLAS_AMBER;
	}
	else
	{
		face = ATLAS_PANEL;
		edge = ATLAS_LINE;
		ink  = ATLAS_CYAN_HI;
	}

	if(!enabled)
	{
		face = ATLAS_GROUND;
		edge = ATLAS_LINE_OFF;
		ink  = ATLAS_OFF;
	}

	if(pressed && enabled)
	{
		// Pressed reads as the face lighting up
		GUI_SetColor((id == ID_BUTTON_SEND) ? ATLAS_CYAN_HI : ATLAS_BAND);
		GUI_FillRect(0, 0, w - 1, h - 1);
		ink = (id == ID_BUTTON_SEND) ? ATLAS_INK : GUI_WHITE;
	}
	else if((id == ID_BUTTON_SEND) || active)
	{
		atlas_bar(0, 0, w, h, (id == ID_BUTTON_SEND) ? ATLAS_CYAN_HI : ATLAS_AMBER, face);
	}
	else
	{
		atlas_bar(0, 0, w, h, ATLAS_BAND, face);
	}

	GUI_SetColor(edge);
	GUI_DrawRect(0, 0, w - 1, h - 1);

	// Label - keys get the big font, the action row the tracked one
	GUI_SetTextMode(GUI_TM_TRANS);

	if((id >= ID_BUTTON_CHAR_0) && (id < (ID_BUTTON_CHAR_0 + 16)))
	{
		GUI_SetFont(&GUI_Font24B_1);
		tw = atlas_text_width(text, 0);
		GUI_SetColor(ink);
		atlas_text((w - tw) / 2, (h - GUI_GetFontSizeY()) / 2, text, 0);
	}
	else
	{
		GUI_SetFont(&GUI_Font16B_1);
		tw = atlas_text_width(text, 3);
		GUI_SetColor(ink);
		atlas_text((w - tw) / 2, (h - GUI_GetFontSizeY()) / 2, text, 3);
	}

	return 0;
}

//*----------------------------------------------------------------------------
//* Function Name       : mc_ui_set_input_enabled
//* Object              : lock/unlock the keys while a message drains out
//* Context    			: CONTEXT_VIDEO (gui task)
//*----------------------------------------------------------------------------
static void mc_ui_set_input_enabled(WM_HWIN hWin, int enabled)
{
	static const int ids[] =
	{
		ID_BUTTON_CHAR_0 + 0,  ID_BUTTON_CHAR_0 + 1,  ID_BUTTON_CHAR_0 + 2,  ID_BUTTON_CHAR_0 + 3,
		ID_BUTTON_CHAR_0 + 4,  ID_BUTTON_CHAR_0 + 5,  ID_BUTTON_CHAR_0 + 6,  ID_BUTTON_CHAR_0 + 7,
		ID_BUTTON_CHAR_0 + 8,  ID_BUTTON_CHAR_0 + 9,  ID_BUTTON_CHAR_0 + 10, ID_BUTTON_CHAR_0 + 11,
		ID_BUTTON_CHAR_0 + 12, ID_BUTTON_CHAR_0 + 13, ID_BUTTON_CHAR_0 + 14, ID_BUTTON_CHAR_0 + 15,
		ID_BUTTON_SPACE, ID_BUTTON_BACKSPACE, ID_BUTTON_CLEAR, ID_BUTTON_SEND
	};
	int	i;

	for(i = 0; i < (int)GUI_COUNTOF(ids); i++)
	{
		WM_HWIN	hItem = WM_GetDialogItem(hWin, ids[i]);

		if(enabled)
			WM_EnableWindow(hItem);
		else
			WM_DisableWindow(hItem);
	}
}

//*----------------------------------------------------------------------------
//* Function Name       : mc_ui_invalidate_all
//* Object              : repaint the whole screen - dialog, the two child
//*						: windows and every button
//* Notes    			: the buttons are skinned, so their faces carry
//*						: enabled state that a plain dialog invalidate
//*						: would not reach
//* Context    			: CONTEXT_VIDEO (gui task)
//*----------------------------------------------------------------------------
static void mc_ui_invalidate_all(WM_HWIN hWin)
{
	int	i;

	WM_InvalidateWindow(hWin);

	if(hMcTitle)
		WM_InvalidateWindow(hMcTitle);

	if(hMcSlot)
		WM_InvalidateWindow(hMcSlot);

	for(i = 0; i < 16; i++)
		WM_InvalidateWindow(WM_GetDialogItem(hWin, ID_BUTTON_CHAR_0 + i));

	WM_InvalidateWindow(WM_GetDialogItem(hWin, ID_BUTTON_SPACE));
	WM_InvalidateWindow(WM_GetDialogItem(hWin, ID_BUTTON_BACKSPACE));
	WM_InvalidateWindow(WM_GetDialogItem(hWin, ID_BUTTON_CLEAR));
	WM_InvalidateWindow(WM_GetDialogItem(hWin, ID_BUTTON_SEND));
	WM_InvalidateWindow(WM_GetDialogItem(hWin, ID_BUTTON_CALLER));
	WM_InvalidateWindow(WM_GetDialogItem(hWin, ID_BUTTON_PEER));
}

//*----------------------------------------------------------------------------
//* Function Name       : mc_ui_model_changed
//* Object              : has anything the panels draw from moved ?
//* Context    			: CONTEXT_VIDEO (gui task, WM_TIMER poll)
//*----------------------------------------------------------------------------
static int mc_ui_model_changed(void)
{
	MC_UI_STATUS	st;
	MC_UI_MODEL		m;

	marschat_get_status(&st);

	memset(&m, 0, sizeof(m));
	m.lines			= mc_ui_line_count;
	m.state			= st.state;
	m.role			= st.role;
	m.pending		= st.pending;
	m.retries		= st.retries;
	m.tx_seq		= st.tx_seq;
	m.last_rx_seq	= st.last_rx_seq;
	m.queued		= st.queued_chunks;
	m.compose_len	= mc_ui_compose_len;
	m.sending		= mc_ui_sending;

	if(memcmp(&m, &mc_ui_model, sizeof(m)) == 0)
		return 0;

	mc_ui_model = m;

	return 1;
}

//*----------------------------------------------------------------------------
//* Function Name       : _cbTitle / _cbSlot
//* Object              : the two child windows that tick
//* Context    			: CONTEXT_VIDEO (gui task)
//*----------------------------------------------------------------------------
static void _cbTitle(WM_MESSAGE *pMsg)
{
	if(pMsg->MsgId == WM_PAINT)
		mc_ui_paint_title();
	else
		WM_DefaultProc(pMsg);
}

static void _cbSlot(WM_MESSAGE *pMsg)
{
	if(pMsg->MsgId == WM_PAINT)
		mc_ui_paint_slot();
	else
		WM_DefaultProc(pMsg);
}

static void _cbControl(WM_MESSAGE * pMsg, int Id, int NCode)
{
	MC_UI_STATUS	st;

	if(NCode != WM_NOTIFICATION_RELEASED)
		return;

	if((Id >= ID_BUTTON_CHAR_0) && (Id < ID_BUTTON_CHAR_0 + 16))
	{
		if((!mc_ui_sending) && (mc_ui_compose_len < MC_UI_COMPOSE_MAX))
		{
			mc_ui_compose[mc_ui_compose_len++] = (char)('A' + (Id - ID_BUTTON_CHAR_0));
			mc_ui_compose[mc_ui_compose_len]   = 0;
		}
		return;
	}

	switch(Id)
	{
		case ID_BUTTON_SPACE:
		{
			if((!mc_ui_sending) && (mc_ui_compose_len < MC_UI_COMPOSE_MAX))
			{
				mc_ui_compose[mc_ui_compose_len++] = ' ';
				mc_ui_compose[mc_ui_compose_len]   = 0;
			}
			break;
		}

		case ID_BUTTON_BACKSPACE:
		{
			if((!mc_ui_sending) && (mc_ui_compose_len > 0))
				mc_ui_compose[--mc_ui_compose_len] = 0;

			break;
		}

		case ID_BUTTON_CLEAR:
		{
			if(!mc_ui_sending)
			{
				mc_ui_compose_len = 0;
				mc_ui_compose[0]  = 0;
			}
			break;
		}

		case ID_BUTTON_SEND:
		{
			uint8_t	err;

			if(mc_ui_sending || (mc_ui_compose_len == 0))
				break;

			// Sending is what starts a session - without one there are no
			// slots to transmit in. Requested before the text is queued so
			// the marschat task has the session up by the time it looks at
			// the queue
			marschat_get_status(&st);

			if(st.state != MC_SESS_ACTIVE)
				marschat_session_start(mc_ui_role);

			err = marschat_ui_send_text(mc_ui_compose);

			if((err == 0) || (err == 4))
			{
				mc_ui_add_line(MC_UI_LINE_TX, mc_ui_compose, NULL);
				mc_ui_sending = 1;
				mc_ui_set_input_enabled(pMsg->hWin, 0);
			}
			else
				mc_ui_add_line(MC_UI_LINE_INFO, "send queue full, try again", NULL);

			break;
		}

		case ID_BUTTON_CALLER:
		case ID_BUTTON_PEER:
		{
			uint8_t	want = (Id == ID_BUTTON_CALLER) ? MC_ROLE_CALLER : MC_ROLE_PEER;

			marschat_get_status(&st);

			// Pressing the role we are already running stops the session
			if((st.state == MC_SESS_ACTIVE) && (st.role == want))
			{
				marschat_session_stop();
				break;
			}

			mc_ui_role = want;
			marschat_session_start(want);
			break;
		}

		default:
			break;
	}

	WM_InvalidateWindow(pMsg->hWin);
}

static void _cbDialog(WM_MESSAGE * pMsg)
{
	WM_HWIN	hItem;
	int		Id, NCode;
	int		i;

	switch (pMsg->MsgId)
	{
		case WM_INIT_DIALOG:
		{
			for(i = 0; i < 16; i++)
			{
				char	label[2];

				label[0] = (char)('A' + i);
				label[1] = 0;

				hItem = WM_GetDialogItem(pMsg->hWin, ID_BUTTON_CHAR_0 + i);
				BUTTON_SetText(hItem, label);
				BUTTON_SetSkin(hItem, mc_ui_button_skin);
			}

			{
				static const int ids[] =
				{
					ID_BUTTON_SPACE, ID_BUTTON_BACKSPACE, ID_BUTTON_CLEAR,
					ID_BUTTON_SEND, ID_BUTTON_CALLER, ID_BUTTON_PEER
				};

				for(i = 0; i < (int)GUI_COUNTOF(ids); i++)
					BUTTON_SetSkin(WM_GetDialogItem(pMsg->hWin, ids[i]), mc_ui_button_skin);
			}

			// The widgets are rebuilt on every entry, the conversation is
			// not: F5 toggles this screen away and back in the middle of
			// a session that runs for as long as the chat does, so the
			// history, the draft and the last rx telemetry all live in
			// statics that deliberately survive it. Only the snapshot is
			// reset, to force the first repaint
			memset(&mc_ui_model, 0xFF, sizeof(mc_ui_model));

			// Re-derive rather than assume: a message queued before the
			// screen was toggled away is still draining, and the input
			// has to come back locked
			{
				MC_UI_STATUS	st;

				marschat_get_status(&st);

				mc_ui_sending = ((st.tx_busy) || (st.pending) || (st.queued_chunks > 0)) ? 1 : 0;
			}

			// The two regions that tick get their own windows so the rest
			// of the screen is not repainted twice a second
			hMcTitle = WM_CreateWindowAsChild(0, 0, MC_UI_W, MC_TITLE_H,
								pMsg->hWin, WM_CF_SHOW, _cbTitle, 0);
			hMcSlot  = WM_CreateWindowAsChild(MC_SLOT_X, MC_SLOT_Y, MC_SLOT_W, MC_SLOT_WIN_H,
								pMsg->hWin, WM_CF_SHOW, _cbSlot, 0);

			hMcTimer = WM_CreateTimer(pMsg->hWin, 0, 500, 0);

			mc_ui_set_input_enabled(pMsg->hWin, !mc_ui_sending);
			break;
		}

		case WM_PAINT:
		{
			atlas_background(0, MC_TITLE_H, MC_UI_W, MC_UI_H - MC_TITLE_H);

			mc_ui_paint_history();
			mc_ui_paint_telemetry();
			mc_ui_paint_compose();
			break;
		}

		case WM_TIMER:
		{
			MC_UI_STATUS	st;
			int				dirty;

			dirty = mc_ui_drain_rx();

			// Message fully drained - clear the draft and unlock input.
			// In a session the last chunk is only really gone once the
			// peer has acknowledged it
			marschat_get_status(&st);

			if((mc_ui_sending) && (!st.tx_busy) && (!st.pending) && (st.queued_chunks == 0))
			{
				mc_ui_sending		= 0;
				mc_ui_compose_len	= 0;
				mc_ui_compose[0]	= 0;

				mc_ui_set_input_enabled(pMsg->hWin, 1);
				dirty = 1;
			}

			if(mc_ui_model_changed())
				dirty = 1;

			if(dirty)
			{
				mc_ui_invalidate_all(pMsg->hWin);
			}
			else
			{
				// The clock and the countdown always move
				WM_InvalidateWindow(hMcTitle);
				WM_InvalidateWindow(hMcSlot);
			}

			WM_RestartTimer(pMsg->Data.v, 500);
			break;
		}

		case WM_DELETE:
		{
			WM_DeleteTimer(hMcTimer);

			// Children go with the dialog - drop the handles so a
			// later repaint cannot reach a dead window
			hMcTitle = 0;
			hMcSlot  = 0;
			break;
		}

		case WM_NOTIFY_PARENT:
		{
			Id    = WM_GetId(pMsg->hWinSrc);
			NCode = pMsg->Data.v;

			_cbControl(pMsg, Id, NCode);
			break;
		}

		case WM_KEY:
		{
			switch (((WM_KEY_INFO*)(pMsg->Data.p))->Key)
			{
				// Back to the radio. Scheduled, not a direct call - the
				// mode switch is what deletes this dialog
				case GUI_KEY_HOME:
					ui_s.req_state = MODE_DESKTOP;
					xTaskNotify(ps.hUiTask, UI_NEW_MODE_EVENT, eSetValueWithOverwrite);
					break;
			}
			break;
		}

		default:
			WM_DefaultProc(pMsg);
			break;
	}
}

//*----------------------------------------------------------------------------
//* Function Name       : _cbBkWindow
//* Object              : desktop window behind the dialog - the ground
//*						: colour shows for the one frame before the dialog
//*						: paints, and wherever the dialog does not reach
//* Context    			: CONTEXT_VIDEO (gui task)
//*----------------------------------------------------------------------------
static void _cbBkWindow(WM_MESSAGE *pMsg)
{
	switch(pMsg->MsgId)
	{
		case WM_PAINT:
			// Not GUI_Clear() - the driver runs in LCD_DRAWMODE_TRANS
			// (ui_proc.c, GUI_SetDrawMode) where a clear does nothing
			atlas_background(0, 0, MC_UI_W, MC_UI_H);
			break;

		default:
			WM_DefaultProc(pMsg);
			break;
	}
}

//*----------------------------------------------------------------------------
//* Function Name       : mc_ui_set_profile
//* Object              : emWin defaults this screen needs
//* Notes    			: the menu profile leaves WINDOW_SetDefaultBkColor
//*						: at GUI_WHITE (ui_menu.c) and it is a sticky
//*						: global, so a dialog created after the menu has
//*						: ever been entered paints a white face under our
//*						: own drawing. FT8 overrides it the same way
//* Context    			: CONTEXT_VIDEO (gui task)
//*----------------------------------------------------------------------------
static void mc_ui_set_profile(void)
{
	WINDOW_SetDefaultBkColor(ATLAS_GROUND);
}

//*----------------------------------------------------------------------------
//* Function Name       : marschat_ui_create
//* Object              : bring the screen up - called by the UI mode
//*						: switch on entry to MODE_DESKTOP_MARSCHAT
//* Context    			: CONTEXT_VIDEO (gui task)
//*----------------------------------------------------------------------------
void marschat_ui_create(void)
{
	mc_ui_set_profile();

	WM_SetCallback(WM_HBKWIN, &_cbBkWindow);

	hMcDialog = GUI_CreateDialogBox(_aDialog, GUI_COUNTOF(_aDialog), _cbDialog, 0, 0, 0);
}

//*----------------------------------------------------------------------------
//* Function Name       : marschat_ui_destroy
//* Object              : tear the screen down on the way back to the
//*						: desktop. Safe to call when it was never up
//* Context    			: CONTEXT_VIDEO (gui task)
//*----------------------------------------------------------------------------
void marschat_ui_destroy(void)
{
	if(hMcDialog)
	{
		WM_SetCallback		(WM_HBKWIN, 0);
		WM_InvalidateWindow	(WM_HBKWIN);

		WM_DeleteWindow(hMcDialog);

		hMcDialog = 0;
	}
}
