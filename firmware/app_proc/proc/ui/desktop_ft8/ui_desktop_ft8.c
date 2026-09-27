/************************************************************************************
**                                                                                 **
**                                 mcHF QRP Transceiver                            **
**                         Krassi Atanassov - M0NKA, 2013-2026                     **
**                                                                                 **
**---------------------------------------------------------------------------------**
**                                                                                 **
**  File name:		ui_desktop_ft8.c                                               **
**  Description:	FT8 desktop - atlas themed, band activity + rx frequency       **
**					lists, slot bar, next transmission                             **
**  Licence:		https://github.com/m0nka/mcHF/blob/main/LICENSE                **
************************************************************************************/
//
// LAYOUT PROTOTYPE. There is no FT8 decoder yet - see claude/FT8/PROJECT.md
// for the roadmap and for why the decode feasibility gate (WP2) comes
// before any of the backend work. Everything in the two lists is static
// demo content, and the action row buttons only toggle local UI state.
//
// What is real: the geometry, the theme, the repaint scoping, the slot
// clock (driven off the RTC, so the countdown and the progress bar do
// track actual UTC), and the create/destroy lifecycle. Those are the
// parts that are expensive to change later, so they are what the
// prototype exists to settle.
//
// Everything is drawn by hand (theme/atlas_draw.c). The only emWin
// widgets are the action row buttons, whose painting is taken over by a
// skin callback while the framework keeps the hit testing. In
// particular the decode lists are NOT emWin LISTBOXes: the MarsChat
// history pane established that hand painting is both faster and free
// of the WM__Paint fault class the MeshCore listbox still suffers from.
//
// Repaints are scoped: the clock and the slot countdown tick twice a
// second and live in their own child windows, so the decode lists below
// them are only redrawn when the data behind them actually changes.
//
#include <stdlib.h>

#include "mchf_pro_board.h"
#include "main.h"

#ifdef CONTEXT_VIDEO

#include "ui_proc.h"
#include "gui.h"
#include "dialog.h"
#include "desktop\ui_controls_layout.h"

#include "rtc.h"

#include "atlas_draw.h"

#include "ui_desktop_ft8.h"

#include "ft8_proc.h"
#include "ft8_radio.h"
#include "ft8_qso.h"

// UI driver public state
extern struct	UI_DRIVER_STATE			ui_s;
extern struct	PROC_STATE				ps;
extern struct	TRANSCEIVER_STATE_UI	tsu;

WM_HWIN				hDesktopFT8 = 0;

static WM_HWIN		hFT8Title;					// title strip, owns the clock
static WM_HWIN		hFT8Slot;					// slot bar, owns the countdown
static WM_HWIN		hFT8Wf;						// waterfall + frequency axis
static WM_HTIMER	hFT8Timer;

// ---------------------------------------------------------------------
// Screen state
//
// Only tx_armed and sel_row are decisions the operator has made. The
// decode lists and the receiver state live in the ft8 task and outlive
// this dialog; the screen keeps a copy, refreshed when the task says the
// history changed

static uint8_t		ft8_tx_armed = 0;			// transmitter armed
static int8_t		ft8_sel_row  = -1;			// selected decode, -1 = none

// Slot phase, recomputed each tick from the RTC
#define FT8_PHASE_RX			0
#define FT8_PHASE_TX			1

static uint8_t		ft8_phase;
static uint8_t		ft8_slot_secs;				// seconds elapsed into the current slot

// Copy of the ft8 task's decode history, oldest first
static FT8_DECODE	ft8_ui_hist[FT8_HISTORY];
static int			ft8_ui_hist_n   = 0;
static ulong		ft8_ui_hist_gen = 0xFFFFFFFF;

// The two panes: all recent decodes, and those near our own audio
// frequency (until the QSO sequencer knows our call, WP7)
static FT8_DECODE	ft8_ui_band[FT8_LIST_ROWS];
static int			ft8_ui_band_n = 0;
static FT8_DECODE	ft8_ui_rxf[FT8_LIST_ROWS];
static int			ft8_ui_rxf_n  = 0;

#define FT8_RX_AUDIO_HZ			1500			// our tx/rx audio offset, WP7 moves it
#define FT8_RX_WINDOW_HZ		60

static FT8_LIVE_STATUS	ft8_ui_live;

// Waterfall - the desktop scope palette, so the two look alike. ARGB,
// opaque alpha added the same way ui_controls_spectrum.c does it
extern const ulong		waterfall_blue[64];

static GUI_COLOR		ft8_wf_colors[64];
static const GUI_LOGPALETTE	ft8_wf_pal = { 64, 0, ft8_wf_colors };

// UI tick. Must be well under the 160 ms waterfall line rate: at 200 ms
// most repaints scrolled one line and every fourth one two, a visible
// jerk about once a second. At 40 ms every line gets its own repaint
#define FT8_UI_TICK_MS			40

// Waterfall pacing: lines on screen, and when the last one went up. The
// writer is jittery (M4 -> icc poll -> ft8 task -> this timer), so the
// screen scrolls on its own clock, one line per FT8_UI_WF_LINE_MS, and
// only hurries when it falls behind (slot starts)
#define FT8_UI_WF_LINE_MS		150
#define FT8_UI_WF_MAX_LAG		2

static ulong			ft8_wf_shown   = 0;
static TickType_t		ft8_wf_adv_t   = 0;
static uint8_t			ft8_ui_secs  = 0xFF;	// clock second last painted

static void ft8_ui_on_button(int id, int ncode);

//*----------------------------------------------------------------------------
//* Function Name       : ft8_ui_dial_hz
//* Object              : dial (usb carrier) frequency of the active vfo
//* Context    			: CONTEXT_VIDEO (gui task)
//*----------------------------------------------------------------------------
static ulong ft8_ui_dial_hz(void)
{
	struct BAND_INFO *b = &tsu.band[tsu.curr_band];

	if(b->active_vfo == VFO_A)
		return b->vfo_a;

	return b->vfo_b;
}

//*----------------------------------------------------------------------------
//* Function Name       : ft8_ui_refresh_lists
//* Object              : pull the decode history from the ft8 task when it
//*						: changed, rebuild both panes
//* Notes    			: returns 1 when the lists changed
//* Context    			: CONTEXT_VIDEO (gui task)
//*----------------------------------------------------------------------------
static int ft8_ui_refresh_lists(void)
{
	ulong	gen;
	int		i, d;

	ft8_ui_hist_n = ft8_proc_get_history(ft8_ui_hist, FT8_HISTORY, &gen);

	if(gen == ft8_ui_hist_gen)
		return 0;

	ft8_ui_hist_gen = gen;

	// Band activity - the newest, oldest at the top like WSJT-X
	ft8_ui_band_n = (ft8_ui_hist_n < FT8_LIST_ROWS) ? ft8_ui_hist_n : FT8_LIST_ROWS;
	memcpy(ft8_ui_band, ft8_ui_hist + ft8_ui_hist_n - ft8_ui_band_n, ft8_ui_band_n * sizeof(FT8_DECODE));

	// RX frequency - our own traffic: to or from us, or from the station
	// we are working
	ft8_ui_rxf_n = 0;
	for(i = ft8_ui_hist_n - 1; (i >= 0) && (ft8_ui_rxf_n < FT8_LIST_ROWS); i--)
	{
		if(ft8_qso_is_ours(ft8_ui_hist[i].msg))
			ft8_ui_rxf[ft8_ui_rxf_n++] = ft8_ui_hist[i];
	}
	(void)d;

	// That walked newest first - flip to oldest first
	for(i = 0; i < ft8_ui_rxf_n / 2; i++)
	{
		FT8_DECODE t = ft8_ui_rxf[i];

		ft8_ui_rxf[i] = ft8_ui_rxf[ft8_ui_rxf_n - 1 - i];
		ft8_ui_rxf[ft8_ui_rxf_n - 1 - i] = t;
	}

	// A selection points at a row that has moved - drop it
	ft8_sel_row = -1;

	return 1;
}

//*----------------------------------------------------------------------------
//* Function Name       : ft8_ui_tick_slot
//* Object              : recompute where we are in the 15 s slot from the
//*						: RTC. Real UTC, not a free-running counter - the
//*						: whole mode depends on slot alignment, so the
//*						: prototype may as well show the truth about it
//* Notes    			: the RTC is PPS disciplined on units that have GPS
//*						: (claude/rtc). On a unit without one this will
//*						: drift, which is exactly what the sync indicator
//*						: in the slot bar is there to say
//* Context    			: CONTEXT_VIDEO (gui task, WM_TIMER)
//*----------------------------------------------------------------------------
static void ft8_ui_tick_slot(void)
{
	RTC_TimeTypeDef	tm = {0};
	RTC_DateTypeDef	dt = {0};
	int				secs;

	// Both, in this order - the STM32 shadow registers do not unlock
	// until the date has been read
	k_GetTime(&tm);
	k_GetDate(&dt);

	secs = ((int)tm.Minutes * 60) + (int)tm.Seconds;

	ft8_slot_secs = (uint8_t)(secs % FT8_SLOT_SECS);

	// The ft8 task decides - it knows the parity and the watchdog
	ft8_phase = (ft8_ui_live.state == FT8_LIVE_TX) ? FT8_PHASE_TX : FT8_PHASE_RX;
}

//*----------------------------------------------------------------------------
//* Function Name       : ft8_ui_snr_colour
//* Object              : decode strength to ink colour, WSJT-X style -
//*						: strong signals should be findable at a glance
//* Context    			: CONTEXT_VIDEO (gui task, WM_PAINT)
//*----------------------------------------------------------------------------
static GUI_COLOR ft8_ui_snr_colour(int8_t snr)
{
	if(snr >= -5)
		return ATLAS_LIME;

	if(snr >= -15)
		return ATLAS_CYAN;

	return ATLAS_DIM;
}

//*----------------------------------------------------------------------------
//* Function Name       : ft8_ui_is_cq
//* Object              : does this decode start with CQ ? those are the
//*						: workable ones, and they get the amber highlight
//* Context    			: CONTEXT_VIDEO (gui task, WM_PAINT)
//*----------------------------------------------------------------------------
static int ft8_ui_is_cq(const char *msg)
{
	return ((msg[0] == 'C') && (msg[1] == 'Q'));
}

//*----------------------------------------------------------------------------
//* Function Name       : ft8_ui_paint_title
//* Object              : title strip - mode, dial frequency, wall clock
//* Notes    			: the clock ticks, so this is its own child window
//*						: and the panels below it stay untouched
//* Context    			: CONTEXT_VIDEO (gui task, WM_PAINT)
//*----------------------------------------------------------------------------
static void ft8_ui_paint_title(void)
{
	RTC_TimeTypeDef	tm = {0};
	RTC_DateTypeDef	dt = {0};
	char			buf[32];
	ulong			dial;
	int				x;

	k_GetTime(&tm);
	k_GetDate(&dt);

	dial = ft8_ui_dial_hz();

	atlas_background(0, 0, FT8_UI_W, FT8_TITLE_H);

	GUI_SetTextMode(GUI_TM_TRANS);

	// Lime chevron, as on the MarsChat title
	GUI_SetColor(ATLAS_LIME);
	GUI_FillRect(10, 8, 12, 22);
	GUI_FillRect(10, 20, 22, 22);

	GUI_SetFont(&GUI_Font24B_1);
	GUI_SetColor(GUI_WHITE);
	x = atlas_text(30, 4, "FT8", 6);

	atlas_ticks(x + 16, 12, 140, 6, 9, 2, ATLAS_CYAN_DEEP);
	atlas_ticks(x + 16, 22, 110, 3, 14, 2, ATLAS_AMBER_DEEP);

	// Clock right, dial frequency just left of it
	snprintf(buf, sizeof(buf), "%02d:%02d:%02d", tm.Hours, tm.Minutes, tm.Seconds);
	GUI_SetColor(ATLAS_AMBER);
	atlas_text_right(FT8_UI_W - 10, 4, buf, 2);

	// Band and the actual dial. The screen tunes the band's FT8 frequency
	// on entry and on BAND - if the dial is anywhere else (retuned from a
	// knob) it goes amber, because off frequency it decodes nothing
	snprintf(buf, sizeof(buf), "%s   %u.%06u MHZ", ft8_radio_band_name(tsu.curr_band),
			(unsigned)(dial / 1000000), (unsigned)(dial % 1000000));

	GUI_SetFont(&GUI_Font16B_1);
	GUI_SetColor((dial == ft8_radio_band_dial(tsu.curr_band)) ? ATLAS_CYAN : ATLAS_AMBER);
	atlas_text_right(FT8_UI_W - 150, 8, buf, 1);

	GUI_SetColor(ATLAS_LINE);
	GUI_DrawHLine(FT8_TITLE_H - 1, 0, FT8_UI_W - 1);
}

//*----------------------------------------------------------------------------
//* Function Name       : ft8_ui_paint_slot
//* Object              : slot bar - what this slot is doing, the countdown
//*						: to the next one, the clock sync source and the
//*						: decode count, plus the slot progress hairline
//* Context    			: CONTEXT_VIDEO (gui task, WM_PAINT)
//*----------------------------------------------------------------------------
static void ft8_ui_paint_slot(void)
{
	char		buf[64];
	const char	*doing;
	GUI_COLOR	c0, c1, ink;
	int			remain, filled;

	atlas_background(0, 0, FT8_SLOT_W, FT8_SLOT_WIN_H);

	// Left cap
	GUI_SetColor(ATLAS_LIME);
	GUI_FillRect(0, 0, 6, FT8_SLOT_H - 1);

	if(ft8_phase == FT8_PHASE_TX)
	{
		doing = "TX SLOT";
		c0    = ATLAS_AMBER;
		c1    = ATLAS_AMBER_DEEP;
		ink   = ATLAS_INK;
	}
	else if(ft8_ui_live.state == FT8_LIVE_CAPTURE)
	{
		doing = "RX CAPTURE";
		c0    = ATLAS_CYAN;
		c1    = ATLAS_CYAN_DEEP;
		ink   = ATLAS_TEXT;
	}
	else if(ft8_ui_live.state == FT8_LIVE_DECODE)
	{
		doing = "DECODING";
		c0    = ATLAS_CYAN_DEEP;
		c1    = ATLAS_PANEL;
		ink   = ATLAS_TEXT;
	}
	else
	{
		// Between slots, or not receiving at all - say which
		doing = (ft8_ui_live.state == FT8_LIVE_WAIT)     ? "WAIT SLOT" :
				(ft8_ui_live.state == FT8_LIVE_NO_ARENA) ? "HF BUSY"   : "RX OFF";
		c0    = ATLAS_PANEL;
		c1    = ATLAS_PANEL;
		ink   = ATLAS_DIM;
	}

	// Mode block on the left, slot state to the right of it
	atlas_bar(10, 0, 250, FT8_SLOT_H, ATLAS_CYAN_HI, ATLAS_CYAN);
	atlas_bar(263, 0, 508, FT8_SLOT_H, c0, c1);

	GUI_SetTextMode(GUI_TM_TRANS);
	GUI_SetFont(&GUI_Font20B_1);

	GUI_SetColor(ATLAS_INK);
	atlas_text(24, 9, ft8_tx_armed ? "TX ARMED" : "RX ONLY", 4);

	remain = FT8_SLOT_SECS - (int)ft8_slot_secs;

	GUI_SetColor(ink);
	// Slot clock correction learned from DT, tenths of a second
	{
		int c = ft8_ui_live.clock_ofs_ms / 100;

		snprintf(buf, sizeof(buf), "%s   %d DEC   CLK %s%d.%d   NEXT %ds", doing,
				(int)ft8_ui_live.last_count, (c < 0) ? "-" : "+", abs(c) / 10, abs(c) % 10, remain);
	}
	atlas_text_right(FT8_SLOT_W - 24, 9, buf, 3);

	// Right cap - amber while armed, so the transmit state is readable
	// from across the bench
	GUI_SetColor(ft8_tx_armed ? ATLAS_AMBER : ATLAS_LIME);
	GUI_FillRect(FT8_SLOT_W - 8, 0, FT8_SLOT_W - 1, FT8_SLOT_H - 1);

	// Slot progress hairline - sweeps once per 15 s slot
	filled = (FT8_SLOT_W * (int)ft8_slot_secs) / FT8_SLOT_SECS;

	atlas_track(0, FT8_PROG_Y, FT8_SLOT_W, FT8_PROG_H, filled,
				ATLAS_LINE_OFF,
				(ft8_phase == FT8_PHASE_TX) ? ATLAS_AMBER : ATLAS_CYAN);
}

//*----------------------------------------------------------------------------
//* Function Name       : ft8_ui_paint_list
//* Object              : one decode list - bracketed panel, column header
//*						: and up to FT8_LIST_ROWS decodes
//* Notes    			: hand painted, not a LISTBOX. Same call as the
//*						: MarsChat history pane: the rows are fixed height
//*						: and fully redrawn anyway, so the widget bought
//*						: nothing but the fault surface it comes with
//* Context    			: CONTEXT_VIDEO (gui task, WM_PAINT)
//*----------------------------------------------------------------------------
static void ft8_ui_paint_list(int x, const char *title, const FT8_DECODE *list,
								int count, int selectable)
{
	int	i, y;

	atlas_panel(x, FT8_LIST_Y, FT8_LIST_W, FT8_LIST_H, 1);

	GUI_SetTextMode(GUI_TM_TRANS);

	// Panel title, top left, and the column header under it
	GUI_SetFont(&GUI_Font13B_1);
	GUI_SetColor(ATLAS_CYAN);
	atlas_text(x + FT8_COL_TIME, FT8_LIST_Y + 5, title, 2);

	GUI_SetColor(ATLAS_DIM);
	atlas_text(x + FT8_COL_TIME,  FT8_LIST_Y + FT8_LIST_HDR_H, "UTC",  1);
	atlas_text(x + FT8_COL_SNR,   FT8_LIST_Y + FT8_LIST_HDR_H, "DB",   1);
	atlas_text(x + FT8_COL_DT,    FT8_LIST_Y + FT8_LIST_HDR_H, "DT",   1);
	atlas_text(x + FT8_COL_FREQ,  FT8_LIST_Y + FT8_LIST_HDR_H, "FREQ", 1);
	atlas_text(x + FT8_COL_MSG,   FT8_LIST_Y + FT8_LIST_HDR_H, "MESSAGE", 1);

	GUI_SetColor(ATLAS_LINE_OFF);
	GUI_DrawHLine(FT8_LIST_Y + FT8_LIST_HDR_H + 16, x + 4, x + FT8_LIST_W - 5);

	if(count > FT8_LIST_ROWS)
		count = FT8_LIST_ROWS;

	for(i = 0; i < count; i++)
	{
		const FT8_DECODE	*d   = &list[i];
		int					sel  = (selectable) && (i == ft8_sel_row);
		int					adt  = (d->dt < 0) ? -d->dt : d->dt;

		y = FT8_LIST_Y + FT8_LIST_HDR_H + 22 + i * FT8_LIST_ROW_H;

		// Selected row gets a filled band, others alternate faintly
		if(sel)
		{
			GUI_SetColor(ATLAS_CYAN_DEEP);
			GUI_FillRect(x + 4, y, x + FT8_LIST_W - 5, y + FT8_LIST_ROW_H - 2);
		}
		else if(i & 1)
		{
			GUI_SetColor(ATLAS_ROW);
			GUI_FillRect(x + 4, y, x + FT8_LIST_W - 5, y + FT8_LIST_ROW_H - 2);
		}

		GUI_SetFont(&GUI_Font13B_1);

		// UTC
		GUI_SetColor(sel ? ATLAS_TEXT : ATLAS_DIM);
		atlas_text(x + FT8_COL_TIME, y + 2, d->time, 0);

		// SNR, coloured by strength
		{
			char	buf[8];

			snprintf(buf, sizeof(buf), "%d", d->snr);
			GUI_SetColor(sel ? ATLAS_TEXT : ft8_ui_snr_colour(d->snr));
			atlas_text(x + FT8_COL_SNR, y + 2, buf, 0);
		}

		// DT and audio frequency
		{
			char	buf[12];

			// Sign by hand - "-0.3" has an integer part of 0
			snprintf(buf, sizeof(buf), "%s%d.%01d", (d->dt < 0) ? "-" : "", adt / 100, (adt % 100) / 10);
			GUI_SetColor(sel ? ATLAS_TEXT : ATLAS_DIM);
			atlas_text(x + FT8_COL_DT, y + 2, buf, 0);

			snprintf(buf, sizeof(buf), "%u", (unsigned)d->freq);
			atlas_text(x + FT8_COL_FREQ, y + 2, buf, 0);
		}

		// Message - CQ in amber, everything else in the normal ink
		GUI_SetColor(sel			   ? ATLAS_TEXT  :
					 ft8_ui_is_cq(d->msg) ? ATLAS_AMBER : ATLAS_TEXT);
		atlas_text(x + FT8_COL_MSG, y + 2, d->msg, 0);
	}

	// Empty list still says so, rather than showing a blank panel
	if(count == 0)
	{
		GUI_SetFont(&GUI_Font16B_1);
		GUI_SetColor(ATLAS_DIM);
		atlas_text(x + FT8_COL_TIME, FT8_LIST_Y + FT8_LIST_HDR_H + 30, "NO DECODES", 2);
	}
}

//*----------------------------------------------------------------------------
//* Function Name       : ft8_ui_paint_tx
//* Object              : the message queued for the next slot we own, plus
//*						: an armed / not armed chip
//* Context    			: CONTEXT_VIDEO (gui task, WM_PAINT)
//*----------------------------------------------------------------------------
static void ft8_ui_paint_tx(void)
{
	const char	*msg;
	GUI_COLOR	col;

	atlas_panel(FT8_TX_X, FT8_TX_Y, FT8_TX_W, FT8_TX_H, 0);

	GUI_SetTextMode(GUI_TM_TRANS);

	col = ft8_tx_armed ? ATLAS_AMBER : ATLAS_DIM;

	atlas_chip(FT8_TX_X + 8, FT8_TX_Y + 8, 34, 18, "TX", col, ft8_tx_armed);

	// The message queued in the ft8 task (CALL CQ sets it, WP7 will)
	msg = ft8_ui_live.tx_msg[0] ? ft8_ui_live.tx_msg : "PRESS CALL CQ";

	GUI_SetFont(&GUI_Font20B_1);
	GUI_SetColor(ft8_tx_armed ? ATLAS_TEXT : ATLAS_DIM);
	atlas_text(FT8_TX_X + 52, FT8_TX_Y + 6, msg, 2);

	{
		char	st[40];

		static const char * const qs[] =
		{
			"", "CQ", "CALLING", "SENT REPORT", "SENT R+REPORT", "RR73", "73", "LOGGED"
		};
		FT8_QSO_STATUS	q;

		ft8_qso_get(&q);

		if(ft8_ui_live.state == FT8_LIVE_TX)
			snprintf(st, sizeof(st), "%s  TX %d/%d", qs[q.state & 7], ft8_ui_live.tx_count, 6);
		else if(ft8_tx_armed)
			snprintf(st, sizeof(st), "%s  NEXT %s %d/%d", qs[q.state & 7],
					ft8_ui_live.tx_parity ? "ODD" : "EVEN", ft8_ui_live.tx_count, 6);
		else if(q.state == FT8_QSO_DONE)
			snprintf(st, sizeof(st), "LOGGED %s", q.call);
		else
			snprintf(st, sizeof(st), "TRANSMITTER DISARMED");

		GUI_SetFont(&GUI_Font13B_1);
		GUI_SetColor(ft8_ui_live.tx_rc ? ATLAS_AMBER : ATLAS_DIM);
		atlas_text_right(FT8_TX_X + FT8_TX_W - 10, FT8_TX_Y + 10, st, 2);
	}
}

//*----------------------------------------------------------------------------
//* Function Name       : ft8_ui_wf_x
//* Object              : audio frequency -> waterfall x
//* Context    			: CONTEXT_VIDEO (gui task)
//*----------------------------------------------------------------------------
static int ft8_ui_wf_x(int hz)
{
	return ((hz - 200) * FT8_WF_W) / 2800;
}

//*----------------------------------------------------------------------------
//* Function Name       : ft8_ui_paint_wf
//* Object              : waterfall image and frequency axis
//* Notes    			: the image is rendered by the ft8 task, this only
//*						: blits it through the palette
//* Context    			: CONTEXT_VIDEO (gui task, WM_PAINT)
//*----------------------------------------------------------------------------
static void ft8_ui_paint_wf(void)
{
	const uint8_t	*ring = ft8_proc_display(NULL);
	char			buf[8];
	int				hz, x, s0, n1;

	if((ring != NULL) && (ft8_wf_shown != 0))
	{
		// Newest shown line on top. Newer lines sit at lower ring slots,
		// so from its slot upwards is newest-first until the ring wraps
		s0 = FT8_DISP_SLOT(ft8_wf_shown - 1);
		n1 = FT8_DISP_RING - s0;
		if(n1 > FT8_WF_H)
			n1 = FT8_WF_H;

		GUI_DrawBitmapExp(0, 0, FT8_WF_W, n1, 1, 1, 8, FT8_WF_W,
						  ring + (ulong)s0 * FT8_WF_W, &ft8_wf_pal);

		if(n1 < FT8_WF_H)
			GUI_DrawBitmapExp(0, n1, FT8_WF_W, FT8_WF_H - n1, 1, 1, 8, FT8_WF_W,
							  ring, &ft8_wf_pal);
	}
	else
	{
		GUI_SetColor(ft8_wf_colors[0]);
		GUI_FillRect(0, 0, FT8_WF_W - 1, FT8_WF_H - 1);
	}

	// Axis strip
	GUI_SetColor(GUI_BLACK);
	GUI_FillRect(0, FT8_WF_H, FT8_WF_W - 1, FT8_WF_WIN_H - 1);

	GUI_SetTextMode(GUI_TM_TRANS);
	GUI_SetFont(&GUI_Font13B_1);

	for(hz = 500; hz <= 3000; hz += 500)
	{
		x = ft8_ui_wf_x(hz);

		GUI_SetColor(ATLAS_DIM);
		GUI_DrawVLine(x, FT8_WF_H, FT8_WF_H + 3);

		snprintf(buf, sizeof(buf), "%d", hz);
		GUI_DispStringAt(buf, x + 3, FT8_WF_H + 2);
	}

	// Our own rx/tx audio offset
	x = ft8_ui_wf_x(FT8_RX_AUDIO_HZ);
	GUI_SetColor(ATLAS_AMBER);
	GUI_FillRect(x - 1, FT8_WF_H, x + 1, FT8_WF_WIN_H - 1);
}

//*----------------------------------------------------------------------------
//* Function Name       : _cbWf
//* Object              : waterfall child window
//* Context    			: CONTEXT_VIDEO (gui task)
//*----------------------------------------------------------------------------
static void _cbWf(WM_MESSAGE *pMsg)
{
	switch(pMsg->MsgId)
	{
		case WM_PAINT:
			ft8_ui_paint_wf();
			break;

		default:
			WM_DefaultProc(pMsg);
			break;
	}
}

//*----------------------------------------------------------------------------
//* Function Name       : ft8_ui_button_skin
//* Object              : atlas skin for the action row - the framework
//*						: keeps hit testing, we do the painting
//* Context    			: CONTEXT_VIDEO (gui task, WM_PAINT)
//*----------------------------------------------------------------------------
static int ft8_ui_button_skin(const WIDGET_ITEM_DRAW_INFO *pDrawItemInfo)
{
	WM_HWIN			hObj = pDrawItemInfo->hWin;
	char			text[24];
	GUI_COLOR		face, edge, ink;
	int				id, w, h, tw;
	int				pressed, enabled, active = 0;

	if(pDrawItemInfo->Cmd != WIDGET_ITEM_DRAW_BACKGROUND)
		return 0;

	id      = WM_GetId(hObj);
	w       = WM_GetWindowSizeX(hObj);
	h       = WM_GetWindowSizeY(hObj);
	pressed = (int)BUTTON_IsPressed(hObj);
	enabled = WM_IsEnabled(hObj);

	BUTTON_GetText(hObj, text, sizeof(text));

	// ENABLE TX latches, so it carries an active state the others do not
	if((id == ID_FT8_BTN_ENABLE) && (ft8_tx_armed))
		active = 1;

	if(active)
	{
		face = ATLAS_AMBER_DEEP;
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
		GUI_SetColor(ATLAS_CYAN);
		ink = ATLAS_CYAN_HI;
	}
	else
	{
		GUI_SetColor(face);
		GUI_FillRect(0, 0, w - 1, h - 1);
		GUI_SetColor(edge);
	}

	GUI_DrawRect(0, 0, w - 1, h - 1);

	// Face text, centred with the atlas letter spacing
	GUI_SetTextMode(GUI_TM_TRANS);
	GUI_SetFont(&GUI_Font16B_1);
	GUI_SetColor(ink);

	tw = atlas_text_width(text, 2);
	atlas_text((w - tw) / 2, (h - 16) / 2, text, 2);

	return 0;
}

//*----------------------------------------------------------------------------
//* Function Name       : ft8_ui_invalidate_all
//* Object              : repaint the whole screen - dialog, both ticking
//*						: child windows and every skinned button
//* Context    			: CONTEXT_VIDEO (gui task)
//*----------------------------------------------------------------------------
static void ft8_ui_invalidate_all(void)
{
	static const int	ids[] =
	{
		ID_FT8_BTN_ENABLE, ID_FT8_BTN_CQ, ID_FT8_BTN_ANSWER,
		ID_FT8_BTN_HALT,   ID_FT8_BTN_LOG, ID_FT8_BTN_BAND
	};
	int	i;

	if(hDesktopFT8 == 0)
		return;

	WM_InvalidateWindow(hDesktopFT8);

	if(hFT8Title)
		WM_InvalidateWindow(hFT8Title);

	if(hFT8Slot)
		WM_InvalidateWindow(hFT8Slot);

	if(hFT8Wf)
		WM_InvalidateWindow(hFT8Wf);

	for(i = 0; i < (int)GUI_COUNTOF(ids); i++)
		WM_InvalidateWindow(WM_GetDialogItem(hDesktopFT8, ids[i]));
}

//*----------------------------------------------------------------------------
//* Function Name       : ft8_ui_on_button
//* Object              : action row presses
//* Notes    			: no backend yet - these move local UI state only,
//*						: so the layout can be judged with the controls
//*						: actually doing something
//* Context    			: CONTEXT_VIDEO (gui task)
//*----------------------------------------------------------------------------
static void ft8_ui_on_button(int id, int ncode)
{
	if(ncode != WM_NOTIFICATION_RELEASED)
		return;

	switch(id)
	{
		case ID_FT8_BTN_ENABLE:
		{
			ft8_proc_tx_arm(!ft8_tx_armed);
			break;
		}

		case ID_FT8_BTN_CQ:
		{
			// Calling CQ means we are not answering anybody. Even slots
			// until the sequencer (WP7) picks the parity from the QSO
			ft8_sel_row = -1;
			ft8_qso_cq();
			break;
		}

		case ID_FT8_BTN_ANSWER:
		{
			// Next CQ in the band activity list, and answer it. Pressing
			// again moves on to the next one. Stand-in for touching a row
			{
				int k, r;

				for(k = 1; k <= ft8_ui_band_n; k++)
				{
					r = (ft8_sel_row + k) % ft8_ui_band_n;

					if(strncmp(ft8_ui_band[r].msg, "CQ ", 3) == 0)
					{
						const FT8_DECODE *d = &ft8_ui_band[r];

						// "hhmmss" - :00 and :30 are the even slots
						uchar parity = (uchar)(((atoi(d->time + 4) % 30) == 0) ? 0 : 1);

						ft8_sel_row = (int8_t)r;
						ft8_qso_answer(d->msg, parity, d->raw_snr);
						break;
					}
				}
			}

			break;
		}

		case ID_FT8_BTN_HALT:
		{
			ft8_proc_tx_halt();
			break;
		}

		case ID_FT8_BTN_LOG:
		{
			// WP8 - until then it runs the decode bench (live rx pauses)
			ft8_proc_request_bench();
			break;
		}

		case ID_FT8_BTN_BAND:
		{
			// Next band with an FT8 frequency - full band change, tuned
			ft8_radio_next_band();
			break;
		}

		default:
			break;
	}

	ft8_ui_invalidate_all();
}

//*----------------------------------------------------------------------------
//* Function Name       : _cbTitle
//* Object              : title strip child window
//* Context    			: CONTEXT_VIDEO (gui task)
//*----------------------------------------------------------------------------
static void _cbTitle(WM_MESSAGE *pMsg)
{
	switch(pMsg->MsgId)
	{
		case WM_PAINT:
			ft8_ui_paint_title();
			break;

		default:
			WM_DefaultProc(pMsg);
			break;
	}
}

//*----------------------------------------------------------------------------
//* Function Name       : _cbSlot
//* Object              : slot bar child window
//* Context    			: CONTEXT_VIDEO (gui task)
//*----------------------------------------------------------------------------
static void _cbSlot(WM_MESSAGE *pMsg)
{
	switch(pMsg->MsgId)
	{
		case WM_PAINT:
			ft8_ui_paint_slot();
			break;

		default:
			WM_DefaultProc(pMsg);
			break;
	}
}

// Only the window is in the static template - the action row is created
// in WM_INIT_DIALOG so the button geometry stays in one place
static const GUI_WIDGET_CREATE_INFO _aDialog[] =
{
	// -----------------------------------------------------------------------------------------------------------
	//							name				id				x	y	xsize		ysize
	// -----------------------------------------------------------------------------------------------------------
	{ WINDOW_CreateIndirect,	"",					ID_FT8_WINDOW,	0,	0,	FT8_UI_W,	FT8_UI_H,	0,	0x64,	0 },
};

//*----------------------------------------------------------------------------
//* Function Name       : _cbDialog
//* Object              : FT8 desktop dialog handler
//* Context    			: CONTEXT_VIDEO (gui task)
//*----------------------------------------------------------------------------
static void _cbDialog(WM_MESSAGE *pMsg)
{
	static const struct
	{
		int			id;
		const char	*text;

	} act[] =
	{
		{ ID_FT8_BTN_ENABLE, "ENABLE TX" },
		{ ID_FT8_BTN_CQ,     "CALL CQ"   },
		{ ID_FT8_BTN_ANSWER, "ANSWER"    },
		{ ID_FT8_BTN_HALT,   "HALT TX"   },
		{ ID_FT8_BTN_LOG,    "LOG"       },
		{ ID_FT8_BTN_BAND,   "BAND"      },
	};

	int	i;

	switch(pMsg->MsgId)
	{
		case WM_INIT_DIALOG:
		{
			// Latch the handle early - GUI_CreateDialogBox has not
			// returned yet, so the static is still 0 at this point
			hDesktopFT8 = pMsg->hWin;

			// Action row
			for(i = 0; i < (int)GUI_COUNTOF(act); i++)
			{
				WM_HWIN	hBtn = BUTTON_CreateEx(FT8_ACT_COL_X(i), FT8_ACT_Y,
											   FT8_ACT_W, FT8_ACT_H,
											   pMsg->hWin, WM_CF_SHOW, 0,
											   act[i].id);

				BUTTON_SetText(hBtn, act[i].text);
				BUTTON_SetSkin(hBtn, ft8_ui_button_skin);
			}

			// The two regions that tick get their own windows so the
			// decode lists are not repainted twice a second
			hFT8Title = WM_CreateWindowAsChild(0, 0, FT8_UI_W, FT8_TITLE_H,
								pMsg->hWin, WM_CF_SHOW, _cbTitle, 0);
			hFT8Slot  = WM_CreateWindowAsChild(FT8_SLOT_X, FT8_SLOT_Y,
								FT8_SLOT_W, FT8_SLOT_WIN_H,
								pMsg->hWin, WM_CF_SHOW, _cbSlot, 0);

			for(i = 0; i < 64; i++)
				ft8_wf_colors[i] = (GUI_COLOR)((0xFFUL << 24) | waterfall_blue[i]);

			hFT8Wf    = WM_CreateWindowAsChild(FT8_WF_X, FT8_WF_Y,
								FT8_WF_W, FT8_WF_WIN_H,
								pMsg->hWin, WM_CF_SHOW, _cbWf, 0);
			ft8_wf_shown = 0;
			ft8_wf_adv_t = xTaskGetTickCount();
			ft8_ui_secs  = 0xFF;

			ft8_ui_tick_slot();
			ft8_proc_get_status(&ft8_ui_live);
			ft8_ui_hist_gen = 0xFFFFFFFF;
			ft8_ui_refresh_lists();

			hFT8Timer = WM_CreateTimer(pMsg->hWin, 0, FT8_UI_TICK_MS, 0);
			break;
		}

		case WM_PAINT:
		{
			// Plain ground - the Atlas grid costs a lot to repaint, and
			// this screen repaints every slot
			atlas_background(0, FT8_TITLE_H, FT8_UI_W, FT8_UI_H - FT8_TITLE_H);

			ft8_ui_paint_list(FT8_LIST_L_X, "BAND ACTIVITY", ft8_ui_band, ft8_ui_band_n, 1);
			ft8_ui_paint_list(FT8_LIST_R_X, "RX FREQUENCY",  ft8_ui_rxf,  ft8_ui_rxf_n,  0);

			ft8_ui_paint_tx();
			break;
		}

		case WM_TIMER:
		{
			uint8_t	was_phase = ft8_phase;
			uchar	was_state = ft8_ui_live.state;
			ulong	gen;

			ft8_ui_tick_slot();
			ft8_proc_get_status(&ft8_ui_live);

			// The transmitter lives in the ft8 task (watchdog may disarm)
			if(ft8_tx_armed != ft8_ui_live.tx_armed)
			{
				ft8_tx_armed = ft8_ui_live.tx_armed;
				ft8_ui_invalidate_all();
			}

			// New decodes - the lists repaint
			if(ft8_ui_refresh_lists())
				ft8_ui_invalidate_all();

			// Clock and slot bar once a second, or when the receiver
			// changes state. That tick repaints nothing else - the
			// waterfall catches up one tick (40 ms) later
			if((ft8_slot_secs != ft8_ui_secs) || (was_state != ft8_ui_live.state))
			{
				ft8_ui_secs = ft8_slot_secs;
				WM_InvalidateWindow(hFT8Title);
				WM_InvalidateWindow(hFT8Slot);
			}
			else
			{
				// Waterfall: one more line when its time has come, or
				// right away when the screen fell behind the writer
				TickType_t now = xTaskGetTickCount();

				ft8_proc_display(&gen);

				// Writer restarted (new session) - start over
				if(gen < ft8_wf_shown)
					ft8_wf_shown = 0;

				if((gen > ft8_wf_shown) &&
				   (((now - ft8_wf_adv_t) >= FT8_UI_WF_LINE_MS) || ((gen - ft8_wf_shown) > FT8_UI_WF_MAX_LAG)))
				{
					// Far behind (screen just opened) - jump to the newest
					if((gen - ft8_wf_shown) > (FT8_DISP_RING - FT8_WF_H))
						ft8_wf_shown = gen;
					else
						ft8_wf_shown++;

					ft8_wf_adv_t = now;
					WM_InvalidateWindow(hFT8Wf);
				}
			}

			if(was_phase != ft8_phase)
				ft8_ui_invalidate_all();

			WM_RestartTimer(pMsg->Data.v, FT8_UI_TICK_MS);
			break;
		}

		case WM_DELETE:
		{
			// Zeroed, a stale handle deleted again later frees whatever emWin
			// reused the number for (hard fault in WM__Paint, handle 7)
			if(hFT8Timer)
				WM_DeleteTimer(hFT8Timer);
			hFT8Timer = 0;

			// Children go with the dialog - drop the handles so a later
			// repaint cannot reach a dead window
			hFT8Title = 0;
			hFT8Slot  = 0;
			hFT8Wf    = 0;
			break;
		}

		case WM_NOTIFY_PARENT:
		{
			ft8_ui_on_button(WM_GetId(pMsg->hWinSrc), pMsg->Data.v);
			break;
		}

		case WM_KEY:
		{
			switch(((WM_KEY_INFO *)(pMsg->Data.p))->Key)
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
			atlas_background(0, 0, FT8_UI_W, FT8_UI_H);
			break;

		default:
			WM_DefaultProc(pMsg);
			break;
	}
}

//*----------------------------------------------------------------------------
//* Function Name       : ft8_ui_set_profile
//* Object              : emWin defaults this screen needs
//* Notes    			: the menu profile leaves WINDOW_SetDefaultBkColor
//*						: at GUI_WHITE (ui_menu.c) and it is a sticky
//*						: global, so a dialog created after the menu has
//*						: ever been entered paints a white face under our
//*						: own drawing
//* Context    			: CONTEXT_VIDEO (gui task)
//*----------------------------------------------------------------------------
static void ft8_ui_set_profile(void)
{
	WINDOW_SetDefaultBkColor(ATLAS_GROUND);
}

//*----------------------------------------------------------------------------
//* Function Name       : ui_desktop_ft8_create
//* Object              : bring the screen up - called by the UI mode
//*						: switch on entry to MODE_DESKTOP_FT8
//* Context    			: CONTEXT_VIDEO (gui task)
//*----------------------------------------------------------------------------
void ui_desktop_ft8_create(void)
{
	ft8_ui_set_profile();

	WM_SetCallback(WM_HBKWIN, &_cbBkWindow);

	hDesktopFT8 = GUI_CreateDialogBox(_aDialog, GUI_COUNTOF(_aDialog), _cbDialog, 0, 0, 0);
}

//*----------------------------------------------------------------------------
//* Function Name       : ui_desktop_ft8_destroy
//* Object              : tear the screen down on the way back to the
//*						: desktop. Safe to call when it was never up
//* Context    			: CONTEXT_VIDEO (gui task)
//*----------------------------------------------------------------------------
void ui_desktop_ft8_destroy(void)
{
	if(hDesktopFT8)
	{
		WM_SetCallback		(WM_HBKWIN, 0);
		WM_InvalidateWindow	(WM_HBKWIN);

		WM_DeleteWindow(hDesktopFT8);

		hDesktopFT8 = 0;
	}
}

#endif
