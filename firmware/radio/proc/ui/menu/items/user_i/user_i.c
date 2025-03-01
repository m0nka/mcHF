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

#ifdef CONTEXT_VIDEO

#include "ui_menu_layout.h"
#include "gui.h"
#include "dialog.h"

#include "ui_menu_module.h"
#include "user_i_icons.h"

extern GUI_CONST_STORAGE GUI_BITMAP bmicon_consumer;

// UI driver public state
extern struct	UI_DRIVER_STATE			ui_s;
  
// Public radio state
extern struct	TRANSCEIVER_STATE_UI	tsu;

// Menu layout definitions from Flash
extern const struct UIMenuLayout menu_layout[];

static void Startup(WM_HWIN hWin, uint16_t xpos, uint16_t ypos);
static void KillUI(void);

WM_HWIN 		hKeypad;

K_ModuleItem_Typedef  user_i =
{
  6,
  "User Interface",
  &bmicon_consumer,
  Startup,
  NULL,
  KillUI
};

WM_HWIN   	hUdialog;

#define ID_WINDOW_0               	(GUI_ID_USER + 0x00)
//#define ID_BUTTON_EXIT            	(GUI_ID_USER + 0x01)

#define ID_CHECKBOX_0				(GUI_ID_USER + 0x02)
#define ID_CHECKBOX_1				(GUI_ID_USER + 0x03)
#define ID_CHECKBOX_2				(GUI_ID_USER + 0x04)
#define ID_CHECKBOX_3				(GUI_ID_USER + 0x05)

//#define ID_RADIO_0         			(GUI_ID_USER + 0x05)
#define ID_ICONVIEW_0    			(GUI_ID_USER + 0x06)

static const GUI_WIDGET_CREATE_INFO _aDialog[] =
{
	// -----------------------------------------------------------------------------------------------------------------------------
	//							name						id					x		y		xsize	ysize	?		?		?
	// -----------------------------------------------------------------------------------------------------------------------------
	// Self
	{ WINDOW_CreateIndirect,	"", 						ID_WINDOW_0,		0,    	0,		800,	430, 	0, 		0x64, 	0 },
	// Back Button
//	{ BUTTON_CreateIndirect, 	"Back",			 			ID_BUTTON_EXIT, 	670, 	375, 	120, 	45, 	0, 		0x0, 	0 },
	// Check boxes
	{ CHECKBOX_CreateIndirect,	"", 						ID_CHECKBOX_0, 		20, 	260,	250, 	30, 	0, 		0x0, 	0 },
	{ CHECKBOX_CreateIndirect,	"", 						ID_CHECKBOX_1, 		20, 	300,	250, 	30, 	0, 		0x0, 	0 },
	{ CHECKBOX_CreateIndirect,	"", 						ID_CHECKBOX_2, 		20, 	340,	250, 	30, 	0, 		0x0, 	0 },
	{ CHECKBOX_CreateIndirect,	"", 						ID_CHECKBOX_3, 		20, 	380,	250, 	30, 	0, 		0x0, 	0 },
	//
	// Radio box						    																(spacing << 8)|(no_items)
	//{ RADIO_CreateIndirect, 	"Radio", 					ID_RADIO_0, 		500, 	300, 	160, 	80, 	0, 		0x2003,	0 },
	//{ ICONVIEW_CreateIndirect, "Iconview", 					ID_ICONVIEW_0, 		12, 	50, 	768, 	180, 	0, 	0x009800fc, 0 },
};

extern 	osMessageQId 			hEspMessage;
struct 	ESPMessage				esp_msg_x;

extern TaskHandle_t 					hVfoTask;

uchar user_i_theme_id;

static void _cbControl(WM_MESSAGE * pMsg, int Id, int NCode)
{
	WM_HWIN hItem;

	switch(Id)
	{
		#if 0
		// -------------------------------------------------------------
		// Button
		case ID_BUTTON_EXIT:
		{
			switch(NCode)
			{
				case WM_NOTIFICATION_RELEASED:
				{
					if(user_i_theme_id != ui_s.theme_id)
					{
						printf("new theme id=%d, needs update on exit\r\n", ui_s.theme_id);

						#if 1
						if(esp_msg_x.ucProcStatus == TASK_PROC_IDLE)
						{
							esp_msg_x.ucMessageID  = 0x04;			// SQLite write
							esp_msg_x.ucProcStatus = TASK_PROC_WORK;

							esp_msg_x.ucData[0] = 0;
							esp_msg_x.ucData[1] = 0;
							esp_msg_x.ucData[2] = 0;
							esp_msg_x.ucData[3] = ui_s.theme_id;

							strcpy((char *)(esp_msg_x.ucData + 5), "theme");
							esp_msg_x.ucData[4] = 5;

							esp_msg_x.usPayloadSize = 11;

							osMessagePut(hEspMessage, (ulong)&esp_msg_x, osWaitForever);
						}
						#endif
					}

					GUI_EndDialog(pMsg->hWin, 0);
					break;
				}
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
					// Save to eeprom
					*(uchar *)(EEP_BASE + EEP_SW_SMOOTH) = CHECKBOX_GetState(hItem);

					break;
				}
				default:
					break;
		    }
			break;
		}

		// ------------------------------------------------------------
		//
		case ID_CHECKBOX_1:
		{
			switch(NCode)
		    {
				case WM_NOTIFICATION_CLICKED:
					break;
				case WM_NOTIFICATION_RELEASED:
					break;
				case WM_NOTIFICATION_VALUE_CHANGED:
				{
					hItem = WM_GetDialogItem(pMsg->hWin, ID_CHECKBOX_1);
					// Save to eeprom
					*(uchar *)(EEP_BASE + EEP_AN_MET_ON) = CHECKBOX_GetState(hItem);

					break;
				}
				default:
					break;
		    }
			break;
		}

		// ------------------------------------------------------------
		//
		case ID_CHECKBOX_2:
		{
			switch(NCode)
		    {
				case WM_NOTIFICATION_CLICKED:
					break;
				case WM_NOTIFICATION_RELEASED:
					break;
				case WM_NOTIFICATION_VALUE_CHANGED:
				{
					hItem = WM_GetDialogItem(pMsg->hWin, ID_CHECKBOX_2);
					// Save to eeprom
					*(uchar *)(EEP_BASE + EEP_KEYER_ON) = CHECKBOX_GetState(hItem);

					break;
				}
				default:
					break;
		    }
			break;
		}

		// ------------------------------------------------------------
		//
		case ID_CHECKBOX_3:
		{
			switch(NCode)
		    {
				case WM_NOTIFICATION_CLICKED:
					break;
				case WM_NOTIFICATION_RELEASED:
					break;
				case WM_NOTIFICATION_VALUE_CHANGED:
				{
					hItem = WM_GetDialogItem(pMsg->hWin, ID_CHECKBOX_3);

					if(CHECKBOX_GetState(hItem))
					{
						// Change demo mode
						tsu.demo_mode = 1;

						// Wake up vfo task(use any notif id)
						if(hVfoTask != NULL)
							xTaskNotify(hVfoTask, 44, eSetValueWithOverwrite);
					}
					else
					{
						// Change demo mode
						tsu.demo_mode = 0;
					}

					// Save to eeprom
					*(uchar *)(EEP_BASE + EEP_DEMO_MODE) = CHECKBOX_GetState(hItem);

					break;
				}
				default:
					break;
		    }
			break;
		}


	#if 0
		case ID_RADIO_0:
		{
			if(NCode == WM_NOTIFICATION_VALUE_CHANGED)
			{
				uchar val;

				hItem = WM_GetDialogItem(pMsg->hWin, ID_RADIO_0);
				val = (uchar)RADIO_GetValue(hItem);
				printf("new theme id=%d\r\n", val);

				if(val < THEME_2)
					ui_s.theme_id = val;
			}
			break;
		}
		#endif

		case ID_ICONVIEW_0:
		{
			if(NCode == WM_NOTIFICATION_RELEASED)
			{
				int sel = ICONVIEW_GetSel(pMsg->hWinSrc);
				//printf("icon %d click\r\n",sel);
				if(sel < MAX_THEME)
					ui_s.theme_id = sel;
			}

			break;
		}

		// -------------------------------------------------------------
		default:
			break;
	}
}

static void _cbDialog(WM_MESSAGE * pMsg)
{
	WM_HWIN 			hItem;
	int 				Id, NCode;
	const void * pData;
	U32          FileSize;

	switch (pMsg->MsgId)
	{
		case WM_INIT_DIALOG:
		{
			// Save public on init
			user_i_theme_id = ui_s.theme_id;

			hItem = ICONVIEW_CreateEx(	4,
			   							10,
										798,
										200,
										pMsg->hWin,
										(WM_CF_SHOW|WM_CF_LATE_CLIP|WM_CF_HASTRANS),
										0,
										ID_ICONVIEW_0,
										264,
										200);

			pData = _GetImageById(ID_ICONVIEW_0_IMAGE_0, &FileSize);
			ICONVIEW_AddStreamedBitmapItem(hItem, pData, menu_layout[THEME_0].name);
			pData = _GetImageById(ID_ICONVIEW_0_IMAGE_1, &FileSize);
			ICONVIEW_AddStreamedBitmapItem(hItem, pData, menu_layout[THEME_1].name);
			pData = _GetImageById(ID_ICONVIEW_0_IMAGE_2, &FileSize);
			ICONVIEW_AddStreamedBitmapItem(hItem, pData, menu_layout[THEME_2].name);

			ICONVIEW_SetFont	 (hItem, &GUI_Font24_1);
			ICONVIEW_SetBkColor	 (hItem, ICONVIEW_CI_UNSEL, GUI_WHITE);
			ICONVIEW_SetBkColor	 (hItem, ICONVIEW_CI_SEL,   GUI_GRAY);
			ICONVIEW_SetTextColor(hItem, ICONVIEW_CI_UNSEL,	GUI_BLACK);
			ICONVIEW_SetTextColor(hItem, ICONVIEW_CI_SEL,	GUI_WHITE);
			ICONVIEW_SetFrame	 (hItem, GUI_COORD_X, 0);
			ICONVIEW_SetFrame	 (hItem, GUI_COORD_Y, 0);
			ICONVIEW_SetSpace	 (hItem, GUI_COORD_X, 0);

			if(ui_s.theme_id < MAX_THEME)
				ICONVIEW_SetSel		(hItem, ui_s.theme_id);
			else
				ICONVIEW_SetSel		(hItem, 0);

			// Init Checkbox
			hItem = WM_GetDialogItem(pMsg->hWin, ID_CHECKBOX_0);
			CHECKBOX_SetFont(hItem,&GUI_Font16_1);
			CHECKBOX_SetText(hItem, "Spectrum Display Smooth Mode");
			CHECKBOX_SetState(hItem, *(uchar *)(EEP_BASE + EEP_SW_SMOOTH));

			// Init Checkbox
			hItem = WM_GetDialogItem(pMsg->hWin, ID_CHECKBOX_1);
			CHECKBOX_SetFont(hItem,&GUI_Font16_1);
			CHECKBOX_SetText(hItem, "Enable Analogue S-Meter");
			CHECKBOX_SetState(hItem, *(uchar *)(EEP_BASE + EEP_AN_MET_ON));

			// Init Checkbox
			hItem = WM_GetDialogItem(pMsg->hWin, ID_CHECKBOX_2);
			CHECKBOX_SetFont(hItem,&GUI_Font16_1);
			CHECKBOX_SetText(hItem, "Show Iambic Keyer Control");
			CHECKBOX_SetState(hItem, *(uchar *)(EEP_BASE + EEP_KEYER_ON));

			// Init Checkbox
			hItem = WM_GetDialogItem(pMsg->hWin, ID_CHECKBOX_3);
			CHECKBOX_SetFont(hItem,&GUI_Font16_1);
			CHECKBOX_SetText(hItem, "Demo Mode");
			CHECKBOX_SetState(hItem, *(uchar *)(EEP_BASE + EEP_DEMO_MODE));

			#if 0
			// Initialization of 'Radio'
			hItem = WM_GetDialogItem(pMsg->hWin, ID_RADIO_0);
			RADIO_SetFont(hItem,&GUI_Font20_1);
			RADIO_SetText(hItem, "Kenwood",	0);
			RADIO_SetText(hItem, "RedDawn",	1);
			RADIO_SetText(hItem, "Icom",	2);

			if(ui_s.theme_id < MAX_THEME)
				RADIO_SetValue(hItem, ui_s.theme_id);
			#endif

			// Doesn't work in menu, maybe create in each individual menu item ?
			hKeypad = GUI_CreateKeyPad(WM_GetDesktopWindowEx(0));

			esp_msg_x.ucProcStatus = TASK_PROC_IDLE;
			break;
		}

		case WM_PAINT:
			break;

		case WM_DELETE:
			break;

		case WM_NOTIFY_PARENT:
		{
			Id    = WM_GetId(pMsg->hWinSrc);    // Id of widget
			NCode = pMsg->Data.v;               // Notification code

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

	hUdialog = GUI_CreateDialogBox(p_widget, GUI_COUNTOF(_aDialog), _cbDialog, hWin, xpos, ypos);

	free(p_widget);
	return;

use_const_decl:
	hUdialog = GUI_CreateDialogBox(_aDialog, GUI_COUNTOF(_aDialog), _cbDialog, hWin, xpos, ypos);
}

static void KillUI(void)
{
	if(user_i_theme_id != ui_s.theme_id)
	{
		printf("new theme id=%d, needs update on exit\r\n", ui_s.theme_id);

		#if 1
		if(esp_msg_x.ucProcStatus == TASK_PROC_IDLE)
		{
			esp_msg_x.ucMessageID  = 0x04;			// SQLite write
			esp_msg_x.ucProcStatus = TASK_PROC_WORK;

			esp_msg_x.ucData[0] = 0;
			esp_msg_x.ucData[1] = 0;
			esp_msg_x.ucData[2] = 0;
			esp_msg_x.ucData[3] = ui_s.theme_id;

			strcpy((char *)(esp_msg_x.ucData + 5), "theme");
			esp_msg_x.ucData[4] = 5;

			esp_msg_x.usPayloadSize = 11;

			osMessagePut(hEspMessage, (ulong)&esp_msg_x, osWaitForever);
		}
		#endif
						}
	//printf("kill menu\r\n");
	GUI_EndDialog(hUdialog, 0);
}

#endif
