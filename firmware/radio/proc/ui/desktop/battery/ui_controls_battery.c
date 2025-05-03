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

#include "ui_proc.h"
#include "ui_controls_battery.h"
#include "desktop\ui_controls_layout.h"

#include "bms_proc.h"

// Public radio state
extern struct	TRANSCEIVER_STATE_UI	tsu;

#ifdef CONTEXT_BMS
extern struct BMSState	bmss;
#endif

uchar curr_batt_value = 0;

static void ui_controls_battery_progress(uchar val)
{
	char buf[10];

	if(val > 100)
		val = 100;

	// Clear
	GUI_SetColor(GUI_WHITE);
	GUI_FillRoundedRect((BATTERY_X + 2),(BATTERY_Y + 5),(BATTERY_X + BATTERY_SIZE_X - 1),(BATTERY_Y + BATTERY_SIZE_Y - 0),2);

	int y0 = BATTERY_Y + BATTERY_SIZE_Y + 5 - 50;//val/2;	// top
	int y1 = BATTERY_Y + BATTERY_SIZE_Y - 0;			// bottom

	//if(bmss.run_on_dc)
	//	GUI_SetColor(GUI_MAGENTA);
	//else if(val < 25)
	//	GUI_SetColor(GUI_LIGHTRED);
	//else
		GUI_SetColor(APPLE_MAC_GREY);

	// Update
	GUI_FillRect((BATTERY_X + 2), y0,(BATTERY_X + BATTERY_SIZE_X - 1), y1);

	sprintf(buf, "%d%%", val);
	int x  = BATTERY_X + BATTERY_SIZE_X/2 - 10;

	if(val < 10)
		x  += 9;
	else if(val < 100)
		x += 5;

	// Text
	GUI_SetColor(GUI_BLACK);
	GUI_SetFont(&GUI_Font20_ASCII);

	#ifdef CONTEXT_BMS
	GUI_DispStringAt(buf, x - 8,  BATTERY_Y + BATTERY_SIZE_Y - 18);

	if(bmss.mins)
	{
		if(bmss.mins > 36000)
			bmss.mins = 36000;

		sprintf(buf, "%dh%dm", bmss.mins/60, bmss.mins%60);
		GUI_DispStringAt(buf, x - 26, BATTERY_Y + BATTERY_SIZE_Y - 42);
	}
	else
		GUI_DispStringAt("NA", x - 6, BATTERY_Y + BATTERY_SIZE_Y - 42);
	#else
	GUI_DispStringAt("offline", x - 12, BATTERY_Y + BATTERY_SIZE_Y - 33);
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
void ui_controls_battery_init(void)
{
	curr_batt_value = 0;

	// Two pixel frame
	GUI_SetColor(HOT_PINK);
	GUI_DrawRoundedRect((BATTERY_X +  0),(BATTERY_Y + 3),(BATTERY_X + BATTERY_SIZE_X),    (BATTERY_Y + BATTERY_SIZE_Y + 1),2);
	GUI_DrawRoundedRect((BATTERY_X +  1),(BATTERY_Y + 4),(BATTERY_X + BATTERY_SIZE_X + 1),(BATTERY_Y + BATTERY_SIZE_Y + 2),2);

	// Terminal
	GUI_FillRect(	(BATTERY_X + BATTERY_SIZE_X),
					(BATTERY_Y + BATTERY_SIZE_Y/2 - 5),
					(BATTERY_X + BATTERY_SIZE_X + 6),
					(BATTERY_Y + BATTERY_SIZE_Y/2 + 11));

	ui_controls_battery_progress(100);
}

//*----------------------------------------------------------------------------
//* Function Name       :
//* Object              :
//* Notes    			:
//* Notes   			:
//* Notes    			:
//* Context    			: CONTEXT_VIDEO
//*----------------------------------------------------------------------------
void ui_controls_battery_quit(void)
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
void ui_controls_battery_touch(void)
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
void ui_controls_battery_refresh(void)
{
	#if 0
	// Exercise the progress bar
	static uchar bv = 0;
	static uchar vskip = 0;
	
	vskip++;
	if(vskip < 5)
		return;

	vskip = 0;

	bv++;
	if(bv == 101)
		bv = 0;

	ui_controls_battery_progress(bv);
	#else
	// Real progress from BMS
	#ifdef CONTEXT_BMS
	if(curr_batt_value != bmss.perc)
	{
		ui_controls_battery_progress(bmss.perc);
		curr_batt_value = bmss.perc;
	}
	#endif

	#endif
}

#endif
