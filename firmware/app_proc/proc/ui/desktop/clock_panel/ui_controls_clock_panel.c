/************************************************************************************
**                                                                                 **
**                                 mcHF QRP Transceiver                            **
**                         Krassi Atanassov - M0NKA, 2013-2026                     **
**                                                                                 **
**---------------------------------------------------------------------------------**
**                                                                                 **
**  File name:                                                                     **
**  Description:                                                                   **
**  Last Modified:                                                                 **
**  Licence:			https://github.com/m0nka/mcHF/blob/main/LICENSE            **
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

#ifdef CONTEXT_GPS
#include "gps_proc.h"
#endif

RTC_DateTypeDef sdatestructureget;
RTC_TimeTypeDef stimestructureget;

// Public radio state
extern struct	TRANSCEIVER_STATE_UI	tsu;

// DSP core state
extern struct 	TransceiverState 		ts;

// FreeRTOS process state
extern struct PROC_STATE 				ps;

uchar ui_dsp_control_init_done 	= 0;
uchar ui_dsp_version_done 		= 0;
uchar loc_fix_mode 				= 0xFF;
short loc_nco_freq 				= 0xFFFF;

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
	if((clock_timer + 900) > ps.epoch)
		return;							// Wait
	else if(clock_timer == 0)
		clock_timer = ps.epoch;			// Init timer
	else
		clock_timer = ps.epoch;			// Reset timer

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
	//..

	// Clear sats area
	GUI_SetColor(CLOCK_PANEL_COL);
	GUI_FillRect(	10,
					(CLOCK_Y + 12),
					20,
					(CLOCK_Y + 25));

	uchar s_cnt = gps_proc_sats_cnt();

	// Temp, show sats count
	sprintf(buf,"%d", s_cnt);

	if(gps_proc_time_set())
		GUI_SetColor(GUI_DARKGREEN);
	else
		GUI_SetColor(GUI_DARKRED);

	GUI_SetFont(&GUI_Font16B_ASCII);
	GUI_DispStringAt(buf, 10, (CLOCK_Y + 12));

	// Temp, refresh data(ToDo: need a better way)
	//if(s_cnt > 5)
	//	ui_controls_clock_init();
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

		GUI_FillRect(380, 192, 390, 198);

		old_blinker = tsu.dsp_blinker;
	}

	if(ps.epoch < (blink_timer + 800))
		return;
	else if(blink_timer == 0)
		blink_timer = ps.epoch;
	else
		blink_timer = ps.epoch;

	if(uc_keep_flag)
		GUI_SetColor(GUI_DARKGREEN);
	else
		GUI_SetColor(CLOCK_PANEL_COL);

	GUI_FillRect(365, 192, 375, 198);

	uc_keep_flag = !uc_keep_flag;
}

static void ui_controls_clock_panel_dsp_details(void)
{
	char   	buff[20];

	if(ui_dsp_control_init_done == 0)
	{
		// Sampling rate(bottom text)
		// ToDo: get from DSP and dynamically refresh on change
		GUI_SetColor(GUI_BLACK);
		GUI_SetFont(&GUI_Font8x8_1);

		switch(ts.samp_rate)
		{
			case SAI_AUDIO_FREQUENCY_48K:
				GUI_DispStringAt("48k", 410, 192);
				break;
			case SAI_AUDIO_FREQUENCY_96K:
				GUI_DispStringAt("96k", 410, 192);
				break;
			case SAI_AUDIO_FREQUENCY_192K:
				GUI_DispStringAt("192k", 410, 192);
				break;
			default:
				GUI_DispStringAt("NA", 410, 192);
				break;
		}
		ui_dsp_control_init_done = 1;
	}

	// DSP firmware version
	#if 0
	if((ui_dsp_version_done == 0) && (tsu.dsp_alive) && ((tsu.dsp_rev3 != 0) || (tsu.dsp_rev4 != 0)))
	{
		GUI_SetColor(HOT_PINK);
		GUI_SetFont(&GUI_Font8x8_1);
		sprintf(buff,"%d.%d",tsu.dsp_rev3,tsu.dsp_rev4);
		GUI_DispStringAt(buff,448, 192);

		ui_dsp_version_done = 1;
	}
	#endif

	// Show VFO mode changes on panel
	if(loc_fix_mode != tsu.band[tsu.curr_band].fixed_mode)
	{
		GUI_SetColor(CLOCK_PANEL_COL);
		//GUI_SetColor(GUI_WHITE);
		GUI_FillRect(448, 192, 500, 198);	//498-550

		GUI_SetColor(GUI_BLUE);
		GUI_SetFont(&GUI_Font8x8_1);

		//printf("fix mode change \r\n");
		if(tsu.band[tsu.curr_band].fixed_mode == 1)
			GUI_DispStringAt("Fixed  ", 450, 192);	// 500
		else
			GUI_DispStringAt("Centre ", 450, 192);

		loc_fix_mode = tsu.band[tsu.curr_band].fixed_mode;
	}

	// Show NCO frequency changes on panel
	if(loc_nco_freq != tsu.band[tsu.curr_band].nco_freq)
	{
		GUI_SetColor(CLOCK_PANEL_COL);
		//GUI_SetColor(GUI_WHITE);
		GUI_FillRect(508, 192, 580, 198);	// 558 - 630

		GUI_SetColor(GUI_DARKRED);
		GUI_SetFont(&GUI_Font8x8_1);

		sprintf(buff,"%dHz", tsu.band[tsu.curr_band].nco_freq);
		GUI_DispStringAt(buff, 510, 192);	// 560

		loc_nco_freq = tsu.band[tsu.curr_band].nco_freq;
	}
}

static void ui_controls_clock_panel_btm_part(void)
{
	GUI_SetColor(CLOCK_PANEL_COL);

	#ifndef STARTEK_43INCH
	GUI_FillRoundedRect(2, 188, 849, 203, 3);
	#else
	GUI_FillRoundedRect(2, 188, 799, 203, 3);
	#endif
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

void ui_controls_clock_show_notification(ulong lora_data)
{
	struct LORA_PACKET_RX lprx;
	char   buff[40];

	// Valid ptr ?
	if(lora_data == 0)
		return;

	// Copy, as sender used temp stack
	memcpy(&lprx, (LORA_PACKET_RX *)lora_data, sizeof(LORA_PACKET_RX));
	//printf("packet type: %d \r\n", lprx.mesh_id);

	// -------------------------------------
	// Packet type
	GUI_SetColor(CLOCK_PANEL_COL);
	GUI_FillRect(570, 190, 590, 200);
	GUI_SetColor(GUI_DARKGREEN);
	GUI_SetFont(&GUI_Font8x8_1);

	if(lprx.mesh_id == MESH_ID_MC)
		GUI_DispStringAt("MC", 572, 192);
	else if(lprx.mesh_id == MESH_ID_MT)
		GUI_DispStringAt("MT", 572, 192);
	else
		GUI_DispStringAt("NA", 572, 192);

	// -------------------------------------
	// Signal info - power
	GUI_SetColor(CLOCK_PANEL_COL);
	GUI_FillRect(590, 190, 660, 200);
	GUI_SetColor(GUI_RED);
	GUI_SetFont(&GUI_Font8x8_1);

	sprintf(buff, "%sdBm", lprx.sig_pwr);
	GUI_DispStringAt(buff, 592, 192);

	// -------------------------------------
	// Signal info - SNR
	GUI_SetColor(CLOCK_PANEL_COL);
	GUI_FillRect(670, 190, 740, 200);
	GUI_SetColor(GUI_DARKGREEN);
	GUI_SetFont(&GUI_Font8x8_1);

	sprintf(buff, "%sdB", lprx.sig_snr);
	GUI_DispStringAt(buff, 672, 192);

	// -------------------------------------
	// Message type
	GUI_SetColor(CLOCK_PANEL_COL);
	GUI_FillRect(750, 190, 796, 200);
	GUI_SetColor(GUI_BLACK);
	GUI_SetFont(&GUI_Font8x8_1);

	GUI_DispStringAt(lprx.msg_type, 752, 192);
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
	// Init publics for this module
	ui_dsp_control_init_done	= 0;
	ui_dsp_version_done 		= 0;
	loc_fix_mode 				= 0xFF;
	loc_nco_freq 				= 0xFFFF;

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

