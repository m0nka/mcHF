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

#include "ui_controls_clock_panel.h"
#include "desktop\ui_controls_layout.h"

#include "rtc.h"
#include "ui_actions.h"

RTC_DateTypeDef sdatestructureget;
RTC_TimeTypeDef stimestructureget;

// Public radio state
extern struct	TRANSCEIVER_STATE_UI	tsu;

// DSP core state
extern struct 	TransceiverState 		ts;

extern ulong 	epoch;

//*----------------------------------------------------------------------------
//* Function Name       : ui_controls_clock_init
//* Object              :
//* Notes    			: only the clock
//* Notes   			:
//* Notes    			:
//* Context    			: CONTEXT_VIDEO
//*----------------------------------------------------------------------------
static void ui_controls_clock_init(void)
{
	char buf[20];

	// Get time
	k_GetTime(&stimestructureget);
	k_GetDate(&sdatestructureget);

	// Time Unit
	GUI_SetColor(CLOCK_COLOR);
	GUI_SetFont(&CLOCK_FONT);

	// Create hours/minutes
	sprintf(buf,"%02d:%02d:%02dz",stimestructureget.Hours, stimestructureget.Minutes,stimestructureget.Seconds);
	GUI_DispStringAt(buf,(CLOCK_X + CLOCK_HOURS_SHIFT), (CLOCK_Y + 2));

	// Create date
	uchar year = sdatestructureget.Year;
	sprintf(buf,"%02d/%02d/%04d",sdatestructureget.Date,sdatestructureget.Month, (year + 2000));
	GUI_DispStringAt(buf,(CLOCK_X + CLOCK_DATES_SHIFT), (CLOCK_Y + 2));
}

//*----------------------------------------------------------------------------
//* Function Name       : ui_controls_clock_refresh
//* Object              :
//* Notes    			: only the clock
//* Notes   			:
//* Notes    			:
//* Context    			: CONTEXT_VIDEO
//*----------------------------------------------------------------------------
static void ui_controls_clock_refresh(void)
{
	char buf[20];
	static ulong clock_timer = 0;

	// Update every 900mS
	if((clock_timer + 900) > epoch)
		return;							// Wait
	else if(clock_timer == 0)
		clock_timer = epoch;			// Init timer
	else
		clock_timer = epoch;			// Reset timer

	// Dump state from RTC
	k_GetTime(&stimestructureget);
	k_GetDate(&sdatestructureget);

	// Clear seconds area
	GUI_SetColor(CLOCK_PANEL_COL);
	GUI_FillRect(	(CLOCK_X + CLOCK_HOURS_SHIFT),
					(CLOCK_Y + 5),
					(CLOCK_X + CLOCK_HOURS_SHIFT + 125),
					(CLOCK_Y + 28));

	GUI_SetColor(CLOCK_COLOR);
	GUI_SetFont(&CLOCK_FONT);

	sprintf(buf,"%02d:%02d:%02dz",stimestructureget.Hours, stimestructureget.Minutes,stimestructureget.Seconds);
	GUI_SetColor(CLOCK_COLOR);
	GUI_DispStringAt(buf,(CLOCK_X + CLOCK_HOURS_SHIFT), (CLOCK_Y + 2));

	// ToDo: Check if date changed, then update...
}

//*----------------------------------------------------------------------------
//* Function Name       : ui_controls_clock_panel_show_alive
//* Object              : create blinking mark to show OS is still running
//* Input Parameters    :
//* Output Parameters   :
//* Functions called    : simple software trigger with timeout, non blocking
//*----------------------------------------------------------------------------
static void ui_controls_clock_panel_show_alive(void)
{
	static uchar old_blinker 	= 0xFF;
	static uchar uc_keep_flag	= 0;
	static ulong blink_timer	= 0;

	// DSP Blinker
	if(old_blinker != tsu.dsp_blinker)
	{
		if(tsu.dsp_blinker)
			GUI_SetColor(HOT_PINK);
		else
			GUI_SetColor(CLOCK_PANEL_COL);

		GUI_FillRect(385, 192, 395, 198);

		old_blinker = tsu.dsp_blinker;
	}

	if(epoch < (blink_timer + 800))
		return;
	else if(blink_timer == 0)
		blink_timer = epoch;
	else
		blink_timer = epoch;

	if(uc_keep_flag)
		GUI_SetColor(GUI_DARKGREEN);
	else
		GUI_SetColor(CLOCK_PANEL_COL);

	GUI_FillRect(370, 192, 380, 198);

	uc_keep_flag = !uc_keep_flag;
}


static void ui_controls_clock_panel_dsp_details(void)
{
	static uchar dsp_control_init_done = 0;
	static uchar dsp_version_done = 0;

	if(dsp_control_init_done == 0)
	{
		// Sampling rate(bottom text)
		// ToDo: get from DSP and dynamically refresh on change
		GUI_SetColor(GUI_BLACK);
		GUI_SetFont(&GUI_Font8x8_1);

		switch(ts.samp_rate)
		{
			case SAI_AUDIO_FREQUENCY_48K:
				GUI_DispStringAt("48k", 420, 192);
				break;
			case SAI_AUDIO_FREQUENCY_96K:
				GUI_DispStringAt("96k", 420, 192);
				break;
			case SAI_AUDIO_FREQUENCY_192K:
				GUI_DispStringAt("192k", 420, 192);
				break;
			default:
				GUI_DispStringAt("NA", 420, 192);
				break;
		}
		dsp_control_init_done = 1;
	}

	// DSP firmware version
	if((dsp_version_done == 0) && (tsu.dsp_alive) && ((tsu.dsp_rev3 != 0) || (tsu.dsp_rev4 != 0)))
	{
		char   	buff[20];

		GUI_SetColor(GUI_BLACK);
		GUI_SetFont(&GUI_Font8x8_1);
		sprintf(buff,"%d.%d",tsu.dsp_rev3,tsu.dsp_rev4);
		GUI_DispStringAt(buff,460, 192);

		dsp_version_done = 1;
	}

}

static void ui_controls_clock_panel_btm_part(void)
{
	GUI_SetColor(CLOCK_PANEL_COL);
	GUI_FillRoundedRect(2, 188, 849, 203, 3);
}

static void ui_controls_clock_panel_top_part(void)
{
	GUI_SetColor(CLOCK_PANEL_COL);

	// Top rectangle
	GUI_FillRoundedRect(2, 148, 352, 200, 3);

	// Internal rounded corner
	GUI_FillRoundedRect(351, 180, 357, 189, 3);
	GUI_SetColor(GUI_BLACK);
	GUI_FillRoundedRect(353, 178, 359, 187, 3);

	// Top Line
	GUI_SetColor(HOT_PINK);
	GUI_FillRoundedRect(30, 156, 326, 160, 1);
}

//*----------------------------------------------------------------------------
//* Function Name       : ui_controls_clock_panel_restore
//* Object              :
//* Notes    			: restore clock panel quicky if overwritten by S-meter
//* Notes   			:
//* Notes    			:
//* Context    			: CONTEXT_VIDEO
//*----------------------------------------------------------------------------
void ui_controls_clock_panel_restore(void)
{
	char buf[20];

	k_GetTime(&stimestructureget);
	k_GetDate(&sdatestructureget);

	ui_controls_clock_panel_top_part();

	GUI_SetColor(CLOCK_COLOR);
	GUI_SetFont(&CLOCK_FONT);

	sprintf(buf,"%02d:%02d:%02dz",stimestructureget.Hours, stimestructureget.Minutes,stimestructureget.Seconds);
	GUI_SetColor(CLOCK_COLOR);
	GUI_DispStringAt(buf,(CLOCK_X + CLOCK_HOURS_SHIFT), (CLOCK_Y + 2));

	uchar year = sdatestructureget.Year;
	sprintf(buf,"%02d/%02d/%04d",sdatestructureget.Date,sdatestructureget.Month, (year + 2000));
	GUI_DispStringAt(buf,(CLOCK_X + CLOCK_DATES_SHIFT), (CLOCK_Y + 2));
}

//*----------------------------------------------------------------------------
//* Function Name       : ui_controls_clock_panel_refresh
//* Object              :
//* Notes    			: whole panel repaint
//* Notes   			:
//* Notes    			:
//* Context    			: CONTEXT_VIDEO
//*----------------------------------------------------------------------------
void ui_controls_clock_panel_refresh(void)
{
	// Clock refresh
	ui_controls_clock_refresh();

	// CPU/DSP status blinkers
	ui_controls_clock_panel_show_alive();

	// DSP details
	ui_controls_clock_panel_dsp_details();
}

//*----------------------------------------------------------------------------
//* Function Name       : ui_controls_clock_panel_init
//* Object              :
//* Notes    			: whole panel init
//* Notes   			:
//* Notes    			:
//* Context    			: CONTEXT_VIDEO
//*----------------------------------------------------------------------------
void ui_controls_clock_panel_init(void)
{
	// Bottom part of panel
	ui_controls_clock_panel_btm_part();

	// Left top part of panel
	ui_controls_clock_panel_top_part();

	// Init clock
	ui_controls_clock_init();
}

//*----------------------------------------------------------------------------
//* Function Name       : ui_controls_clock_panel_quit
//* Object              :
//* Notes    			:
//* Notes   			:
//* Notes    			:
//* Context    			: CONTEXT_VIDEO
//*----------------------------------------------------------------------------
void ui_controls_clock_panel_quit(void)
{

}

#endif

