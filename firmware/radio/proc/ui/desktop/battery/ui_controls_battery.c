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
#if 0
	char buf[10];

	if(val > 100)
		val = 100;

	// Clear
	GUI_SetColor(GUI_WHITE);
	GUI_FillRoundedRect((BATTERY_X + 2),(BATTERY_Y + 5),(BATTERY_X + BATTERY_SIZE_X - 1),(BATTERY_Y + BATTERY_SIZE_Y - 0),2);

	int y0 = BATTERY_Y + BATTERY_SIZE_Y + 2 - val/2;	// top
	int y1 = BATTERY_Y + BATTERY_SIZE_Y - 0;			// bottom

	if(bmss.run_on_dc)
		GUI_SetColor(GUI_MAGENTA);
	else if(val < 25)
		GUI_SetColor(GUI_LIGHTRED);
	else
		GUI_SetColor(GUI_LIGHTGREEN);

	// Update
	GUI_FillRect((BATTERY_X + 2), y0,(BATTERY_X + BATTERY_SIZE_X - 1),y1);

	sprintf(buf, "%d", val);
	int x  = BATTERY_X + 3;

	if(val < 10)
		x  += 9;
	else if(val < 100)
		x += 5;

	// Text
	GUI_SetColor(GUI_BLACK);
	GUI_SetFont(&GUI_Font8x15B_1);
	GUI_DispStringAt(buf,x,BATTERY_Y + BATTERY_SIZE_Y - 18);
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
	GUI_SetColor(GUI_WHITE);
	GUI_DrawRoundedRect((BATTERY_X +  0),(BATTERY_Y + 3),(BATTERY_X + BATTERY_SIZE_X),    (BATTERY_Y + BATTERY_SIZE_Y + 1),2);
	GUI_DrawRoundedRect((BATTERY_X +  1),(BATTERY_Y + 4),(BATTERY_X + BATTERY_SIZE_X + 1),(BATTERY_Y + BATTERY_SIZE_Y + 2),2);

	// Terminal
	GUI_FillRect(		(BATTERY_X + 10),(BATTERY_Y + 0),(BATTERY_X + 20),				   (BATTERY_Y + 3));

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
