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

#ifdef CONTEXT_VIDEO

#include "gui.h"
#include "dialog.h"

#include "codec_hw.h"
#include "ui_controls_dsp_stat.h"
#include "desktop\ui_controls_layout.h"

// Public radio state
extern struct	TRANSCEIVER_STATE_UI	tsu;

// DSP core state
extern struct 	TransceiverState 		ts;

uchar dsp_control_init_done = 0;
uchar skip_dsp_check = 0;
uchar dsp_version_done = 0;

//*----------------------------------------------------------------------------
//* Function Name       : ui_controls_dsp_stat_print
//* Object              :
//* Input Parameters    :
//* Output Parameters   :
//* Functions called    :
//*----------------------------------------------------------------------------
static void ui_controls_dsp_stat_print(void)
{
	skip_dsp_check++;
	if(skip_dsp_check < 4)
		return;

	skip_dsp_check = 0;

	if(!dsp_control_init_done)
	{
		// Destroy control
		GUI_SetColor(GUI_BLACK);
		GUI_FillRect(DSP_POS_X, DSP_POS_Y, (DSP_POS_X + DSP_POS_SIZE_X), (DSP_POS_Y + DSP_POS_SIZE_Y));

		// Create frame
		GUI_SetColor(GUI_LIGHTBLUE);

		// Blinker outline (double line)
		GUI_DrawRoundedRect(DSP_POS_X,		DSP_POS_Y + 16,	(DSP_POS_X + DSP_POS_SIZE_X    ), DSP_POS_Y + 30, 2);
		GUI_DrawRoundedRect(DSP_POS_X + 1,	DSP_POS_Y + 17,	(DSP_POS_X + DSP_POS_SIZE_X - 1), DSP_POS_Y + 29, 2);

		// Top rectangle (around 'DSP' text)
		GUI_FillRoundedRect(DSP_POS_X + 0,	DSP_POS_Y, (DSP_POS_X + DSP_POS_SIZE_X), (DSP_POS_Y + 15), 2);

		// Bottom rectangle
		GUI_FillRoundedRect(DSP_POS_X + 0,	(DSP_POS_Y + 31), (DSP_POS_X + DSP_POS_SIZE_X), (DSP_POS_Y + DSP_POS_SIZE_Y),2);

		// Outside frame
		GUI_SetColor(GUI_WHITE);
		GUI_DrawRoundedRect(DSP_POS_X, DSP_POS_Y, (DSP_POS_X + DSP_POS_SIZE_X), (DSP_POS_Y + DSP_POS_SIZE_Y), 2);
		GUI_DrawRoundedRect(DSP_POS_X + 1, DSP_POS_Y + 1, (DSP_POS_X + DSP_POS_SIZE_X - 1), (DSP_POS_Y + DSP_POS_SIZE_Y - 1), 2);

		// Horizontal divider
		GUI_DrawHLine((DSP_POS_Y + DSP_POS_SIZE_Y - 18), DSP_POS_X, (DSP_POS_X + DSP_POS_SIZE_X));

		dsp_control_init_done = 1;
	}

	if(!tsu.dsp_alive)
		GUI_SetColor(GUI_GRAY);
	else
		GUI_SetColor(GUI_WHITE);

	// Print text
	GUI_SetFont(&GUI_Font8x16_1);
	GUI_DispStringAt("DSP",	DSP_POS_X + 12,	DSP_POS_Y + 2);

	// No blinking
	if(!tsu.dsp_alive)
		return;

	// Create blinker
	if(tsu.dsp_blinker)
		GUI_SetColor(GUI_RED);
	else
		GUI_SetColor(GUI_BLACK);

	GUI_FillRoundedRect((DSP_POS_X + 7), (DSP_POS_Y + 20), (DSP_POS_X + 40), (DSP_POS_Y + 25), 2);

	// Sampling rate(bottom text)
	// ToDo: get from DSP and dynamically refresh on change
	GUI_SetColor(GUI_WHITE);
	GUI_SetFont(&GUI_Font8x16_1);

	switch(ts.samp_rate)
	{
		case SAI_AUDIO_FREQUENCY_48K:
			GUI_DispStringAt("48k", (DSP_POS_X + 13), (DSP_POS_Y + DSP_POS_SIZE_Y - 16));
			break;
		case SAI_AUDIO_FREQUENCY_96K:
			GUI_DispStringAt("96k", (DSP_POS_X + 13), (DSP_POS_Y + DSP_POS_SIZE_Y - 16));
			break;
		case SAI_AUDIO_FREQUENCY_192K:
			GUI_DispStringAt("192k", (DSP_POS_X + 8), (DSP_POS_Y + DSP_POS_SIZE_Y - 16));
			break;
		default:
			GUI_DispStringAt("NA", (DSP_POS_X + 13), (DSP_POS_Y + DSP_POS_SIZE_Y - 16));
			break;
	}

	// DSP firmware version
	if((dsp_version_done == 0) && (tsu.dsp_alive) && ((tsu.dsp_rev3 != 0) || (tsu.dsp_rev4 != 0)))
	{
		char   	buff[20];

		GUI_SetColor(GUI_BLACK);
		GUI_SetFont(&GUI_Font8x8_1);
		sprintf(buff,"%d.%d",tsu.dsp_rev3,tsu.dsp_rev4);
		GUI_DispStringAt(buff,(DSP_POS_X + 9), (DSP_POS_Y + DSP_POS_SIZE_Y - 30));

		dsp_version_done = 1;
	}
}

//*----------------------------------------------------------------------------
//* Function Name       :
//* Object              :
//* Notes    			:
//* Notes   			:
//* Notes    			:
//* Context    			: CONTEXT_VIDEO
//*----------------------------------------------------------------------------
void ui_controls_dsp_stat_init(void)
{
	dsp_control_init_done = 0;
	dsp_version_done      = 0;
	ui_controls_dsp_stat_print();
}

//*----------------------------------------------------------------------------
//* Function Name       :
//* Object              :
//* Notes    			:
//* Notes   			:
//* Notes    			:
//* Context    			: CONTEXT_VIDEO
//*----------------------------------------------------------------------------
void ui_controls_dsp_stat_quit(void)
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
void ui_controls_dsp_stat_touch(void)
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
void ui_controls_dsp_stat_refresh(void)
{
	ui_controls_dsp_stat_print();
}

#endif
