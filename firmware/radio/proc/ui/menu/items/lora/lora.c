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
#include "main.h"
#include "mchf_pro_board.h"

#ifdef CONTEXT_VIDEO

#include "ui_menu_layout.h"
#include "gui.h"
#include "dialog.h"

#include "ui_menu_module.h"

#include "ui_lora_state.h"

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
  "GHz Radios",
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
};

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

			break;
		}

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

