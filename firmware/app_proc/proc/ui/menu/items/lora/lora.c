/************************************************************************************
**                                                                                 **
**                                 mcHF QRP Transceiver                            **
**                         Krassi Atanassov - M0NKA, 2013-2026                     **
**                                                                                 **
**---------------------------------------------------------------------------------**
**                                                                                 **
**  File name:                                                                     **
**  Description:                                                                   **
**  Last Modified:                                                                 **
**  Licence:			https://github.com/m0nka/mcHF/blob/main/LICENSE            **
************************************************************************************/
#include "main.h"
#include "mchf_pro_board.h"

#ifdef CONTEXT_VIDEO

#include "ui_menu_layout.h"
#include "gui.h"
#include "dialog.h"

#include "ui_menu_module.h"

#include "ui_lora_state.h"

// Non HF radios share this page - Sub-GHz, GNSS and the BT audio module
#include "rtc.h"
#include "gps_proc.h"
#ifdef CONTEXT_GPS
#include "gps_calib.h"
#endif

extern GUI_CONST_STORAGE GUI_BITMAP bmheliumhntlogo;
  
// UI driver public state
extern struct	UI_DRIVER_STATE			ui_s;
extern 			TaskHandle_t 			hUiTask;
extern struct 	UI_LORA_STATE 			uls;

// Public radio state
extern struct	TRANSCEIVER_STATE_UI	tsu;

// Menu layout definitions from Flash
extern const struct UIMenuLayout menu_layout[];

static void Startup(WM_HWIN hWin, uint16_t xpos, uint16_t ypos);
static void KillLora(void);

K_ModuleItem_Typedef  lora =
{
  2,
  "Sub-GHz",
  &bmheliumhntlogo,
  Startup,
  NULL,
  KillLora
};

WM_HWIN   	hLdialog;

#define ID_WINDOW_0               	(GUI_ID_USER + 0x00)
//#define ID_BUTTON_EXIT            	(GUI_ID_USER + 0x01)

#define ID_BUTTON_UI_RESET		  	(GUI_ID_USER + 0x02)
//#define ID_BUTTON_DSP_RESET		  	(GUI_ID_USER + 0x03)
//#define ID_BUTTON_EEP_RESET		  	(GUI_ID_USER + 0x04)
#define ID_CHECKBOX_0				(GUI_ID_USER + 0x03)

// GNSS clock calibration
#define ID_BUTTON_GPS_CAL			(GUI_ID_USER + 0x05)
#define ID_BUTTON_GPS_ACCEPT		(GUI_ID_USER + 0x06)
#define ID_BUTTON_GPS_CLEAR			(GUI_ID_USER + 0x07)

// Live refresh of the calibration read out
#define GPS_CAL_TIMER_MS			1000

static const GUI_WIDGET_CREATE_INFO _aDialog[] =
{
	// -----------------------------------------------------------------------------------------------------------------------------
	//							name					id						x		y		xsize	ysize	?		?		?
	// -----------------------------------------------------------------------------------------------------------------------------
	// Self
	{ WINDOW_CreateIndirect,	"", 					ID_WINDOW_0,			0,    	0,		800,	430, 	0, 		0x64, 	0 },
	// Back Button
	//{ BUTTON_CreateIndirect, 	"Back",			 		ID_BUTTON_EXIT, 		670, 	375, 	120, 	45, 	0, 		0x0, 	0 },
	//
	{ BUTTON_CreateIndirect, 	"LORA TX",		 		ID_BUTTON_UI_RESET,		40, 	40, 	120, 	45, 	0, 		0x0, 	0 },
	//{ BUTTON_CreateIndirect, 	"Power OFF",		 	ID_BUTTON_DSP_RESET,	40, 	120, 	120, 	45, 	0, 		0x0, 	0 },
	//{ BUTTON_CreateIndirect, 	"Kill Backup",	 		ID_BUTTON_EEP_RESET,	40, 	200, 	120, 	45, 	0, 		0x0, 	0 },

	// Check boxes
	{ CHECKBOX_CreateIndirect,	"", 			ID_CHECKBOX_0, 		20, 	260,	250, 	30, 	0, 		0x0, 	0 },

	{ TEXT_CreateIndirect, 		"OFF",					GUI_ID_TEXT0,			180,	40,		120, 	45,  	0, 		0x0,	0 },

	#ifdef CONTEXT_GPS
	// ---- GNSS clock calibration, right hand column -----------------------------------------------------------------------------
	{ TEXT_CreateIndirect,		"GNSS clock trim",		GUI_ID_TEXT1,			400,	30,		380,	25,		0,		0x0,	0 },
	{ TEXT_CreateIndirect,		"",						GUI_ID_TEXT2,			400,	65,		380,	25,		0,		0x0,	0 },
	{ TEXT_CreateIndirect,		"",						GUI_ID_TEXT3,			400,	95,		380,	25,		0,		0x0,	0 },
	{ TEXT_CreateIndirect,		"",						GUI_ID_TEXT4,			400,	125,	380,	25,		0,		0x0,	0 },
	{ TEXT_CreateIndirect,		"",						GUI_ID_TEXT5,			400,	155,	380,	25,		0,		0x0,	0 },

	{ BUTTON_CreateIndirect,	"Start",				ID_BUTTON_GPS_CAL,		400,	195,	110,	45,		0,		0x0,	0 },
	{ BUTTON_CreateIndirect,	"Accept",				ID_BUTTON_GPS_ACCEPT,	520,	195,	110,	45,		0,		0x0,	0 },
	{ BUTTON_CreateIndirect,	"Default",				ID_BUTTON_GPS_CLEAR,	640,	195,	110,	45,		0,		0x0,	0 },
	#endif
};

#ifdef CONTEXT_GPS
static WM_HTIMER	hTimerGpsCal;

//*----------------------------------------------------------------------------
//* Function Name       : _gps_calib_refresh
//* Object              : repaint the calibration read out. Everything is
//*						: scaled integer - gps_calib.c hands out ppm x100 so
//*						: no float formatting is needed anywhere
//* Context    			: CONTEXT_VIDEO (gui task)
//*----------------------------------------------------------------------------
static void _gps_calib_refresh(WM_HWIN hDlg)
{
	gps_calib_stat_t	st;
	WM_HWIN				hItem;
	char				buf[64];
	int					have, r;
	char				sign;

	have = (gps_calib_get(&st) == 0);

	// State of the pulse train
	hItem = WM_GetDialogItem(hDlg, GUI_ID_TEXT2);
	if(!st.running)
		sprintf(buf, "Idle - %d sats", (int)gps_proc_sats_cnt());
	else if(!st.armed)
		sprintf(buf, "Waiting for fix - %d sats", (int)gps_proc_sats_cnt());
	else
		sprintf(buf, "Measuring - %d sats, %u bad edges",
				(int)gps_proc_sats_cnt(), (unsigned)st.glitches);
	TEXT_SetText(hItem, buf);

	// Progress. Sweep is the dithering quality and the real gate on a
	// result, so it is on screen rather than hidden in the maths
	hItem = WM_GetDialogItem(hDlg, GUI_ID_TEXT3);
	sprintf(buf, "%u pps, %u:%02u elapsed, sweep %u.%u lsb",
			(unsigned)st.samples,
			(unsigned)(st.span_s / 3600u), (unsigned)((st.span_s / 60u) % 60u),
			(unsigned)(st.sweep_lsb_x10 / 10u), (unsigned)(st.sweep_lsb_x10 % 10u));
	TEXT_SetText(hItem, buf);

	// Measured drift
	hItem = WM_GetDialogItem(hDlg, GUI_ID_TEXT4);
	if(have)
	{
		r    = (int)st.residual_ppm_x100;
		sign = (r < 0) ? '-' : '+';
		if(r < 0)
			r = -r;

		sprintf(buf, "Drift %c%d.%02d +/- %d.%02d ppm", sign, r / 100, r % 100,
				(int)(st.err_ppm_x100 / 100), (int)(st.err_ppm_x100 % 100));
	}
	else if(st.running)
	{
		if(st.eta_s != 0)
			sprintf(buf, "Need ~%u more min", (unsigned)((st.eta_s + 59u) / 60u));
		else
			sprintf(buf, "Collecting...");
	}
	else
	{
		sprintf(buf, "Not measured");
	}
	TEXT_SetText(hItem, buf);

	// Trim in force, and what accepting would store
	hItem = WM_GetDialogItem(hDlg, GUI_ID_TEXT5);
	if(have)
		sprintf(buf, "Trim %d -> %d ppm", (int)st.trim_now_ppm, (int)st.trim_new_ppm);
	else
		sprintf(buf, "Trim %d ppm", (int)st.trim_now_ppm);
	TEXT_SetText(hItem, buf);

	hItem = WM_GetDialogItem(hDlg, ID_BUTTON_GPS_CAL);
	BUTTON_SetText(hItem, st.running ? "Stop" : "Start");

	// Accepting a result that does not exist yet would store a fantasy
	hItem = WM_GetDialogItem(hDlg, ID_BUTTON_GPS_ACCEPT);
	if(have)
		WM_EnableWindow(hItem);
	else
		WM_DisableWindow(hItem);
}
#endif

// API Driver messaging
//extern osMessageQId 					hApiMessage;
//struct APIMessage						api_reset;

static void _cbControl(WM_MESSAGE * pMsg, int Id, int NCode)
{
	WM_HWIN hItem;

	switch(Id)
	{
		#if 0
		// -------------------------------------------------------------
		// Button - exit
		case ID_BUTTON_EXIT:
		{
			switch(NCode)
			{
				case WM_NOTIFICATION_RELEASED:
					GUI_EndDialog(pMsg->hWin, 0);
					break;
			}
			break;
		}
		#endif

		// -------------------------------------------------------------
		// Button - restart UI
		case ID_BUTTON_UI_RESET:
		{
			switch(NCode)
			{
				case WM_NOTIFICATION_RELEASED:
				{
					printf("...Lora TX\r\n");

					// ToDo: Fix messaging between tasks!
					//uchar s_r = ui_actions_ipc_msg(1, 8, NULL);
					//vTaskDelay(100);
					//uchar w_r = ui_actions_ipc_msg(0, 8, NULL);

					break;
				}
			}
			break;
		}

		#ifdef CONTEXT_GPS
		// -------------------------------------------------------------
		// Start / stop a GNSS clock trim measurement. The RTC is NOT
		// touched by this - gps_proc.c syncs it once on the first fix and
		// then only observes, which is what makes the drift measurable
		case ID_BUTTON_GPS_CAL:
		{
			if(NCode == WM_NOTIFICATION_RELEASED)
			{
				gps_calib_stat_t st;

				gps_calib_get(&st);

				if(st.running)
					gps_calib_stop();
				else
					gps_calib_start();

				_gps_calib_refresh(pMsg->hWin);
			}
			break;
		}

		// -------------------------------------------------------------
		// Store the measured trim for this unit. gps_calib_accept() does
		// the trim_new = trim_now - residual subtraction itself
		case ID_BUTTON_GPS_ACCEPT:
		{
			if(NCode == WM_NOTIFICATION_RELEASED)
			{
				if(gps_calib_accept() != 0)
					printf("gps calib: accept refused\r\n");

				_gps_calib_refresh(pMsg->hWin);
			}
			break;
		}

		// -------------------------------------------------------------
		// Forget the measured value, back to the compiled in default
		case ID_BUTTON_GPS_CLEAR:
		{
			if(NCode == WM_NOTIFICATION_RELEASED)
			{
				rtc_calib_ppm_clear();
				_gps_calib_refresh(pMsg->hWin);
			}
			break;
		}
		#endif

		// ------------------------------------------------------------
		//
		case ID_CHECKBOX_0:
		{
			switch(NCode)
		    {
				case WM_NOTIFICATION_CLICKED:
					break;
				case WM_NOTIFICATION_RELEASED:
					break;
				case WM_NOTIFICATION_VALUE_CHANGED:
				{
					hItem = WM_GetDialogItem(pMsg->hWin, ID_CHECKBOX_0);

					// Save state, switch will be done by the bt task in audio.c
					tsu.bt_enabled = CHECKBOX_GetState(hItem);

					break;
				}
				default:
					break;
		    }
			break;
		}

#if 0
		// -------------------------------------------------------------
		//
		case ID_BUTTON_DSP_RESET:
		{
			switch(NCode)
			{
				case WM_NOTIFICATION_RELEASED:
				{
					printf("...power off\r\n");
					vTaskDelay(300);

					#if 0
					GPIO_InitTypeDef  GPIO_InitStruct;

					// PG11 is power hold
					GPIO_InitStruct.Pin   = GPIO_PIN_11;
					GPIO_InitStruct.Mode  = GPIO_MODE_INPUT;	//GPIO_MODE_OUTPUT_PP;
					//GPIO_InitStruct.Pull  = GPIO_PULLUP;
					GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
					HAL_GPIO_Init(GPIOG, &GPIO_InitStruct);
					//HAL_GPIO_WritePin(GPIOG, GPIO_PIN_11, 1);	// drop power
					#endif

					power_off();
					break;
				}
			}
			break;
		}

		// -------------------------------------------------------------
		// Button - reset eeprom to default
		case ID_BUTTON_EEP_RESET:
		{
			switch(NCode)
			{
				case WM_NOTIFICATION_RELEASED:
				{
					HAL_PWREx_EnterSTOPMode(PWR_MAINREGULATOR_ON, PWR_STOPENTRY_WFI, PWR_D3_DOMAIN);
					break;
				}
			}
			break;
		}
#endif
		// -------------------------------------------------------------
		default:
			break;
	}
}

static void _cbDialog(WM_MESSAGE * pMsg)
{
	WM_HWIN 			hItem;
	int 				Id, NCode;

	switch (pMsg->MsgId)
	{
		case WM_INIT_DIALOG:
		{
			// Init Checkbox
			hItem = WM_GetDialogItem(pMsg->hWin, ID_CHECKBOX_0);
			CHECKBOX_SetFont(hItem,&GUI_Font16_1);
			CHECKBOX_SetText(hItem, "Enable BT Audio");
			CHECKBOX_SetState(hItem, tsu.bt_enabled);

			#ifdef CONTEXT_GPS
			{
				int i;

				hItem = WM_GetDialogItem(pMsg->hWin, GUI_ID_TEXT1);
				TEXT_SetFont(hItem, &GUI_Font20_1);
				TEXT_SetTextColor(hItem, GUI_WHITE);

				for(i = 0; i < 4; i++)
				{
					hItem = WM_GetDialogItem(pMsg->hWin, GUI_ID_TEXT2 + i);
					TEXT_SetFont(hItem, &GUI_Font16_1);
					TEXT_SetTextColor(hItem, GUI_WHITE);
				}

				_gps_calib_refresh(pMsg->hWin);

				// A measurement runs for hours, so the page only needs a
				// slow tick to follow it
				hTimerGpsCal = WM_CreateTimer(pMsg->hWin, 0, GPS_CAL_TIMER_MS, 0);
			}
			#endif

			break;
		}

		#ifdef CONTEXT_GPS
		case WM_TIMER:
		{
			_gps_calib_refresh(pMsg->hWin);
			WM_RestartTimer(pMsg->Data.v, GPS_CAL_TIMER_MS);
			break;
		}
		#endif

		case WM_PAINT:
		{
			// NOT WORKING !!!
			/*
			char buff[50];

			if(uls.force_ui_repaint)
			{
				memset(buff, 0, sizeof(buff));
				if(ui_lora_str_state(uls.last_event, buff) == 0)
				{
					hItem = WM_GetDialogItem(pMsg->hWin, GUI_ID_TEXT0);
					TEXT_SetText(hItem, buff);
				}
				uls.force_ui_repaint = 0;
			}
			*/

			break;
		}

		case WM_DELETE:
			#ifdef CONTEXT_GPS
			// The measurement itself keeps running in the PPS ISR - only
			// the read out timer belongs to this window
			WM_DeleteTimer(hTimerGpsCal);
			#endif
			break;

		case WM_NOTIFY_PARENT:
		{
			Id    = WM_GetId(pMsg->hWinSrc);    /* Id of widget */
			NCode = pMsg->Data.v;               /* Notification code */

			_cbControl(pMsg,Id,NCode);
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

	hLdialog = GUI_CreateDialogBox(p_widget, GUI_COUNTOF(_aDialog), _cbDialog, hWin, xpos, ypos);

	free(p_widget);
	return;

use_const_decl:
	hLdialog = GUI_CreateDialogBox(_aDialog, GUI_COUNTOF(_aDialog), _cbDialog, hWin, xpos, ypos);
}

static void KillLora(void)
{
	//printf("kill menu\r\n");
	GUI_EndDialog(hLdialog, 0);
}

#endif

