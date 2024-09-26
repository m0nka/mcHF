/************************************************************************************
**                                                                                 **
**                             mcHF Pro QRP Transceiver                            **
**                         Krassi Atanassov - M0NKA, 2013-2024                     **
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

#ifdef CONTEXT_VIDEO

#include "ui_menu_layout.h"
#include "gui.h"
#include "dialog.h"

#include "ui_menu_module.h"
#include "ui_actions.h"

#include "menu_pa.h"

extern GUI_CONST_STORAGE GUI_BITMAP bmtx;

// Public radio state
extern struct	TRANSCEIVER_STATE_UI	tsu;
extern TaskHandle_t 					hAudioTask;
extern TaskHandle_t 					hIccTask;
extern TaskHandle_t 					hTrxTask;

// UI driver public state
extern struct	UI_DRIVER_STATE			ui_s;

// Menu layout definitions from Flash
extern const struct UIMenuLayout menu_layout[];

static void Startup(WM_HWIN hWin, uint16_t xpos, uint16_t ypos);
static void KillPA(void);

K_ModuleItem_Typedef  menu_pa =
{
  5,
  "Power Amplifier",
  &bmtx,
  Startup,
  NULL,
  KillPA
};


static const GUI_WIDGET_CREATE_INFO _aDialog[] = 
{
	// -----------------------------------------------------------------------------------------------------------------------------
	//							name			id					x		y		xsize		ysize		?		?		?
	// -----------------------------------------------------------------------------------------------------------------------------
	// Self
	{ WINDOW_CreateIndirect,	"", 			ID_WINDOW_0,		0,    	0,		800,		430, 		0, 		0x64, 	0 },
	// Back Button
	//{ BUTTON_CreateIndirect, 	"Quit",			ID_BUTTON_EXIT, 	765, 	375, 	80, 		45, 		0, 		0x0, 	0 },
	// TUNE button
	{ BUTTON_CreateIndirect, 	"TUNE",			GUI_ID_BUTTON0, 	150, 	325, 	100, 		80, 		0, 		0x0, 	0 },

	// Check boxes
	{ CHECKBOX_CreateIndirect,	"Chk0", 		ID_CHECKBOX_0, 		CHK1X, 	CHK1Y,	200, 		30, 		0, 		0x0, 	0 },
	{ CHECKBOX_CreateIndirect,	"Chk1",	 		ID_CHECKBOX_1, 		CHK2X, 	CHK2Y,	200, 		30, 		0, 		0x0, 	0 },

	// Sliders, bias and power factor
	{ SLIDER_CreateIndirect, 	"Bias0", 		ID_SLIDER_0, 		SLD1X, 	SLD1Y,  SLD1SX, 	SLD1SY, 	0, 		0x0, 	0 },
	{ SLIDER_CreateIndirect, 	"Bias1", 		ID_SLIDER_1, 		SLD2X, 	SLD2Y,  SLD2SX, 	SLD2SY,		0, 		0x0, 	0 },
	{ SLIDER_CreateIndirect, 	"BandPF", 		ID_SLIDER_2, 		SLD3X, 	SLD3Y,  SLD3SX, 	SLD3SY,		0, 		0x0, 	0 },

	// Slider text
	{ EDIT_CreateIndirect, 		"Edit0",		GUI_ID_EDIT0,		TXT1X,	 TXT1Y,	TXT1SX, 	TXT1SY,  	0, 		0x0,	0 },
	{ EDIT_CreateIndirect, 		"Edit1", 		GUI_ID_EDIT1,		TXT2X,	 TXT2Y,	TXT2SX, 	TXT2SY,  	0, 		0x0,	0 },
	{ EDIT_CreateIndirect, 		"Edit2", 		GUI_ID_EDIT2,		TXT3X,	 TXT3Y,	TXT3SX, 	TXT3SY,  	0, 		0x0,	0 },

	// Power out and Vcc text
	{ EDIT_CreateIndirect, 		"Edit3", 		GUI_ID_EDIT3,		TXT4X,	 TXT4Y,	TXT4SX, 	TXT4SY,  	0, 		0x0,	0 },
	{ EDIT_CreateIndirect, 		"Edit4", 		GUI_ID_EDIT4,		TXT5X,	 TXT5Y,	TXT5SX, 	TXT5SY,  	0, 		0x0,	0 },

	// Band selection
	{ TEXT_CreateIndirect, 		"Band List",	GUI_ID_TEXT0,		10,		40,		120, 		40,  		0, 		0x0,	0 },
	{ LISTBOX_CreateIndirect, 	"BandList",		GUI_ID_LISTBOX0, 	10, 	90, 	120, 		340, 		0, 		0x0, 	0 },

	{ TEXT_CreateIndirect,"Finals Bias/Power Factor",GUI_ID_TEXT1,		150,	40,		550, 		40,  		0, 		0x0,	0 }
};

WM_HWIN hPA;

uchar allow_bias_update = 0;

static void _show_pa_sch(void)
{
	//GUI_DrawBitmap(&bmtx_block397, DIAG_X, DIAG_Y);

	if(tsu.rxtx)
		GUI_SetColor(GUI_RED);
	else
		GUI_SetColor(GUI_GREEN);

	//GUI_DrawRoundedRect(DIAG_X, DIAG_Y, DIAG_X + DIAG_SZ_X, DIAG_Y + DIAG_SZ_Y, 3);
	//GUI_DrawRoundedRect(DIAG_X - 1, DIAG_Y - 1, DIAG_X + DIAG_SZ_X + 1, DIAG_Y + DIAG_SZ_Y + 1, 3);
	//GUI_DrawRoundedRect(DIAG_X - 2, DIAG_Y - 2, DIAG_X + DIAG_SZ_X + 2, DIAG_Y + DIAG_SZ_Y + 2, 3);

	GUI_DrawRoundedRect(FRAME_X, FRAME_Y, FRAME_X + FRAME_SZ_X, FRAME_Y + FRAME_SZ_Y, 3);
	GUI_DrawRoundedRect(FRAME_X - 1, FRAME_Y - 1, FRAME_X + FRAME_SZ_X + 1, FRAME_Y + FRAME_SZ_Y + 1, 3);
	GUI_DrawRoundedRect(FRAME_X - 2, FRAME_Y - 2, FRAME_X + FRAME_SZ_X + 2, FRAME_Y + FRAME_SZ_Y + 2, 3);
}

static void _toggle_rx_tx_loc(void)
{
	uchar new_state;

	if((hAudioTask == NULL)||(hIccTask == NULL))
		return;

	if(tsu.rxtx)
		new_state = 0;
	else
		new_state = 1;

	tsu.tune = new_state;
	tsu.rxtx = new_state;

	if(new_state)
		xTaskNotify(hIccTask, 	UI_ICC_TUNE, 	eSetValueWithOverwrite);	// Change DSP mode to TUNE via cmd

	xTaskNotify(hAudioTask, UI_RXTX_SWITCH, eSetValueWithOverwrite);		// Switch Codec path
	vTaskDelay(100);

	if(!tsu.rxtx)
	{
		HAL_HSEM_FastTake(HSEM_ID_21);										// Fast DSP RX/TX switch
		HAL_HSEM_Release (HSEM_ID_21, 0);

		xTaskNotify(hIccTask, 	UI_ICC_TUNE, 	eSetValueWithOverwrite);	// Change DSP mode, TUNE OFF
	}
	else
	{
		HAL_HSEM_FastTake(HSEM_ID_20);
		HAL_HSEM_Release (HSEM_ID_20, 0);
	}
}

static void _bias_set(WM_MESSAGE * pMsg)
{
	WM_HWIN hChk0, hChk1,hSld,hEdit;
	uchar val1, val2;
	ulong msg = 1;
	uchar perc;
	char 	tbuf[20];

	if(!allow_bias_update)
		return;

	hChk0 = WM_GetDialogItem(pMsg->hWin, ID_CHECKBOX_0);
	val1 = (uchar)CHECKBOX_GetState(hChk0);
	//printf("chk0 value=%d\r\n",val1);

	if(val1)
	{
		hSld = WM_GetDialogItem(pMsg->hWin, ID_SLIDER_0);
		perc = SLIDER_GetValue(hSld);
		//printf("slider0 value=%d\r\n", perc);

		tsu.bias0 = perc * 50;
		CHECKBOX_SetText(hChk0, "ON");
	}
	else
	{
		tsu.bias0 = 0;
		CHECKBOX_SetText(hChk0, "OFF");
	}

	hEdit = WM_GetDialogItem(pMsg->hWin, GUI_ID_EDIT0 + 0);
	sprintf(tbuf, "%d.%dV", tsu.bias0/1000, (tsu.bias0%1000)/10);
	EDIT_SetText(hEdit, tbuf);

	hChk1 = WM_GetDialogItem(pMsg->hWin, ID_CHECKBOX_1);
	val2 = (uchar)CHECKBOX_GetState(hChk1);
	//printf("chk1 value=%d\r\n",val2);

	if(val2)
	{
		hSld = WM_GetDialogItem(pMsg->hWin, ID_SLIDER_1);
		perc = SLIDER_GetValue(hSld);
		//printf("slider1 value=%d\r\n",perc);

		tsu.bias1 = perc * 50;
		CHECKBOX_SetText(hChk1, "ON");
	}
	else
	{
		tsu.bias1 = 0;
		CHECKBOX_SetText(hChk1, "OFF");
	}

	hEdit = WM_GetDialogItem(pMsg->hWin, GUI_ID_EDIT0 + 1);
	sprintf(tbuf, "%d.%dV", tsu.bias1/1000, (tsu.bias1%1000)/10);
	EDIT_SetText(hEdit, tbuf);

	//       all		bias0	   		bias1	   power on
	msg |= (2|4|8) | (val1 << 16) | (val2 << 24) | (1 << 8);

	#ifdef CONTEXT_TRX
	if(hTrxTask != NULL)
	xTaskNotify(hTrxTask, msg, eSetValueWithOverwrite);
	#endif
}

static void menu_pa_edit_look(WM_HWIN hEdit)
{
	EDIT_SetFont(hEdit,&GUI_Font16B_1);
	EDIT_SetBkColor(hEdit,EDIT_CI_ENABLED,GUI_LIGHTBLUE);
	EDIT_SetTextColor(hEdit,EDIT_CI_ENABLED,GUI_WHITE);
	EDIT_SetTextAlign(hEdit,TEXT_CF_HCENTER|TEXT_CF_VCENTER);
}

static void _cbControl(WM_MESSAGE * pMsg, int Id, int NCode)
{
	WM_HWIN hItem, hEdit;
	char 	tbuf[20];

	switch(Id)
	{
		#if 0
		// Exit Button
		case ID_BUTTON_EXIT:
		{
			switch(NCode)
			{
				case WM_NOTIFICATION_RELEASED:
				{
					if(tsu.rxtx)				// Never leave in TX mode
						_toggle_rx_tx_loc();

					GUI_EndDialog(pMsg->hWin, 0);
					break;
				}
			}
			break;
		}
		#endif

		// TUNE button
		case GUI_ID_BUTTON0:
		{
			switch(NCode)
			{
				case WM_NOTIFICATION_RELEASED:
				{
					//printf("TUNE click\r\n");
					_toggle_rx_tx_loc();			// change trx mode
					WM_InvalidateWindow(hPA);		// repaint bar around sch
					break;
				}
			}
			break;
		}

		case ID_SLIDER_0:
		case ID_SLIDER_1:
		{
			switch(NCode)
			{
		      case WM_NOTIFICATION_CLICKED:
		        break;
		      case WM_NOTIFICATION_RELEASED:
		        break;
		      case WM_NOTIFICATION_VALUE_CHANGED:
		    	  _bias_set(pMsg);
		        break;
		      }
		      break;
		}

		case ID_SLIDER_2:
		{
			switch(NCode)
			{
		      case WM_NOTIFICATION_CLICKED:
		        break;
		      case WM_NOTIFICATION_RELEASED:
		        break;
		      case WM_NOTIFICATION_VALUE_CHANGED:
		      {
				hItem = WM_GetDialogItem(pMsg->hWin, ID_SLIDER_2);
				tsu.band[tsu.curr_band].power_factor = SLIDER_GetValue(hItem);

				hEdit = WM_GetDialogItem(pMsg->hWin, GUI_ID_EDIT0 + 2);
				sprintf(tbuf, "%d", tsu.band[tsu.curr_band].power_factor);
				EDIT_SetText(hEdit, tbuf);

				// ToDo: push to DSP ?

		        break;
		      }
		     }
		    break;
		}

		case GUI_ID_LISTBOX0:
		{
			switch(NCode)
		    {
				case WM_NOTIFICATION_CLICKED:
					break;
				case WM_NOTIFICATION_RELEASED:
					break;
				case WM_NOTIFICATION_SEL_CHANGED:
				{
					hItem = WM_GetDialogItem(pMsg->hWin, GUI_ID_LISTBOX0);
					int sel = LISTBOX_GetSel(hItem);

					if(sel != LISTBOX_ALL_ITEMS)
					{
						//char 	buf[50];
						//LISTBOX_GetItemText(hItem,sel,buf,sizeof(buf));
						//printf("lb0 text=%s(%d)\r\n", buf, sel);

						ui_actions_change_band((uchar)(sel + 2), 1);

						hItem = WM_GetDialogItem(pMsg->hWin, ID_SLIDER_2);
						SLIDER_SetValue(hItem, tsu.band[tsu.curr_band].power_factor);

						hEdit = WM_GetDialogItem(pMsg->hWin, GUI_ID_EDIT0 + 2);
						sprintf(tbuf, "%d", tsu.band[tsu.curr_band].power_factor);
						EDIT_SetText(hEdit, tbuf);
					}

					break;
				}
				default:
					break;
		    }
			break;
		}

		case ID_CHECKBOX_0:
		{
			if(NCode == WM_NOTIFICATION_VALUE_CHANGED)
			{
				#if 0
				hItem = WM_GetDialogItem(pMsg->hWin, ID_CHECKBOX_0);
				uchar val = (uchar)CHECKBOX_GetState(hItem);
				printf("chk0 value=%d\r\n",val);

				xTaskNotify(hTrxTask, ((val << 8)|(4|3)), eSetValueWithOverwrite);
				#endif

				_bias_set(pMsg);
			}
			break;
		}

		case ID_CHECKBOX_1:
		{
			if(NCode == WM_NOTIFICATION_VALUE_CHANGED)
			{
				#if 0
				hItem = WM_GetDialogItem(pMsg->hWin, ID_CHECKBOX_1);
				uchar val = (uchar)CHECKBOX_GetState(hItem);
				printf("chk1 value=%d\r\n",val);

				xTaskNotify(hTrxTask, ((val << 8)|(8|3)), eSetValueWithOverwrite);
				#endif

				_bias_set(pMsg);
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
	WM_HWIN hItem, hEdit;
	int 	Id, NCode;
	WM_HWIN hDlg;
	uchar 	perc;
	char 	tbuf[20];

	hDlg = pMsg->hWin;

	switch (pMsg->MsgId)
	{
		case WM_INIT_DIALOG:
		{
			//GUI_DrawBitmap(&bmtx_block397, DIAG_X, DIAG_Y);
			//GUI_SetColor(GUI_DARKGREEN);
			//GUI_DrawRoundedRect(DIAG_X, DIAG_Y, DIAG_X + DIAG_SZ_X, DIAG_Y + DIAG_SZ_Y, 3);
			//GUI_DrawRoundedRect(DIAG_X - 1, DIAG_Y - 1, DIAG_X + DIAG_SZ_X + 1, DIAG_Y + DIAG_SZ_Y + 1, 3);
			_show_pa_sch();

			// Init Checkbox
			hItem = WM_GetDialogItem(pMsg->hWin, ID_CHECKBOX_0);
			CHECKBOX_SetFont(hItem,&GUI_Font16B_1);
			CHECKBOX_SetText(hItem, "ON");
			CHECKBOX_SetState(hItem, 1);
			// Init Checkbox
			hItem = WM_GetDialogItem(pMsg->hWin, ID_CHECKBOX_1);
			CHECKBOX_SetFont(hItem,&GUI_Font16B_1);
			CHECKBOX_SetText(hItem, "ON");
			CHECKBOX_SetState(hItem, 1);

	  		for (int i = 0; i < 2; i++)
	    	{
	    		hEdit = WM_GetDialogItem(hDlg, GUI_ID_EDIT0 + i + 3);
	    		//EDIT_SetFont(hEdit,&GUI_Font16B_1);
	    		//EDIT_SetBkColor(hEdit,EDIT_CI_ENABLED,GUI_LIGHTBLUE);
	    		//EDIT_SetTextColor(hEdit,EDIT_CI_ENABLED,GUI_WHITE);
	    		//EDIT_SetTextAlign(hEdit,TEXT_CF_HCENTER|TEXT_CF_VCENTER);
	    		menu_pa_edit_look(hEdit);

	    		if(i == 0)
	    			EDIT_SetText(hEdit, "20W");
	    		else
	    			EDIT_SetText(hEdit, "16V");
	    	}

			// Fill voltage 1 edit
			hEdit = WM_GetDialogItem(hDlg, GUI_ID_EDIT0 + 0);
			//EDIT_SetFont(hEdit,&GUI_Font16B_1);
			//EDIT_SetBkColor(hEdit,EDIT_CI_ENABLED,GUI_LIGHTBLUE);
			//EDIT_SetTextColor(hEdit,EDIT_CI_ENABLED,GUI_WHITE);
			//EDIT_SetTextAlign(hEdit,TEXT_CF_HCENTER|TEXT_CF_VCENTER);
			menu_pa_edit_look(hEdit);
			sprintf(tbuf, "%d.%dV", tsu.bias0/1000, (tsu.bias0%1000)/10);
			EDIT_SetText(hEdit, tbuf);

			// Fill voltage 2 edit
			hEdit = WM_GetDialogItem(hDlg, GUI_ID_EDIT0 + 1);
			menu_pa_edit_look(hEdit);
			sprintf(tbuf, "%d.%dV", tsu.bias1/1000, (tsu.bias1%1000)/10);
			EDIT_SetText(hEdit, tbuf);

			// Fill PF  edit
			hEdit = WM_GetDialogItem(hDlg, GUI_ID_EDIT0 + 2);
			menu_pa_edit_look(hEdit);
			sprintf(tbuf, "%d", tsu.band[tsu.curr_band].power_factor);
			EDIT_SetText(hEdit, tbuf);

			hItem = WM_GetDialogItem(pMsg->hWin, ID_SLIDER_0);
			SLIDER_SetWidth(hItem, 20);
			SLIDER_SetRange(hItem,0, 100);

			// Slider value from voltage
			perc = (tsu.bias0*100)/5000;
			SLIDER_SetValue(hItem, perc);

			hItem = WM_GetDialogItem(pMsg->hWin, ID_SLIDER_1);
			SLIDER_SetWidth(hItem, 20);
			SLIDER_SetRange(hItem,0, 100);

			// Slider value from voltage
			perc = (tsu.bias1*100)/5000;
			SLIDER_SetValue(hItem, perc);

			hItem = WM_GetDialogItem(pMsg->hWin, ID_SLIDER_2);
			SLIDER_SetWidth(hItem, 20);
			SLIDER_SetRange(hItem, 0, 255);
			SLIDER_SetValue(hItem, tsu.band[tsu.curr_band].power_factor);

			// Band list
			hItem = WM_GetDialogItem(pMsg->hWin, GUI_ID_TEXT0);
			TEXT_SetFont(hItem,&GUI_Font24B_1);
			TEXT_SetBkColor(hItem,GUI_LIGHTBLUE);
			TEXT_SetTextColor(hItem,GUI_WHITE);
			TEXT_SetTextAlign(hItem,TEXT_CF_HCENTER|TEXT_CF_VCENTER);

			hItem = WM_GetDialogItem(pMsg->hWin, GUI_ID_LISTBOX0);
			LISTBOX_SetFont(hItem, &GUI_Font32B_1);
			LISTBOX_SetTextColor(hItem, LISTBOX_CI_UNSEL, GUI_LIGHTBLUE);
			LISTBOX_AddString(hItem, "160m");
			LISTBOX_AddString(hItem, "80m");
			LISTBOX_AddString(hItem, "60m");
			LISTBOX_AddString(hItem, "40m");
			LISTBOX_AddString(hItem, "30m");
			LISTBOX_AddString(hItem, "20m");
			LISTBOX_AddString(hItem, "17m");
			LISTBOX_AddString(hItem, "15m");
			LISTBOX_AddString(hItem, "12m");
			LISTBOX_AddString(hItem, "10m");
			LISTBOX_SetSel(hItem, tsu.curr_band - 2);

			hItem = WM_GetDialogItem(pMsg->hWin, GUI_ID_TEXT1);
			TEXT_SetFont(hItem, &GUI_Font24B_1);
			TEXT_SetBkColor(hItem, GUI_DARKCYAN);
			TEXT_SetTextColor(hItem, GUI_WHITE);
			TEXT_SetTextAlign(hItem, TEXT_CF_HCENTER|TEXT_CF_VCENTER);

			allow_bias_update = 1;
			break;
		}

		case WM_PAINT:
		{
			_show_pa_sch();
			break;
		}

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

	hPA = GUI_CreateDialogBox(p_widget, GUI_COUNTOF(_aDialog), _cbDialog, hWin, xpos, ypos);

	free(p_widget);
	return;

use_const_decl:
	hPA = GUI_CreateDialogBox(_aDialog, GUI_COUNTOF(_aDialog), _cbDialog, hWin, xpos, ypos);
}

static void KillPA(void)
{
	if(tsu.rxtx)	// Never leave in TX mode
		_toggle_rx_tx_loc();

	//printf("kill menu\r\n");
	GUI_EndDialog(hPA, 0);
}

#endif
