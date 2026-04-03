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
#include "version.h"

#ifdef CONTEXT_VIDEO

#include "ui_proc.h"
#include "gui.h"
#include "dialog.h"

#include "ui_controls_cpu_stat.h"
#include "desktop\ui_controls_layout.h"

#ifdef STARTEK_5INCH
#define SPROG_X				(PROG_PANEL_X + 16)
#else
#define SPROG_X				(PROG_PANEL_X + 7)
#endif

// Public radio state
extern struct	TRANSCEIVER_STATE_UI	tsu;
//extern ulong epoch;

uint old_usage = 0;

static void ui_controls_cpu_stat_prog_bar(uchar val)
{
	if(val > 100)
		val = 100;
	else if(val < 35)
		val = 35;

	val = (100 - val);

	GUI_SetColor(GUI_BLACK);
	GUI_FillRoundedRect(	(SPROG_X),
							(SPEAKER_Y - 40),
							(SPROG_X + 20),
							(SPEAKER_Y - 40 + 80),
							2);

	GUI_SetColor(GUI_LIGHTBLUE);
	GUI_FillRoundedRect(	(SPROG_X),
							(SPEAKER_Y - (40 - val)),
							(SPROG_X + 20),
							(SPEAKER_Y - 40 + 80),
							2);
}

//*----------------------------------------------------------------------------
//* Function Name       : ui_controls_cpu_stat_show_cpu_load
//* Object              : display load on the CPU
//* Input Parameters    :
//* Output Parameters   :
//* Functions called    :
//*----------------------------------------------------------------------------
static void ui_controls_cpu_stat_show_cpu_load(void)
{
	char tmp[30];
	uint usage;

	usage = osGetCPUUsage();
	if(usage == old_usage)
		return;

	old_usage = usage;

	//printf("aver %2d \r\n", usage);

	// Update progress
	ui_controls_cpu_stat_prog_bar(usage);

	// Fix width
	if(usage > 99) usage = 99;

	// To string
	sprintf((char *)tmp , "%2d", usage);

	// Clear dynamic part
	GUI_SetColor(GUI_LIGHTBLUE);
	GUI_FillRect(	(SPROG_X + 2),
					(SPEAKER_Y - 40 + 67),
					(SPROG_X + 16),
					(SPEAKER_Y - 40 + 78));

	// Show CPU load
	GUI_SetFont(&GUI_Font16B_ASCII);
	GUI_SetColor(GUI_WHITE);
	GUI_DispStringAt(tmp, (SPROG_X + 4), (SPEAKER_Y - 40 + 65));
}

//*----------------------------------------------------------------------------
//* Function Name       :
//* Object              :
//* Notes    			:
//* Notes   			:
//* Notes    			:
//* Context    			: CONTEXT_VIDEO
//*----------------------------------------------------------------------------
void ui_controls_cpu_stat_init(void)
{
	old_usage = 0;

	// System status progress bar
	ui_controls_cpu_stat_prog_bar(100);
}

//*----------------------------------------------------------------------------
//* Function Name       :
//* Object              :
//* Notes    			:
//* Notes   			:
//* Notes    			:
//* Context    			: CONTEXT_VIDEO
//*----------------------------------------------------------------------------
void ui_controls_cpu_stat_quit(void)
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
void ui_controls_cpu_stat_touch(void)
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
void ui_controls_cpu_stat_refresh(void)
{
	ui_controls_cpu_stat_show_cpu_load();
}
#endif
