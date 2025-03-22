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

#include "ui_controls_clock.h"
#include "desktop\ui_controls_layout.h"

//#include "stm32h7xx_hal_rtc.h"
#include "rtc.h"
#include "ui_actions.h"

#define CLOCK_UNITS_SHIFT 	10
#define CLOCK_HOURS_SHIFT 	0
#define CLOCK_SECND_SHIFT 	62
#define CLOCK_DATES_SHIFT 	130
#define CLOCK_LOCKS_SHIFT 	280

#define CLOCK_COLOR			GUI_LIGHTGRAY

RTC_DateTypeDef sdatestructureget;
RTC_TimeTypeDef stimestructureget;

uchar time_skip 	 = 0;
uchar loc_seconds 	 = 0;
uchar loc_minutes 	 = 0;
uchar date_shown 	 = 0;
uchar utc_active_req = 1;
uchar lock_type		 = 0;

//extern RTC_HandleTypeDef RtcHandle;

// Public radio state
extern struct	TRANSCEIVER_STATE_UI	tsu;

static void ui_controls_clock_reload_time(void)
{
	// Still waiting msg response ?
	if(utc_active_req)
	{
		uchar t_buff[10];

		//printf("wait...\r\n");

		if(ui_actions_ipc_msg(0, 6, t_buff) == 0)
		{
			//print_hex_array(t_buff + 1, 6);

			if((t_buff[0] == 1)&&(t_buff[6] >= 0x79))	// success on NTP call/time valid
			{
				RTC_TimeTypeDef Time;
				RTC_DateTypeDef Date;

				//printf("will set UTC\r\n");

				Time.Hours = t_buff[1];
				Time.Minutes = t_buff[2];
				Time.Seconds = t_buff[3];
				Time.StoreOperation = 0;
				Time.SubSeconds = 0;
				Time.DayLightSaving = 0;
				//HAL_RTC_SetTime(&RtcHandle, &Time, RTC_FORMAT_BIN);
				k_SetTime(&Time);

				ulong c_year = t_buff[6] + 1900;
				c_year -= 2000;
				//printf("year - calc: %d\r\n", c_year);

				Date.Date 	= t_buff[4];
				Date.Month 	= t_buff[5];
				Date.Year 	= c_year;
				//HAL_RTC_SetDate(&RtcHandle, &Date, RTC_FORMAT_BIN);
				k_SetDate(&Date);

				lock_type = 1;
			}

			// Disable request
			utc_active_req = 0;
		}
	}

	// -- Read both, keep order!!!! Some ST bug relating to read of RTC --
	//HAL_RTC_GetTime(&RtcHandle, &stimestructureget, RTC_FORMAT_BIN);
	//HAL_RTC_GetDate(&RtcHandle, &sdatestructureget, RTC_FORMAT_BIN);
	//printf("%02d:%02d:%2d\r\n", stimestructureget.Hours, stimestructureget.Minutes, stimestructureget.Seconds);
	k_GetTime(&stimestructureget);
	k_GetDate(&sdatestructureget);
}

#if 0
//*----------------------------------------------------------------------------
//* Function Name       : ui_controls_clock_restore
//* Object              :
//* Notes    			: special call from s-meter auto device
//* Notes   			: to restore control after deletion by needle repaint
//* Notes    			:
//* Context    			: CONTEXT_VIDEO
//*----------------------------------------------------------------------------
void ui_controls_clock_restore(void)
{
	char buf[20];

	// Frame around clock
	//GUI_SetColor(GUI_WHITE);
	//GUI_DrawRect(CLOCK_X, CLOCK_Y, (CLOCK_X + CLOCK_SIZE_X), (CLOCK_Y + CLOCK_SIZE_Y));

	// Time Unit
	GUI_SetColor(GUI_LIGHTRED);
	GUI_SetFont(&GUI_Font8x16_1);
	GUI_DispStringAt("UTC",(CLOCK_X + CLOCK_UNITS_SHIFT), (CLOCK_Y + 2));

	// Clear lock options
	GUI_SetColor(GUI_BLACK);
	GUI_FillRect((CLOCK_X + CLOCK_LOCKS_SHIFT), (CLOCK_Y + 7), (CLOCK_X + CLOCK_LOCKS_SHIFT + 60), (CLOCK_Y + 15));

	// Lock options
	GUI_SetColor(GUI_LIGHTRED);
	GUI_SetFont(&GUI_Font8x16_1);

	if(lock_type == 0)
		GUI_DispStringAt("Lock:RTC", (CLOCK_X + CLOCK_LOCKS_SHIFT), (CLOCK_Y + 2));
	else
		GUI_DispStringAt("Lock:NTP", (CLOCK_X + CLOCK_LOCKS_SHIFT), (CLOCK_Y + 2));

	// Create hours/minutes
	sprintf(buf,"%02d:%02d",stimestructureget.Hours, stimestructureget.Minutes);
	GUI_SetColor(GUI_LIGHTRED);
	GUI_DispStringAt(buf,(CLOCK_X + CLOCK_HOURS_SHIFT), (CLOCK_Y + 2));

	// Create seconds area
	sprintf(buf,"%02d",stimestructureget.Seconds);
	GUI_SetColor(GUI_LIGHTRED);
	GUI_SetFont(&GUI_Font8x8_1);
	GUI_DispStringAt(buf,(CLOCK_X + CLOCK_SECND_SHIFT), (CLOCK_Y + 7));

	// Create date
	GUI_SetFont(&GUI_Font8x16_1);
	GUI_SetColor(GUI_LIGHTBLUE);
	uchar year = sdatestructureget.Year;
	sprintf(buf,"%02d/%02d/%04d",sdatestructureget.Date,sdatestructureget.Month, (year + 2000));
	GUI_DispStringAt(buf,(CLOCK_X + CLOCK_DATES_SHIFT), (CLOCK_Y + 2));
}
#endif

//*----------------------------------------------------------------------------
//* Function Name       :
//* Object              :
//* Notes    			:
//* Notes   			:
//* Notes    			:
//* Context    			: CONTEXT_VIDEO
//*----------------------------------------------------------------------------
void ui_controls_clock_refresh(void)
{
	char buf[20];

	time_skip++;

	// Adjust skip interval, so we don't miss seconds
	if(time_skip < 8)
		return;

	time_skip = 0;

	// Dump state from RTC
	ui_controls_clock_reload_time();

	// Update seconds
	if(loc_seconds != stimestructureget.Seconds)
	{
		//printf("sec update\r\n");

		// Clear seconds area
		GUI_SetColor(GUI_BLACK);
		GUI_FillRect((CLOCK_X + CLOCK_SECND_SHIFT), (CLOCK_Y + 5), (CLOCK_X + CLOCK_SECND_SHIFT + 22), (CLOCK_Y + 22));

		sprintf(buf,"%02d",stimestructureget.Seconds);

		// Update seconds area
		GUI_SetColor(CLOCK_COLOR);
		GUI_SetFont(&GUI_Font24B_ASCII);
		GUI_DispStringAt(buf,(CLOCK_X + CLOCK_SECND_SHIFT), (CLOCK_Y + 2));

		// Save to local
		loc_seconds = stimestructureget.Seconds;
	}

	// Update minutes
	if(loc_minutes != stimestructureget.Minutes)
	{
		// Clear hour/min area
		GUI_SetColor(GUI_BLACK);
		GUI_FillRect((CLOCK_X + CLOCK_HOURS_SHIFT), (CLOCK_Y + 5), (CLOCK_X + CLOCK_HOURS_SHIFT + 52), (CLOCK_Y + 22));	// delete time

		GUI_SetFont(&GUI_Font24B_ASCII);
		GUI_SetColor(CLOCK_COLOR);

		sprintf(buf,"%02d:%02d",stimestructureget.Hours, stimestructureget.Minutes);
		GUI_DispStringAt(buf,(CLOCK_X + CLOCK_HOURS_SHIFT), (CLOCK_Y + 2));

		// Save to local
		loc_minutes = stimestructureget.Minutes;
	}
return;
	if((utc_active_req == 0)&&(!date_shown))
	{
		// Clear date area
		GUI_SetColor(GUI_BLACK);
		GUI_FillRect((CLOCK_X + CLOCK_DATES_SHIFT),(CLOCK_Y + 2),(CLOCK_X + CLOCK_DATES_SHIFT + 80), (CLOCK_Y + 16));	// delete date

		uchar year = sdatestructureget.Year;
		//printf("year - read: %d\r\n", year);

		GUI_SetFont(&GUI_Font24B_ASCII);
		GUI_SetColor(CLOCK_COLOR);

		sprintf(buf,"%02d/%02d/%04d",sdatestructureget.Date,sdatestructureget.Month, (year + 2000));
		GUI_DispStringAt(buf,(CLOCK_X + CLOCK_DATES_SHIFT), (CLOCK_Y + 2));

		date_shown = 1;
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
void ui_controls_clock_init(void)
{
	char buf[20];

	//HAL_RTC_GetTime(&RtcHandle, &stimestructureget, RTC_FORMAT_BIN);
	//HAL_RTC_GetDate(&RtcHandle, &sdatestructureget, RTC_FORMAT_BIN);
	k_GetTime(&stimestructureget);
	k_GetDate(&sdatestructureget);

	time_skip   = 0;
	loc_seconds = 0;
	loc_minutes = 0;

	// Frame around clock
	//GUI_SetColor(GUI_WHITE);
	//GUI_DrawRect(CLOCK_X, CLOCK_Y, (CLOCK_X + CLOCK_SIZE_X), (CLOCK_Y + CLOCK_SIZE_Y));

	// Time Unit
	//GUI_SetColor(GUI_LIGHTRED);
	GUI_SetFont(&GUI_Font24B_ASCII);
	//GUI_DispStringAt("UTC",(CLOCK_X + CLOCK_UNITS_SHIFT), (CLOCK_Y + 2));

	// Lock options
	//GUI_SetColor(GUI_LIGHTRED);
	//GUI_SetFont(&GUI_Font8x16_1);

	//if(lock_type == 0)
	//	GUI_DispStringAt("Lock:RTC", (CLOCK_X + CLOCK_LOCKS_SHIFT), (CLOCK_Y + 2));
	//else
	//	GUI_DispStringAt("Lock:NTP", (CLOCK_X + CLOCK_LOCKS_SHIFT), (CLOCK_Y + 2));

	// Create hours/minutes
	sprintf(buf,"%02d:%02d:%02dz",stimestructureget.Hours, stimestructureget.Minutes,stimestructureget.Seconds);
	GUI_SetColor(CLOCK_COLOR);
	GUI_DispStringAt(buf,(CLOCK_X + CLOCK_HOURS_SHIFT), (CLOCK_Y + 2));

	// Create seconds area
	//sprintf(buf,"%02d",stimestructureget.Seconds);
	//GUI_SetColor(GUI_LIGHTRED);
	////GUI_SetFont(&GUI_Font8x8_1);
	//GUI_DispStringAt(buf,(CLOCK_X + CLOCK_SECND_SHIFT), (CLOCK_Y + 7));

	// Create date
	//GUI_SetFont(&GUI_Font8x16_1);
	//GUI_SetColor(GUI_LIGHTBLUE);
	uchar year = sdatestructureget.Year;
	sprintf(buf,"%02d/%02d/%04d",sdatestructureget.Date,sdatestructureget.Month, (year + 2000));
	GUI_DispStringAt(buf,(CLOCK_X + CLOCK_DATES_SHIFT), (CLOCK_Y + 2));

	//if(year < 21)
	//	ui_actions_ipc_msg(1, 6, NULL);	// get UTC
	//else
	//	printf("== RTC time looks good ==\r\n");
}

//*----------------------------------------------------------------------------
//* Function Name       :
//* Object              :
//* Notes    			:
//* Notes   			:
//* Notes    			:
//* Context    			: CONTEXT_VIDEO
//*----------------------------------------------------------------------------
void ui_controls_clock_quit(void)
{

}

#endif

