/************************************************************************************
**                                                                                 **
**                                 mcHF QRP Transceiver                            **
**                         Krassi Atanassov - M0NKA, 2013-2026                     **
**                                                                                 **
**---------------------------------------------------------------------------------**
**                                                                                 **
**  File name:		dsp_s.c                                                        **
**  Description:	Baseband menu - the UHSDR menu settings that belong to        **
**					the baseband, one tab per group. Every change goes straight   **
**					to the M4 core (ICC_SET_DSP_SETTINGS), the set is saved to     **
**					the virtual eeprom when the menu closes                        **
**  Licence:		https://github.com/m0nka/mcHF/blob/main/LICENSE                **
************************************************************************************/
#include "mchf_pro_board.h"

#ifdef CONTEXT_VIDEO

#include "ui_menu_layout.h"
#include "gui.h"
#include "dialog.h"

#include "ui_menu_module.h"
#include "ui_actions.h"
#include "radio_init.h"

#include "dsp_s.h"

extern GUI_CONST_STORAGE GUI_BITMAP bmicon_compref;

// UI driver public state
extern struct	UI_DRIVER_STATE			ui_s;

// Menu layout definitions from Flash
extern const struct UIMenuLayout menu_layout[];

static void Startup(WM_HWIN hWin, uint16_t xpos, uint16_t ypos);
static void KillDsps(void);

K_ModuleItem_Typedef  dsp_s =
{
  4,
  "Baseband",
  &bmicon_compref,
  Startup,
  NULL,
  KillDsps
};

WM_HWIN   	hDSdialog;

// Set while a page fills its widgets - the widgets notify on creation and
// on every programmatic update, those must not be taken as user changes
static uchar dsm_building = 0;

// Anything changed since the menu opened, save on close
static uchar dsm_modified = 0;

// Page windows, index = DSM_PAGE_xxx. The page number is not taken from the
// window id - a dialog does not get the id of its create info entry, so all
// pages came up empty with that. dsm_init_page tells WM_INIT_DIALOG which
// page it builds (the handle is only known after GUI_CreateDialogBox returns)
static WM_HWIN	dsm_page_win[DSM_PAGE_NUM];
static uchar	dsm_init_page = 0;

typedef struct
{
	uchar				id;			// DSP_SET_xxx
	uchar				page;		// DSM_PAGE_xxx
	uchar				slot;		// column * DSM_ROWS + row
	uchar				kind;		// DSK_xxx
	short				step;		// DSK_SPIN
	const char			*label;
	const char * const	*opts;		// DSK_CHOICE/DSK_LIST, index = value - min

} DSM_ITEM;

static const char * const dsm_opt_hang[]	= { "Auto", "Off", "On" };
static const char * const dsm_opt_comp[]	= { "Off", "0", "1", "2", "3", "4", "5", "6", "7", "8", "9", "10", "11", "12", "Custom" };
static const char * const dsm_opt_ssbf[]	= { "Soprano", "Tenor", "Bass" };
static const char * const dsm_opt_tune[]	= { "Single", "Two tone" };
static const char * const dsm_opt_fmdev[]	= { "2.5 kHz", "5 kHz" };
static const char * const dsm_opt_burst[]	= { "Off", "1750 Hz", "2135 Hz" };

// Same order as fm_subaudible_tone_table[] on the M4 core
static const char * const dsm_opt_ctcss[]	=
{
	"Off",
	"67.0",  "69.3",  "71.9",  "74.4",  "77.0",  "79.7",  "82.5",  "85.4",  "88.5",  "91.5",
	"94.8",  "97.4",  "100.0", "103.5", "107.2", "110.9", "114.8", "118.8", "123.0", "127.3",
	"131.8", "136.5", "141.3", "146.2", "150.0", "151.4", "156.7", "159.8", "162.2", "165.5",
	"167.9", "171.3", "173.8", "177.3", "179.9", "183.5", "186.2", "189.9", "192.8", "196.6",
	"199.5", "203.5", "206.5", "210.7", "213.8", "218.1", "221.3", "225.7", "229.1", "233.6",
	"237.1", "241.8", "245.5", "250.3", "254.1"
};

// The per setting widget id blocks in dsp_s.h are 0x40 wide - fails to
// compile (negative array size) once the settings list outgrows them
typedef char dsm_id_block_check[(DSP_SET_COUNT <= 0x40) ? 1 : -1];

// Six rotated tabs have to fit the 425 pixel height or the multipage adds
// scroll arrows - short names, no padding spaces (with those the 32 pixel
// font did not fit, without them it takes ~380 pixels)
static const char * const dsm_page_name[DSM_PAGE_NUM] = { "AGC", "Noise", "RX", "FM", "TX", "CW" };

static const DSM_ITEM dsm_items[] =
{
	// -----------------------------------------------------------------------------------------------------
	//	id								page				slot	kind			step	label
	// -----------------------------------------------------------------------------------------------------
	// AGC - mode and RF gain (threshold) are on the on-screen AGC dialog
	{ DSP_SET_AGC_SLOPE,				DSM_PAGE_AGC,		0,		DSK_SPIN,		1,		"Slope (dB)",				NULL			},
	{ DSP_SET_AGC_DECAY_SLOW,			DSM_PAGE_AGC,		1,		DSK_SPIN,		10,		"Decay SLOW (ms)",			NULL			},
	{ DSP_SET_AGC_DECAY_MED,			DSM_PAGE_AGC,		2,		DSK_SPIN,		10,		"Decay MEDIUM (ms)",		NULL			},
	{ DSP_SET_AGC_DECAY_FAST,			DSM_PAGE_AGC,		3,		DSK_SPIN,		10,		"Decay FAST (ms)",			NULL			},
	{ DSP_SET_AGC_DECAY_LONG,			DSM_PAGE_AGC,		4,		DSK_SPIN,		10,		"Decay CUSTOM (ms)",		NULL			},
	{ DSP_SET_AGC_HANG_MODE,			DSM_PAGE_AGC,		5,		DSK_CHOICE,		0,		"Hang",						dsm_opt_hang	},
	{ DSP_SET_AGC_HANG_TIME,			DSM_PAGE_AGC,		6,		DSK_SPIN,		10,		"Hang time, On (ms)",		NULL			},
	{ DSP_SET_AGC_HANG_THRESH,			DSM_PAGE_AGC,		7,		DSK_SPIN,		1,		"Hang threshold (dB)",		NULL			},
	{ DSP_SET_AGC_HANG_DECAY,			DSM_PAGE_AGC,		8,		DSK_SPIN,		100,	"Hang decay (ms)",			NULL			},
	// Noise
	{ DSP_SET_NR_ENABLE,				DSM_PAGE_NOISE,		0,		DSK_CHECK,		0,		"Noise reduction",			NULL			},
	{ DSP_SET_NR_STRENGTH,				DSM_PAGE_NOISE,		1,		DSK_SPIN,		5,		"NR strength",				NULL			},
	{ DSP_SET_NB_ENABLE,				DSM_PAGE_NOISE,		2,		DSK_CHECK,		0,		"Noise blanker",			NULL			},
	{ DSP_SET_NB_LEVEL,					DSM_PAGE_NOISE,		3,		DSK_SPIN,		1,		"NB level",					NULL			},
	{ DSP_SET_ANOTCH_ENABLE,			DSM_PAGE_NOISE,		4,		DSK_CHECK,		0,		"Auto notch",				NULL			},
	{ DSP_SET_ANOTCH_RATE,				DSM_PAGE_NOISE,		5,		DSK_SPIN,		1,		"Auto notch rate",			NULL			},
	{ DSP_SET_MNOTCH_ENABLE,			DSM_PAGE_NOISE,		6,		DSK_CHECK,		0,		"Manual notch",				NULL			},
	{ DSP_SET_MNOTCH_FREQ,				DSM_PAGE_NOISE,		7,		DSK_SPIN,		10,		"Notch freq (Hz)",			NULL			},
	{ DSP_SET_MPEAK_ENABLE,				DSM_PAGE_NOISE,		8,		DSK_CHECK,		0,		"Peak filter",				NULL			},
	{ DSP_SET_MPEAK_FREQ,				DSM_PAGE_NOISE,		9,		DSK_SPIN,		10,		"Peak freq (Hz)",			NULL			},
	{ DSP_SET_NR_BETA,					DSM_PAGE_NOISE,		10,		DSK_SPIN,		2,		"NR beta (x1000)",			NULL			},
	{ DSP_SET_NR_ASNR,					DSM_PAGE_NOISE,		11,		DSK_SPIN,		1,		"NR asnr",					NULL			},
	{ DSP_SET_NR_SMOOTH_WIDTH,			DSM_PAGE_NOISE,		12,		DSK_SPIN,		1,		"NR smooth width",			NULL			},
	{ DSP_SET_NR_SMOOTH_THRESH,			DSM_PAGE_NOISE,		13,		DSK_SPIN,		5,		"NR smooth threshold",		NULL			},
	// RX
	{ DSP_SET_SAM_ENABLE,				DSM_PAGE_RX,		0,		DSK_CHECK,		0,		"AM as SyncAM (SAM)",		NULL			},
	{ DSP_SET_SAM_PLL_RANGE,			DSM_PAGE_RX,		1,		DSK_SPIN,		10,		"SAM PLL range (Hz)",		NULL			},
	{ DSP_SET_SAM_PLL_ZETA,				DSM_PAGE_RX,		2,		DSK_SPIN,		1,		"SAM PLL step response",	NULL			},
	{ DSP_SET_SAM_PLL_BW,				DSM_PAGE_RX,		3,		DSK_SPIN,		5,		"SAM PLL bandwidth (Hz)",	NULL			},
	{ DSP_SET_SAM_FADE_LEVELER,			DSM_PAGE_RX,		4,		DSK_CHECK,		0,		"SAM fade leveler",			NULL			},
	{ DSP_SET_IQ_AUTO_CORR,				DSM_PAGE_RX,		5,		DSK_CHECK,		0,		"RX IQ auto correction",	NULL			},
	{ DSP_SET_RX_BASS,					DSM_PAGE_RX,		6,		DSK_SPIN,		1,		"RX bass (dB)",				NULL			},
	{ DSP_SET_RX_TREBLE,				DSM_PAGE_RX,		7,		DSK_SPIN,		1,		"RX treble (dB)",			NULL			},
	// FM
	{ DSP_SET_FM_DEV_5K,				DSM_PAGE_FM,		0,		DSK_CHOICE,		0,		"FM deviation",				dsm_opt_fmdev	},
	{ DSP_SET_FM_SQUELCH,				DSM_PAGE_FM,		1,		DSK_SPIN,		1,		"FM squelch (0 = open)",	NULL			},
	{ DSP_SET_FM_TONE_BURST,			DSM_PAGE_FM,		2,		DSK_CHOICE,		0,		"Tone burst",				dsm_opt_burst	},
	{ DSP_SET_FM_CTCSS_GEN,				DSM_PAGE_FM,		5,		DSK_LIST,		0,		"CTCSS TX (Hz)",			dsm_opt_ctcss	},
	{ DSP_SET_FM_CTCSS_DET,				DSM_PAGE_FM,		7,		DSK_LIST,		0,		"CTCSS RX (Hz)",			dsm_opt_ctcss	},
	// TX
	{ DSP_SET_TX_MIC_GAIN,				DSM_PAGE_TX,		0,		DSK_SPIN,		1,		"Mic gain",					NULL			},
	{ DSP_SET_TX_COMP_LEVEL,			DSM_PAGE_TX,		1,		DSK_CHOICE,		0,		"Speech compressor",		dsm_opt_comp	},
	{ DSP_SET_TX_ALC_RELEASE,			DSM_PAGE_TX,		2,		DSK_SPIN,		1,		"ALC release (Custom)",		NULL			},
	{ DSP_SET_TX_ALC_GAIN,				DSM_PAGE_TX,		3,		DSK_SPIN,		1,		"ALC gain (Custom)",		NULL			},
	{ DSP_SET_TX_SSB_FILTER,			DSM_PAGE_TX,		4,		DSK_CHOICE,		0,		"SSB TX filter",			dsm_opt_ssbf	},
	{ DSP_SET_TX_AM_FILTER,				DSM_PAGE_TX,		5,		DSK_CHECK,		0,		"AM TX filter",				NULL			},
	{ DSP_SET_TX_TUNE_TONE,				DSM_PAGE_TX,		6,		DSK_CHOICE,		0,		"Tune tone",				dsm_opt_tune	},
	{ DSP_SET_TX_BASS,					DSM_PAGE_TX,		7,		DSK_SPIN,		1,		"TX bass (dB)",				NULL			},
	{ DSP_SET_TX_TREBLE,				DSM_PAGE_TX,		8,		DSK_SPIN,		1,		"TX treble (dB)",			NULL			},
	// CW
	{ DSP_SET_CW_SPEED,					DSM_PAGE_CW,		0,		DSK_SPIN,		1,		"Keyer speed (wpm)",		NULL			},
	{ DSP_SET_CW_WEIGHT,				DSM_PAGE_CW,		1,		DSK_SPIN,		5,		"Keyer weight (x100)",		NULL			},
	{ DSP_SET_CW_SIDETONE,				DSM_PAGE_CW,		2,		DSK_SPIN,		10,		"Sidetone/offset (Hz)",		NULL			},
	{ DSP_SET_CW_PADDLE_REV,			DSM_PAGE_CW,		3,		DSK_CHECK,		0,		"Paddle reverse",			NULL			},
	{ DSP_SET_CW_RX_DELAY,				DSM_PAGE_CW,		4,		DSK_SPIN,		1,		"TX to RX delay (x10 ms)",	NULL			},
};

static const GUI_WIDGET_CREATE_INFO _aDialog[] =
{
	// -----------------------------------------------------------------------------------------------------------------------------
	//							name						id					x		y		xsize	ysize	?		?		?
	// -----------------------------------------------------------------------------------------------------------------------------
	// Self
	{ WINDOW_CreateIndirect,	"", 						ID_WINDOW_0,		0,    	0,		800,	430, 	0, 		0x64, 	0 },
};

// One per page, the page number goes in the id
static GUI_WIDGET_CREATE_INFO _aPage[] =
{
	{ WINDOW_CreateIndirect,	"", 						ID_DSM_PAGE,		0,    	0,		DSM_PAGE_X,	DSM_PAGE_Y, 	0, 		0x0, 	0 },
};

static const DSM_ITEM *dsm_find_item(uchar id)
{
	uchar i;

	for(i = 0; i < GUI_COUNTOF(dsm_items); i++)
	{
		if(dsm_items[i].id == id)
			return &dsm_items[i];
	}

	return NULL;
}

//*----------------------------------------------------------------------------
//* Function Name       : dsm_show
//* Object              : put the current value of one setting on its widget
//* Input Parameters    :
//* Output Parameters   :
//* Functions called    :
//*----------------------------------------------------------------------------
static void dsm_show(WM_HWIN hPage, const DSM_ITEM *it)
{
	WM_HWIN hItem = WM_GetDialogItem(hPage, ID_DSM_CTRL + it->id);
	short	val   = dsp_settings[it->id];
	short	idx   = val - dsp_settings_min[it->id];

	if(hItem == 0)
		return;

	dsm_building++;

	switch(it->kind)
	{
		case DSK_SPIN:
			SPINBOX_SetValue(hItem, val);
			break;

		case DSK_CHECK:
			CHECKBOX_SetState(hItem, (val != 0));
			break;

		case DSK_CHOICE:
			TEXT_SetText(hItem, it->opts[idx]);
			break;

		case DSK_LIST:
			LISTBOX_SetSel(hItem, idx);
			break;

		default:
			break;
	}

	dsm_building--;
}

static void dsm_set(uchar id, short val)
{
	if(ui_actions_change_dsp_setting(id, val))
		dsm_modified = 1;
}

//*----------------------------------------------------------------------------
//* Function Name       : dsm_create_item
//* Object              : label + widget(s) of one setting in its page slot
//* Input Parameters    :
//* Output Parameters   :
//* Functions called    :
//*----------------------------------------------------------------------------
static void dsm_create_item(WM_HWIN hPage, const DSM_ITEM *it)
{
	WM_HWIN hItem;
	int		x = DSM_COL_X(it->slot);
	int		y = DSM_ROW_Y(it->slot);
	int		i, num;

	// A checkbox carries its own label
	if(it->kind == DSK_CHECK)
	{
		hItem = CHECKBOX_CreateEx(x, y + 12, DSM_CTRL_X, DSM_CTRL_Y, hPage, WM_CF_SHOW, 0, ID_DSM_CTRL + it->id);
		CHECKBOX_SetFont(hItem, &GUI_Font20_1);
		CHECKBOX_SetText(hItem, it->label);
		dsm_show(hPage, it);
		return;
	}

	hItem = TEXT_CreateEx(x, y, DSM_CTRL_X, DSM_LABEL_Y, hPage, WM_CF_SHOW, TEXT_CF_HCENTER|TEXT_CF_VCENTER, 0, it->label);
	TEXT_SetFont(hItem, &GUI_Font16_1);
	TEXT_SetBkColor(hItem, GUI_LIGHTBLUE);
	TEXT_SetTextColor(hItem, GUI_WHITE);

	y += (DSM_LABEL_Y + 2);

	switch(it->kind)
	{
		case DSK_SPIN:
		{
			hItem = SPINBOX_CreateEx(x, y, DSM_CTRL_X, DSM_CTRL_Y, hPage, WM_CF_SHOW, ID_DSM_CTRL + it->id,
									 dsp_settings_min[it->id], dsp_settings_max[it->id]);
			SPINBOX_SetEdge(hItem, SPINBOX_EDGE_CENTER);
			SPINBOX_SetButtonSize(hItem, 60);
			SPINBOX_SetFont(hItem, &GUI_Font24B_1);
			SPINBOX_SetTextColor(hItem, SPINBOX_CI_ENABLED, GUI_LIGHTBLUE);
			SPINBOX_SetStep(hItem, it->step);
			break;
		}

		case DSK_CHOICE:
		{
			hItem = BUTTON_CreateEx(x, y, 60, DSM_CTRL_Y, hPage, WM_CF_SHOW, 0, ID_DSM_PREV + it->id);
			BUTTON_SetFont(hItem, &GUI_Font24B_1);
			BUTTON_SetText(hItem, "<");

			hItem = BUTTON_CreateEx(x + DSM_CTRL_X - 60, y, 60, DSM_CTRL_Y, hPage, WM_CF_SHOW, 0, ID_DSM_NEXT + it->id);
			BUTTON_SetFont(hItem, &GUI_Font24B_1);
			BUTTON_SetText(hItem, ">");

			hItem = TEXT_CreateEx(x + 62, y, DSM_CTRL_X - 124, DSM_CTRL_Y, hPage, WM_CF_SHOW, TEXT_CF_HCENTER|TEXT_CF_VCENTER, ID_DSM_CTRL + it->id, "");
			TEXT_SetFont(hItem, &GUI_Font24B_1);
			TEXT_SetBkColor(hItem, GUI_WHITE);
			TEXT_SetTextColor(hItem, GUI_LIGHTBLUE);
			break;
		}

		case DSK_LIST:
		{
			hItem = LISTBOX_CreateEx(x, y, DSM_CTRL_X, DSM_LIST_Y, hPage, WM_CF_SHOW, 0, ID_DSM_CTRL + it->id, NULL);
			LISTBOX_SetFont(hItem, &GUI_Font24B_1);
			LISTBOX_SetTextColor(hItem, LISTBOX_CI_UNSEL, GUI_LIGHTBLUE);

			num = dsp_settings_max[it->id] - dsp_settings_min[it->id] + 1;
			for(i = 0; i < num; i++)
				LISTBOX_AddString(hItem, it->opts[i]);

			SCROLLBAR_CreateAttached(hItem, SCROLLBAR_CF_VERTICAL);
			break;
		}

		default:
			return;
	}

	dsm_show(hPage, it);
}

// Yes/No confirmation, same as the Battery Manager Shutdown button
static void dsm_cbMessageBox(WM_MESSAGE* pMsg)
{
	switch (pMsg->MsgId)
	{
		case WM_NOTIFY_PARENT:
		{
			if(pMsg->Data.v == WM_NOTIFICATION_RELEASED)
				GUI_EndDialog(pMsg->hWin, (WM_GetId(pMsg->hWinSrc) == GUI_ID_OK) ? 1 : 0);

			break;
		}

		default:
			WM_DefaultProc(pMsg);
			break;
	}
}

static int dsm_confirm(WM_HWIN hWin, const char* pText)
{
	WM_HWIN hFrame, hClient, hBut;

	hFrame = FRAMEWIN_CreateEx(168, 110, 400, 200, hWin, WM_CF_SHOW, FRAMEWIN_CF_MOVEABLE, 0, "Baseband", &dsm_cbMessageBox);

	FRAMEWIN_SetClientColor   (hFrame, GUI_WHITE);
	FRAMEWIN_SetFont          (hFrame, &GUI_Font16B_ASCII);
	FRAMEWIN_SetTextAlign     (hFrame, GUI_TA_HCENTER);

	hClient = WM_GetClientWindow(hFrame);
	TEXT_CreateEx(10, 40, 370, 230, hClient, WM_CF_SHOW, GUI_TA_HCENTER, 0, pText);

	hBut = BUTTON_CreateEx(220, 100, 110, 40, hClient, WM_CF_SHOW, 0, GUI_ID_CANCEL);
	BUTTON_SetText        (hBut, "No");
	hBut = BUTTON_CreateEx(60, 100, 110, 40, hClient, WM_CF_SHOW, 0, GUI_ID_OK);
	BUTTON_SetText        (hBut, "Yes");

	WM_SetFocus(hFrame);
	WM_MakeModal(hFrame);

	return GUI_ExecCreatedDialog(hFrame);
}

//*----------------------------------------------------------------------------
//* Function Name       : dsm_page_defaults
//* Object              : 'Defaults' button - reset the settings on this page
//* Input Parameters    :
//* Output Parameters   :
//* Functions called    :
//*----------------------------------------------------------------------------
static void dsm_page_defaults(WM_HWIN hPage, uchar page)
{
	uchar i;

	for(i = 0; i < GUI_COUNTOF(dsm_items); i++)
	{
		if(dsm_items[i].page != page)
			continue;

		dsm_set(dsm_items[i].id, dsp_settings_def[dsm_items[i].id]);
		dsm_show(hPage, &dsm_items[i]);
	}
}

//*----------------------------------------------------------------------------
//* Function Name       : dsm_page_control
//* Object              : widget notifications of one page
//* Input Parameters    :
//* Output Parameters   :
//* Functions called    :
//*----------------------------------------------------------------------------
static void dsm_page_control(WM_MESSAGE * pMsg, int Id, int NCode)
{
	const DSM_ITEM	*it;
	short			val;

	if(dsm_building)
		return;

	// Page defaults
	if(Id == ID_DSM_DEFAULTS)
	{
		uchar page;

		if(NCode != WM_NOTIFICATION_RELEASED)
			return;

		for(page = 0; page < DSM_PAGE_NUM; page++)
		{
			if(dsm_page_win[page] == pMsg->hWin)
			{
				char buf[64];

				snprintf(buf, sizeof(buf), "Reset all %s settings to defaults?", dsm_page_name[page]);

				if(dsm_confirm(pMsg->hWin, buf))
					dsm_page_defaults(pMsg->hWin, page);

				break;
			}
		}

		return;
	}

	// Choice '<' / '>', wraps around
	if(((Id >= ID_DSM_PREV) && (Id < (ID_DSM_PREV + DSP_SET_COUNT))) ||
	   ((Id >= ID_DSM_NEXT) && (Id < (ID_DSM_NEXT + DSP_SET_COUNT))))
	{
		uchar next = (Id >= ID_DSM_NEXT);

		if(NCode != WM_NOTIFICATION_RELEASED)
			return;

		it = dsm_find_item(next ? (Id - ID_DSM_NEXT) : (Id - ID_DSM_PREV));
		if(it == NULL)
			return;

		val = dsp_settings[it->id] + (next ? 1 : -1);

		if(val < dsp_settings_min[it->id])
			val = dsp_settings_max[it->id];
		if(val > dsp_settings_max[it->id])
			val = dsp_settings_min[it->id];

		dsm_set(it->id, val);
		dsm_show(pMsg->hWin, it);
		return;
	}

	if((Id < ID_DSM_CTRL) || (Id >= (ID_DSM_CTRL + DSP_SET_COUNT)))
		return;

	it = dsm_find_item(Id - ID_DSM_CTRL);
	if(it == NULL)
		return;

	switch(it->kind)
	{
		case DSK_SPIN:
		{
			if(NCode == WM_NOTIFICATION_VALUE_CHANGED)
				dsm_set(it->id, SPINBOX_GetValue(pMsg->hWinSrc));

			break;
		}

		case DSK_CHECK:
		{
			if(NCode == WM_NOTIFICATION_VALUE_CHANGED)
				dsm_set(it->id, (CHECKBOX_GetState(pMsg->hWinSrc) != 0));

			break;
		}

		case DSK_LIST:
		{
			if(NCode == WM_NOTIFICATION_SEL_CHANGED)
			{
				val = LISTBOX_GetSel(pMsg->hWinSrc);
				if(val >= 0)
					dsm_set(it->id, dsp_settings_min[it->id] + val);
			}

			break;
		}

		default:
			break;
	}
}

static void _cbPage(WM_MESSAGE * pMsg)
{
	WM_HWIN hItem;
	uchar	page, i;

	switch (pMsg->MsgId)
	{
		case WM_INIT_DIALOG:
		{
			page = dsm_init_page;

			WINDOW_SetBkColor(pMsg->hWin, GUI_WHITE);

			dsm_building++;

			for(i = 0; i < GUI_COUNTOF(dsm_items); i++)
			{
				if(dsm_items[i].page == page)
					dsm_create_item(pMsg->hWin, &dsm_items[i]);
			}

			// Narrower than the slot, same centre - keeps it away from the tabs
			hItem = BUTTON_CreateEx(DSM_COL_X(DSM_SLOT_DEFAULTS) + ((DSM_CTRL_X - DSM_DEFAULTS_X) / 2),
									DSM_ROW_Y(DSM_SLOT_DEFAULTS) + DSM_LABEL_Y + 2,
									DSM_DEFAULTS_X, DSM_CTRL_Y, pMsg->hWin, WM_CF_SHOW, 0, ID_DSM_DEFAULTS);
			BUTTON_SetFont(hItem, &GUI_Font24B_1);
			BUTTON_SetText(hItem, "Defaults");

			dsm_building--;
			break;
		}

		case WM_NOTIFY_PARENT:
			dsm_page_control(pMsg, WM_GetId(pMsg->hWinSrc), pMsg->Data.v);
			break;

		default:
			WM_DefaultProc(pMsg);
			break;
	}
}

static void _cbDialog(WM_MESSAGE * pMsg)
{
	WM_HWIN 	hMulti, hPage;
	uchar		i;

	switch (pMsg->MsgId)
	{
		case WM_INIT_DIALOG:
		{
			dsm_modified = 0;

			// Same geometry as the Battery Manager
			hMulti = MULTIPAGE_CreateEx(5, 0, DSM_MULTI_X, 425, pMsg->hWin, WM_CF_SHOW, 0, ID_DSM_MULTIPAGE);

			for(i = 0; i < DSM_PAGE_NUM; i++)
			{
				dsm_init_page = i;

				hPage = GUI_CreateDialogBox(_aPage, GUI_COUNTOF(_aPage), _cbPage, WM_UNATTACHED, 0, 0);
				dsm_page_win[i] = hPage;
				MULTIPAGE_AddPage(hMulti, hPage, dsm_page_name[i]);
			}

			// Tabs vertical on the right, as the Battery Manager
			MULTIPAGE_SetRotation(hMulti, MULTIPAGE_CF_ROTATE_CW);
			MULTIPAGE_SetAlign	 (hMulti, MULTIPAGE_ALIGN_RIGHT);
			MULTIPAGE_SetFont	 (hMulti, &GUI_Font32B_ASCII);

			MULTIPAGE_SelectPage(hMulti, 0);
			break;
		}

		// Whichever way the menu closes
		case WM_DELETE:
		{
			memset(dsm_page_win, 0, sizeof(dsm_page_win));

			// Closed with the home key the handle stayed set, and the menu
			// framework calls every module's kill() on exit - GUI_EndDialog()
			// on a deleted (maybe reused) handle
			hDSdialog = 0;

			if(dsm_modified)
			{
				radio_init_dsp_settings_save();
				dsm_modified = 0;
			}

			break;
		}

		// Process key messages not supported by ICON_VIEW control
		case WM_KEY:
		{
			switch (((WM_KEY_INFO*)(pMsg->Data.p))->Key)
			{
		        // Return from menu
		        case GUI_KEY_HOME:
		        {
		        	//printf("GUI_KEY_HOME\r\n");
		        	GUI_EndDialog(pMsg->hWin, 0);
		        	break;
		        }
			}
			break;
		}

		default:
			WM_DefaultProc(pMsg);
			break;
	}
}

//*----------------------------------------------------------------------------
//* Function Name       : Startup
//* Object              :
//* Input Parameters    :
//* Output Parameters   :
//* Functions called    :
//*----------------------------------------------------------------------------
static void Startup(WM_HWIN hWin, uint16_t xpos, uint16_t ypos)
{
	// Does the current theme require shift of the window ?
	if(menu_layout[ui_s.theme_id].iconview_y == 0)
		goto use_const_decl;

	GUI_WIDGET_CREATE_INFO *p_widget = malloc(sizeof(_aDialog));
	if(p_widget == NULL)
		goto use_const_decl;	// looking ugly is the least of our problems now

	memcpy(p_widget, _aDialog,sizeof(_aDialog));
	p_widget[0].y0 = menu_layout[ui_s.theme_id].iconview_y;	// shift

	hDSdialog = GUI_CreateDialogBox(p_widget, GUI_COUNTOF(_aDialog), _cbDialog, hWin, xpos, ypos);

	free(p_widget);
	return;

use_const_decl:
	hDSdialog = GUI_CreateDialogBox(_aDialog, GUI_COUNTOF(_aDialog), _cbDialog, hWin, xpos, ypos);
}

static void KillDsps(void)
{
	//printf("kill menu\r\n");

	if(hDSdialog)
	{
		GUI_EndDialog(hDSdialog, 0);
		hDSdialog = 0;
	}
}

#endif
