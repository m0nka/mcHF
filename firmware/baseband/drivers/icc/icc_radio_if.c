/************************************************************************************
**                                                                                 **
**                                 mcHF QRP Transceiver                            **
**                         Krassi Atanassov - M0NKA, 2013-2026                     **
**                                                                                 **
**---------------------------------------------------------------------------------**
**                                                                                 **
**  File name:		icc_radio_if.c                                                 **
**  Description:	UHSDR side of the ICC glue - translates the wire protocol     **
**					commands/state from the M7 core into UHSDR API calls.          **
**					This is the only ICC file that includes the UHSDR headers      **
**  Licence:		https://github.com/m0nka/mcHF/blob/main/LICENSE                **
************************************************************************************/

// Compiled only for the STM32H747 CM4 baseband build
#ifdef H7_M4_CORE

#include <stdio.h>
#include <string.h>

#include "uhsdr_board.h"
#include "audio_driver.h"
#include "audio_filter.h"
#include "audio_agc.h"
#include "uhsdr_hw_i2s.h"
#include "drivers/audio/cw/cw_gen.h"

#include "icc_radio_if.h"
#include "icc_spectrum.h"
#include "icc_mc_tx.h"

// ------------------------------------------------------------------
// mcHF Pro board pins handled by the DSP core (V9 rev A, see
// base_hw/mchf_pro_pinmap.h - names prefixed here to avoid clashing
// with the legacy UHSDR board config macros)
//
// PB12 - TX exciter power (PTT)
#define ICC_PTT_PIN				GPIO_PIN_12
#define ICC_PTT_PORT			GPIOB
//
// PE2 - CW paddle DIT
#define ICC_PADDLE_DIT_PIN		GPIO_PIN_2
#define ICC_PADDLE_DIT_PORT		GPIOE
//
// PG3 - CW paddle DAH
#define ICC_PADDLE_DAH_PIN		GPIO_PIN_3
#define ICC_PADDLE_DAH_PORT		GPIOG

// ------------------------------------------------------------------
// Local state
//
// Virtual paddle state driven by the M7 on-screen keyer (HSEM 22..25)
static volatile uint8_t	virtual_dit_down = 0;
static volatile uint8_t	virtual_dah_down = 0;
//
// Deferred requests set in IRQ context, executed in the idle thread
// (PTT on/off requests use the native ts.ptt_req/ts.tx_stop_req flags)
static volatile uint8_t	tune_txrx_req = 0xFF;		// 0xFF = nothing pending
//
static uint8_t			audio_started = 0;

// ------------------------------------------------------------------
// Wire protocol value mapping.
//
// Demodulator modes: the wire protocol uses the classic mcHF numbering
// (USB=0 LSB=1 CW=2 AM=3 FM=4 DIGI=5), UHSDR inserts SAM at 4
static uint8_t icc_radio_map_demod(uint8_t wire_mode)
{
	switch(wire_mode)
	{
		case 4:		return DEMOD_FM;
		case 5:		return DEMOD_DIGI;
		default:	return wire_mode;		// USB/LSB/CW/AM identical
	}
}

// AGC: wire uses slow=0/med=1/fast=2/custom=3/off=4, the WDSP AGC uses
// very long=0/long=1/slow=2/med=3/fast=4/off=5
static uint8_t icc_radio_map_agc(uint8_t wire_agc)
{
	switch(wire_agc)
	{
		case 0:		return 2;	// slow
		case 1:		return 3;	// med
		case 2:		return 4;	// fast
		case 3:		return 3;	// custom -> med
		case 4:		return 5;	// off
		default:	return 3;
	}
}

//*----------------------------------------------------------------------------
//* Function Name       : icc_radio_select_filter_path
//* Object              : select an UHSDR filter path matching the classic
//* Object              : mcHF filter id (300Hz..wide) for the current mode
//* Context    			: CONTEXT_ICC
//*----------------------------------------------------------------------------
static void icc_radio_select_filter_path(uint8_t wire_filter_id)
{
	uint8_t bandwidth_id;
	uint16_t filter_mode = AudioFilter_GetFilterModeFromDemodMode(ts.dmod_mode);

	// Wire ids match the first entries of the UHSDR filter enum
	// (AUDIO_300HZ, AUDIO_500HZ, AUDIO_1P8KHZ, AUDIO_2P3KHZ, AUDIO_3P6KHZ)
	switch(wire_filter_id)
	{
		case 0:	bandwidth_id = AUDIO_300HZ;		break;
		case 1:	bandwidth_id = AUDIO_500HZ;		break;
		case 2:	bandwidth_id = AUDIO_1P8KHZ;	break;
		case 3:	bandwidth_id = AUDIO_2P3KHZ;	break;
		case 4:	bandwidth_id = AUDIO_3P6KHZ;	break;
		default:
			// wide - let the rules pick the widest applicable path below
			bandwidth_id = 0xFF;
			break;
	}

	// Walk the filter path table (bounded - AudioFilter_NextApplicableFilterPath
	// cycles forever and must not be used as an iterator here!) and take the
	// first applicable path built on the wanted filter, or for "wide" the
	// last (== widest) applicable one
	uint8_t path = 0;
	uint8_t found = 0;

	for(uint8_t p = 1; p < AUDIO_FILTER_PATH_NUM; p++)
	{
		if(!AudioFilter_IsApplicableFilterPath(PATH_ALL_APPLICABLE, filter_mode, p))
		{
			continue;
		}

		if(bandwidth_id == 0xFF)
		{
			// wide request: keep the last (== widest) applicable path
			path = p;
			found = 1;
		}
		else if(FilterPathInfo[p].id == bandwidth_id)
		{
			// .id is the AUDIO_xxx bandwidth id of the path
			// (.filter_select_id is only the sub-variant number!)
			path = p;
			found = 1;
			break;
		}
	}

	if(found)
	{
		ts.filter_path = path;
	}
	else
	{
		printf("no filter path for id %d\r\n", wire_filter_id);
	}
}

//*----------------------------------------------------------------------------
//* Function Name       : icc_radio_map_nco
//* Object              : map wire NCO frequency (Hz) to the UHSDR frequency
//* Object              : translation mode
//* Context    			: CONTEXT_ICC
//*----------------------------------------------------------------------------
static void icc_radio_map_nco(int16_t nco_freq)
{
	if(nco_freq >= 9000)
		ts.iq_freq_mode = FREQ_IQ_CONV_P12KHZ;
	else if(nco_freq >= 3000)
		ts.iq_freq_mode = FREQ_IQ_CONV_P6KHZ;
	else if(nco_freq <= -9000)
		ts.iq_freq_mode = FREQ_IQ_CONV_M12KHZ;
	else if(nco_freq <= -3000)
		ts.iq_freq_mode = FREQ_IQ_CONV_M6KHZ;
	else
		ts.iq_freq_mode = FREQ_IQ_CONV_MODE_OFF;
}

// ------------------------------------------------------------------
// TX/RX switching - minimal M4 side variant of RadioManagement_SwitchTxRx.
// The RF side (oscillator, band relays, PA bias) is handled by the M7 core,
// here we only flip the DSP processing direction and the TX exciter PTT line
static void icc_radio_switch_txrx(uint8_t tx_on)
{
	if(tx_on)
	{
		if(ts.txrx_mode == TRX_MODE_TX)
			return;

		UhsdrHwI2s_Codec_ClearTxDmaBuffer();

		ts.txrx_mode = TRX_MODE_TX;
		HAL_GPIO_WritePin(ICC_PTT_PORT, ICC_PTT_PIN, GPIO_PIN_SET);		// TX exciter power on

		// Notify M7 core
		icc_proc_notify_of_tx();
	}
	else
	{
		if(ts.txrx_mode == TRX_MODE_RX)
			return;

		ts.txrx_mode = TRX_MODE_RX;
		HAL_GPIO_WritePin(ICC_PTT_PORT, ICC_PTT_PIN, GPIO_PIN_RESET);	// TX exciter power off

		// Notify M7 core
		icc_proc_notify_of_rx();
	}
}

//*----------------------------------------------------------------------------
//* Function Name       : icc_radio_apply_trx_state
//* Object              : full state upload from the M7 core (ICC_SET_TRX_STATE)
//* Context    			: CONTEXT_ICC
//*----------------------------------------------------------------------------
void icc_radio_apply_trx_state(const icc_radio_settings_t *st)
{
	// Sampling rate is fixed by the SAI hw setup, keep the UHSDR default
	// unless the M7 sends something sane
	if((st->samp_rate == 48000) || (st->samp_rate == 96000))
	{
		ts.samp_rate = st->samp_rate;
	}

	// Audio gains
	ts.rx_gain[RX_AUDIO_SPKR].value = st->audio_gain;
	ts.rx_gain[RX_AUDIO_SPKR].max   = 30;

	// CW keyer
	ts.cw_sidetone_freq	= st->sidetone_freq;
	ts.cw_keyer_speed	= st->keyer_speed;
	ts.cw_keyer_mode	= st->keyer_mode;		// same values: IAM_B=0/IAM_A=1/STRAIGHT=2
	ts.cw_rx_delay		= st->cw_rx_delay;
	CwGen_SetSpeed();

	// TX
	// A zero power factor from the M7 means "not calibrated/not set" (no
	// PA or power table on this radio yet) - keep the local bring-up
	// default instead of silently muting the whole tx chain. Bench found
	// 2026-07-19: this zero killed the MarsChat WP5 radiated test
	if(st->tx_power_factor > 0.0f)
		ts.tx_power_factor = st->tx_power_factor;

	// AGC
	agc_wdsp_conf.mode	= icc_radio_map_agc(st->agc_mode);

	// Frequency translation
	icc_radio_map_nco(st->nco_freq);

	// Demodulator mode + filter, applied together through the processing chain
	ts.dmod_mode = icc_radio_map_demod(st->dmod_mode);
	icc_radio_select_filter_path(st->filter_id);

	AudioDriver_SetProcessingChain(ts.dmod_mode, true);

	printf("trx state: mode %d filt %d nco %d agc %d\r\n",
			ts.dmod_mode, st->filter_id, st->nco_freq, st->agc_mode);
}

//*----------------------------------------------------------------------------
//* Function Name       : icc_radio_start_audio
//* Object              : start SAI streaming (ICC_START_I2S_PROC), 0 = ok.
//* Object              : The M7 core resets/configures the codec via I2C
//* Object              : right after this call returns success
//* Context    			: CONTEXT_ICC
//*----------------------------------------------------------------------------
uint8_t icc_radio_start_audio(void)
{
	if(!audio_started)
	{
		// Set up the dynamic part of the processing chain in case the
		// M7 core did not upload the state yet
		AudioDriver_SetProcessingChain(ts.dmod_mode, true);

		// Start SAI DMA streaming (H7 M4 SAI hw driver)
		UhsdrHwI2s_Codec_StartDMA();

		audio_started = 1;

		// Now enable paddles/ptt processing
		ts.paddles_active = true;
	}

	return 0;
}

void icc_radio_change_demod_mode(uint8_t dmod_mode, uint8_t iamb_type)
{
	ts.dmod_mode = icc_radio_map_demod(dmod_mode);
	ts.cw_keyer_mode = iamb_type;

	AudioDriver_SetProcessingChain(ts.dmod_mode, false);
}

void icc_radio_change_agc_mode(uint8_t agc_mode, uint8_t rf_gain)
{
	(void)rf_gain;

	agc_wdsp_conf.mode = icc_radio_map_agc(agc_mode);
	AudioAgc_SetupAgcWdsp(ads.decimated_freq, ts.dmod_mode == DEMOD_AM || ts.dmod_mode == DEMOD_SAM);
}

void icc_radio_change_filter(uint8_t filter_id)
{
	icc_radio_select_filter_path(filter_id);
	printf("  filter path %d\r\n", ts.filter_path);
	AudioDriver_SetProcessingChain(ts.dmod_mode, false);
	printf("  chain ok\r\n");
}

void icc_radio_change_stereo(uint8_t stereo_mode)
{
	// The UHSDR DSP always feeds both output channels, nothing to do here yet
	(void)stereo_mode;
}

void icc_radio_set_nco_freq(int16_t nco_freq)
{
	icc_radio_map_nco(nco_freq);
	AudioDriver_SetProcessingChain(ts.dmod_mode, false);
}

void icc_radio_set_band_power_factor(uint8_t band)
{
	// TX power factor comes with the full state upload, the band index is
	// only needed by the M7 side band hardware control
	(void)band;
}

void icc_radio_set_tune_mode(uint8_t tune_on)
{
	ts.tune = tune_on;

	// TX/RX switch itself arrives via the fast HSEM lines (20/21)
}

// ------------------------------------------------------------------
// Fast HSEM notifications from the M7 core - IRQ context, defer the
// actual switching to the idle thread
void icc_radio_tune_txrx(uint8_t tx_on)
{
	if(ts.tune)
	{
		tune_txrx_req = tx_on;
	}
}

void icc_radio_virtual_dah(uint8_t down)
{
	virtual_dah_down = down;

	if(down)
	{
		if(ts.dmod_mode == DEMOD_CW)
		{
			CwGen_DahIRQ();
		}
		else if(is_ssb(ts.dmod_mode) || (ts.dmod_mode == DEMOD_AM) || (ts.dmod_mode == DEMOD_FM))
		{
			ts.ptt_req = true;
		}
	}
}

void icc_radio_virtual_dit(uint8_t down)
{
	virtual_dit_down = down;

	if(down && (ts.dmod_mode == DEMOD_CW))
	{
		CwGen_DitIRQ();
	}
}

// ------------------------------------------------------------------
// Physical CW paddle interrupts (DIT = PE2/EXTI2, DAH = PG3/EXTI3)
void EXTI2_IRQHandler(void)
{
	if(__HAL_GPIO_EXTID2_GET_IT(ICC_PADDLE_DIT_PIN) != 0x00U)
	{
		if(ts.paddles_active)
		{
			if((HAL_GPIO_ReadPin(ICC_PADDLE_DIT_PORT, ICC_PADDLE_DIT_PIN) == GPIO_PIN_RESET) || virtual_dit_down)
			{
				if(ts.dmod_mode == DEMOD_CW)
				{
					CwGen_DitIRQ();
				}
			}
		}

		__HAL_GPIO_EXTID2_CLEAR_IT(ICC_PADDLE_DIT_PIN);
	}
}

void EXTI3_IRQHandler(void)
{
	if(__HAL_GPIO_EXTID2_GET_IT(ICC_PADDLE_DAH_PIN) != 0x00U)
	{
		if(ts.paddles_active)
		{
			if((HAL_GPIO_ReadPin(ICC_PADDLE_DAH_PORT, ICC_PADDLE_DAH_PIN) == GPIO_PIN_RESET) || virtual_dah_down)
			{
				if(ts.dmod_mode == DEMOD_CW)
				{
					CwGen_DahIRQ();
				}
				else if(is_ssb(ts.dmod_mode) || (ts.dmod_mode == DEMOD_AM) || (ts.dmod_mode == DEMOD_FM))
				{
					ts.ptt_req = true;
				}
			}
		}

		__HAL_GPIO_EXTID2_CLEAR_IT(ICC_PADDLE_DAH_PIN);
	}
}

// ------------------------------------------------------------------
// Board paddle/PTT line state - replaces the uhsdr_board.c versions
// which are not compiled for the M4 core. Combines the physical GPIO
// lines with the virtual paddle state driven by the M7 on-screen keyer
bool Board_PttDahLinePressed(void)
{
	return (HAL_GPIO_ReadPin(ICC_PADDLE_DAH_PORT, ICC_PADDLE_DAH_PIN) == GPIO_PIN_RESET) || virtual_dah_down;
}

bool Board_DitLinePressed(void)
{
	return (HAL_GPIO_ReadPin(ICC_PADDLE_DIT_PORT, ICC_PADDLE_DIT_PIN) == GPIO_PIN_RESET) || virtual_dit_down;
}

// Status LEDs live on the M7 side of the radio
void Board_GreenLed(ledstate_t state)
{
	(void)state;
}

void Board_RedLed(ledstate_t state)
{
	(void)state;
}

//*----------------------------------------------------------------------------
//* Function Name       : icc_radio_hw_init
//* Object              : PTT output + CW paddle EXTI init, called once at boot
//* Context    			: CONTEXT_ICC
//*----------------------------------------------------------------------------
void icc_radio_hw_init(void)
{
	GPIO_InitTypeDef gpio_init;

	__HAL_RCC_GPIOB_CLK_ENABLE();
	__HAL_RCC_GPIOE_CLK_ENABLE();
	__HAL_RCC_GPIOG_CLK_ENABLE();

	// PTT output, exciter off
	gpio_init.Mode  = GPIO_MODE_OUTPUT_PP;
	gpio_init.Pull  = GPIO_NOPULL;
	gpio_init.Speed = GPIO_SPEED_FREQ_LOW;
	gpio_init.Pin   = ICC_PTT_PIN;
	HAL_GPIO_Init(ICC_PTT_PORT, &gpio_init);
	HAL_GPIO_WritePin(ICC_PTT_PORT, ICC_PTT_PIN, GPIO_PIN_RESET);

	// Paddle inputs with falling edge interrupt
	gpio_init.Mode  = GPIO_MODE_IT_FALLING;
	gpio_init.Pull  = GPIO_PULLUP;
	gpio_init.Pin   = ICC_PADDLE_DIT_PIN;
	HAL_GPIO_Init(ICC_PADDLE_DIT_PORT, &gpio_init);

	gpio_init.Pin   = ICC_PADDLE_DAH_PIN;
	HAL_GPIO_Init(ICC_PADDLE_DAH_PORT, &gpio_init);

	HAL_NVIC_SetPriority((IRQn_Type)(EXTI2_IRQn), 2U, 0x00);
	HAL_NVIC_EnableIRQ	((IRQn_Type)(EXTI2_IRQn));

	HAL_NVIC_SetPriority((IRQn_Type)(EXTI3_IRQn), 2U, 0x00);
	HAL_NVIC_EnableIRQ	((IRQn_Type)(EXTI3_IRQn));
}

//*----------------------------------------------------------------------------
//* Function Name       : icc_radio_ptt_handler
//* Object              : PTT press/release handling for the voice modes,
//* Object              : M4 side variant of the CLINT dsp_idle_proc_ptt_off()
//* Context    			: CONTEXT_ICC (superloop)
//*----------------------------------------------------------------------------
static void icc_radio_ptt_handler(void)
{
	static uint32_t ptt_break = 0;

	// Not when tuning, in this case we are TXing already anyway
	if(ts.tune)
		return;

	// Not while the MarsChat streamer transmits - its keying is driven
	// by icc_mc_tx_key_request(), the voice mode PTT release logic
	// below would unkey it ten superloop iterations in
	if(icc_mc_tx_active())
		return;

	// PTT on request - set by the paddle/PTT interrupts, the M7 virtual
	// keyer or by cw_gen via RadioManagement_Request_TxOn()
	if(ts.ptt_req)
	{
		if((ts.txrx_mode == TRX_MODE_RX) && ((!ts.tx_disable) || (ts.dmod_mode == DEMOD_CW)))
		{
			icc_radio_switch_txrx(1);
		}

		ts.tx_stop_req = false;
		ts.ptt_req = false;
		return;
	}

	if(ts.txrx_mode == TRX_MODE_TX)
	{
		// Explicit stop request (cw_gen keyer finished + TX->RX delay elapsed)
		if(ts.tx_stop_req)
		{
			ts.tx_stop_req = false;
			ptt_break = 0;
			icc_radio_switch_txrx(0);
		}
		// Voice modes: return to RX when the PTT line (DAH input) is released
		else if(is_ssb(ts.dmod_mode) || (ts.dmod_mode == DEMOD_AM) || (ts.dmod_mode == DEMOD_FM))
		{
			if((HAL_GPIO_ReadPin(ICC_PADDLE_DAH_PORT, ICC_PADDLE_DAH_PIN) == GPIO_PIN_SET) && (!virtual_dah_down))
			{
				ptt_break++;
				if(ptt_break > 10)
				{
					ptt_break = 0;
					icc_radio_switch_txrx(0);
				}
			}
			else
			{
				ptt_break = 0;
			}
		}
	}
	else
	{
		ts.tx_stop_req = false;
	}
}

//*----------------------------------------------------------------------------
//* Function Name       : icc_radio_idle_thread
//* Object              : non urgent, time taking operations from the superloop
//* Context    			: CONTEXT_ICC (superloop)
//*----------------------------------------------------------------------------
void icc_radio_idle_thread(void)
{
	// Deferred tune mode TX/RX switch (HSEM 20/21 from the M7 core)
	if(tune_txrx_req != 0xFF)
	{
		uint8_t req = tune_txrx_req;
		tune_txrx_req = 0xFF;

		icc_radio_switch_txrx(req);
	}

	// PTT/TX-stop handling (voice modes and cw_gen requests)
	icc_radio_ptt_handler();

	// MarsChat symbol streamer exciter keying - the streamer keys the
	// TX itself on ICC_MC_TX_START and unkeys when the stream is done
	switch(icc_mc_tx_key_request())
	{
		case 1:	icc_radio_switch_txrx(1);	break;
		case 2:	icc_radio_switch_txrx(0);	break;
		default:							break;
	}

	// Bring-up heartbeat - shows the superloop is alive and whether
	// the SAI DMA stream is running
	{
		extern volatile uint32_t sai_block_count;
		static uint32_t next_beat = 10000;

		if(HAL_GetTick() > next_beat)
		{
			next_beat = HAL_GetTick() + 5000;
			//printf("hb: audio %d, sai blocks %u\r\n", audio_started, (unsigned)sai_block_count);
		}
	}
}

#endif // H7_M4_CORE
