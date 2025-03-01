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

#ifdef CONTEXT_VFO

#include "si5351.h"
#include "vfo_i2c.h"
#include "vfo_cw_gen.h"

#include "vfo_proc.h"

// Public radio state
extern struct	TRANSCEIVER_STATE_UI	tsu;

// DSP core state
extern struct 	TransceiverState 		ts;

extern 			TaskHandle_t 			hUiTask;
extern TaskHandle_t 					hIccTask;

uchar vfo_init_done = 0;
uchar vfo_loc_demo_mode = 0;
//short vfo_loc_nco = 0xFFFF;

static uchar vfo_proc_set_freq(void)
{
	if(tsu.curr_band > BAND_MODE_GEN)
	{
		printf("vfo_proc_set_freq, curr band: %d, CRITICAL ERROR!\r\n", tsu.curr_band);
		return 100;
	}
	// Local copy of active frequency
	ulong freq = tsu.band[tsu.curr_band].vfo_a;

	// Need four time actual frequencry due to the mixer/exciter switches
	freq *= 4;

	//printf("vfo freq = %d\r\n", freq);

	uint64_t f = (uint64_t)freq * 100ULL;

	return Si5351_set_freq(f, SI5351_CLK0);
}

//*----------------------------------------------------------------------------
//* Function Name       : vfo_proc_hw_init
//* Object              :
//* Notes    			:
//* Notes   			:
//* Notes    			:
//* Context    			: CONTEXT_RESET
//*----------------------------------------------------------------------------
void vfo_proc_hw_init(void)
{
	//printf("vfo_proc_hw_init\r\n");

	// I2C init
	if(BSP_I2C2_Init() != 0)
	{
		printf("vfo i2c2 init err 1!\r\n");
		return;
	}

	// Init VFO
	Si5351_init(SI5351_CRYSTAL_LOAD_8PF, 0, 0);
	//--Si5351_set_freq(1400000000ULL, SI5351_CLK0);

	// Set VFO initial frequency
	if(vfo_proc_set_freq() == 0)
		vfo_init_done = 1;
	else
		printf("vfo i2c2 init err 2!\r\n");

	//printf("vfo_proc_hw_init ok\r\n");
}

void vfo_proc_power_cleanup(void)
{
	Si5351_reset();
}

//*----------------------------------------------------------------------------
//* Function Name       : vfo_proc_worker
//* Object              :
//* Notes    			: command handler, all notifications here
//* Notes   			:
//* Notes    			:
//* Context    			: CONTEXT_VFO
//*----------------------------------------------------------------------------
static void vfo_proc_worker(ulong notif_val)
{
	if(!vfo_init_done)
		return;

	switch(notif_val)
	{
		case UI_NEW_FREQ_EVENT:
		{
			// Set VFO
			if(vfo_proc_set_freq() == 0)
			{
				if(hUiTask != NULL)
					xTaskNotify(hUiTask, UI_NEW_FREQ_EVENT, eSetValueWithOverwrite);
			}
			break;
		}

		default:
			break;
	}
}

//*----------------------------------------------------------------------------
//* Function Name       : vfo_proc_demo_mode_handler
//* Object              :
//* Notes    			: periodic process when Demo mode is used
//* Notes   			:
//* Notes    			:
//* Context    			: CONTEXT_VFO
//*----------------------------------------------------------------------------
static void vfo_proc_demo_mode_handler(void)
{
	// Off, no change
	if((vfo_loc_demo_mode == tsu.demo_mode)&&(!tsu.demo_mode))
		return;

	// Enter demo mode
	if((vfo_loc_demo_mode != tsu.demo_mode)&&(tsu.demo_mode))
	{
		//printf("vfo-> enter demo mode\r\n");

		// Init CW transmitter instance
		vfo_cw_gen_start(0, 14200000, "CQ CQ CQ DE M0NKA");
		//vfo_cw_gen_start(1, 14212000, "TEST TEST TEST");

		vfo_loc_demo_mode = tsu.demo_mode;
	}

	if((vfo_loc_demo_mode != tsu.demo_mode)&&(!tsu.demo_mode))
	{
		//printf("vfo-> exit demo mode\r\n");

		// Stop all
		Si5351_output_enable(SI5351_CLK1, 0);
		Si5351_output_enable(SI5351_CLK2, 0);

		vfo_loc_demo_mode = tsu.demo_mode;
		return;
	}

	// Process while ON
	vfo_cw_gen_proc();
}

//*----------------------------------------------------------------------------
//* Function Name       : vfo_proc_task
//* Object              :
//* Notes    			:
//* Notes   			:
//* Notes    			:
//* Context    			: CONTEXT_VFO
//*----------------------------------------------------------------------------
void vfo_proc_task(void const *arg)
{
	ulong 	ulNotificationValue = 0, ulNotif;
	ulong	vfo_task_sleep_time;

	vTaskDelay(VF0_PROC_START_DELAY);
	//printf("vfo proc start\r\n");

	// Init CW gen
	vfo_cw_gen_init();

vfo_proc_loop:

	if(tsu.demo_mode)
		vfo_task_sleep_time = CW_DIT_RESOLUTION;
	else
		vfo_task_sleep_time = VFO_PROC_SLEEP_TIME;	// deep sleep

	ulNotif = xTaskNotifyWait(0x00, ULONG_MAX, &ulNotificationValue, vfo_task_sleep_time);
	if((ulNotif)&&(ulNotificationValue))
	{
		vfo_proc_worker(ulNotificationValue);
	}

	vfo_proc_demo_mode_handler();
	goto vfo_proc_loop;
}

#endif
