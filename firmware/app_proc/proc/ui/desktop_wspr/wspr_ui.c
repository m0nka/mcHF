/************************************************************************************
**                                                                                 **
**                                 mcHF QRP Transceiver                            **
**                         Krassi Atanassov - M0NKA, 2013-2026                     **
**                                                                                 **
**---------------------------------------------------------------------------------**
**                                                                                 **
**  File name:		wspr_ui.c                                                      **
**  Description:	WSPR app screen - plain emWin widgets (menu look), layout as   **
**					agreed in claude/ui_proto/wspr_app.html. The beacon and        **
**					receiver logic is in proc/wspr/wspr_app.c                      **
**  Licence:		https://github.com/m0nka/mcHF/blob/main/LICENSE                **
************************************************************************************/
#include "mchf_pro_board.h"
#include "main.h"

#if defined(CONTEXT_VIDEO) && defined(CONTEXT_WSPR)

#include "gui.h"
#include "dialog.h"

#include "rtc.h"

#include "wspr_app.h"
#include "wspr_ui.h"

// FreeRTOS process state
extern struct PROC_STATE			ps;

// UI driver public state
extern struct UI_DRIVER_STATE		ui_s;

// Public radio state
extern struct TRANSCEIVER_STATE_UI	tsu;

#define ID_WSPR_WINDOW				(GUI_ID_USER + 0x10)
#define ID_WSPR_TITLE				(GUI_ID_USER + 0x11)
#define ID_WSPR_LIST				(GUI_ID_USER + 0x14)
#define ID_WSPR_HDR_BAND			(GUI_ID_USER + 0x15)
#define ID_WSPR_BAND				(GUI_ID_USER + 0x16)
#define ID_WSPR_HDR_STATION			(GUI_ID_USER + 0x17)
#define ID_WSPR_LBL_CALL			(GUI_ID_USER + 0x18)
#define ID_WSPR_CALL				(GUI_ID_USER + 0x19)
#define ID_WSPR_LBL_GRID			(GUI_ID_USER + 0x1A)
#define ID_WSPR_GRID				(GUI_ID_USER + 0x1B)
#define ID_WSPR_LBL_POWER			(GUI_ID_USER + 0x1C)
#define ID_WSPR_POWER				(GUI_ID_USER + 0x1D)
#define ID_WSPR_HDR_TX				(GUI_ID_USER + 0x1E)
#define ID_WSPR_TX_EN				(GUI_ID_USER + 0x1F)
#define ID_WSPR_LBL_RATIO			(GUI_ID_USER + 0x20)
#define ID_WSPR_RATIO				(GUI_ID_USER + 0x21)
#define ID_WSPR_LBL_OFS				(GUI_ID_USER + 0x22)
#define ID_WSPR_OFS					(GUI_ID_USER + 0x23)
#define ID_WSPR_BTN_MON				(GUI_ID_USER + 0x24)
#define ID_WSPR_BTN_TXNEXT			(GUI_ID_USER + 0x25)
#define ID_WSPR_BTN_CLEAR			(GUI_ID_USER + 0x26)
#define ID_WSPR_STATUS				(GUI_ID_USER + 0x27)

#define WSPR_UI_TICK_MS				250

// Colours - custom ones through GUI_MAKE_COLOR (ARGB build, a raw value
// would draw fully transparent)
#define WSPR_PINK					GUI_MAKE_COLOR(0x00C1B6FF)		// menu title bar
#define WSPR_RX_BAR					GUI_MAKE_COLOR(0x00D08F3A)
#define WSPR_BAR_BK					GUI_MAKE_COLOR(0x00EEEEEE)

// Dropdowns - closed box, one row of the open list
#define WSPR_DD_H					36
#define WSPR_DD_ROW					34

// Painted children - title info (dial, UTC) and the cycle bar. Like the
// FT8 and MarsChat screens, what changes every tick is drawn in WM_PAINT
// and only invalidated, a widget setter reallocates its text in the emWin
// heap on every call
#define WSPR_INFO_X					206
#define WSPR_INFO_Y					3
#define WSPR_INFO_W					588
#define WSPR_INFO_H					30
#define WSPR_BAR_X					8
#define WSPR_BAR_Y					42
#define WSPR_BAR_W					524
#define WSPR_BAR_H					28
#define WSPR_BAR_MAX				119

// Right panel
#define WSPR_RP_X					544
#define WSPR_RP_W					248
#define WSPR_RP_FX					616			// field column
#define WSPR_RP_FW					176

static const GUI_WIDGET_CREATE_INFO _aDialog[] =
{
	{ WINDOW_CreateIndirect,	"",					ID_WSPR_WINDOW,			0,				0,		800,			480,	0, 0x0, 0 },
	//
	// Title bar, menu style - no exit button, the F key closes the app
	{ TEXT_CreateIndirect,		"WSPR",				ID_WSPR_TITLE,			6,				3,		200,			30,		TEXT_CF_LEFT|TEXT_CF_VCENTER	},
	//
	// Spot list (title info and cycle bar are painted children, see above)
	{ LISTVIEW_CreateIndirect,	NULL,				ID_WSPR_LIST,			8,				78,		524,			346,	0, 0x0, 0 },
	//
	// Band
	{ TEXT_CreateIndirect,		"Band",				ID_WSPR_HDR_BAND,		WSPR_RP_X,		42,		WSPR_RP_W,		20,		TEXT_CF_HCENTER|TEXT_CF_VCENTER	},
	{ DROPDOWN_CreateIndirect,	NULL,				ID_WSPR_BAND,			WSPR_RP_X,		66,		WSPR_RP_W,		WSPR_DD_H + (10 * WSPR_DD_ROW),	0, 0x0, 0 },
	//
	// Station - from settings.ini, read only here
	{ TEXT_CreateIndirect,		"Station",			ID_WSPR_HDR_STATION,	WSPR_RP_X,		110,	WSPR_RP_W,		20,		TEXT_CF_HCENTER|TEXT_CF_VCENTER	},
	{ TEXT_CreateIndirect,		"Call",				ID_WSPR_LBL_CALL,		WSPR_RP_X,		134,	70,				36,		TEXT_CF_LEFT|TEXT_CF_VCENTER	},
	{ TEXT_CreateIndirect,		"",					ID_WSPR_CALL,			WSPR_RP_FX,		134,	WSPR_RP_FW,		36,		TEXT_CF_LEFT|TEXT_CF_VCENTER	},
	{ TEXT_CreateIndirect,		"Grid",				ID_WSPR_LBL_GRID,		WSPR_RP_X,		174,	70,				36,		TEXT_CF_LEFT|TEXT_CF_VCENTER	},
	{ TEXT_CreateIndirect,		"",					ID_WSPR_GRID,			WSPR_RP_FX,		174,	WSPR_RP_FW,		36,		TEXT_CF_LEFT|TEXT_CF_VCENTER	},
	{ TEXT_CreateIndirect,		"Power",			ID_WSPR_LBL_POWER,		WSPR_RP_X,		214,	70,				36,		TEXT_CF_LEFT|TEXT_CF_VCENTER	},
	{ DROPDOWN_CreateIndirect,	NULL,				ID_WSPR_POWER,			WSPR_RP_FX,		214,	WSPR_RP_FW,		WSPR_DD_H + (WSPR_APP_POWERS * WSPR_DD_ROW), 0, 0x0, 0 },
	//
	// Transmit - the two lower dropdowns open upwards, no room below
	{ TEXT_CreateIndirect,		"Transmit",			ID_WSPR_HDR_TX,			WSPR_RP_X,		258,	WSPR_RP_W,		20,		TEXT_CF_HCENTER|TEXT_CF_VCENTER	},
	{ CHECKBOX_CreateIndirect,	"",					ID_WSPR_TX_EN,			WSPR_RP_X,		282,	WSPR_RP_W,		30,		0, 0x0, 0 },
	{ TEXT_CreateIndirect,		"TX ratio",			ID_WSPR_LBL_RATIO,		WSPR_RP_X,		314,	70,				36,		TEXT_CF_LEFT|TEXT_CF_VCENTER	},
	{ DROPDOWN_CreateIndirect,	NULL,				ID_WSPR_RATIO,			WSPR_RP_FX,		314,	WSPR_RP_FW,		WSPR_DD_H + (WSPR_APP_RATIOS * WSPR_DD_ROW), DROPDOWN_CF_UP, 0x0, 0 },
	{ TEXT_CreateIndirect,		"Offset",			ID_WSPR_LBL_OFS,		WSPR_RP_X,		354,	70,				36,		TEXT_CF_LEFT|TEXT_CF_VCENTER	},
	{ DROPDOWN_CreateIndirect,	NULL,				ID_WSPR_OFS,			WSPR_RP_FX,		354,	WSPR_RP_FW,		WSPR_DD_H + (WSPR_APP_OFFSETS * WSPR_DD_ROW), DROPDOWN_CF_UP, 0x0, 0 },
	//
	// Buttons and status line
	{ BUTTON_CreateIndirect,	"Monitor",			ID_WSPR_BTN_MON,		8,				432,	150,			42,		0, 0x0, 0 },
	{ BUTTON_CreateIndirect,	"TX next",			ID_WSPR_BTN_TXNEXT,		166,			432,	150,			42,		0, 0x0, 0 },
	{ BUTTON_CreateIndirect,	"Clear",			ID_WSPR_BTN_CLEAR,		324,			432,	150,			42,		0, 0x0, 0 },
	{ TEXT_CreateIndirect,		"",					ID_WSPR_STATUS,			484,			432,	308,			42,		TEXT_CF_RIGHT|TEXT_CF_VCENTER	},
};

// Spot list columns
static const struct
{
	const char	*name;
	uchar		width;
	uchar		numeric;			// sort as a number

} wspr_cols[] =
{
	{ "UTC",		46,		0 },
	{ "dB",			36,		1 },
	{ "DT",			40,		0 },
	{ "Freq MHz",	96,		0 },
	{ "Dr",			30,		1 },
	{ "Call",		80,		0 },
	{ "Grid",		50,		0 },
	{ "dBm",		40,		1 },
	{ "km",			58,		1 },
};

#define WSPR_COLS					9

// Radio band names, BAND_MODE_xx
static const char * const wspr_band_names[MAX_BANDS] =
{
	"2200m", "630m", "160m", "80m", "60m", "40m", "30m", "20m", "17m", "15m", "12m", "10m"
};

static WM_HWIN			hWsprDlg   = 0;
static WM_HTIMER		hWsprTimer = 0;

// Dropdown row -> radio band
static uchar			wspr_band_row[MAX_BANDS];
static uchar			wspr_band_rows = 0;

// Set while the screen moves a widget itself, its notifications are not
// operator input then
static uchar			wspr_ui_busy = 0;

static ulong			wspr_ui_gen  = 0xFFFFFFFF;
static WSPR_SPOT		wspr_ui_spots[WSPR_APP_MAX_SPOTS];

static WM_HWIN			hWsprInfo  = 0;
static WM_HWIN			hWsprBar   = 0;

// What is on screen - the tick touches a window only when its part changed
static struct
{
	char		info[64];
	char		bar_txt[64];
	int			bar_val;
	GUI_COLOR	bar_col;
	char		status[64];
	uchar		status_red;
	uchar		monitor;
	uchar		tx_ok;

} wspr_ui_shown;

//*----------------------------------------------------------------------------
//* Function Name       : wspr_ui_band_sel
//* Object              : dropdown row of the current band
//*----------------------------------------------------------------------------
static int wspr_ui_band_sel(void)
{
	int i;

	for(i = 0; i < wspr_band_rows; i++)
	{
		if(wspr_band_row[i] == tsu.curr_band)
			return i;
	}

	return 0;
}

//*----------------------------------------------------------------------------
//* Function Name       : wspr_ui_fmt_freq
//* Object              : " 7.038 600" - fixed width, so the column sorts as
//*						: text (small printf, no float)
//*----------------------------------------------------------------------------
static void wspr_ui_fmt_freq(char *buf, int len, ulong hz)
{
	snprintf(buf, len, "%2u.%03u %03u", (uint)(hz / 1000000), (uint)((hz / 1000) % 1000), (uint)(hz % 1000));
}

//*----------------------------------------------------------------------------
//* Function Name       : wspr_ui_dropdown
//* Object              : style and fill one dropdown
//*----------------------------------------------------------------------------
static void wspr_ui_dropdown(WM_HWIN hDlg, int id)
{
	WM_HWIN h = WM_GetDialogItem(hDlg, id);

	DROPDOWN_SetFont(h, &GUI_Font20_1);
	DROPDOWN_SetTextHeight(h, WSPR_DD_H - 2);
	DROPDOWN_SetItemSpacing(h, (WSPR_DD_ROW - GUI_Font20_1.YSize));
}

//*----------------------------------------------------------------------------
//* Function Name       : wspr_ui_init
//* Object              : style and fill everything, WM_INIT_DIALOG
//*----------------------------------------------------------------------------
static void wspr_ui_init(WM_HWIN hDlg)
{
	static const int	hdrs[]	 = { ID_WSPR_HDR_BAND, ID_WSPR_HDR_STATION, ID_WSPR_HDR_TX };
	static const int	labels[] = { ID_WSPR_LBL_CALL, ID_WSPR_LBL_GRID, ID_WSPR_LBL_POWER, ID_WSPR_LBL_RATIO, ID_WSPR_LBL_OFS };
	WSPR_APP_CFG		cfg;
	WM_HWIN				h;
	HEADER_Handle		hHdr;
	char				buf[32];
	int					i;

	wspr_app_get_cfg(&cfg);

	WINDOW_SetBkColor(hDlg, GUI_WHITE);

	// Title bar
	h = WM_GetDialogItem(hDlg, ID_WSPR_TITLE);
	TEXT_SetFont(h, &GUI_Font24B_1);
	TEXT_SetBkColor(h, WSPR_PINK);
	TEXT_SetTextColor(h, GUI_WHITE);
	TEXT_SetText(h, "  WSPR");

	// Section headers and labels
	for(i = 0; i < (int)GUI_COUNTOF(hdrs); i++)
	{
		h = WM_GetDialogItem(hDlg, hdrs[i]);
		TEXT_SetFont(h, &GUI_Font16_1);
		TEXT_SetBkColor(h, GUI_LIGHTBLUE);
		TEXT_SetTextColor(h, GUI_WHITE);
	}

	for(i = 0; i < (int)GUI_COUNTOF(labels); i++)
	{
		h = WM_GetDialogItem(hDlg, labels[i]);
		TEXT_SetFont(h, &GUI_Font16_1);
		TEXT_SetTextColor(h, GUI_BLACK);
	}

	// Station, read only - typed into settings.ini on a PC
	h = WM_GetDialogItem(hDlg, ID_WSPR_CALL);
	TEXT_SetFont(h, &GUI_Font24B_1);
	TEXT_SetTextColor(h, (tsu.station_call[0] != 0) ? GUI_DARKBLUE : GUI_RED);
	TEXT_SetText(h, (tsu.station_call[0] != 0) ? tsu.station_call : "not set");

	h = WM_GetDialogItem(hDlg, ID_WSPR_GRID);
	TEXT_SetFont(h, &GUI_Font24B_1);
	TEXT_SetTextColor(h, (tsu.station_grid[0] != 0) ? GUI_DARKBLUE : GUI_RED);
	TEXT_SetText(h, (tsu.station_grid[0] != 0) ? tsu.station_grid : "not set");

	// Spot list - vertical scroll bar comes and goes with the row count
	h = WM_GetDialogItem(hDlg, ID_WSPR_LIST);
	LISTVIEW_SetFont(h, &GUI_Font16_1);
	LISTVIEW_SetRowHeight(h, 26);
	LISTVIEW_SetHeaderHeight(h, 24);
	LISTVIEW_SetGridVis(h, 0);
	LISTVIEW_SetTextColor(h, LISTVIEW_CI_UNSEL, GUI_BLACK);
	LISTVIEW_SetBkColor(h, LISTVIEW_CI_UNSEL, GUI_WHITE);
	LISTVIEW_SetAutoScrollV(h, 1);

	hHdr = LISTVIEW_GetHeader(h);
	HEADER_SetFont(hHdr, &GUI_Font16_1);
	HEADER_SetTextColor(hHdr, GUI_WHITE);

	for(i = 0; i < WSPR_COLS; i++)
	{
		LISTVIEW_AddColumn(h, wspr_cols[i].width, wspr_cols[i].name, GUI_TA_LEFT | GUI_TA_VCENTER);
		LISTVIEW_SetCompareFunc(h, i, wspr_cols[i].numeric ? LISTVIEW_CompareDec : LISTVIEW_CompareText);
	}

	// Tap a column header to sort, again to reverse
	LISTVIEW_EnableSort(h);

	wspr_ui_busy = 1;

	// Band - only the bands with a WSPR frequency
	h = WM_GetDialogItem(hDlg, ID_WSPR_BAND);
	wspr_ui_dropdown(hDlg, ID_WSPR_BAND);

	wspr_band_rows = 0;
	for(i = 0; i < MAX_BANDS; i++)
	{
		if(!wspr_app_band_ok(i))
			continue;

		snprintf(buf, sizeof(buf), "%s  ", wspr_band_names[i]);
		wspr_ui_fmt_freq(buf + strlen(buf), sizeof(buf) - strlen(buf), wspr_app_band_dial(i));
		DROPDOWN_AddString(h, buf);

		wspr_band_row[wspr_band_rows++] = (uchar)i;
	}

	DROPDOWN_SetListHeight(h, wspr_band_rows * WSPR_DD_ROW);
	DROPDOWN_SetSel(h, wspr_ui_band_sel());

	h = WM_GetDialogItem(hDlg, ID_WSPR_POWER);
	wspr_ui_dropdown(hDlg, ID_WSPR_POWER);
	for(i = 0; i < WSPR_APP_POWERS; i++)
		DROPDOWN_AddString(h, wspr_app_power_text(i));
	DROPDOWN_SetSel(h, cfg.power);

	h = WM_GetDialogItem(hDlg, ID_WSPR_RATIO);
	wspr_ui_dropdown(hDlg, ID_WSPR_RATIO);
	for(i = 0; i < WSPR_APP_RATIOS; i++)
		DROPDOWN_AddString(h, wspr_app_ratio_text(i));
	DROPDOWN_SetSel(h, cfg.tx_ratio);

	h = WM_GetDialogItem(hDlg, ID_WSPR_OFS);
	wspr_ui_dropdown(hDlg, ID_WSPR_OFS);
	for(i = 0; i < WSPR_APP_OFFSETS; i++)
		DROPDOWN_AddString(h, wspr_app_offset_text(i));
	DROPDOWN_SetSel(h, cfg.offset);

	h = WM_GetDialogItem(hDlg, ID_WSPR_TX_EN);
	CHECKBOX_SetFont(h, &GUI_Font16_1);
	CHECKBOX_SetText(h, "Enable TX");
	CHECKBOX_SetState(h, cfg.tx_enable);

	wspr_ui_busy = 0;

	for(i = ID_WSPR_BTN_MON; i <= ID_WSPR_BTN_CLEAR; i++)
		BUTTON_SetFont(WM_GetDialogItem(hDlg, i), &GUI_Font20B_1);

	h = WM_GetDialogItem(hDlg, ID_WSPR_STATUS);
	TEXT_SetFont(h, &GUI_Font16_1);
	TEXT_SetTextColor(h, GUI_DARKGRAY);
}

//*----------------------------------------------------------------------------
//* Function Name       : wspr_ui_fill_list
//* Object              : rebuild the spot list from the app, newest first
//*						: (an active column sort re-applies on its own)
//*----------------------------------------------------------------------------
static void wspr_ui_fill_list(WM_HWIN hDlg)
{
	WM_HWIN				h = WM_GetDialogItem(hDlg, ID_WSPR_LIST);
	static char			txt[WSPR_COLS][14];
	GUI_ConstString		row[WSPR_COLS];
	WSPR_SPOT			*s;
	ushort				n, i;
	int					c, dt;

	n = wspr_app_get_spots(wspr_ui_spots, WSPR_APP_MAX_SPOTS);

	for(c = 0; c < WSPR_COLS; c++)
		row[c] = txt[c];

	LISTVIEW_DeleteAllRows(h);

	for(i = 0; i < n; i++)
	{
		s = &wspr_ui_spots[i];

		dt = (s->dt10 < 0) ? -s->dt10 : s->dt10;

		snprintf(txt[0], 14, "%02d%02d", s->utc / 100, s->utc % 100);
		snprintf(txt[1], 14, "%d", s->snr);
		snprintf(txt[2], 14, "%c%d.%d", (s->dt10 < 0) ? '-' : ' ', dt / 10, dt % 10);
		wspr_ui_fmt_freq(txt[3], 14, s->freq_hz);
		snprintf(txt[4], 14, "%d", s->drift);
		snprintf(txt[5], 14, "%s", s->call);
		snprintf(txt[6], 14, "%s", s->grid);
		snprintf(txt[7], 14, "%d", s->dbm);

		if(s->km != 0xFFFF)
			snprintf(txt[8], 14, "%u", (uint)s->km);
		else
			txt[8][0] = 0;

		LISTVIEW_AddRow(h, row);
	}

}

//*----------------------------------------------------------------------------
//* Function Name       : wspr_ui_tick
//* Object              : clock, cycle bar, status, new spots
//*----------------------------------------------------------------------------
static void wspr_ui_tick(WM_HWIN hDlg)
{
	RTC_TimeTypeDef		tm = {0};
	RTC_DateTypeDef		dt = {0};
	WSPR_APP_STATUS		st;
	WSPR_APP_CFG		cfg;
	WM_HWIN				h;
	GUI_COLOR			col;
	char				buf[64], f[16];
	int					sec;
	uchar				red;

	wspr_app_get_status(&st);
	wspr_app_get_cfg(&cfg);

	k_GetTime(&tm);
	k_GetDate(&dt);

	// Title - dial and UTC, repainted once a second
	wspr_ui_fmt_freq(f, sizeof(f), wspr_app_band_dial(tsu.curr_band));
	snprintf(buf, sizeof(buf), "%s MHz     %02d:%02d:%02d UTC  ", f, tm.Hours, tm.Minutes, tm.Seconds);
	if(strcmp(buf, wspr_ui_shown.info) != 0)
	{
		strcpy(wspr_ui_shown.info, buf);
		WM_InvalidateWindow(hWsprInfo);
	}

	// Cycle bar
	sec = st.cycle_sec;

	switch(st.phase)
	{
		case WSPR_PH_TX:
			col = GUI_RED;
			if((sec < 1) || (sec > 111))
				snprintf(buf, sizeof(buf), "TX");
			else
				snprintf(buf, sizeof(buf), "TX  %d dBm  %d / 110 s", wspr_app_power_dbm(cfg.power), sec - 1);
			break;

		case WSPR_PH_RX:
			col = WSPR_RX_BAR;
			snprintf(buf, sizeof(buf), "RX  %d / 114 s", (sec > 114) ? 114 : sec);
			break;

		case WSPR_PH_DECODE:
			col = WSPR_RX_BAR;
			snprintf(buf, sizeof(buf), "Decoding...");
			break;

		case WSPR_PH_WAIT:
			col = WSPR_BAR_BK;
			snprintf(buf, sizeof(buf), "Waiting for the even minute  %d s", 120 - sec);
			break;

		default:
			col = WSPR_BAR_BK;
			snprintf(buf, sizeof(buf), "Idle");
			break;
	}

	if(sec < 0)
		sec = 0;
	if(sec > WSPR_BAR_MAX)
		sec = WSPR_BAR_MAX;

	if((sec != wspr_ui_shown.bar_val) || (col != wspr_ui_shown.bar_col) || (strcmp(buf, wspr_ui_shown.bar_txt) != 0))
	{
		wspr_ui_shown.bar_val = sec;
		wspr_ui_shown.bar_col = col;
		strcpy(wspr_ui_shown.bar_txt, buf);
		WM_InvalidateWindow(hWsprBar);
	}

	// Status line
	switch(st.tx_block)
	{
		case WSPR_TX_NO_STATION:
			snprintf(buf, sizeof(buf), "No TX: set call/grid in settings.ini");
			break;
		case WSPR_TX_BAD_STATION:
			snprintf(buf, sizeof(buf), "No TX: call/grid not valid for WSPR");
			break;
		case WSPR_TX_NO_CLOCK:
			snprintf(buf, sizeof(buf), "No TX: clock not set");
			break;
		default:
			if(st.tx_next)
				snprintf(buf, sizeof(buf), "TX next cycle  |  %u spots", st.spot_count);
			else
				snprintf(buf, sizeof(buf), "%u spots, %u TX", st.spot_count, st.tx_count);
			break;
	}

	red = (st.tx_block != WSPR_TX_OK) ? 1 : 0;
	if((red != wspr_ui_shown.status_red) || (strcmp(buf, wspr_ui_shown.status) != 0))
	{
		wspr_ui_shown.status_red = red;
		strcpy(wspr_ui_shown.status, buf);

		h = WM_GetDialogItem(hDlg, ID_WSPR_STATUS);
		TEXT_SetTextColor(h, red ? GUI_RED : GUI_DARKGRAY);
		TEXT_SetText(h, buf);
	}

	if(cfg.monitor != wspr_ui_shown.monitor)
	{
		wspr_ui_shown.monitor = cfg.monitor;
		BUTTON_SetText(WM_GetDialogItem(hDlg, ID_WSPR_BTN_MON), cfg.monitor ? "Monitor ON" : "Monitor OFF");
	}

	// TX next only makes sense when we may transmit
	if((!red) != wspr_ui_shown.tx_ok)
	{
		wspr_ui_shown.tx_ok = !red;

		h = WM_GetDialogItem(hDlg, ID_WSPR_BTN_TXNEXT);
		if(!red)
			WM_EnableWindow(h);
		else
			WM_DisableWindow(h);
	}

	// New spots
	if(st.spot_gen != wspr_ui_gen)
	{
		wspr_ui_gen = st.spot_gen;
		wspr_ui_fill_list(hDlg);
	}
}

//*----------------------------------------------------------------------------
//* Function Name       : wspr_ui_on_notify
//* Object              : operator input
//*----------------------------------------------------------------------------
static void wspr_ui_on_notify(WM_HWIN hDlg, int id, int ncode)
{
	WSPR_APP_CFG	cfg;
	WM_HWIN			h;
	int				sel;

	if(wspr_ui_busy)
		return;

	wspr_app_get_cfg(&cfg);

	switch(id)
	{
		case ID_WSPR_BAND:
		{
			if(ncode != WM_NOTIFICATION_SEL_CHANGED)
				return;

			h	= WM_GetDialogItem(hDlg, ID_WSPR_BAND);
			sel = DROPDOWN_GetSel(h);

			// Refused while transmitting - put the selection back
			if((sel < 0) || (sel >= wspr_band_rows) || !wspr_app_set_band(wspr_band_row[sel]))
			{
				wspr_ui_busy = 1;
				DROPDOWN_SetSel(h, wspr_ui_band_sel());
				wspr_ui_busy = 0;
			}
			return;
		}

		case ID_WSPR_POWER:
		case ID_WSPR_RATIO:
		case ID_WSPR_OFS:
		{
			if(ncode != WM_NOTIFICATION_SEL_CHANGED)
				return;

			sel = DROPDOWN_GetSel(WM_GetDialogItem(hDlg, id));
			if(sel < 0)
				return;

			if(id == ID_WSPR_POWER)
				cfg.power = (uchar)sel;
			else if(id == ID_WSPR_RATIO)
				cfg.tx_ratio = (uchar)sel;
			else
				cfg.offset = (uchar)sel;
			break;
		}

		case ID_WSPR_TX_EN:
		{
			if(ncode != WM_NOTIFICATION_VALUE_CHANGED)
				return;

			cfg.tx_enable = (uchar)CHECKBOX_GetState(WM_GetDialogItem(hDlg, id));
			break;
		}

		case ID_WSPR_BTN_MON:
		{
			if(ncode != WM_NOTIFICATION_RELEASED)
				return;

			cfg.monitor = !cfg.monitor;
			break;
		}

		case ID_WSPR_BTN_TXNEXT:
		{
			if(ncode == WM_NOTIFICATION_RELEASED)
				wspr_app_tx_next();
			return;
		}

		case ID_WSPR_BTN_CLEAR:
		{
			if(ncode == WM_NOTIFICATION_RELEASED)
				wspr_app_clear_spots();
			return;
		}

		default:
			return;
	}

	wspr_app_set_cfg(&cfg);
}

//*----------------------------------------------------------------------------
//* Function Name       : _cbInfo / _cbBar
//* Object              : the two painted children. Fills, not GUI_Clear() -
//*						: a clear does nothing in LCD_DRAWMODE_TRANS
//* Context    			: CONTEXT_VIDEO (gui task)
//*----------------------------------------------------------------------------
static void _cbInfo(WM_MESSAGE *pMsg)
{
	GUI_RECT r;

	if(pMsg->MsgId != WM_PAINT)
	{
		WM_DefaultProc(pMsg);
		return;
	}

	WM_GetClientRect(&r);

	GUI_SetColor(WSPR_PINK);
	GUI_FillRectEx(&r);

	GUI_SetFont(&GUI_Font20_1);
	GUI_SetColor(GUI_WHITE);
	GUI_SetTextMode(GUI_TM_TRANS);
	GUI_DispStringInRect(wspr_ui_shown.info, &r, GUI_TA_RIGHT | GUI_TA_VCENTER);
}

static void _cbBar(WM_MESSAGE *pMsg)
{
	GUI_RECT	r;
	int			x;

	if(pMsg->MsgId != WM_PAINT)
	{
		WM_DefaultProc(pMsg);
		return;
	}

	WM_GetClientRect(&r);

	// Filled part, then the rest
	x = r.x0 + ((r.x1 - r.x0 + 1) * wspr_ui_shown.bar_val) / WSPR_BAR_MAX;

	GUI_SetColor(wspr_ui_shown.bar_col);
	if(x > r.x0)
		GUI_FillRect(r.x0, r.y0, x - 1, r.y1);

	GUI_SetColor(WSPR_BAR_BK);
	if(x <= r.x1)
		GUI_FillRect(x, r.y0, r.x1, r.y1);

	GUI_SetFont(&GUI_Font16B_1);
	GUI_SetColor(GUI_BLACK);
	GUI_SetTextMode(GUI_TM_TRANS);
	GUI_DispStringInRect(wspr_ui_shown.bar_txt, &r, GUI_TA_HCENTER | GUI_TA_VCENTER);
}

//*----------------------------------------------------------------------------
//* Function Name       : _cbDialog
//* Context    			: CONTEXT_VIDEO (gui task)
//*----------------------------------------------------------------------------
static void _cbDialog(WM_MESSAGE *pMsg)
{
	switch(pMsg->MsgId)
	{
		case WM_INIT_DIALOG:
		{
			// Latch early - GUI_CreateDialogBox has not returned yet
			hWsprDlg = pMsg->hWin;

			wspr_ui_init(pMsg->hWin);

			// Nothing shown yet - the first tick fills every part
			memset(&wspr_ui_shown, 0, sizeof(wspr_ui_shown));
			wspr_ui_shown.bar_val	 = -1;
			wspr_ui_shown.status_red = 0xFF;
			wspr_ui_shown.monitor	 = 0xFF;
			wspr_ui_shown.tx_ok		 = 0xFF;

			hWsprInfo = WM_CreateWindowAsChild(WSPR_INFO_X, WSPR_INFO_Y, WSPR_INFO_W, WSPR_INFO_H, pMsg->hWin, WM_CF_SHOW, _cbInfo, 0);
			hWsprBar  = WM_CreateWindowAsChild(WSPR_BAR_X,  WSPR_BAR_Y,  WSPR_BAR_W,  WSPR_BAR_H,  pMsg->hWin, WM_CF_SHOW, _cbBar,  0);

			wspr_ui_gen = 0xFFFFFFFF;
			wspr_ui_tick(pMsg->hWin);

			hWsprTimer = WM_CreateTimer(pMsg->hWin, 0, WSPR_UI_TICK_MS, 0);
			break;
		}

		case WM_TIMER:
		{
			wspr_ui_tick(pMsg->hWin);

			WM_RestartTimer(pMsg->Data.v, WSPR_UI_TICK_MS);
			break;
		}

		case WM_DELETE:
		{
			// Zeroed - a stale handle deleted again later frees whatever
			// emWin reused the number for
			if(hWsprTimer)
				WM_DeleteTimer(hWsprTimer);
			hWsprTimer = 0;

			// The children go with the dialog, only forget them
			hWsprInfo = 0;
			hWsprBar  = 0;
			break;
		}

		case WM_NOTIFY_PARENT:
		{
			wspr_ui_on_notify(pMsg->hWin, WM_GetId(pMsg->hWinSrc), pMsg->Data.v);
			break;
		}

		case WM_KEY:
		{
			switch(((WM_KEY_INFO *)(pMsg->Data.p))->Key)
			{
				// Back to the radio. Scheduled - the mode switch deletes us
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
//* Object              : white behind the dialog. Not GUI_Clear() - the
//*						: driver runs in LCD_DRAWMODE_TRANS, a clear does
//*						: nothing there
//*----------------------------------------------------------------------------
static void _cbBkWindow(WM_MESSAGE *pMsg)
{
	switch(pMsg->MsgId)
	{
		case WM_PAINT:
			GUI_SetColor(GUI_WHITE);
			GUI_FillRect(0, 0, 799, 479);
			break;

		default:
			WM_DefaultProc(pMsg);
			break;
	}
}

//*----------------------------------------------------------------------------
//* Function Name       : wspr_ui_create
//* Object              : called by the UI mode switch on MODE_DESKTOP_WSPR
//* Context    			: CONTEXT_VIDEO (gui task)
//*----------------------------------------------------------------------------
void wspr_ui_create(void)
{
	WM_SetCallback(WM_HBKWIN, &_cbBkWindow);

	hWsprDlg = GUI_CreateDialogBox(_aDialog, GUI_COUNTOF(_aDialog), _cbDialog, 0, 0, 0);
}

//*----------------------------------------------------------------------------
//* Function Name       : wspr_ui_destroy
//* Object              : back to the desktop. Safe when never created
//* Context    			: CONTEXT_VIDEO (gui task)
//*----------------------------------------------------------------------------
void wspr_ui_destroy(void)
{
	if(hWsprDlg)
	{
		WM_SetCallback		(WM_HBKWIN, 0);
		WM_InvalidateWindow	(WM_HBKWIN);

		WM_DeleteWindow(hWsprDlg);

		hWsprDlg = 0;
	}
}

#endif
