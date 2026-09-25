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

// UI driver public state
extern struct	UI_DRIVER_STATE			ui_s;
extern struct	PROC_STATE				ps;
extern struct	TRANSCEIVER_STATE_UI	tsu;

WM_HWIN				hDesktopFT8 = 0;

static WM_HWIN		hFT8Title;					// title strip, owns the clock
static WM_HWIN		hFT8Slot;					// slot bar, owns the countdown
static WM_HTIMER	hFT8Timer;

// ---------------------------------------------------------------------
// Screen state
//
// Only tx_armed and sel_row are decisions the operator has made; the
// rest is derived from the RTC on every tick. Nothing here survives a
// screen teardown yet - once there is a decoder the decode lists will
// have to move into the FT8 task and outlive this dialog, the way the
// MarsChat history does

static uint8_t		ft8_tx_armed = 0;			// transmitter armed
static int8_t		ft8_sel_row  = -1;			// selected decode, -1 = none
static uint8_t		ft8_band_idx = 0;

// Slot phase, recomputed each tick from the RTC
#define FT8_PHASE_RX			0
#define FT8_PHASE_TX			1

static uint8_t		ft8_phase;
static uint8_t		ft8_slot_secs;				// seconds elapsed into the current slot

// ---------------------------------------------------------------------
// Demo content
//
// Stand-in for what the decoder will eventually produce. Shaped exactly
// like a real decode so the list painter does not have to change when
// the backend lands: utc, snr in dB, dt in hundredths of a second,
// audio frequency in Hz, and the unpacked message text

typedef struct
{
	char		time[8];
	int8_t		snr;
	int16_t		dt;								// hundredths of a second
	uint16_t	freq;							// audio Hz
	char		msg[24];

} FT8_DECODE;

static const FT8_DECODE	ft8_demo_band[] =
{
	{ "133900", -19,  10, 1199, "K2NRS R5DU -25"   },
	{ "133900", -17, 190,  853, "CQ UR4LBG KN89"   },
	{ "133900",  -6, 210,  905, "CQ OK1HEH JN79"   },
	{ "133900",   0, 230, 1831, "UA6HI E74BYZ R-24"},
	{ "133900", -18, 200, 2064, "GI3SG RX9ATX -25" },
	{ "133900",  -9, 130, 2181, "CQ ER1OO KN46"    },
	{ "133900",  -9, 210, 2260, "SE3X UN7LZ R-08"  },
	{ "133900",  -6, 220,  918, "CQ OH6HPS KP10"   },
	{ "133915", -18, 210, 2134, "CQ E73DN JN93"    },
	{ "133915", -11, 150, 1422, "M0NKA DL2ABC -13" },
	{ "133915",  -3, 240, 1677, "CQ SP9XYZ JO90"   },
};

static const FT8_DECODE	ft8_demo_rxfreq[] =
{
	{ "133915", -11, 150, 1500, "M0NKA DL2ABC -13" },
	{ "133845",  -8, 140, 1500, "M0NKA DL2ABC IO91"},
};

// FT8 dial frequencies, USB carrier. Tapping BAND steps this list
static const struct
{
	const char	*name;
	uint32_t	dial_hz;

} ft8_bands[] =
{
	{ "80M", 3573000  },
	{ "40M", 7074000  },
	{ "30M", 10136000 },
	{ "20M", 14074000 },
	{ "17M", 18100000 },
	{ "15M", 21074000 },
	{ "12M", 24915000 },
	{ "10M", 28074000 },
};

#define FT8_BAND_COUNT		(int)(sizeof(ft8_bands) / sizeof(ft8_bands[0]))

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
//* Function Name       : ft8_ui_sync_band
//* Object              : point the band plan entry at whichever FT8 band
//*						: the VFO is already sitting on, so the screen does
//*						: not open claiming a band the radio is not on
//* Notes    			: leaves the index alone when the VFO is nowhere
//*						: near an FT8 frequency - the title then shows the
//*						: mismatch in amber, which is the useful answer
//* Context    			: CONTEXT_VIDEO (gui task)
//*----------------------------------------------------------------------------
static void ft8_ui_sync_band(void)
{
	ulong	dial = ft8_ui_dial_hz();
	int		i;

	for(i = 0; i < FT8_BAND_COUNT; i++)
	{
		if(ft8_bands[i].dial_hz == dial)
		{
			ft8_band_idx = (uint8_t)i;
			return;
		}
	}
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

	// Even slots are ours by convention when armed. The real sequencer
	// will own this decision - it depends on who called whom
	if((ft8_tx_armed) && (((secs / FT8_SLOT_SECS) & 1) == 0))
		ft8_phase = FT8_PHASE_TX;
	else
		ft8_phase = FT8_PHASE_RX;
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

	// Selected FT8 band and the actual dial. BAND steps the band plan
	// entry but does not retune the VFO yet, so the two can disagree -
	// and when they do the frequency goes amber, because a VFO that is
	// not on the band's FT8 frequency will decode precisely nothing
	snprintf(buf, sizeof(buf), "%s   %u.%06u MHZ", ft8_bands[ft8_band_idx].name,
			(unsigned)(dial / 1000000), (unsigned)(dial % 1000000));

	GUI_SetFont(&GUI_Font16B_1);
	GUI_SetColor((dial == ft8_bands[ft8_band_idx].dial_hz) ? ATLAS_CYAN : ATLAS_AMBER);
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
	char		buf[48];
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
	else if(ft8_slot_secs < FT8_TX_SECS)
	{
		doing = "RX CAPTURE";
		c0    = ATLAS_CYAN;
		c1    = ATLAS_CYAN_DEEP;
		ink   = ATLAS_TEXT;
	}
	else
	{
		doing = "DECODING";
		c0    = ATLAS_CYAN_DEEP;
		c1    = ATLAS_PANEL;
		ink   = ATLAS_TEXT;
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
	snprintf(buf, sizeof(buf), "%s   %d DECODES   NEXT IN %ds", doing,
			(int)(sizeof(ft8_demo_band) / sizeof(ft8_demo_band[0])), remain);
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
		int					frac = d->dt % 100;

		if(frac < 0)
			frac = -frac;

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
		atlas_text(x + FT8_COL_TIME, y + 4, d->time, 0);

		// SNR, coloured by strength
		{
			char	buf[8];

			snprintf(buf, sizeof(buf), "%d", d->snr);
			GUI_SetColor(sel ? ATLAS_TEXT : ft8_ui_snr_colour(d->snr));
			atlas_text(x + FT8_COL_SNR, y + 4, buf, 0);
		}

		// DT and audio frequency
		{
			char	buf[12];

			snprintf(buf, sizeof(buf), "%d.%01d", d->dt / 100, frac / 10);
			GUI_SetColor(sel ? ATLAS_TEXT : ATLAS_DIM);
			atlas_text(x + FT8_COL_DT, y + 4, buf, 0);

			snprintf(buf, sizeof(buf), "%u", (unsigned)d->freq);
			atlas_text(x + FT8_COL_FREQ, y + 4, buf, 0);
		}

		// Message - CQ in amber, everything else in the normal ink
		GUI_SetColor(sel			   ? ATLAS_TEXT  :
					 ft8_ui_is_cq(d->msg) ? ATLAS_AMBER : ATLAS_TEXT);
		atlas_text(x + FT8_COL_MSG, y + 4, d->msg, 0);
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

	// With no sequencer yet this is a fixed string - the real one comes
	// from the QSO state machine (WP7)
	if(ft8_sel_row >= 0)
		msg = "M0NKA DL2ABC -13";
	else
		msg = "CQ M0NKA IO91";

	GUI_SetFont(&GUI_Font20B_1);
	GUI_SetColor(ft8_tx_armed ? ATLAS_TEXT : ATLAS_DIM);
	atlas_text(FT8_TX_X + 52, FT8_TX_Y + 6, msg, 2);

	GUI_SetFont(&GUI_Font13B_1);
	GUI_SetColor(ATLAS_DIM);
	atlas_text_right(FT8_TX_X + FT8_TX_W - 10, FT8_TX_Y + 10,
					ft8_tx_armed ? "SENDS IN THE NEXT EVEN SLOT" : "TRANSMITTER DISARMED", 2);
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
			ft8_tx_armed ^= 1;
			break;
		}

		case ID_FT8_BTN_CQ:
		{
			// Calling CQ means we are not answering anybody
			ft8_sel_row  = -1;
			ft8_tx_armed = 1;
			break;
		}

		case ID_FT8_BTN_ANSWER:
		{
			// Stand-in for "work the decode under the cursor" - steps
			// through the demo list until there is touch hit testing
			// on the rows themselves
			ft8_sel_row++;

			if(ft8_sel_row >= (int)(sizeof(ft8_demo_band) / sizeof(ft8_demo_band[0])))
				ft8_sel_row = -1;

			break;
		}

		case ID_FT8_BTN_HALT:
		{
			ft8_tx_armed = 0;
			break;
		}

		case ID_FT8_BTN_LOG:
		{
			// WP8
			break;
		}

		case ID_FT8_BTN_BAND:
		{
			ft8_band_idx++;

			if(ft8_band_idx >= FT8_BAND_COUNT)
				ft8_band_idx = 0;

			// Does not retune the VFO yet - the band plan table is here
			// so the button has somewhere real to point when it does
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

			ft8_ui_sync_band();
			ft8_ui_tick_slot();

			hFT8Timer = WM_CreateTimer(pMsg->hWin, 0, 500, 0);
			break;
		}

		case WM_PAINT:
		{
			atlas_background(0, FT8_TITLE_H, FT8_UI_W, FT8_UI_H - FT8_TITLE_H);
			atlas_grid(0, FT8_TITLE_H, FT8_UI_W, FT8_UI_H - FT8_TITLE_H, 80);

			ft8_ui_paint_list(FT8_LIST_L_X, "BAND ACTIVITY", ft8_demo_band,
							(int)(sizeof(ft8_demo_band) / sizeof(ft8_demo_band[0])), 1);

			ft8_ui_paint_list(FT8_LIST_R_X, "RX FREQUENCY", ft8_demo_rxfreq,
							(int)(sizeof(ft8_demo_rxfreq) / sizeof(ft8_demo_rxfreq[0])), 0);

			ft8_ui_paint_tx();
			break;
		}

		case WM_TIMER:
		{
			uint8_t	was_phase = ft8_phase;

			ft8_ui_tick_slot();

			// The slot bar and clock always move; the rest of the
			// screen only when the phase actually flips
			WM_InvalidateWindow(hFT8Title);
			WM_InvalidateWindow(hFT8Slot);

			if(was_phase != ft8_phase)
				ft8_ui_invalidate_all();

			WM_RestartTimer(pMsg->Data.v, 500);
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
