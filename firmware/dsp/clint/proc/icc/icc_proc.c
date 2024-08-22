
#include "mchf_board.h"
#include "version.h"

#include "stm32h7xx_hal.h"
#include "stm32h747i_discovery.h"
#include "openamp.h"

//#ifdef CONTEXT_ICC

#include <stdio.h>

#include "icc_proc.h"

#include "dsp_proc.h"
#include "audio_sai.h"		// temp to dump samples
#include "dsp_idle_proc.h"
#include "ui_rotary.h"
#include "cw_gen.h"
#include "softdds.h"

#define RPMSG_SERVICE_NAME              "stm32_icc_service"
//#define RPMSG_SERVICE_NAME1             "stm32_icc_broadcast"

//static  uint32_t 						message;
static volatile int 					message_received;
static volatile unsigned int 			received_data;

static struct rpmsg_endpoint 			rp_endpoint;
//static struct rpmsg_endpoint 			rp_endpoint1;

static void icc_proc_hw_cleanup(void);
extern void sleep_always(void);
extern void allow_reload(void);

uchar icc_init_done = 0;
uchar icc_allow_broadcast = 0;
uchar icc_termination_req = 0;

// ------------------------------------------------
// Frequency public
extern __IO DialFrequency 		df;

// ------------------------------------------------
// Transceiver state public structure
extern __IO TransceiverState 	ts;
extern __IO PaddleState			ps;

unsigned long ui_blink = 0;

unsigned char icc_out_buffer[RPMSG_BUFFER_SIZE - 32];
unsigned char icc_in_buffer [RPMSG_BUFFER_SIZE - 32];

//*----------------------------------------------------------------------------
//* Function Name       :
//* Object              :
//* Object              :
//* Input Parameters    :
//* Output Parameters   :
//* Functions called    :
//*----------------------------------------------------------------------------
void HSEM2_IRQHandler(void)
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
	switch(SemMask)
	{
		// OpenAmp message notification
		case __HAL_HSEM_SEMID_TO_MASK(HSEM_ID_0):
		{
			MAILBOX_Irq();
			break;
		}

		// Switch to TX notification
		case __HAL_HSEM_SEMID_TO_MASK(HSEM_ID_20):
		{
			// Only for TUNE mode ???
			if(ts.tune)
			{
				printf("tune tx\r\n");
				ts.txrx_mode = 1;
				HAL_GPIO_WritePin(PTT_PIN_PORT, PTT_PIN, GPIO_PIN_SET);		// Tx exciter power on
			}

			break;
		}

		// Switch to RX notification
		case __HAL_HSEM_SEMID_TO_MASK(HSEM_ID_21):
		{
			// Only for TUNE mode ???
			if(ts.tune)
			{
				printf("tune rx\r\n");
				ts.txrx_mode = 0;
				HAL_GPIO_WritePin(PTT_PIN_PORT, PTT_PIN, GPIO_PIN_RESET);	// Tx exciter power off
			}

			break;
		}

		// DAH down
		case __HAL_HSEM_SEMID_TO_MASK(HSEM_ID_22):
		{
			//printf("dah down\r\n");
			//HAL_GPIO_TogglePin(GPIOF, GPIO_PIN_6);		// doesn't work
			ps.virtual_dah_down = 1;
//!			__HAL_GPIO_EXTI_GENERATE_SWIT(GPIO_PIN_12);
			break;
		}

		// DIT down
		case __HAL_HSEM_SEMID_TO_MASK(HSEM_ID_23):
		{
			//printf("dit down\r\n");
			ps.virtual_dit_down = 1;
//!			__HAL_GPIO_EXTI_GENERATE_SWIT(GPIO_PIN_13);
			break;
		}

		// DAH up
		case __HAL_HSEM_SEMID_TO_MASK(HSEM_ID_24):
		{
			//printf("dah up\r\n");
			ps.virtual_dah_down = 0;
			break;
		}

		// DIT up
		case __HAL_HSEM_SEMID_TO_MASK(HSEM_ID_25):
		{
			//printf("dit up\r\n");
			ps.virtual_dit_down = 0;
			break;
		}

		// Sleep mode
		case __HAL_HSEM_SEMID_TO_MASK(HSEM_ID_26):
		{
			HAL_NVIC_DisableIRQ((IRQn_Type)(EXTI15_10_IRQn)); // Conflict with GPIOG13 used as SW DAH
			#if 1
			//printf("standby mode\r\n");
			HAL_PWREx_EnterSTANDBYMode(PWR_D2_DOMAIN);
			#else
			//printf("stop mode\r\n");
			HAL_PWREx_EnterSTOPMode(PWR_MAINREGULATOR_ON, PWR_STOPENTRY_WFI, PWR_D2_DOMAIN);
			#endif
			break;
		}

		// Sleep mode
		case __HAL_HSEM_SEMID_TO_MASK(HSEM_ID_27):
		{
			//printf("core sleep\r\n");
			icc_termination_req = 1;
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
	//icc_allow_broadcast = 0;

	received_data = *((unsigned int *) data);
	if(len < sizeof(icc_in_buffer))
	{
		uchar *payload = (uchar *)(data + 4);
		memcpy(icc_in_buffer, payload, (len - 4));
	}

	// Ack receive
	message_received = 1;

	return 0;
}

static int rpmsg_recv_callback1(struct rpmsg_endpoint *ept, void *data, size_t len, uint32_t src, void *priv)
{
	return 0;
}

// Async post to M7 core
void icc_proc_async_broadcast(uchar *buff)
{
#if 0	// doesn't work ;(
	int32_t status = 0;

	if((!icc_init_done)||(!icc_allow_broadcast))
		return;

	//printf("broadcast\r\n");

	icc_out_buffer[0] = 1;
	memcpy(icc_out_buffer, buff, 1024);
	status = OPENAMP_send(&rp_endpoint, icc_out_buffer, 512);
	if (status < 0)
	{
		printf("err send: %d\r\n", status);
	}
#endif
}

void icc_proc_notify_fft_ready(void)
{
	HAL_HSEM_FastTake(HSEM_ID_4);
	HAL_HSEM_Release (HSEM_ID_4, 0);
}

// Very fast notification to M7 core when switching to TX
void icc_proc_notify_of_tx(void)
{
	HAL_HSEM_FastTake(HSEM_ID_10);
	HAL_HSEM_Release (HSEM_ID_10, 0);
}

// Very fast notification to M7 core when switching to RX
void icc_proc_notify_of_rx(void)
{
	HAL_HSEM_FastTake(HSEM_ID_11);
	HAL_HSEM_Release (HSEM_ID_11, 0);
}

ushort icc_proc_post(void)
{
	return ui_driver_get_buffer(icc_out_buffer);
}

void icc_proc_fw_version(void)
{
	// DSP Version
	icc_out_buffer[0x00] = MCHF_D_VER_MAJOR;
	icc_out_buffer[0x01] = MCHF_D_VER_MINOR;
	icc_out_buffer[0x02] = MCHF_D_VER_RELEASE;
	icc_out_buffer[0x03] = MCHF_D_VER_BUILD;
}

/*
void service_destroy_cb(struct rpmsg_endpoint *ept)
{
	printf("== RPC service destroyed ==\r\n");
	OPENAMP_kill_endpoint(&rp_endpoint);
	OPENAMP_DeInit();
}*/

//*----------------------------------------------------------------------------
//* Function Name       : icc_proc_hw_init
//* Object              :
//* Notes    			:
//* Notes   			:
//* Notes    			:
//* Context    			: CONTEXT_ICC
//*----------------------------------------------------------------------------
void icc_proc_hw_init(void)
{
	int32_t status = 0;

	//printf("icc init...\r\n");

	// Init the mailbox use notify the other core on new message
	//MAILBOX_Init();	- called by MX_OPENAMP_Init()

	// Init OpenAmp and libmetal libraries
	if (MX_OPENAMP_Init(RPMSG_REMOTE, NULL) != HAL_OK)
	{
		//printf("err 1\r\n");
		return;
	}

	// create a endpoint for rmpsg communication
	status = OPENAMP_create_endpoint(&rp_endpoint, RPMSG_SERVICE_NAME, RPMSG_ADDR_ANY, rpmsg_recv_callback, NULL);
	if (status < 0)
	{
		//printf("err 2\r\n");
		return;
	}

#if 0
	status = OPENAMP_create_endpoint(&rp_endpoint1, RPMSG_SERVICE_NAME1, RPMSG_ADDR_ANY, rpmsg_recv_callback1, NULL);
	if (status < 0)
	{
		printf("err 3\r\n");
		return;
	}
#endif

	//printf("icc init done\r\n");
	icc_init_done = 1;
}

#if 0
static void icc_proc_hw_cleanup(void)
{
	OPENAMP_kill_endpoint(&rp_endpoint);
	OPENAMP_DeInit();
}
#endif

//*----------------------------------------------------------------------------
//* Function Name       : icc_proc_cmd_handler
//* Object              :
//* Notes    			:
//* Notes   			:
//* Notes    			:
//* Context    			: CONTEXT_ICC
//*----------------------------------------------------------------------------
static ushort icc_proc_cmd_handler(uchar cmd)
{
	ushort ret_size = 1;

	switch(cmd)
	{
		// Waterfall/spectrum FFT data to M7 core
		case ICC_BROADCAST:
			ret_size = icc_proc_post();
			break;

		// Nothing, just init
		case ICC_START_ICC_INIT:
			break;

		// Start all local processes
		case ICC_START_I2S_PROC:
		{
			// Init the RX Hilbert transform/filter prior to initializing the audio!
			UiCalcRxPhaseAdj();

			// Init TX Hilbert transform/filter
			UiCalcTxPhaseAdj();

			// Get filter value so we can init audio with it
			//--UiDriverLoadFilterValue();

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
			#endif
			df.tune_upd 		= 0;
			//df.nco_freq			= 0;//-6000;

			// Start SAI driver
			icc_out_buffer[0x00] = audio_driver_init();

			// Background DSP processor init
			ui_driver_init();

			icc_allow_broadcast = 1;
			break;
		}

		// Blinking LED
		case ICC_TOGGLE_LED:
			#ifdef BOARD_EVAL_747
			BSP_LED_Toggle(LED_BLUE);
			#endif
			break;

		// Return FW version
		case ICC_GET_FW_VERSION:
			icc_proc_fw_version();
			ret_size = 5;
			break;

		// Update local transceiver state
		case ICC_SET_TRX_STATE:
		{
			//print_hex_array(icc_in_buffer, 16);

			#if 0
			ulong i, x = 0;

			for(i = 0; i < 432; i++)
				x += icc_in_buffer[i];

			printf("chksum %d\r\n", x);
			#endif

			memcpy((uchar *)(&ts.samp_rate), icc_in_buffer, sizeof(struct TransceiverState));

			df.nco_freq = ts.nco_freq;
			break;
		}

		// Quick NCO change
		case ICC_SET_NCO_FREQ:
		{
			df.nco_freq  = icc_in_buffer[0] << 0;
			df.nco_freq |= icc_in_buffer[1] << 8;
			//printf("nco %d\r\n", df.nco_freq);
			break;
		}

		// Update most needed parameters
		case ICC_CHANGE_BAND:
		{
			df.nco_freq  = icc_in_buffer[0] << 0;
			df.nco_freq |= icc_in_buffer[1] << 8;
			dsp_idle_proc_change_demod_mode(icc_in_buffer[2], icc_in_buffer[3]);
			dsp_idle_proc_change_filter(icc_in_buffer[4]);
			UiDriverSetBandPowerFactor(icc_in_buffer[5]);
			break;
		}

		// Change demodulator mode
		case ICC_CHANGE_DEMOD_MODE:
		{
			printf("change demod mode %d/%d\r\n", icc_in_buffer[0], icc_in_buffer[1]);
			dsp_idle_proc_change_demod_mode(icc_in_buffer[0], icc_in_buffer[1]);
			break;
		}

		// Change AGC mode
		case ICC_CHANGE_AGC_MODE:
		{
			printf("change agc mode (%d/%d)\r\n", icc_in_buffer[0], icc_in_buffer[1]);
			dsp_idle_proc_change_agc_mode(icc_in_buffer[0], icc_in_buffer[1]);
			break;
		}

		// Change filter
		case ICC_CHANGE_FILTER:
		{
			printf("change filter %d\r\n", icc_in_buffer[0]);
			dsp_idle_proc_change_filter(icc_in_buffer[0]);
			break;
		}

		// Change stereo mode
		case ICC_CHANGE_STEREO:
		{
			printf("stereo mode %d\r\n", icc_in_buffer[0]);
			ts.stereo_mode = icc_in_buffer[0];
			break;
		}

		case ICC_SET_TUNE_MODE:
		{
			//printf("tune mode %d\r\n", icc_in_buffer[0]);
			ts.tune = icc_in_buffer[0];

			//printf("sidetone_freq %d\r\n", ts.sidetone_freq);
			//printf("samp rate %d\r\n", ts.samp_rate);

			if(ts.tune)
				softdds_setfreq(750.0f, ts.samp_rate, 0);
			else
				softdds_setfreq(0.0, ts.samp_rate, 0);

			break;
		}

		default:
			printf("unknown msg %d\r\n",cmd);
			return 1;
	}

	return ret_size;
}

//*----------------------------------------------------------------------------
//* Function Name       : icc_proc_worker
//* Object              :
//* Notes    			:
//* Notes   			:
//* Notes    			:
//* Context    			: CONTEXT_ICC
//*----------------------------------------------------------------------------
static void icc_proc_worker(void)
{
	int32_t status = 0;
	ushort ret_size;

	if(!icc_init_done)
		return;

	OPENAMP_check_for_message();

	// Something to process ?
	if(message_received == 0)
	  return;

	// Clear flag
	message_received = 0;

	// Process command
	ret_size = icc_proc_cmd_handler((uchar)received_data);

	// Response to M7 core
	status = OPENAMP_send(&rp_endpoint, icc_out_buffer, ret_size);
	if (status < 0)
	{
		printf("err send msg: %d (cmd: %d)\r\n", status, (uchar)received_data);
	}

	//icc_allow_broadcast = 1;
}

//*----------------------------------------------------------------------------
//* Function Name       : icc_proc_task
//* Object              :
//* Notes    			: Ok, the method of core real time reload is little bit
//* Notes   			: mad, so before changing anything on both sides of the
//* Notes    			: RPC, make sure you study it a bit
//* Context    			: CONTEXT_ICC
//*----------------------------------------------------------------------------
void icc_proc_task(void const * argument)
{
	// Calls here from superloop only if important HW init is done
	if(!icc_init_done)
		return;

	// Do we have RPC service termination request ?
	if(icc_termination_req)
	{
		// Signal de-init
		icc_init_done = 0;

		// Kill RPC - suppose to work on theory, but sometimes the other side
		// doesn't get notified, regarldless of how long do we wait here :(
		OPENAMP_DeInit();

		// Give some time to RPC callbacks on master to kick in
		HAL_Delay(3000);

		printf("will standby....\r\n");

		HAL_PWREx_EnterSTANDBYMode(PWR_D2_DOMAIN);
		//HAL_PWREx_EnterSTOPMode(PWR_MAINREGULATOR_ON, PWR_STOPENTRY_WFI, PWR_D2_DOMAIN);

		// Never here
		while(1);
	}

	// Process commands
	icc_proc_worker();
}
