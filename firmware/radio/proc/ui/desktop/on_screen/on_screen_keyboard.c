/************************************************************************************
**                                                                                 **
**                             mcHF Pro QRP Transceiver                            **
**                         Krassi Atanassov - M0NKA, 2013-2024                     **
**                                                                                 **
**---------------------------------------------------------------------------------**
**                                                                                 **
**  File name:                                                                     **
**  Description:                                                                   **
**  Last Modified:                                                                 **
**  Licence:       The mcHF project is released for radio amateurs experimentation **
**               and non-commercial use only.Check 3rd party drivers for licensing **
************************************************************************************/
#include "mchf_pro_board.h"
#include "main.h"

#ifdef CONTEXT_VIDEO

#include "on_screen_keyboard.h"
#include "ui_actions.h"

#include "radio_init.h"

//#define KEYB_IS_TRANPARENT

#define ID_WINDOW_KEYB          	(GUI_ID_USER + 0x50)

// Row 1
#define ID_BUTTON_SSB	          	(GUI_ID_USER + 0x51)
#define ID_BUTTON_160M	          	(GUI_ID_USER + 0x52)
#define ID_BUTTON_80M	          	(GUI_ID_USER + 0x53)
#define ID_BUTTON_60M	          	(GUI_ID_USER + 0x54)
#define ID_BUTTON_40M	          	(GUI_ID_USER + 0x55)
#define ID_BUTTON_DSP	          	(GUI_ID_USER + 0x56)

// Row 2
#define ID_BUTTON_CW	          	(GUI_ID_USER + 0x57)
#define ID_BUTTON_30M	          	(GUI_ID_USER + 0x58)
#define ID_BUTTON_20M	          	(GUI_ID_USER + 0x59)
#define ID_BUTTON_17M	          	(GUI_ID_USER + 0x5A)
#define ID_BUTTON_15M	          	(GUI_ID_USER + 0x5B)
#define ID_BUTTON_TXPO	          	(GUI_ID_USER + 0x5C)

// Row 3
#define ID_BUTTON_AM	          	(GUI_ID_USER + 0x5D)
#define ID_BUTTON_12M	          	(GUI_ID_USER + 0x5E)
#define ID_BUTTON_10M	          	(GUI_ID_USER + 0x5F)
#define ID_BUTTON_GEN	          	(GUI_ID_USER + 0x60)
#define ID_BUTTON_ENTER          	(GUI_ID_USER + 0x61)
#define ID_BUTTON_STEP          	(GUI_ID_USER + 0x62)

// Row 4
#define ID_BUTTON_FIX	          	(GUI_ID_USER + 0x63)
#define ID_BUTTON_2200M	          	(GUI_ID_USER + 0x64)
#define ID_BUTTON_630M	          	(GUI_ID_USER + 0x65)
#define ID_BUTTON_SPAN	          	(GUI_ID_USER + 0x66)
// NA
#define ID_BUTTON_SPLIT          	(GUI_ID_USER + 0x67)

#define KEYB_X						130
#define KEYB_Y						203

#define KEYB_SIZE_X					595
#define KEYB_SIZE_Y					250

#ifdef KEYB_IS_TRANPARENT
#define K_CF						WM_CF_SHOW|WM_CF_MEMDEV|WM_CF_HASTRANS
#else
#define K_CF						0x64
#endif

static const GUI_WIDGET_CREATE_INFO KeybDialog[] =
{
	// -----------------------------------------------------------------------------------------------------------------------------
	//							name		id					x		y		xsize				ysize				?		?		?
	// -----------------------------------------------------------------------------------------------------------------------------
	// Self
	{ WINDOW_CreateIndirect,	"", 		ID_WINDOW_KEYB,		0,		0,		KEYB_SIZE_X,		KEYB_SIZE_Y, 		0, 		K_CF, 	0 },

	// Row 1
	{ BUTTON_CreateIndirect, 	"SSB",		ID_BUTTON_SSB,		 20, 	15, 	80, 				45, 				0, 		0x0, 	0 },
	{ BUTTON_CreateIndirect, 	"160m",		ID_BUTTON_160M,		115, 	15, 	80, 				45, 				0, 		0x0, 	0 },
	{ BUTTON_CreateIndirect, 	"80m",		ID_BUTTON_80M,		210, 	15, 	80, 				45, 				0, 		0x0, 	0 },
	{ BUTTON_CreateIndirect, 	"60m",		ID_BUTTON_60M,		305, 	15, 	80, 				45, 				0, 		0x0, 	0 },
	{ BUTTON_CreateIndirect, 	"40m",		ID_BUTTON_40M,		400, 	15, 	80, 				45, 				0, 		0x0, 	0 },
	{ BUTTON_CreateIndirect, 	"TUNE",		ID_BUTTON_DSP,		495, 	15, 	80, 				45, 				0, 		0x0, 	0 },

	// Row 2
	{ BUTTON_CreateIndirect, 	"CW",		ID_BUTTON_CW,		 20, 	75, 	80, 				45, 				0, 		0x0, 	0 },
	{ BUTTON_CreateIndirect, 	"30m",		ID_BUTTON_30M,		115, 	75, 	80, 				45, 				0, 		0x0, 	0 },
	{ BUTTON_CreateIndirect, 	"20m",		ID_BUTTON_20M,		210, 	75, 	80, 				45, 				0, 		0x0, 	0 },
	{ BUTTON_CreateIndirect, 	"17m",		ID_BUTTON_17M,		305, 	75, 	80, 				45, 				0, 		0x0, 	0 },
	{ BUTTON_CreateIndirect, 	"15m",		ID_BUTTON_15M,		400, 	75, 	80, 				45, 				0, 		0x0, 	0 },
	{ BUTTON_CreateIndirect, 	"PWR",		ID_BUTTON_TXPO,		495, 	75, 	80, 				45, 				0, 		0x0, 	0 },

	// Row 3
	{ BUTTON_CreateIndirect, 	"AM",		ID_BUTTON_AM,		 20, 	135, 	80, 				45, 				0, 		0x0, 	0 },
	{ BUTTON_CreateIndirect, 	"12m",		ID_BUTTON_12M,		115, 	135, 	80, 				45, 				0, 		0x0, 	0 },
	{ BUTTON_CreateIndirect, 	"10m",		ID_BUTTON_10M,		210, 	135, 	80, 				45, 				0, 		0x0, 	0 },
	{ BUTTON_CreateIndirect, 	"GEN",		ID_BUTTON_GEN,		305, 	135, 	80, 				45, 				0, 		0x0, 	0 },
	{ BUTTON_CreateIndirect, 	"QUIT",		ID_BUTTON_ENTER,	400, 	135, 	80, 			   105, 				0, 		0x0, 	0 },
	{ BUTTON_CreateIndirect, 	"STEP",		ID_BUTTON_STEP,		495, 	135, 	80, 				45, 				0, 		0x0, 	0 },

	// Row 4
	{ BUTTON_CreateIndirect, 	"FIX",		ID_BUTTON_FIX,		 20, 	195, 	80, 				45, 				0, 		0x0, 	0 },
	{ BUTTON_CreateIndirect, 	"2200m",	ID_BUTTON_2200M,	115, 	195, 	80, 				45, 				0, 		0x0, 	0 },
	{ BUTTON_CreateIndirect, 	"630m",		ID_BUTTON_630M,		210, 	195, 	80, 				45, 				0, 		0x0, 	0 },
	{ BUTTON_CreateIndirect, 	"SPAN",		ID_BUTTON_SPAN,		305, 	195, 	80, 				45, 				0, 		0x0, 	0 },
	// NA
	{ BUTTON_CreateIndirect, 	"SPLIT",	ID_BUTTON_SPLIT,	495, 	195, 	80, 				45, 				0, 		0x0, 	0 },
};

WM_HWIN 	hKeybDialog = 0;

static void KH_cbControl(WM_MESSAGE * pMsg, int Id, int NCode)
{
	//WM_HWIN hItem;

	if(NCode != WM_NOTIFICATION_RELEASED)
		return;

	switch(Id)
	{
		case ID_BUTTON_2200M:
			ui_actions_change_band(BAND_MODE_2200, 0);
			break;
		case ID_BUTTON_630M:
			ui_actions_change_band(BAND_MODE_630, 0);
			break;
		case ID_BUTTON_160M:
			ui_actions_change_band(BAND_MODE_160, 0);
			break;
		case ID_BUTTON_80M:
			ui_actions_change_band(BAND_MODE_80, 0);
			break;
		case ID_BUTTON_60M:
			ui_actions_change_band(BAND_MODE_60, 0);
			break;
		case ID_BUTTON_40M:
			ui_actions_change_band(BAND_MODE_40, 0);
			break;
		case ID_BUTTON_30M:
			ui_actions_change_band(BAND_MODE_30, 0);
			break;
		case ID_BUTTON_20M:
			ui_actions_change_band(BAND_MODE_20, 0);
			break;
		case ID_BUTTON_17M:
			ui_actions_change_band(BAND_MODE_17, 0);
			break;
		case ID_BUTTON_15M:
			ui_actions_change_band(BAND_MODE_15, 0);
			break;
		case ID_BUTTON_12M:
			ui_actions_change_band(BAND_MODE_12, 0);
			break;
		case ID_BUTTON_10M:
			ui_actions_change_band(BAND_MODE_10, 0);
			break;
		case ID_BUTTON_GEN:
			ui_actions_change_band(BAND_MODE_GEN, 0);
			break;

		case ID_BUTTON_FIX:
			ui_actions_change_vfo_mode();
			break;
		case ID_BUTTON_SPAN:
			ui_actions_change_span();
			break;
		case ID_BUTTON_STEP:
			ui_actions_change_step();
			break;

		case ID_BUTTON_TXPO:
			ui_actions_change_power_level();
			break;

		case ID_BUTTON_SSB:
			ui_actions_change_demod_mode(radio_init_default_mode_from_band());
			break;
		case ID_BUTTON_CW:
			ui_actions_change_demod_mode(DEMOD_CW);
			break;
		case ID_BUTTON_AM:
			ui_actions_change_demod_mode(DEMOD_AM);
			break;

		//case ID_BUTTON_ATT:
		//	ui_actions_toggle_atten();
		//	break;

		case ID_BUTTON_SPLIT:
			ui_actions_change_dsp_core();
			break;

		case ID_BUTTON_ENTER:
			GUI_EndDialog(pMsg->hWin, 0);
			break;

		default:
			break;
	}
}

static void KeybHandler(WM_MESSAGE *pMsg)
{
	//WM_HWIN hItem;
	int 		Id, NCode;
	//GUI_RECT	Rect;

	switch (pMsg->MsgId)
	{
		case WM_INIT_DIALOG:
		{
			#ifdef KEYB_IS_TRANPARENT
			WINDOW_SetBkColor(pMsg->hWin, GUI_INVALID_COLOR);
			#else
			WINDOW_SetBkColor(pMsg->hWin, GUI_LIGHTGRAY);
			#endif

			//hItem = WM_GetDialogItem(pMsg->hWin, ID_WINDOW_KEYB);
			//WM_SetTransState(hItem, WM_CF_HASTRANS);

			//TEXT_SetFont(hItem,&GUI_Font16B_1);
			//TEXT_SetBkColor(hItem,GUI_LIGHTBLUE);
			//TEXT_SetTextColor(hItem,GUI_WHITE);
			//TEXT_SetTextAlign(hItem,TEXT_CF_RIGHT|TEXT_CF_VCENTER);

			//hTimerWiFi = WM_CreateTimer(pMsg->hWin, 0, WIFI_TIMER_RESOLUTION, 0);
			break;
		}

		case WM_TIMER:
		{
			#if 0
			if(tsu.wifi_rssi)
			{
				char buf[30];
				hItem = WM_GetDialogItem(pMsg->hWin, ID_TEXT_WIFI);
				sprintf(buf, "%d dBm", tsu.wifi_rssi);
				TEXT_SetText(hItem, buf);

				WM_InvalidateWindow(hWiFiDialog);
			}
			#endif

			//TEXT_SetText(hItem, "test");
			//WM_InvalidateWindow(hWiFiDialog);

			//WM_RestartTimer(pMsg->Data.v, WIFI_TIMER_RESOLUTION);
			break;
		}

		case WM_PAINT:
			//WM_GetClientRect(&Rect);	// will create border when transparent
			//GUI_DrawRectEx(&Rect);
			break;


		case WM_DELETE:
		{
			//WM_DeleteTimer(hTimerWiFi);

			WM_HideWindow(hKeybDialog);

			// Clear screen
			//GUI_SetBkColor(GUI_BLACK);
			//GUI_Clear();

			// Init controls
			//ui_proc_init_desktop();
			ui_controls_spectrum_init(WM_HBKWIN);
			ui_proc_clear_active();

			hKeybDialog = 0;

			break;
		}

		case WM_NOTIFY_PARENT:
		{
			Id    = WM_GetId(pMsg->hWinSrc);    /* Id of widget */
			NCode = pMsg->Data.v;               /* Notification code */

			KH_cbControl(pMsg,Id,NCode);
			break;
		}

		// Trap keyboard messages
		case WM_KEY:
		{
			switch (((WM_KEY_INFO*)(pMsg->Data.p))->Key)
			{
		        // Return from menu
		        case GUI_KEY_HOME:
		        {
		        	//printf("GUI_KEY_HOME\r\n");
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

uchar on_screen_keyboard_init(WM_HWIN hParent)
{
	if(hKeybDialog == 0)
	{
		hKeybDialog = GUI_CreateDialogBox(KeybDialog, GUI_COUNTOF(KeybDialog), KeybHandler, hParent, KEYB_X, KEYB_Y);
		return 1;
	}
	//else
	//	WM_ShowWindow(hKeybDialog);

	return 0;
}

void on_screen_keyboard_quit(void)
{
	GUI_EndDialog(hKeybDialog, 0);
	hKeybDialog = 0;
	//WM_HideWindow(hWiFiDialog);
}

void on_screen_keyboard_refresh(void)
{
	if(hKeybDialog == 0)
		WM_InvalidateWindow(hKeybDialog);
}

#endif
