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
#include "mchf_icc_def.h"

#include "codec_hw.h"
#include "virt_eeprom.h"
#include "radio_init.h"

// Public radio state
extern struct	TRANSCEIVER_STATE_UI	tsu;

// DSP core state
extern struct 	TransceiverState 		ts;

//*----------------------------------------------------------------------------
//* Function Name       : radio_init_eep_chksum
//* Object              :
//* Input Parameters    :
//* Output Parameters   :
//* Functions called    :
//*----------------------------------------------------------------------------
ulong radio_init_eep_chksum(void)
{
	ulong i, chk;

	for(i = 4, chk = 0; i < 0x1000; i++)
	{
		chk += virt_eeprom_read(i);
	}
	//printf("Eeprom chk 0x%x\r\n", chk);

	return chk;
}

//*----------------------------------------------------------------------------
//* Function Name       : save_band_info
//* Object              :
//* Input Parameters    :
//* Output Parameters   :
//* Functions called    :
//*----------------------------------------------------------------------------
static void save_band_info(void)
{
	uchar  *src = (uchar *)(&(tsu.band[0].band_start));
	ushort dest = EEP_BANDS;
	ushort size = (MAX_BANDS * sizeof(BAND_INFO));
	ushort i;

	for(i = 0; i < size; i++)
	{
		virt_eeprom_write((dest + i), *src++);
	}
}

//*----------------------------------------------------------------------------
//* Function Name       : transceiver_load_eep_values
//* Object              :
//* Input Parameters    :
//* Output Parameters   :
//* Functions called    :
//*----------------------------------------------------------------------------
static int radio_init_load_eep_values(void)
{
	uchar *bkp = (uchar *)EEP_BASE;
	uchar r0, val;
	ulong read_chk, calc_chk;

	if(!tsu.eeprom_init_done)
	{
		printf("eep init not done\r\n");
		return 1;
	}

	// Test - force write
	//--virt_eeprom_write(EEP_BASE_ADDR, 0x00);

	// Check if data is valid
	val = virt_eeprom_read(EEP_BASE_ADDR);
	if(val != 0x73)
	{
		printf("eep bad sig(0x%x)\r\n", val);
		return 2;
	}

	// Calculate checksum
	calc_chk = radio_init_eep_chksum();

	// Read saved value
	read_chk  = virt_eeprom_read(EEP_BASE_ADDR + 1) << 16;
	read_chk |= virt_eeprom_read(EEP_BASE_ADDR + 2) <<  8;
	read_chk |= virt_eeprom_read(EEP_BASE_ADDR + 3);

	// Verify checksum
	if(calc_chk != read_chk)
	{
		printf("eep check sum: 0x%x/0x%x \r\n", calc_chk, read_chk);
		return 2;
	}

	// All at once
	memcpy((uchar *)(&(tsu.band[0].band_start)),(bkp + EEP_BANDS),(MAX_BANDS * sizeof(BAND_INFO)));

	#if 0
	ulong i;
	// Debug print band values from eeprom
	for(i = 0; i < MAX_BANDS; i++)
	{
		printf("------------------------------\r\n");
		printf("vfo_a = %d\r\n",tsu.band[i].vfo_a);
		printf("vfo_b = %d\r\n",tsu.band[i].vfo_b);
		printf("start = %d\r\n",tsu.band[i].band_start);
		printf("__end = %d\r\n",tsu.band[i].band_end);
		printf("ncofr = %d\r\n",tsu.band[i].nco_freq);

		printf("acvfo = %d\r\n",tsu.band[i].active_vfo);
		printf("c/fix = %d\r\n",tsu.band[i].fixed_mode);
		printf("tstep = %d\r\n",tsu.band[i].step);
		printf("filte = %d\r\n",tsu.band[i].filter);
		printf("demod = %d\r\n",tsu.band[i].demod_mode);
		//printf("dspmo = %d\r\n",tsu.band[i].dsp_mode);
		printf("volum = %d\r\n",tsu.band[i].volume);
	}
	#endif

	// Load band
	r0 = virt_eeprom_read(EEP_CURR_BAND);
	if(r0 != 0xFF)
	{
		tsu.curr_band = r0;

		// Overload protection
		if(tsu.curr_band > BAND_MODE_GEN)
			tsu.curr_band = BAND_MODE_80;
	}
	else
		tsu.curr_band = BAND_MODE_80;

	// Load Demo mode flag
	r0 = virt_eeprom_read(EEP_DEMO_MODE);
	if(r0 != 0xFF)
		tsu.demo_mode = r0;
	else
		tsu.demo_mode = 0;

	// Load Brightness
	r0 = virt_eeprom_read(EEP_BRIGHTNESS);
	if(r0 != 0xFF)
		tsu.brightness = r0;
	else
		tsu.brightness = 80;

	// Load S-meter type
	r0 = virt_eeprom_read(EEP_SMET_TYPE);
	if(r0 != 0xFF)
		tsu.smet_type = r0;
	else
		tsu.smet_type = 0;

	// Load AGC mode
	r0 = virt_eeprom_read(EEP_AGC_MODE);
	if(r0 != 0xFF)
		tsu.agc_mode = r0;
	else
		tsu.agc_mode = AGC_MED;

	// Load RF Gain
	r0 = virt_eeprom_read(EEP_RF_GAIN);
	if(r0 != 0xFF)
		tsu.rf_gain = r0;
	else
		tsu.rf_gain = 30;

	#if 0
	printf("curr band: %d\r\n",tsu.curr_band);
	printf("demo mode: %d\r\n",tsu.demo_mode);
	printf("smet type: %d\r\n",tsu.smet_type);
	printf("brightnes: %d\r\n",tsu.brightness);
	#endif

	return 0;
}

static void radio_init_load_dsp_values(void)
{
	// Defaults always
	ts.txrx_mode 		= TRX_MODE_RX;					// start in RX
	ts.samp_rate		= SAI_AUDIO_FREQUENCY_48K;		// set sampling rate

	//ts.enc_one_mode 	= ENC_ONE_MODE_AUDIO_GAIN;
	//ts.enc_two_mode 	= ENC_TWO_MODE_RF_GAIN;
	//ts.enc_thr_mode		= ENC_THREE_MODE_RIT;

	ts.band		  		= BAND_MODE_20;				// band from eeprom
	ts.band_change		= 0;						// used in muting audio during band change
	ts.filter_band		= 0;						// used to indicate the bpf filter selection for power detector coefficient selection
	ts.dmod_mode 		= DEMOD_LSB;				// demodulator mode
	ts.audio_gain		= 0;//DEFAULT_AUDIO_GAIN;		// Set initial volume
	ts.audio_gain		= 0;//MAX_VOLUME_DEFAULT;		// Set max volume default
	ts.audio_gain_active = 1;						// this variable is used in the active RX audio processing function

	ts.rf_gain			= 30;//DEFAULT_RF_GAIN;			//
	ts.max_rf_gain		= 3;//MAX_RF_GAIN_DEFAULT;		// setting for maximum gain (e.g. minimum S-meter reading)
	ts.rf_codec_gain	= 0;//DEFAULT_RF_CODEC_GAIN_VAL;	// Set default RF gain (0 = lowest, 8 = highest, 9 = "Auto")
	ts.rit_value		= 0;						// RIT value
	ts.agc_mode			= AGC_OFF;//AGC_DEFAULT;				// AGC setting
	ts.agc_custom_decay	= 12;//AGC_CUSTOM_DEFAULT;		// Default setting for AGC custom setting - higher = slower



	ts.filter_id		= AUDIO_DEFAULT_FILTER;		// startup audio filter
	ts.filter_300Hz_select	= FILTER_300HZ_DEFAULT;	// Select 750 Hz center filter as default
	ts.filter_500Hz_select	= FILTER_500HZ_DEFAULT;	// Select 750 Hz center filter as default
	ts.filter_1k8_select	= FILTER_1K8_DEFAULT;	// Select 1425 Hz center filter as default
	ts.filter_2k3_select	= FILTER_2K3_DEFAULT;	// Select 1412 Hz center filter as default
	ts.filter_3k6_select	= FILTER_3K6_DEFAULT;	// This is enabled by default
	ts.filter_wide_select	= FILTER_WIDE_DEFAULT;	// This is enabled by default


	//
	ts.st_gain			= 5;//DEFAULT_SIDETONE_GAIN;	// Sidetone gain
	ts.keyer_mode		= CW_MODE_STRAIGHT;			// CW keyer mode
	ts.keyer_speed		= 12;//DEFAULT_KEYER_SPEED;		// CW keyer speed
	ts.sidetone_freq	= 750;//CW_SIDETONE_FREQ_DEFAULT;	// CW sidetone and TX offset frequency
	ts.paddle_reverse	= 0;						// Paddle defaults to NOT reversed
	ts.cw_rx_delay		= 8;//CW_RX_DELAY_DEFAULT;		// Delay of TX->RX turnaround
	ts.unmute_delay_count		= 450;//SSB_RX_DELAY;		// Used to time TX->RX delay turnaround

	//
	ts.nb_setting		= 0;						// Noise Blanker setting
	//
	ts.tx_iq_lsb_gain_balance 	= 1;				// Default settings for RX and TX gain and phase balance
	ts.tx_iq_lsb_gain_balance 	= 1;
	ts.tx_iq_usb_gain_balance 	= 1;
	ts.tx_iq_usb_gain_balance 	= 1;

	ts.rx_iq_lsb_gain_balance 	= -15;
	ts.rx_iq_lsb_phase_balance 	= 0;

	ts.rx_iq_usb_gain_balance 	= 0;
	ts.rx_iq_usb_phase_balance 	= 0;
	//
	ts.tune_freq		= 0;
	ts.tune_freq_old	= 0;
	//
//	ts.calib_mode		= 0;						// calibrate mode
	ts.menu_mode		= 0;						// menu mode
	ts.menu_item		= 0;						// menu item selection
	ts.menu_var			= 0;						// menu item change variable
	ts.menu_var_changed	= 0;						// TRUE if a menu variable was changed and that an EEPROM save should be done

	//ts.txrx_lock		= 0;						// unlocked on start
	ts.audio_unmute		= 0;						// delayed un-mute not needed
	ts.buffer_clear		= 0;						// used on return from TX to purge the audio buffers

	ts.tx_audio_source	= 0;//TX_AUDIO_MIC;				// default source is microphone
	ts.tx_mic_gain		= 15;//MIC_GAIN_DEFAULT;			// default microphone gain
	ts.tx_mic_gain_mult	= ts.tx_mic_gain;			// actual operating value for microphone gain
	ts.mic_boost		= 0;
	ts.tx_line_gain		= 12;//LINE_GAIN_DEFAULT;		// default line gain

	ts.tune				= 0;						// reset tuning flag

	ts.pa_bias			= 0;//DEFAULT_PA_BIAS;			// Use lowest possible voltage as default
	ts.pa_cw_bias		= 0;//DEFAULT_PA_BIAS;			// Use lowest possible voltage as default (nonzero sets separate bias for CW mode)
	ts.freq_cal			= 0;							// Initial setting for frequency calibration
	ts.power_level		= PA_LEVEL_DEFAULT;			// See mchf_board.h for setting
	//
//	ts.codec_vol		= 0;						// Holder for codec volume
//	ts.codec_mute_state	= 0;						// Holder for codec mute state
//	ts.codec_was_muted = 0;							// Indicator that codec *was* muted
	//
	ts.powering_down	= 0;						// TRUE if powering down
	//
	ts.scope_speed		= 5;//SPECTRUM_SCOPE_SPEED_DEFAULT;	// default rate of spectrum scope update

	ts.waterfall_speed	= 15;//WATERFALL_SPEED_DEFAULT_SPI;		// default speed of update of the waterfall for parallel displays
	//
	ts.scope_filter		= 4;//SPECTRUM_SCOPE_FILTER_DEFAULT;	// default filter strength for spectrum scope
//!	ts.scope_trace_colour	= SPEC_COLOUR_TRACE_DEFAULT;	// default colour for the spectrum scope trace
//!	ts.scope_grid_colour	= SPEC_COLOUR_GRID_DEFAULT;		// default colour for the spectrum scope grid
//!	ts.scope_grid_colour_active = Grid;
//!	ts.scope_centre_grid_colour = SPEC_COLOUR_GRID_DEFAULT;		// color of center line of scope grid
//!	ts.scope_centre_grid_colour_active = Grid;
//!	ts.scope_scale_colour	= SPEC_COLOUR_SCALE_DEFAULT;	// default colour for the spectrum scope frequency scale at the bottom
	ts.scope_agc_rate	= 25;//SPECTRUM_SCOPE_AGC_DEFAULT;		// load default spectrum scope AGC rate
	ts.spectrum_db_scale = DB_DIV_10;					// default to 10dB/division
	//
	ts.menu_item		= 0;						// start out with a reasonable menu item
	//
	ts.radio_config_menu_enable = 0;				// TRUE if radio configuration menu is to be enabled
	//
	ts.cat_mode_active	= 0;						// TRUE if CAT mode is active
	//
	ts.xverter_mode		= 0;						// TRUE if transverter mode is active (e.g. offset of display)
	ts.xverter_offset	= 0;						// Frequency offset in transverter mode (added to frequency display)
	//
	ts.refresh_freq_disp	= 1;					// TRUE if frequency/color display is to be refreshed when next called - NORMALLY LEFT AT 0 (FALSE)!!!
													// This is NOT reset by the LCD function, but must be enabled/disabled externally
	//
	ts.tx_power_factor	= 0.50;						// TX power factor
	//
	ts.pwr_80m_5w_adj	= TX_POWER_FACTOR_80_DEFAULT;
	ts.pwr_60m_5w_adj	= TX_POWER_FACTOR_60_DEFAULT;
	ts.pwr_40m_5w_adj	= TX_POWER_FACTOR_40_DEFAULT;
	ts.pwr_30m_5w_adj	= TX_POWER_FACTOR_30_DEFAULT;
	ts.pwr_20m_5w_adj	= TX_POWER_FACTOR_20_DEFAULT;
	ts.pwr_17m_5w_adj	= TX_POWER_FACTOR_17_DEFAULT;
	ts.pwr_15m_5w_adj	= TX_POWER_FACTOR_15_DEFAULT;
	ts.pwr_12m_5w_adj	= TX_POWER_FACTOR_12_DEFAULT;
	ts.pwr_10m_5w_adj	= TX_POWER_FACTOR_10_DEFAULT;
	//
	ts.filter_cw_wide_disable		= 0;			// TRUE if wide filters are to be disabled in CW mode
	ts.filter_ssb_narrow_disable	= 0;			// TRUE if narrow (CW) filters are to be disabled in SSB mdoe
	ts.am_mode_disable				= 0;			// TRUE if AM mode is to be disabled
	//
	ts.tx_meter_mode	= 0;//METER_SWR;
	//
	ts.alc_decay		= 10;//ALC_DECAY_DEFAULT;		// ALC Decay (release) default value
	ts.alc_decay_var	= 10;//ALC_DECAY_DEFAULT;		// ALC Decay (release) default value
	ts.alc_tx_postfilt_gain		= 1;//ALC_POSTFILT_GAIN_DEFAULT;	// Post-filter added gain default (used for speech processor/ALC)
	ts.alc_tx_postfilt_gain_var		= 1;//ALC_POSTFILT_GAIN_DEFAULT;	// Post-filter added gain default (used for speech processor/ALC)
	ts.tx_comp_level	= 0;		// 0=Release Time/Pre-ALC gain manually adjusted, >=1:  Value calculated by this parameter
	//
	ts.freq_step_config		= 0;			// disabled both marker line under frequency and swapping of STEP buttons
	//
	ts.nb_disable		= 0;				// TRUE if noise blanker is to be disabled
	//
	ts.dsp_active		= 0;				// TRUE if DSP noise reduction is to be enabled
	ts.dsp_active_toggle	= 0xff;			// used to hold the button G2 "toggle" setting.
	ts.dsp_nr_delaybuf_len = 192;//DSP_NR_BUFLEN_DEFAULT;
	ts.dsp_nr_strength	= 0;				// "Strength" of DSP noise reduction (0 = weak)
	ts.dsp_nr_numtaps 	= 96;//DSP_NR_NUMTAPS_DEFAULT;		// default for number of FFT taps for noise reduction
	ts.dsp_notch_numtaps = 96;//DSP_NOTCH_NUMTAPS_DEFAULT;	// default for number of FFT taps for notch filter
	ts.dsp_notch_delaybuf_len =	104;//DSP_NOTCH_DELAYBUF_DEFAULT;
	ts.dsp_inhibit		= 1;				// TRUE if DSP is to be inhibited - power up with DSP disabled
	ts.dsp_inhibit_mute = 0;				// holder for "dsp_inhibit" during muting operations to allow restoration of previous state
	ts.dsp_timed_mute	= 0;				// TRUE if DSP is to be muted for a timed amount
	ts.dsp_inhibit_timing = 0;				// used to time inhibiting of DSP when it must be turned off for some reason
	ts.reset_dsp_nr		= 0;				// TRUE if DSP NR coefficients are to be reset when "audio_driver_set_rx_audio_filter()" is called
	ts.lcd_backlight_brightness = 0;		// = 0 full brightness
	ts.lcd_backlight_blanking = 0;			// MSB = 1 for auto-off of backlight, lower nybble holds time for auto-off in seconds
	//
	ts.tune_step		= 0;				// Used for press-and-hold step size changing mode
	ts.frequency_lock	= 0;				// TRUE if frequency knob is locked
	//
	ts.tx_disable		= 0;				// TRUE if transmitter is to be disabled
	ts.misc_flags1		= 0;				// Used to hold individual status flags, stored in EEPROM location "EEPROM_MISC_FLAGS1"
	ts.misc_flags2		= 0;				// Used to hold individual status flags, stored in EEPROM location "EEPROM_MISC_FLAGS2"
	ts.sysclock			= 0;				// This counts up from zero when the unit is powered up at precisely 100 Hz over the long term.  This
											// is NEVER reset and is used for timing certain events.
	ts.version_number_release	= 0;		// version release - used to detect firmware change
	ts.version_number_build = 0;			// version build - used to detect firmware change
	ts.nb_agc_time_const	= 0;			// used to calculate the AGC time constant
	ts.cw_offset_mode	= 0;				// CW offset mode (USB, LSB, etc.)
	ts.cw_lsb			= 0;				// Flag that indicates CW operates in LSB mode when TRUE
	ts.iq_freq_mode		= 0;				// used to set/configure the I/Q frequency/conversion mode
	ts.conv_sine_flag	= 0;				// FALSE until the sine tables for the frequency conversion have been built (normally zero, force 0 to rebuild)
	ts.lsb_usb_auto_select	= 0;			// holds setting of LSB/USB auto-select above/below 10 MHz
	ts.hold_off_spectrum_scope	= 0;		// this is a timer used to hold off updates of the spectrum scope when an SPI LCD display interface is used
	ts.lcd_blanking_time = 0;				// this holds the system time after which the LCD is blanked - if blanking is enabled
	ts.lcd_blanking_flag = 0;				// if TRUE, the LCD is blanked completely (e.g. backlight is off)
	ts.freq_cal_adjust_flag = 0;			// set TRUE if frequency calibration is in process
	ts.xvtr_adjust_flag = 0;				// set TRUE if transverter offset adjustment is in process
	ts.rx_muting = 0;						// set TRUE if audio output is to be muted
	ts.rx_blanking_time = 0;				// this is a timer used to delay the un-blanking of the audio after a large synthesizer tuning step
	ts.vfo_mem_mode = 0;					// this is used to record the VFO/memory mode (0 = VFO "A" = backwards compatibility)
											// LSB+6 (0x40) = 0:  VFO A,  1 = VFO B
											// LSB+7 (0x80) = 0:  Normal mode, 1 = SPLIT mode
											// Other bits are currently reserved
	ts.voltmeter_calibrate	= 100;//POWER_VOLTMETER_CALIBRATE_DEFAULT;	// Voltmeter calibration constant
	ts.thread_timer = 0;					// used to time thread
//!	ts.waterfall_color_scheme = WATERFALL_COLOR_DEFAULT;	// color scheme for waterfall display
	ts.waterfall_vert_step_size = 2;//WATERFALL_STEP_SIZE_DEFAULT;		// step size in waterfall display
//!	ts.waterfall_offset = WATERFALL_OFFSET_DEFAULT;		// Offset for waterfall display (brightness)
//!	ts.waterfall_contrast = WATERFALL_CONTRAST_DEFAULT;	// contrast setting for waterfall display
	ts.spectrum_scope_scheduler = 0;		// timer for scheduling the next update of the spectrum scope update
	ts.spectrum_scope_nosig_adjust = 20;	//SPECTRUM_SCOPE_NOSIG_ADJUST_DEFAULT;	// Adjustment for no signal adjustment conditions for spectrum scope
//!	ts.waterfall_nosig_adjust = WATERFALL_NOSIG_ADJUST_DEFAULT;		// Adjustment for no signal adjustment conditions for waterfall
	ts.waterfall_size	= 0;//WATERFALL_SIZE_DEFAULT;		// adjustment for waterfall size
	ts.fft_window_type = FFT_WINDOW_DEFAULT;			// FFT Windowing type
	ts.dvmode = 0;							// disable "DV" mode RX/TX functions by default
	ts.tx_audio_muting_timing = 0;			// timing value used for muting TX audio when keying PTT to suppress "click" or "thump"
	ts.tx_audio_muting_timer = 0;			// timer used for muting TX audio when keying PTT to suppress "click" or "thump"
	ts.tx_audio_muting_flag = 0;			// when TRUE, audio is to be muted after PTT/keyup
//!	ts.filter_disp_colour = FILTER_DISP_COLOUR_DEFAULT;	//
	ts.vfo_mem_flag = 0;					// when TRUE, memory mode is enabled
	ts.mem_disp = 0;						// when TRUE, memory display is enabled
}

static void radio_init_misc_values(void)
{
	// Set clocks in use - fix in HW init!!
	//--tsu.main_clk = EXT_16MHZ_XTAL;
	//--tsu.rcc_clk = EXT_32KHZ_XTAL;

	tsu.use_full_span		= 0;	// use frequency translate

	// DSP status
	tsu.dsp_alive 			= 0;
	tsu.dsp_seq_number_old 	= 0;
	tsu.dsp_seq_number	 	= 0;	// track if alive
	tsu.dsp_freq			= 0;	// set as zero ?
	tsu.dsp_volume			= 0;	// set as zero ?

	// Local status
	//tsu.audio_volume		= START_UP_AUDIO_VOL;
	//tsu.vfo_a 				= 0xFFFFFFFF;			// Invalid, do not update DSP
	//tsu.vfo_b 				= 0xFFFFFFFF;			// Invalid, do not update DSP
	//tsu.active_vfo			= VFO_A;
	tsu.step				= 0xFFFFFFFF;			// Invalid, do not update DSP
	//tsu.demod_mode 			= 0xFF;					// Invalid, do not update DSP
	tsu.curr_band			= 0xFF;					// Invalid, do not update DSP
	//tsu.curr_filter			= 0xFF;					// Invalid, do not update DSP
	tsu.step_idx			= 0xFF;					// Invalid, do not update DSP
	//tsu.nco_freq			= 0;					// NCO -20kHz to +20kHz, zero disables translation routines in the the DSP
	//tsu.fixed_mode			= 0;					// vfo in centre mode
	tsu.cw_tx_state			= 0;					// rx mode
	tsu.cw_iamb_type		= 0;					// iambic type, nothing selected

	// DSP API requests
	//tsu.update_audio_dsp_req 	= 0;
	//tsu.update_freq_dsp_req  	= 0;
	//tsu.update_band_dsp_req		= 0;
	//tsu.update_demod_dsp_req	= 0;
	//tsu.update_filter_dsp_req	= 0;
	//tsu.update_nco_dsp_req		= 0;
	//tsu.update_dsp_eep_req 		= 0;
	//tsu.update_dsp_restart 		= 0;

	// Mute off
	tsu.audio_mute_flag = 0;

	// UI values
	tsu.demo_mode 		= 0;
	tsu.brightness		= 80;
	tsu.smet_type		= 0;
}

//*----------------------------------------------------------------------------
//* Function Name       : radio_init_eep_defaults
//* Object              :
//* Input Parameters    :
//* Output Parameters   :
//* Functions called    :
//*----------------------------------------------------------------------------
void radio_init_eep_defaults(void)
{
	ulong i;

	printf("load eep defaults \r\n");

	// Generate band info values
	for(i = 0; i < MAX_BANDS; i++)
	{
		// Startup volume
		tsu.band[i].volume 			= 0;
		tsu.band[i].audio_balance 	= 8;
		tsu.band[i].st_volume		= 3;
		tsu.band[i].step 			= T_STEP_1KHZ;
		tsu.band[i].filter			= AUDIO_3P6KHZ;
		tsu.band[i].atten	 		= ATTEN_0DB;
		tsu.band[i].nco_freq		= 0;
		tsu.band[i].active_vfo		= VFO_A;
		tsu.band[i].fixed_mode		= 0;
		tsu.band[i].span			= 40000;

		// VFOs values
		switch(i)
		{
			case BAND_MODE_2200:
				tsu.band[i].vfo_a 		= BAND_FREQ_2200;
				tsu.band[i].vfo_b 		= BAND_FREQ_2200;
				tsu.band[i].band_start 	= BAND_FREQ_2200;
				tsu.band[i].band_end 	= (BAND_FREQ_2200 + BAND_SIZE_2200);
				tsu.band[i].demod_mode	= DEMOD_LSB;
				tsu.band[i].power_factor= 0;
				tsu.band[i].tx_power	= 0xFF;
				break;

			case BAND_MODE_630:
				tsu.band[i].vfo_a 		= BAND_FREQ_630;
				tsu.band[i].vfo_b 		= BAND_FREQ_630;
				tsu.band[i].band_start 	= BAND_FREQ_630;
				tsu.band[i].band_end 	= (BAND_FREQ_630 + BAND_SIZE_630);
				tsu.band[i].demod_mode	= DEMOD_LSB;
				tsu.band[i].power_factor= 0;
				tsu.band[i].tx_power	= 0xFF;
				break;

			case BAND_MODE_160:
				tsu.band[i].vfo_a 		= BAND_FREQ_160;
				tsu.band[i].vfo_b 		= BAND_FREQ_160;
				tsu.band[i].band_start 	= BAND_FREQ_160;
				tsu.band[i].band_end 	= (BAND_FREQ_160 + BAND_SIZE_160);
				tsu.band[i].demod_mode	= DEMOD_LSB;
				tsu.band[i].power_factor= TX_POWER_FACTOR_160_DEFAULT;
				tsu.band[i].tx_power	= PA_LEVEL_0_5W;
				break;

			case BAND_MODE_80:
				tsu.band[i].vfo_a 		= BAND_FREQ_80;
				tsu.band[i].vfo_b 		= BAND_FREQ_80;
				tsu.band[i].band_start 	= BAND_FREQ_80;
				tsu.band[i].band_end 	= (BAND_FREQ_80 + BAND_SIZE_80);
				tsu.band[i].demod_mode	= DEMOD_LSB;
				tsu.band[i].power_factor= TX_POWER_FACTOR_80_DEFAULT;
				tsu.band[i].tx_power	= PA_LEVEL_0_5W;
				break;

			case BAND_MODE_60:
				tsu.band[i].vfo_a 		= BAND_FREQ_60;
				tsu.band[i].vfo_b 		= BAND_FREQ_60;
				tsu.band[i].band_start 	= BAND_FREQ_60;
				tsu.band[i].band_end 	= (BAND_FREQ_60 + BAND_SIZE_60);
				tsu.band[i].demod_mode	= DEMOD_LSB;
				tsu.band[i].power_factor= TX_POWER_FACTOR_60_DEFAULT;
				tsu.band[i].tx_power	= PA_LEVEL_0_5W;
				break;

			case BAND_MODE_40:
				tsu.band[i].vfo_a 		= BAND_FREQ_40;
				tsu.band[i].vfo_b 		= BAND_FREQ_40;
				tsu.band[i].band_start 	= BAND_FREQ_40;
				tsu.band[i].band_end 	= (BAND_FREQ_40 + BAND_SIZE_40);
				tsu.band[i].demod_mode	= DEMOD_LSB;
				tsu.band[i].power_factor= TX_POWER_FACTOR_40_DEFAULT;
				tsu.band[i].tx_power	= PA_LEVEL_0_5W;
				break;

			case BAND_MODE_30:
				tsu.band[i].vfo_a 		= BAND_FREQ_30;
				tsu.band[i].vfo_b 		= BAND_FREQ_30;
				tsu.band[i].band_start 	= BAND_FREQ_30;
				tsu.band[i].band_end 	= (BAND_FREQ_30 + BAND_SIZE_30);
				tsu.band[i].demod_mode	= DEMOD_USB;
				tsu.band[i].power_factor= TX_POWER_FACTOR_30_DEFAULT;
				tsu.band[i].tx_power	= PA_LEVEL_0_5W;
				break;

			case BAND_MODE_20:
				tsu.band[i].vfo_a 		= BAND_FREQ_20;
				tsu.band[i].vfo_b 		= BAND_FREQ_20;
				tsu.band[i].band_start 	= BAND_FREQ_20;
				tsu.band[i].band_end 	= (BAND_FREQ_20 + BAND_SIZE_20);
				tsu.band[i].demod_mode	= DEMOD_USB;
				tsu.band[i].power_factor= TX_POWER_FACTOR_20_DEFAULT;
				tsu.band[i].tx_power	= PA_LEVEL_0_5W;
				break;

			case BAND_MODE_17:
				tsu.band[i].vfo_a 		= BAND_FREQ_17;
				tsu.band[i].vfo_b 		= BAND_FREQ_17;
				tsu.band[i].band_start 	= BAND_FREQ_17;
				tsu.band[i].band_end 	= (BAND_FREQ_17 + BAND_SIZE_17);
				tsu.band[i].demod_mode	= DEMOD_USB;
				tsu.band[i].power_factor= TX_POWER_FACTOR_17_DEFAULT;
				tsu.band[i].tx_power	= PA_LEVEL_0_5W;
				break;

			case BAND_MODE_15:
				tsu.band[i].vfo_a 		= BAND_FREQ_15;
				tsu.band[i].vfo_b 		= BAND_FREQ_15;
				tsu.band[i].band_start 	= BAND_FREQ_15;
				tsu.band[i].band_end 	= (BAND_FREQ_15 + BAND_SIZE_15);
				tsu.band[i].demod_mode	= DEMOD_USB;
				tsu.band[i].power_factor= TX_POWER_FACTOR_15_DEFAULT;
				tsu.band[i].tx_power	= PA_LEVEL_0_5W;
				break;

			case BAND_MODE_12:
				tsu.band[i].vfo_a 		= BAND_FREQ_12;
				tsu.band[i].vfo_b 		= BAND_FREQ_12;
				tsu.band[i].band_start 	= BAND_FREQ_12;
				tsu.band[i].band_end 	= (BAND_FREQ_12 + BAND_SIZE_12);
				tsu.band[i].demod_mode	= DEMOD_USB;
				tsu.band[i].power_factor= TX_POWER_FACTOR_12_DEFAULT;
				tsu.band[i].tx_power	= PA_LEVEL_0_5W;
				break;

			case BAND_MODE_10:
				tsu.band[i].vfo_a 		= BAND_FREQ_10;
				tsu.band[i].vfo_b 		= BAND_FREQ_10;
				tsu.band[i].band_start 	= BAND_FREQ_10;
				tsu.band[i].band_end 	= (BAND_FREQ_10 + BAND_SIZE_10);
				tsu.band[i].demod_mode	= DEMOD_USB;
				tsu.band[i].power_factor= TX_POWER_FACTOR_10_DEFAULT;
				tsu.band[i].tx_power	= PA_LEVEL_0_5W;
				break;

			case BAND_MODE_GEN:
				tsu.band[i].vfo_a 		= BAND_FREQ_GEN;
				tsu.band[i].vfo_b 		= BAND_FREQ_GEN;
				tsu.band[i].band_start 	= BAND_FREQ_GEN;
				tsu.band[i].band_end 	= (BAND_FREQ_GEN + BAND_SIZE_GEN);
				tsu.band[i].demod_mode	= DEMOD_AM;
				tsu.band[i].power_factor= 0;
				tsu.band[i].tx_power	= 0xFF;
				break;
			default:
				break;
		}
	}

	// Save
	save_band_info();

	// Save system defaults
	virt_eeprom_write(EEP_CURR_BAND, BAND_MODE_20);

	// Save UI defaults
	virt_eeprom_write(EEP_SW_SMOOTH,   0);
	virt_eeprom_write(EEP_DEMO_MODE,   0);
	virt_eeprom_write(EEP_BRIGHTNESS, 80);
	virt_eeprom_write(EEP_SMET_TYPE,   0);
	virt_eeprom_write(EEP_AGC_MODE,   AGC_MED);
	virt_eeprom_write(EEP_RF_GAIN,    30);

	// Generate checksum
	ulong chk = radio_init_eep_chksum();

	// Save it
	virt_eeprom_write(EEP_BASE_ADDR + 1, chk >>  16);
	virt_eeprom_write(EEP_BASE_ADDR + 2, chk >>   8);
	virt_eeprom_write(EEP_BASE_ADDR + 3, chk & 0xFF);

	// Set as initialised
	virt_eeprom_write(EEP_BASE_ADDR, 0x73);
}

//*----------------------------------------------------------------------------
//* Function Name       : radio_init_ui_to_dsp
//* Object              :
//* Object              : Copy local params to DSP struct
//* Input Parameters    :
//* Output Parameters   :
//* Functions called    :
//*----------------------------------------------------------------------------
void radio_init_ui_to_dsp(void)
{
	ts.txrx_mode 		= TRX_MODE_RX;
	ts.api_band 		= 0;
	ts.api_iamb_type 	= 0;

	ts.dmod_mode 		= tsu.band[tsu.curr_band].demod_mode;
	ts.filter_id 		= tsu.band[tsu.curr_band].filter;

	ts.audio_gain 		= tsu.band[tsu.curr_band].volume;

	#if 0
	ts.agc_mode			= AGC_OFF;
	ts.rf_gain			= 25;
	#else
	ts.agc_mode			= AGC_MED;
	ts.rf_gain			= 30;
	#endif

	ts.nco_freq			= tsu.band[tsu.curr_band].nco_freq;
}

uchar radio_init_default_mode_from_band(void)
{
	if(tsu.curr_band < BAND_MODE_30)
		return DEMOD_LSB;
	else
		return DEMOD_USB;
}

void radio_init_show_current_demod_mode(uchar mode)
{
	switch(mode)
	{
		case DEMOD_USB:
			printf("mode is USB\r\n");
			break;
		case DEMOD_LSB:
			printf("mode is LSB\r\n");
			break;
		case DEMOD_CW:
			printf("mode is CW\r\n");
			break;
		case DEMOD_AM:
			printf("mode is AM\r\n");
			break;
		case DEMOD_FM:
			printf("mode is FM\r\n");
			break;
		default:
			printf("mode is Unknown\r\n");
			break;
	}
}

void radio_init_save_before_off(void)
{
	#if 0
	printf("curr band: %d\r\n",tsu.curr_band);
	printf("demo mode: %d\r\n",tsu.demo_mode);
	printf("smet type: %d\r\n",tsu.smet_type);
	printf("brightnes: %d\r\n",tsu.brightness);
	#endif

	// Save
	save_band_info();

	// Save system defaults
	virt_eeprom_write(EEP_CURR_BAND, tsu.curr_band);

	// Save UI defaults
	virt_eeprom_write(EEP_SW_SMOOTH,  0);
	virt_eeprom_write(EEP_DEMO_MODE,  tsu.demo_mode);
	virt_eeprom_write(EEP_BRIGHTNESS, tsu.brightness);
	virt_eeprom_write(EEP_SMET_TYPE,  tsu.smet_type);
	virt_eeprom_write(EEP_AGC_MODE,   tsu.agc_mode);
	virt_eeprom_write(EEP_RF_GAIN,    tsu.rf_gain);

	// Generate checksum
	ulong chk = radio_init_eep_chksum();

	// Save it
	virt_eeprom_write(EEP_BASE_ADDR + 1, chk >>  16);
	virt_eeprom_write(EEP_BASE_ADDR + 2, chk >>   8);
	virt_eeprom_write(EEP_BASE_ADDR + 3, chk & 0xFF);

	// Set as initialised
	virt_eeprom_write(EEP_BASE_ADDR, 0x73);

	HAL_PWR_DisableBkUpAccess();
	//--HAL_PWREx_EnableBkUpReg();
}

//*----------------------------------------------------------------------------
//* Function Name       : radio_init_on_reset
//* Object              :
//* Object              :
//* Input Parameters    :
//* Output Parameters   :
//* Functions called    :
//*----------------------------------------------------------------------------
void radio_init_on_reset(void)
{
	int res;

	// Eeprom flags
	tsu.eeprom_init_done	= 0;
	//tsu.eeprom_data_valid 	= 0;

	radio_init_misc_values();

	// Enable virtual eeprom
	virt_eeprom_init();

	// Init pub struct
	res = radio_init_load_eep_values();

	// ToDo: Are we going to send those to the DSP core at all ?
	radio_init_load_dsp_values();

	if(res == 0)
		return;

	// Restore eeprom
	radio_init_eep_defaults();

	// By default side encoder changes audio volume
	tsu.active_side_enc_id 	= 0;
	tsu.stereo_mode 		= 0;		// mono by default
	tsu.agc_mode			= AGC_MED;
	tsu.rf_gain				= 30;
	tsu.keyer_mode 			= ts.keyer_mode;
	tsu.tune				= 0;
	tsu.bias0				= 0;
	tsu.bias1				= 0;

	// Enforce 20m on eeprom on error
	tsu.curr_band 						= BAND_MODE_20;
	tsu.band[tsu.curr_band].volume 		= 0;
	tsu.band[tsu.curr_band].active_vfo  = VFO_A;
	tsu.band[tsu.curr_band].vfo_a 		= 14074*1000 + 000;
	tsu.band[tsu.curr_band].fixed_mode 	= 0;
	tsu.band[tsu.curr_band].nco_freq	= 0;
	tsu.band[tsu.curr_band].demod_mode	= DEMOD_USB;

	// Enforce 80m - test
	#if 0
	tsu.curr_band 						= BAND_MODE_80;
	tsu.band[tsu.curr_band].volume 		= 10;
	tsu.band[tsu.curr_band].vfo_a 		= 3697*1000;
	tsu.band[tsu.curr_band].fixed_mode 	= 0;
	tsu.band[tsu.curr_band].nco_freq	= 0;
	tsu.band[tsu.curr_band].demod_mode	= DEMOD_LSB;
	tsu.band[tsu.curr_band].filter		= AUDIO_3P6KHZ;
	#endif

	// Enforce 40m - test
	#if 0
	tsu.curr_band 						= BAND_MODE_40;
	tsu.band[tsu.curr_band].volume 		= 11;
	tsu.band[tsu.curr_band].vfo_a 		= 7147*1000;
	tsu.band[tsu.curr_band].fixed_mode 	= 0;
	tsu.band[tsu.curr_band].nco_freq	= 0;
	tsu.band[tsu.curr_band].demod_mode	= DEMOD_LSB;
	#endif
}
