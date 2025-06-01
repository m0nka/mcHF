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

