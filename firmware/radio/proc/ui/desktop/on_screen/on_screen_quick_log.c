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

//
// ToDo: Fix compatibility with other onscreen dialogs!
//

#ifdef CONTEXT_VIDEO

#include "gui.h"
#include "dialog.h"
//#include "ST_GUI_Addons.h"
#include "ui_proc.h"
#include "rtc.h"
#include "desktop\ui_controls_layout.h"

#include "on_screen_quick_log.h"

// Public UI driver state
extern struct	UI_DRIVER_STATE			ui_s;

// Public radio state
//extern struct	TRANSCEIVER_STATE_UI	tsu;

#define ID_CHECKBOX_0				(GUI_ID_USER + 0x01)
#define ID_TEXT_LIST_2             	(GUI_ID_USER + 0x02)
#define ID_LISTBOX_2         		(GUI_ID_USER + 0x03)

#define ID_TEXT_TIME				(GUI_ID_USER + 0x04)
#define ID_EDIT_TIME        		(GUI_ID_USER + 0x05)

#define ID_TEXT_DATE				(GUI_ID_USER + 0x06)
#define ID_EDIT_DATE        		(GUI_ID_USER + 0x07)

#define ID_TEXT_BAND				(GUI_ID_USER + 0x08)
#define ID_EDIT_BAND        		(GUI_ID_USER + 0x09)

#define ID_TEXT_FREQ				(GUI_ID_USER + 0x0A)
#define ID_EDIT_FREQ        		(GUI_ID_USER + 0x0B)

#define ID_TEXT_MODE				(GUI_ID_USER + 0x0C)
#define ID_EDIT_MODE        		(GUI_ID_USER + 0x0D)

#define ID_TEXT_RSTR				(GUI_ID_USER + 0x0E)
#define ID_EDIT_RSTR        		(GUI_ID_USER + 0x0F)

#define ID_TEXT_RSTS				(GUI_ID_USER + 0x10)
#define ID_EDIT_RSTS        		(GUI_ID_USER + 0x11)

#define ID_TEXT_LOC					(GUI_ID_USER + 0x12)
#define ID_EDIT_LOC	        		(GUI_ID_USER + 0x13)

#define ID_TEXT_CALL				(GUI_ID_USER + 0x14)
#define ID_EDIT_CALL        		(GUI_ID_USER + 0x15)

#define ID_TEXT_NAME				(GUI_ID_USER + 0x16)
#define ID_EDIT_NAME        		(GUI_ID_USER + 0x17)

#define ID_BUTTON_ADD            	(GUI_ID_USER + 0x18)

#define QL_X						254
#define QL_Y						138

static const GUI_WIDGET_CREATE_INFO _aDialogCreate[] =
{
  // -----------------------------------------------------------------------------------------------------------------------------
  //						name						id					x		y		xsize	ysize	?		?		?
  // -----------------------------------------------------------------------------------------------------------------------------
  { FRAMEWIN_CreateIndirect, "Quick Log Entry", 	0,       				190,  	110, 	390, 	245, 	FRAMEWIN_CF_ACTIVE	},
  // Edit boxes
  { TEXT_CreateIndirect,     "Time",				ID_TEXT_TIME,   		5,		7,  	70,  	25, 	0, 	  	0,		0   },
  { EDIT_CreateIndirect,     "time.Edit",  			ID_EDIT_TIME,   		60,  	5,  	100,  	25, 	0,		0,		0 	},
  //
  { TEXT_CreateIndirect,     "Date",				ID_TEXT_DATE,   		5,		37,  	70,  	25, 	0, 	  	0,		0   },
  { EDIT_CreateIndirect,     "date.Edit",    		ID_EDIT_DATE,   		60,  	35,  	100,  	25, 	0,		0x0,	0 	},
  //
  { TEXT_CreateIndirect,     "Band",				ID_TEXT_DATE,   		5,		67,  	70,  	25, 	0, 	  	0,		0   },
  { EDIT_CreateIndirect,     "band.Edit",    		ID_EDIT_BAND,   		60,  	65,  	100,  	25, 	0,		0x0,	0 	},
  //
  { TEXT_CreateIndirect,     "Freq",				ID_TEXT_FREQ,   		5,		97,  	70,  	25, 	0, 	  	0,		0   },
  { EDIT_CreateIndirect,     "freq.Edit",    		ID_EDIT_FREQ,   		60,  	95,  	100,  	25, 	0,		0x0,	0 	},
  //
  { TEXT_CreateIndirect,     "Mode",				ID_TEXT_MODE,   		5,		127,  	70,  	25, 	0, 	  	0,		0   },
  { EDIT_CreateIndirect,     "freq.Edit",    		ID_EDIT_MODE,   		60,  	125,  	100,  	25, 	0,		0x0,	0 	},
  //
  { TEXT_CreateIndirect,     "RST-R",				ID_TEXT_RSTR,   		175,	7,  	70,  	25, 	0, 	  	0,		0   },
  { EDIT_CreateIndirect,     "rstr.Edit",    		ID_EDIT_RSTR,   		235,  	5,  	100,  	25, 	0,		0x0,	0 	},
  //
  { TEXT_CreateIndirect,     "RST-S",				ID_TEXT_RSTS,   		175,	37,  	70,  	25, 	0, 	  	0,		0   },
  { EDIT_CreateIndirect,     "rsts.Edit",    		ID_EDIT_RSTS,   		235,  	35,  	100,  	25, 	0,		0x0,	0 	},
  //
  { TEXT_CreateIndirect,     "Locator",				ID_TEXT_LOC,   			175,	67,  	70,  	25, 	0, 	  	0,		0   },
  { EDIT_CreateIndirect,     "rsts.Edit",    		ID_EDIT_LOC,   			235,  	65,  	100,  	25, 	0,		0x0,	0 	},
  //
  { TEXT_CreateIndirect,     "Callsign",			ID_TEXT_CALL,   		175,	97,  	70,  	25, 	0, 	  	0,		0   },
  { EDIT_CreateIndirect,     "rsts.Edit",    		ID_EDIT_CALL,   		235,  	95,  	100,  	25, 	0,		0x0,	0 	},
  //
  { TEXT_CreateIndirect,     "Name",				ID_TEXT_NAME,   		175,	127,  	70,  	25, 	0, 	  	0,		0   },
  { EDIT_CreateIndirect,     "rsts.Edit",    		ID_EDIT_NAME,   		235,  	125,  	100,  	25, 	0,		0x0,	0 	},
  // Buttons
  { BUTTON_CreateIndirect,	 "Add",					ID_BUTTON_ADD, 			60, 	165, 	275, 	40, 	0, 		0x0, 	0   },
};

WM_HWIN hQuickLogWin = 0;

//*----------------------------------------------------------------------------
//* Function Name       : ui_quick_log_show_clock
//* Object              : Log time and date
//* Input Parameters    :
//* Output Parameters   :
//* Functions called    :
//*----------------------------------------------------------------------------
static void on_screen_quick_log_show_clock(WM_MESSAGE * pMsg)
{
#if 0
	WM_HWIN 			hEdit;
	char 				buf[50];

	RTC_DateTypeDef     Date;
	RTC_TimeTypeDef     Time;

	WM_InvalidateWindow(pMsg->hWin);

	// Due to STM32, need to read both, in exact order, otherwise subsequent calls will fail
	k_GetTime(&Time);
	k_GetDate(&Date);

	EnterCriticalSection();
	sprintf(buf,"%02d:%02d:%02d",Time.Hours,Time.Minutes,Time.Seconds);
	ExitCriticalSection();

	hEdit = WM_GetDialogItem(pMsg->hWin, ID_EDIT_TIME);
	EDIT_SetText(hEdit, buf);

	EnterCriticalSection();
	sprintf(buf,"%04d - %02d - %02d",(2018 + Date.Year), Date.Month, Date.Date);
	ExitCriticalSection();

	hEdit = WM_GetDialogItem(pMsg->hWin, ID_EDIT_DATE);
	EDIT_SetText(hEdit, buf);
#endif
}

//*----------------------------------------------------------------------------
//* Function Name       : _cbControl
//* Object              : Control related events(touch, change value, etc)
//* Input Parameters    :
//* Output Parameters   :
//* Functions called    :
//*----------------------------------------------------------------------------
static void _cbControl(WM_MESSAGE *pMsg, int Id,int NCode)
{
	switch(Id)
	{
		// -------------------------------------------------------------
		// Add Button - add entry to log, then exit
		//			    we can also return by holding the '0' button,
		// 				but nothing will be saved
		case ID_BUTTON_ADD:
		{
			if(NCode == WM_NOTIFICATION_RELEASED)
			{
				// Create log entry
				// ...

				// Change public state
				ui_s.req_state = MODE_DESKTOP;

				// Exit
				GUI_EndDialog(pMsg->hWin, 0);
			}

			break;
		}
		default:
			break;
	}
	WM_InvalidateWindow(WM_GetClientWindow(pMsg->hWin));
}

//*----------------------------------------------------------------------------
//* Function Name       : _cbBkWindow
//* Object              : Foreground window handler
//* Input Parameters    :
//* Output Parameters   :
//* Functions called    :
//*----------------------------------------------------------------------------
static void _cbBkWindow(WM_MESSAGE* pMsg)
{
	switch (pMsg->MsgId)
	{
		case WM_PAINT:
			break;
		default:
			WM_DefaultProc(pMsg);
	}
}

//*----------------------------------------------------------------------------
//* Function Name       : _cbCallback
//* Object              : Dialog handler
//* Input Parameters    :
//* Output Parameters   :
//* Functions called    :
//*----------------------------------------------------------------------------
static void _cbCallback(WM_MESSAGE * pMsg)
{
	WM_HWIN 			hDlg,hEdit,hItem;

	int     Sel;
	int     NCode;
	int     Id;
	int     i;

	GUI_PID_STATE * pState;
	hDlg = pMsg->hWin;

	switch (pMsg->MsgId)
	{
    	case WM_INIT_DIALOG:
    	{
			// Time Edit box
			hEdit = WM_GetDialogItem(hDlg, ID_EDIT_TIME);
			EDIT_SetFont(hEdit,&GUI_Font20B_1);
			EDIT_SetBkColor(hEdit,EDIT_CI_ENABLED,GUI_BLACK);
			EDIT_SetTextColor(hEdit,EDIT_CI_ENABLED,GUI_YELLOW);
			EDIT_SetTextAlign(hEdit,TEXT_CF_HCENTER|TEXT_CF_VCENTER);
			// Date Edit box
			hEdit = WM_GetDialogItem(hDlg, ID_EDIT_DATE);
			EDIT_SetFont(hEdit,&GUI_Font16B_1);
			EDIT_SetBkColor(hEdit,EDIT_CI_ENABLED,GUI_BLACK);
			EDIT_SetTextColor(hEdit,EDIT_CI_ENABLED,GUI_YELLOW);
			EDIT_SetTextAlign(hEdit,TEXT_CF_HCENTER|TEXT_CF_VCENTER);
			EDIT_SetMaxLen(hEdit,16);												// default 8 char limit is not enough
			// Show clock
			on_screen_quick_log_show_clock(pMsg);
			//
			// Band Edit box
			hEdit = WM_GetDialogItem(hDlg, ID_EDIT_BAND);
			EDIT_SetFont(hEdit,&GUI_Font16B_1);
			EDIT_SetBkColor(hEdit,EDIT_CI_ENABLED,GUI_BLACK);
			EDIT_SetTextColor(hEdit,EDIT_CI_ENABLED,GUI_GREEN);
			EDIT_SetTextAlign(hEdit,TEXT_CF_HCENTER|TEXT_CF_VCENTER);
			EDIT_SetText(hEdit,"20m");
			// Frequency Edit box
			hEdit = WM_GetDialogItem(hDlg, ID_EDIT_FREQ);
			EDIT_SetFont(hEdit,&GUI_Font20B_1);
			EDIT_SetBkColor(hEdit,EDIT_CI_ENABLED,GUI_BLACK);
			EDIT_SetTextColor(hEdit,EDIT_CI_ENABLED,GUI_GREEN);
			EDIT_SetTextAlign(hEdit,TEXT_CF_HCENTER|TEXT_CF_VCENTER);
			EDIT_SetMaxLen(hEdit,24);												// default 8 char limit is not enough
			EDIT_SetText(hEdit,"14.074.000");
			// Mode Edit box
			hEdit = WM_GetDialogItem(hDlg, ID_EDIT_MODE);
			EDIT_SetFont(hEdit,&GUI_Font20_1);
			EDIT_SetBkColor(hEdit,EDIT_CI_ENABLED,GUI_BLACK);
			EDIT_SetTextColor(hEdit,EDIT_CI_ENABLED,GUI_GREEN);
			EDIT_SetTextAlign(hEdit,TEXT_CF_HCENTER|TEXT_CF_VCENTER);
			EDIT_SetText(hEdit,"SSB");
			// RST-R Edit box
			hEdit = WM_GetDialogItem(hDlg, ID_EDIT_RSTR);
			EDIT_SetFont(hEdit,&GUI_Font20_1);
			EDIT_SetBkColor(hEdit,EDIT_CI_ENABLED,GUI_BLACK);
			EDIT_SetTextColor(hEdit,EDIT_CI_ENABLED,GUI_GREEN);
			EDIT_SetTextAlign(hEdit,TEXT_CF_HCENTER|TEXT_CF_VCENTER);
			EDIT_SetText(hEdit,"599");
			// RST-S Edit box
			hEdit = WM_GetDialogItem(hDlg, ID_EDIT_RSTS);
			EDIT_SetFont(hEdit,&GUI_Font20_1);
			EDIT_SetBkColor(hEdit,EDIT_CI_ENABLED,GUI_BLACK);
			EDIT_SetTextColor(hEdit,EDIT_CI_ENABLED,GUI_GREEN);
			EDIT_SetTextAlign(hEdit,TEXT_CF_HCENTER|TEXT_CF_VCENTER);
			EDIT_SetText(hEdit,"599");
			// Locator Edit box
			hEdit = WM_GetDialogItem(hDlg, ID_EDIT_LOC);
			EDIT_SetFont(hEdit,&GUI_Font20_1);
			EDIT_SetBkColor(hEdit,EDIT_CI_ENABLED,GUI_BLACK);
			EDIT_SetTextColor(hEdit,EDIT_CI_ENABLED,GUI_GREEN);
			EDIT_SetTextAlign(hEdit,TEXT_CF_HCENTER|TEXT_CF_VCENTER);
			// Name Edit box
			hEdit = WM_GetDialogItem(hDlg, ID_EDIT_NAME);
			EDIT_SetFont(hEdit,&GUI_Font20_1);
			EDIT_SetBkColor(hEdit,EDIT_CI_ENABLED,GUI_BLACK);
			EDIT_SetTextColor(hEdit,EDIT_CI_ENABLED,GUI_GREEN);
			EDIT_SetTextAlign(hEdit,TEXT_CF_HCENTER|TEXT_CF_VCENTER);
			// Callsign Edit box
			hEdit = WM_GetDialogItem(hDlg, ID_EDIT_CALL);
			EDIT_SetFont(hEdit,&GUI_Font20_1);
			EDIT_SetBkColor(hEdit,EDIT_CI_ENABLED,GUI_BLACK);
			EDIT_SetTextColor(hEdit,EDIT_CI_ENABLED,GUI_GREEN);
			EDIT_SetTextAlign(hEdit,TEXT_CF_HCENTER|TEXT_CF_VCENTER);
			//
			WM_SetFocus(hItem); // doesn't work!!!

    		break;
    	}

    	// ToDo: Implement proper keypad handling to allow to control
    	//       only from the keyboard, in case we have a faulty touch screen
    	case WM_KEY:
    	{
    		switch (((WM_KEY_INFO*)(pMsg->Data.p))->Key)
    		{
    			case GUI_KEY_ESCAPE:
    				GUI_EndDialog(hDlg, 1);
    				break;
    			case GUI_KEY_ENTER:
    				GUI_EndDialog(hDlg, 0);
    				break;
    		}
    		break;
    	}

    	case WM_NOTIFY_PARENT:
    	{
    		Id    = WM_GetId(pMsg->hWinSrc);      // Id of widget
    		NCode = pMsg->Data.v;                 // Notification code

    		_cbControl(pMsg, Id,NCode);
    		break;
    	}

    	default:
    		WM_DefaultProc(pMsg);
	}
}

//*----------------------------------------------------------------------------
//* Function Name       : on_screen_quick_log_set_profile
//* Object              :
//* Input Parameters    :
//* Output Parameters   :
//* Functions called    :
//*----------------------------------------------------------------------------
static void on_screen_quick_log_set_profile(void)
{
	TEXT_SetDefaultTextColor(GUI_STCOLOR_LIGHTBLUE);
	TEXT_SetDefaultFont(&GUI_Font16_1);
}

//*----------------------------------------------------------------------------
//* Function Name       : on_screen_quick_log_create
//* Object              :
//* Input Parameters    :
//* Output Parameters   :
//* Functions called    :
//*----------------------------------------------------------------------------
void on_screen_quick_log_create(WM_HWIN hParent)
{
	on_screen_quick_log_set_profile();

	WM_SetCallback(WM_HBKWIN, &_cbBkWindow);
	hQuickLogWin = GUI_CreateDialogBox(_aDialogCreate, GUI_COUNTOF(_aDialogCreate), &_cbCallback, hParent, QL_X, QL_Y);
}

//*----------------------------------------------------------------------------
//* Function Name       : on_screen_quick_log_destroy
//* Object              :
//* Input Parameters    :
//* Output Parameters   :
//* Functions called    :
//*----------------------------------------------------------------------------
void on_screen_quick_log_destroy(void)
{
	if(hQuickLogWin)
	{
		WM_SetCallback		(WM_HBKWIN, 0);
		WM_InvalidateWindow	(WM_HBKWIN);
		WM_DeleteWindow(hQuickLogWin);
	}
}

#endif
