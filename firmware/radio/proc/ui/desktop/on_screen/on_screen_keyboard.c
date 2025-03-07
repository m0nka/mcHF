/************************************************************************************
**                                                                                 **
**                             mcHF Pro QRP Transceiver                            **
**                         Krassi Atanassov - M0NKA, 2013-2025                     **
**                                                                                 **
**---------------------------------------------------------------------------------**
**                                                                                 **
**  File name:                                                                     **
**  Description:                                                                   **
**  Last Modified:                                                                 **
**  Licence:               GNU GPLv3                                               **
************************************************************************************/
#include "mchf_pro_board.h"
#include "main.h"

#ifdef CONTEXT_VIDEO

#include "ui_proc.h"
#include "ui_actions.h"
#include "radio_init.h"
#include "spectrum/ui_controls_spectrum.h"

#include "on_screen_keyboard.h"


//#define KEYB_IS_TRANPARENT

#define ID_WINDOW_KEYB          	(GUI_ID_USER + 0x50)

// Row 1
#define ID_BUTTON_X1Y1	          	(GUI_ID_USER + 0x51)
#define ID_BUTTON_X2Y1	          	(GUI_ID_USER + 0x52)
#define ID_BUTTON_X3Y1	          	(GUI_ID_USER + 0x53)
#define ID_BUTTON_X4Y1	          	(GUI_ID_USER + 0x54)
#define ID_BUTTON_X5Y1	          	(GUI_ID_USER + 0x55)
#define ID_BUTTON_X6Y1	          	(GUI_ID_USER + 0x56)

// Row 2
#define ID_BUTTON_X1Y2	          	(GUI_ID_USER + 0x57)
#define ID_BUTTON_X2Y2	          	(GUI_ID_USER + 0x58)
#define ID_BUTTON_X3Y2	          	(GUI_ID_USER + 0x59)
#define ID_BUTTON_X4Y2	          	(GUI_ID_USER + 0x5A)
#define ID_BUTTON_X5Y2	          	(GUI_ID_USER + 0x5B)
#define ID_BUTTON_X6Y2	          	(GUI_ID_USER + 0x5C)

// Row 3
#define ID_BUTTON_X1Y3	          	(GUI_ID_USER + 0x5D)
#define ID_BUTTON_X2Y3	          	(GUI_ID_USER + 0x5E)
#define ID_BUTTON_X3Y3	          	(GUI_ID_USER + 0x5F)
#define ID_BUTTON_X4Y3	          	(GUI_ID_USER + 0x60)
#define ID_BUTTON_X5Y3          	(GUI_ID_USER + 0x61)
#define ID_BUTTON_X6Y3          	(GUI_ID_USER + 0x62)

// Row 4
#define ID_BUTTON_X1Y4	          	(GUI_ID_USER + 0x63)
#define ID_BUTTON_X2Y4	          	(GUI_ID_USER + 0x64)
#define ID_BUTTON_X3Y4	          	(GUI_ID_USER + 0x65)
#define ID_BUTTON_X4Y4	          	(GUI_ID_USER + 0x66)
// NA
#define ID_BUTTON_X6Y4          	(GUI_ID_USER + 0x67)

#define KEYB_X						130
#define KEYB_Y						131

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
	{ BUTTON_CreateIndirect, 	"SSB",		ID_BUTTON_X1Y1,		 20, 	15, 	80, 				45, 				0, 		0x0, 	0 },
	{ BUTTON_CreateIndirect, 	"160m",		ID_BUTTON_X2Y1,		115, 	15, 	80, 				45, 				0, 		0x0, 	0 },
	{ BUTTON_CreateIndirect, 	"80m",		ID_BUTTON_X3Y1,		210, 	15, 	80, 				45, 				0, 		0x0, 	0 },
	{ BUTTON_CreateIndirect, 	"60m",		ID_BUTTON_X4Y1,		305, 	15, 	80, 				45, 				0, 		0x0, 	0 },
	{ BUTTON_CreateIndirect, 	"40m",		ID_BUTTON_X5Y1,		400, 	15, 	80, 				45, 				0, 		0x0, 	0 },
	{ BUTTON_CreateIndirect, 	"STEP+",	ID_BUTTON_X6Y1,		495, 	15, 	80, 				45, 				0, 		0x0, 	0 },

	// Row 2
	{ BUTTON_CreateIndirect, 	"CW",		ID_BUTTON_X1Y2,		 20, 	75, 	80, 				45, 				0, 		0x0, 	0 },
	{ BUTTON_CreateIndirect, 	"30m",		ID_BUTTON_X2Y2,		115, 	75, 	80, 				45, 				0, 		0x0, 	0 },
	{ BUTTON_CreateIndirect, 	"20m",		ID_BUTTON_X3Y2,		210, 	75, 	80, 				45, 				0, 		0x0, 	0 },
	{ BUTTON_CreateIndirect, 	"17m",		ID_BUTTON_X4Y2,		305, 	75, 	80, 				45, 				0, 		0x0, 	0 },
	{ BUTTON_CreateIndirect, 	"15m",		ID_BUTTON_X5Y2,		400, 	75, 	80, 				45, 				0, 		0x0, 	0 },
	{ BUTTON_CreateIndirect, 	"STEP-",	ID_BUTTON_X6Y2,		495, 	75, 	80, 				45, 				0, 		0x0, 	0 },

	// Row 3
	{ BUTTON_CreateIndirect, 	"AM",		ID_BUTTON_X1Y3,		 20, 	135, 	80, 				45, 				0, 		0x0, 	0 },
	{ BUTTON_CreateIndirect, 	"12m",		ID_BUTTON_X2Y3,		115, 	135, 	80, 				45, 				0, 		0x0, 	0 },
	{ BUTTON_CreateIndirect, 	"10m",		ID_BUTTON_X3Y3,		210, 	135, 	80, 				45, 				0, 		0x0, 	0 },
	{ BUTTON_CreateIndirect, 	"---",		ID_BUTTON_X4Y3,		305, 	135, 	80, 				45, 				0, 		0x0, 	0 },
	{ BUTTON_CreateIndirect, 	"MENU",		ID_BUTTON_X5Y3,		400, 	135, 	80, 			   105, 				0, 		0x0, 	0 },
	{ BUTTON_CreateIndirect, 	"FILT",		ID_BUTTON_X6Y3,		495, 	135, 	80, 				45, 				0, 		0x0, 	0 },

	// Row 4
	{ BUTTON_CreateIndirect, 	"FIX",		ID_BUTTON_X1Y4,		 20, 	195, 	80, 				45, 				0, 		0x0, 	0 },
	{ BUTTON_CreateIndirect, 	"Audio",	ID_BUTTON_X2Y4,		115, 	195, 	80, 				45, 				0, 		0x0, 	0 },
	{ BUTTON_CreateIndirect, 	"AGC",		ID_BUTTON_X3Y4,		210, 	195, 	80, 				45, 				0, 		0x0, 	0 },
	{ BUTTON_CreateIndirect, 	"---",		ID_BUTTON_X4Y4,		305, 	195, 	80, 				45, 				0, 		0x0, 	0 },
	// NA
	{ BUTTON_CreateIndirect, 	"---",		ID_BUTTON_X6Y4,		495, 	195, 	80, 				45, 				0, 		0x0, 	0 },
};

WM_HWIN 	hKeybDialog = 0;

// Public radio state
extern struct	TRANSCEIVER_STATE_UI	tsu;

static void KH_cbControl(WM_MESSAGE * pMsg, int Id, int NCode)
{
	//WM_HWIN hItem;

	if(NCode != WM_NOTIFICATION_RELEASED)
		return;

	switch(Id)
	{
		case ID_BUTTON_X2Y4:
			//ui_actions_change_band(BAND_MODE_2200, 0);	// ToDo: need unload and load Audio dialog
			break;
		case ID_BUTTON_X3Y4:
			//ui_actions_change_band(BAND_MODE_630, 0);		// ToDo: same but AGC dialog
			break;
		case ID_BUTTON_X2Y1:
			ui_actions_change_band(BAND_MODE_160, 0);
			break;
		case ID_BUTTON_X3Y1:
			ui_actions_change_band(BAND_MODE_80, 0);
			break;
		case ID_BUTTON_X4Y1:
			ui_actions_change_band(BAND_MODE_60, 0);
			break;
		case ID_BUTTON_X5Y1:
			ui_actions_change_band(BAND_MODE_40, 0);
			break;
		case ID_BUTTON_X2Y2:
			ui_actions_change_band(BAND_MODE_30, 0);
			break;
		case ID_BUTTON_X3Y2:
			ui_actions_change_band(BAND_MODE_20, 0);
			break;
		case ID_BUTTON_X4Y2:
			ui_actions_change_band(BAND_MODE_17, 0);
			break;
		case ID_BUTTON_X5Y2:
			ui_actions_change_band(BAND_MODE_15, 0);
			break;
		case ID_BUTTON_X2Y3:
			ui_actions_change_band(BAND_MODE_12, 0);
			break;
		case ID_BUTTON_X3Y3:
			ui_actions_change_band(BAND_MODE_10, 0);
			break;
		case ID_BUTTON_X4Y3:
			//ui_actions_change_band(BAND_MODE_GEN, 0);
			break;

		case ID_BUTTON_X1Y4:
			ui_actions_change_vfo_mode();
			break;
		case ID_BUTTON_X4Y4:
			ui_actions_change_span();
			break;
		case ID_BUTTON_X6Y3:
		{
			(tsu.band[tsu.curr_band].filter)++;
				if(tsu.band[tsu.curr_band].filter > AUDIO_WIDE)
					tsu.band[tsu.curr_band].filter = AUDIO_300HZ;
				ui_actions_change_filter(tsu.band[tsu.curr_band].filter);
			break;
		}

		case ID_BUTTON_X6Y2:
			ui_actions_change_step(0);
			break;

		case ID_BUTTON_X6Y1:
			//ui_actions_change_power_level();
			ui_actions_change_step(1);
			break;

		case ID_BUTTON_X1Y1:
			ui_actions_change_demod_mode(radio_init_default_mode_from_band());
			break;
		case ID_BUTTON_X1Y2:
			ui_actions_change_demod_mode(DEMOD_CW);
			break;
		case ID_BUTTON_X1Y3:
			ui_actions_change_demod_mode(DEMOD_AM);
			break;

		//case ID_BUTTON_ATT:
		//	ui_actions_toggle_atten();
		//	break;

		case ID_BUTTON_X6Y4:
		{
			//ui_actions_change_dsp_core();

			break;
		}

		case ID_BUTTON_X5Y3:
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

		        case GUI_KEY_ENTER:
		        {
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
	WM_HideWindow(hKeybDialog);
	GUI_EndDialog(hKeybDialog, 0);
	hKeybDialog = 0;
}

void on_screen_keyboard_refresh(void)
{
	if(hKeybDialog == 0)
		WM_InvalidateWindow(hKeybDialog);
}

#endif
