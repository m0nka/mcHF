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

#include "ui_controls_keyer.h"
#include "desktop\ui_controls_layout.h"

#include "GUI.h"

// ------------------------------
//#include "touch_driver.h"
//extern struct TD 		t_d;
// ------------------------------

// Public radio state
extern struct	TRANSCEIVER_STATE_UI	tsu;

#define ID_WINDOW_KEYER          	(GUI_ID_USER + 0x50)

//#define ID_BUTTON_EXIT          	(GUI_ID_USER + 0x51)

#define ID_BUTTON_DAH 	         	(GUI_ID_USER + 0x52)
#define ID_BUTTON_DIT   	       	(GUI_ID_USER + 0x53)

#define KEYER_X						750
#define KEYER_Y						203

#define KEYER_SIZE_X				105
#define KEYER_SIZE_Y				250

#ifdef KEYER_IS_TRANPARENT
#define KE_CF						WM_CF_SHOW|WM_CF_MEMDEV|WM_CF_HASTRANS
#else
#define KE_CF						0x64
#endif

static const GUI_WIDGET_CREATE_INFO KeyerDialog[] =
{
	// -----------------------------------------------------------------------------------------------------------------------------
	//							name		id					x		y		xsize				ysize				?		?		?
	// -----------------------------------------------------------------------------------------------------------------------------
	// Self
	{ WINDOW_CreateIndirect,	"", 		ID_WINDOW_KEYER,	0,		0,		KEYER_SIZE_X,		KEYER_SIZE_Y, 		0, 		KE_CF, 	0 },

	//{ BUTTON_CreateIndirect, 	"x",		ID_BUTTON_EXIT,		5, 		5, 		40, 			   	240, 				0, 		0x0, 	0 },

	{ BUTTON_CreateIndirect, 	"-",		ID_BUTTON_DAH,		0, 		0, 		104, 			   	122, 				0, 		0x0, 	0 },
	{ BUTTON_CreateIndirect, 	".",		ID_BUTTON_DIT,		0, 		128, 	104, 			   	122, 				0, 		0x0, 	0 },
};

//uchar	local_dah_press = 0;
//uchar	local_dit_press = 0;

WM_HWIN 	hKeyerDialog = 0;
//WM_HTIMER   hTimerKeyer;

static void KEH_cbControl(WM_MESSAGE * pMsg, int Id, int NCode)
{
	//HAL_GPIO_WritePin(GPIOG,GPIO_PIN_12, 0);		// doesn't work
	//__HAL_GPIO_EXTI_GENERATE_SWIT(EXTI_LINE_12);	// doesn't work

	switch(Id)
	{
		case ID_BUTTON_DAH:
		{
			if(NCode == WM_NOTIFICATION_MOVED_OUT)
			{
				//printf("dah moved\r\n");
				HAL_HSEM_FastTake(HSEM_ID_24);
				HAL_HSEM_Release (HSEM_ID_24, 0);
			}
			else if(NCode == WM_NOTIFICATION_CLICKED)
			{
				//printf("dah clicked\r\n");
				HAL_HSEM_FastTake(HSEM_ID_22);
				HAL_HSEM_Release (HSEM_ID_22, 0);
			}
			else if((NCode == WM_NOTIFICATION_RELEASED))
			{
				//printf("dah released\r\n");
				HAL_HSEM_FastTake(HSEM_ID_24);
				HAL_HSEM_Release (HSEM_ID_24, 0);
			}

			break;
		}

		case ID_BUTTON_DIT:
		{
			if(NCode == WM_NOTIFICATION_CLICKED)
			{
				//printf("dit down\r\n");
				HAL_HSEM_FastTake(HSEM_ID_23);
				HAL_HSEM_Release (HSEM_ID_23, 0);
			}
			else if((NCode == WM_NOTIFICATION_RELEASED)||(NCode == WM_NOTIFICATION_MOVED_OUT))
			{
				//printf("dit up\r\n");
				HAL_HSEM_FastTake(HSEM_ID_25);
				HAL_HSEM_Release (HSEM_ID_25, 0);
			}

			break;
		}

		default:
			break;
	}
}

static void KeyerHandler(WM_MESSAGE *pMsg)
{
	WM_HWIN hItem;
	int 		Id, NCode;
	//GUI_RECT	Rect;

	switch (pMsg->MsgId)
	{
		case WM_INIT_DIALOG:
		{
			#if 0
			GPIO_InitTypeDef  GPIO_InitStruct;
			GPIO_InitStruct.Mode  = GPIO_MODE_IT_FALLING;
			GPIO_InitStruct.Pull  = GPIO_NOPULL;
			GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
			GPIO_InitStruct.Pin   = GPIO_PIN_12;
			HAL_GPIO_Init(GPIOG, &GPIO_InitStruct);
			#endif

			//#ifdef KEYB_IS_TRANPARENT
			WINDOW_SetBkColor(pMsg->hWin, GUI_INVALID_COLOR);
			//#else
			//WINDOW_SetBkColor(pMsg->hWin, GUI_LIGHTGRAY);
			//#endif

			hItem = WM_GetDialogItem(pMsg->hWin, ID_BUTTON_DAH);
			BUTTON_SetFont(hItem, GUI_FONT_32_1);
			hItem = WM_GetDialogItem(pMsg->hWin, ID_BUTTON_DIT);
			BUTTON_SetFont(hItem, GUI_FONT_32_1);

			//hItem = WM_GetDialogItem(pMsg->hWin, ID_WINDOW_KEYB);
			//WM_SetTransState(hItem, WM_CF_HASTRANS);

			//TEXT_SetFont(hItem,&GUI_Font16B_1);
			//TEXT_SetBkColor(hItem,GUI_LIGHTBLUE);
			//TEXT_SetTextColor(hItem,GUI_WHITE);
			//TEXT_SetTextAlign(hItem,TEXT_CF_RIGHT|TEXT_CF_VCENTER);

			//hTimerKeyer = WM_CreateTimer(pMsg->hWin, 0, 500, 0);
			break;
		}

		case WM_TIMER:
		{
			//printf("timer ");
			//WM_RestartTimer(pMsg->Data.v, WIFI_TIMER_RESOLUTION);
			break;
		}

		case WM_PAINT:
			//WM_GetClientRect(&Rect);	// will create border when transparent
			//GUI_DrawRectEx(&Rect);
			break;


		case WM_DELETE:
		{
			//WM_DeleteTimer(hTimerKeyer);
			WM_HideWindow(hKeyerDialog);

			ui_controls_spectrum_init(WM_HBKWIN);
			//ui_proc_clear_active();

			hKeyerDialog = 0;
			break;
		}

		case WM_NOTIFY_PARENT:
		{
			Id    = WM_GetId(pMsg->hWinSrc);    /* Id of widget */
			NCode = pMsg->Data.v;               /* Notification code */

			KEH_cbControl(pMsg,Id,NCode);
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

#if 0
static void ui_controls_keyer_draw_top(uchar press)
{
	// Button background
	if(press)
		GUI_SetColor(GUI_WHITE);
	else
		GUI_SetColor(GUI_GRAY);

	GUI_FillRoundedFrame(	IAMB_BTN_TOP_X0,
							IAMB_BTN_TOP_Y0,
							IAMB_BTN_TOP_X1,
							IAMB_BTN_TOP_Y1,
							SW_FRAME_CORNER_R,
							SW_FRAME_WIDTH
						);
	// Dah symbol
	if(press)
		GUI_SetColor(GUI_GRAY);
	else
		GUI_SetColor(GUI_WHITE);

	GUI_FillRoundedFrame(	(IAMB_BTN_TOP_X0 + IAMB_KEYER_SIZE_X/2 - 25),
							(IAMB_BTN_TOP_Y0 + IAMB_KEYER_SIZE_Y/4 - 10),
							(IAMB_BTN_TOP_X0 + IAMB_KEYER_SIZE_X/2 + 25),
							(IAMB_BTN_TOP_Y0 + IAMB_KEYER_SIZE_Y/4 +  6),
							8,
							1
						);
}

static void ui_controls_keyer_draw_btm(uchar press)
{
	// Button background
	if(press)
		GUI_SetColor(GUI_WHITE);
	else
		GUI_SetColor(GUI_GRAY);

	GUI_FillRoundedFrame(	IAMB_BTN_BTM_X0,
							IAMB_BTN_BTM_Y0,
							IAMB_BTN_BTM_X1,
							IAMB_BTN_BTM_Y1,
							SW_FRAME_CORNER_R,
							SW_FRAME_WIDTH
						);
	// Dit symbol
	if(press)
		GUI_SetColor(GUI_GRAY);
	else
		GUI_SetColor(GUI_WHITE);

	GUI_FillRoundedFrame(	(IAMB_BTN_BTM_X0 + IAMB_KEYER_SIZE_X/2 -  8),
							(IAMB_BTN_BTM_Y0 + IAMB_KEYER_SIZE_Y/4 - 10),
							(IAMB_BTN_BTM_X0 + IAMB_KEYER_SIZE_X/2 +  8),
							(IAMB_BTN_BTM_Y0 + IAMB_KEYER_SIZE_Y/4 +  6),
							6,
							1
						);

}
#endif

//*----------------------------------------------------------------------------
//* Function Name       : ui_controls_keyer_init
//* Object              :
//* Notes    			:
//* Notes   			:
//* Notes    			:
//* Context    			: CONTEXT_VIDEO
//*----------------------------------------------------------------------------
uchar ui_controls_keyer_init(WM_HWIN hParent)
{
#if 0
	// Draw control frame
	GUI_SetColor(GUI_ORANGE);
	GUI_DrawRoundedFrame(	IAMB_KEYER_X,
							IAMB_KEYER_Y,
							(IAMB_KEYER_X + IAMB_KEYER_SIZE_X),
							(IAMB_KEYER_Y + IAMB_KEYER_SIZE_Y),
							SW_FRAME_CORNER_R,
							SW_FRAME_WIDTH
						);

	// Draw separator
	GUI_DrawHLine((IAMB_KEYER_Y + IAMB_KEYER_SIZE_Y/2 - 1),(IAMB_KEYER_X + 0),(IAMB_KEYER_X + IAMB_KEYER_SIZE_X));
	GUI_DrawHLine((IAMB_KEYER_Y + IAMB_KEYER_SIZE_Y/2 + 0),(IAMB_KEYER_X + 0),(IAMB_KEYER_X + IAMB_KEYER_SIZE_X));
	GUI_DrawHLine((IAMB_KEYER_Y + IAMB_KEYER_SIZE_Y/2 + 1),(IAMB_KEYER_X + 0),(IAMB_KEYER_X + IAMB_KEYER_SIZE_X));

	// Draw buttons
	ui_controls_keyer_draw_top(0);
	ui_controls_keyer_draw_btm(0);
#endif

//	if(hKeyerDialog == 0)
//	{
//		hKeyerDialog = GUI_CreateDialogBox(KeyerDialog, GUI_COUNTOF(KeyerDialog), KeyerHandler, hParent, KEYER_X, KEYER_Y);
//		return 1;
//	}
//	else
//	{
		// Doesn't work
		//ui_controls_keyer_quit();
		//ui_controls_spectrum_init(WM_HBKWIN);
		//vTaskDelay(300);
//	}

	return 0;
}

//*----------------------------------------------------------------------------
//* Function Name       :
//* Object              :
//* Notes    			:
//* Notes   			:
//* Notes    			:
//* Context    			: CONTEXT_VIDEO
//*----------------------------------------------------------------------------
void ui_controls_keyer_quit(void)
{
//	if(hKeyerDialog != 0)
//	{
////		GUI_EndDialog(hKeyerDialog, 0);
//		hKeyerDialog = 0;
//	}
}

//*----------------------------------------------------------------------------
//* Function Name       :
//* Object              :
//* Notes    			:
//* Notes   			:
//* Notes    			:
//* Context    			: CONTEXT_VIDEO
//*----------------------------------------------------------------------------
void ui_controls_keyer_touch(void)
{
#if 0
	//printf("keyer touch\r\n");

	// Only in CW mode and if displayed
	if(tsu.band[tsu.curr_band].demod_mode != DEMOD_CW) //&& (ui_sw.ctrl_type == SW_CONTROL_MID))
		return;

	// Top button
	if(		(t_d.point_x[0] >= IAMB_BTN_TOP_X0) &&\
			(t_d.point_x[0] <= IAMB_BTN_TOP_X1) &&\
			(t_d.point_y[0] >= IAMB_BTN_TOP_Y0) &&\
			(t_d.point_y[0] <= IAMB_BTN_TOP_Y1)
	  )
	{
		//printf("------------------------\r\n");
		//printf("top touched\r\n");

		// Paint as active
		local_dah_press = 1;
		ui_controls_keyer_draw_top(1);

		// Deactivate the other
		if(local_dit_press) ui_controls_keyer_draw_btm(0);

		// Pass message to API driver
		tsu.cw_iamb_type 	= 2;							// Set keyer button
		if(tsu.cw_tx_state == 0) tsu.cw_tx_state 	= 1;	// Set state
		return;
	}

	// Bottom button
	if(		(t_d.point_x[0] >= IAMB_BTN_BTM_X0) &&\
			(t_d.point_x[0] <= IAMB_BTN_BTM_X1) &&\
			(t_d.point_y[0] >= IAMB_BTN_BTM_Y0) &&\
			(t_d.point_y[0] <= IAMB_BTN_BTM_Y1)
	  )
	{
		//printf("------------------------\r\n");
		//printf("bottom touched\r\n");

		// Paint as active
		local_dit_press = 1;
		ui_controls_keyer_draw_btm(1);

		// Deactivate the other
		if(local_dah_press) ui_controls_keyer_draw_top(0);

		// Pass message to API driver
		tsu.cw_iamb_type 	= 1;							// Set keyer button
		if(tsu.cw_tx_state == 0) tsu.cw_tx_state = 1;		// Set state
		return;
	}
#endif
}

//*----------------------------------------------------------------------------
//* Function Name       :
//* Object              :
//* Notes    			:
//* Notes   			:
//* Notes    			:
//* Context    			: CONTEXT_VIDEO
//*----------------------------------------------------------------------------
void ui_controls_keyer_refresh(void)
{
	//printf("refresh->cw_tx_state = %d\r\n",tsu.cw_tx_state);
	//printf("refresh->t_d.pending = %d\r\n",t_d.pending);
#if 0
	// Button was pressed
	if((!t_d.pending) && (tsu.cw_tx_state == 1))
	//if((!t_d.pending) && (local_dah_press || local_dit_press))
	{
		//printf("keyer reset\r\n");

		tsu.cw_iamb_type 	= 0;
		tsu.cw_tx_state 	= 2;

		// Reset
		if(local_dah_press) ui_controls_keyer_draw_top(0);
		if(local_dit_press) ui_controls_keyer_draw_btm(0);
	}
#endif

//	if(hKeyerDialog == 0)
//		WM_InvalidateWindow(hKeyerDialog);
}

#endif
