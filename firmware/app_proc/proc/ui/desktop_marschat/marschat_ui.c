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
// Collapsible keyboard: the 26 character keys and the shift key live in
// hMcKeyboard, a child window that slides up from below the screen on
// compose-bar tap and slides back down after SEND or an explicit hide.
// The action row (SPACE .. PEER) stays on hMcDialog so the session
// controls are always reachable; SPACE/DEL/CLEAR are hidden while the
// keyboard is off-screen, and SEND/CALLER/PEER spread wider
static WM_HWIN				hMcKeyboard;
static WM_HWIN				hMcCharKeys[MC_KEY_CHARS];
static WM_HWIN				hMcShiftKey;
static WM_HWIN				hMcTypeBtn;				// "TYPE" button, shown when kb hidden
static uint8_t				mc_ui_kb_shown = 0;

// Slide animation state
static GUI_ANIM_HANDLE		hMcAnim;

typedef struct
{
	WM_HWIN	hWin;
	int		dir;								// 0 = show (up), 1 = hide (down)
} MC_KB_ANIM;

static MC_KB_ANIM			mc_kb_anim;

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

// The two keyboard pages, each MC_KEY_CHARS long. The shift key flips
// between them; a char key's face and the character a press appends both
// come from the active page at the key's index (id offset from CHAR_0).
// Every glyph here must exist in mc_charset_latin so it can be encoded
static const char	mc_ui_page_letters[MC_KEY_CHARS + 1] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
static const char	mc_ui_page_symbols[MC_KEY_CHARS + 1] = "0123456789.,?!-/@:'()\"+=&;";

static uint8_t		mc_ui_shift = 0;			// 0 letters page, 1 symbols page

static const char *mc_ui_page(void)
{
	return mc_ui_shift ? mc_ui_page_symbols : mc_ui_page_letters;
}

// ---------------------------------------------------------------------
// Keyboard slide animation - same API pattern as c_keypad.c but the
// visual is the Atlas-themed keyboard, not the old Segger skin
//
// Dir 0 = slide up (show), 1 = slide down (hide).  The Pos parameter
// goes from 0 to GUI_ANIM_RANGE over MC_ANIM_TIME with ANIM_ACCELDECEL
// easing, and each step repositions hMcKeyboard via WM_MoveTo
static void mc_ui_anim_step(GUI_ANIM_INFO *pInfo, void *pVoid)
{
	MC_KB_ANIM	*p = (MC_KB_ANIM *)pVoid;
	int			y;

	if(p->dir)
	{
		// Hiding: MC_KB_Y → MC_UI_H (off the bottom)
		y = MC_KB_Y + (((MC_UI_H - MC_KB_Y) * pInfo->Pos) / GUI_ANIM_RANGE);
	}
	else
	{
		// Showing: MC_UI_H → MC_KB_Y
		y = MC_UI_H - (((MC_UI_H - MC_KB_Y) * pInfo->Pos) / GUI_ANIM_RANGE);
	}

	WM_MoveTo(p->hWin, 0, y);
}

static void mc_ui_anim_done(void *pVoid)
{
	(void)pVoid;
	hMcAnim = 0;

	// Final repaint to clean up any exposed region
	WM_InvalidateWindow(hMcDialog);
}

static void mc_ui_anim_start(int dir)
{
	mc_kb_anim.hWin = hMcKeyboard;
	mc_kb_anim.dir  = dir;

	hMcAnim = GUI_ANIM_Create(MC_ANIM_TIME, 10, &mc_kb_anim, 0);
	GUI_ANIM_AddItem(hMcAnim, 0, MC_ANIM_TIME, ANIM_ACCELDECEL, &mc_kb_anim, mc_ui_anim_step);
	GUI_ANIM_StartEx(hMcAnim, 1, mc_ui_anim_done);
}

// Layout the action row buttons for the current keyboard state. TYPE is
// always visible and toggles between "TYPE" (opens keyboard) and "HIDE"
// (closes it). When the keyboard is showing, SPACE/DEL/CLEAR appear and
// CALLER/PEER hide. When the keyboard is hidden, the reverse
static void mc_ui_layout_action_row(int kb_shown)
{
	WM_HWIN	hSpace  = WM_GetDialogItem(hMcDialog, ID_BUTTON_SPACE);
	WM_HWIN	hDel    = WM_GetDialogItem(hMcDialog, ID_BUTTON_BACKSPACE);
	WM_HWIN	hClear  = WM_GetDialogItem(hMcDialog, ID_BUTTON_CLEAR);
	WM_HWIN	hSend   = WM_GetDialogItem(hMcDialog, ID_BUTTON_SEND);
	WM_HWIN	hCaller = WM_GetDialogItem(hMcDialog, ID_BUTTON_CALLER);
	WM_HWIN	hPeer   = WM_GetDialogItem(hMcDialog, ID_BUTTON_PEER);

	if(kb_shown)
	{
		// Keyboard open: show text-entry buttons, hide session buttons
		WM_ShowWindow(hSpace);
		WM_ShowWindow(hDel);
		WM_ShowWindow(hClear);
		WM_HideWindow(hCaller);
		WM_HideWindow(hPeer);

		BUTTON_SetText(hMcTypeBtn, "HIDE");

		// [HIDE] [SPACE] [DEL] [CLEAR] [SEND]
		WM_SetWindowPos(hMcTypeBtn, 10,  MC_ACT_Y, 110, MC_ACT_H);
		WM_SetWindowPos(hSpace,     124, MC_ACT_Y, 200, MC_ACT_H);
		WM_SetWindowPos(hDel,       328, MC_ACT_Y, 110, MC_ACT_H);
		WM_SetWindowPos(hClear,     442, MC_ACT_Y, 110, MC_ACT_H);
		WM_SetWindowPos(hSend,      556, MC_ACT_Y, 234, MC_ACT_H);
	}
	else
	{
		// Keyboard hidden: hide text-entry buttons, show session buttons
		WM_HideWindow(hSpace);
		WM_HideWindow(hDel);
		WM_HideWindow(hClear);
		WM_ShowWindow(hCaller);
		WM_ShowWindow(hPeer);

		BUTTON_SetText(hMcTypeBtn, "TYPE");

		// [TYPE] [SEND] [CALLER] [PEER]
		WM_SetWindowPos(hMcTypeBtn, MC_FULL_TYPE_X,    MC_ACT_Y, MC_FULL_TYPE_W,   MC_ACT_H);
		WM_SetWindowPos(hSend,      MC_FULL_SEND_X,    MC_ACT_Y, MC_FULL_SEND_W,   MC_ACT_H);
		WM_SetWindowPos(hCaller,    MC_FULL_CALLER_X,  MC_ACT_Y, MC_FULL_CALLER_W, MC_ACT_H);
		WM_SetWindowPos(hPeer,      MC_FULL_PEER_X,    MC_ACT_Y, MC_FULL_PEER_W,   MC_ACT_H);
	}
}

static void mc_ui_show_keyboard(void)
{
	if(mc_ui_kb_shown || mc_ui_sending || hMcAnim)
		return;

	mc_ui_kb_shown = 1;

	mc_ui_layout_action_row(1);

	WM_InvalidateWindow(hMcDialog);
	mc_ui_anim_start(0);
}

static void mc_ui_hide_keyboard(void)
{
	if(!mc_ui_kb_shown || hMcAnim)
		return;

	mc_ui_kb_shown = 0;

	mc_ui_layout_action_row(0);

	WM_InvalidateWindow(hMcDialog);
	mc_ui_anim_start(1);
}

// The character keys are now created in a grid inside hMcKeyboard (see
// mc_ui_create_keys), so only the window and the fixed action row live
// in the static template
static const GUI_WIDGET_CREATE_INFO _aDialog[] =
{
	// -----------------------------------------------------------------------------------------------------------------------------
	//							name						id						x		y		xsize	ysize
	// -----------------------------------------------------------------------------------------------------------------------------
	{ WINDOW_CreateIndirect,	"", 						ID_WINDOW_0,			0,		0,		MC_UI_W,	MC_UI_H,	0,	0x64,	0 },

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
	int				is_key;

	if(pDrawItemInfo->Cmd != WIDGET_ITEM_DRAW_BACKGROUND)
		return 0;									// text and focus are ours too, drawn below

	id      = WM_GetId(hObj);
	w       = WM_GetWindowSizeX(hObj);
	h       = WM_GetWindowSizeY(hObj);
	pressed = (int)BUTTON_IsPressed(hObj);
	enabled = WM_IsEnabled(hObj);

	BUTTON_GetText(hObj, text, sizeof(text));

	// Keyboard character keys and shift keep the original skin
	is_key = ((id >= ID_BUTTON_CHAR_0) && (id < (ID_BUTTON_CHAR_0 + MC_KEY_CHARS)))
		   || (id == ID_BUTTON_SHIFT);

	if(is_key)
	{
		// ------- keyboard key: gradient fill, existing look -------
		if((id == ID_BUTTON_SHIFT))
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
			GUI_SetColor(ATLAS_BAND);
			GUI_FillRect(0, 0, w - 1, h - 1);
			ink = GUI_WHITE;
		}
		else
		{
			atlas_bar(0, 0, w, h, ATLAS_BAND, face);
		}

		GUI_SetColor(edge);
		GUI_DrawRect(0, 0, w - 1, h - 1);

		GUI_SetTextMode(GUI_TM_TRANS);
		GUI_SetFont(&GUI_Font24B_1);
		tw = atlas_text_width(text, 0);
		GUI_SetColor(ink);
		atlas_text((w - tw) / 2, (h - GUI_GetFontSizeY()) / 2, text, 0);
	}
	else
	{
		// ------- action row: atlas translucent panel style -------
		// Semi-transparent flat rectangle, no gradient, lighter
		// cyan text in large caps - matches the DEMODULATE /
		// TRIANGULATE buttons in the Atlas reference
		marschat_get_status(&st);

		if((id == ID_BUTTON_CALLER) || (id == ID_BUTTON_PEER))
		{
			uint8_t want = (id == ID_BUTTON_CALLER) ? MC_ROLE_CALLER : MC_ROLE_PEER;

			active = ((st.state == MC_SESS_ACTIVE) && (st.role == want)) ? 1 : 0;
		}

		if(id == ID_BUTTON_SEND)
		{
			face = ATLAS_CYAN_DEEP;
			edge = ATLAS_CYAN;
			ink  = ATLAS_CYAN_HI;
		}
		else if(active)
		{
			face = ATLAS_AMBER_DEEP;
			edge = ATLAS_AMBER;
			ink  = ATLAS_AMBER;
		}
		else
		{
			face = ATLAS_LINE_OFF;
			edge = ATLAS_LINE;
			ink  = ATLAS_CYAN;
		}

		if(!enabled)
		{
			face = ATLAS_GROUND;
			edge = ATLAS_LINE_OFF;
			ink  = ATLAS_OFF;
		}

		if(pressed && enabled)
		{
			// Brighten the fill, keep it flat
			GUI_SetColor(ATLAS_OFF);
			GUI_FillRect(0, 0, w - 1, h - 1);
			ink = ATLAS_CYAN_HI;
		}
		else
		{
			// Flat fill - no gradient
			GUI_SetColor(face);
			GUI_FillRect(0, 0, w - 1, h - 1);
		}

		GUI_SetColor(edge);
		GUI_DrawRect(0, 0, w - 1, h - 1);

		GUI_SetTextMode(GUI_TM_TRANS);
		GUI_SetFont(&GUI_Font16B_1);
		tw = atlas_text_width(text, 3);
		GUI_SetColor(ink);
		atlas_text((w - tw) / 2, (h - GUI_GetFontSizeY()) / 2, text, 3);
	}

	return 0;
}

//*----------------------------------------------------------------------------
//* Function Name       : mc_ui_apply_page
//* Object              : relabel the character keys for the active page and
//*						: set the shift key to what it switches to, then
//*						: repaint just the keyboard
//* Context    			: CONTEXT_VIDEO (gui task)
//*----------------------------------------------------------------------------
static void mc_ui_apply_page(void)
{
	const char	*page = mc_ui_page();
	int			i;

	for(i = 0; i < MC_KEY_CHARS; i++)
	{
		char	label[2];

		label[0] = page[i];
		label[1] = 0;

		BUTTON_SetText(hMcCharKeys[i], label);
		WM_InvalidateWindow(hMcCharKeys[i]);
	}

	// The shift face names the page it takes you TO, phone-keyboard style
	BUTTON_SetText(hMcShiftKey, mc_ui_shift ? "ABC" : "123");
	WM_InvalidateWindow(hMcShiftKey);
}

//*----------------------------------------------------------------------------
//* Function Name       : mc_ui_create_keys
//* Object              : build the 26 character keys plus the shift key as a
//*						: grid of skinned buttons, children of hMcKeyboard
//*						: (the sliding container, not the dialog). Coordinates
//*						: are relative to the container - column x is the same
//*						: because the container starts at x=0, row y is just
//*						: the row index times the pitch
//* Context    			: CONTEXT_VIDEO (gui task, WM_INIT_DIALOG)
//*----------------------------------------------------------------------------
static void mc_ui_create_keys(void)
{
	int	i;

	for(i = 0; i < MC_KEY_CHARS; i++)
	{
		int		col = i % MC_KEY_COLS;
		int		row = i / MC_KEY_COLS;

		hMcCharKeys[i] = BUTTON_CreateEx(MC_KEY_COL_X(col),
										 row * (MC_KEY_H + MC_KEY_VGAP),
										 MC_KEY_W, MC_KEY_H,
										 hMcKeyboard, WM_CF_SHOW, 0,
										 ID_BUTTON_CHAR_0 + i);

		BUTTON_SetSkin(hMcCharKeys[i], mc_ui_button_skin);
	}

	// Shift in the last slot
	{
		int		col = MC_KEY_CHARS % MC_KEY_COLS;
		int		row = MC_KEY_CHARS / MC_KEY_COLS;

		hMcShiftKey = BUTTON_CreateEx(MC_KEY_COL_X(col),
									  row * (MC_KEY_H + MC_KEY_VGAP),
									  MC_KEY_W, MC_KEY_H,
									  hMcKeyboard, WM_CF_SHOW, 0,
									  ID_BUTTON_SHIFT);

		BUTTON_SetSkin(hMcShiftKey, mc_ui_button_skin);
	}

	// Labels come from the active page
	mc_ui_apply_page();
}

//*----------------------------------------------------------------------------
//* Function Name       : mc_ui_set_input_enabled
//* Object              : lock/unlock the keys while a message drains out.
//*						: The char keys and shift live in hMcKeyboard, the
//*						: action row lives in hMcDialog
//* Context    			: CONTEXT_VIDEO (gui task)
//*----------------------------------------------------------------------------
static void mc_ui_set_input_enabled(int enabled)
{
	static const int action_ids[] =
	{
		ID_BUTTON_SPACE, ID_BUTTON_BACKSPACE, ID_BUTTON_CLEAR, ID_BUTTON_SEND
	};
	int	i;

	// Char keys (children of hMcKeyboard)
	for(i = 0; i < MC_KEY_CHARS; i++)
	{
		if(enabled)
			WM_EnableWindow(hMcCharKeys[i]);
		else
			WM_DisableWindow(hMcCharKeys[i]);
	}

	// Shift (child of hMcKeyboard)
	if(enabled)
		WM_EnableWindow(hMcShiftKey);
	else
		WM_DisableWindow(hMcShiftKey);

	// Action row (children of hMcDialog)
	for(i = 0; i < (int)GUI_COUNTOF(action_ids); i++)
	{
		WM_HWIN	hItem = WM_GetDialogItem(hMcDialog, action_ids[i]);

		if(enabled)
			WM_EnableWindow(hItem);
		else
			WM_DisableWindow(hItem);
	}
}

//*----------------------------------------------------------------------------
//* Function Name       : mc_ui_invalidate_all
//* Object              : repaint the whole screen - dialog, both child
//*						: windows and every button in both the dialog and
//*						: the keyboard container
//* Notes    			: the buttons are skinned, so their faces carry
//*						: enabled state that a plain dialog invalidate
//*						: would not reach
//* Context    			: CONTEXT_VIDEO (gui task)
//*----------------------------------------------------------------------------
static void mc_ui_invalidate_all(void)
{
	int	i;

	WM_InvalidateWindow(hMcDialog);

	if(hMcTitle)
		WM_InvalidateWindow(hMcTitle);

	if(hMcSlot)
		WM_InvalidateWindow(hMcSlot);

	if(hMcKeyboard)
		WM_InvalidateWindow(hMcKeyboard);

	// Char keys and shift (children of hMcKeyboard)
	for(i = 0; i < MC_KEY_CHARS; i++)
		WM_InvalidateWindow(hMcCharKeys[i]);

	WM_InvalidateWindow(hMcShiftKey);

	// Action row (children of hMcDialog)
	WM_InvalidateWindow(WM_GetDialogItem(hMcDialog, ID_BUTTON_SPACE));
	WM_InvalidateWindow(WM_GetDialogItem(hMcDialog, ID_BUTTON_BACKSPACE));
	WM_InvalidateWindow(WM_GetDialogItem(hMcDialog, ID_BUTTON_CLEAR));
	WM_InvalidateWindow(WM_GetDialogItem(hMcDialog, ID_BUTTON_SEND));
	WM_InvalidateWindow(WM_GetDialogItem(hMcDialog, ID_BUTTON_CALLER));
	WM_InvalidateWindow(WM_GetDialogItem(hMcDialog, ID_BUTTON_PEER));

	if(hMcTypeBtn)
		WM_InvalidateWindow(hMcTypeBtn);
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

//*----------------------------------------------------------------------------
//* Function Name       : mc_ui_on_button
//* Object              : handle a button release from either the dialog
//*						: (action row) or the keyboard container (char keys,
//*						: shift). No pMsg dependency - both callbacks forward
//*						: the id and notification code here
//* Context    			: CONTEXT_VIDEO (gui task)
//*----------------------------------------------------------------------------
static void mc_ui_on_button(int id, int ncode)
{
	MC_UI_STATUS	st;

	if(ncode != WM_NOTIFICATION_RELEASED)
		return;

	if((id >= ID_BUTTON_CHAR_0) && (id < ID_BUTTON_CHAR_0 + MC_KEY_CHARS))
	{
		if((!mc_ui_sending) && (mc_ui_compose_len < MC_UI_COMPOSE_MAX))
		{
			mc_ui_compose[mc_ui_compose_len++] = mc_ui_page()[id - ID_BUTTON_CHAR_0];
			mc_ui_compose[mc_ui_compose_len]   = 0;
		}

		WM_InvalidateWindow(hMcDialog);
		return;
	}

	// Shift pages the character keys between letters and symbols
	if(id == ID_BUTTON_SHIFT)
	{
		mc_ui_shift ^= 1;
		mc_ui_apply_page();
		return;
	}

	switch(id)
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
				mc_ui_set_input_enabled(0);

				// Collapse the keyboard after queuing a message
				mc_ui_hide_keyboard();
			}
			else
				mc_ui_add_line(MC_UI_LINE_INFO, "send queue full, try again", NULL);

			break;
		}

		case ID_BUTTON_TYPE:
		{
			if(mc_ui_kb_shown)
				mc_ui_hide_keyboard();
			else
				mc_ui_show_keyboard();
			break;
		}

		case ID_BUTTON_CALLER:
		case ID_BUTTON_PEER:
		{
			uint8_t	want = (id == ID_BUTTON_CALLER) ? MC_ROLE_CALLER : MC_ROLE_PEER;

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

	WM_InvalidateWindow(hMcDialog);
}

//*----------------------------------------------------------------------------
//* Function Name       : mc_ui_paint_status
//* Object              : atlas-styled status panel drawn in the space freed
//*						: by the collapsed keyboard. Richer layout than the
//*						: compact telemetry column: signal quality, session
//*						: state, and the slot timeline at full width
//* Context    			: CONTEXT_VIDEO (gui task, WM_PAINT, !mc_ui_kb_shown)
//*----------------------------------------------------------------------------
static void mc_ui_paint_status(void)
{
	MC_UI_STATUS	st;
	char			buf[32];
	int				col1 = MC_STAT_X + 16;
	int				col2 = MC_STAT_X + 220;
	int				col3 = MC_STAT_X + 520;
	int				y    = MC_STAT_Y + 6;
	int				bar_w, progress;

	marschat_get_status(&st);

	atlas_panel(MC_STAT_X, MC_STAT_Y, MC_STAT_W, MC_STAT_H, 1);
	GUI_SetTextMode(GUI_TM_TRANS);

	// ---- Column 1: signal quality ----
	GUI_SetFont(&GUI_Font13B_1);
	GUI_SetColor(ATLAS_DIM);
	atlas_text(col1, y, "SIGNAL", 2);

	y += 18;

	if(mc_ui_have_rx)
	{
		snprintf(buf, sizeof(buf), "%d dB", mc_ui_last_snr);
		GUI_SetFont(&GUI_Font24B_1);
		GUI_SetColor(ATLAS_CYAN);
		atlas_text(col1, y, buf, 1);

		// SNR quality bar
		bar_w = mc_ui_last_snr + 40;				// -40 dB = 0, 0 dB = 40
		if(bar_w < 0) bar_w = 0;
		if(bar_w > 60) bar_w = 60;

		atlas_track(col1, y + 30, 160, 6, (bar_w * 160) / 60, ATLAS_LINE_OFF, ATLAS_CYAN);
	}
	else
	{
		GUI_SetFont(&GUI_Font24B_1);
		GUI_SetColor(ATLAS_DIM);
		atlas_text(col1, y, "NO RX", 1);

		atlas_track(col1, y + 30, 160, 6, 0, ATLAS_LINE_OFF, ATLAS_CYAN);
	}

	y += 42;

	if(mc_ui_have_rx)
	{
		int frac = mc_ui_last_dt % 100;
		if(frac < 0) frac = -frac;

		snprintf(buf, sizeof(buf), "DT %d.%02d s", mc_ui_last_dt / 100, frac);
	}
	else
		snprintf(buf, sizeof(buf), "DT -");

	GUI_SetFont(&GUI_Font13B_1);
	GUI_SetColor(ATLAS_DIM);
	atlas_text(col1, y, buf, 2);

	// ---- Column 2: slot timeline (full width) ----
	y = MC_STAT_Y + 6;

	GUI_SetFont(&GUI_Font13B_1);
	GUI_SetColor(ATLAS_DIM);
	atlas_text(col2, y, "SLOT TIMELINE", 2);

	y += 20;

	// Large slot map - 120 s burst (111 s tone + 9 s decode/CW)
	atlas_track(col2, y, 260, 10, (260 * 111) / 120, ATLAS_CYAN_DEEP, ATLAS_CYAN_DEEP);

	// Lime tail (decode + CW id window)
	GUI_SetColor(ATLAS_LIME);
	GUI_FillRect(col2 + (260 * 111) / 120 + 1, y, col2 + 259, y + 9);

	// Tick marks every 30 s
	GUI_SetColor(ATLAS_DIM);
	GUI_DrawVLine(col2 + (260 * 30) / 120, y + 11, y + 15);
	GUI_DrawVLine(col2 + (260 * 60) / 120, y + 11, y + 15);
	GUI_DrawVLine(col2 + (260 * 90) / 120, y + 11, y + 15);

	GUI_SetFont(&GUI_Font13B_1);
	atlas_text(col2, y + 13, "0", 0);
	atlas_text(col2 + 260 - 20, y + 13, "120s", 0);

	y += 32;

	// Current slot progress
	if(st.state != MC_SESS_OFF)
	{
		progress = ((120 - st.secs_to_slot) * 260) / 120;
		if(progress < 0) progress = 0;

		atlas_track(col2, y, 260, 8, progress,
					ATLAS_LINE_OFF,
					st.tx_busy ? ATLAS_AMBER : ATLAS_CYAN);

		GUI_SetFont(&GUI_Font13B_1);
		GUI_SetColor(st.tx_busy ? ATLAS_AMBER : ATLAS_CYAN);

		snprintf(buf, sizeof(buf), "%s IN %ds", st.next_is_ours ? "TX" : "RX",
				st.secs_to_slot);
		atlas_text(col2, y + 12, buf, 2);
	}
	else
	{
		atlas_track(col2, y, 260, 8, 0, ATLAS_LINE_OFF, ATLAS_CYAN);
	}

	// ---- Column 3: session / ARQ state ----
	y = MC_STAT_Y + 6;

	GUI_SetFont(&GUI_Font13B_1);
	GUI_SetColor(ATLAS_DIM);
	atlas_text(col3, y, "SESSION", 2);

	y += 20;

	snprintf(buf, sizeof(buf), "SEQ %d", st.tx_seq);
	mc_ui_telem_cell(col3, y, "TX", buf, ATLAS_AMBER);

	snprintf(buf, sizeof(buf), "SEQ %d", st.last_rx_seq);
	mc_ui_telem_cell(col3 + 120, y, "ACK", buf, ATLAS_AMBER);

	y += 40;

	if(st.pending)
		snprintf(buf, sizeof(buf), "%d / %d", st.retries, MC_ARQ_RETRIES);
	else
		snprintf(buf, sizeof(buf), "-");

	mc_ui_telem_cell(col3, y, "TRIES", buf, st.pending ? ATLAS_AMBER : ATLAS_DIM);

	snprintf(buf, sizeof(buf), "%d", st.queued_chunks);
	mc_ui_telem_cell(col3 + 120, y, "QUEUE", buf, st.queued_chunks ? ATLAS_CYAN : ATLAS_DIM);

	// ---- Decorative ticks below the panel ----
	atlas_ticks(MC_STAT_X + 12, MC_STAT_Y + MC_STAT_H + 2, MC_STAT_W - 24, 2, 8, 1, ATLAS_CYAN_DEEP);
}

//*----------------------------------------------------------------------------
//* Function Name       : _cbKeyboard
//* Object              : callback for the keyboard container window. Draws
//*						: the atlas background behind the character keys and
//*						: forwards button presses to mc_ui_on_button
//* Context    			: CONTEXT_VIDEO (gui task)
//*----------------------------------------------------------------------------
static void _cbKeyboard(WM_MESSAGE *pMsg)
{
	switch(pMsg->MsgId)
	{
		case WM_PAINT:
		{
			atlas_background(0, 0, MC_KB_W, MC_KB_H);

			// Top accent: lime hairline then a subtle border
			GUI_SetColor(ATLAS_LIME);
			GUI_DrawHLine(0, 0, MC_KB_W - 1);
			GUI_SetColor(ATLAS_LINE);
			GUI_DrawHLine(1, 0, MC_KB_W - 1);
			break;
		}

		case WM_NOTIFY_PARENT:
			mc_ui_on_button(WM_GetId(pMsg->hWinSrc), pMsg->Data.v);
			break;

		default:
			WM_DefaultProc(pMsg);
			break;
	}
}

static void _cbDialog(WM_MESSAGE * pMsg)
{
	int		Id, NCode;
	int		i;

	switch (pMsg->MsgId)
	{
		case WM_INIT_DIALOG:
		{
			// Latch the dialog handle early - GUI_CreateDialogBox has
			// not returned yet, so the static hMcDialog is still 0 at
			// this point, but mc_ui_layout_action_row and friends need
			// it to find the template buttons via WM_GetDialogItem
			hMcDialog = pMsg->hWin;

			// The keyboard container lives below the visible area and
			// slides up on demand. Created before the char keys so it
			// can serve as their parent
			hMcKeyboard = WM_CreateWindowAsChild(0, MC_UI_H, MC_KB_W, MC_KB_H,
								pMsg->hWin, WM_CF_SHOW, _cbKeyboard, 0);

			mc_ui_create_keys();

			// TYPE button - created programmatically (not in the
			// template) so it can be shown/hidden. It opens the
			// keyboard and is the only way to trigger it - WM_TOUCH
			// on a WINDOW is unreliable on a resistive panel
			hMcTypeBtn = BUTTON_CreateEx(MC_FULL_TYPE_X, MC_ACT_Y,
										 MC_FULL_TYPE_W, MC_ACT_H,
										 pMsg->hWin, WM_CF_SHOW, 0,
										 ID_BUTTON_TYPE);

			BUTTON_SetText(hMcTypeBtn, "TYPE");
			BUTTON_SetSkin(hMcTypeBtn, mc_ui_button_skin);

			// Action row buttons come from the static template - just
			// apply the atlas skin
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

			// Keyboard starts hidden - set the expanded action row
			mc_ui_kb_shown = 0;
			hMcAnim        = 0;
			mc_ui_layout_action_row(0);
			mc_ui_set_input_enabled(!mc_ui_sending);
			break;
		}

		case WM_PAINT:
		{
			atlas_background(0, MC_TITLE_H, MC_UI_W, MC_UI_H - MC_TITLE_H);
			atlas_grid(0, MC_TITLE_H, MC_UI_W, MC_UI_H - MC_TITLE_H, 80);

			mc_ui_paint_history();
			mc_ui_paint_telemetry();
			mc_ui_paint_compose();

			// The status panel is always painted behind the keyboard
			// container. When the container is on-screen it hides the
			// panel; when it slides away the panel is revealed
			if(!mc_ui_kb_shown)
				mc_ui_paint_status();

			break;
		}

		case WM_TIMER:
		{
			MC_UI_STATUS	st;
			int				dirty;

			dirty = mc_ui_drain_rx();

			// Message fully drained - clear the draft and unlock input
			marschat_get_status(&st);

			if((mc_ui_sending) && (!st.tx_busy) && (!st.pending) && (st.queued_chunks == 0))
			{
				mc_ui_sending		= 0;
				mc_ui_compose_len	= 0;
				mc_ui_compose[0]	= 0;

				mc_ui_set_input_enabled(1);
				dirty = 1;
			}

			if(mc_ui_model_changed())
				dirty = 1;

			if(dirty)
			{
				mc_ui_invalidate_all();
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
			hMcTitle    = 0;
			hMcSlot     = 0;
			hMcKeyboard = 0;

			memset(hMcCharKeys, 0, sizeof(hMcCharKeys));
			hMcShiftKey = 0;
			hMcTypeBtn  = 0;
			hMcAnim     = 0;
			break;
		}

		case WM_NOTIFY_PARENT:
		{
			Id    = WM_GetId(pMsg->hWinSrc);
			NCode = pMsg->Data.v;

			mc_ui_on_button(Id, NCode);
			break;
		}

		// WM_TOUCH removed — TYPE/HIDE button is the sole keyboard
		// toggle; WM_TOUCH on a plain WINDOW was unreliable on the
		// resistive panel anyway

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
