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

extern GUI_CONST_STORAGE GUI_BITMAP bmicon_compref;

// UI driver public state
extern struct	UI_DRIVER_STATE			ui_s;
  
// Menu layout definitions from Flash
extern const struct UIMenuLayout menu_layout[];

static void Startup(WM_HWIN hWin, uint16_t xpos, uint16_t ypos);
static void KillDsps(void);

K_ModuleItem_Typedef  dsp_s =
{
  4,
  "Standard DSP Menu",
  &bmicon_compref,
  Startup,
  NULL,
  KillDsps
};

WM_HWIN   	hDSdialog;

#define ID_WINDOW_0               	(GUI_ID_USER + 0x00)
//#define ID_BUTTON_EXIT           	(GUI_ID_USER + 0x01)

#define ID_CHECKBOX_0				(GUI_ID_USER + 0x03)
#define ID_CHECKBOX_1				(GUI_ID_USER + 0x04)
#define ID_CHECKBOX_2				(GUI_ID_USER + 0x05)
#define ID_CHECKBOX_3				(GUI_ID_USER + 0x06)
#define ID_CHECKBOX_4				(GUI_ID_USER + 0x07)
#define ID_CHECKBOX_5				(GUI_ID_USER + 0x08)
#define ID_CHECKBOX_6				(GUI_ID_USER + 0x09)
#define ID_CHECKBOX_7				(GUI_ID_USER + 0x0A)
#define ID_CHECKBOX_8				(GUI_ID_USER + 0x0B)

#define ID_TEXT_SPIN_0             	(GUI_ID_USER + 0x0C)
#define ID_SPINBOX_0       			(GUI_ID_USER + 0x0D)
#define ID_TEXT_SPIN_1             	(GUI_ID_USER + 0x0E)
#define ID_SPINBOX_1       			(GUI_ID_USER + 0x0F)
#define ID_TEXT_SPIN_2             	(GUI_ID_USER + 0x10)
#define ID_SPINBOX_2       			(GUI_ID_USER + 0x11)
#define ID_TEXT_SPIN_3             	(GUI_ID_USER + 0x12)
#define ID_SPINBOX_3       			(GUI_ID_USER + 0x13)

#define ID_TEXT_LIST_0             	(GUI_ID_USER + 0x14)
#define ID_LISTBOX_0         		(GUI_ID_USER + 0x15)
#define ID_TEXT_LIST_1             	(GUI_ID_USER + 0x16)
#define ID_LISTBOX_1         		(GUI_ID_USER + 0x17)
#define ID_TEXT_LIST_2             	(GUI_ID_USER + 0x18)
#define ID_LISTBOX_2         		(GUI_ID_USER + 0x19)
#define ID_TEXT_LIST_3             	(GUI_ID_USER + 0x1A)
#define ID_LISTBOX_3         		(GUI_ID_USER + 0x1B)

#define ID_RADIO_0         			(GUI_ID_USER + 0x1C)

static const GUI_WIDGET_CREATE_INFO _aDialog[] = 
{
	// -----------------------------------------------------------------------------------------------------------------------------
	//							name						id					x		y		xsize	ysize	?		?		?
	// -----------------------------------------------------------------------------------------------------------------------------
	// Self
	{ WINDOW_CreateIndirect,	"", 						ID_WINDOW_0,		0,    	0,		800,	430, 	0, 		0x64, 	0 },
	// Back Button
	//{ BUTTON_CreateIndirect, 	"Back",			 			ID_BUTTON_EXIT, 	670, 	375, 	120, 	45, 	0, 		0x0, 	0 },
	// Check boxes
	{ CHECKBOX_CreateIndirect,	"Checkbox", 				ID_CHECKBOX_0, 		10, 	 10,	200, 	30, 	0, 		0x0, 	0 },
	{ CHECKBOX_CreateIndirect,	"Checkbox", 				ID_CHECKBOX_1, 		10, 	 50,	200, 	30, 	0, 		0x0, 	0 },
	{ CHECKBOX_CreateIndirect,	"Checkbox", 				ID_CHECKBOX_2, 		10, 	 90,	200, 	30, 	0, 		0x0, 	0 },
	{ CHECKBOX_CreateIndirect,	"Checkbox", 				ID_CHECKBOX_3, 		10, 	130,	200, 	30, 	0, 		0x0, 	0 },
	{ CHECKBOX_CreateIndirect,	"Checkbox", 				ID_CHECKBOX_4, 		10, 	170,	200, 	30, 	0, 		0x0, 	0 },
	{ CHECKBOX_CreateIndirect,	"Checkbox", 				ID_CHECKBOX_5, 		10, 	210,	200, 	30, 	0, 		0x0, 	0 },
	{ CHECKBOX_CreateIndirect,	"Checkbox", 				ID_CHECKBOX_6, 		10, 	250,	200, 	30, 	0, 		0x0, 	0 },
	{ CHECKBOX_CreateIndirect,	"Checkbox", 				ID_CHECKBOX_7, 		10, 	290,	200, 	30, 	0, 		0x0, 	0 },
	{ CHECKBOX_CreateIndirect,	"Checkbox", 				ID_CHECKBOX_8, 		10, 	330,	200, 	30, 	0, 		0x0, 	0 },
	// Spin boxes
	{ TEXT_CreateIndirect, 		"SAM PLL Locking Range", 	ID_TEXT_SPIN_0,		200,	 10,	160, 	20,  	0, 		0x0,	0 },
	{ SPINBOX_CreateIndirect, 	"Spinbox", 					ID_SPINBOX_0, 		200, 	 28, 	160, 	50, 	0, 		0x0, 	0 },
	{ TEXT_CreateIndirect, 		"SAM PLL Step Response", 	ID_TEXT_SPIN_1,		200,	 90,	160, 	20,  	0, 		0x0,	0 },
	{ SPINBOX_CreateIndirect, 	"Spinbox", 					ID_SPINBOX_1, 		200, 	108, 	160, 	50, 	0, 		0x0, 	0 },
	{ TEXT_CreateIndirect, 		"SAM PLL Bandwidth(Hz)", 	ID_TEXT_SPIN_2,		200,	170,	160, 	20,  	0, 		0x0,	0 },
	{ SPINBOX_CreateIndirect, 	"Spinbox", 					ID_SPINBOX_2, 		200, 	188, 	160, 	50, 	0, 		0x0, 	0 },
	{ TEXT_CreateIndirect, 		"RF Gain",					ID_TEXT_SPIN_3,		200,	250,	160, 	20,  	0, 		0x0,	0 },
	{ SPINBOX_CreateIndirect, 	"Spinbox", 					ID_SPINBOX_3, 		200, 	268, 	160, 	50, 	0, 		0x0, 	0 },
	// List boxes
	{ TEXT_CreateIndirect, 		"FM Sub Tone Generate", 	ID_TEXT_LIST_0,		400,	 10,	160, 	20,  	0, 		0x0,	0 },
	{ LISTBOX_CreateIndirect, 	"Listbox", 					ID_LISTBOX_0, 		400, 	 28, 	160, 	130, 	0, 		0x0, 	0 },
	{ TEXT_CreateIndirect, 		"FM Sub Tone Detect", 		ID_TEXT_LIST_1,		400,	170,	160, 	20,  	0, 		0x0,	0 },
	{ LISTBOX_CreateIndirect, 	"Listbox", 					ID_LISTBOX_1, 		400, 	188, 	160, 	130, 	0, 		0x0, 	0 },
	{ TEXT_CreateIndirect, 		"FM Tone Burst",	 		ID_TEXT_LIST_2,		600,	10,		160, 	20,  	0, 		0x0,	0 },
	{ LISTBOX_CreateIndirect, 	"Listbox", 					ID_LISTBOX_2, 		600, 	28, 	160, 	130, 	0, 		0x0, 	0 },
	{ TEXT_CreateIndirect, 		"RX/TX Freq Translate",		ID_TEXT_LIST_3,		600,	170,	160, 	20,  	0, 		0x0,	0 },
	{ LISTBOX_CreateIndirect, 	"Listbox", 					ID_LISTBOX_3, 		600, 	188, 	160, 	130, 	0, 		0x0, 	0 },
	// Radio box
	{ RADIO_CreateIndirect, 	"Radio", 					ID_RADIO_0, 		200, 	330, 	160, 	80, 	0, 		0x1002,	0 },
};

static void settings1_list_add_items(WM_HWIN hItem)
{
	LISTBOX_AddString(hItem, "OFF");
	LISTBOX_AddString(hItem, "67.0");
	LISTBOX_AddString(hItem, "69.3");
	LISTBOX_AddString(hItem, "71.9");
	LISTBOX_AddString(hItem, "74.4");
	LISTBOX_AddString(hItem, "77.0");
	LISTBOX_AddString(hItem, "79.7");
	LISTBOX_AddString(hItem, "82.5");
	LISTBOX_AddString(hItem, "85.4");
	LISTBOX_AddString(hItem, "88.5");
	LISTBOX_AddString(hItem, "91.5");
	LISTBOX_AddString(hItem, "94.8");
	LISTBOX_AddString(hItem, "97.4");
	LISTBOX_AddString(hItem, "100.0");
	LISTBOX_AddString(hItem, "103.5");
	LISTBOX_AddString(hItem, "107.2");
	LISTBOX_AddString(hItem, "110.9");
	LISTBOX_AddString(hItem, "114.8");
	LISTBOX_AddString(hItem, "118.8");
	LISTBOX_AddString(hItem, "123.0");
	LISTBOX_AddString(hItem, "127.3");
	LISTBOX_AddString(hItem, "131.8");
	LISTBOX_AddString(hItem, "136.5");
	LISTBOX_AddString(hItem, "141.3");
	LISTBOX_AddString(hItem, "146.2");
	LISTBOX_AddString(hItem, "150.0");
	LISTBOX_AddString(hItem, "151.4");
	LISTBOX_AddString(hItem, "156.7");
	LISTBOX_AddString(hItem, "162.2");
	LISTBOX_AddString(hItem, "167.9");
	LISTBOX_AddString(hItem, "173.8");
	LISTBOX_AddString(hItem, "179.9");
	LISTBOX_AddString(hItem, "186.2");
	LISTBOX_AddString(hItem, "192.8");
	LISTBOX_AddString(hItem, "199.5");
	LISTBOX_AddString(hItem, "206.5");
	LISTBOX_AddString(hItem, "213.8");
	LISTBOX_AddString(hItem, "221.3");
	LISTBOX_AddString(hItem, "229.1");
	LISTBOX_AddString(hItem, "237.1");
	LISTBOX_AddString(hItem, "245.5");
	LISTBOX_AddString(hItem, "254.1");
}

static void _cbControl(WM_MESSAGE * pMsg, int Id, int NCode)
{
	WM_HWIN 	hItem;
	int 		sel;
	char 		buf[50];

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
					GUI_EndDialog(pMsg->hWin, 0);
					break;
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
					printf("chk0 value=%d\r\n",CHECKBOX_GetState(hItem));
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
					printf("chk1 value=%d\r\n",CHECKBOX_GetState(hItem));
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
					printf("chk2 value=%d\r\n",CHECKBOX_GetState(hItem));
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
					printf("chk3 value=%d\r\n",CHECKBOX_GetState(hItem));
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
					printf("chk4 value=%d\r\n",CHECKBOX_GetState(hItem));
					break;
				}
				default:
					break;
		    }
			break;
		}

		// ------------------------------------------------------------
		//
		case ID_CHECKBOX_5:
		{
			switch(NCode)
		    {
				case WM_NOTIFICATION_CLICKED:
					break;
				case WM_NOTIFICATION_RELEASED:
					break;
				case WM_NOTIFICATION_VALUE_CHANGED:
				{
					hItem = WM_GetDialogItem(pMsg->hWin, ID_CHECKBOX_5);
					printf("chk5 value=%d\r\n",CHECKBOX_GetState(hItem));
					break;
				}
				default:
					break;
		    }
			break;
		}

		// ------------------------------------------------------------
		//
		case ID_CHECKBOX_6:
		{
			switch(NCode)
		    {
				case WM_NOTIFICATION_CLICKED:
					break;
				case WM_NOTIFICATION_RELEASED:
					break;
				case WM_NOTIFICATION_VALUE_CHANGED:
				{
					hItem = WM_GetDialogItem(pMsg->hWin, ID_CHECKBOX_6);
					printf("chk6 value=%d\r\n",CHECKBOX_GetState(hItem));
					break;
				}
				default:
					break;
		    }
			break;
		}

		// ------------------------------------------------------------
		//
		case ID_CHECKBOX_7:
		{
			switch(NCode)
		    {
				case WM_NOTIFICATION_CLICKED:
					break;
				case WM_NOTIFICATION_RELEASED:
					break;
				case WM_NOTIFICATION_VALUE_CHANGED:
				{
					hItem = WM_GetDialogItem(pMsg->hWin, ID_CHECKBOX_7);
					printf("chk7 value=%d\r\n",CHECKBOX_GetState(hItem));
					break;
				}
				default:
					break;
		    }
			break;
		}

		// ------------------------------------------------------------
		//
		case ID_CHECKBOX_8:
		{
			switch(NCode)
		    {
				case WM_NOTIFICATION_CLICKED:
					break;
				case WM_NOTIFICATION_RELEASED:
					break;
				case WM_NOTIFICATION_VALUE_CHANGED:
				{
					hItem = WM_GetDialogItem(pMsg->hWin, ID_CHECKBOX_8);
					printf("chk8 value=%d\r\n",CHECKBOX_GetState(hItem));
					break;
				}
				default:
					break;
		    }
			break;
		}

		// ------------------------------------------------------------
		//
		case ID_SPINBOX_0:
		{
			switch(NCode)
		    {
				case WM_NOTIFICATION_CLICKED:
					break;
				case WM_NOTIFICATION_RELEASED:
					break;
				case WM_NOTIFICATION_VALUE_CHANGED:
				{
					hItem = WM_GetDialogItem(pMsg->hWin, ID_SPINBOX_0);
					printf("sp0 value=%d\r\n",SPINBOX_GetValue(hItem));
					break;
				}
				default:
					break;
		    }
			break;
		}

		// ------------------------------------------------------------
		//
		case ID_SPINBOX_1:
		{
			switch(NCode)
		    {
				case WM_NOTIFICATION_CLICKED:
					break;
				case WM_NOTIFICATION_RELEASED:
					break;
				case WM_NOTIFICATION_VALUE_CHANGED:
				{
					hItem = WM_GetDialogItem(pMsg->hWin, ID_SPINBOX_1);
					printf("sp1 value=%d\r\n",SPINBOX_GetValue(hItem));
					break;
				}
				default:
					break;
		    }
			break;
		}

		// ------------------------------------------------------------
		//
		case ID_SPINBOX_2:
		{
			switch(NCode)
		    {
				case WM_NOTIFICATION_CLICKED:
					break;
				case WM_NOTIFICATION_RELEASED:
					break;
				case WM_NOTIFICATION_VALUE_CHANGED:
				{
					hItem = WM_GetDialogItem(pMsg->hWin, ID_SPINBOX_2);
					printf("sp2 value=%d\r\n",SPINBOX_GetValue(hItem));
					break;
				}
				default:
					break;
		    }
			break;
		}

		// ------------------------------------------------------------
		//
		case ID_SPINBOX_3:
		{
			switch(NCode)
		    {
				case WM_NOTIFICATION_CLICKED:
					break;
				case WM_NOTIFICATION_RELEASED:
					break;
				case WM_NOTIFICATION_VALUE_CHANGED:
				{
					hItem = WM_GetDialogItem(pMsg->hWin, ID_SPINBOX_3);
					printf("sp3 value=%d\r\n",SPINBOX_GetValue(hItem));
					break;
				}
				default:
					break;
		    }
			break;
		}

		// ------------------------------------------------------------
		//
		case ID_LISTBOX_0:
		{
			switch(NCode)
		    {
				case WM_NOTIFICATION_CLICKED:
					break;
				case WM_NOTIFICATION_RELEASED:
					break;
				case WM_NOTIFICATION_SEL_CHANGED:
				{
					hItem = WM_GetDialogItem(pMsg->hWin, ID_LISTBOX_0);
					sel = LISTBOX_GetSel(hItem);
					if(sel != LISTBOX_ALL_ITEMS)
					{
						LISTBOX_GetItemText(hItem,sel,buf,sizeof(buf));
						printf("lb0 text=%s\r\n",buf);
					}

					break;
				}
				default:
					break;
		    }
			break;
		}

		// ------------------------------------------------------------
		//
		case ID_LISTBOX_1:
		{
			switch(NCode)
		    {
				case WM_NOTIFICATION_CLICKED:
					break;
				case WM_NOTIFICATION_RELEASED:
					break;
				case WM_NOTIFICATION_SEL_CHANGED:
				{
					hItem = WM_GetDialogItem(pMsg->hWin, ID_LISTBOX_1);
					sel = LISTBOX_GetSel(hItem);
					if(sel != LISTBOX_ALL_ITEMS)
					{
						LISTBOX_GetItemText(hItem,sel,buf,sizeof(buf));
						printf("lb1 text=%s\r\n",buf);
					}

					break;
				}
				default:
					break;
		    }
			break;
		}

		// ------------------------------------------------------------
		//
		case ID_LISTBOX_2:
		{
			switch(NCode)
		    {
				case WM_NOTIFICATION_CLICKED:
					break;
				case WM_NOTIFICATION_RELEASED:
					break;
				case WM_NOTIFICATION_SEL_CHANGED:
				{
					hItem = WM_GetDialogItem(pMsg->hWin, ID_LISTBOX_2);
					sel = LISTBOX_GetSel(hItem);
					if(sel != LISTBOX_ALL_ITEMS)
					{
						LISTBOX_GetItemText(hItem,sel,buf,sizeof(buf));
						printf("lb2 text=%s\r\n",buf);
					}

					break;
				}
				default:
					break;
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
	SCROLLBAR_Handle 	hScrollV;

	switch (pMsg->MsgId)
	{
		case WM_INIT_DIALOG:
		{
			// Init Exit button
			//hItem = BUTTON_CreateEx(695, 375, 100, 60, pMsg->hWin, WM_CF_SHOW, 0, ID_BUTTON_EXIT);
			//WM_SetCallback(hItem, _cbButton_exit);
    
			// Format Title
			//hItem = WM_GetDialogItem(pMsg->hWin, ID_TEXT_TITLE);
			//TEXT_SetFont(hItem,&GUI_FontAvantGarde32);

			// Init Checkbox
			hItem = WM_GetDialogItem(pMsg->hWin, ID_CHECKBOX_0);
			CHECKBOX_SetFont(hItem,&GUI_Font16_1);
			CHECKBOX_SetText(hItem, "LSB/USB Auto Select");
			CHECKBOX_SetState(hItem, 1);
			// Init Checkbox
			hItem = WM_GetDialogItem(pMsg->hWin, ID_CHECKBOX_1);
			CHECKBOX_SetFont(hItem,&GUI_Font16_1);
			CHECKBOX_SetText(hItem, "Digital Modes");
			CHECKBOX_SetState(hItem, 1);
			// Init Checkbox
			hItem = WM_GetDialogItem(pMsg->hWin, ID_CHECKBOX_2);
			CHECKBOX_SetFont(hItem,&GUI_Font16_1);
			CHECKBOX_SetText(hItem, "CW Mode");
			CHECKBOX_SetState(hItem, 1);
			// Init Checkbox
			hItem = WM_GetDialogItem(pMsg->hWin, ID_CHECKBOX_3);
			CHECKBOX_SetFont(hItem,&GUI_Font16_1);
			CHECKBOX_SetText(hItem, "AM Mode");
			CHECKBOX_SetState(hItem, 1);
			// Init Checkbox
			hItem = WM_GetDialogItem(pMsg->hWin, ID_CHECKBOX_4);
			CHECKBOX_SetFont(hItem,&GUI_Font16_1);
			CHECKBOX_SetText(hItem, "SyncAM Mode");
			CHECKBOX_SetState(hItem, 1);
			// Init Checkbox
			hItem = WM_GetDialogItem(pMsg->hWin, ID_CHECKBOX_5);
			CHECKBOX_SetFont(hItem,&GUI_Font16_1);
			CHECKBOX_SetText(hItem, "SAM Fade Leveler");
			CHECKBOX_SetState(hItem, 1);
			// Init Checkbox
			hItem = WM_GetDialogItem(pMsg->hWin, ID_CHECKBOX_6);
			CHECKBOX_SetFont(hItem,&GUI_Font16_1);
			CHECKBOX_SetText(hItem, "FM Mode");
			CHECKBOX_SetState(hItem, 1);

			// Init Spinbox
			hItem = WM_GetDialogItem(pMsg->hWin, ID_TEXT_SPIN_0);
			TEXT_SetFont(hItem,&GUI_Font16_1);
			TEXT_SetBkColor(hItem,GUI_LIGHTBLUE);
			TEXT_SetTextColor(hItem,GUI_WHITE);
			TEXT_SetTextAlign(hItem,TEXT_CF_HCENTER|TEXT_CF_VCENTER);
			hItem = WM_GetDialogItem(pMsg->hWin, ID_SPINBOX_0);
			SPINBOX_SetFont(hItem, &GUI_Font32_1);
			SPINBOX_SetTextColor(hItem,SPINBOX_CI_ENABLED,GUI_LIGHTBLUE);
			SPINBOX_SetRange(hItem,1000,3000);
			SPINBOX_SetStep(hItem,10);
			SPINBOX_SetValue(hItem,2500);
			// Init Spinbox
			hItem = WM_GetDialogItem(pMsg->hWin, ID_TEXT_SPIN_1);
			TEXT_SetFont(hItem,&GUI_Font16_1);
			TEXT_SetBkColor(hItem,GUI_LIGHTBLUE);
			TEXT_SetTextColor(hItem,GUI_WHITE);
			TEXT_SetTextAlign(hItem,TEXT_CF_HCENTER|TEXT_CF_VCENTER);
			hItem = WM_GetDialogItem(pMsg->hWin, ID_SPINBOX_1);
			SPINBOX_SetFont(hItem, &GUI_Font32_1);
			SPINBOX_SetTextColor(hItem,SPINBOX_CI_ENABLED,GUI_LIGHTBLUE);
			SPINBOX_SetRange(hItem,50,100);
			SPINBOX_SetStep(hItem,1);
			SPINBOX_SetValue(hItem,65);
			// Init Spinbox
			hItem = WM_GetDialogItem(pMsg->hWin, ID_TEXT_SPIN_2);
			TEXT_SetFont(hItem,&GUI_Font16_1);
			TEXT_SetBkColor(hItem,GUI_LIGHTBLUE);
			TEXT_SetTextColor(hItem,GUI_WHITE);
			TEXT_SetTextAlign(hItem,TEXT_CF_HCENTER|TEXT_CF_VCENTER);
			hItem = WM_GetDialogItem(pMsg->hWin, ID_SPINBOX_2);
			SPINBOX_SetFont(hItem, &GUI_Font32_1);
			SPINBOX_SetTextColor(hItem,SPINBOX_CI_ENABLED,GUI_LIGHTBLUE);
			SPINBOX_SetRange(hItem,25,500);
			SPINBOX_SetStep(hItem,5);
			SPINBOX_SetValue(hItem,250);
			// Init Spinbox
			hItem = WM_GetDialogItem(pMsg->hWin, ID_TEXT_SPIN_3);
			TEXT_SetFont(hItem,&GUI_Font16_1);
			TEXT_SetBkColor(hItem,GUI_LIGHTBLUE);
			TEXT_SetTextColor(hItem,GUI_WHITE);
			TEXT_SetTextAlign(hItem,TEXT_CF_HCENTER|TEXT_CF_VCENTER);
			hItem = WM_GetDialogItem(pMsg->hWin, ID_SPINBOX_3);
			SPINBOX_SetFont(hItem, &GUI_Font32_1);
			SPINBOX_SetTextColor(hItem,SPINBOX_CI_ENABLED,GUI_LIGHTBLUE);
			SPINBOX_SetRange(hItem,0,50);
			SPINBOX_SetStep(hItem,1);
			SPINBOX_SetValue(hItem,47);

			// Init Listbox
			hItem = WM_GetDialogItem(pMsg->hWin, ID_TEXT_LIST_0);
			TEXT_SetFont(hItem,&GUI_Font16_1);
			TEXT_SetBkColor(hItem,GUI_LIGHTBLUE);
			TEXT_SetTextColor(hItem,GUI_WHITE);
			TEXT_SetTextAlign(hItem,TEXT_CF_HCENTER|TEXT_CF_VCENTER);
			hItem = WM_GetDialogItem(pMsg->hWin, ID_LISTBOX_0);
			LISTBOX_SetFont(hItem, &GUI_Font32_1);
			LISTBOX_SetTextColor(hItem,LISTBOX_CI_UNSEL,GUI_LIGHTBLUE);
			settings1_list_add_items(hItem);
			hScrollV = SCROLLBAR_CreateAttached(hItem, SCROLLBAR_CF_VERTICAL);
			//--SCROLLBAR_SetColor(hScrollV,SCROLLBAR_CI_THUMB|SCROLLBAR_CI_SHAFT|SCROLLBAR_CI_ARROW,GUI_RED);
			// Init Listbox
			hItem = WM_GetDialogItem(pMsg->hWin, ID_TEXT_LIST_1);
			TEXT_SetFont(hItem,&GUI_Font16_1);
			TEXT_SetBkColor(hItem,GUI_LIGHTBLUE);
			TEXT_SetTextColor(hItem,GUI_WHITE);
			TEXT_SetTextAlign(hItem,TEXT_CF_HCENTER|TEXT_CF_VCENTER);
			hItem = WM_GetDialogItem(pMsg->hWin, ID_LISTBOX_1);
			LISTBOX_SetFont(hItem, &GUI_Font32_1);
			LISTBOX_SetTextColor(hItem,LISTBOX_CI_UNSEL,GUI_LIGHTBLUE);
			settings1_list_add_items(hItem);
			hScrollV = SCROLLBAR_CreateAttached(hItem, SCROLLBAR_CF_VERTICAL);
			//--SCROLLBAR_SetColor(hScrollV,SCROLLBAR_CI_THUMB|SCROLLBAR_CI_SHAFT|SCROLLBAR_CI_ARROW,GUI_RED);
			// Init Listbox
			hItem = WM_GetDialogItem(pMsg->hWin, ID_TEXT_LIST_2);
			TEXT_SetFont(hItem,&GUI_Font16_1);
			TEXT_SetBkColor(hItem,GUI_LIGHTBLUE);
			TEXT_SetTextColor(hItem,GUI_WHITE);
			TEXT_SetTextAlign(hItem,TEXT_CF_HCENTER|TEXT_CF_VCENTER);
			hItem = WM_GetDialogItem(pMsg->hWin, ID_LISTBOX_2);
			LISTBOX_SetFont(hItem, &GUI_Font32_1);
			LISTBOX_SetTextColor(hItem,LISTBOX_CI_UNSEL,GUI_LIGHTBLUE);
			LISTBOX_AddString(hItem, "OFF");
			LISTBOX_AddString(hItem, "1750Hz");
			LISTBOX_AddString(hItem, "2135Hz");
			hScrollV = SCROLLBAR_CreateAttached(hItem, SCROLLBAR_CF_VERTICAL);
			//--SCROLLBAR_SetColor(hScrollV,SCROLLBAR_CI_THUMB|SCROLLBAR_CI_SHAFT|SCROLLBAR_CI_ARROW,GUI_RED);
			// Init Listbox
			hItem = WM_GetDialogItem(pMsg->hWin, ID_TEXT_LIST_3);
			TEXT_SetFont(hItem,&GUI_Font32_1);
			TEXT_SetBkColor(hItem,GUI_LIGHTBLUE);
			TEXT_SetTextColor(hItem,GUI_WHITE);
			TEXT_SetTextAlign(hItem,TEXT_CF_HCENTER|TEXT_CF_VCENTER);
			hItem = WM_GetDialogItem(pMsg->hWin, ID_LISTBOX_3);
			LISTBOX_SetFont(hItem, &GUI_Font32_1);
			LISTBOX_SetTextColor(hItem,LISTBOX_CI_UNSEL,GUI_LIGHTBLUE);
			LISTBOX_AddString(hItem, "OFF");
			LISTBOX_AddString(hItem, "+6kHz");
			LISTBOX_AddString(hItem, "-6kHz");
			LISTBOX_AddString(hItem, "+12kHz");
			LISTBOX_AddString(hItem, "-12kHz");
			hScrollV = SCROLLBAR_CreateAttached(hItem, SCROLLBAR_CF_VERTICAL);
			//--SCROLLBAR_SetColor(hScrollV,SCROLLBAR_CI_THUMB|SCROLLBAR_CI_SHAFT|SCROLLBAR_CI_ARROW,GUI_RED);

			// Initialization of 'Radio'
			hItem = WM_GetDialogItem(pMsg->hWin, ID_RADIO_0);
			//RADIO_SetFont(hItem,&GUI_FontAvantGarde16);
			RADIO_SetText(hItem, "Narrow - 2.5kHz", 0);
			RADIO_SetText(hItem, "Wide   - 5.0kHz", 1);

			break;
		}
    
		case WM_PAINT:
			break;

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

	hDSdialog = GUI_CreateDialogBox(p_widget, GUI_COUNTOF(_aDialog), _cbDialog, hWin, xpos, ypos);

	free(p_widget);
	return;

use_const_decl:
	hDSdialog = GUI_CreateDialogBox(_aDialog, GUI_COUNTOF(_aDialog), _cbDialog, hWin, xpos, ypos);
}

static void KillDsps(void)
{
	//printf("kill menu\r\n");
	GUI_EndDialog(hDSdialog, 0);
}

#endif
