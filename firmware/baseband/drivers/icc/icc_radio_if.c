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
#include "audio_management.h"
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
//
// PF6 - front panel TX LED. Same pin as TX_LED_PIN/TX_LED_PIO in the UHSDR
// side main.h, redefined here because this TU cannot include that header.
// Driven from icc_radio_switch_txrx() below so the LED mirrors the exciter
// PTT line rather than the arrival of any particular ICC command
#define ICC_TX_LED_PIN			GPIO_PIN_6
#define ICC_TX_LED_PORT			GPIOF

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

// Bring-up probe: peak level of the mic block arriving from the codec while
// transmitting, written by the SAI callback (uhsdr_hw_i2s.c), printed below
volatile int32_t		icc_tx_audio_peak = 0;

// CW generator TX on/off request counters (icc_uhsdr_stubs.c)
extern volatile uint32_t icc_txon_requests;
extern volatile uint32_t icc_txoff_requests;

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

		// Commit the choice to the per-mode "last used" memory as well.
		// AudioDriver_SetProcessingChain() re-reads the path from this slot
		// (PATH_LAST_USED_IN_MODE) and overwrites ts.filter_path with it, so
		// without this the selection above is silently discarded and the
		// filter never changes - see AudioDriver_SetProcessingChain()
		ts.filter_path_mem[filter_mode][0] = path;
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
//
// This is the single choke point for keying on this core - the tune HSEM
// lines, the PTT/cw_gen handler and the MarsChat symbol streamer all come
// through here - so the front panel TX LED is driven alongside the PTT pin.
// It used to be toggled from the ICC command handlers instead, which left
// it stuck on after a MarsChat transmission: that path ends on the M4 by
// itself and no ICC command marks the end (see icc_proc.c ICC_MC_TX_STOP)
static void icc_radio_switch_txrx(uint8_t tx_on)
{
	if(tx_on)
	{
		if(ts.txrx_mode == TRX_MODE_TX)
			return;

		UhsdrHwI2s_Codec_ClearTxDmaBuffer();

		ts.txrx_mode = TRX_MODE_TX;
		HAL_GPIO_WritePin(ICC_PTT_PORT, ICC_PTT_PIN, GPIO_PIN_SET);			// TX exciter power on
		HAL_GPIO_WritePin(ICC_TX_LED_PORT, ICC_TX_LED_PIN, GPIO_PIN_SET);	// Front panel TX LED on

		// Re-arm the tone generator for this direction - CW sidetone on
		// TX, the tune tone when tuning, silence on RX. UHSDR does this
		// in RadioManagement_SwitchTxRx(), which is not built on the M4
		AudioManagement_SetSidetoneForDemodMode(ts.dmod_mode, ts.tune);

		// Notify M7 core
		icc_proc_notify_of_tx();

		printf("txrx: 1 dmod %d tune %d keyer %d\r\n",
				ts.dmod_mode, ts.tune, ts.cw_keyer_mode);
	}
	else
	{
		if(ts.txrx_mode == TRX_MODE_RX)
			return;

		ts.txrx_mode = TRX_MODE_RX;
		HAL_GPIO_WritePin(ICC_PTT_PORT, ICC_PTT_PIN, GPIO_PIN_RESET);		// TX exciter power off
		HAL_GPIO_WritePin(ICC_TX_LED_PORT, ICC_TX_LED_PIN, GPIO_PIN_RESET);	// Front panel TX LED off

		// Re-arm the tone generator for this direction - CW sidetone on
		// TX, the tune tone when tuning, silence on RX. UHSDR does this
		// in RadioManagement_SwitchTxRx(), which is not built on the M4
		AudioManagement_SetSidetoneForDemodMode(ts.dmod_mode, false);

		// Notify M7 core
		icc_proc_notify_of_rx();

		printf("txrx: 0 dmod %d tune %d keyer %d\r\n",
				ts.dmod_mode, ts.tune, ts.cw_keyer_mode);
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
	// TX power. The UI level is authoritative - it is what the operator sees
	// and it used to be ignored entirely here, so every transmission ran at
	// the hard coded 0.50 regardless of the front panel setting
	icc_radio_set_power_level(st->power_level);

	// Microphone gain. Codec_SwitchMicTxRxMode() is the only function that
	// derives ts.tx_mic_gain_mult, and it is #ifndef H7_M4_CORE (it drives a
	// WM8731; the Pro has a CS4245 on the M7), so on this core the multiplier
	// stayed 0 and tx_processor.c multiplied every mic sample by zero - TX
	// keyed but put out no modulation. Calculation copied from that function
	ts.tx_audio_source = TX_AUDIO_MIC;
	ts.tx_gain[TX_AUDIO_MIC] = st->tx_mic_gain;

	if(ts.tx_gain[TX_AUDIO_MIC] > 50)
		ts.tx_mic_gain_mult = (ts.tx_gain[TX_AUDIO_MIC] - 35) / 3;
	else
		ts.tx_mic_gain_mult = ts.tx_gain[TX_AUDIO_MIC];

	// Speech compressor. AudioManagement_CalcTxCompLevel() derives the ALC
	// post-filter gain and decay from this, and it is another value the M4
	// never loads from config - left at 0 it picks alc_params[0], the least
	// compression entry, whose post-filter gain works out as x1.0. With the
	// ALC knee at 30000 the audio never reaches it, so speech kept its full
	// dynamic range: quiet passages under-modulated, close/loud ones hit the
	// limiter. Has to be followed by a recalc - TxProcessor_Init() only ran
	// it once at boot, before the M7 uploaded anything
	ts.tx_comp_level = st->tx_comp_level;
	AudioManagement_CalcTxCompLevel();

	// AGC
	agc_wdsp_conf.mode	= icc_radio_map_agc(st->agc_mode);

	// Frequency translation
	icc_radio_map_nco(st->nco_freq);

	// Demodulator mode + filter, applied together through the processing chain
	ts.dmod_mode = icc_radio_map_demod(st->dmod_mode);
	icc_radio_select_filter_path(st->filter_id);

	AudioDriver_SetProcessingChain(ts.dmod_mode, true);

	// Tone generator follows the mode (CW sidetone frequency, silence
	// otherwise) - see RadioManagement_SetDemodMode() in the UHSDR tree
	AudioManagement_SetSidetoneForDemodMode(ts.dmod_mode, false);

	printf("trx state: mode %d filt %d nco %d agc %d mic %d\r\n",
			ts.dmod_mode, st->filter_id, st->nco_freq, st->agc_mode, ts.tx_mic_gain_mult);
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

	// Tone generator follows the mode (CW sidetone frequency, silence
	// otherwise) - see RadioManagement_SetDemodMode() in the UHSDR tree
	AudioManagement_SetSidetoneForDemodMode(ts.dmod_mode, false);

	// Bring-up trace: wire value in, and the mode/path that actually stuck
	// after the processing chain re-selected them
	printf("demod: wire %d -> dmod %d, path %d\r\n",
			dmod_mode, ts.dmod_mode, ts.filter_path);
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
	AudioDriver_SetProcessingChain(ts.dmod_mode, false);

	// Bring-up trace: wire id in, and the path that survived the chain
	printf("filter: wire %d -> path %d (dmod %d)\r\n",
			filter_id, ts.filter_path, ts.dmod_mode);
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

//*----------------------------------------------------------------------------
//* Function Name       : icc_radio_set_power_level
//* Object              : map the UI power level (PA_LEVEL_xxx) onto the TX
//* Object              : IQ scaling factor used by tx_processor.c
//* Context    			: CONTEXT_ICC
//*----------------------------------------------------------------------------
void icc_radio_set_power_level(uint8_t level)
{
	// TX output power goes as the square of the IQ amplitude, so the factors
	// are sqrt(P / 20W) referred to 0.50, which measured ~15-20 W on the bench
	// (SN 0003, 20 m, 2026-09-19). These are NOMINAL - there is no per-band PA
	// calibration yet, so the watt labels are indicative only
	static const float pwr_tbl[] =
	{
		0.079f,		// 0.5 W
		0.112f,		// 1 W
		0.158f,		// 2 W
		0.250f,		// 5 W
		0.354f,		// 10 W
		0.433f,		// 15 W
		0.500f		// 20 W
	};

	// Unknown or disabled band (the M7 stores 0xFF for those) - fall back to
	// the lowest setting rather than whatever was in force before
	if(level >= (sizeof(pwr_tbl) / sizeof(pwr_tbl[0])))
		level = 0;

	ts.tx_power_factor = pwr_tbl[level];

	printf("tx power: level %d factor %d/1000\r\n",
			level, (int)(ts.tx_power_factor * 1000.0f));
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
		// Upstream (the uhsdr_main.c paddle handler) requests TX on EVERY DAH
		// press, in every mode, and additionally runs the CW state machine when
		// in CW. Making the two mutually exclusive left CW unable to key at all:
		// the mic PTT shares this line, so in CW nothing ever requested TX, and
		// in straight-key mode CwGen_DahIRQ() is a no-op on top of that
		ts.ptt_req = true;

		if(ts.dmod_mode == DEMOD_CW)
		{
			CwGen_DahIRQ();
		}
	}
}

void icc_radio_virtual_dit(uint8_t down)
{
	virtual_dit_down = down;

	if(down && (ts.dmod_mode == DEMOD_CW))
	{
		// Upstream requests TX on a DIT press too (straight key excepted)
		if(ts.cw_keyer_mode != CW_KEYER_MODE_STRAIGHT)
			ts.ptt_req = true;

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
					// Upstream requests TX on a DIT press too (straight key excepted)
					if(ts.cw_keyer_mode != CW_KEYER_MODE_STRAIGHT)
						ts.ptt_req = true;

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
				// Upstream (the uhsdr_main.c paddle handler) requests TX on EVERY DAH
				// press, in every mode, and additionally runs the CW state machine when
				// in CW. Making the two mutually exclusive left CW unable to key at all:
				// the mic PTT shares this line, so in CW nothing ever requested TX, and
				// in straight-key mode CwGen_DahIRQ() is a no-op on top of that
				ts.ptt_req = true;

				if(ts.dmod_mode == DEMOD_CW)
				{
					CwGen_DahIRQ();
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

	// While transmitting, dump what the TX chain is actually seeing: peak is
	// the mic level arriving off the codec, dah/stop show the CW key state
	{
		static uint32_t next_tx_dump = 0;

		if(ts.txrx_mode == TRX_MODE_TX)
		{
			if(HAL_GetTick() > next_tx_dump)
			{
				next_tx_dump = HAL_GetTick() + 250;

				printf("tx: peak %d alc %d comp %d mic %d dah %d\r\n",
						(int)icc_tx_audio_peak, (int)(ads.alc_val * 100.0f),
						ts.tx_comp_level, ts.tx_mic_gain_mult,
						Board_PttDahLinePressed());
			}
		}
		else
		{
			next_tx_dump = 0;
		}
	}
}

#endif // H7_M4_CORE
