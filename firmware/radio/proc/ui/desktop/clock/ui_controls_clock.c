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

#include "rtc.h"
#include "ui_actions.h"

#define CLOCK_UNITS_SHIFT 	10
#define CLOCK_HOURS_SHIFT 	0
#define CLOCK_SECND_SHIFT 	62
#define CLOCK_DATES_SHIFT 	160
#define CLOCK_LOCKS_SHIFT 	280

#define CLOCK_COLOR			GUI_ORANGE
#define CLOCK_FONT			GUI_Font32B_ASCII

RTC_DateTypeDef sdatestructureget;
RTC_TimeTypeDef stimestructureget;

// Public radio state
extern struct	TRANSCEIVER_STATE_UI	tsu;

extern ulong 	epoch;

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
	GUI_SetColor(GUI_BLACK);
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

