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
#include "version.h"

#ifdef CONTEXT_VIDEO

#include "ui_proc.h"
#include "gui.h"
#include "dialog.h"
//#include "ST_GUI_Addons.h"

#include "ui_controls_cpu_stat.h"
#include "desktop\ui_controls_layout.h"

long 	skip_cpu = 0;		// use this to skip print too often, but calculate average usage
ulong 	cpu_aver = 0;		// accumulator
ulong	cpu_cnt = 0;		// count var
uchar 	uc_keep_alive = 0;
uchar 	uc_keep_flag  = 0;

static void ui_controls_cpu_stat_prog_bar(uchar val)
{
	if(val > 100)
		val = 100;

	val = (100 - val);

	GUI_SetColor(GUI_BLACK);
	GUI_FillRoundedRect(	(SPEAKER_X - 65),
							(SPEAKER_Y - 40),
							(SPEAKER_X - 65 + 20),
							(SPEAKER_Y - 40 + 80),
							2);

	GUI_SetColor(GUI_LIGHTBLUE);
	GUI_FillRoundedRect(	(SPEAKER_X - 65),
							(SPEAKER_Y - (40 - val)),
							(SPEAKER_X - 65 + 20),
							(SPEAKER_Y - 40 + 80),
							2);
}

#if 0
//*----------------------------------------------------------------------------
//* Function Name       : ui_controls_cpu_stat_show_alive
//* Object              : create blinking mark to show OS is still running
//* Input Parameters    :
//* Output Parameters   :
//* Functions called    : simple software trigger with timeout, non blocking
//*----------------------------------------------------------------------------
static void ui_controls_cpu_stat_show_alive(void)
{
	uc_keep_alive++;
	if(uc_keep_alive < 6)
		return;

	if(uc_keep_flag)
		GUI_SetColor(GUI_GREEN);
	else
		GUI_SetColor(GUI_BLACK);

	GUI_FillRoundedRect(BLINKER_X,BLINKER_Y,BLINKER_X + 10,BLINKER_Y + 6,2);

	uc_keep_alive = 0;
	uc_keep_flag = !uc_keep_flag;
}
#endif

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

	skip_cpu++;
	if(skip_cpu < 10)
	{
		// Load accumulator
		cpu_aver += osGetCPUUsage();
		cpu_cnt++;

		return;
	}
	skip_cpu = 0;

	// Get average
	usage = cpu_aver/cpu_cnt;
	if(usage > 99) usage = 99;

	// Update progress
	ui_controls_cpu_stat_prog_bar(usage);

	//EnterCriticalSection();
	sprintf((char *)tmp , "%2d", usage);
	//ExitCriticalSection();

	// Clear dynamic part
	GUI_SetColor(GUI_LIGHTBLUE);
	GUI_FillRect(	(SPEAKER_X - 65 + 2),
					(SPEAKER_Y - 40 + 67),
					(SPEAKER_X - 65 + 16),
					(SPEAKER_Y - 40 + 78));

	// Show CPU load
	GUI_SetFont(&GUI_Font16B_ASCII);
	GUI_SetColor(GUI_WHITE);
	GUI_DispStringAt(tmp, (SPEAKER_X - 65 + 4), (SPEAKER_Y - 40 + 65));

	// Reset accumulator
	cpu_aver 	= 0;
	cpu_cnt 	= 0;
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
	// System status progress bar
#if 0
	GUI_SetColor(GUI_LIGHTBLUE);
	GUI_FillRoundedRect(	(SPEAKER_X - 65),
							(SPEAKER_Y - 20),
							(SPEAKER_X - 65 + 20),
							(SPEAKER_Y - 40 + 80),
							2);
#endif

	ui_controls_cpu_stat_prog_bar(100);

	// DSP Status
	//GUI_SetColor(GUI_WHITE);
	//GUI_DrawHLine((SPEAKER_Y - 20), (SPEAKER_X - 65), (SPEAKER_X - 65 + 20));
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
	//
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
	//ui_controls_cpu_stat_show_alive();
}
#endif
