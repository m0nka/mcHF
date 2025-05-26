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

#include "ui_menu_layout.h"
#include "gui.h"
#include "dialog.h"

#include "rtc.h"

#include "ui_menu_module.h"
#include "stm32h7xx_hal_rtc.h"

#include "clock.h"

static void Startup(WM_HWIN hWin, uint16_t xpos, uint16_t ypos);
static void KillClock(void);

// UI driver public state
extern struct	UI_DRIVER_STATE			ui_s;

// Menu layout definitions from Flash
extern const struct UIMenuLayout menu_layout[];

extern GUI_CONST_STORAGE GUI_BITMAP bmClockGenerator;

extern RTC_HandleTypeDef RtcHandle;

K_ModuleItem_Typedef  clock =
{
  1,
  "Clock Settings",
  &bmClockGenerator,
  Startup,
  NULL,
  KillClock
};

WM_HWIN   	hCdialog;

#define ID_WINDOW_0               	(GUI_ID_USER + 0x00)
//#define ID_BUTTON_EXIT            	(GUI_ID_USER + 0x01)

#define ID_SPINBOX_HOUR          	(GUI_ID_USER + 0x02)
#define ID_SPINBOX_MINUTE        	(GUI_ID_USER + 0x03)
#define ID_BUTTON_APPLY			  	(GUI_ID_USER + 0x04)
#define ID_CALENDAR              	(GUI_ID_USER + 0x05)
#define ID_SPINBOX_SEC           	(GUI_ID_USER + 0x06)
#define ID_TEXT_SPIN_0             	(GUI_ID_USER + 0x07)
#define ID_BUTTON_LOCK			  	(GUI_ID_USER + 0x08)

static const GUI_WIDGET_CREATE_INFO _aDialog[] = 
{
	// -----------------------------------------------------------------------------------------------------------------------------
	//							name			id					x		y		xsize	ysize	?		?		?
	// -----------------------------------------------------------------------------------------------------------------------------
	// Self
	{ WINDOW_CreateIndirect,	"", 			ID_WINDOW_0,		0,    	0,		854,	430, 	0, 		0x64, 	0 },
	// Header text
	{ TEXT_CreateIndirect, 		"Header1", 		ID_TEXT_SPIN_0,		485,	30,		360, 	50,  	0, 		0x0,	0 },
	//
	{ SPINBOX_CreateIndirect, 	"Spinbox", 		ID_SPINBOX_HOUR, 	485,    88, 	110, 	160, 	0, 		0x0, 	0 },
	//
	{ SPINBOX_CreateIndirect, 	"Spinbox", 		ID_SPINBOX_MINUTE, 	610, 	88, 	110, 	160, 	0, 		0x0, 	0 },
	//
	{ SPINBOX_CreateIndirect, 	"Spinbox", 		ID_SPINBOX_SEC, 	735, 	88, 	110, 	160, 	0, 		0x0, 	0 },
	//
	{ BUTTON_CreateIndirect, 	"Update",		ID_BUTTON_APPLY, 	485, 	270, 	110, 	45, 	0, 		0x0, 	0 },
	{ BUTTON_CreateIndirect, 	"Lock",			ID_BUTTON_APPLY, 	735, 	270, 	110, 	45, 	0, 		0x0, 	0 },
};

GUI_POINT 		aPointsDest[3][4];

WM_HTIMER 		hTimerTime;
uint8_t 		DisableAutoRefresh = 0;
CALENDAR_DATE  	hDate;

static const GUI_POINT aPoints[3][4] = {

  // Hour Needle
  {{ 0 * AA_FACTOR, 2 * AA_FACTOR},
   {-1 * AA_FACTOR,-4 * AA_FACTOR},
   { 0 * AA_FACTOR,-6 * AA_FACTOR},
   { 1 * AA_FACTOR,-4 * AA_FACTOR}},

  // Min Needle
  {{ 0 * AA_FACTOR, 2 * AA_FACTOR},
   {-1 * AA_FACTOR,-2 * AA_FACTOR},
   { 0 * AA_FACTOR,-8 * AA_FACTOR},
   { 1 * AA_FACTOR,-2 * AA_FACTOR}},
   // Sec Needle
  {{0 * AA_FACTOR, 1 * AA_FACTOR},
   { 1 * AA_FACTOR, 1 * AA_FACTOR},
   { 1 * AA_FACTOR,-34 * AA_FACTOR},
   {0 * AA_FACTOR,-34 * AA_FACTOR}},
};

static void DrawNeedle(uint32_t index, uint16_t x0, uint16_t y0) 
{
  /* draw Needles */
  if(index == 2)
  {
    GUI_SetColor(GUI_RED);
    GUI_AA_FillPolygon(aPointsDest[index], 4, AA_FACTOR * x0, AA_FACTOR * y0);
  }
  else 
  {
    GUI_SetColor(GUI_LIGHTBLUE);
    GUI_AA_FillPolygon(aPointsDest[index], 4, AA_FACTOR * x0, AA_FACTOR * y0);
  }
}

static void GUI_UpdateClock(uint16_t x0, uint16_t y0, uint8_t hour, uint8_t min, uint8_t sec)
{
  int8_t i = 0;
  int32_t SinHQ, CosHQ ,a = 0;
  uint16_t xPos, yPos;
    
  GUI_AA_EnableHiRes();
  GUI_AA_SetFactor(AA_FACTOR);
  
  // Outside circle
  GUI_SetColor(GUI_ORANGE);
  GUI_AA_DrawArc(AA_FACTOR * x0, AA_FACTOR * y0, AA_FACTOR * 39, AA_FACTOR * 39, 0, 360);  
  GUI_SetColor(GUI_ORANGE);
  GUI_AA_DrawArc(AA_FACTOR * x0, AA_FACTOR * y0, AA_FACTOR * 40, AA_FACTOR * 40, 0, 360);  
  GUI_SetColor(GUI_ORANGE);
  GUI_AA_DrawArc(AA_FACTOR * x0, AA_FACTOR * y0, AA_FACTOR * 41, AA_FACTOR * 41, 0, 360);
  GUI_SetColor(GUI_WHITE);  
  GUI_AA_FillCircle(AA_FACTOR * x0, AA_FACTOR * y0, AA_FACTOR * 38); 
  GUI_SetBkColor(GUI_TRANSPARENT);

  // Dial numbers
  GUI_SetBkColor(GUI_WHITE);
  GUI_SetColor(GUI_GRAY);  
  GUI_DispStringAt("12", x0 - 10,	y0 - 35);
  GUI_DispStringAt( "6", x0 -  5,	y0 + 18);
  GUI_DispStringAt( "9", x0 - 33,	y0 -  9);
  GUI_DispStringAt( "3", x0 + 23,	y0 -  9);
  
  // Dots next to numbers
  for (i = 0; i <= 12; i++) 
  {
    a = i * 30000;
    SinHQ = GUI__SinHQ(a);
    CosHQ = GUI__CosHQ(a);

    xPos = x0 + ((36 * CosHQ) >> 16);
    yPos = y0 - ((36 * SinHQ) >> 16);
    
    GUI_AA_FillCircle(AA_FACTOR * xPos, AA_FACTOR * yPos, AA_FACTOR * 1); 
  }
  
  GUI_MagnifyPolygon(aPointsDest[0], aPoints[0], 4, 4);
  GUI_RotatePolygon(aPointsDest[0], aPointsDest[0], 4, - 2 * PI * (float)((float)hour + (float)min /60) / 12);    
  DrawNeedle(0, x0, y0);
  
  GUI_MagnifyPolygon(aPointsDest[1], aPoints[1], 4, 4);
  GUI_RotatePolygon(aPointsDest[1], aPointsDest[1], 4, - 2 * PI * (float)((float)min + (float)sec / 60) / 60);
  DrawNeedle(1, x0, y0);  
  
  GUI_MagnifyPolygon(aPointsDest[2], aPoints[2], 4, 1);
  GUI_RotatePolygon(aPointsDest[2], aPointsDest[2], 4, - 2 * PI * sec / 60);    
  DrawNeedle(2, x0, y0); 

  GUI_AA_DisableHiRes();
}

static void _cbDialog(WM_MESSAGE * pMsg)
{
	WM_HWIN hItem;
	int Id, NCode;

	RTC_DateTypeDef          Date;
	RTC_TimeTypeDef          Time;
	hItem = pMsg->hWin;
	switch (pMsg->MsgId)
	{
		case WM_TIMER:
		{
			WM_InvalidateWindow(pMsg->hWin);
			WM_RestartTimer(pMsg->Data.v, 1000);
			break;
		}
    
		case WM_DELETE:
		{
			WM_DeleteTimer(hTimerTime);
			DisableAutoRefresh = 0;
			break;
		}
    
		case WM_INIT_DIALOG:
		{
			k_GetTime(&Time);
			k_GetDate(&Date);
    
			CALENDAR_Create(pMsg->hWin, CALENDAR_X, CALENDAR_Y, (2000 + Date.Year), Date.Month, Date.Date, 2, ID_CALENDAR, WM_CF_SHOW);

			// Header
			hItem = WM_GetDialogItem(pMsg->hWin, ID_TEXT_SPIN_0);
			TEXT_SetFont(hItem,&GUI_Font20_1);
			TEXT_SetBkColor(hItem,GUI_LIGHTBLUE);
			TEXT_SetTextColor(hItem,GUI_WHITE);
			TEXT_SetTextAlign(hItem,TEXT_CF_HCENTER|TEXT_CF_VCENTER);
			TEXT_SetText(hItem,"HOUR              MIN                SEC");

			hItem = WM_GetDialogItem(pMsg->hWin, ID_SPINBOX_HOUR);
			SPINBOX_SetFont(hItem,&GUI_Font24_1);
			SPINBOX_SetRange(hItem, 0, 23);
      
			hItem = WM_GetDialogItem(pMsg->hWin, ID_SPINBOX_MINUTE);
			SPINBOX_SetFont(hItem,&GUI_Font24_1);
			SPINBOX_SetRange(hItem, 0, 59);
      
			hItem = WM_GetDialogItem(pMsg->hWin, ID_SPINBOX_SEC);
			SPINBOX_SetFont(hItem,&GUI_Font24_1);
			SPINBOX_SetRange(hItem, 0, 59);
      
			hTimerTime = WM_CreateTimer(pMsg->hWin, 0, 1000, 0);
			break;
		}

		case WM_PAINT:
		{
			//
			// ToDo: fix this implementation, repaint is just too much...
			//
			if(DisableAutoRefresh == 0)
			{
				k_GetTime(&Time);
				k_GetDate(&Date);
				//HAL_RTC_GetTime(&RtcHandle, &Time, RTC_FORMAT_BIN);
				//HAL_RTC_GetDate(&RtcHandle, &Date, RTC_FORMAT_BIN);

				//printf("upd0 %d\r\n",Time.Seconds);

				GUI_UpdateClock(AN_CLOCK_X, AN_CLOCK_Y, Time.Hours, Time.Minutes, Time.Seconds);
      
				hItem = WM_GetDialogItem(pMsg->hWin, ID_SPINBOX_HOUR);
				SPINBOX_SetValue(hItem, Time.Hours);
      
				hItem = WM_GetDialogItem(pMsg->hWin, ID_SPINBOX_MINUTE);
				SPINBOX_SetValue(hItem, Time.Minutes);
      
				hItem = WM_GetDialogItem(pMsg->hWin, ID_SPINBOX_SEC);
				SPINBOX_SetValue(hItem, Time.Seconds);

				//hItem = WM_GetDialogItem(pMsg->hWin, ID_CALENDAR);
				//WM_InvalidateWindow(hItem);
			}
			else
			{
				//printf("upd1 %d\r\n",Time.Seconds);

				hItem = WM_GetDialogItem(pMsg->hWin, ID_SPINBOX_HOUR);
				Time.Hours = SPINBOX_GetValue(hItem);
      
				hItem = WM_GetDialogItem(pMsg->hWin, ID_SPINBOX_MINUTE);
				Time.Minutes = SPINBOX_GetValue(hItem);
      
				hItem = WM_GetDialogItem(pMsg->hWin, ID_SPINBOX_SEC);
				Time.Seconds = SPINBOX_GetValue(hItem);
      
				GUI_UpdateClock(AN_CLOCK_X, AN_CLOCK_Y, Time.Hours, Time.Minutes, Time.Seconds);
			}
			break;
		}

		case WM_NOTIFY_PARENT:
		{
			Id    = WM_GetId(pMsg->hWinSrc);    // Id of widget
			NCode = pMsg->Data.v;               // Notification code

			switch (NCode)
			{
				case WM_NOTIFICATION_CLICKED:      /* React only if released */

					switch (Id)
					{
						case ID_SPINBOX_HOUR:
						case ID_SPINBOX_MINUTE:
						case ID_SPINBOX_SEC:
							if(DisableAutoRefresh == 0)
							{
								DisableAutoRefresh = 1;
							}
							break;
					}
					break;
      
				case WM_NOTIFICATION_RELEASED:

					switch (Id)
					{
						case ID_BUTTON_APPLY:
						{
							hItem = WM_GetDialogItem(pMsg->hWin, ID_SPINBOX_SEC);
							Time.Seconds = SPINBOX_GetValue(hItem);
        
							hItem = WM_GetDialogItem(pMsg->hWin, ID_SPINBOX_MINUTE);
							Time.Minutes = SPINBOX_GetValue(hItem);
        
							hItem = WM_GetDialogItem(pMsg->hWin, ID_SPINBOX_HOUR);
							Time.Hours = SPINBOX_GetValue(hItem);

							hItem = WM_GetDialogItem(pMsg->hWin, ID_CALENDAR);
							CALENDAR_GetSel (hItem, &hDate);
							CALENDAR_SetSel(hItem, &hDate);
							WM_InvalidateWindow(hItem);
        
							//printf("year sel: %d\r\n", hDate.Year);

							if(	(hDate.Day > 0) &&
								(hDate.Day <= 31) &&
								(hDate.Month > 0) &&
								(hDate.Month <= 12) &&
								(hDate.Year >= 2000))
							{
								Date.Date  = hDate.Day;
								Date.Month = hDate.Month;
								Date.Year  = hDate.Year - 2000;
								//Date.WeekDay = 0;
								//printf("year set: %d\r\n", Date.Year);

								k_SetDate(&Date);
								k_SetTime(&Time);
							}
							DisableAutoRefresh = 0;
							break;
						}

						//case ID_BUTTON_EXIT:
						//	GUI_EndDialog(pMsg->hWin, 0);
						//	break;
					}
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

	hCdialog = GUI_CreateDialogBox(p_widget, GUI_COUNTOF(_aDialog), _cbDialog, hWin, xpos, ypos);

	free(p_widget);
	return;

use_const_decl:
	hCdialog = GUI_CreateDialogBox(_aDialog, GUI_COUNTOF(_aDialog), _cbDialog, hWin, xpos, ypos);
}

static void KillClock(void)
{
	//printf("kill menu\r\n");
	GUI_EndDialog(hCdialog, 0);
}

#endif

