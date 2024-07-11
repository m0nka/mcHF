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
#include "mchf_pro_board.h"
#include "main.h"

//#ifdef CONTEXT_VIDEO && CONTEXT_BMS

#if defined(CONTEXT_VIDEO) && defined(CONTEXT_BMS)

#include "on_screen_power.h"
#include "ui_actions.h"

#include "radio_init.h"
#include "bms_proc.h"

//#define KEYB_IS_TRANPARENT

#define ID_WINDOW_KEYB          	(GUI_ID_USER + 0x50)

//#define ID_BUTTON_DSP	          	(GUI_ID_USER + 0x51)
#define ID_BUTTON_SPLIT          	(GUI_ID_USER + 0x52)
#define ID_PROGBAR_0 				(GUI_ID_USER + 0x53)
#define ID_HEADER_0      			(GUI_ID_USER + 0x58)

#define PWR_X						130
#define PWR_Y						203

#define PWR_SIZE_X					595
#define PWR_SIZE_Y					250

#ifdef KEYB_IS_TRANPARENT
#define K_CF						WM_CF_SHOW|WM_CF_MEMDEV|WM_CF_HASTRANS
#else
#define K_CF						0x64
#endif

//#define SOPTS						SLIDER_CF_VERTICAL

//#define TXT10X						50
//#define TXT10Y						20

//#define TXT10SX						40
//#define TXT10SY						30

static const GUI_WIDGET_CREATE_INFO PowerDialog[] =
{
	// -----------------------------------------------------------------------------------------------------------------------------
	//							name		id					x		y		xsize			ysize				?		?		?
	// -----------------------------------------------------------------------------------------------------------------------------
	// Self
	{ WINDOW_CreateIndirect,	"", 		ID_WINDOW_KEYB,		0,		0,		PWR_SIZE_X,		PWR_SIZE_Y, 		0, 		K_CF, 	0 },

	// Buttons
	//{ BUTTON_CreateIndirect, 	"POWER OFF",ID_BUTTON_DSP,		260, 	200, 	140, 			40, 				0, 		0x0, 	0 },
	{ BUTTON_CreateIndirect, 	"QUIT",		ID_BUTTON_SPLIT,	440, 	200, 	140,	 		40,  				0, 		0x0, 	0 },

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
	{ TEXT_CreateIndirect, 		"",			GUI_ID_TEXT3,		450,	110,	125, 			30,  				0, 		0x0,	0 },

	// Text descriptions
	//{ TEXT_CreateIndirect, 		"adc1",		GUI_ID_TEXT0,		10,		TXT10Y +   0,	30, 		30,  		0, 		0x0,	0 },
	//{ TEXT_CreateIndirect, 		"adc2",		GUI_ID_TEXT1,		10,		TXT10Y +  40,	30, 		30,  		0, 		0x0,	0 },
	//{ TEXT_CreateIndirect, 		"adc3",		GUI_ID_TEXT2,		10,		TXT10Y +  80,	30, 		30,  		0, 		0x0,	0 },
	//{ TEXT_CreateIndirect, 		"adc4",		GUI_ID_TEXT3,		10,		TXT10Y + 120,	30, 		30,  		0, 		0x0,	0 },
	//{ TEXT_CreateIndirect, 		"sm1",		GUI_ID_TEXT4,		110,	TXT10Y +   0,	30, 		30,  		0, 		0x0,	0 },
	//{ TEXT_CreateIndirect, 		"sm2",		GUI_ID_TEXT5,		110,	TXT10Y +  40,	30, 		30,  		0, 		0x0,	0 },
	//{ TEXT_CreateIndirect, 		"sm3",		GUI_ID_TEXT6,		110,	TXT10Y +  80,	30, 		30,  		0, 		0x0,	0 },
	//{ TEXT_CreateIndirect, 		"sm4",		GUI_ID_TEXT7,		110,	TXT10Y + 120,	30, 		30,  		0, 		0x0,	0 },
	//{ TEXT_CreateIndirect, 		"cell1",	GUI_ID_TEXT8,		210,	TXT10Y +   0,	30, 		30,  		0, 		0x0,	0 },
	//{ TEXT_CreateIndirect, 		"cell2",	GUI_ID_TEXT9,		210,	TXT10Y +  40,	30, 		30,  		0, 		0x0,	0 },
	//{ TEXT_CreateIndirect, 		"cell3",	GUI_ID_TEXT9 + 1,	210,	TXT10Y +  80,	30, 		30,  		0, 		0x0,	0 },
	//{ TEXT_CreateIndirect, 		"cell4",	GUI_ID_TEXT9 + 2,	210,	TXT10Y + 120,	30, 		30,  		0, 		0x0,	0 },
	//{ TEXT_CreateIndirect, 		"bat1",		GUI_ID_TEXT9 + 3,	310,	TXT10Y +   0,	30, 		30,  		0, 		0x0,	0 },
	//{ TEXT_CreateIndirect, 		"bat2",		GUI_ID_TEXT9 + 4,	310,	TXT10Y +  40,	30, 		30,  		0, 		0x0,	0 },
	//{ TEXT_CreateIndirect, 		"bat3",		GUI_ID_TEXT9 + 5,	310,	TXT10Y +  80,	30, 		30,  		0, 		0x0,	0 },
	//{ TEXT_CreateIndirect, 		"bat4",		GUI_ID_TEXT9 + 6,	310,	TXT10Y + 120,	30, 		30,  		0, 		0x0,	0 },
	//{ TEXT_CreateIndirect, 		"chgr",		GUI_ID_TEXT9 + 7,	 10,	TXT10Y + 160,	30, 		30,  		0, 		0x0,	0 },
	//{ TEXT_CreateIndirect, 		"load",		GUI_ID_TEXT9 + 8,	110,	TXT10Y + 160,	30, 		30,  		0, 		0x0,	0 },
	//{ TEXT_CreateIndirect, 		"curr",		GUI_ID_TEXT9 + 9,	210,	TXT10Y + 160,	30, 		30,  		0, 		0x0,	0 },
	//{ TEXT_CreateIndirect, 		"err",		GUI_ID_TEXT9 + 10,	310,	TXT10Y + 160,	30, 		30,  		0, 		0x0,	0 },

	// Value boxes
	//{ EDIT_CreateIndirect, 		"Edit0",	GUI_ID_EDIT0,		42,	 	TXT10Y +   0,	60,			30,  		0, 		0x0,	0 },
	//{ EDIT_CreateIndirect, 		"Edit1",	GUI_ID_EDIT1,		42,	 	TXT10Y +  40,	60, 		30,  		0, 		0x0,	0 },
	//{ EDIT_CreateIndirect, 		"Edit2",	GUI_ID_EDIT2,		42,	 	TXT10Y	+  80,	60, 		30,  		0, 		0x0,	0 },
	//{ EDIT_CreateIndirect, 		"Edit3",	GUI_ID_EDIT3,		42,	 	TXT10Y + 120,	60, 		30,  		0, 		0x0,	0 },
	//{ EDIT_CreateIndirect, 		"Edit4",	GUI_ID_EDIT4,		142,	TXT10Y +   0,	60,			30,  		0, 		0x0,	0 },
	//{ EDIT_CreateIndirect, 		"Edit5",	GUI_ID_EDIT5,		142,	TXT10Y +  40,	60, 		30,  		0, 		0x0,	0 },
	//{ EDIT_CreateIndirect, 		"Edit6",	GUI_ID_EDIT6,		142,	TXT10Y	+  80,	60, 		30,  		0, 		0x0,	0 },
	//{ EDIT_CreateIndirect, 		"Edit7",	GUI_ID_EDIT7,		142,	TXT10Y + 120,	60, 		30,  		0, 		0x0,	0 },
	//{ EDIT_CreateIndirect, 		"Edit8",	GUI_ID_EDIT8,		242,	TXT10Y +   0,	60,			30,  		0, 		0x0,	0 },
	//{ EDIT_CreateIndirect, 		"Edit9",	GUI_ID_EDIT9,		242,	TXT10Y +  40,	60, 		30,  		0, 		0x0,	0 },
	//{ EDIT_CreateIndirect, 		"Edit9",	GUI_ID_EDIT9 + 1,	242,	TXT10Y	+  80,	60, 		30,  		0, 		0x0,	0 },
	//{ EDIT_CreateIndirect, 		"Edit9",	GUI_ID_EDIT9 + 2,	242,	TXT10Y + 120,	60, 		30,  		0, 		0x0,	0 },
	//{ EDIT_CreateIndirect, 		"Edit8",	GUI_ID_EDIT9 + 3,	342,	TXT10Y +   0,	60,			30,  		0, 		0x0,	0 },
	//{ EDIT_CreateIndirect, 		"Edit9",	GUI_ID_EDIT9 + 4,	342,	TXT10Y +  40,	60, 		30,  		0, 		0x0,	0 },
	//{ EDIT_CreateIndirect, 		"Edit9",	GUI_ID_EDIT9 + 5,	342,	TXT10Y	+  80,	60, 		30,  		0, 		0x0,	0 },
	//{ EDIT_CreateIndirect, 		"Edit9",	GUI_ID_EDIT9 + 6,	342,	TXT10Y + 120,	60, 		30,  		0, 		0x0,	0 },
	//{ EDIT_CreateIndirect, 		"Edit8",	GUI_ID_EDIT9 + 7,	 42,	TXT10Y + 160,	60,			30,  		0, 		0x0,	0 },
	//{ EDIT_CreateIndirect, 		"Edit9",	GUI_ID_EDIT9 + 8,	142,	TXT10Y + 160,	60, 		30,  		0, 		0x0,	0 },
	//{ EDIT_CreateIndirect, 		"Edit9",	GUI_ID_EDIT9 + 9,	242,	TXT10Y	+ 160,	60, 		30,  		0, 		0x0,	0 },
	//{ EDIT_CreateIndirect, 		"Edit9",	GUI_ID_EDIT9 + 10,	342,	TXT10Y + 160,	60, 		30,  		0, 		0x0,	0 },

	// Temperature text
	//{ TEXT_CreateIndirect, 		"--",		GUI_ID_TEXT9 + 11,	410,	TXT10Y +   0,	60, 		30,  		0, 		0x0,	0 },
	//{ TEXT_CreateIndirect, 		"--",		GUI_ID_TEXT9 + 12,	410,	TXT10Y +  40,	60, 		30,  		0, 		0x0,	0 },
	//{ TEXT_CreateIndirect, 		"--",		GUI_ID_TEXT9 + 13,	410,	TXT10Y +  80,	60, 		30,  		0, 		0x0,	0 },
	//{ TEXT_CreateIndirect, 		"--",		GUI_ID_TEXT9 + 14,	410,	TXT10Y + 120,	60, 		30,  		0, 		0x0,	0 },

	//{ BUTTON_CreateIndirect, 	"BAL ON",	GUI_ID_BUTTON0,		20, 	215, 			80, 		30, 		0, 		0x0, 	0 },
	//{ BUTTON_CreateIndirect, 	"BAL A",	GUI_ID_BUTTON1,		120, 	215, 			80, 		30, 		0, 		0x0, 	0 },
	//{ BUTTON_CreateIndirect, 	"BAL B",	GUI_ID_BUTTON2,		220, 	215, 			80, 		30, 		0, 		0x0, 	0 },
	//{ BUTTON_CreateIndirect, 	"BAL C",	GUI_ID_BUTTON3,		320, 	215, 			80, 		30, 		0, 		0x0, 	0 },

	//{ SLIDER_CreateIndirect, 	"Cal", 		GUI_ID_SLIDER0, 	480, 	20,  			40, 		220, 		SOPTS, 	0, 		0 },
	//{ TEXT_CreateIndirect, 		"--",		GUI_ID_TEXT9 + 15,	540,	125,			60, 		30,  		0, 		0x0,	0 },
};

extern struct BMSState	bmss;

WM_HWIN 	hPowerDialog = 0;

//uchar bal_state[4];

#if 0
static void bms_cntr_init(void)
{
	GPIO_InitTypeDef  gpio_init_structure;

	gpio_init_structure.Mode  = GPIO_MODE_OUTPUT_PP;
	gpio_init_structure.Pull  = GPIO_NOPULL;
	gpio_init_structure.Speed = GPIO_SPEED_FREQ_LOW;

	// BAL1_ON is PG6
	gpio_init_structure.Pin   = BAL1_ON;
	HAL_GPIO_Init(BAL13_PORT, &gpio_init_structure);

	// BAL2_ON is PG3
	gpio_init_structure.Pin   = BAL2_ON;
	HAL_GPIO_Init(BAL13_PORT, &gpio_init_structure);

	// BAL3_ON is PG2
	gpio_init_structure.Pin   = BAL3_ON;
	HAL_GPIO_Init(BAL13_PORT, &gpio_init_structure);

	// BAL4_ON is PD7
	gpio_init_structure.Pin   = BAL4_ON;
	HAL_GPIO_Init(BAL4_PORT, &gpio_init_structure);

	//for(int i = 0; i < 4; i++)
	//	bal_state[i] = 0;
}
#endif

static void PW_cbControl(WM_MESSAGE * pMsg, int Id, int NCode)
{
	WM_HWIN hItem;

	switch(Id)
	{
		#if 0
		case ID_BUTTON_DSP:
		{
			if(NCode != WM_NOTIFICATION_RELEASED)
				return;

			printf("...power off screen\r\n");
			vTaskDelay(300);

			power_off();
			break;
		}
		#endif

		case ID_BUTTON_SPLIT:
		{
			if(NCode != WM_NOTIFICATION_RELEASED)
				return;

			GUI_EndDialog(pMsg->hWin, 0);
			break;
		}

		#if 0
		case GUI_ID_BUTTON0:
		{
			if(NCode != WM_NOTIFICATION_RELEASED)
				return;

			if(bal_state[0])
			{
				printf("bal1 off\r\n");

				// BAL1_ON is PG6
				HAL_GPIO_WritePin(BAL13_PORT, BAL1_ON, GPIO_PIN_RESET);
			}
			else
			{
				printf("bal1 on\r\n");

				// BAL1_ON is PG6
				HAL_GPIO_WritePin(BAL13_PORT, BAL1_ON, GPIO_PIN_SET);
			}
			bal_state[0] = !bal_state[0];

			break;
		}

		case GUI_ID_BUTTON1:
		{
			if(NCode != WM_NOTIFICATION_RELEASED)
				return;

			if(bal_state[1])
			{
				printf("bal2 off\r\n");

				// BAL2_ON is PG3
				HAL_GPIO_WritePin(BAL13_PORT, BAL2_ON, GPIO_PIN_RESET);
			}
			else
			{
				printf("bal2 on\r\n");

				// BAL2_ON is PG3
				HAL_GPIO_WritePin(BAL13_PORT, BAL2_ON, GPIO_PIN_SET);
			}
			bal_state[1] = !bal_state[1];

			break;
		}

		case GUI_ID_BUTTON2:
		{
			if(NCode != WM_NOTIFICATION_RELEASED)
				return;

			if(bal_state[2])
			{
				printf("bal3 off\r\n");

				// BAL3_ON is PG2
				HAL_GPIO_WritePin(BAL13_PORT, BAL3_ON, GPIO_PIN_RESET);
			}
			else
			{
				printf("bal3 on\r\n");

				// BAL3_ON is PG2
				HAL_GPIO_WritePin(BAL13_PORT, BAL3_ON, GPIO_PIN_SET);
			}
			bal_state[2] = !bal_state[2];

			break;
		}

		case GUI_ID_BUTTON3:
		{
			if(NCode != WM_NOTIFICATION_RELEASED)
				return;

			if(bal_state[3])
			{
				printf("bal4 off\r\n");

				// BAL4_ON is PD7
				HAL_GPIO_WritePin(BAL4_PORT, BAL4_ON, GPIO_PIN_RESET);
			}
			else
			{
				printf("bal4 on\r\n");

				// BAL4_ON is PD7
				HAL_GPIO_WritePin(BAL4_PORT, BAL4_ON, GPIO_PIN_SET);
			}
			bal_state[3] = !bal_state[3];

			break;
		}

		case GUI_ID_SLIDER0:
		{
			char buf[10];

			if(NCode != WM_NOTIFICATION_VALUE_CHANGED)
				return;

			//printf("slider\r\n");

			hItem = WM_GetDialogItem(pMsg->hWin, GUI_ID_SLIDER0);
			int val = SLIDER_GetValue(hItem);

			val -= 100;
			val *= 3;

			// Calibrate
			//--bmss.cal[5] = val;

			sprintf(buf, "%d\r\n", val);

			hItem = WM_GetDialogItem(pMsg->hWin, GUI_ID_TEXT9 + 15);
			TEXT_SetText(hItem, buf);

		    break;
		}
		#endif

		default:
			break;
	}
}

ulong total_error = 0;

static void UpdateExitBox(WM_HWIN hDlg)
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
		sprintf(buf, "%d.%02dV - %2d.%02dC", bmss.c[i]/1000, (bmss.c[i]%1000)/10, bmss.t[i]/100, bmss.t[i]%100);
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

static void PowerHandler(WM_MESSAGE *pMsg)
{
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
			WINDOW_SetBkColor(pMsg->hWin, GUI_LIGHTGRAY);
			#endif

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
			UpdateExitBox(hDlg);

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
		{
			//WM_GetClientRect(&Rect);	// will create border when transparent
			//GUI_DrawRectEx(&Rect);

			// ToDo: Why multiple repaints ??
			//printf("paint\r\n");

			UpdateExitBox(hDlg);
			break;
		}

		case WM_DELETE:
		{
			//WM_DeleteTimer(hTimerWiFi);

			WM_HideWindow(hPowerDialog);

			// Clear screen
			//GUI_SetBkColor(GUI_BLACK);
			//GUI_Clear();

			// Init controls
			//ui_proc_init_desktop();
			ui_controls_spectrum_init(WM_HBKWIN);
			ui_proc_clear_active();

			hPowerDialog = 0;

			break;
		}

		case WM_NOTIFY_PARENT:
		{
			Id    = WM_GetId(pMsg->hWinSrc);    /* Id of widget */
			NCode = pMsg->Data.v;               /* Notification code */

			PW_cbControl(pMsg,Id,NCode);
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

uchar on_screen_power_init(WM_HWIN hParent)
{
	if(hPowerDialog == 0)
	{
	    //HEADER_SetDefaultBkColor(GUI_LIGHTBLUE);
	    //HEADER_SetDefaultTextColor(GUI_BLUE);
	    HEADER_SetDefaultFont(&GUI_Font20_1);

		hPowerDialog = GUI_CreateDialogBox(PowerDialog, GUI_COUNTOF(PowerDialog), PowerHandler, hParent, PWR_X, PWR_Y);
		return 1;
	}
	//else
	//	WM_ShowWindow(hKeybDialog);

	return 0;
}

void on_screen_power_quit(void)
{
	GUI_EndDialog(hPowerDialog, 0);
	hPowerDialog = 0;
	//WM_HideWindow(hWiFiDialog);
}

void on_screen_power_refresh(void)
{
	if(hPowerDialog == 0)
		return;

	#ifdef CONTEXT_BMS
	if(!bmss.rr)
		return;
	#endif

	//printf("bms update req\r\n");
	WM_InvalidateWindow(hPowerDialog);
}

#endif
