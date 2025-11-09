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
#include "main.h"
#include "mchf_pro_board.h"

#ifdef CONTEXT_AUDIO

#include "audio_proc.h"

#include "shared_i2c.h"
#include "codec_hw.h"

// Public radio state
extern struct	TRANSCEIVER_STATE_UI	tsu;
extern struct	UI_DRIVER_STATE			ui_s;
extern 			TaskHandle_t 			hUiTask;

//*----------------------------------------------------------------------------
//* Function Name       : audio_proc_worker
//* Object              :
//* Notes    			:
//* Notes   			:
//* Notes    			:
//* Context    			: CONTEXT_AUDIO
//*----------------------------------------------------------------------------
static void audio_proc_worker(ulong ulCmd)
{
	// Ok, the DSP core can go off-line at any moment, make
	// sure it is still alive
	if(tsu.dsp_alive == 0)
		return;

	switch(ulCmd)
	{
		// -------------------------
		// HW init from notification
		case UI_NEW_SAI_INIT_DONE:
		{
			//printf("->%s<-notified of SAI running, will reset codec...\r\n", pcTaskGetName(NULL));

			// Hold reset high until MCLK1/2 is on
			codec_hw_reset();

			// Set codec regs
			codec_task_init();

			break;
		}

		// Needed for codec re-load
		case UI_NEW_SAI_CLEANUP:
		{
			//printf("->%s<-notified of DSP core reload...\r\n", pcTaskGetName(NULL));

			// Mute audio path
			#ifdef USE_HARD_MUTE
			#ifndef PCB_V9_REV_A
			HAL_GPIO_WritePin(CODEC_MUTE_PORT, CODEC_MUTE, GPIO_PIN_RESET);
			#else
			HAL_GPIO_WritePin(CODEC_MUTE_PORT, CODEC_MUTE, GPIO_PIN_SET);
			#endif
			#endif

			// Keep the coded in reset for SAI re-init
			HAL_GPIO_WritePin(CODEC_RESET_PORT, CODEC_RESET, GPIO_PIN_RESET);

			break;
		}

		case UI_RXTX_SWITCH:
		{
			//printf("audio proc, awaken to change RX/TX state...\r\n");

			if(tsu.rxtx)
				codec_hw_set_audio_route(CODEC_ROUTE_TX);
			else
				codec_hw_set_audio_route(CODEC_ROUTE_RX);

			break;
		}

		// -------------------------
		// Volume change request
		case UI_NEW_AUDIO_EVENT:
		{
			// Update coded volume
			codec_hw_volume();

			// Notify UI
			if(hUiTask != NULL)
				xTaskNotify(hUiTask, UI_NEW_AUDIO_EVENT, eSetValueWithOverwrite);

			break;
		}

		default:
			break;
	}
}

//*----------------------------------------------------------------------------
//* Function Name       : audio_proc_wait_dsp
//* Object              :
//* Notes    			:
//* Notes   			:
//* Notes    			:
//* Context    			: CONTEXT_AUDIO
//*----------------------------------------------------------------------------
static uchar audio_proc_wait_dsp(void)
{
	ulong	print_skip = 0;

	while(tsu.dsp_alive == 0)
	{
		print_skip++;
		if(print_skip > 100)
		{
			printf("->%s<-waiting the DSP core to wake up...\r\n", pcTaskGetName(NULL));
			print_skip = 0;
		}
		vTaskDelay(200);
	}

	return 0;
}

//*----------------------------------------------------------------------------
//* Function Name       : audio_proc_hw_init
//* Object              :
//* Notes    			:
//* Notes   			:
//* Notes    			:
//* Context    			: CONTEXT_RESET
//*----------------------------------------------------------------------------
void audio_proc_hw_init(void)
{
	//printf("audio_proc_hw_init\r\n");

	// Init codec
	codec_hw_init();

	//printf("audio_proc_hw_init ok\r\n");
}

//*----------------------------------------------------------------------------
//* Function Name       : audio_proc_hw_init
//* Object              :
//* Notes    			:
//* Notes   			:
//* Notes    			:
//* Context    			: CONTEXT ANY
//*----------------------------------------------------------------------------
void audio_proc_power_cleanup(void)
{
	codec_hw_power_cleanup();
}

static void btm_proc_task(void *arg)
{
	uchar old_bt_state = 0;
	uchar new_bt_state;
	uchar loc_bt_enabled = 0xFF;

	//printf("bt proc start\r\n");

	for(;;)
	{
		new_bt_state = HAL_GPIO_ReadPin(BT_COMM_STAT_PORT, BT_COMM_STAT_PIN);
		if(new_bt_state != old_bt_state)
		{
			//
			// ToDo: Send MUTE message to audio task, instead of hardware access here
			// so we can syncronize manual mute with automatic from BT module!
			//
			if(new_bt_state)
			{
				printf("== bt connected ==\r\n");
				#ifdef USE_HARD_MUTE
				#ifndef PCB_V9_REV_A
				HAL_GPIO_WritePin(CODEC_MUTE_PORT, CODEC_MUTE, GPIO_PIN_RESET);	// mute
				#else
				HAL_GPIO_WritePin(CODEC_MUTE_PORT, CODEC_MUTE, GPIO_PIN_SET);	// mute
				#endif
				#endif
			}
			else
			{
				printf("== bt disconnected ==\r\n");
				#ifdef USE_HARD_MUTE
				#ifndef PCB_V9_REV_A
				HAL_GPIO_WritePin(CODEC_MUTE_PORT, CODEC_MUTE, GPIO_PIN_SET);	// unmute
				#else
				HAL_GPIO_WritePin(CODEC_MUTE_PORT, CODEC_MUTE, GPIO_PIN_RESET);	// unmute
				#endif
				#endif
			}
			old_bt_state = new_bt_state;
		}

		// Change BT module power state
		if(tsu.bt_enabled != loc_bt_enabled)
		{
			// Power state update
			#ifndef PCB_V9_REV_A
			if(tsu.bt_enabled)
				HAL_GPIO_WritePin(RFM_DIO2_PORT, RFM_DIO2, GPIO_PIN_RESET);
			else
				HAL_GPIO_WritePin(RFM_DIO2_PORT, RFM_DIO2, GPIO_PIN_SET);
			#else
			#ifndef BT_EN_INV
			if(tsu.bt_enabled)
			#else
			if(!tsu.bt_enabled)
			#endif
				HAL_GPIO_WritePin(BT_EN_PORT, BT_EN_PIN, GPIO_PIN_RESET);
			else
				HAL_GPIO_WritePin(BT_EN_PORT, BT_EN_PIN, GPIO_PIN_SET);
			#endif

			loc_bt_enabled = tsu.bt_enabled;
		}

		// Yield execution
		vTaskDelay(200);
	}
}

//*----------------------------------------------------------------------------
//* Function Name       : audio_proc_task
//* Object              :
//* Notes    			:
//* Notes   			:
//* Notes    			:
//* Context    			: CONTEXT_AUDIO
//*----------------------------------------------------------------------------
void audio_proc_task(void const * argument)
{
	ulong 	ulNotificationValue = 0, ulNotif;

	// Stall here, maybe exit with timeout ?
	#ifdef CONTEXT_ICC
	audio_proc_wait_dsp();
	#else
	goto audio_proc_exit;
	#endif

	// BT Monitor task
    xTaskCreate((TaskFunction_t)btm_proc_task,\
    					"btm_proc",\
						128,\
						NULL,\
						AUDIO_PROC_PRIORITY,\
						NULL);

	//printf("audio proc start\r\n");

audio_proc_loop:

	ulNotif = xTaskNotifyWait(0x00, ULONG_MAX, &ulNotificationValue, AUDIO_PROC_SLEEP_TIME);
	if((ulNotif)&&(ulNotificationValue))
	{
		audio_proc_worker(ulNotificationValue);
	}
	goto audio_proc_loop;

//audio_proc_exit:
	vTaskDelete(NULL);
}
#endif
