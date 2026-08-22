/************************************************************************************
**                                                                                 **
**                                 mcHF QRP Transceiver                            **
**                         Krassi Atanassov - M0NKA, 2013-2026                     **
**                                                                                 **
**---------------------------------------------------------------------------------**
**                                                                                 **
**  File name:		icc_proc.c                                                     **
**  Description:	M4 core side of the inter core comms driver. Same OpenAMP     **
**					RPC protocol as the CLINT baseband project, but translating   **
**					the commands to the UHSDR DSP code via icc_radio_if           **
**  Licence:		https://github.com/m0nka/mcHF/blob/main/LICENSE                **
************************************************************************************/

// Compiled only for the STM32H747 CM4 baseband build
#ifdef H7_M4_CORE

#include "main.h"

#include <stdio.h>
#include <string.h>

#include "stm32h7xx_hal.h"
#include "openamp.h"

// Wire protocol - HSEM ids, command codes and the shared TransceiverState
// layout. Do not include any UHSDR board header in this translation unit,
// the two worlds define conflicting types/macros!
//
// The M7 core builds the wire struct with mchf_types.h where bool is
// "typedef int" (4 bytes). Force the identical layout here - stdbool.h
// (pulled in by the OpenAMP headers above) would make bool 1 byte and
// shift every field after the first bool member
#ifdef bool
#undef bool
#endif
#define bool int
#include "mchf_icc_def.h"
#undef bool

#include "icc_proc.h"
#include "icc_radio_if.h"
#include "icc_spectrum.h"
#include "icc_wspr.h"
#include "icc_mc_tx.h"

#define RPMSG_SERVICE_NAME              "stm32_icc_service"

static volatile int 					message_received;
static volatile unsigned int 			received_data;

static struct rpmsg_endpoint 			rp_endpoint;

static uchar icc_init_done = 0;
static uchar icc_termination_req = 0;

// Local shadow of the wire format transceiver state, filled by the M7 core
static struct TransceiverState wire_ts;

static unsigned char icc_out_buffer[RPMSG_BUFFER_SIZE - 32];
static unsigned char icc_in_buffer [RPMSG_BUFFER_SIZE - 32];

//*----------------------------------------------------------------------------
//* Function Name       : HSEM2_IRQHandler
//* Object              : CM4 hardware semaphore interrupt entry
//* Context    			: CONTEXT_IRQ
//*----------------------------------------------------------------------------
void HSEM2_IRQHandler(void)
{
	HAL_HSEM_IRQHandler();
}

//*----------------------------------------------------------------------------
//* Function Name       : HAL_HSEM_FreeCallback
//* Object              : fast notifications from the M7 core
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

		// Switch to TX notification (Tune button on M7 UI)
		case __HAL_HSEM_SEMID_TO_MASK(HSEM_ID_20):
		{
			icc_radio_tune_txrx(1);
			break;
		}

		// Switch to RX notification
		case __HAL_HSEM_SEMID_TO_MASK(HSEM_ID_21):
		{
			icc_radio_tune_txrx(0);
			break;
		}

		// DAH down (on screen iambic keyer)
		case __HAL_HSEM_SEMID_TO_MASK(HSEM_ID_22):
		{
			icc_radio_virtual_dah(1);
			break;
		}

		// DIT down
		case __HAL_HSEM_SEMID_TO_MASK(HSEM_ID_23):
		{
			icc_radio_virtual_dit(1);
			break;
		}

		// DAH up
		case __HAL_HSEM_SEMID_TO_MASK(HSEM_ID_24):
		{
			icc_radio_virtual_dah(0);
			break;
		}

		// DIT up
		case __HAL_HSEM_SEMID_TO_MASK(HSEM_ID_25):
		{
			icc_radio_virtual_dit(0);
			break;
		}

		// Sleep mode
		case __HAL_HSEM_SEMID_TO_MASK(HSEM_ID_26):
		{
			HAL_PWREx_EnterSTANDBYMode(PWR_D2_DOMAIN);
			break;
		}

		// Core reload request
		case __HAL_HSEM_SEMID_TO_MASK(HSEM_ID_27):
		{
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
//* Function Name       : rpmsg_recv_callback
//* Object              : incoming rpmsg from the M7 core
//* Context    			: CONTEXT_IRQ
//*----------------------------------------------------------------------------
static int rpmsg_recv_callback(struct rpmsg_endpoint *ept, void *data, size_t len, uint32_t src, void *priv)
{
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

// Notify M7 core that new FFT data is available for collection
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

static void icc_proc_fw_version(void)
{
	// DSP Version
	icc_out_buffer[0x00] = MCHF_D_VER_MAJOR;
	icc_out_buffer[0x01] = MCHF_D_VER_MINOR;
	icc_out_buffer[0x02] = MCHF_D_VER_RELEASE;
	icc_out_buffer[0x03] = MCHF_D_VER_BUILD;
}

//*----------------------------------------------------------------------------
//* Function Name       : icc_proc_extract_settings
//* Object              : convert the wire struct to the neutral settings type
//* Context    			: CONTEXT_ICC
//*----------------------------------------------------------------------------
static void icc_proc_extract_settings(icc_radio_settings_t *st)
{
	st->samp_rate					= wire_ts.samp_rate;

	st->rf_gain						= wire_ts.rf_gain;
	st->rf_codec_gain				= wire_ts.rf_codec_gain;
	st->audio_gain					= wire_ts.audio_gain;
	st->st_gain						= wire_ts.st_gain;
	st->max_rf_gain					= wire_ts.max_rf_gain;

	st->agc_mode					= wire_ts.agc_mode;
	st->agc_custom_decay			= wire_ts.agc_custom_decay;

	st->dmod_mode					= wire_ts.dmod_mode;
	st->filter_id					= wire_ts.filter_id;
	st->band						= wire_ts.band;

	st->keyer_mode					= wire_ts.keyer_mode;
	st->keyer_speed					= wire_ts.keyer_speed;
	st->sidetone_freq				= wire_ts.sidetone_freq;
	st->paddle_reverse				= wire_ts.paddle_reverse;
	st->cw_rx_delay					= wire_ts.cw_rx_delay;

	st->power_level					= wire_ts.power_level;
	st->tx_audio_source				= wire_ts.tx_audio_source;
	st->tx_mic_gain					= wire_ts.tx_mic_gain;
	st->tx_line_gain				= wire_ts.tx_line_gain;
	st->tx_power_factor				= wire_ts.tx_power_factor;

	st->rx_iq_lsb_gain_balance		= wire_ts.rx_iq_lsb_gain_balance;
	st->rx_iq_usb_gain_balance		= wire_ts.rx_iq_usb_gain_balance;
	st->rx_iq_am_gain_balance		= wire_ts.rx_iq_am_gain_balance;
	st->rx_iq_lsb_phase_balance		= wire_ts.rx_iq_lsb_phase_balance;
	st->rx_iq_usb_phase_balance		= wire_ts.rx_iq_usb_phase_balance;
	st->tx_iq_lsb_gain_balance		= wire_ts.tx_iq_lsb_gain_balance;
	st->tx_iq_usb_gain_balance		= wire_ts.tx_iq_usb_gain_balance;
	st->tx_iq_lsb_phase_balance		= wire_ts.tx_iq_lsb_phase_balance;
	st->tx_iq_usb_phase_balance		= wire_ts.tx_iq_usb_phase_balance;

	st->fft_window_type				= wire_ts.fft_window_type;
	st->scope_filter				= wire_ts.scope_filter;
	st->spectrum_db_scale			= wire_ts.spectrum_db_scale;
	st->scope_agc_rate				= wire_ts.scope_agc_rate;
	st->spectrum_scope_nosig_adjust	= wire_ts.spectrum_scope_nosig_adjust;

	st->nco_freq					= wire_ts.nco_freq;
	st->stereo_mode					= wire_ts.stereo_mode;
	st->tune						= wire_ts.tune;
	st->txrx_mode					= wire_ts.txrx_mode;

	st->dsp_active					= wire_ts.dsp_active;
	st->dsp_nr_strength				= wire_ts.dsp_nr_strength;
}

//*----------------------------------------------------------------------------
//* Function Name       : icc_proc_hw_init
//* Object              : init OpenAMP remote side and create the RPC endpoint
//* Context    			: CONTEXT_ICC
//*----------------------------------------------------------------------------
void icc_proc_hw_init(void)
{
	int32_t status = 0;

	// Init OpenAmp and libmetal libraries (also inits the HSEM mailbox)
	if (MX_OPENAMP_Init(RPMSG_REMOTE, NULL) != HAL_OK)
	{
		printf("icc err 1\r\n");
		return;
	}

	// Create the endpoint for rpmsg communication
	status = OPENAMP_create_endpoint(&rp_endpoint, RPMSG_SERVICE_NAME, RPMSG_ADDR_ANY, rpmsg_recv_callback, NULL);
	if (status < 0)
	{
		printf("icc err 2\r\n");
		return;
	}

	icc_init_done = 1;
}

//*----------------------------------------------------------------------------
//* Function Name       : icc_proc_cmd_handler
//* Object              : execute a command from the M7 core
//* Context    			: CONTEXT_ICC
//*----------------------------------------------------------------------------
static ushort icc_proc_cmd_handler(uchar cmd)
{
	ushort ret_size = 1;

	switch(cmd)
	{
		// Waterfall/spectrum FFT data to M7 core
		case ICC_BROADCAST:
			ret_size = icc_spectrum_get_buffer(icc_out_buffer);
			break;

		// Nothing, just init
		case ICC_START_ICC_INIT:
			break;

		// Start all local processes
		case ICC_START_I2S_PROC:
		{
			//printf("i2s start req\r\n");

			// Start UHSDR audio processing and SAI streaming,
			// response byte checked by the M7 core (0 = ok)
			icc_out_buffer[0x00] = icc_radio_start_audio();

			// Background spectrum processor init
			icc_spectrum_init();

			//printf("i2s start: %d\r\n", icc_out_buffer[0x00]);
			break;
		}

		// Blinking LED - 747 EVAL board testing, not used on the radio hw
		case ICC_TOGGLE_LED:
			break;

		// Return FW version
		case ICC_GET_FW_VERSION:
			icc_proc_fw_version();
			ret_size = 5;
			break;

		// Update local transceiver state
		case ICC_SET_TRX_STATE:
		{
			icc_radio_settings_t st;

			memcpy((uchar *)(&wire_ts.samp_rate), icc_in_buffer, sizeof(struct TransceiverState));

			icc_proc_extract_settings(&st);
			icc_radio_apply_trx_state(&st);
			icc_spectrum_apply_settings(&st);
			break;
		}

		// Quick NCO change
		case ICC_SET_NCO_FREQ:
		{
			short nco;

			nco  = icc_in_buffer[0] << 0;
			nco |= icc_in_buffer[1] << 8;
			icc_radio_set_nco_freq(nco);
			break;
		}

		// Update most needed parameters
		case ICC_CHANGE_BAND:
		{
			short nco;

			nco  = icc_in_buffer[0] << 0;
			nco |= icc_in_buffer[1] << 8;
			icc_radio_set_nco_freq(nco);
			icc_radio_change_demod_mode(icc_in_buffer[2], icc_in_buffer[3]);
			icc_radio_change_filter(icc_in_buffer[4]);
			icc_radio_set_band_power_factor(icc_in_buffer[5]);
			break;
		}

		// Change demodulator mode
		case ICC_CHANGE_DEMOD_MODE:
		{
			//printf("change demod mode %d/%d\r\n", icc_in_buffer[0], icc_in_buffer[1]);
			icc_radio_change_demod_mode(icc_in_buffer[0], icc_in_buffer[1]);
			break;
		}

		// Change AGC mode
		case ICC_CHANGE_AGC_MODE:
		{
			//printf("change agc mode (%d/%d)\r\n", icc_in_buffer[0], icc_in_buffer[1]);
			icc_radio_change_agc_mode(icc_in_buffer[0], icc_in_buffer[1]);
			break;
		}

		// Change filter
		case ICC_CHANGE_FILTER:
		{
			//printf("change filter %d\r\n", icc_in_buffer[0]);
			icc_radio_change_filter(icc_in_buffer[0]);
			break;
		}

		// Change stereo mode
		case ICC_CHANGE_STEREO:
		{
			//printf("stereo mode %d\r\n", icc_in_buffer[0]);
			icc_radio_change_stereo(icc_in_buffer[0]);
			break;
		}

		// Tune mode on/off
		case ICC_SET_TUNE_MODE:
		{
			// The TX LED follows the exciter keying in icc_radio_switch_txrx(),
			// which the tune HSEM lines (20/21) reach on their own
			icc_radio_set_tune_mode(icc_in_buffer[0]);
			break;
		}

		// WSPR capture control
		case ICC_WSPR_START:
			icc_wspr_start();
			icc_out_buffer[0x00] = 0;
			break;

		case ICC_WSPR_STOP:
			icc_wspr_stop();
			icc_out_buffer[0x00] = 0;
			break;

		// One buffered capture chunk to the M7 core
		case ICC_WSPR_READ:
			ret_size = icc_wspr_get_buffer(icc_out_buffer);
			break;

		// MarsChat/WSPR symbol transmitter (keys the exciter itself, and
		// the TX LED follows that keying in icc_radio_switch_txrx())
		case ICC_MC_TX_START:
		{
			icc_out_buffer[0x00] = icc_mc_tx_start(icc_in_buffer);
			break;
		}

		// Abort only. The M7 core does NOT send this at the end of a normal
		// transmission - the streamer runs to MC_PH_TAIL and unkeys itself
		// through icc_mc_tx_key_request(), so nothing currently reaches here
		case ICC_MC_TX_STOP:
		{
			icc_mc_tx_stop();
			icc_out_buffer[0x00] = 0;
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
//* Object              : poll for new command and post the response
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
		printf("err send msg: %d (cmd: %d)\r\n", (int)status, (uchar)received_data);
	}
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
		// doesn't get notified, regardless of how long do we wait here :(
		OPENAMP_DeInit();

		// Give some time to RPC callbacks on master to kick in
		HAL_Delay(3000);

		printf("will standby....\r\n");

		HAL_PWREx_EnterSTANDBYMode(PWR_D2_DOMAIN);

		// Never here
		while(1);
	}

	// Process commands
	icc_proc_worker();
}

#endif // H7_M4_CORE
