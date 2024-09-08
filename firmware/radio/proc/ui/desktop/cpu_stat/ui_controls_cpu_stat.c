/************************************************************************************
**                                                                                 **
**                             mcHF Pro QRP Transceiver                            **
**                         Krassi Atanassov - M0NKA, 2013-2024                     **
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
#include "version.h"
//#include "main.h"

#ifdef CONTEXT_VIDEO

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

	// Clear dynamic part
	GUI_SetColor(GUI_WHITE);
	GUI_FillRect(CPU_L_X + 53,	CPU_L_Y + 1,	CPU_L_X + 102,	CPU_L_Y + 13);

	// Create frame
	GUI_SetColor(GUI_GRAY);
	GUI_DrawRect(CPU_L_X - 1,	CPU_L_Y - 1,	CPU_L_X + 106,	CPU_L_Y + 15);
	GUI_DrawRect(CPU_L_X,		CPU_L_Y,		CPU_L_X + 105,	CPU_L_Y + 14);
	GUI_FillRect(CPU_L_X + 20,	CPU_L_Y,		CPU_L_X + 52,	CPU_L_Y + 14);
	GUI_FillRect(CPU_L_X + 103,	CPU_L_Y - 1, 	CPU_L_X + 104, CPU_L_Y + 15);

	// Get average
	usage = cpu_aver/cpu_cnt;

	//EnterCriticalSection();
	sprintf((char *)tmp , "%3d%%", usage);
	//ExitCriticalSection();

	// Show CPU load
	GUI_SetFont(&GUI_Font8x16_1);
	GUI_SetColor(GUI_WHITE);
	GUI_DispStringAt("CPU", CPU_L_X + 25, CPU_L_Y + 1);
	GUI_SetColor(GUI_GRAY);
	GUI_DispStringAt(tmp,CPU_L_X + 65,CPU_L_Y + 1);

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
	char   	buff[100];

	// Debug print CPU firmware version
	GUI_SetColor(GUI_WHITE);
	GUI_SetFont(&GUI_Font8x16_1);
	//sprintf(buff,"CPU v: %d.%d.%d.%d",MCHF_R_VER_MAJOR, MCHF_R_VER_MINOR, MCHF_R_VER_RELEASE, MCHF_R_VER_BUILD);
	//GUI_DispStringAt(buff,360,55);

	// This ok as init ?
	ui_controls_cpu_stat_show_cpu_load();
	ui_controls_cpu_stat_show_alive();
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
	ui_controls_cpu_stat_show_alive();
}
#endif
