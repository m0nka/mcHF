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

#include "ui_controls_cpu_stat.h"
#include "desktop\ui_controls_layout.h"

#ifdef STARTEK_5INCH
#define SPROG_X				(PROG_PANEL_X + 16)
#else
#define SPROG_X				(PROG_PANEL_X + 7)
#endif

// Public radio state
extern struct	TRANSCEIVER_STATE_UI	tsu;
extern ulong epoch;

long 	skip_cpu = 0;
ulong 	cpu_aver = 0;
ulong	cpu_cnt  = 0;

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
	static uchar old_blinker 	= 0xFF;
	static uchar uc_keep_flag	= 0;
	static ulong blink_timer	= 0;

	// DSP Blinker
	if(old_blinker != tsu.dsp_blinker)
	{
		if(tsu.dsp_blinker)
			GUI_SetColor(GUI_RED);
		else
			GUI_SetColor(GUI_BLACK);

		GUI_FillRect(	(SPROG_X + 25),
						(SPEAKER_Y + 17),
						(SPROG_X + 31),
						(SPEAKER_Y + 17) + 10);

		old_blinker = tsu.dsp_blinker;
	}

	if(epoch < (blink_timer + 800))
		return;
	else if(blink_timer == 0)
		blink_timer = epoch;
	else
		blink_timer = epoch;

	if(uc_keep_flag)
		GUI_SetColor(GUI_GREEN);
	else
		GUI_SetColor(GUI_BLACK);

	GUI_FillRect(	(SPROG_X + 25),
					(SPEAKER_Y + 30),
					(SPROG_X + 31),
					(SPEAKER_Y + 30) + 10);

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
	GUI_FillRect(	(SPROG_X + 2),
					(SPEAKER_Y - 40 + 67),
					(SPROG_X + 16),
					(SPEAKER_Y - 40 + 78));

	// Show CPU load
	GUI_SetFont(&GUI_Font16B_ASCII);
	GUI_SetColor(GUI_WHITE);
	GUI_DispStringAt(tmp, (SPROG_X + 4), (SPEAKER_Y - 40 + 65));

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
	//--ui_controls_cpu_stat_show_alive(); - moved to the clock panel
}
#endif
