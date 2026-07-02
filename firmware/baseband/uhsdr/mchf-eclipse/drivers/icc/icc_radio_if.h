/************************************************************************************
**                                                                                 **
**                                 mcHF QRP Transceiver                            **
**                         Krassi Atanassov - M0NKA, 2013-2026                     **
**                                                                                 **
**---------------------------------------------------------------------------------**
**                                                                                 **
**  File name:		icc_radio_if.h                                                 **
**  Description:	neutral interface between the ICC protocol layer and the      **
**					UHSDR radio code. This header must not include either the     **
**					wire protocol definitions (mchf_icc_def.h) or the UHSDR       **
**					board headers (uhsdr_board.h), so both sides can include it   **
**  Licence:		https://github.com/m0nka/mcHF/blob/main/LICENSE                **
************************************************************************************/
#ifndef __ICC_RADIO_IF_H
#define __ICC_RADIO_IF_H

#include <stdint.h>

// Subset of the wire format TransceiverState (see common/mchf_icc_def.h) that
// the M4 DSP code actually consumes. The ICC layer fills it from the wire
// struct, the UHSDR glue translates it to the native ts structure
typedef struct
{
	uint32_t	samp_rate;

	// Gain related
	int32_t		rf_gain;
	uint8_t		rf_codec_gain;
	uint8_t		audio_gain;
	uint8_t		st_gain;
	uint8_t		max_rf_gain;

	// AGC
	uint8_t		agc_mode;
	uint8_t		agc_custom_decay;

	// Mode/filter/band
	uint8_t		dmod_mode;
	uint8_t		filter_id;
	uint8_t		band;

	// CW keyer
	uint8_t		keyer_mode;
	uint8_t		keyer_speed;
	uint32_t	sidetone_freq;
	uint8_t		paddle_reverse;
	uint8_t		cw_rx_delay;

	// TX
	uint8_t		power_level;
	uint8_t		tx_audio_source;
	uint8_t		tx_mic_gain;
	uint8_t		tx_line_gain;
	float		tx_power_factor;

	// IQ corrections
	int32_t		rx_iq_lsb_gain_balance;
	int32_t		rx_iq_usb_gain_balance;
	int32_t		rx_iq_am_gain_balance;
	int32_t		rx_iq_lsb_phase_balance;
	int32_t		rx_iq_usb_phase_balance;
	int32_t		tx_iq_lsb_gain_balance;
	int32_t		tx_iq_usb_gain_balance;
	int32_t		tx_iq_lsb_phase_balance;
	int32_t		tx_iq_usb_phase_balance;

	// Spectrum/waterfall display processing
	uint8_t		fft_window_type;
	uint8_t		scope_filter;
	uint8_t		spectrum_db_scale;
	uint8_t		scope_agc_rate;
	uint8_t		spectrum_scope_nosig_adjust;

	// Runtime
	int16_t		nco_freq;
	uint8_t		stereo_mode;
	uint8_t		tune;
	uint8_t		txrx_mode;

	// DSP (noise reduction/notch)
	uint8_t		dsp_active;
	uint8_t		dsp_nr_strength;

} icc_radio_settings_t;

// ------------------------------------------------------------------
// Implemented in icc_radio_if.c (UHSDR side), called by the ICC layer
//
// Full state upload (ICC_SET_TRX_STATE)
void	icc_radio_apply_trx_state(const icc_radio_settings_t *st);
//
// Start audio processing chain + SAI streaming (ICC_START_I2S_PROC), 0 = ok
uint8_t	icc_radio_start_audio(void);
//
// Individual live updates
void	icc_radio_change_demod_mode(uint8_t dmod_mode, uint8_t iamb_type);
void	icc_radio_change_agc_mode(uint8_t agc_mode, uint8_t rf_gain);
void	icc_radio_change_filter(uint8_t filter_id);
void	icc_radio_change_stereo(uint8_t stereo_mode);
void	icc_radio_set_nco_freq(int16_t nco_freq);
void	icc_radio_set_band_power_factor(uint8_t band);
void	icc_radio_set_tune_mode(uint8_t tune_on);
//
// Fast HSEM notifications from the M7 core (IRQ context!)
void	icc_radio_tune_txrx(uint8_t tx_on);
void	icc_radio_virtual_dit(uint8_t down);
void	icc_radio_virtual_dah(uint8_t down);
//
// Superloop idle work: S-meter, PTT release, deferred TX/RX switching
void	icc_radio_idle_thread(void);
//
// Local hw init done once at boot (CW paddle EXTI etc.)
void	icc_radio_hw_init(void);

// ------------------------------------------------------------------
// Implemented in icc_proc.c, called by the UHSDR side
//
// Notify M7 core of TX/RX switch (fast HSEM lines)
void	icc_proc_notify_of_tx(void);
void	icc_proc_notify_of_rx(void);

#endif
