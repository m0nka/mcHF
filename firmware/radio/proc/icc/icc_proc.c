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

#include "hw\openamp.h"
#include "stm32h747i_discovery_audio.h"

// In this file it might be little bit confusing what function in what context
// executes, is it actually the ICC task process or HSEM IRQ call chaining into
// a callback. Putthing a wrong call here and there really messes things up

#include "icc_proc.h"
//#include "mchf_icc_def.h"
//#include "hw_dsp_eep.h"
#include "radio_init.h"

// Public radio state
extern struct	TRANSCEIVER_STATE_UI	tsu;
extern struct 	TransceiverState 		ts;
extern struct 	UI_SW					ui_sw;

extern 			TaskHandle_t 			hIccTask;
extern 			TaskHandle_t 			hUiTask;
extern			TaskHandle_t 			hAudioTask;

#ifdef CONTEXT_ICC

#define RPMSG_CHAN_NAME              "stm32_icc_service"
//#define RPMSG_CHAN_NAME1             "stm32_icc_broadcast"

extern const unsigned char dsp_idle[816];

uint32_t	message = 0;
int	 		message_received = 0;
int 		service_created  = 0;

static struct rpmsg_endpoint 	rp_endpoint;
//static struct rpmsg_endpoint 	rp_endpoint1;

uchar 							dsp_remote_init_done = 0;
uchar 							dsp_delayed_init_state = 0;

uchar 							aRxBuffer[RPMSG_BUFFER_SIZE - 32];		// ToDo: try malloc per cmd ???
uchar 							aTxBuffer[RPMSG_BUFFER_SIZE - 32];		//		 --------------------

//*----------------------------------------------------------------------------
//* Function Name       :
//* Object              :
//* Notes    			:
//* Notes   			:
//* Notes    			:
//* Context    			: CONTEXT_IRQ
//*----------------------------------------------------------------------------
void HSEM1_IRQHandler(void)
{
	HAL_HSEM_IRQHandler();
}

//*----------------------------------------------------------------------------
//* Function Name       :
//* Object              :
//* Notes    			:
//* Notes   			:
//* Notes    			:
//* Context    			: CONTEXT_IRQ
//*----------------------------------------------------------------------------
void HAL_HSEM_FreeCallback(uint32_t SemMask)
{
	BaseType_t xHigherPriorityTaskWoken;

	switch(SemMask)
	{
		// OpenAmp message notification
		case __HAL_HSEM_SEMID_TO_MASK(HSEM_ID_1):
		{
			MAILBOX_Irq();
			break;
		}

		// FFT Data ready, OpenAmp on the M7 core is in Master mode, task sleeps
		// all the time, unless awaken by command. So notification from M4 core
		// will wake it up and make it send command to dump waterfall data. Ideally
		// should be a faster way (like immediate access to shared RAM, etc..)
		case __HAL_HSEM_SEMID_TO_MASK(HSEM_ID_4):
		{
			//printf("fft ready\r\n");

			if(hIccTask != NULL)
			{
				xHigherPriorityTaskWoken = pdFALSE;
				xTaskNotifyFromISR(hIccTask, UI_ICC_PROC_BROADCART, eSetBits, &xHigherPriorityTaskWoken );
				portYIELD_FROM_ISR( xHigherPriorityTaskWoken );
			}

			break;
		}

		// Switch to TX notification
		case __HAL_HSEM_SEMID_TO_MASK(HSEM_ID_10):
		{
			//printf("tx\r\n");

			tsu.rxtx = 1;
			if(hAudioTask != NULL)
			{
				xHigherPriorityTaskWoken = pdFALSE;
				xTaskNotifyFromISR(hAudioTask, UI_RXTX_SWITCH, eSetBits, &xHigherPriorityTaskWoken );
				portYIELD_FROM_ISR( xHigherPriorityTaskWoken );
			}
			break;
		}

		// Switch to RX notification
		case __HAL_HSEM_SEMID_TO_MASK(HSEM_ID_11):
		{
			//printf("rx\r\n");

			tsu.rxtx = 0;
			if(hAudioTask != NULL)
			{
				xHigherPriorityTaskWoken = pdFALSE;
				xTaskNotifyFromISR(hAudioTask, UI_RXTX_SWITCH, eSetBits, &xHigherPriorityTaskWoken );
				portYIELD_FROM_ISR( xHigherPriorityTaskWoken );
			}
			break;
		}

		default:
			break;
	}

	// Clear IRQ
	HAL_HSEM_ActivateNotification(SemMask);
}

//*----------------------------------------------------------------------------
//* Function Name       :
//* Object              :
//* Notes    			:
//* Notes   			:
//* Notes    			:
//* Context    			: CONTEXT_IRQ
//*----------------------------------------------------------------------------
static int rpmsg_recv_callback(struct rpmsg_endpoint *ept, void *data, size_t len, uint32_t src, void *priv)
{
	//printf("rx size: %d\r\n",len);

	if(service_created == 0)
		return 0;

	if(len <= sizeof(aRxBuffer))
	{
		memcpy(aRxBuffer,data,len);
		message_received = 1;
	}

	return 0;
}

//*----------------------------------------------------------------------------
//* Function Name       :
//* Object              :
//* Notes    			:
//* Notes   			:
//* Notes    			:
//* Context    			: CONTEXT_IRQ
//*----------------------------------------------------------------------------
static void service_destroy_cb(struct rpmsg_endpoint *ept)
{
	//printf("== RPC service destroyed ==\r\n");
	service_created = 0;
}

//*----------------------------------------------------------------------------
//* Function Name       :
//* Object              :
//* Notes    			:
//* Notes   			:
//* Notes    			:
//* Context    			: CONTEXT_IRQ
//*----------------------------------------------------------------------------
static void new_service_cb(struct rpmsg_device *rdev, const char *name, uint32_t dest)
{
	// create a endpoint for rmpsg communication
	OPENAMP_create_endpoint(&rp_endpoint, name, dest, rpmsg_recv_callback, service_destroy_cb);

	//printf("== RPC Service created ==\r\n");
	service_created = 1;
}

static void api_ui_process_broadcast(void)
{
#ifdef CONTEXT_VIDEO
	ulong i;

	//printf("fft: %d\r\n", aRxBuffer[0]);
	if(aRxBuffer[0] == 0x9F)
	{
		// Copy FFT
		for(i = 0; i < 1024; i++)
			ui_sw.fft_dsp[i] = aRxBuffer[i + 10];

		//printf("s=%d\r\n", aRxBuffer[1]);
		ui_sw.sm_value = aRxBuffer[1];			// s-meter value

		ui_sw.updated = 1;
	}
	else // corruption ?
		printf("fft NA\r\n");
#endif
}

//*----------------------------------------------------------------------------
//* Function Name       : icc_proc_cmd_xchange
//* Object              :
//* Notes    			:
//* Notes   			:
//* Notes    			:
//* Context    			: CONTEXT_ICC
//*----------------------------------------------------------------------------
static uchar icc_proc_cmd_xchange(uchar cmd, uchar *buff, ushort size)
{
	int32_t status;

	// Untested !!!!
	if(service_created == 0)
		return 1;

	// Payload or just command
	if((buff != NULL) && (size != 0) && (size < (sizeof(aRxBuffer) - 4)))
	{
		aRxBuffer[0] = cmd;
		memcpy(aRxBuffer + 4, buff, size);
		status = OPENAMP_send(&rp_endpoint, aRxBuffer, (size + 4));
	}
	else
	{
		message = cmd;
		status = OPENAMP_send(&rp_endpoint, &message, sizeof(message));
	}

	if(status < 0)
	{
		printf("unable to send msg to core!\r\n");
		return 2;
	}

	// ToDo: timeout here...
	//
	while (message_received == 0)
	{
		osDelay(1);
		OPENAMP_check_for_message();
	}
	message_received = 0;

	//printf("data: %02x\r\n", received_data);
	return 0;
}

//*----------------------------------------------------------------------------
//* Function Name       : icc_proc_init_rpc
//* Object              :
//* Notes    			:
//* Notes   			:
//* Notes    			:
//* Context    			: CONTEXT_ICC
//*----------------------------------------------------------------------------
static uchar icc_proc_init_rpc(void)
{
	int res;

	// Initialise the rpmsg endpoint to set default addresses to RPMSG_ADDR_ANY
	rpmsg_init_ept(&rp_endpoint, RPMSG_CHAN_NAME, RPMSG_ADDR_ANY, RPMSG_ADDR_ANY, NULL, NULL);

	// ---------------------------------------------------------------------------------------
	// Initialise OpenAmp and libmetal libraries
	//
	// Note: in order to not fail here, we need to point openAmp malloc() and free() impl to
	//	     FreeRTOS allocator. There must be a way(undocumented) but easiest is to
	//		 modify "libmetal\lib\include\metal\system\generic\alloc.h"
	//
	// Update: actually the way is via syscalls.c, via _sbrk(), but will need to find time
	//           time and experiment with it. Also will have to switch at some point the
	//           default FreeRTOS allocator to TLFS!
	//
	// ---------------------------------------------------------------------------------------

	res = MX_OPENAMP_Init(RPMSG_MASTER, new_service_cb);
	if(res != HAL_OK)
	{
		printf("MX_OPENAMP_Init err: %d\r\n", res);
		return 1;
	}

	// The rpmsg service is initiate by the remote processor, on A7 new_service_cb
	// callback is received on service creation. Wait for the callback
	if(OPENAMP_Wait_EndPointready(&rp_endpoint) != 0)
	{
		OPENAMP_DeInit();
		printf("endpoint not ready \r\n");
		return 2;
	}

	// Callback activated ?
	if(service_created == 0)
	{
		OPENAMP_DeInit();
		printf("service not created \r\n");
		return 3;
	}

	// Start I2S process - dummy call to set comms
	if(icc_proc_cmd_xchange(ICC_START_ICC_INIT, NULL, 0))
	{
		OPENAMP_DeInit();
		printf("dummy call failed \r\n");
		return 4;
	}

	return 0;
}

//*----------------------------------------------------------------------------
//* Function Name       : icc_proc_wake_second_core
//* Object              :
//* Notes    			: we can reload D2 SRAM here, or could be done
//* Notes   			: by the bootloader already. The core sleeps in
//* Notes    			: asm startup section handler
//* Notes    			: waking up is via HSEM notification here
//* Notes    			: so whatever rest of the .text section contains
//* Notes    			: it will be executed (well hopefully valid code)
//* Notes    			:
//* Context    			: CONTEXT_ICC
//*----------------------------------------------------------------------------
static ulong icc_proc_wake_second_core(void)
{
	//int32_t timeout = 0xFFFF;
	//int 	i;


	return 0;

	#if 0
	ulong  chk;
	uchar  *dsp_header = (uchar *)0x081D0000;

	// Checksum of header
	for(i = 0, chk = 0; i < 816; i++)
	{
		chk += *dsp_header++;
	}
	printf("dsp core checksum: %d\r\n", chk);
	#endif

	// Read DSP ID
	ulong dsp_id = READ_REG(BKP_REG_DSP_ID);




	// Test, for development
	//memcpy((void *)D2_AXISRAM_BASE, (void *)0x081D0000, 0x30000);




	// Decide what to do
	switch(dsp_id)
	{
		//case DSP_IDLE:
		//{
			//printf("== blank dsp code (will try flash) ==\r\n");
			//memcpy((void *)D2_AXISRAM_BASE, (void *)0x081D0000, 0x30000);
			//return 1;
		//}

		case DSP_LOADED_CLINT:
			printf("== clint dsp core ==\r\n");
			SET_BIT(RCC->GCR, RCC_BOOT_C2) ;
			break;

		case DSP_LOADED_UHSDR:
			printf("== uhsdr dsp core ==\r\n");
			SET_BIT(RCC->GCR, RCC_BOOT_C2) ;
			break;

		default:
			printf("unknown dsp core: %d\r\n", dsp_id);
			return 1;
	}

	// Remap M4 core boot address (overwrites fuses)
	//HAL_SYSCFG_CM4BootAddConfig(SYSCFG_BOOT_ADDR0, D2_AXISRAM_BASE);
	// Remap M4 core boot address (overwrites fuses)
	//HAL_RCCEx_EnableBootCore(RCC_BOOT_C2);

	//CLEAR_BIT(RCC->GCR, RCC_BOOT_C2) ;

	// Reload code - alrady loaded by bootloader
	///memcpy((void *)D2_AXISRAM_BASE, (void *)0x081D0000, 0x30000);

	//printf("RCC_BOOT_C2:%d\r\n", READ_BIT(RCC->GCR, RCC_BOOT_C2));
	//SET_BIT(RCC->GCR, RCC_BOOT_C2) ;
	//printf("RCC_BOOT_C2:%d\r\n", READ_BIT(RCC->GCR, RCC_BOOT_C2));

#if 0
	HAL_HSEM_FastTake(HSEM_ID_0);

	// Release HSEM in order to notify the CPU2(CM4)
	HAL_HSEM_Release(HSEM_ID_0, 0);

	// wait until CPU2 wakes up from stop mode
	while((__HAL_RCC_GET_FLAG(RCC_FLAG_D2CKRDY) == RESET) && (timeout-- > 0));
	if(timeout < 0)
		return 1;
#endif

	return 0;
}

//*----------------------------------------------------------------------------
//* Function Name       : icc_proc_task
//* Object              :
//* Notes    			: 1.Physically load core bit to D2 RAM
//* Notes   			: 2.Wake the code up
//* Notes   			: 3.Init RPC services
//* Notes    			:
//* Context    			: CONTEXT_ICC
//*----------------------------------------------------------------------------
static int icc_proc_prepare_for_comms(void)
{
	uchar res;

	// Load/Wake up M4 core
	if(icc_proc_wake_second_core())
	{
		printf("unable to load/wake-up the M4 core!\r\n");
		return 1;
	}

	// Init RPC comms
	res = icc_proc_init_rpc();
	if(res)
	{
		//printf("unable to init RPC(%d)!\r\n", res);
		return 2;
	}

	return 0;
}

//*----------------------------------------------------------------------------
//* Function Name       : icc_proc_blink_remote_led
//* Object              :
//* Notes    			: Every 300 mS
//* Notes   			:
//* Notes    			:
//* Context    			: CONTEXT_ICC
//*----------------------------------------------------------------------------
static void icc_proc_blink_remote_led(void)
{
	static ulong ul_blink = 0;

	ul_blink++;
	if(ul_blink < 20)
		return;

	// EVAL only ???
	//if(icc_proc_cmd_xchange(ICC_TOGGLE_LED, NULL, 0) != 0)
	//{
	//	tsu.dsp_alive = 0;								// Ack to UI lost comm to DSP
	//	printf("dsp not alive!\r\n");
	//}
	//else
		tsu.dsp_blinker = !tsu.dsp_blinker;				// Fill blinker

	ul_blink = 0;
}

//*----------------------------------------------------------------------------
//* Function Name       : icc_proc_delayed_dsp_init
//* Object              :
//* Notes    			:
//* Notes   			:
//* Notes    			:
//* Context    			: CONTEXT_ICC
//*----------------------------------------------------------------------------
static void icc_proc_delayed_dsp_init(void)
{
	// Update state machine only if DSP is still alive and init not done
	if((tsu.dsp_alive == 0) || (dsp_remote_init_done == 1))
		return;

	switch(dsp_delayed_init_state)
	{
		// Read running DSP firmware version
		case 0:
		{
			if(icc_proc_cmd_xchange(ICC_GET_FW_VERSION, NULL, 0) == 0)
			{
				tsu.dsp_rev1 = aRxBuffer[0];
				tsu.dsp_rev2 = aRxBuffer[1];
				tsu.dsp_rev3 = aRxBuffer[2];
				tsu.dsp_rev4 = aRxBuffer[3];
				printf("DSP:%d.%d.%d.%d\r\n",tsu.dsp_rev1,tsu.dsp_rev2,tsu.dsp_rev3,tsu.dsp_rev4);
			}
			else
				tsu.dsp_alive = 0;

			break;
		}

		// Upload transceiver state structure local copy
		case 1:
		{
			uchar  *out_ptr = (uchar *)(&ts.samp_rate);
			ushort out_size = sizeof(ts);

			// Init values
			#if 0
			//ts.txrx_mode 		= TRX_MODE_RX;
			ts.api_band 		= 0;
			ts.api_iamb_type 	= 0;

			//ts.dmod_mode 		= DEMOD_LSB;
			ts.audio_gain 		= 6;
			ts.filter_id 		= 3;

			ts.agc_mode			= AGC_MED;
			ts.rf_gain			= 50;
			#else
			radio_init_ui_to_dsp();
			#endif

			#if 0
			printf("to send:%d\r\n", out_size);

			ulong i, x = 0;

			for(i = 0; i < 432; i++)
				x += *(out_ptr + i);

			printf("chksum %d\r\n", x);
			#endif

			icc_proc_cmd_xchange(ICC_SET_TRX_STATE, out_ptr, out_size);
			break;
		}

		case 2:
		{
			if(icc_proc_cmd_xchange(ICC_START_I2S_PROC, NULL, 0) == 0)
			{
				printf("SAI state: %x\r\n", aRxBuffer[0]);

			    // Notify local Audio task, so it can do Reset and I2C init
			    if((hAudioTask != NULL)&&(aRxBuffer[0] == 0))
			    	xTaskNotify(hAudioTask, UI_NEW_SAI_INIT_DONE, eSetValueWithOverwrite);
			}
			break;
		}

		default:
			dsp_remote_init_done = 1;
			return;
	}

	// Next state
	dsp_delayed_init_state++;
}

static uchar icc_proc_wait_delayed_dsp_init(void)
{

icc_proc_loop:

	// Update state machine
	icc_proc_delayed_dsp_init();

	// Just run the delayed init state machine
	if(dsp_remote_init_done == 0)
	{
		// ToDo: timeout ????

		vTaskDelay(20);
		goto icc_proc_loop;
	}

	return 0;
}

//*----------------------------------------------------------------------------
//* Function Name       : icc_proc_dsp_command
//* Object              :
//* Notes    			:
//* Notes   			:
//* Notes    			:
//* Context    			: CONTEXT_ICC
//*----------------------------------------------------------------------------
static void icc_proc_dsp_command(ulong cmd)
{
	uchar data[50];

	if(dsp_remote_init_done == 0)
		return;

	switch(cmd)
	{
		// Awaken by M4 HSEM IRQ to dump FFT data
		case UI_ICC_PROC_BROADCART:
		{
			if(icc_proc_cmd_xchange(ICC_BROADCAST, NULL, 0) == 0)
			{
				api_ui_process_broadcast();
				icc_proc_blink_remote_led();
			}
			else
				tsu.dsp_alive = 0;

			break;
		}

		// Change band - update all DSP params together
		// nco_freq;			*
		// demod_mode			*
		// filter				*
		// keyer mode			*
		case UI_ICC_CHANGE_BAND:
		{
			data[0] = (uchar)(tsu.band[tsu.curr_band].nco_freq >> 0);
			data[1] = (uchar)(tsu.band[tsu.curr_band].nco_freq >> 8);
			data[2] = tsu.band[tsu.curr_band].demod_mode;
			data[3] = tsu.keyer_mode;
			data[4] = tsu.band[tsu.curr_band].filter;
			data[5] = tsu.curr_band;
			icc_proc_cmd_xchange(ICC_CHANGE_BAND, data, 6);
			break;
		}

		// Change NCO frequency
		case UI_ICC_NCO_FREQ:
		{
			short nco_f = tsu.band[tsu.curr_band].nco_freq;
			//printf("nco %d\r\n",nco_f);

			data[0] = (uchar)(nco_f >> 0);
			data[1] = (uchar)(nco_f >> 8);
			icc_proc_cmd_xchange(ICC_SET_NCO_FREQ, data, 2);
			break;
		}

		// Change demodulator mode
		case UI_ICC_DEMOD_MODE:
		{
			data[0] = tsu.band[tsu.curr_band].demod_mode;
			data[1] = tsu.keyer_mode;
			icc_proc_cmd_xchange(ICC_CHANGE_DEMOD_MODE, data, 2);
			break;
		}

		// Change AGC Mode
		case UI_ICC_AGC_MODE:
		{
			data[0] = tsu.agc_mode;
			data[1] = tsu.rf_gain;
			icc_proc_cmd_xchange(ICC_CHANGE_AGC_MODE, data, 2);
			break;
		}

		// Change Filter
		case UI_ICC_FITER:
		{
			data[0] = tsu.band[tsu.curr_band].filter;
			icc_proc_cmd_xchange(ICC_CHANGE_FILTER, data, 1);
			break;
		}

		// Change stereo mode
		case UI_ICC_STEREO:
		{
			data[0] = tsu.stereo_mode;
			icc_proc_cmd_xchange(ICC_CHANGE_STEREO, data, 1);
			break;
		}

		// Set tune mode
		case UI_ICC_TUNE:
		{
			data[0] = tsu.tune;
			icc_proc_cmd_xchange(ICC_SET_TUNE_MODE, data, 1);
			break;
		}

		default:
			return;
	}
}

//*----------------------------------------------------------------------------
//* Function Name       : icc_proc_dsp_on
//* Object              :
//* Notes    			:
//* Notes   			:
//* Notes    			:
//* Context    			: CONTEXT_ICC
//*----------------------------------------------------------------------------
static uchar icc_proc_dsp_on(void)
{
	printf("DSP ON...\r\n");

	// Reset driver publics
	tsu.dsp_alive 		 	= 0;			// remote core assumed off-line
	service_created 	 	= 0;			// RPC service not created
	dsp_remote_init_done 	= 0;			// delayed DSP init not done
	dsp_delayed_init_state	= 0;			// delayed init state machine always starts from zero

	// Need loaded and working core first
	if(icc_proc_prepare_for_comms())
		return 1;

	// Second core is up and running
	// according to the outside world (system processes on the M7 core)
	tsu.dsp_alive = 1;

	// Now do the remote DSP init
	// 1. Try to read DSP core version to make sure the code really runs
	// 2. Transfer init dsp parameters that exists as defaults in the M7 core
	// 3. Synchronise local audio proc I2C comms to Codec SAI comms on the M4 core
	if(icc_proc_wait_delayed_dsp_init())
		return 2;

	// Ready for actual commands
	tsu.dsp_alive = 2;

	return 0;
}

//*----------------------------------------------------------------------------
//* Function Name       : icc_proc_dsp_off
//* Object              :
//* Notes    			:
//* Notes   			:
//* Notes    			:
//* Context    			: CONTEXT_ICC
//*----------------------------------------------------------------------------
static uchar icc_proc_dsp_off(void)
{
	printf("DSP OFF...\r\n");

	// Notify audio process to stop taking responses
	if(hAudioTask != NULL)
		xTaskNotify(hAudioTask, UI_NEW_SAI_CLEANUP, eSetValueWithOverwrite);

	// Give it time
	vTaskDelay(100);

	// Mark as offline(but not before, still the audio request need to pass off)
	tsu.dsp_alive = 0;

	// 1. Kill endpoint on slave
	// 2. Cause core to sleep in ASM handler, still via WFE
	//    instruction, but with possibility to overwrite execution ram with other core
	//    and just wake up by notification
	HAL_HSEM_FastTake(HSEM_ID_27);
	HAL_HSEM_Release (HSEM_ID_27, 0);

	printf("wait remote termination..\r\n");

	// ToDo: Need timeout ???
	while(service_created)
	{
	    OPENAMP_check_for_message();
	}
	printf("remote RPC destroyed\r\n");

	// Kill PRC
	//--OPENAMP_kill_endpoint(&rp_endpoint);
	OPENAMP_DeInit();

	printf("local RPC destroyed\r\n");

	//CLEAR_BIT(RCC->GCR, RCC_BOOT_C2) ;
}

//*----------------------------------------------------------------------------
//* Function Name       : icc_proc_dsp_core_reload
//* Object              :
//* Notes    			: Update D2 SRAM
//* Notes   			:
//* Notes    			:
//* Context    			: CONTEXT_ICC
//*----------------------------------------------------------------------------
static uchar icc_proc_dsp_core_reload(void)
{
#if 0
// Test toggle between two cores on SD card
static uchar curr_core = 0;
WRITE_REG(BKP_REG_DSP_ID, curr_core);

// Need FS reload, how ?
if(curr_core)
	memcpy((void *)D2_AXISRAM_BASE, (void *)0x081D0000, 0x30000);
else
	memcpy((void *)D2_AXISRAM_BASE, (void *)0x081A0000, 0x30000);

HAL_HSEM_FastTake(HSEM_ID_0);

// Release HSEM in order to notify the CPU2(CM4)
HAL_HSEM_Release(HSEM_ID_0, 0);

// wait until CPU2 wakes up from stop mode
//int32_t timeout = 0xFFFF;
//while((__HAL_RCC_GET_FLAG(RCC_FLAG_D2CKRDY) == RESET) && (timeout-- > 0));
//if(timeout < 0)
//	printf("unable to wake-up M4, how to handle?\r\n");

curr_core = !curr_core;

	int32_t timeout = 0xFFFF;
	int 	i, res = 0;
	ulong  chk;

	HAL_SYSCFG_CM4BootAddConfig(SYSCFG_BOOT_ADDR0, D2_AXISRAM_BASE);
	CLEAR_BIT(RCC->GCR, RCC_BOOT_C2);

	// Copy CM4 code to D2_SRAM memory
	memcpy((void *)D2_AXISRAM_BASE, dsp_idle, sizeof(dsp_idle));

	// Remap M4 core boot address (overwrites fuses)
	HAL_RCCEx_EnableBootCore(RCC_BOOT_C2);

	// Wait until CPU2 boots and enters in stop mode or timeout
	while((__HAL_RCC_GET_FLAG(RCC_FLAG_D2CKRDY) != RESET) && (timeout-- > 0))
	{
		__asm("nop");
	}

	if(timeout < 0)
		printf("timeouyt\r\n");
#endif

	memcpy((void *)D2_AXISRAM_BASE, (void *)0x081D0000, 0x30000);
	return 0;
}

//*----------------------------------------------------------------------------
//* Function Name       : icc_proc_task
//* Object              :
//* Notes    			:
//* Notes   			:
//* Notes    			:
//* Context    			: CONTEXT ANY
//*----------------------------------------------------------------------------
void icc_proc_power_cleanup(void)
{
	// Why are those empty ?
}

//*----------------------------------------------------------------------------
//* Function Name       : icc_proc_task
//* Object              :
//* Notes    			:
//* Notes   			:
//* Notes    			:
//* Context    			: CONTEXT_RESET
//*----------------------------------------------------------------------------
void icc_proc_hw_init(void)
{
	// Why are those empty ?
}

//*----------------------------------------------------------------------------
//* Function Name       : icc_proc_task
//* Object              :
//* Notes    			:
//* Notes   			:
//* Notes    			:
//* Context    			: CONTEXT_ICC
//*----------------------------------------------------------------------------
void icc_proc_task(void const *arg)
{
	ulong 	ulNotificationValue = 0, ulNotif;

	vTaskDelay(ICC_PROC_START_DELAY);
	printf("icc proc start\r\n");

	// DSP on
	if(icc_proc_dsp_on())
		goto icc_init_failed;

icc_proc_loop:

	// Sleep forever and wait notification
	ulNotif = xTaskNotifyWait(0x00, ULONG_MAX, &ulNotificationValue, ICC_PROC_SLEEP_TIME);

	// Process commands
	if((ulNotif) && (ulNotificationValue))
	{
		// Core reload take precedence than other mundane commands
		if(ulNotificationValue == UI_ICC_CHANGE_CORE)
		{
			icc_proc_dsp_off();

			// Togle cores
			ulong dsp_id = READ_REG(BKP_REG_DSP_ID);
			if(dsp_id == DSP_LOADED_CLINT)
				dsp_id = DSP_LOADED_UHSDR;
			else
				dsp_id = DSP_LOADED_CLINT;
			WRITE_REG(BKP_REG_DSP_ID, dsp_id);

			// Working core re-load via bootloader restart
			#if 1
			// Enter reason for reset
			WRITE_REG(BKP_REG_RESET_REASON, RESET_DSP_RELOAD);
			HAL_PWR_DisableBkUpAccess();

			// Restart to bootloader
			NVIC_SystemReset();
			#else										// non-working without restart, ToDo: Fix it!
			icc_proc_dsp_core_reload();
			vTaskDelay(500);
			icc_proc_dsp_on();
			#endif
		}
		else
		{
			//printf("command to route to M4: %02x\r\n", ulNotificationValue);
			icc_proc_dsp_command(ulNotificationValue);
		}
	}

	goto icc_proc_loop;

	// Disable driver
icc_init_failed:
	vTaskSuspend(NULL);
}

#endif
