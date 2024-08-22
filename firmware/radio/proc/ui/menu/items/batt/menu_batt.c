/************************************************************************************
**                                                                                 **
**                             mcHF Pro QRP Transceiver                            **
**                         Krassi Atanassov - M0NKA, 2013-2021                     **
**                                                                                 **
**---------------------------------------------------------------------------------**
**                                                                                 **
**  File name:                                                                     **
**  Description:                                                                   **
**  Last Modified:                                                                 **
**  Licence:       The mcHF project is released for radio amateurs experimentation **
**               and non-commercial use only.Check 3rd party drivers for licensing **
************************************************************************************/
#include "main.h"
#include "mchf_pro_board.h"

#ifdef CONTEXT_VIDEO

#include "ui_menu_layout.h"
#include "gui.h"
#include "dialog.h"

#include "ui_menu_module.h"
#include "bms_proc.h"

#include "menu_batt.h"

extern GUI_CONST_STORAGE GUI_BITMAP bmicon_power;
  
// UI driver public state
extern struct	UI_DRIVER_STATE			ui_s;
extern 			TaskHandle_t 			hUiTask;
extern struct 	BMSState				bmss;

// Menu layout definitions from Flash
extern const struct UIMenuLayout menu_layout[];

static void Startup(WM_HWIN hWin, uint16_t xpos, uint16_t ypos);
static void KillBatt(void);

K_ModuleItem_Typedef  menu_batt =
{
  2,
  "Battery Manager",
  &bmicon_power,
  Startup,
  NULL,
  KillBatt
};

//#define ID_BUTTON_DSP	          	(GUI_ID_USER + 0x51)
#define ID_BUTTON_SPLIT          	(GUI_ID_USER + 0x52)
#define ID_PROGBAR_0 				(GUI_ID_USER + 0x53)
#define ID_HEADER_0      			(GUI_ID_USER + 0x58)

#define ID_BUTTON_UI_RESET		  	(GUI_ID_USER + 0x02)
#define ID_BUTTON_DSP_RESET		  	(GUI_ID_USER + 0x03)
#define ID_BUTTON_EEP_RESET		  	(GUI_ID_USER + 0x04)

static const GUI_WIDGET_CREATE_INFO _aDialog[] = 
{
	// -----------------------------------------------------------------------------------------------------------------------------
	//							name					id						x		y		xsize	ysize	?		?		?
	// -----------------------------------------------------------------------------------------------------------------------------
	// Self
	{ WINDOW_CreateIndirect,	"", 					ID_WINDOW_0,			0,    	0,		800,	430, 	0, 		0x64, 	0 },
};

static const GUI_WIDGET_CREATE_INFO _aDialogCreate1[] =
{
	// -----------------------------------------------------------------------------------------------------------------------------
	//							name					id						x		y		xsize	ysize	?		?		?
	// -----------------------------------------------------------------------------------------------------------------------------
	// Self
 	{ WINDOW_CreateIndirect,   	"", 		0,              	0,   	0, 		790, 	400, 		FRAMEWIN_CF_MOVEABLE 		  },
	//
	// Balancer state header
	{ HEADER_CreateIndirect, 	"", 		ID_HEADER_0, 		10, 	10, 	565, 			25, 				0, 		0x0, 	0 },

	// Battery cells as progress bars
	{ PROGBAR_CreateIndirect, 	"", 		ID_PROGBAR_0, 		10, 	45, 	125, 			65, 				0, 		0x0, 	0 },
	{ PROGBAR_CreateIndirect, 	"", 		ID_PROGBAR_0 + 1, 	155, 	45, 	125, 			65, 				0, 		0x0, 	0 },
	{ PROGBAR_CreateIndirect, 	"", 		ID_PROGBAR_0 + 2, 	305, 	45, 	125, 			65, 				0, 		0x0, 	0 },
	{ PROGBAR_CreateIndirect, 	"", 		ID_PROGBAR_0 + 3, 	450, 	45, 	125, 			65, 				0, 		0x0, 	0 },

	// Cell descriptions
	{ TEXT_CreateIndirect, 		"",			GUI_ID_TEXT0,		10,		110,	125, 			30,  				0, 		0x0,	0 },
	{ TEXT_CreateIndirect, 		"",			GUI_ID_TEXT1,		155,	110,	125, 			30,  				0, 		0x0,	0 },
	{ TEXT_CreateIndirect, 		"",			GUI_ID_TEXT2,		305,	110,	125, 			30,  				0, 		0x0,	0 },
	{ TEXT_CreateIndirect, 		"",			GUI_ID_TEXT3,		450,	110,	125, 			30,  				0, 		0x0,	0 }
};

static const GUI_WIDGET_CREATE_INFO _aDialogCreate2[] =
{
	// -----------------------------------------------------------------------------------------------------------------------------
	//							name		id					x		y		xsize	ysize		?		?		?
	// -----------------------------------------------------------------------------------------------------------------------------
	// Self
   	{ WINDOW_CreateIndirect,   	"", 		0,              	0,   	0, 		790, 	430, 		FRAMEWIN_CF_MOVEABLE 		  },
	//
	{ LISTVIEW_CreateIndirect, 	"", 		ID_LISTVIEW, 		5, 		5, 		780, 	300, 		0, 			0, 				0 },
	//
	{ SLIDER_CreateIndirect, 	"", 		GUI_ID_SLIDER0, 	5, 		370,  	780, 	40, 		SOPTS, 		0, 				0 },
	// Radio box
	{ RADIO_CreateIndirect, 	"", 		ID_RADIO_0, 		5, 		315, 	140, 	40, 		0, 			0x1002,			0 },
	{ RADIO_CreateIndirect, 	"", 		ID_RADIO_1, 		160, 	315, 	140, 	40, 		0, 			0x1002,			0 },
};

static const GUI_WIDGET_CREATE_INFO _aDialogCreate3[] =
{
	// -----------------------------------------------------------------------------------------------------------------------------
	//							name					id						x		y		xsize	ysize	?		?		?
	// -----------------------------------------------------------------------------------------------------------------------------
	// Self
   	{ WINDOW_CreateIndirect,   	"", 		0,              	0,   	0, 		790, 	400, 		FRAMEWIN_CF_MOVEABLE 		  },
	//
	{ BUTTON_CreateIndirect, 	"Restart UI",	 		ID_BUTTON_UI_RESET,		40, 	40, 	120, 	45, 	0, 		0x0, 	0 },
	{ BUTTON_CreateIndirect, 	"Power OFF",		 	ID_BUTTON_DSP_RESET,	40, 	120, 	120, 	45, 	0, 		0x0, 	0 },
	{ BUTTON_CreateIndirect, 	"Kill Backup",	 		ID_BUTTON_EEP_RESET,	40, 	200, 	120, 	45, 	0, 		0x0, 	0 },
};

// Default ListView values
static const char * _aTable_1[BATT_MAX_ROW][BATT_MAX_COLUMN + 1] = {

		// ch 	  	description   		input 	adj		calc	trunc	cal1	cal2
		{ "ch3",  	"Cell1", 			"",  	"",		"", 	"", 	"",		"",  0 },
		{ "ch4",  	"Cell2 (1+2)", 		"", 	"", 	"", 	"", 	"",		"",  0 },
		{ "ch5",  	"Cell3 (1+2+3)",	"", 	"", 	"", 	"", 	"", 	"",  0 },
		{ "ch7",  	"Cell4 (1+2+3+4)",	"", 	"",	 	"", 	"", 	"", 	"",  0 },
		{ "ch8",  	"Battery load",		"", 	"", 	"", 	"", 	"", 	"",  0 },
		{ "ch9",  	"DC input",			"", 	"", 	"", 	"", 	"", 	"",  0 },
		{ "",   	"Battery current",	"", 	"", 	"", 	"", 	"", 	"",  0 },
		{ "ch11", 	"Cell1 temp",		"", 	"", 	"", 	"", 	"", 	"",  0 },
		{ "ch14", 	"Cell2 temp", 		"", 	"", 	"", 	"", 	"", 	"",  0 },
		{ "ch15", 	"Cell3 temp", 		"", 	"", 	"", 	"", 	"", 	"",  0 },
		{ "ch16", 	"Cell4 temp",	 	"", 	"", 	"",	 	"", 	"", 	"",  0 }

};

WM_HWIN   			hBdialog;
LISTWHEEL_Handle 	hMulti;
WM_HTIMER 			hTimerBatt;
WM_HTIMER 			hTimerBattA;

static void UpdateMonitorFrame(WM_HWIN hDlg)
{
	#ifdef CONTEXT_BMS
	int i;
	char buf[40];

	if(!bmss.rr)
		return;

	//printf("ui update\r\n");

	#if 0
	WM_HWIN hEdit;
	for(i = 0; i < 16; i++)
	{
		hEdit = WM_GetDialogItem(hDlg, GUI_ID_EDIT0 + i);

		//sprintf(buf, "%d.%dV", bmss.a[i]/1000, (bmss.a[i]%1000)/10);
		if(i < 4)
			sprintf(buf, "%d", bmss.a[i]);
		else if(i < 8)
			sprintf(buf, "%d", bmss.s[i - 4]);
		else if(i < 12)
			sprintf(buf, "%d", bmss.c[i - 8]);
		else
			sprintf(buf, "%d.%02dV", bmss.c[i - 12]/1000, (bmss.c[i - 12]%1000)/10);

		EDIT_SetText(hEdit, buf);
	}

	// Charger voltage
	hEdit = WM_GetDialogItem(hDlg, GUI_ID_EDIT0 + i);
	sprintf(buf, "%d", bmss.chgr);
	EDIT_SetText(hEdit, buf);
	i++;

	// Load voltage
	hEdit = WM_GetDialogItem(hDlg, GUI_ID_EDIT0 + i);
	sprintf(buf, "%d", bmss.load);
	EDIT_SetText(hEdit, buf);
	i++;

	// Current draw
	hEdit = WM_GetDialogItem(hDlg, GUI_ID_EDIT0 + i);
	sprintf(buf, "%d", bmss.curr);
	EDIT_SetText(hEdit, buf);
	i++;

	total_error += bmss.t_err;

	// Accum measurement error
	hEdit = WM_GetDialogItem(hDlg, GUI_ID_EDIT0 + i);
	sprintf(buf, "%d", total_error);
	EDIT_SetText(hEdit, buf);
	i++;

	// Temperature
	for(i = 0; i < 4; i++)
	{
		hEdit = WM_GetDialogItem(hDlg, GUI_ID_TEXT0 + i + 20);

		sprintf(buf, "%2d.%02dC", bmss.t[i]/100, bmss.t[i]%100);
		TEXT_SetText(hEdit, buf);
	}
	#else
	WM_HWIN hItem, hHeader;
	ulong perc_val;

	hHeader = WM_GetDialogItem(hDlg, ID_HEADER_0);
	HEADER_SetItemText(hHeader, 0, "CELL1");
	HEADER_SetItemText(hHeader, 2, "CELL2");
	HEADER_SetItemText(hHeader, 4, "CELL3");
	HEADER_SetItemText(hHeader, 6, "CELL4");

	if(bmss.run_on_dc == 0)
		HEADER_SetTextColor(hHeader, GUI_BLUE);
	else
		HEADER_SetTextColor(hHeader, GUI_MAGENTA);

	for(i = 0; i < 4; i++)
	{
		hItem = WM_GetDialogItem(hDlg, ID_PROGBAR_0 + i);

		perc_val = 0;

		// 2600 -  4200, per cell safe range
		// range 1600 mV, 1% is 16 mV
		if(bmss.c[i] > 2600)
		{
			perc_val  = bmss.c[i] - 2600;	// minus base level
			perc_val /= 16;					// to %
		}
		//printf("%d: %d, %d\r\n", i, bmss.c[i], perc_val);
		PROGBAR_SetValue(hItem, perc_val);

		hItem = WM_GetDialogItem(hDlg, GUI_ID_TEXT0 + i);
		sprintf(buf, "%d.%02dV - %2d.%02dC", (int)(bmss.c[i]/1000), (int)((bmss.c[i]%1000)/10), (int)(bmss.t[i]/100), (int)(bmss.t[i]%100));
		TEXT_SetText(hItem, buf);
		TEXT_SetTextColor(hItem, GUI_DARKBLUE);

		// Show balancer state
		if((bmss.run_on_dc == 0)&&(bmss.usBalID[i] == 1))
		{
			TEXT_SetTextColor(hItem, GUI_LIGHTRED);
			HEADER_SetItemText(hHeader, (i*2), "LOADED");
		}
	}
	#endif

	// Clear update flag
	bmss.rr = 0;

	#endif
}

static void UpdateCalibrationFrame(WM_HWIN hDlg)
{
	#ifdef CONTEXT_BMS
	WM_HWIN hItem;
	char buf[30];
	int i, j;

	if(!bmss.rr)
		return;

	//printf("ui update\r\n");

	hItem = WM_GetDialogItem(hDlg, ID_LISTVIEW);

	// ------------------------------------------------------------------------------------
	// CELLs voltage
	#if 0
	for(i = 0; i < 16; i++)
	{
		if(i < 4)
		{
			if(bmss.a[i] != 0)
			{
				sprintf(buf, "%d", (int)bmss.a[i]);
				LISTVIEW_SetItemText(hItem, COLUMN_INPUT, i, buf);
			}
		}
		else if(i < 8)
		{
			if(bmss.a[i - 4] != 0)
			{
				sprintf(buf, "%d", (int)bmss.s[i - 4]);
				LISTVIEW_SetItemText(hItem, COLUMN_ADJ, (i - 4), buf);
			}
		}
		else if(i < 12)
		{
			if(bmss.a[i - 8] != 0)
			{
				sprintf(buf, "%dmV", (int)bmss.c[i - 8]);
				LISTVIEW_SetItemText(hItem, COLUMN_CALC, (i - 8), buf);
			}
		}
		else
		{
			if(bmss.a[i - 12] != 0)
			{
				sprintf(buf, "%d.%02dV", (int)bmss.c[i - 12]/1000, (int)(bmss.c[i - 12]%1000)/10);
				LISTVIEW_SetItemText(hItem, COLUMN_TRUNC, (i - 12), buf);
			}
		}
	}
	#else
	for(i = 0; i < 4; i++)
	{
		if(bmss.a[i] != 0)
		{
			sprintf(buf, "%d", (int)bmss.a[i]);
			LISTVIEW_SetItemText(hItem, COLUMN_INPUT, i, buf);
		}

		if(bmss.s[i] != 0)
		{
			sprintf(buf, "%d", (int)bmss.s[i]);
			LISTVIEW_SetItemText(hItem, COLUMN_ADJ, i, buf);
		}

		if(bmss.c[i] != 0)
		{
			sprintf(buf, "%dmV", (int)bmss.c[i]);
			LISTVIEW_SetItemText(hItem, COLUMN_CALC, i, buf);
			sprintf(buf, "%d.%02dV", (int)bmss.c[i]/1000, (int)(bmss.c[i]%1000)/10);
			LISTVIEW_SetItemText(hItem, COLUMN_TRUNC, i, buf);
		}
	}
	#endif

	// ------------------------------------------------------------------------------------
	// Calibration factors
	for(i = 0; i < 11; i++)
	{
		if(i < 6)
			j = i;
		else if(i == 6)	// current is calculated value, doesn't have calibration factor
			continue;
		else
			j = (i - 1);

		if(bmss.cal_adc[j] != 0)
		{
			sprintf(buf, "%d", (int)bmss.cal_adc[j]);
			LISTVIEW_SetItemText(hItem, COLUMN_CAL1, i, buf);
		}

		if(bmss.cal_res[j] != 0)
		{
			sprintf(buf, "%d", (int)bmss.cal_res[j]);
			LISTVIEW_SetItemText(hItem, COLUMN_CAL2, i, buf);
		}
	}

	// ------------------------------------------------------------------------------------
	// Load voltage
	if(bmss.a[5] != 0)
	{
		sprintf(buf, "%d", (int)bmss.a[5]);
		LISTVIEW_SetItemText(hItem, COLUMN_INPUT, ROW_BLOAD, buf);
	}

	if(bmss.load != 0)
	{
		sprintf(buf, "%d", (int)bmss.load);
		LISTVIEW_SetItemText(hItem, COLUMN_ADJ, ROW_BLOAD, buf);

		sprintf(buf, "%d.%02dV",(int)bmss.load/1000, (int)(bmss.load%1000)/10);
		LISTVIEW_SetItemText(hItem, COLUMN_TRUNC, ROW_BLOAD, buf);
	}

	// ------------------------------------------------------------------------------------
	// DC input voltage
	if(bmss.a[4] != 0)
	{
		sprintf(buf, "%d", (int)bmss.a[4]);
		LISTVIEW_SetItemText(hItem, COLUMN_INPUT, ROW_DCIN, buf);
	}

	if(bmss.chgr != 0)
	{
		sprintf(buf, "%d", (int)bmss.chgr);
		LISTVIEW_SetItemText(hItem, COLUMN_ADJ, ROW_DCIN, buf);

		sprintf(buf, "%d.%02dV", (int)bmss.chgr/1000, (int)(bmss.chgr%1000)/10);
		LISTVIEW_SetItemText(hItem, COLUMN_TRUNC, ROW_DCIN, buf);
	}

	// ------------------------------------------------------------------------------------
	// Current draw
	if(bmss.curr != 0)
	{
		sprintf(buf, "%dmA", (int)bmss.curr);
		LISTVIEW_SetItemText(hItem, COLUMN_CALC, ROW_BCURR, buf);
	}

	// ------------------------------------------------------------------------------------
	// Temperature
	for(i = 0; i < 4; i++)
	{
		sprintf(buf, "%d", (int)bmss.a[6 + i]);
		LISTVIEW_SetItemText(hItem, COLUMN_INPUT, (ROW_CELL1T + i), buf);
		sprintf(buf, "%2d.%1d C", (int)(bmss.t[i]/100), (int)(bmss.t[i]%100));	// "°C"
		LISTVIEW_SetItemText(hItem, COLUMN_CALC, (ROW_CELL1T + i), buf);
	}

	// Clear update flag
	bmss.rr = 0;

	#endif
}

static void _cbMonitorControl(WM_MESSAGE * pMsg, int Id, int NCode)
{
	//WM_HWIN hItem;

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
		default:
			break;
	}
}

static void _cbCalibrationControl(WM_MESSAGE * pMsg, int Id, int NCode)
{
	//WM_HWIN hItem;

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
		default:
			break;
	}
}

static void _cbSettingsControl(WM_MESSAGE * pMsg, int Id, int NCode)
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
					printf("...system reset\r\n");
					vTaskDelay(300);

					NVIC_SystemReset();
					break;
				}
			}
			break;
		}

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

		// -------------------------------------------------------------
		default:
			break;
	}
}


//
// Monitor Tab
//
static void _cbDialog1(WM_MESSAGE * pMsg)
{
#if 0
  char       acBuffer[32];
  int        NCode;
  int        Id;
  static int Time;

  switch (pMsg->MsgId) {
  case WM_PAINT:
    GUI_SetTextMode(GUI_TM_TRANS);
    GUI_SetColor(GUI_BLACK);
    GUI_SetFont(&GUI_Font13B_1);
    if (Time) {
      sprintf(acBuffer, "System time: %d", Time);
      GUI_DispStringAt(acBuffer, 5, 60);
    }
    break;
  case WM_NOTIFY_PARENT:
    NCode = pMsg->Data.v;
    Id = WM_GetId(pMsg->hWinSrc);
    if (NCode == WM_NOTIFICATION_RELEASED) {
      switch (Id) {
      case GUI_ID_BUTTON0:
        Time = GUI_GetTime();
        break;
      }
    }
    WM_InvalidateWindow(pMsg->hWin);
    break;
  default:
    WM_DefaultProc(pMsg);
  }
#else
	WM_HWIN 	hItem, hEdit;
	int 		Id, NCode;
	//GUI_RECT	Rect;
	WM_HWIN hDlg;

	hDlg = pMsg->hWin;

	switch (pMsg->MsgId)
	{
		case WM_INIT_DIALOG:
		{
			//char buf[10];

			//printf("init\r\n");

			//--bms_cntr_init();

			#ifdef KEYB_IS_TRANPARENT
			WINDOW_SetBkColor(pMsg->hWin, GUI_INVALID_COLOR);
			#else
			//WINDOW_SetBkColor(pMsg->hWin, GUI_LIGHTGRAY);
			#endif

			hTimerBattA = WM_CreateTimer(pMsg->hWin, 0, BTIMER_PERIOD, 0);

			// Initialisation of Balancer Header
			hItem = WM_GetDialogItem(pMsg->hWin, ID_HEADER_0);
			HEADER_AddItem(hItem, 125, "CELL1", 	14);
			HEADER_AddItem(hItem,  20, "", 			14);
			HEADER_AddItem(hItem, 125, "CELL2", 	14);
			HEADER_AddItem(hItem,  25, "", 			14);
			HEADER_AddItem(hItem, 125, "CELL3", 	14);
			HEADER_AddItem(hItem,  20, "", 			14);
			HEADER_AddItem(hItem, 125, "CELL4", 	14);

			#if 0
			for (int i = 0; i < 24; i++)
			{
				hItem = WM_GetDialogItem(pMsg->hWin, GUI_ID_TEXT0 + i);

				if(i < 20)
					TEXT_SetFont(hItem,&GUI_Font13B_1);
				else
					TEXT_SetFont(hItem,&GUI_Font20B_1);

				TEXT_SetBkColor(hItem,GUI_LIGHTBLUE);
				TEXT_SetTextColor(hItem,GUI_WHITE);
				TEXT_SetTextAlign(hItem,TEXT_CF_HCENTER|TEXT_CF_VCENTER);

				if(i < 20)
				{
					hEdit = WM_GetDialogItem(hDlg, GUI_ID_EDIT0 + i);
					EDIT_SetFont(hEdit,&GUI_Font20B_1);
					EDIT_SetBkColor(hEdit,EDIT_CI_ENABLED,GUI_LIGHTBLUE);
					EDIT_SetTextColor(hEdit,EDIT_CI_ENABLED,GUI_WHITE);
					EDIT_SetTextAlign(hEdit,TEXT_CF_HCENTER|TEXT_CF_VCENTER);
				}
			}
			#else
			for (int i = 0; i < 4; i++)
			{
				hItem = WM_GetDialogItem(pMsg->hWin, GUI_ID_TEXT0 + i);

				TEXT_SetFont(hItem,&GUI_Font16B_1);
				//TEXT_SetBkColor(hItem, GUI_WHITE);
				TEXT_SetTextColor(hItem, GUI_DARKBLUE);
				TEXT_SetTextAlign(hItem, TEXT_CF_HCENTER|TEXT_CF_VCENTER);
				//TEXT_SetText(hItem, "== test ==");
			}
			#endif
			UpdateMonitorFrame(hDlg);

			#if 0
			hItem = WM_GetDialogItem(pMsg->hWin, GUI_ID_SLIDER0);
			SLIDER_SetRange(hItem, 0, 200);
			SLIDER_SetValue(hItem, 100);
			SLIDER_SetNumTicks(hItem, 20);
			SLIDER_SetWidth(hItem, 15);
			#endif

			//hItem = WM_GetDialogItem(pMsg->hWin, ID_PROGBAR_0);
			//PROGBAR_SetValue(hItem, 50);
			//hItem = WM_GetDialogItem(pMsg->hWin, ID_PROGBAR_0 + 1);
			//PROGBAR_SetValue(hItem, 20);
			//hItem = WM_GetDialogItem(pMsg->hWin, ID_PROGBAR_0 + 2);
			//PROGBAR_SetValue(hItem, 80);
			//hItem = WM_GetDialogItem(pMsg->hWin, ID_PROGBAR_0 + 3);
			//PROGBAR_SetValue(hItem, 100);

			//hTimerWiFi = WM_CreateTimer(pMsg->hWin, 0, WIFI_TIMER_RESOLUTION, 0);
			break;
		}

		case WM_TIMER:
		{
			WM_InvalidateWindow(pMsg->hWin);
			WM_RestartTimer(pMsg->Data.v, BTIMER_PERIOD);
			break;
		}

		case WM_DELETE:
		{
			WM_DeleteTimer(hTimerBattA);
			break;
		}

		case WM_PAINT:
		{
			//WM_GetClientRect(&Rect);	// will create border when transparent
			//GUI_DrawRectEx(&Rect);

			// ToDo: Why multiple repaints ??
			//printf("paint\r\n");

			UpdateMonitorFrame(hDlg);
			break;
		}

		case WM_NOTIFY_PARENT:
		{
			Id    = WM_GetId(pMsg->hWinSrc);    /* Id of widget */
			NCode = pMsg->Data.v;               /* Notification code */

			_cbMonitorControl(pMsg,Id,NCode);
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
#endif
}

//
// Calibration Tab
//
static void _cbDialog2(WM_MESSAGE * pMsg)
{
	int 	Id, NCode;
	WM_HWIN hDlg;

	hDlg = pMsg->hWin;

	switch (pMsg->MsgId)
	{
		case WM_INIT_DIALOG:
		{
			HEADER_Handle hHeader;
			WM_HWIN 	  hItem;

			hTimerBatt = WM_CreateTimer(pMsg->hWin, 0, BTIMER_PERIOD, 0);

			// ListView
			hItem   = WM_GetDialogItem(pMsg->hWin, ID_LISTVIEW);
			hHeader = LISTVIEW_GetHeader(hItem);

			//--HEADER_SetBkColor  (hHeader, GUI_RED);
			//--HEADER_SetTextColor(hHeader, GUI_BLUE);
			HEADER_SetHeight   (hHeader, 20);

			LISTVIEW_AddColumn(hItem, 70,  	"adc ch", 		GUI_TA_LEFT    | GUI_TA_VCENTER);
			LISTVIEW_AddColumn(hItem, 180, 	"description", 	GUI_TA_LEFT    | GUI_TA_VCENTER);
			LISTVIEW_AddColumn(hItem, 80,  	"input mV", 	GUI_TA_HCENTER | GUI_TA_VCENTER);
			LISTVIEW_AddColumn(hItem, 110, 	"adjusted mV",	GUI_TA_HCENTER | GUI_TA_VCENTER);
			LISTVIEW_AddColumn(hItem, 100,	"calculated", 	GUI_TA_HCENTER | GUI_TA_VCENTER);
			LISTVIEW_AddColumn(hItem, 100,  "truncated",	GUI_TA_HCENTER | GUI_TA_VCENTER);
			LISTVIEW_AddColumn(hItem, 60,   "cal1",			GUI_TA_HCENTER | GUI_TA_VCENTER);
			LISTVIEW_AddColumn(hItem, 60,  	"cal2",			GUI_TA_HCENTER | GUI_TA_VCENTER);

			for(int i = 0; i < BATT_MAX_ROW; i++)
				LISTVIEW_AddRow(hItem,_aTable_1[i]);

			LISTVIEW_SetGridVis(hItem, 1);
			LISTVIEW_SetFont(hItem, &GUI_Font24B_1);

			hItem = WM_GetDialogItem(pMsg->hWin, GUI_ID_SLIDER0);
			SLIDER_SetRange(hItem, 0, 200);
			SLIDER_SetValue(hItem, 100);
			SLIDER_SetNumTicks(hItem, 20);
			SLIDER_SetWidth(hItem, 20);

			hItem = WM_GetDialogItem(pMsg->hWin, ID_RADIO_0);
			RADIO_SetFont(hItem,&GUI_Font20B_1);
			RADIO_SetSpacing(hItem, 23);
			RADIO_SetText(hItem, "Factory", 0);
			RADIO_SetText(hItem, "User", 	1);

			hItem = WM_GetDialogItem(pMsg->hWin, ID_RADIO_1);
			RADIO_SetFont(hItem,&GUI_Font20B_1);
			RADIO_SetSpacing(hItem, 23);
			RADIO_SetText(hItem, "cal1", 0);
			RADIO_SetText(hItem, "cal2", 1);

			break;
		}

		case WM_TIMER:
		{
			WM_InvalidateWindow(pMsg->hWin);
			WM_RestartTimer(pMsg->Data.v, BTIMER_PERIOD);
			break;
		}

		case WM_DELETE:
		{
			WM_DeleteTimer(hTimerBatt);
			break;
		}

		case WM_PAINT:
			UpdateCalibrationFrame(hDlg);
			break;

		case WM_NOTIFY_PARENT:
		{
			Id    = WM_GetId(pMsg->hWinSrc);    /* Id of widget */
			NCode = pMsg->Data.v;               /* Notification code */

			_cbCalibrationControl(pMsg,Id,NCode);(pMsg,Id,NCode);
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

//
// Settings Tab
//
static void _cbDialog3(WM_MESSAGE * pMsg)
{
	int 	Id, NCode;
	WM_HWIN hDlg;

	hDlg = pMsg->hWin;

	switch (pMsg->MsgId)
	{
		case WM_INIT_DIALOG:
		{

			break;
		}

		case WM_PAINT:
			//UpdateCalibrationFrame(hDlg);
			break;

		case WM_NOTIFY_PARENT:
		{
			Id    = WM_GetId(pMsg->hWinSrc);    /* Id of widget */
			NCode = pMsg->Data.v;               /* Notification code */

			_cbSettingsControl(pMsg,Id,NCode);
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

//
// Main menu window
//
static void _cbDialog(WM_MESSAGE * pMsg)
{
	switch (pMsg->MsgId)
	{
		case WM_INIT_DIALOG:
		{
			WM_HWIN hDialog;

		    // Create multipage widget
		    hMulti = MULTIPAGE_CreateEx(5, 40, 830, 430, WM_HBKWIN, WM_CF_SHOW, 0, GUI_ID_MULTIPAGE0);

		    // Create dialog windows to add them as pages
		    hDialog = GUI_CreateDialogBox(_aDialogCreate1, GUI_COUNTOF(_aDialogCreate1), _cbDialog1, WM_UNATTACHED, 0, 0);
		    MULTIPAGE_AddPage(hMulti, hDialog, "Monitor");

		    hDialog = GUI_CreateDialogBox(_aDialogCreate2, GUI_COUNTOF(_aDialogCreate2), _cbDialog2, WM_UNATTACHED, 0, 0);
		    MULTIPAGE_AddPage(hMulti, hDialog, "Calibration");

		    hDialog = GUI_CreateDialogBox(_aDialogCreate3, GUI_COUNTOF(_aDialogCreate3), _cbDialog3, WM_UNATTACHED, 0, 0);
		    MULTIPAGE_AddPage(hMulti, hDialog, "Settings");

		    // Set alignment of tabs
		    MULTIPAGE_SetRotation(hMulti, MULTIPAGE_CF_ROTATE_CW);
		    MULTIPAGE_SetAlign	 (hMulti, MULTIPAGE_ALIGN_RIGHT);
		    MULTIPAGE_SetFont	 (hMulti, &GUI_Font32B_ASCII);

		    // StartUp Tab
		    MULTIPAGE_SelectPage (hMulti, 0);

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

	hBdialog = GUI_CreateDialogBox(p_widget, GUI_COUNTOF(_aDialog), _cbDialog, hWin, xpos, ypos);

	free(p_widget);
	return;

use_const_decl:
	hBdialog = GUI_CreateDialogBox(_aDialog, GUI_COUNTOF(_aDialog), _cbDialog, hWin, xpos, ypos);
}

static void KillBatt(void)
{
	//printf("kill menu\r\n");
	GUI_EndDialog(hMulti,   0);
	GUI_EndDialog(hBdialog, 0);
}

#endif
