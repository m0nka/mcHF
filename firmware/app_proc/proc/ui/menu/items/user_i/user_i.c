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
#include "mchf_pro_board.h"
#include "main.h"

#ifdef CONTEXT_VIDEO

#include "ui_menu_layout.h"
#include "gui.h"
#include "dialog.h"

#include "ui_menu_module.h"
#include "user_i_icons.h"

#include "shared_tim.h"

extern GUI_CONST_STORAGE GUI_BITMAP bmicon_consumer;

// UI driver public state
extern struct	UI_DRIVER_STATE			ui_s;
  
// Public radio state
extern struct	TRANSCEIVER_STATE_UI	tsu;

// FreeRTOS process state
extern struct PROC_STATE 				ps;

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
//#define ID_BUTTON_EXIT            (GUI_ID_USER + 0x01)

#define ID_CHECKBOX_0				(GUI_ID_USER + 0x02)
#define ID_CHECKBOX_1				(GUI_ID_USER + 0x03)
#define ID_CHECKBOX_2				(GUI_ID_USER + 0x04)
#define ID_CHECKBOX_3				(GUI_ID_USER + 0x05)
#define ID_CHECKBOX_4				(GUI_ID_USER + 0x06)

//#define ID_RADIO_0         		(GUI_ID_USER + 0x05)
#define ID_ICONVIEW_0    			(GUI_ID_USER + 0x07)

// Quick launch keys - one dropdown per key, F2..F4
#define ID_DROPDOWN_FKEY			(GUI_ID_USER + 0x08)		// + key index, 3 ids
#define ID_TEXT_FKEY_HDR			(GUI_ID_USER + 0x0B)
#define ID_TEXT_FKEY				(GUI_ID_USER + 0x0C)		// + key index, 3 ids
#define ID_TEXT_FKEY_NOTE			(GUI_ID_USER + 0x0F)

#define FKEY_DD_TEXT_H				40		// closed dropdown, touch sized
#define FKEY_DD_ITEM_H				40		// one row of the open list
#define FKEY_DD_LIST_MAX			(FKEY_APP_COUNT * FKEY_DD_ITEM_H)

// Indexed by FKEY_APP_xx
static const char * const fkey_app_names[FKEY_APP_COUNT] =
{
	"FT8", "MeshCore", "MarsChat", "WSPR", "None"
};

// Dropdown rows - only the apps that are in this build
static const uchar fkey_menu_apps[] =
{
	#ifdef CONTEXT_FT8
	FKEY_APP_FT8,
	#endif
	#ifdef CONTEXT_MESHCORE
	FKEY_APP_MESHCORE,
	#endif
	#ifdef CONTEXT_MARSCHAT
	FKEY_APP_MARSCHAT,
	#endif
	#ifdef CONTEXT_WSPR
	FKEY_APP_WSPR,
	#endif
	FKEY_APP_NONE
};

// Set while the dialog moves a dropdown itself, so the SEL_CHANGED that
// DROPDOWN_SetSel() sends back is not taken for an operator pick
static uchar fkey_dd_busy;

#define FKEY_MENU_ROWS				((int)GUI_COUNTOF(fkey_menu_apps))

static const GUI_WIDGET_CREATE_INFO _aDialog[] =
{
	// -----------------------------------------------------------------------------------------------------------------------------
	//							name			id					x		y		xsize	ysize	?		?		?
	// -----------------------------------------------------------------------------------------------------------------------------
	// Self
	{ WINDOW_CreateIndirect,	"", 			ID_WINDOW_0,		0,    	0,		800,	430, 	0, 		0x64, 	0 },
	//
	// Backlight slider
	{ TEXT_CreateIndirect,     	"Backlight" ,	0,                	10,		10,  	100,  	20, 			TEXT_CF_LEFT 				},
	{ EDIT_CreateIndirect,     	NULL,     		GUI_ID_EDIT0,   	10,  	35,  	40,  	40, 			EDIT_CI_DISABELD,		3 	},
	{ SLIDER_CreateIndirect,   	NULL,     		GUI_ID_SLIDER0,		60, 	35, 	400,  	40 									},
	//
	// Check boxes
	{ CHECKBOX_CreateIndirect,	"", 			ID_CHECKBOX_0, 		20, 	160,	250, 	30, 	0, 		0x0, 	0 },
	{ CHECKBOX_CreateIndirect,	"", 			ID_CHECKBOX_1, 		20, 	200,	250, 	30, 	0, 		0x0, 	0 },
	{ CHECKBOX_CreateIndirect,	"", 			ID_CHECKBOX_2, 		20, 	240,	250, 	30, 	0, 		0x0, 	0 },
	{ CHECKBOX_CreateIndirect,	"", 			ID_CHECKBOX_3, 		20, 	280,	250, 	30, 	0, 		0x0, 	0 },
	{ CHECKBOX_CreateIndirect,	"", 			ID_CHECKBOX_4, 		20, 	100,	250, 	30, 	0, 		0x0, 	0 },
	//
	// Quick launch keys (ysize of a dropdown = closed box + open list)
	{ TEXT_CreateIndirect,		"Quick launch keys", ID_TEXT_FKEY_HDR,	520,	16,		262,	20,		TEXT_CF_HCENTER|TEXT_CF_VCENTER		},
	{ TEXT_CreateIndirect,		"F2",			ID_TEXT_FKEY + 0,	520,	50,		50,		44,		TEXT_CF_LEFT|TEXT_CF_VCENTER		},
	{ DROPDOWN_CreateIndirect,	NULL,			ID_DROPDOWN_FKEY + 0, 572,	50,		210,	44 + FKEY_DD_LIST_MAX, 0, 0x0, 0	},
	{ TEXT_CreateIndirect,		"F3",			ID_TEXT_FKEY + 1,	520,	110,	50,		44,		TEXT_CF_LEFT|TEXT_CF_VCENTER		},
	{ DROPDOWN_CreateIndirect,	NULL,			ID_DROPDOWN_FKEY + 1, 572,	110,	210,	44 + FKEY_DD_LIST_MAX, 0, 0x0, 0	},
	{ TEXT_CreateIndirect,		"F4",			ID_TEXT_FKEY + 2,	520,	170,	50,		44,		TEXT_CF_LEFT|TEXT_CF_VCENTER		},
	{ DROPDOWN_CreateIndirect,	NULL,			ID_DROPDOWN_FKEY + 2, 572,	170,	210,	44 + FKEY_DD_LIST_MAX, 0, 0x0, 0	},
	{ TEXT_CreateIndirect,		"Picking an app that is already on\nanother key swaps the two keys.", ID_TEXT_FKEY_NOTE, 520, 228, 262, 40, TEXT_CF_LEFT },
	//
	// Radio box						    																(spacing << 8)|(no_items)
	//{ RADIO_CreateIndirect, 	"Radio", 					ID_RADIO_0, 		500, 	300, 	160, 	80, 	0, 		0x2003,	0 },
	//{ ICONVIEW_CreateIndirect, "Iconview", 					ID_ICONVIEW_0, 		12, 	50, 	768, 	180, 	0, 	0x009800fc, 0 },
};

//extern 	osMessageQId 			hEspMessage;
//struct 	ESPMessage				esp_msg_x;

//extern TaskHandle_t 					hVfoTask;

uchar user_i_theme_id;

//*----------------------------------------------------------------------------
//* Function Name       : user_i_fkey_row
//* Object              : dropdown row of an app, the None row if the app is
//*						: not in this build
//*----------------------------------------------------------------------------
static int user_i_fkey_row(uchar app)
{
	int i;

	for(i = 0; i < FKEY_MENU_ROWS; i++)
	{
		if(fkey_menu_apps[i] == app)
			return i;
	}

	return (FKEY_MENU_ROWS - 1);
}

//*----------------------------------------------------------------------------
//* Function Name       : user_i_fkey_init
//* Object              : style and fill the quick launch dropdowns
//*----------------------------------------------------------------------------
static void user_i_fkey_init(WM_HWIN hDlg)
{
	WM_HWIN hItem;
	int 	k, i;

	hItem = WM_GetDialogItem(hDlg, ID_TEXT_FKEY_HDR);
	TEXT_SetFont(hItem, &GUI_Font16_1);
	TEXT_SetBkColor(hItem, GUI_LIGHTBLUE);
	TEXT_SetTextColor(hItem, GUI_WHITE);

	hItem = WM_GetDialogItem(hDlg, ID_TEXT_FKEY_NOTE);
	TEXT_SetFont(hItem, &GUI_Font16_1);
	TEXT_SetTextColor(hItem, GUI_GRAY);

	fkey_dd_busy = 1;

	for(k = 0; k < FKEY_COUNT; k++)
	{
		hItem = WM_GetDialogItem(hDlg, ID_TEXT_FKEY + k);
		TEXT_SetFont(hItem, &GUI_Font24B_1);
		TEXT_SetTextColor(hItem, GUI_LIGHTBLUE);

		hItem = WM_GetDialogItem(hDlg, ID_DROPDOWN_FKEY + k);
		DROPDOWN_SetFont(hItem, &GUI_Font20_1);
		DROPDOWN_SetTextHeight(hItem, FKEY_DD_TEXT_H);
		DROPDOWN_SetItemSpacing(hItem, (FKEY_DD_ITEM_H - GUI_Font20_1.YSize));
		DROPDOWN_SetListHeight(hItem, (FKEY_MENU_ROWS * FKEY_DD_ITEM_H));

		for(i = 0; i < FKEY_MENU_ROWS; i++)
			DROPDOWN_AddString(hItem, fkey_app_names[fkey_menu_apps[i]]);

		DROPDOWN_SetSel(hItem, user_i_fkey_row(tsu.fkey_app[k]));
	}

	fkey_dd_busy = 0;
}

//*----------------------------------------------------------------------------
//* Function Name       : user_i_fkey_pick
//* Object              : operator picked an app for a key. If another key
//*						: has it already, that key gets this key's old app,
//*						: so no app is ever on two keys. Saved by the
//*						: storage task's idle autosave and settings.ini
//*----------------------------------------------------------------------------
static void user_i_fkey_pick(WM_HWIN hDlg, int k)
{
	WM_HWIN hItem;
	uchar	app, old;
	int		j, sel;

	if(fkey_dd_busy)
		return;

	sel = DROPDOWN_GetSel(WM_GetDialogItem(hDlg, ID_DROPDOWN_FKEY + k));
	if((sel < 0) || (sel >= FKEY_MENU_ROWS))
		return;

	app = fkey_menu_apps[sel];
	old = tsu.fkey_app[k];
	if(app == old)
		return;

	fkey_dd_busy = 1;

	if(app != FKEY_APP_NONE)
	{
		for(j = 0; j < FKEY_COUNT; j++)
		{
			if((j == k) || (tsu.fkey_app[j] != app))
				continue;

			tsu.fkey_app[j] = old;

			hItem = WM_GetDialogItem(hDlg, ID_DROPDOWN_FKEY + j);
			DROPDOWN_SetSel(hItem, user_i_fkey_row(old));
		}
	}

	tsu.fkey_app[k] = app;

	fkey_dd_busy = 0;

	printf("F%d -> %s\r\n", (k + 2), fkey_app_names[app]);
}

static void _cbControl(WM_MESSAGE * pMsg, int Id, int NCode)
{
	WM_HWIN hItem;
	WM_HWIN hSlider;
	WM_HWIN hEdit;
	ulong 	v = 0;

	// Quick launch key dropdowns
	if((Id >= ID_DROPDOWN_FKEY) && (Id < (ID_DROPDOWN_FKEY + FKEY_COUNT)))
	{
		if(NCode == WM_NOTIFICATION_SEL_CHANGED)
			user_i_fkey_pick(pMsg->hWin, (Id - ID_DROPDOWN_FKEY));

		return;
	}

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

		// Audio gain slider
		case GUI_ID_SLIDER0:
		{
			if(NCode != WM_NOTIFICATION_VALUE_CHANGED)
				break;

			hSlider = WM_GetDialogItem(pMsg->hWin, GUI_ID_SLIDER0);
			hEdit   = WM_GetDialogItem(pMsg->hWin, GUI_ID_EDIT0);

			// Get slider position
			v = SLIDER_GetValue(hSlider);

			// Update Edit box
			EDIT_SetValue(hEdit, v);

			tsu.brightness = (uchar)v;
			shared_tim_change(tsu.brightness);

			// Update eeprom
//!			WRITE_EEPROM(EEP_BRIGHTNESS, tsu.brightness);

			break;
		}

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
					tsu.wf_enabled = CHECKBOX_GetState(hItem);

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
					tsu.smet_type = CHECKBOX_GetState(hItem);

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

					tsu.sc_enabled = CHECKBOX_GetState(hItem);

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
						if(ps.hVfoTask != NULL)
							xTaskNotify(ps.hVfoTask, 44, eSetValueWithOverwrite);
					}
					else
					{
						// Change demo mode
						tsu.demo_mode = 0;
					}

					// Save to eeprom
//!					*(uchar *)(EEP_BASE + EEP_DEMO_MODE) = tsu.demo_mode;

					break;
				}
				default:
					break;
		    }
			break;
		}

		// ------------------------------------------------------------
		//
		case ID_CHECKBOX_4:
		{
			switch(NCode)
		    {
				case WM_NOTIFICATION_CLICKED:
					break;
				case WM_NOTIFICATION_RELEASED:
					break;
				case WM_NOTIFICATION_VALUE_CHANGED:
				{
					hItem = WM_GetDialogItem(pMsg->hWin, ID_CHECKBOX_4);

					if(CHECKBOX_GetState(hItem))
					{
						tsu.pwm_backlight = 1;
					}
					else
					{
						tsu.pwm_backlight = 0;
					}

					// Re-init
					shared_tim_init();
					shared_tim_change(tsu.brightness);

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
#if 0
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
#endif
		// -------------------------------------------------------------
		default:
			break;
	}
}

static void _cbDialog(WM_MESSAGE * pMsg)
{
	WM_HWIN 			hItem, hSlider, hEdit;
	int 				Id, NCode;
	//const void * pData;
	//U32          FileSize;
	WM_HWIN hDlg;

	hDlg = pMsg->hWin;

	switch (pMsg->MsgId)
	{
		case WM_INIT_DIALOG:
		{
			// Save public on init
			user_i_theme_id = ui_s.theme_id;

			#if 0
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
			#endif

			// Fix edit box
			hEdit = WM_GetDialogItem(hDlg, GUI_ID_EDIT0);
			EDIT_SetFont(hEdit,&GUI_Font16_1);
			EDIT_SetBkColor(hEdit,EDIT_CI_ENABLED,GUI_LIGHTBLUE);
			EDIT_SetTextColor(hEdit,EDIT_CI_ENABLED,GUI_WHITE);
			EDIT_SetTextAlign(hEdit,TEXT_CF_HCENTER|TEXT_CF_VCENTER);

			// Fix scroll
			hSlider = WM_GetDialogItem(hDlg, GUI_ID_SLIDER0);
			SLIDER_SetWidth(hSlider, 20);
			SLIDER_SetRange(hSlider,0, 100);

			SLIDER_SetValue(hSlider, tsu.brightness);
			EDIT_SetDecMode(hEdit, tsu.brightness, 0, 100, 0, 0);
			WM_SetFocus(hSlider);

			// Init Checkbox
			hItem = WM_GetDialogItem(pMsg->hWin, ID_CHECKBOX_0);
			CHECKBOX_SetFont(hItem,&GUI_Font16_1);
			CHECKBOX_SetText(hItem, "Waterfall Enabled");
			CHECKBOX_SetState(hItem, tsu.wf_enabled);

			// Init Checkbox
			hItem = WM_GetDialogItem(pMsg->hWin, ID_CHECKBOX_1);
			CHECKBOX_SetFont(hItem,&GUI_Font16_1);
			CHECKBOX_SetText(hItem, "Enable Analogue S-Meter");
			//CHECKBOX_SetState(hItem, *(uchar *)(EEP_BASE + EEP_AN_MET_ON));
			CHECKBOX_SetState(hItem, tsu.smet_type);

			// Init Checkbox
			hItem = WM_GetDialogItem(pMsg->hWin, ID_CHECKBOX_2);
			CHECKBOX_SetFont(hItem,&GUI_Font16_1);
			CHECKBOX_SetText(hItem, "Spectrum Scope Enabled");
			CHECKBOX_SetState(hItem, tsu.sc_enabled);

			// Init Checkbox
			hItem = WM_GetDialogItem(pMsg->hWin, ID_CHECKBOX_3);
			CHECKBOX_SetFont(hItem,&GUI_Font16_1);
			CHECKBOX_SetText(hItem, "Demo Mode");
			CHECKBOX_SetState(hItem, tsu.demo_mode);

			hItem = WM_GetDialogItem(pMsg->hWin, ID_CHECKBOX_4);
			CHECKBOX_SetFont(hItem,&GUI_Font16_1);
			CHECKBOX_SetText(hItem, "PWM Backlight");
			CHECKBOX_SetState(hItem, tsu.pwm_backlight);

			// Quick launch keys
			user_i_fkey_init(hDlg);

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
			//--hKeypad = GUI_CreateKeyPad(WM_GetDesktopWindowEx(0));

			//esp_msg_x.ucProcStatus = TASK_PROC_IDLE;
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

		#if 0
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

	if(hUdialog)
	{
		GUI_EndDialog(hUdialog, 0);
		hUdialog = 0;
	}
}

#endif
