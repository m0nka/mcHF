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

#include "gui.h"
#include "dialog.h"
//#include "ST_GUI_Addons.h"

#include "ui_controls_filter.h"
#include "desktop\ui_controls_layout.h"

// Public radio state
extern struct	TRANSCEIVER_STATE_UI	tsu;

uchar loc_filter = 0x99;

static void ui_controls_filter_select(void)
{
	uchar filter = tsu.band[tsu.curr_band].filter;

	// No needless repaint
	if(loc_filter == filter)
		return;

	// Create DSP update request
	//tsu.update_filter_dsp_req = 1;

	// Repaint filter background
	GUI_SetColor(FIL_BKG_COLOR);
	GUI_FillRect(FILTER_X + 34,FILTER_Y + 1,(FILTER_X + FILTER_SIZE_X - 2),(FILTER_Y + FILTER_SIZE_Y - 1));

	GUI_SetFont(&GUI_Font20_1);
	GUI_SetColor(GUI_WHITE);

	// De-selected filters text
	GUI_DispStringAt("300Hz",  (FILTER_X + FIL_BTN_X + FIL_BTN_SHFT*0), (FILTER_Y + 3));
	GUI_DispStringAt("500Hz",  (FILTER_X + FIL_BTN_X + FIL_BTN_SHFT*1), (FILTER_Y + 3));
	GUI_DispStringAt("1.8kHz", (FILTER_X + FIL_BTN_X + FIL_BTN_SHFT*2), (FILTER_Y + 3));
	GUI_DispStringAt("2.3kHz", (FILTER_X + FIL_BTN_X + FIL_BTN_SHFT*3), (FILTER_Y + 3));
	GUI_DispStringAt("3.6kHz", (FILTER_X + FIL_BTN_X + FIL_BTN_SHFT*4), (FILTER_Y + 3));
	GUI_DispStringAt("10kHz",  (FILTER_X + FIL_BTN_X + FIL_BTN_SHFT*5), (FILTER_Y + 3));

	//printf("dsp filter: %d\r\n",tsu.dsp_filter);

	// Based on DSP value, selected filter
	switch(filter)
	{
		case AUDIO_300HZ:
			GUI_SetColor(GUI_WHITE);
			GUI_FillRect((FILTER_X + FIL_BTN_X + FIL_BTN_SHFT*0 - 2),FILTER_Y,(FILTER_X + FIL_BTN_X + FIL_BTN_SHFT*1 - 13),(FILTER_Y + FILTER_SIZE_Y));
			GUI_SetColor(GUI_LIGHTBLUE);
			GUI_DispStringAt("300Hz",(FILTER_X + FIL_BTN_X + FIL_BTN_SHFT*0), (FILTER_Y + 3));
			break;

		case AUDIO_500HZ:
			GUI_SetColor(GUI_WHITE);
			GUI_FillRect((FILTER_X + FIL_BTN_X + FIL_BTN_SHFT*1 - 2),FILTER_Y,(FILTER_X + FIL_BTN_X + FIL_BTN_SHFT*2 - 13),(FILTER_Y + FILTER_SIZE_Y));
			GUI_SetColor(GUI_LIGHTBLUE);
			GUI_DispStringAt("500Hz",(FILTER_X + FIL_BTN_X + FIL_BTN_SHFT*1), (FILTER_Y + 3));
			break;

		case AUDIO_1P8KHZ:
			GUI_SetColor(GUI_WHITE);
			GUI_FillRect((FILTER_X + FIL_BTN_X + FIL_BTN_SHFT*2 - 2),FILTER_Y,(FILTER_X + FIL_BTN_X + FIL_BTN_SHFT*3 - 8),(FILTER_Y + FILTER_SIZE_Y));
			GUI_SetColor(GUI_LIGHTBLUE);
			GUI_DispStringAt("1.8kHz",(FILTER_X + FIL_BTN_X + FIL_BTN_SHFT*2), (FILTER_Y + 3));
			break;

		case AUDIO_2P3KHZ:
			GUI_SetColor(GUI_WHITE);
			GUI_FillRect((FILTER_X + FIL_BTN_X + FIL_BTN_SHFT*3 - 2),FILTER_Y,(FILTER_X + FIL_BTN_X + FIL_BTN_SHFT*4 - 8),(FILTER_Y + FILTER_SIZE_Y));
			GUI_SetColor(GUI_LIGHTBLUE);
			GUI_DispStringAt("2.3kHz",(FILTER_X + FIL_BTN_X + FIL_BTN_SHFT*3), (FILTER_Y + 3));
			break;

		case AUDIO_3P6KHZ:
			GUI_SetColor(GUI_WHITE);
			GUI_FillRect((FILTER_X + FIL_BTN_X + FIL_BTN_SHFT*4 - 2),FILTER_Y,(FILTER_X + FIL_BTN_X + FIL_BTN_SHFT*5 - 8),(FILTER_Y + FILTER_SIZE_Y));
			GUI_SetColor(GUI_LIGHTBLUE);
			GUI_DispStringAt("3.6kHz",(FILTER_X + FIL_BTN_X + FIL_BTN_SHFT*4), (FILTER_Y + 3));
			break;

		case AUDIO_WIDE:
			GUI_SetColor(GUI_WHITE);
			GUI_FillRect((FILTER_X + FIL_BTN_X + FIL_BTN_SHFT*5 - 2),FILTER_Y,(FILTER_X + FIL_BTN_X + FIL_BTN_SHFT*6 - 13),(FILTER_Y + FILTER_SIZE_Y));
			GUI_SetColor(GUI_LIGHTBLUE);
			GUI_DispStringAt("10kHz",(FILTER_X + FIL_BTN_X + FIL_BTN_SHFT*5), (FILTER_Y + 3));
			break;

		default:
			break;
	}

	// Save state
	loc_filter = filter;

	//--WRITE_EEPROM(EEP_CURFILTER,loc_filter);
}

//*----------------------------------------------------------------------------
//* Function Name       :
//* Object              :
//* Notes    			:
//* Notes   			:
//* Notes    			:
//* Context    			: CONTEXT_VIDEO
//*----------------------------------------------------------------------------
void ui_controls_filter_init(void)
{
	loc_filter = 0x99;

	// Fill full control
	GUI_SetColor(FIL_BKG_COLOR);
	GUI_FillRoundedRect(FILTER_X,FILTER_Y,(FILTER_X + FILTER_SIZE_X),(FILTER_Y + FILTER_SIZE_Y),2);
	GUI_SetColor(GUI_WHITE);

	// White frame
	GUI_DrawRoundedRect(FILTER_X,FILTER_Y,(FILTER_X + FILTER_SIZE_X),(FILTER_Y + FILTER_SIZE_Y),2);
	GUI_DrawRoundedRect(FILTER_X - 1,FILTER_Y - 1,(FILTER_X + FILTER_SIZE_X + 1),(FILTER_Y + FILTER_SIZE_Y + 1),2);

	// Control text background
	GUI_FillRoundedRect(FILTER_X,FILTER_Y,(FILTER_X + 36),(FILTER_Y + FILTER_SIZE_Y),2);

	GUI_SetColor(FIL_BKG_COLOR);
	GUI_SetFont(&GUI_Font20_1);

	// Control text
	GUI_DispStringAt("FIL", (FILTER_X + 4), (FILTER_Y + 3));

	// Initial load
	//tsu.curr_filter = tsu.dsp_filter;

	// Paint filters with selection
	ui_controls_filter_select();
}

//*----------------------------------------------------------------------------
//* Function Name       :
//* Object              :
//* Notes    			:
//* Notes   			:
//* Notes    			:
//* Context    			: CONTEXT_VIDEO
//*----------------------------------------------------------------------------
void ui_controls_filter_quit(void)
{

}

//*----------------------------------------------------------------------------
//* Function Name       :
//* Object              :
//* Notes    			:
//* Notes   			:
//* Notes    			:
//* Context    			: CONTEXT_VIDEO
//*----------------------------------------------------------------------------
void ui_controls_filter_touch(void)
{
	// Just a test, need proper touch scroll !!!
	//tsu.curr_filter++;
	//if(tsu.curr_filter > AUDIO_WIDE)
	//	tsu.curr_filter = AUDIO_300HZ;
}

//*----------------------------------------------------------------------------
//* Function Name       :
//* Object              :
//* Notes    			:
//* Notes   			:
//* Notes    			:
//* Context    			: CONTEXT_VIDEO
//*----------------------------------------------------------------------------
void ui_controls_filter_refresh(void)
{
	ui_controls_filter_select();
}

#endif
