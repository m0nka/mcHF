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
// Common
#include "mchf_board.h"

#include "audio_sai.h"
#include "cw_gen.h"

#include <limits.h>

#include "dsp_proc.h"
#include "dsp_idle_proc.h"

// Encoders
#include "ui_rotary.h"

#include "softdds.h"

// IIR lattice ARMA filters with time-reversed elements
//
#include "filters/iir_300hz.h"
#include "filters/iir_500hz.h"
#include "filters/iir_1_8k.h"
#include "filters/iir_2_3k.h"
#include "filters/iir_3_6k.h"
#include "filters/iir_10k.h"

#include "filters/iir_2k7_tx_bpf.h"
//
#include "filters/fir_rx_decimate_4.h"	// with low-pass filtering
#include "filters/fir_rx_decimate_4_min_lpf.h"	// This has minimized LPF for the 10 kHz filter mode
#include "filters/fir_rx_interpolate_16.h"	// filter for interpolate-by-16 operation
#include "filters/fir_rx_interpolate_16_10kHz.h"	// This has relaxed LPF for the 10 kHz filter mode

static void Audio_Init(void);

// ---------------------------------
// DMA buffers for I2S
//__IO int16_t 	tx_buffer[BUFF_LEN+1];	- moved to hw/audio_sai.c !
//__IO int16_t	rx_buffer[BUFF_LEN+1];

//static int16_t	test_a[1000];	// grab a large chunk of RAM - for testing, and to prevent "memory leak" anomalies (kludgy work-around - problem to be solved!)
//
float32_t	lms1_nr_delay[LMS_NR_DELAYBUF_SIZE_MAX+16];
//
float32_t	lms2_nr_delay[LMS_NOTCH_DELAYBUF_SIZE_MAX + 16];
//
//
float32_t errsig1[64+10];
float32_t errsig2[64+10];
float32_t result[64+10];
//
// LMS Filters for RX
arm_lms_norm_instance_f32	lms1Norm_instance;
arm_lms_instance_f32	lms1_instance;
float32_t	lms1StateF32[DSP_NR_NUMTAPS_MAX + BUFF_LEN];
float32_t	lms1NormCoeff_f32[DSP_NR_NUMTAPS_MAX + BUFF_LEN];
//
//static int16_t	test_j[1000];	// grab a large chunk of RAM - for testing, and to prevent "memory leak" anomalies (kludgy work-around)
//
arm_lms_norm_instance_f32	lms2Norm_instance;
arm_lms_instance_f32	lms2_instance;
float32_t	lms2StateF32[DSP_NOTCH_NUMTAPS_MAX + BUFF_LEN];
float32_t	lms2NormCoeff_f32[DSP_NOTCH_NUMTAPS_MAX + BUFF_LEN];
//
//
//
float32_t	agc_delay	[AGC_DELAY_BUFSIZE+16];
//
//
//static int16_t	test_c[5000];	// grab a large chunk of RAM - for testing, and to prevent "memory leak" anomalies (kludgy work-around - problem to be solved!)
//static int16_t	test_c[1000];	// grab a large chunk of RAM - for testing, and to prevent "memory leak" anomalies (kludgy work-around - problem to be solved!)
//
// Audio RX - Decimator
static	arm_fir_decimate_instance_f32	DECIMATE_RX;
float32_t			decimState[FIR_RXAUDIO_BLOCK_SIZE + FIR_RXAUDIO_NUM_TAPS];
//
//static int16_t	test_d[1000];	// grab a large chunk of RAM - for testing, and to prevent "memory leak" anomalies (kludgy work-around - problem to be solved!)
//
// Audio RX - Interpolator
static	arm_fir_interpolate_instance_f32 INTERPOLATE_RX;
float32_t			interpState[FIR_RXAUDIO_BLOCK_SIZE + FIR_RXAUDIO_NUM_TAPS];
//
//static int16_t	test_e[1000];	// grab a large chunk of RAM - for testing, and to prevent "memory leak" anomalies (kludgy work-around - problem to be solved!)
//
// variables for RX IIR filters
static float32_t		iir_rx_state[FIR_RXAUDIO_BLOCK_SIZE + FIR_RXAUDIO_NUM_TAPS];
static arm_iir_lattice_instance_f32	IIR_PreFilter;
//
//static int16_t	test_f[1000];	// grab a large chunk of RAM - for testing, and to prevent "memory leak" anomalies (kludgy work-around - problem to be solved!)
//
//
// variables for TX IIR filter
//
float32_t		iir_tx_state[FIR_RXAUDIO_BLOCK_SIZE + FIR_RXAUDIO_NUM_TAPS];
arm_iir_lattice_instance_f32	IIR_TXFilter;
//
//static int16_t	test_b[1000];	// grab a large chunk of RAM - for testing, and to prevent "memory leak" anomalies (kludgy work-around - problem to be solved!)
//
// RX Hilbert transform (90 degree) FIR filters
//

__IO	arm_fir_instance_f32 	FIR_I;
__IO	arm_fir_instance_f32 	FIR_Q;
//
__IO	arm_fir_instance_f32	FIR_I_TX;
__IO	arm_fir_instance_f32	FIR_Q_TX;
// ---------------------------------

// Transceiver state public structure
extern __IO TransceiverState 	ts;

// Public paddle state
//! extern __IO PaddleState			ps;

// ------------------------------------------------
// Frequency public
extern __IO DialFrequency 				df;

// Spectrum display public
__IO	SpectrumDisplay			sd;

// Audio driver publics
//__IO
AudioDriverState		ads;

// S meter public
__IO	SMeter					sm;
//
// Keypad driver publics
//! extern __IO	KeypadState				ks;
//
__IO	FilterCoeffs		fc;
//

uchar current_filter;

//
//*----------------------------------------------------------------------------
//* Function Name       : audio_driver_config_nco
//* Object              :
//* Object              :
//* Input Parameters    :
//* Output Parameters   :
//* Functions called    :
//*----------------------------------------------------------------------------
short loc_nco_freq = 0;
void audio_driver_config_nco(void)
{
#if 0
	// Configure NCO for the frequency translate function - NOT USED for the "Static" local oscillator!
	// see "audio_driver.h" for values
	ads.Osc_Cos = CONV_NCO_COS;
	ads.Osc_Sin = CONV_NCO_SIN;
	ads.Osc_Vect_Q = 1;
	ads.Osc_Vect_I = 0;
	ads.Osc_Gain = 0;
	ads.Osc_Q = 0;
	ads.Osc_I = 0;
#endif

	if(loc_nco_freq == df.nco_freq)
		return;

	loc_nco_freq = df.nco_freq;

	float rate = 2 * PI * (loc_nco_freq) / ts.samp_rate;//(48000);

	ads.Osc_Cos = cos(rate);
	ads.Osc_Sin = sin(rate);
	ads.Osc_Vect_Q = 1;
	ads.Osc_Vect_I = 0;
	ads.Osc_Gain = 0;
	ads.Osc_Q = 0;
	ads.Osc_I = 0;
}

//*----------------------------------------------------------------------------
//* Function Name       : audio_driver_init
//* Object              :
//* Object              :
//* Input Parameters    :
//* Output Parameters   :
//* Functions called    :
//*----------------------------------------------------------------------------
uchar audio_driver_init(void)
{
	uchar x, res;

	static uchar audio_init_done = 0;

	if(audio_init_done)
		return 0;

	//printf("audio driver init...\r\n");

	// Init Soft DDS
	softdds_setfreq(0.0, ts.samp_rate, 0);

	// CW module init
	cw_gen_init();

	// Audio filter disabled
	ads.af_dissabled = 1;

	// Reset S meter public
	sm.skip		= 0;
	sm.s_count	= 0;
	sm.curr_max	= 0;
	sm.gain_calc = 0;	// gain calculation used for S-meter

	ads.agc_val = 1;			// Post AF Filter gain (AGC)
	ads.agc_holder = 1;			// initialize holder for AGC value
	for(x = 0; x < BUFF_LEN; x++)	// initialize running buffer for AGC delay
		ads.agc_valbuf[x] = 1;
	//
	ads.alc_val = 1;			// init TX audio auto-level-control (ALC)
	//
	ads.decimation_rate	=	ts.samp_rate/4;		//RX_DECIMATION_RATE_12KHZ;		// Decimation rate, when enabled

	//
	UiCalcAGCDecay();	// initialize AGC decay ("hang time") values
	//
	UiCalcRFGain();		// convert from user RF gain value to "working" RF gain value
	//
	UiCalcALCDecay();	// initialize ALC decay values
	//
	UiCalcAGCVals();	// calculate AGC internal values from user settings
	//
	UiCalcNB_AGC();		// set up noise blanker AGC values
	//
	UiCWSidebandMode();	// set up CW sideband mode setting
	//
	// The "active" NCO in the frequency translate function is NOT used, but rather a "static" sine that is an integer divisor of the sample rate.
	//
	//#ifdef DSP_MODE
	//	audio_driver_config_nco();	// Configure the NCO in the frequency translate function
	//#endif
	//
	ads.tx_filter_adjusting = 0;	// used to disable TX I/Q filter during adjustment

	// Audio init
	Audio_Init();

	// SAI hardware init/Start DMA transfers
	res = audio_sai_hw_init();

	// Audio filter enabled
	ads.af_dissabled = 0;
	current_filter = ts.filter_id;

	if(res == 0)
	{
		// Prevent multiple init
		audio_init_done = 1;
	}

	return res;
}

//*----------------------------------------------------------------------------
//* Function Name       : audio_driver_stop
//* Object              :
//* Object              :
//* Input Parameters    :
//* Output Parameters   :
//* Functions called    :
//*----------------------------------------------------------------------------
void audio_driver_stop(void)
{
//!	I2S_Block_Stop();
}

//*----------------------------------------------------------------------------
//* Function Name       : audio_driver_thread
//* Object              :
//* Object              :
//* Input Parameters    :
//* Output Parameters   :
//* Functions called    :
//*----------------------------------------------------------------------------
void audio_driver_thread(void)
{
	if(current_filter != ts.filter_id)
	{
		audio_driver_set_rx_audio_filter();
		current_filter = ts.filter_id;
	}
}

//*----------------------------------------------------------------------------
//* Function Name       : audio_driver_set_rx_audio_filter
//* Object              :
//* Object              : select audio filter
//* Input Parameters    :
//* Output Parameters   :
//* Functions called    :
//*----------------------------------------------------------------------------
//
// WARNING:  You CANNOT reliably use the built-in IIR and FIR "init" functions when using CONST-based coefficient tables!  If you do so, you risk filters
//	not initializing properly!  If you use the "init" functions, you MUST copy CONST-based coefficient tables to RAM first!
//  This information is from recommendations by online references for using ARM math/DSP functions
//

void audio_driver_set_rx_audio_filter(void)
{
	uint32_t	i;
	float	mu_calc;
	bool	dsp_inhibit_temp;

	// Lock - temporarily disable filter

	dsp_inhibit_temp = ts.dsp_inhibit;
	ts.dsp_inhibit = 1;	// disable DSP while doing adjustment
	ads.af_dissabled = 1;

	switch(ts.filter_id)	{
		case AUDIO_300HZ:
		    IIR_PreFilter.numStages = IIR_300hz_numStages;		// number of stages
		    if(ts.filter_300Hz_select == 1)	{
				IIR_PreFilter.pkCoeffs = (float *)IIR_300hz_500_pkCoeffs;	// point to reflection coefficients
				IIR_PreFilter.pvCoeffs = (float *)IIR_300hz_500_pvCoeffs;	// point to ladder coefficients
		    }
		    else if(ts.filter_300Hz_select == 2)	{
				IIR_PreFilter.pkCoeffs = (float *)IIR_300hz_550_pkCoeffs;	// point to reflection coefficients
				IIR_PreFilter.pvCoeffs = (float *)IIR_300hz_550_pvCoeffs;	// point to ladder coefficients
		    }
		    else if(ts.filter_300Hz_select == 3)	{
				IIR_PreFilter.pkCoeffs = (float *)IIR_300hz_600_pkCoeffs;	// point to reflection coefficients
				IIR_PreFilter.pvCoeffs = (float *)IIR_300hz_600_pvCoeffs;	// point to ladder coefficients
		    }
		    else if(ts.filter_300Hz_select == 4)	{
				IIR_PreFilter.pkCoeffs = (float *)IIR_300hz_650_pkCoeffs;	// point to reflection coefficients
				IIR_PreFilter.pvCoeffs = (float *)IIR_300hz_650_pvCoeffs;	// point to ladder coefficients
		    }
		    else if(ts.filter_300Hz_select == 5)	{
				IIR_PreFilter.pkCoeffs = (float *)IIR_300hz_700_pkCoeffs;	// point to reflection coefficients
				IIR_PreFilter.pvCoeffs = (float *)IIR_300hz_700_pvCoeffs;	// point to ladder coefficients
		    }
		    else if(ts.filter_300Hz_select == 7)	{
				IIR_PreFilter.pkCoeffs = (float *)IIR_300hz_800_pkCoeffs;	// point to reflection coefficients
				IIR_PreFilter.pvCoeffs = (float *)IIR_300hz_800_pvCoeffs;	// point to ladder coefficients
		    }
		    else if(ts.filter_300Hz_select == 8)	{
				IIR_PreFilter.pkCoeffs = (float *)IIR_300hz_850_pkCoeffs;	// point to reflection coefficients
				IIR_PreFilter.pvCoeffs = (float *)IIR_300hz_850_pvCoeffs;	// point to ladder coefficients
		    }
		    else if(ts.filter_300Hz_select == 9)	{
				IIR_PreFilter.pkCoeffs = (float *)IIR_300hz_900_pkCoeffs;	// point to reflection coefficients
				IIR_PreFilter.pvCoeffs = (float *)IIR_300hz_900_pvCoeffs;	// point to ladder coefficients
		    }
		    else if(ts.filter_300Hz_select == 10)	{
				IIR_PreFilter.pkCoeffs = (float *)IIR_300hz_950_pkCoeffs;	// point to reflection coefficients
				IIR_PreFilter.pvCoeffs = (float *)IIR_300hz_950_pvCoeffs;	// point to ladder coefficients
		    }
		    else	{	// default value
				IIR_PreFilter.pkCoeffs = (float *)IIR_300hz_750_pkCoeffs;	// point to reflection coefficients
				IIR_PreFilter.pvCoeffs = (float *)IIR_300hz_750_pvCoeffs;	// point to ladder coefficients
		    }
			break;
		case AUDIO_500HZ:
		    IIR_PreFilter.numStages = IIR_500hz_numStages;		// number of stages
		    if(ts.filter_500Hz_select == 1)	{
				IIR_PreFilter.pkCoeffs = (float *)IIR_500hz_550_pkCoeffs;	// point to reflection coefficients
				IIR_PreFilter.pvCoeffs = (float *)IIR_500hz_550_pvCoeffs;	// point to ladder coefficients
		    }
		    else if(ts.filter_500Hz_select == 2)	{
				IIR_PreFilter.pkCoeffs = (float *)IIR_500hz_650_pkCoeffs;	// point to reflection coefficients
				IIR_PreFilter.pvCoeffs = (float *)IIR_500hz_650_pvCoeffs;	// point to ladder coefficients
		    }
		    else if(ts.filter_500Hz_select == 4)	{
				IIR_PreFilter.pkCoeffs = (float *)IIR_500hz_850_pkCoeffs;	// point to reflection coefficients
				IIR_PreFilter.pvCoeffs = (float *)IIR_500hz_850_pvCoeffs;	// point to ladder coefficients
		    }
		    else if(ts.filter_500Hz_select == 5)	{
				IIR_PreFilter.pkCoeffs = (float *)IIR_500hz_950_pkCoeffs;	// point to reflection coefficients
				IIR_PreFilter.pvCoeffs = (float *)IIR_500hz_950_pvCoeffs;	// point to ladder coefficients
		    }
		    else	{	// default value
				IIR_PreFilter.pkCoeffs = (float *)IIR_500hz_750_pkCoeffs;	// point to reflection coefficients
				IIR_PreFilter.pvCoeffs = (float *)IIR_500hz_750_pvCoeffs;	// point to ladder coefficients
		    }
			break;
		case AUDIO_1P8KHZ:
		    IIR_PreFilter.numStages = IIR_1k8_numStages;		// number of stages
		    if(ts.filter_1k8_select == 1)	{
				IIR_PreFilter.pkCoeffs = (float *)IIR_1k8_1k125_pkCoeffs;	// point to reflection coefficients
				IIR_PreFilter.pvCoeffs = (float *)IIR_1k8_1k125_pvCoeffs;	// point to ladder coefficients
		    }
		    else if(ts.filter_1k8_select == 2)	{
				IIR_PreFilter.pkCoeffs = (float *)IIR_1k8_1k275_pkCoeffs;	// point to reflection coefficients
				IIR_PreFilter.pvCoeffs = (float *)IIR_1k8_1k275_pvCoeffs;	// point to ladder coefficients
		    }
		    else if(ts.filter_1k8_select == 4)	{
				IIR_PreFilter.pkCoeffs = (float *)IIR_1k8_1k575_pkCoeffs;	// point to reflection coefficients
				IIR_PreFilter.pvCoeffs = (float *)IIR_1k8_1k575_pvCoeffs;	// point to ladder coefficients
		    }
		    else if(ts.filter_1k8_select == 5)	{
				IIR_PreFilter.pkCoeffs = (float *)IIR_1k8_1k725_pkCoeffs;	// point to reflection coefficients
				IIR_PreFilter.pvCoeffs = (float *)IIR_1k8_1k725_pvCoeffs;	// point to ladder coefficients
		    }
		    else	{	// default value
				IIR_PreFilter.pkCoeffs = (float *)IIR_1k8_1k425_pkCoeffs;	// point to reflection coefficients
				IIR_PreFilter.pvCoeffs = (float *)IIR_1k8_1k425_pvCoeffs;	// point to ladder coefficients
		    }
			break;
		case AUDIO_2P3KHZ:
		    IIR_PreFilter.numStages = IIR_2k3_numStages;		// number of stages
		    if(ts.filter_2k3_select == 1)	{
				IIR_PreFilter.pkCoeffs = (float *)IIR_2k3_1k275_pkCoeffs;	// point to reflection coefficients
				IIR_PreFilter.pvCoeffs = (float *)IIR_2k3_1k275_pvCoeffs;	// point to ladder coefficients
		    }
		    else if(ts.filter_2k3_select == 3)	{
				IIR_PreFilter.pkCoeffs = (float *)IIR_2k3_1k562_pkCoeffs;	// point to reflection coefficients
				IIR_PreFilter.pvCoeffs = (float *)IIR_2k3_1k562_pvCoeffs;	// point to ladder coefficients
		    }
		    else if(ts.filter_2k3_select == 4)	{
				IIR_PreFilter.pkCoeffs = (float *)IIR_2k3_1k712_pkCoeffs;	// point to reflection coefficients
				IIR_PreFilter.pvCoeffs = (float *)IIR_2k3_1k712_pvCoeffs;	// point to ladder coefficients
		    }
		    else	{	// default value
				IIR_PreFilter.pkCoeffs = (float *)IIR_2k3_1k412_pkCoeffs;	// point to reflection coefficients
				IIR_PreFilter.pvCoeffs = (float *)IIR_2k3_1k412_pvCoeffs;	// point to ladder coefficients
		    }
			break;
		case AUDIO_3P6KHZ:
		    IIR_PreFilter.numStages = IIR_3k6_numStages;		// number of stages
			IIR_PreFilter.pkCoeffs = (float *)IIR_3k6_pkCoeffs;	// point to reflection coefficients
			IIR_PreFilter.pvCoeffs = (float *)IIR_3k6_pvCoeffs;	// point to ladder coefficients
			break;
		case AUDIO_WIDE:
		    IIR_PreFilter.numStages = IIR_10k_numStages;		// number of stages
			IIR_PreFilter.pkCoeffs = (float *)IIR_10k_pkCoeffs;	// point to reflection coefficients
			IIR_PreFilter.pvCoeffs = (float *)IIR_10k_pvCoeffs;	// point to ladder coefficients
			break;
		default:
			break;
	}
	//
	// Initialize IIR filter state buffer
 	//
    for(i = 0; i < FIR_RXAUDIO_BLOCK_SIZE+FIR_RXAUDIO_NUM_TAPS-1; i++)	{	// initialize state buffer to zeroes
    	iir_rx_state[i] = 0;
    }
	IIR_PreFilter.pState = (float32_t *)&iir_rx_state;					// point to state array for IIR filter

	//
	// Initialize LMS (DSP Noise reduction) filter
	// It is (sort of) initalized "twice" since this it what it seems to take for the LMS function to
	// start reliably and consistently!
	//
	uint16_t	calc_taps;
	//
	if((ts.dsp_nr_numtaps < DSP_NR_NUMTAPS_MIN) || (ts.dsp_nr_numtaps > DSP_NR_NUMTAPS_MAX))
		calc_taps = DSP_NR_NUMTAPS_DEFAULT;
	else
		calc_taps = (uint16_t)ts.dsp_nr_numtaps;
	//
	// Load settings into instance structure
	//
	// LMS instance 1 is pre-AGC DSP NR
	// LMS instance 3 is post-AGC DSP NR
	//
	lms1Norm_instance.numTaps = calc_taps;
	lms1Norm_instance.pCoeffs = lms1NormCoeff_f32;
	lms1Norm_instance.pState = lms1StateF32;
	//
	// Calculate "mu" (convergence rate) from user "DSP Strength" setting.  This needs to be significantly de-linearized to
	// squeeze a wide range of adjustment (e.g. several magnitudes) into a fairly small numerical range.
	//
	mu_calc = (float)ts.dsp_nr_strength;		// get user setting
	/*
	mu_calc = DSP_NR_STRENGTH_MAX-mu_calc;		// invert (0 = minimum))
	mu_calc /= 2.6;								// scale calculation
	mu_calc *= mu_calc;							// square value
	mu_calc += 1;								// offset by one
	mu_calc /= 40;								// rescale
	mu_calc += 1;								// prevent negative log result
	mu_calc = log10f(mu_calc);					// de-linearize
	lms1Norm_instance.mu = mu_calc;				//
	*/
	//
	// New DSP NR "mu" calculation method as of 0.0.214
	//
	mu_calc /= 2;	// scale input value
	mu_calc += 2;	// offset zero value
	mu_calc /= 10;	// convert from "bels" to "deci-bels"
	mu_calc = powf(10,mu_calc);		// convert to ratio
	mu_calc = 1/mu_calc;			// invert to fraction
	lms1Norm_instance.mu = mu_calc;

	// Debug display of mu calculation
//	char txt[16];
//	sprintf(txt, " %d ", (ulong)(mu_calc * 10000));
//	UiLcdHy28_PrintText(POS_BOTTOM_BAR_F3_X,POS_BOTTOM_BAR_F3_Y,txt,0xFFFF,0,0);
//
//
	for(i = 0; i < LMS_NR_DELAYBUF_SIZE_MAX + BUFF_LEN; i++)	{		// clear LMS delay buffers
		lms1_nr_delay[i] = 0;
	}
	//
	for(i = 0; i < DSP_NR_NUMTAPS_MAX + BUFF_LEN; i++)	{		// clear LMS state buffer
		lms1StateF32[i] = 0;			// zero state buffer
		if(ts.reset_dsp_nr)	{			// are we to reset the coefficient buffer as well?
			lms1NormCoeff_f32[i] = 0;		// yes - zero coefficient buffers
		}
	}
	//
	// use "canned" init to initialize the filter coefficients
	//
	arm_lms_norm_init_f32(&lms1Norm_instance, calc_taps, &lms1NormCoeff_f32[0], &lms1StateF32[0], (float32_t)mu_calc, 64);
	//
	//
	if((ts.dsp_nr_delaybuf_len > DSP_NR_BUFLEN_MAX) || (ts.dsp_nr_delaybuf_len < DSP_NR_BUFLEN_MIN))
			ts.dsp_nr_delaybuf_len = DSP_NR_BUFLEN_DEFAULT;
	//
	// LMS instance 2 - Automatic Notch Filter
	//
	calc_taps = (uint16_t)ts.dsp_notch_numtaps;
	lms2Norm_instance.numTaps = calc_taps;
	lms2Norm_instance.pCoeffs = lms2NormCoeff_f32;
	lms2Norm_instance.pState = lms2StateF32;
	//
	// Calculate "mu" (convergence rate) from user "Notch ConvRate" setting
	//
	mu_calc = (float)ts.dsp_notch_mu;		// get user setting
//!	mu_calc = mu_calc;		// (0 = slowest)
	mu_calc += 1;
	mu_calc /= 1500;
	mu_calc += 1;
	mu_calc = log10f(mu_calc);
	//
	// use "canned" init to initialize the filter coefficients
	//
	arm_lms_norm_init_f32(&lms2Norm_instance, calc_taps, &lms2NormCoeff_f32[0], &lms2StateF32[0], (float32_t)mu_calc, 64);

	//
	for(i = 0; i < LMS_NR_DELAYBUF_SIZE_MAX + BUFF_LEN; i++)		// clear LMS delay buffer
		lms2_nr_delay[i] = 0;
	//
	for(i = 0; i < DSP_NOTCH_NUMTAPS_MAX + BUFF_LEN; i++)	{		// clear LMS state and coefficient buffers
		lms2StateF32[i] = 0;			// zero state buffer
		if(ts.reset_dsp_nr)				// are we to reset the coefficient buffer?
			lms2NormCoeff_f32[i] = 0;		// yes - zero coefficient buffer
	}
	//
	if((ts.dsp_notch_delaybuf_len > DSP_NOTCH_BUFLEN_MAX) || (ts.dsp_notch_delaybuf_len < DSP_NOTCH_BUFLEN_MIN))
				ts.dsp_nr_delaybuf_len = DSP_NOTCH_DELAYBUF_DEFAULT;
	//
	// Adjust decimation rate based on selected filter
	//
	if(ts.filter_id != AUDIO_WIDE)	{		// Not 10 kHz receiver bandwidth
		ads.decimation_rate = RX_DECIMATION_RATE_12KHZ;
		DECIMATE_RX.pCoeffs = (float32_t *)&FirRxDecimate[0];		// Filter coefficients for lower-rate (slightly strong LPF)
		INTERPOLATE_RX.pCoeffs = (float32_t *)&FirRxInterpolate[0];	// Filter coefficients
	}
	else	{								// This *IS* the 10 kHz receiver bandwidth
		ads.decimation_rate = RX_DECIMATION_RATE_24KHZ;
		DECIMATE_RX.pCoeffs = (float32_t *)&FirRxDecimateMinLPF[0];	// Filter coefficients for higher rate (weak LPF:  Hilbert is used for main LPF!)
		INTERPOLATE_RX.pCoeffs = (float32_t *)&FirRxInterpolate10KHZ[0];	// Filter coefficients for higher rate (relaxed LPF)
	}
	//
	ads.agc_decimation_scaling = (float)ads.decimation_rate;
	ads.agc_delay_buflen = AGC_DELAY_BUFSIZE/(ulong)ads.decimation_rate;	// calculate post-AGC delay based on post-decimation sampling rate
	//
    // Set up RX decimation/filter
	DECIMATE_RX.M = ads.decimation_rate;			// Decimation factor  (48 kHz / 4 = 12 kHz)
	DECIMATE_RX.numTaps = RX_DECIMATE_NUM_TAPS;		// Number of taps in FIR filter

	DECIMATE_RX.pState = &decimState[0];			// Filter state variables
	//
	// Set up RX interpolation/filter
	// NOTE:  Phase Length MUST be an INTEGER and is the number of taps divided by the decimation rate, and it must be greater than 1.
	//
	INTERPOLATE_RX.L = ads.decimation_rate;			// Interpolation factor, L  (12 kHz * 4 = 48 kHz)
	INTERPOLATE_RX.phaseLength = RX_INTERPOLATE_NUM_TAPS/ads.decimation_rate;	// Phase Length ( numTaps / L )
	INTERPOLATE_RX.pState = &interpState[0];		// Filter state variables
	//
	for(i = 0; i < FIR_RXAUDIO_BLOCK_SIZE + FIR_RXAUDIO_NUM_TAPS; i++)	{	// Initialize all filter state variables
		decimState[i] = 0;
		interpState[i] = 0;
	}
	//
	ads.dsp_zero_count = 0;		// initialize "zero" count to detect if DSP has crashed
	//
	// Unlock - re-enable filtering
	//
	ads.af_dissabled = 0;
	ts.dsp_inhibit = dsp_inhibit_temp;
	//
}

uchar audio_check_nr_dsp_state(void)
{
	uchar i;

	for(i = 0; i < 192; i++)	{
		if((lms1NormCoeff_f32[i] <= -1) || (lms1NormCoeff_f32[i] >= 1))
			return(1);
	}

	return(0);

}
//
//*----------------------------------------------------------------------------
//* Function Name       : Audio_Init
//* Object              :
//* Object              : init filters
//* Input Parameters    :
//* Output Parameters   :
//* Functions called    :
//*----------------------------------------------------------------------------
static void Audio_Init(void)
{
	uint32_t	i;

	// -------------------
	// Init TX audio filter - Do so "manually" since built-in init functions don't work with CONST coefficients
	//
	IIR_TXFilter.numStages = IIR_TX_2k7_numStages;		// number of stages
	IIR_TXFilter.pkCoeffs = (float *)IIR_TX_2k7_pkCoeffs;	// point to reflection coefficients
	IIR_TXFilter.pvCoeffs = (float *)IIR_TX_2k7_pvCoeffs;	// point to ladder coefficients

    for(i = 0; i < FIR_RXAUDIO_BLOCK_SIZE+FIR_RXAUDIO_NUM_TAPS-1; i++)	{	// initialize state buffer to zeroes
    	iir_tx_state[i] = 0;
    }
    IIR_TXFilter.pState = (float32_t *)&iir_tx_state;

    // Decimation/Interpolation is set up "manually" because the built-in functions do NOT work reliably with coefficients
    // stored in CONST memory!
    //
	if((ts.dsp_nr_delaybuf_len < DSP_NR_BUFLEN_MIN) || (ts.dsp_nr_delaybuf_len > DSP_NR_BUFLEN_MAX))
		ts.dsp_nr_delaybuf_len = DSP_NR_BUFLEN_DEFAULT;
	//
	// -------------------
	// Init RX audio filters
	audio_driver_set_rx_audio_filter();
}
//
//
//*----------------------------------------------------------------------------
//* Function Name       : audio_rx_noise_blanker
//* Object              : noise blanker
//* Object              :
//* Input Parameters    : I/Q 16 bit audio data, size of buffer
//* Output Parameters   :
//* Functions called    :
//*----------------------------------------------------------------------------
static void audio_rx_noise_blanker(int16_t *src, int16_t size)
{
#if 1
	static int16_t	delay_buf[34];
	static uchar	delbuf_inptr = 0, delbuf_outptr = 2;
	ulong	i;
	float	sig;
	float  nb_short_setting;
//	static float avg_sig;
	static	uchar	nb_delay = 0;
	static float	nb_agc = 0;
	//
	if((!ts.nb_setting) || (ts.nb_disable) || (ts.dmod_mode == DEMOD_AM) || (ts.filter_id == AUDIO_WIDE))	{// bail out if noise blanker disabled, in AM mode, or set to 10 kHz
		return;
	}

	nb_short_setting = (float)ts.nb_setting;		// convert and rescale NB1 setting for higher resolution in adjustment
	nb_short_setting /= 2;

	for(i = 0; i < size/2; i+=4)	{		// Noise blanker function - "Unrolled" 4x for maximum execution speed
		//
		sig = (float)fabs(*src);		// get signal amplitude.  We need only look at one of the two audio channels since they will be the same.
		sig /= ads.codec_gain_calc;	// Scale for codec A/D gain adjustment
		//
//		avg_sig = (avg_sig * NB_AVG_WEIGHT) + ((float)(*src) * NB_SIG_WEIGHT);	// IIR-filtered short-term average signal level (e.g. low-pass audio)
		//
		delay_buf[delbuf_inptr++] = *src;	// copy first byte into delay buffer
		delay_buf[delbuf_inptr++] = *(src+1);	// copy second byte into delay buffer
		//
		nb_agc = (ads.nb_agc_filt * nb_agc) + (ads.nb_sig_filt * sig);		// IIR-filtered "AGC" of current overall signal level
		//
		if(((sig) > (nb_agc * (((MAX_NB_SETTING/2) + 1.75) - nb_short_setting))) && (nb_delay == 0))	{	// did a pulse exceed the threshold?
			nb_delay = 16;		// yes - set the blanking duration counter
		}
		//
		if(!nb_delay)	{		// blank counter not active
			*src = delay_buf[delbuf_outptr++];		// pass through delayed audio, unchanged
			*(src+1) = delay_buf[delbuf_outptr++];
		}
		else	{	// It is within the blanking pulse period
			*src = 0; // (int16_t)avg_sig;		// set the audio buffer to "mute" during the blanking period
			*(src+1) = 0; //(int16_t)avg_sig;
			nb_delay--;						// count down the number of samples that we are to blank
		}
		//
//		delbuf_inptr += 2;					// update count of input of circular delay buffer
//		delbuf_outptr = delbuf_inptr + 2;	// output is always just "after" output of delay buffer
		delbuf_outptr &= 0x1e;				// set limit to count, forcing lsb of pointer to zero.
		delbuf_inptr &= 0x1e;
		//
		src++;								// update pointer to source material
		src++;								// (the I/Q pair of audio data)
		//
		// Next "unrolled" instance
		//
		sig = (float)fabs(*src);		// get signal amplitude.  We need only look at one of the two audio channels since they will be the same.
		sig /= ads.codec_gain_calc;	// Scale for codec A/D gain adjustment
		//
//		avg_sig = (avg_sig * NB_AVG_WEIGHT) + ((float)(*src) * NB_SIG_WEIGHT);	// IIR-filtered short-term average signal level (e.g. low-pass audio)
		//
		delay_buf[delbuf_inptr++] = *src;	// copy first byte into delay buffer
		delay_buf[delbuf_inptr++] = *(src+1);	// copy second byte into delay buffer
		//
		nb_agc = (NB_AGC_FILT * nb_agc) + (NB_SIG_FILT * sig);		// IIR-filtered "AGC" of current overall signal level
		//
		if(((sig) > (nb_agc * (((MAX_NB_SETTING/2) + 1.75) - nb_short_setting))) && (nb_delay == 0))	{	// did a pulse exceed the threshold?
			nb_delay = 16;		// yes - set the blanking duration counter
		}
		//
		if(!nb_delay)	{		// blank counter not active
			*src = delay_buf[delbuf_outptr++];		// pass through delayed audio, unchanged
			*(src+1) = delay_buf[delbuf_outptr++];
		}
		else	{	// It is within the blanking pulse period
			*src = 0; // (int16_t)avg_sig;		// set the audio buffer to "mute" during the blanking period
			*(src+1) = 0; //(int16_t)avg_sig;
			nb_delay--;						// count down the number of samples that we are to blank
		}
		//
//		delbuf_inptr += 2;					// update count of input of circular delay buffer
//		delbuf_outptr = delbuf_inptr + 2;	// output is always just "after" output of delay buffer
		delbuf_outptr &= 0x1e;				// set limit to count, forcing lsb of pointer to zero.
		delbuf_inptr &= 0x1e;
		//
		src++;								// update pointer to source material
		src++;								// (the I/Q pair of audio data)
		//
		// Next "unrolled" instance
		//
		sig = (float)fabs(*src);		// get signal amplitude.  We need only look at one of the two audio channels since they will be the same.
		sig /= ads.codec_gain_calc;	// Scale for codec A/D gain adjustment
		//
//		avg_sig = (avg_sig * NB_AVG_WEIGHT) + ((float)(*src) * NB_SIG_WEIGHT);	// IIR-filtered short-term average signal level (e.g. low-pass audio)
		//
		delay_buf[delbuf_inptr++] = *src;	// copy first byte into delay buffer
		delay_buf[delbuf_inptr++] = *(src+1);	// copy second byte into delay buffer
		//
		nb_agc = (NB_AGC_FILT * nb_agc) + (NB_SIG_FILT * sig);		// IIR-filtered "AGC" of current overall signal level
		//
		if(((sig) > (nb_agc * (((MAX_NB_SETTING/2) + 1.75) - nb_short_setting))) && (nb_delay == 0))	{	// did a pulse exceed the threshold?
			nb_delay = 16;		// yes - set the blanking duration counter
		}
		//
		if(!nb_delay)	{		// blank counter not active
			*src = delay_buf[delbuf_outptr++];		// pass through delayed audio, unchanged
			*(src+1) = delay_buf[delbuf_outptr++];
		}
		else	{	// It is within the blanking pulse period
			*src = 0; // (int16_t)avg_sig;		// set the audio buffer to "mute" during the blanking period
			*(src+1) = 0; //(int16_t)avg_sig;
			nb_delay--;						// count down the number of samples that we are to blank
		}
		//
//		delbuf_inptr += 2;					// update count of input of circular delay buffer
//		delbuf_outptr = delbuf_inptr + 2;	// output is always just "after" output of delay buffer
		delbuf_outptr &= 0x1e;				// set limit to count, forcing lsb of pointer to zero.
		delbuf_inptr &= 0x1e;
		//
		src++;								// update pointer to source material
		src++;								// (the I/Q pair of audio data)
		//
		// Last "unrolled" instance
		//
		sig = (float)fabs(*src);		// get signal amplitude.  We need only look at one of the two audio channels since they will be the same.
		sig /= ads.codec_gain_calc;	// Scale for codec A/D gain adjustment
		//
//		avg_sig = (avg_sig * NB_AVG_WEIGHT) + ((float)(*src) * NB_SIG_WEIGHT);	// IIR-filtered short-term average signal level (e.g. low-pass audio)
		//
		delay_buf[delbuf_inptr++] = *src;	// copy first byte into delay buffer
		delay_buf[delbuf_inptr++] = *(src+1);	// copy second byte into delay buffer
		//
		nb_agc = (NB_AGC_FILT * nb_agc) + (NB_SIG_FILT * sig);		// IIR-filtered "AGC" of current overall signal level
		//
		if(((sig) > (nb_agc * (((MAX_NB_SETTING/2) + 1.75) - nb_short_setting))) && (nb_delay == 0))	{	// did a pulse exceed the threshold?
			nb_delay = 16;		// yes - set the blanking duration counter
		}
		//
		if(!nb_delay)	{		// blank counter not active
			*src = delay_buf[delbuf_outptr++];		// pass through delayed audio, unchanged
			*(src+1) = delay_buf[delbuf_outptr++];
		}
		else	{	// It is within the blanking pulse period
			*src = 0; // (int16_t)avg_sig;		// set the audio buffer to "mute" during the blanking period
			*(src+1) = 0; //(int16_t)avg_sig;
			nb_delay--;						// count down the number of samples that we are to blank
		}
		//
//		delbuf_inptr += 2;					// update count of input of circular delay buffer
//		delbuf_outptr = delbuf_inptr + 2;	// output is always just "after" output of delay buffer
		delbuf_outptr &= 0x1e;				// set limit to count, forcing lsb of pointer to zero.
		delbuf_inptr &= 0x1e;
		//
		src++;								// update pointer to source material
		src++;								// (the I/Q pair of audio data)
	}
#endif
}

//
//*----------------------------------------------------------------------------
//* Function Name       : audio_rx_freq_conv
//* Object              : Does I/Q frequency conversion
//* Object              :
//* Input Parameters    : size of array on which to work; dir: determines direction of shift - see below;  Also uses variables in ads structure
//* Output Parameters   : uses variables in ads structure
//* Functions called    :
//*----------------------------------------------------------------------------
static void audio_rx_freq_conv(int16_t size, int16_t dir)
{
	ulong 		i;
	float32_t	rad_calc;
	static bool flag = 0;
	//
	// Below is the "on-the-fly" version of the frequency translator, generating a "live" version of the oscillator (NCO), which can be any
	// frequency, based on the values of "ads.Osc_Cos" and "ads.Osc_Sin".  While this does function, the generation of the SINE takes a LOT
	// of processor time!
	//
	// The values for the NCO are configured in the function "audio_driver_config_nco()".
	//
	// This commented-out version also lacks the "dir" (direction) control which selects either high or low side translation.
	// This code is left here so that everyone can see how it is actually done in plain, "un-ARM" code.
	//
	#ifdef DSP_MODE
	static float32_t	q_temp, i_temp;

	// Do we need NCO off ?
	if(df.nco_freq == 0)
		return;

	// Recalculate NCO, if needed
	audio_driver_config_nco();

	for(i = 0; i < size/2; i++)
	{
		// generate local oscillator on-the-fly:  This takes a lot of processor time!
		ads.Osc_Q = (ads.Osc_Vect_Q * ads.Osc_Cos) - (ads.Osc_Vect_I * ads.Osc_Sin);	// Q channel of oscillator
		ads.Osc_I = (ads.Osc_Vect_I * ads.Osc_Cos) + (ads.Osc_Vect_Q * ads.Osc_Sin);	// I channel of oscillator
		ads.Osc_Gain = 1.95 - ((ads.Osc_Vect_Q * ads.Osc_Vect_Q) + (ads.Osc_Vect_I * ads.Osc_Vect_I));	// Amplitude control of oscillator

		// rotate vectors while maintaining constant oscillator amplitude
		ads.Osc_Vect_Q = ads.Osc_Gain * ads.Osc_Q;
		ads.Osc_Vect_I = ads.Osc_Gain * ads.Osc_I;

		// do actual frequency conversion
		i_temp = ads.i_buffer[i];	// save temporary copies of data
		q_temp = ads.q_buffer[i];

		ads.i_buffer[i] = (i_temp * ads.Osc_Q) + (q_temp * ads.Osc_I);	// multiply I/Q data by sine/cosine data to do translation
		ads.q_buffer[i] = (q_temp * ads.Osc_Q) - (i_temp * ads.Osc_I);
	}
	#endif

	#ifndef DSP_MODE
	//
	// Below is the frequency translation code that uses a "pre-calculated" sine wave - which means that the translation must be done at a sub-
	// multiple of the sample frequency.  This pre-calculation eliminates the processor overhead required to generate a sine wave on the fly.
	// This also makes extensive use of the optimized ARM vector instructions for the calculation of the final I/Q vectors
	//
	// Pre-calculate 6 kHz quadrature sine wave(s) ONCE for the conversion
	//
	if(!flag)	{		// have we already calculated the sine wave?
		for(i = 0; i < size/2; i++)	{		// No, let's do it!
			rad_calc = (float32_t)i;		// convert to float the current position within the buffer
			rad_calc /= (size/2);			// make this a fraction
			rad_calc *= (PI * 2);			// convert to radians
			rad_calc *= 4;					// multiply by number of cycles that we want within this block (4 = 6 kHz)
			//
			ads.Osc_Q_buffer[i] = arm_cos_f32(rad_calc);	// get sine and cosine values and store in pre-calculated array
			ads.Osc_I_buffer[i] = arm_sin_f32(rad_calc);
		}
		flag = 1;	// signal that once we have generated the quadrature sine waves, we shall not do it again
	}
	//
	// Do frequency conversion using optimized ARM math functions
	//
	if(!dir)	{	// Conversion is "above" on RX (LO needs to be set lower)
		arm_mult_f32((float32_t *)ads.i_buffer, (float32_t *)ads.Osc_Q_buffer, (float32_t *)ads.a_buffer, size/2);	// multiply products for converted I channel
		arm_mult_f32((float32_t *)ads.q_buffer, (float32_t *)ads.Osc_I_buffer, (float32_t *)ads.b_buffer, size/2);
		//
		arm_mult_f32((float32_t *)ads.q_buffer, (float32_t *)ads.Osc_Q_buffer, (float32_t *)ads.c_buffer, size/2);	// multiply products for converted Q channel
		arm_mult_f32((float32_t *)ads.i_buffer, (float32_t *)ads.Osc_I_buffer, (float32_t *)ads.d_buffer, size/2);
		//
		arm_add_f32((float32_t *)ads.a_buffer, (float32_t *)ads.b_buffer, (float32_t *)ads.i_buffer, size/2);	// summation for I channel
		arm_sub_f32((float32_t *)ads.c_buffer, (float32_t *)ads.d_buffer, (float32_t *)ads.q_buffer, size/2);	// difference for Q channel
	}
	else	{	// Conversion is "below" on RX (LO needs to be set higher)
		arm_mult_f32((float32_t *)ads.q_buffer, (float32_t *)ads.Osc_Q_buffer, (float32_t *)ads.a_buffer, size/2);	// multiply products for converted I channel
		arm_mult_f32((float32_t *)ads.i_buffer, (float32_t *)ads.Osc_I_buffer, (float32_t *)ads.b_buffer, size/2);
		//
		arm_mult_f32((float32_t *)ads.i_buffer, (float32_t *)ads.Osc_Q_buffer, (float32_t *)ads.c_buffer, size/2);	// multiply products for converted Q channel
		arm_mult_f32((float32_t *)ads.q_buffer, (float32_t *)ads.Osc_I_buffer, (float32_t *)ads.d_buffer, size/2);
		//
		arm_add_f32((float32_t *)ads.a_buffer, (float32_t *)ads.b_buffer, (float32_t *)ads.q_buffer, size/2);	// summation for I channel
		arm_sub_f32((float32_t *)ads.c_buffer, (float32_t *)ads.d_buffer, (float32_t *)ads.i_buffer, size/2);	// difference for Q channel
	}
	#endif
}


//
//*----------------------------------------------------------------------------
//* Function Name       : audio_rx_agc_processor
//* Object              :
//* Object              : Processor for receiver AGC
//* Input Parameters    :
//* Output Parameters   :
//* Functions called    :
//*----------------------------------------------------------------------------
static void audio_rx_agc_processor(int16_t psize)
{
#if 1
	static float		agc_calc, agc_var;
	static ulong 		i;
	static ulong		agc_delay_inbuf = 0, agc_delay_outbuf = 0;

	//
	// AGC function - Look-ahead type by KA7OEI, revised September 2015 to eliminate possible low-order, low-frequency instabilities associated with steady-state signals.
	// Note that even though it gets only one AGC value per cycle, it *does* do "psize/2" calculations to iterate out the AGC value more precisely than it would
	// were it called once per DMA cycle.  If it is called once per DMA cycle it will tend to weakly oscillate under certain conditions and possibly overshoot/undershoot.
	//
	for(i = 0; i < psize/2; i++)	{
		if(ts.agc_mode != AGC_OFF)	{
			if(ts.dmod_mode == DEMOD_AM)		// if in AM, get the recovered DC voltage from the detected carrier
				agc_calc = ads.am_agc * ads.agc_val;
			else	{							// not AM - get the amplitude of the recovered audio
				agc_calc = fabs(ads.a_buffer[i]) * ads.agc_val;
				//agc_calc = max_signal * ads.agc_val;	// calculate current level by scaling it with AGC value
			}
			//
			if(agc_calc < ads.agc_knee)	{	// is audio below AGC "knee" value?
				agc_var = ads.agc_knee - agc_calc;	// calculate difference between agc value and "knee" value
				agc_var /= ads.agc_knee;	// calculate ratio of difference between knee value and this value
				ads.agc_val += ads.agc_val*ads.agc_decay * ads.agc_decimation_scaling * agc_var;	// Yes - Increase gain slowly for AGC DECAY - scale time constant with decimation
			}
			else	{
				agc_var = agc_calc - ads.agc_knee;	// calculate difference between agc value and "knee" value
				agc_var /= ads.agc_knee;	// calculate ratio of difference between knee value and this value
				ads.agc_val -= ads.agc_val * AGC_ATTACK * agc_var;	// Fast attack to increase attenuation (do NOT scale w/decimation or else oscillation results)
				if(ads.agc_val <= AGC_VAL_MIN)	// Prevent zero or "negative" gain values
					ads.agc_val = AGC_VAL_MIN;
			}
			if(ads.agc_val >= ads.agc_rf_gain)	{	// limit AGC to reasonable values when low/no signals present
				ads.agc_val = ads.agc_rf_gain;
				if(ads.agc_val >= ads.agc_val_max)	// limit maximum gain under no-signal conditions
					ads.agc_val = ads.agc_val_max;
			}
		}
		else	// AGC Off - manual AGC gain
			ads.agc_val = ads.agc_rf_gain;			// use logarithmic gain value in RF gain control
		//
		ads.agc_valbuf[i] = ads.agc_val;			// store in "running" AGC history buffer for later application to audio data
	}
	//
	// Delay the post-AGC audio slightly so that the AGC's "attack" will very slightly lead the audio being acted upon by the AGC.
	// This eliminates a "click" that can occur when a very strong signal appears due to the AGC lag.  The delay is adjusted based on
	// decimation rate so that it is constant for all settings.
	//
	arm_copy_f32((float32_t *)ads.a_buffer, (float32_t *)&agc_delay[agc_delay_inbuf], psize/2);	// put new data into the delay buffer
	arm_copy_f32((float32_t *)&agc_delay[agc_delay_outbuf], (float32_t *)ads.a_buffer, psize/2);	// take old data out of the delay buffer
	// Update the in/out pointers to the AGC delay buffer
	agc_delay_inbuf += psize/2;						// update circular de-correlation delay buffer
	agc_delay_outbuf = agc_delay_inbuf + psize/2;
	agc_delay_inbuf %= ads.agc_delay_buflen;
	agc_delay_outbuf %= ads.agc_delay_buflen;
	//
	//
	// Now apply pre-calculated AGC values to delayed audio
	//
	arm_mult_f32((float32_t *)ads.a_buffer, (float32_t *)ads.agc_valbuf, (float32_t *)ads.a_buffer, psize/2);		// do vector multiplication to apply delayed "running" AGC data
	//
#endif
}
//
//*----------------------------------------------------------------------------
//* Function Name       : audio_rx_processor
//* Object              :
//* Object              : audio sample processor
//* Input Parameters    :
//* Output Parameters   :
//* Functions called    :
//*----------------------------------------------------------------------------
static void audio_rx_processor(int16_t *src, int16_t *dst, int16_t size)
{
	static ulong 		i;
	int16_t				psize;		// processing size, with decimation
	static ulong		lms1_inbuf = 0, lms1_outbuf = 0;
	static ulong		lms2_inbuf = 0, lms2_outbuf = 0;
	float				post_agc_gain_scaling;

	int16_t *src_a = src; // test for crashing the IRQ only
	int16_t *dst_a = dst;

	psize = size/(int16_t)ads.decimation_rate;	// rescale sample size inside decimated portion based on decimation factor

	audio_rx_noise_blanker(src, size);		// do noise blanker function

	// ------------------------
	// Split stereo channels
	for(i = 0; i < size/2; i++)
	{
		// Collect I/Q samples
		if(sd.state == 0)
		{
			sd.FFT_Samples[sd.samp_ptr] = (float32_t)(*(src + 1));	// get floating point data for FFT for spectrum scope/waterfall display
			sd.samp_ptr++;
			sd.FFT_Samples[sd.samp_ptr] = (float32_t)(*(src));
			sd.samp_ptr++;

			// On overload, update state machine,
			// reset pointer and wait
			if(sd.samp_ptr >= FFT_IQ_BUFF_LEN1*2)
			{
				sd.samp_ptr = 0;
				sd.state    = 1;
			}
		}
		else
		{
			#ifdef USE_FFT_BACK_BUFFER
			if(sd.state_a == 0)
			{
				sd.FFT_SamplesA[sd.samp_ptr_a] = (float32_t)(*(src + 1));
				sd.samp_ptr_a++;
				sd.FFT_SamplesA[sd.samp_ptr_a] = (float32_t)(*(src));
				sd.samp_ptr_a++;

				if(sd.samp_ptr_a >= FFT_IQ_BUFF_LEN1*2)
				{
					sd.samp_ptr_a = 0;
					sd.state_a    = 1;
				}
			}
			#else
			sd.samp_ptr_a += 2;	// Just data collection
			#endif
		}

		if(*src > ADC_CLIP_WARN_THRESHOLD/4)	{		// This is the release threshold for the auto RF gain
			ads.adc_quarter_clip = 1;
			if(*src > ADC_CLIP_WARN_THRESHOLD/2)	{		// This is the trigger threshold for the auto RF gain
					ads.adc_half_clip = 1;
					if(*src > ADC_CLIP_WARN_THRESHOLD)			// This is the threshold for the red clip indicator on S-meter
						ads.adc_clip = 1;
			}
		}

		// 16 bit format - convert to float and increment
		ads.i_buffer[i] = (float32_t)*src++;
		ads.q_buffer[i] = (float32_t)*src++;
	}

	// Apply gain corrections for I/Q gain balancing
	arm_scale_f32((float32_t *)ads.i_buffer, (float32_t)ts.rx_adj_gain_var_i, (float32_t *)ads.i_buffer, size/2);
	arm_scale_f32((float32_t *)ads.q_buffer, (float32_t)ts.rx_adj_gain_var_q, (float32_t *)ads.q_buffer, size/2);

	#ifndef DSP_MODE
	if(ts.iq_freq_mode)	{		// is receive frequency conversion to be done?
		if(ts.iq_freq_mode == 1)			// Yes - "RX LO LOW" mode
			audio_rx_freq_conv(size, 1);
		else								// "RX LO HIGH" mode
			audio_rx_freq_conv(size, 0);
	}
	#endif

	#ifdef DSP_MODE
	audio_rx_freq_conv(size, 0);
	#endif
	//
	// ------------------------
	// IQ SSB processing - Do 0-90 degree Phase-added Hilbert Transform
	// *** *EXCEPT* in AM mode - see the function "UiCalcRxPhaseAdj()"
	//    In AM, the FIR below does ONLY low-pass filtering appropriate for the filter bandwidth selected when in AM mode, in
	//	  which case there is ***NO*** audio phase shift applied to the I/Q channels.
	//
	arm_fir_f32((arm_fir_instance_f32 *)&FIR_I,(float32_t *)(ads.i_buffer),(float32_t *)(ads.i_buffer),size/2);	// shift 0 degree FIR
	arm_fir_f32((arm_fir_instance_f32 *)&FIR_Q,(float32_t *)(ads.q_buffer),(float32_t *)(ads.q_buffer),size/2);	// shift +90 degrees FIR (plus RX IQ phase adjustment)

	//
	//	Demodulation, optimized using fast ARM math functions as much as possible
	//
	switch(ts.dmod_mode)	{
		case DEMOD_LSB:
			arm_sub_f32((float32_t *)ads.i_buffer, (float32_t *)ads.q_buffer, (float32_t *)ads.a_buffer, size/2);	// difference of I and Q - LSB
			break;
		case DEMOD_CW:
			if(!ts.cw_lsb)	// is this USB RX mode?  (LSB of mode byte was zero)
				arm_add_f32((float32_t *)ads.i_buffer, (float32_t *)ads.q_buffer, (float32_t *)ads.a_buffer, size/2);	// sum of I and Q - USB
			else	// No, it is LSB RX mode
				arm_sub_f32((float32_t *)ads.i_buffer, (float32_t *)ads.q_buffer, (float32_t *)ads.a_buffer, size/2);	// difference of I and Q - LSB
			break;
		case DEMOD_AM:
			arm_mult_f32((float32_t *)ads.i_buffer, (float32_t *)ads.i_buffer, (float32_t *)ads.a_buffer, size/2);		// square I - store in buffer "a"
			arm_mult_f32((float32_t *)ads.q_buffer, (float32_t *)ads.q_buffer, (float32_t *)ads.b_buffer, size/2);		// square Q - store in buffer "b"
			arm_add_f32((float32_t *)ads.a_buffer, (float32_t *)ads.b_buffer, (float32_t *)ads.a_buffer, size/2);		// sum squares - store in buffer "a"
			for(i = 0; i < size/2; i++)	{										// square root of contents
				arm_sqrt_f32((float32_t)ads.a_buffer[i], (float32_t *)&ads.a_buffer[i]);
			}
			arm_mean_f32((float32_t *)ads.a_buffer, size/2, &ads.am_agc);	// get "average" value of "a" buffer - the recovered DC value - for the AGC
			ads.am_agc *= AM_SCALING;	// rescale AM AGC to match SSB scaling so that AGC comes out the same
			break;
		case DEMOD_USB:
		case DEMOD_DIGI:
		default:
			arm_add_f32((float32_t *)ads.i_buffer, (float32_t *)ads.q_buffer, (float32_t *)ads.a_buffer, size/2);	// sum of I and Q - USB
		break;
	}

	// Do decimation down to lower rate for heavy-duty processing to reduce processor load
	arm_fir_decimate_f32(&DECIMATE_RX, (float32_t *)ads.a_buffer, (float32_t *)ads.a_buffer, size/2);		// LPF built into decimation (Yes, you can decimate-in-place!)

	// DSP Automatic Notch Filter using LMS (Least Mean Squared) algorithm
	if((!ads.af_dissabled) && (ts.dsp_active & 4) && (ts.dmod_mode != DEMOD_CW) && (!ts.dsp_inhibit))	{	// No notch in CW mode
		arm_copy_f32((float32_t *)ads.a_buffer, (float32_t *)&lms2_nr_delay[lms2_inbuf], psize/2);	// put new data into the delay buffer
		//
		arm_lms_norm_f32(&lms2Norm_instance, (float32_t *)ads.a_buffer, (float32_t *)&lms2_nr_delay[lms2_outbuf], (float32_t *)errsig2, (float32_t *)ads.a_buffer, psize/2);	// do automatic notch
		//
		lms2_inbuf += psize/2;				// update circular de-correlation delay buffer
		lms2_outbuf = lms2_inbuf + psize/2;
		lms2_inbuf %= ts.dsp_notch_delaybuf_len;
		lms2_outbuf %= ts.dsp_notch_delaybuf_len;
	}

	// DSP noise reduction using LMS (Least Mean Squared) algorithm
	// This is the pre-filter/AGC instance
	if((ts.dsp_active & 1) && (!(ts.dsp_active & 2)) && (!ads.af_dissabled) && (!ts.dsp_inhibit))	{	// Do this if enabled and "Pre-AGC" DSP NR enabled
		arm_copy_f32((float32_t *)ads.a_buffer, (float32_t *)&lms1_nr_delay[lms1_inbuf], psize/2);	// put new data into the delay buffer
		//
		arm_lms_norm_f32(&lms1Norm_instance, (float32_t *)ads.a_buffer, (float32_t *)&lms1_nr_delay[lms1_outbuf], (float32_t *)ads.a_buffer, (float32_t *)errsig1 ,psize/2);	// do noise reduction
		//
		// Detect if the DSP output has gone to (near) zero output - a sign of it crashing!
		//
		if((((ulong)fabs(ads.a_buffer[0])) * DSP_ZERO_DET_MULT_FACTOR) < DSP_OUTPUT_MINVAL)	{	// is DSP level too low?
			// For some stupid reason we can't just compare above to a small fractional value  (e.g. "x < 0.001") so we must multiply it first!
			if(ads.dsp_zero_count < MAX_DSP_ZERO_COUNT)	{
				ads.dsp_zero_count++;
			}
		}
		else
			ads.dsp_zero_count = 0;
		//
		ads.dsp_nr_sample = ads.a_buffer[0];		// provide a sample of the DSP output for crash detection
		//
		lms1_inbuf += psize/2;	// bump input to the next location in our de-correlation buffer
		lms1_outbuf = lms1_inbuf + psize/2;	// advance output to same distance ahead of input
		lms1_inbuf %= ts.dsp_nr_delaybuf_len;
		lms1_outbuf %= ts.dsp_nr_delaybuf_len;
	}

	// Apply audio filter
	if((!ads.af_dissabled)	&& (ts.filter_id != AUDIO_WIDE))	{	// we don't need to filter if running in "wide" mode (Hilbert/FIR does the job!)
		// IIR ARMA-type lattice filter
		arm_iir_lattice_f32(&IIR_PreFilter, (float32_t *)ads.a_buffer, (float32_t *)ads.a_buffer, psize/2);
	}

	// now process the samples and perform the receiver AGC function
	audio_rx_agc_processor(psize);

	// DSP noise reduction using LMS (Least Mean Squared) algorithm
	// This is the post-filter, post-AGC instance
	if((ts.dsp_active & 1) && (ts.dsp_active & 2) && (!ads.af_dissabled) && (!ts.dsp_inhibit))	{	// Do DSP NR if enabled and if post-DSP NR enabled
		arm_copy_f32((float32_t *)ads.a_buffer, (float32_t *)&lms1_nr_delay[lms1_inbuf], psize/2);	// put new data into the delay buffer
		//
		arm_lms_norm_f32(&lms1Norm_instance, (float32_t *)ads.a_buffer, (float32_t *)&lms1_nr_delay[lms1_outbuf], (float32_t *)ads.a_buffer, (float32_t *)errsig1, psize/2);	// Do noise reduction
		//
		// "DC" bias detection to determine if the DSP has crashed
		//
		// Detect if the DSP output has gone to (near) zero output - a sign of it crashing!
		//
		if((((ulong)fabs(ads.a_buffer[0])) * DSP_ZERO_DET_MULT_FACTOR) < DSP_OUTPUT_MINVAL)	{	// is DSP level too low?
			// For some stupid reason we can't just compare it to a small fractional value  (e.g. "x < 0.001") so we must multiply it first!
			if(ads.dsp_zero_count < MAX_DSP_ZERO_COUNT)	{
				ads.dsp_zero_count++;
			}
		}
		else
			ads.dsp_zero_count = 0;
		//
		ads.dsp_nr_sample = ads.a_buffer[0];		// provide a sample of the DSP output for additional crash detection
		//
		lms1_inbuf += psize/2;					// update circular de-correlation delay buffer
		lms1_outbuf = lms1_inbuf + psize/2;
		lms1_inbuf %= ts.dsp_nr_delaybuf_len;
		lms1_outbuf %= ts.dsp_nr_delaybuf_len;
	}

	// Calculate scaling based on decimation rate since this affects the audio gain
	if(ts.filter_id != AUDIO_WIDE)
		post_agc_gain_scaling = POST_AGC_GAIN_SCALING_DECIMATE_4;
	else
		post_agc_gain_scaling = POST_AGC_GAIN_SCALING_DECIMATE_2;

	// Scale audio to according to AGC setting, demodulation mode and required fixed levels
	if(ts.dmod_mode == DEMOD_AM)
		arm_scale_f32((float32_t *)ads.a_buffer,(float32_t)(ads.post_agc_gain * post_agc_gain_scaling * (AM_SCALING * AM_AUDIO_SCALING)), (float32_t *)ads.a_buffer, psize/2);	// apply fixed amount of audio gain scaling to make the audio levels correct along with AGC
	else
		arm_scale_f32((float32_t *)ads.a_buffer,(float32_t)(ads.post_agc_gain * post_agc_gain_scaling), (float32_t *)ads.a_buffer, psize/2);	// apply fixed amount of audio gain scaling to make the audio levels correct along with AGC

	// resample back to original sample rate while doing low-pass filtering to minimize aliasing effects
	arm_fir_interpolate_f32(&INTERPOLATE_RX, (float32_t *)ads.a_buffer,(float32_t *) ads.b_buffer, psize/2);

	if(ts.rx_muting)	{
		arm_fill_f32(0, (float32_t *)ads.a_buffer, size/2);
		arm_fill_f32(0, (float32_t *)ads.b_buffer, size/2);
	}
	else	{
		arm_scale_f32((float32_t *)ads.b_buffer, (float32_t)LINE_OUT_SCALING_FACTOR, (float32_t *)ads.a_buffer, size/2);		// Do fixed scaling of audio for LINE OUT and copy to "a" buffer in one operation
		//
		// AF gain in "ts.audio_gain-active"
		//  0 - 16: via codec command
		// 17 - 20: soft gain after decoder
		//
		if(ts.audio_gain > 16)	// is volume control above highest hardware setting?
			arm_scale_f32((float32_t *)ads.b_buffer, (float32_t)ts.audio_gain_active, (float32_t *)ads.b_buffer, size/2);	// yes, do software volume control adjust on "b" buffer
	}

	// Transfer processed audio to DMA buffer
	i = 0;			// init sample transfer counter
	while(i < size/2)	{						// transfer to DMA buffer and do conversion to INT - Unrolled to speed it up
		#if 1
		*dst++ = (int16_t)ads.b_buffer[i];		// Speaker channel (variable level)
		*dst++ = (int16_t)ads.b_buffer[i++];

		*dst++ = (int16_t)ads.b_buffer[i];		// Speaker channel (variable level)
		*dst++ = (int16_t)ads.b_buffer[i++];

		*dst++ = (int16_t)ads.b_buffer[i];		// Speaker channel (variable level)
		*dst++ = (int16_t)ads.b_buffer[i++];

		*dst++ = (int16_t)ads.b_buffer[i];		// Speaker channel (variable level)
		*dst++ = (int16_t)ads.b_buffer[i++];
		#else
		*dst++ = (int16_t)ads.a_buffer[i];
		*dst++ = (int16_t)ads.a_buffer[i++];	// LINE OUT (constant level)

		*dst++ = (int16_t)ads.a_buffer[i];
		*dst++ = (int16_t)ads.a_buffer[i++];	// LINE OUT (constant level)

		*dst++ = (int16_t)ads.a_buffer[i];
		*dst++ = (int16_t)ads.a_buffer[i++];	// LINE OUT (constant level)

		*dst++ = (int16_t)ads.a_buffer[i];
		*dst++ = (int16_t)ads.a_buffer[i++];	// LINE OUT (constant level)
		#endif
	}

	// Test only - monitor for DSP crash
	//for(int i = 0; i < size; i++)
	//	dst_a[i] = src_a[i];
}

//
// This is a stripped-down RX signal processor - a work in progress
//
//*----------------------------------------------------------------------------
//* Function Name       : audio_dv_rx_processor
//* Object              :
//* Object              : audio sample processor - minimized USB-only RX processor with internal decimation to 8 kHz
//* Input Parameters    :
//* Output Parameters   :
//* Functions called    :
//*----------------------------------------------------------------------------
//!extern ulong sample_count;
static void audio_dv_rx_processor(int16_t *src, int16_t *dst, int16_t size)
{

	static ulong 		i;
	static float		max_signal, min_signal;
	uint32_t			result_index;
	static float		agc_calc;
	//
	int16_t				psize;		// processing size, with decimation
	//
	static ulong		agc_delay_inbuf = 0, agc_delay_outbuf = 0;
	//
	float				post_agc_gain_scaling;

	psize = size/(int16_t)ads.decimation_rate;	// rescale sample size inside decimated portion based on decimation factor:  This must be set to 6 for DV1300 mode!
	//
	//
	max_signal = 0;		// Init peak detector - used as part of secondary AGC loop, below
	//
	// ------------------------
	// Split stereo channels
	for(i = 0; i < size/2; i++)
	{
		//
		// Collect I/Q samples
		if(sd.state == 0)
		{
			sd.FFT_Samples[sd.samp_ptr] = (float32_t)(*(src + 1));	// get floating point data for FFT for spectrum scope/waterfall display
			sd.samp_ptr++;
			sd.FFT_Samples[sd.samp_ptr] = (float32_t)(*(src));
			sd.samp_ptr++;

			// On overload, update state machine,
			// reset pointer and wait
			if(sd.samp_ptr >= FFT_IQ_BUFF_LEN1*2)
			{
				sd.samp_ptr = 0;
				sd.state    = 1;
			}
		}
		//
		if(*src > ADC_CLIP_WARN_THRESHOLD/4)	{		// This is the release threshold for the auto RF gain
			ads.adc_quarter_clip = 1;
			if(*src > ADC_CLIP_WARN_THRESHOLD/2)	{		// This is the trigger threshold for the auto RF gain
					ads.adc_half_clip = 1;
					if(*src > ADC_CLIP_WARN_THRESHOLD)			// This is the threshold for the red clip indicator on S-meter
						ads.adc_clip = 1;
			}
		}
		//
		// 16 bit format - convert to float and increment
		ads.i_buffer[i] = (float32_t)*src++;
		ads.q_buffer[i] = (float32_t)*src++;
		//
	}
	//
	// Apply gain corrections for I/Q gain balancing
	//
	arm_scale_f32((float32_t *)ads.i_buffer, (float32_t)ts.rx_adj_gain_var_i, (float32_t *)ads.i_buffer, size/2);
	//
	arm_scale_f32((float32_t *)ads.q_buffer, (float32_t)ts.rx_adj_gain_var_q, (float32_t *)ads.q_buffer, size/2);
	//
	//
	if(ts.iq_freq_mode)	{		// is receive frequency conversion to be done?
		if(ts.iq_freq_mode == 1)			// Yes - "RX LO LOW" mode
			audio_rx_freq_conv(size, 1);
		else								// "RX LO HIGH" mode
			audio_rx_freq_conv(size, 0);
	}
	//
	// ------------------------
	// IQ SSB processing - Do 0-90 degree Phase-added Hilbert Transform
	// *** *EXCEPT* in AM mode - see the function "UiCalcRxPhaseAdj()"
	//    In AM, the FIR below does ONLY low-pass filtering appropriate for the filter bandwidth selected when in AM mode, in
	//	  which case there is ***NO*** audio phase shift applied to the I/Q channels.
	//
	arm_fir_f32((arm_fir_instance_f32 *)&FIR_I,(float32_t *)(ads.i_buffer),(float32_t *)(ads.i_buffer),size/2);	// shift 0 degree FIR
	arm_fir_f32((arm_fir_instance_f32 *)&FIR_Q,(float32_t *)(ads.q_buffer),(float32_t *)(ads.q_buffer),size/2);	// shift +90 degrees FIR (plus RX IQ phase adjustment)
	//

	switch(ts.dmod_mode)	{
		case DEMOD_LSB:
			arm_sub_f32((float32_t *)ads.i_buffer, (float32_t *)ads.q_buffer, (float32_t *)ads.a_buffer, size/2);	// difference of I and Q - LSB
			break;
		case DEMOD_USB:
		default:
			arm_add_f32((float32_t *)ads.i_buffer, (float32_t *)ads.q_buffer, (float32_t *)ads.a_buffer, size/2);	// sum of I and Q - USB
			break;
	}
	//
	//
	// Do decimation down to lower rate for heavy-duty processing to reduce processor load
	//
	arm_fir_decimate_f32(&DECIMATE_RX, (float32_t *)ads.a_buffer, (float32_t *)ads.a_buffer, size/2);		// LPF built into decimation (Yes, you can decimate-in-place!)
	//
	//
	//
	// now process the samples and perform the receiver AGC function
	//
	audio_rx_agc_processor(psize);
	//
	//
	// Now apply pre-calculated AGC values to delayed audio
	//
	arm_mult_f32((float32_t *)ads.a_buffer, (float32_t *)ads.agc_valbuf, (float32_t *)ads.a_buffer, psize/2);		// do vector multiplication to apply delayed "running" AGC data
	//
	//
	// ***************************************************************************************************
	//
	// DV Demodulator goes here.  ads.a_buffer must now be at 8ksps
	//
	// ***************************************************************************************************
	//

	//sample_count = psize;						// 16
	//sample_count = size;						// 64
	//sample_count = ads.decimation_rate;		// 4
//!	sample_count += psize;

	//
	// Calculate scaling based on decimation rate since this affects the audio gain
	//
	post_agc_gain_scaling = POST_AGC_GAIN_SCALING_DECIMATE_4;
	//
	// Scale audio to according to AGC setting, demodulation mode and required fixed levels
	//
	arm_scale_f32((float32_t *)ads.a_buffer,(float32_t)(ads.post_agc_gain * post_agc_gain_scaling), (float32_t *)ads.a_buffer, psize/2);	// apply fixed amount of audio gain scaling to make the audio levels correct along with AGC
	//
	// resample back to original sample rate while doing low-pass filtering to minimize aliasing effects
	//
	arm_fir_interpolate_f32(&INTERPOLATE_RX, (float32_t *)ads.a_buffer,(float32_t *) ads.b_buffer, psize/2);
	//
	if(ts.rx_muting)	{
		arm_fill_f32(0, (float32_t *)ads.a_buffer, size/2);
		arm_fill_f32(0, (float32_t *)ads.b_buffer, size/2);
	}
	else	{
		arm_scale_f32((float32_t *)ads.b_buffer, (float32_t)LINE_OUT_SCALING_FACTOR, (float32_t *)ads.a_buffer, size/2);		// Do fixed scaling of audio for LINE OUT and copy to "a" buffer in one operation
		//
		// AF gain in "ts.audio_gain-active"
		//  0 - 16: via codec command
		// 17 - 20: soft gain after decoder
		//
		if(ts.audio_gain > 16)	// is volume control above highest hardware setting?
			arm_scale_f32((float32_t *)ads.b_buffer, (float32_t)ts.audio_gain_active, (float32_t *)ads.b_buffer, size/2);	// yes, do software volume control adjust on "b" buffer
	}
	//
	// Transfer processed audio to DMA buffer
	//
	i = 0;			// init sample transfer counter
	while(i < size/2)	{						// transfer to DMA buffer and do conversion to INT - Unrolled to speed it up
		*dst++ = (int16_t)ads.b_buffer[i];		// Speaker channel (variable level)
		*dst++ = (int16_t)ads.a_buffer[i++];		// LINE OUT (constant level)

		*dst++ = (int16_t)ads.b_buffer[i];		// Speaker channel (variable level)
		*dst++ = (int16_t)ads.a_buffer[i++];		// LINE OUT (constant level)

		*dst++ = (int16_t)ads.b_buffer[i];		// Speaker channel (variable level)
		*dst++ = (int16_t)ads.a_buffer[i++];		// LINE OUT (constant level)

		*dst++ = (int16_t)ads.b_buffer[i];		// Speaker channel (variable level)
		*dst++ = (int16_t)ads.a_buffer[i++];		// LINE OUT (constant level)
	}
	//
}

//
//*----------------------------------------------------------------------------
//* Function Name       : audio_tx_compressor (look-ahead type) by KA7OEI
//* Object              :
//* Object              : speech compressor/processor for TX audio
//* Input Parameters    : size of buffer to processes, gain scaling factor
//* Input Parameters    : also processes/compresses audio in "ads.i_buffer" and "ads.q_buffer"
//* Output Parameters   : data via "ads.i_buffer" and "ads.q_buffer"
//* Functions called    : none
//*----------------------------------------------------------------------------
static void audio_tx_compressor(int16_t size, float gain_scaling)
{
	ulong i;
	static ulong		alc_delay_inbuf = 0, alc_delay_outbuf = 0;
	static float		alc_calc, alc_var;

	// ------------------------
	// Do ALC processing on audio buffer - look-ahead type by KA7OEI
	for(i = 0; i < size/2; i++)
	{
		if(!ts.tune)	{	// if NOT in TUNE mode, do ALC processing
			// perform ALC on post-filtered audio (You will notice the striking similarity to the AGC code!)
			//
			alc_calc = fabs(ads.i_buffer[i] * ads.alc_val);	// calculate current level by scaling it with ALC value (both channels will be the same amplitude-wise)
			if(alc_calc < ALC_KNEE)	{	// is audio below ALC "knee" value?
				alc_var = ALC_KNEE - alc_calc;	// calculate difference between ALC value and "knee" value
				alc_var /= ALC_KNEE;			// calculate ratio of difference between knee value and this value
				ads.alc_val += ads.alc_val * ads.alc_decay * alc_var;	// (ALC DECAY) Yes - Increase gain slowly
			}
			else	{
				alc_var = alc_calc - ALC_KNEE;			// calculate difference between ALC value and "knee" value
				alc_var /= ALC_KNEE;			// calculate ratio of difference between knee value and this value
				ads.alc_val -= ads.alc_val * ALC_ATTACK * alc_var;	// Fast attack to increase gain
				if(ads.alc_val <= ALC_VAL_MIN)	// Prevent zero or "negative" gain values
					ads.alc_val = ALC_VAL_MIN;
			}
			if(ads.alc_val >= ALC_VAL_MAX)	// limit to fixed values within the code
				ads.alc_val = ALC_VAL_MAX;
		}
		else	{	// are we in TUNE mode?
			ads.alc_val = ALC_VAL_MAX;		// yes, disable ALC and set to MAXIMUM ALC gain (e.g. unity - no gain reduction)
		}
		ads.agc_valbuf[i] = (ads.alc_val * gain_scaling);	// store in "running" ALC history buffer for later application to audio data
	}
	//
	// Delay the post-ALC audio slightly so that the ALC's "attack" will very slightly lead the audio being acted upon by the ALC.
	// This eliminates a "click" that can occur when a very strong signal appears due to the ALC lag.  The delay is adjusted based on
	// decimation rate so that it is constant for all settings.
	//
	arm_copy_f32((float32_t *)ads.a_buffer, (float32_t *)&agc_delay[alc_delay_inbuf], size/2);	// put new data into the delay buffer
	arm_copy_f32((float32_t *)&agc_delay[alc_delay_outbuf], (float32_t *)ads.a_buffer, size/2);	// take old data out of the delay buffer
	//
	// Update the in/out pointers to the ALC delay buffer
	//
	alc_delay_inbuf += size/2;
	alc_delay_outbuf = alc_delay_inbuf + size/2;
	alc_delay_inbuf %= ALC_DELAY_BUFSIZE;
	alc_delay_outbuf %= ALC_DELAY_BUFSIZE;
	//
	arm_mult_f32((float32_t *)ads.i_buffer, (float32_t *)ads.agc_valbuf, (float32_t *)ads.i_buffer, size/2);		// Apply ALC gain corrections to TX audio channels
	arm_mult_f32((float32_t *)ads.q_buffer, (float32_t *)ads.agc_valbuf, (float32_t *)ads.q_buffer, size/2);
}

//*----------------------------------------------------------------------------
//* Function Name       : audio_tx_processor
//* Object              :
//* Object              : audio sample processor
//* Input Parameters    :
//* Output Parameters   :
//* Functions called    :
//*----------------------------------------------------------------------------
static void audio_tx_processor(int16_t *src, int16_t *dst, int16_t size)
{
	static ulong 		i;
	float32_t			gain_calc, min, max;
	int16_t				*ptr;
	uint32_t			pindex;

	// -----------------------------
	// TUNE mode handler for CW mode
	//
	//if((ts.tune) && ((ts.dmod_mode != DEMOD_LSB) && (ts.dmod_mode != DEMOD_AM) && (ts.dmod_mode != DEMOD_USB)))	// Tune mode - but NOT in USB/LSB/AM mode
	if(ts.tune)
	{
		softdds_runf((float32_t *)ads.i_buffer, (float32_t *)ads.q_buffer,size/2);		// generate tone/modulation for TUNE
		//
		// Equalize based on band and simultaneously apply I/Q gain adjustments
		//
		arm_scale_f32((float32_t *)ads.i_buffer, (float32_t)(ts.tx_power_factor * ts.tx_adj_gain_var_i), (float32_t *)ads.i_buffer, size/2);
		arm_scale_f32((float32_t *)ads.q_buffer, (float32_t)(ts.tx_power_factor * ts.tx_adj_gain_var_q), (float32_t *)ads.q_buffer, size/2);
		//
		// Output I and Q as stereo
		for(i = 0; i < size/2; i++)	{
			//if(!ts.cw_lsb)	{	// CW in USB mode
				*dst++ = (int16_t)ads.i_buffer[i];	// save left channel
				*dst++ = (int16_t)ads.q_buffer[i];	// save right channel
			//}
			//else	{			// CW in LSB mode
			//	*dst++ = (int16_t)ads.q_buffer[i];	// save left channel
			//	*dst++ = (int16_t)ads.i_buffer[i];	// save right channel
			//}
		}
		return;
	}

	// -----------------------------
	// CW handler
	//
	else if(ts.dmod_mode == DEMOD_CW)	{
		// Generate CW
		if(cw_gen_process((float32_t *)ads.i_buffer, (float32_t *)ads.q_buffer,size) == 0)
		{
			// Pause or inactivity
			for(i = 0; i < size/2; i++)
			{
				*dst++ = 0;
				*dst++ = 0;
			}
		}
		else	{
			//
			// Equalize based on band and simultaneously apply I/Q gain adjustments
			//
			arm_scale_f32((float32_t *)ads.i_buffer, (float32_t)(ts.tx_power_factor * ts.tx_adj_gain_var_i), (float32_t *)ads.i_buffer, size/2);
			arm_scale_f32((float32_t *)ads.q_buffer, (float32_t)(ts.tx_power_factor * ts.tx_adj_gain_var_q), (float32_t *)ads.q_buffer, size/2);
			//
			// Copy soft DDS data
			for(i = 0; i < size/2; i++)	{
				if(!ts.cw_lsb)	{	// CW in USB mode
					*dst++ = (int16_t)ads.i_buffer[i];	// save left channel
					*dst++ = (int16_t)ads.q_buffer[i];	// save right channel
				}
				else	{			// CW in LSB mode
					*dst++ = (int16_t)ads.q_buffer[i];	// save left channel
					*dst++ = (int16_t)ads.i_buffer[i];	// save right channel
				}
			}
		}
		return;
	}
	//
	// ------------------------
	// SSB processor
	//
	else if((ts.dmod_mode == DEMOD_LSB) || (ts.dmod_mode == DEMOD_USB))	{	// Do this ONLY if in USB or LSB mode, of course!

		if(ts.tune)	{	// TUNE mode?  If so, generate tone so we can adjust TX IQ phase and gain
			softdds_runf((float32_t *)ads.a_buffer, (float32_t *)ads.a_buffer,size/2);		// load audio buffer with the tone - DDS produces quadrature channels, but we need only one
		}
		else	{		// Not tune mode - use audio from CODEC
			// Fill I and Q buffers with left channel(same as right)
			for(i = 0; i < size/2; i++)	{				// Copy to single buffer
				ads.a_buffer[i] = (float)*src;
				src += 2;								// Next sample
			}
		}
		//
		if(ts.tx_audio_source == TX_AUDIO_LINEIN)		// Are we in LINE IN mode?
			gain_calc = LINE_IN_GAIN_RESCALE;			// Yes - fixed gain scaling for line input - the rest is done in hardware
		else	{
			gain_calc = (float)ts.tx_mic_gain_mult;		// We are in MIC In mode:  Calculate Microphone gain
			gain_calc /= MIC_GAIN_RESCALE;				// rescale microphone gain to a reasonable range
		}
		//
		// Apply gain if not in TUNE mode
		if(!ts.tune)	{
			arm_scale_f32((float32_t *)ads.a_buffer, (float32_t)gain_calc, (float32_t *)ads.a_buffer, size/2);	// apply gain
			//
			arm_max_f32((float32_t *)ads.a_buffer, size/2, &max, &pindex);		// find absolute value of audio in buffer after gain applied
			arm_min_f32((float32_t *)ads.a_buffer, size/2, &min, &pindex);
			min = fabs(min);
			if(min > max)
				max = min;
			ads.peak_audio = max;
		}
		//
		//	TX audio filtering
		//
		if(!ts.tune)	{	// NOT in TUNE mode, apply the TX equalization filtering.  This "flattens" the audio
						// prior to being applied to the Hilbert transformer as well as added low-pass filtering.
						// It does this by applying a "peak" to the bottom end to compensate for the roll-off caused by the Hilbert
						// and then a gradual roll-off toward the high end.  The net result is a very flat (to better than 1dB) response
						// over the 275-2500 Hz range.
						//
			if(!(ts.misc_flags1 & 64))	// Do the audio filtering *IF* it is to be enabled
				arm_iir_lattice_f32(&IIR_TXFilter, (float *)ads.a_buffer, (float *)ads.a_buffer, size/2);
		}
		//
		// This is a phase-added 0-90 degree Hilbert transformer that also does low-pass and high-pass filtering
		// to the transmitted audio.  As noted above, it "clobbers" the low end, which is why we made up for it with the above filter.
		//
		if(ads.tx_filter_adjusting == 0)	{	//	is the filter NOT being adjusted?  (e.g. disable filter while we alter coefficients)
			// yes - apply transformation AND audio filtering to buffer data
			// + 0 deg to I data
			arm_fir_f32((arm_fir_instance_f32 *)&FIR_I_TX,(float32_t *)(ads.a_buffer),(float32_t *)(ads.i_buffer),size/2);

			// - 90 deg to Q data
			arm_fir_f32((arm_fir_instance_f32 *)&FIR_Q_TX,(float32_t *)(ads.a_buffer),(float32_t *)(ads.q_buffer), size/2);
		}

		if(!ts.tune)	{	// do post-filter gain calculations if we are NOT in TUNE mode
			// perform post-filter gain operation
			//
			gain_calc = (float)ts.alc_tx_postfilt_gain_var;		// get post-filter gain setting
			gain_calc /= 2;									// halve it
			gain_calc += 0.5;								// offset it so that 2 = unity
			arm_scale_f32((float32_t *)ads.i_buffer, (float32_t)gain_calc, (float32_t *)ads.i_buffer, size/2);		// use optimized function to apply scaling to I/Q buffers
			arm_scale_f32((float32_t *)ads.q_buffer, (float32_t)gain_calc, (float32_t *)ads.q_buffer, size/2);
		}
		//
		audio_tx_compressor(size, SSB_ALC_GAIN_CORRECTION);	// Do the TX ALC and speech compression/processing
		//
		if(ts.iq_freq_mode)	{		// is transmit frequency conversion to be done?
			if(ts.dmod_mode == DEMOD_LSB)	{		// Is it LSB?
				if(ts.iq_freq_mode == 1)			// yes - is it "RX LO HIGH" mode?
					audio_rx_freq_conv(size, 0);	// set conversion to "LO IS HIGH" mode
				else								// it is in "RX LO LOW" mode
					audio_rx_freq_conv(size, 1);	// set conversion to "RX LO LOW" mode
			}
			else	{								// It is USB!
				if(ts.iq_freq_mode == 1)			// yes - is it "RX LO HIGH" mode?
					audio_rx_freq_conv(size, 1);	// set conversion to "RX LO LOW" mode
				else								// it is in "RX LO LOW" mode
					audio_rx_freq_conv(size, 0);	// set conversion to "LO IS HIGH" mode
			}
		}
		//
		// Equalize based on band and simultaneously apply I/Q gain adjustments
		//
		arm_scale_f32((float32_t *)ads.i_buffer, (float32_t)(ts.tx_power_factor * ts.tx_adj_gain_var_i * SSB_GAIN_COMP), (float32_t *)ads.i_buffer, size/2);
		arm_scale_f32((float32_t *)ads.q_buffer, (float32_t)(ts.tx_power_factor * ts.tx_adj_gain_var_q * SSB_GAIN_COMP), (float32_t *)ads.q_buffer, size/2);
		//
		// ------------------------
		// Output I and Q as stereo data
		for(i = 0; i < size/2; i++)	{
			// Prepare data for DAC
			if(ts.dmod_mode == DEMOD_USB)	{
				*dst++ = (int16_t)ads.i_buffer[i];	// save left channel
				*dst++ = (int16_t)ads.q_buffer[i];	// save right channel
			}
			else	{		// Save in the opposite order for LSB
				*dst++ = (int16_t)ads.q_buffer[i];	// save left channel
				*dst++ = (int16_t)ads.i_buffer[i];	// save right channel
			}
		}
		return;
	}
	// -----------------------------
	// AM handler
	//
	else if(ts.dmod_mode == DEMOD_AM)	{	//	Is it in AM mode *AND* is frequency translation active?
		if(ts.iq_freq_mode)	{				// is translation active?
			// Translation is active - Fill I and Q buffers with left channel(same as right)
			for(i = 0; i < size/2; i++)	{				// Copy to single buffer
				ads.a_buffer[i] = (float)*src;
				src += 2;								// Next sample
			}
			//
			if(ts.tx_audio_source == TX_AUDIO_LINEIN)		// Are we in LINE IN mode?
				gain_calc = LINE_IN_GAIN_RESCALE;			// Yes - fixed gain scaling for line input - the rest is done in hardware
			else	{
				gain_calc = (float)ts.tx_mic_gain_mult;		// We are in MIC In mode:  Calculate Microphone gain
				gain_calc /= MIC_GAIN_RESCALE;				// rescale microphone gain to a reasonable range
			}

			// Apply gain if not in TUNE mode
			//
			if(!ts.tune)	{
				arm_scale_f32((float32_t *)ads.a_buffer, (float32_t)gain_calc, (float32_t *)ads.a_buffer, size/2);	// apply gain
				//
				arm_max_f32((float32_t *)ads.a_buffer, size/2, &max, &pindex);		// find absolute value of audio in buffer after gain applied
				arm_min_f32((float32_t *)ads.a_buffer, size/2, &min, &pindex);
				min = fabs(min);
				if(min > max)
					max = min;
				ads.peak_audio = max;
			}
			//
			// Apply the TX equalization filtering:  This "flattens" the audio
			// prior to being applied to the Hilbert transformer as well as added low-pass filtering.
			// It does this by applying a "peak" to the bottom end to compensate for the roll-off caused by the Hilbert
			// and then a gradual roll-off toward the high end.  The net result is a very flat (to better than 1dB) response
			// over the 275-2500 Hz range.
			//
			if(!(ts.misc_flags1 & 8))	// Do the audio filtering *IF* it is to be enabled
				arm_iir_lattice_f32(&IIR_TXFilter, (float *)ads.a_buffer, (float *)ads.a_buffer, size/2);	// this is the 275-2500-ish bandpass filter with low-end pre-emphasis
			//
			// This is a phase-added 0-90 degree Hilbert transformer that also does low-pass and high-pass filtering
			// to the transmitted audio.  As noted above, it "clobbers" the low end, which is why we made up for it with the above filter.
			// Apply transformation AND audio filtering to buffer data
			//
			// + 0 deg to I data
			arm_fir_f32((arm_fir_instance_f32 *)&FIR_I_TX,(float32_t *)(ads.a_buffer),(float32_t *)(ads.i_buffer),size/2);
			// - 90 deg to Q data
			arm_fir_f32((arm_fir_instance_f32 *)&FIR_Q_TX,(float32_t *)(ads.a_buffer),(float32_t *)(ads.q_buffer), size/2);
			//
			// perform post-filter gain operation
			//
			gain_calc = (float)ts.alc_tx_postfilt_gain_var;		// get post-filter gain setting
			gain_calc /= 2;									// halve it
			gain_calc += 0.5;								// offset it so that 2 = unity
			//
			arm_scale_f32((float32_t *)ads.i_buffer, (float32_t)gain_calc, (float32_t *)ads.i_buffer, size/2);		// use optimized function to apply scaling to I/Q buffers
			arm_scale_f32((float32_t *)ads.q_buffer, (float32_t)gain_calc, (float32_t *)ads.q_buffer, size/2);
			//
			audio_tx_compressor(size, AM_ALC_GAIN_CORRECTION);	// Do the TX ALC and speech compression/processing
			//
			// First, generate the LOWER sideband of the AM signal
			// copy contents to temporary holding buffers for later generation of USB AM carrier
			//
			arm_copy_f32((float32_t *)ads.i_buffer, (float32_t *)ads.e_buffer, size/2);
			arm_copy_f32((float32_t *)ads.q_buffer, (float32_t *)ads.f_buffer, size/2);
			//
			//
			// generate AM carrier by applying a "DC bias" to the audio
			//
			arm_offset_f32((float32_t *)ads.i_buffer, AM_CARRIER_LEVEL, (float32_t *)ads.i_buffer, size/2);
			arm_offset_f32((float32_t *)ads.q_buffer, (-1 * AM_CARRIER_LEVEL), (float32_t *)ads.q_buffer, size/2);
			//
			// check and apply correct translate mode
			//
			if(ts.iq_freq_mode == FREQ_IQ_CONV_LO_HIGH)			// is it "RX LO HIGH" mode?
				audio_rx_freq_conv(size, 0);	// set "RX LO IS HIGH" mode
			else								// It must be "RX LO LOW" mode
				audio_rx_freq_conv(size, 1);	// set conversion to "RX LO IS LOW" mode
			//
			//
			// Equalize based on band and simultaneously apply I/Q gain adjustments
			//
			arm_scale_f32((float32_t *)ads.i_buffer, (float32_t)(ts.tx_power_factor * ts.tx_adj_gain_var_i * AM_GAIN_COMP), (float32_t *)ads.i_buffer, size/2);
			arm_scale_f32((float32_t *)ads.q_buffer, (float32_t)(ts.tx_power_factor * ts.tx_adj_gain_var_q * AM_GAIN_COMP), (float32_t *)ads.q_buffer, size/2);
			//
			ptr = dst;	// save a copy of the destination pointer to the DMA data before we increment it for production of USB data
			//
			// Output I and Q as stereo - storing an LSB AM signal in the DMA buffer
			//
			for(i = 0; i < size/2; i++)	{
				// Prepare data for DAC
				*dst++ = (int16_t)ads.q_buffer[i];	// save left channel
				*dst++ = (int16_t)ads.i_buffer[i];	// save right channel
			}
			//
			// Now, generate the upper sideband of the AM signal:  We must do much of this again to produce an identical (but "different") signal (opposite polarity)
			//
			arm_negate_f32((float32_t *)ads.e_buffer, (float32_t *)ads.q_buffer, size/2);
			arm_negate_f32((float32_t *)ads.f_buffer, (float32_t *)ads.i_buffer, size/2);
			//
			// generate AM carrier by applying a "DC bias" to the audio
			//
			arm_offset_f32((float32_t *)ads.i_buffer, AM_CARRIER_LEVEL, (float32_t *)ads.i_buffer, size/2);
			arm_offset_f32((float32_t *)ads.q_buffer, (-1 * AM_CARRIER_LEVEL), (float32_t *)ads.q_buffer, size/2);
			//
			// check and apply correct translate mode
			//
			if(ts.iq_freq_mode == FREQ_IQ_CONV_LO_HIGH)			// is it "RX LO HIGH" mode?
				audio_rx_freq_conv(size, 0);	// set "LO IS HIGH" mode
			else								// It must be "RX LO LOW" mode
				audio_rx_freq_conv(size, 1);	// set conversion to "RX LO IS LOW" mode
			//
			// Equalize based on band and simultaneously apply I/Q gain adjustments
			//
			arm_scale_f32((float32_t *)ads.i_buffer, (float32_t)(ts.tx_power_factor * ts.tx_adj_gain_var_i * AM_GAIN_COMP), (float32_t *)ads.i_buffer, size/2);
			arm_scale_f32((float32_t *)ads.q_buffer, (float32_t)(ts.tx_power_factor * ts.tx_adj_gain_var_q * AM_GAIN_COMP), (float32_t *)ads.q_buffer, size/2);
			//
			dst = ptr;	// restore the copy of the pointer to the DMA data (yes, I know we could have used "ptr" below...)
			//
			// Output I and Q as stereo
			for(i = 0; i < size/2; i++)	{
				//
				// Prepare data for DAC, adding the USB AM data to the already-stored LSB AM data
				//
				(*dst) += (int16_t)ads.q_buffer[i];	// save left channel
				dst++;
				(*dst) += (int16_t)ads.i_buffer[i];	// save right channel
				dst++;
			}
		}
		else	{	// Translate mode is NOT active - we CANNOT do full-carrier AM! (if we tried, we'd end up with DSB SSB because of the "DC hole"!))
			for(i = 0; i < size/2; i++)	{				// send nothing out to the DAC if AM attempted with translate mode turned off!
				*dst++ = 0;	// save left channel
				*dst++ = 0;	// save right channel
			}
		}
	}
	return;
}
//
// This is a stripped-down TX processor - work in progress
//
//*----------------------------------------------------------------------------
//* Function Name       : audio_dv_tx_processor
//* Object              :
//* Object              : audio sample processor for DV modes - USB only
//* Input Parameters    :
//* Output Parameters   :
//* Functions called    :
//*----------------------------------------------------------------------------
static void audio_dv_tx_processor(int16_t *src, int16_t *dst, int16_t size)
{
	static ulong 		i;
	float32_t			gain_calc, min, max;
//	int16_t				*ptr;
	uint32_t			pindex;
	int16_t				psize;		// processing size, with decimation
	float				post_agc_gain_scaling;


	psize = size/(int16_t)ads.decimation_rate;	// rescale sample size inside decimated portion based on decimation factor

	// Not tune mode - use audio from CODEC
	// Fill I and Q buffers with left channel(same as right)
	for(i = 0; i < size/2; i++)	{				// Copy to single buffer
		ads.a_buffer[i] = (float)*src;
		src += 2;								// Next sample
	}
	//
	if(ts.tx_audio_source == TX_AUDIO_LINEIN)		// Are we in LINE IN mode?
		gain_calc = LINE_IN_GAIN_RESCALE;			// Yes - fixed gain scaling for line input - the rest is done in hardware
	else	{
		gain_calc = (float)ts.tx_mic_gain_mult;		// We are in MIC In mode:  Calculate Microphone gain
		gain_calc /= MIC_GAIN_RESCALE;				// rescale microphone gain to a reasonable range
	}
	//
	//
	// Do decimation down to lower rate for heavy-duty processing to reduce processor load
	//
	arm_fir_decimate_f32(&DECIMATE_RX, (float32_t *)ads.a_buffer, (float32_t *)ads.a_buffer, size/2);		// LPF built into decimation (Yes, you can decimate-in-place!)
	//
	//
	// *****************************   DV Modulator goes here - ads.a_buffer must be at 8 ksps
	//
	//
	//
	//
	// Calculate scaling based on decimation rate since this affects the audio gain
	//
	post_agc_gain_scaling = POST_AGC_GAIN_SCALING_DECIMATE_4;
	//
	// Scale audio to according to AGC setting, demodulation mode and required fixed levels
	//
	arm_scale_f32((float32_t *)ads.a_buffer,(float32_t)(post_agc_gain_scaling), (float32_t *)ads.a_buffer, psize/2);	// apply fixed amount of audio gain scaling to make the audio levels correct along with AGC
	//
	// resample back to original sample rate while doing low-pass filtering to minimize aliasing effects
	//
	arm_fir_interpolate_f32(&INTERPOLATE_RX, (float32_t *)ads.a_buffer,(float32_t *) ads.b_buffer, psize/2);
	//
	//
	// Apply gain if not in TUNE mode
	arm_scale_f32((float32_t *)ads.a_buffer, (float32_t)gain_calc, (float32_t *)ads.a_buffer, size/2);	// apply gain
	//
	arm_max_f32((float32_t *)ads.a_buffer, size/2, &max, &pindex);		// find absolute value of audio in buffer after gain applied
	arm_min_f32((float32_t *)ads.a_buffer, size/2, &min, &pindex);
	min = fabs(min);
	if(min > max)
		max = min;
	ads.peak_audio = max;
	//
	// This is a phase-added 0-90 degree Hilbert transformer that also does low-pass and high-pass filtering
	// to the transmitted audio.
	//
	//
	gain_calc = (float)ts.alc_tx_postfilt_gain_var;		// get post-filter gain setting
	gain_calc /= 2;									// halve it
	gain_calc += 0.5;								// offset it so that 2 = unity
	arm_scale_f32((float32_t *)ads.i_buffer, (float32_t)gain_calc, (float32_t *)ads.i_buffer, size/2);		// use optimized function to apply scaling to I/Q buffers
	arm_scale_f32((float32_t *)ads.q_buffer, (float32_t)gain_calc, (float32_t *)ads.q_buffer, size/2);
	//

	if(ts.iq_freq_mode)	{		// is transmit frequency conversion to be done?
		if(ts.dmod_mode == DEMOD_LSB)	{		// Is it LSB?
			if(ts.iq_freq_mode == 1)			// yes - is it "RX LO HIGH" mode?
				audio_rx_freq_conv(size, 0);	// set conversion to "LO IS HIGH" mode
			else								// it is in "RX LO LOW" mode
				audio_rx_freq_conv(size, 1);	// set conversion to "RX LO LOW" mode
		}
		else	{								// It is USB!
			if(ts.iq_freq_mode == 1)			// yes - is it "RX LO HIGH" mode?
				audio_rx_freq_conv(size, 1);	// set conversion to "RX LO LOW" mode
			else								// it is in "RX LO LOW" mode
				audio_rx_freq_conv(size, 0);	// set conversion to "LO IS HIGH" mode
		}
	}

	//
	// Equalize based on band and simultaneously apply I/Q gain adjustments
	//
	arm_scale_f32((float32_t *)ads.i_buffer, (float32_t)(ts.tx_power_factor * ts.tx_adj_gain_var_i * SSB_GAIN_COMP), (float32_t *)ads.i_buffer, size/2);
	arm_scale_f32((float32_t *)ads.q_buffer, (float32_t)(ts.tx_power_factor * ts.tx_adj_gain_var_q * SSB_GAIN_COMP), (float32_t *)ads.q_buffer, size/2);
	//
	// ------------------------
	// Output I and Q as stereo data
	for(i = 0; i < size/2; i++)	{
		// Prepare data for DAC
		if(ts.dmod_mode == DEMOD_USB)	{
			*dst++ = (int16_t)ads.i_buffer[i];	// save left channel
			*dst++ = (int16_t)ads.q_buffer[i];	// save right channel
		}
		else	{		// Save in the opposite order for LSB
			*dst++ = (int16_t)ads.q_buffer[i];	// save left channel
			*dst++ = (int16_t)ads.i_buffer[i];	// save right channel
		}
	}
	return;
}

// ToDo: malloc it!
uchar   delay_idx = 0;
int16_t delay_buff[148];

// Simple pseudo stereo via reverb
static void audio_proc_fake_stereo_reverb(int16_t *dst, int16_t size)
{
	for(int i = 0; i < size/2; i++)
	{
		int16_t x = delay_buff[delay_idx + i*2];
		delay_buff[delay_idx + i*2] = *(dst + i*2);
		*(dst + i*2) = x;
	}

	delay_idx += size;
	if(delay_idx >= 128)
		delay_idx = 0;
}

//*----------------------------------------------------------------------------
//* Function Name       : I2S_RX_CallBack
//* Object              :
//* Object              : audio sample processor
//* Input Parameters    :
//* Output Parameters   :
//* Functions called    :
//*----------------------------------------------------------------------------
#if (DATA_SIZE == SAI_PROTOCOL_DATASIZE_16BIT)
void I2S_RX_CallBack(int16_t *src, int16_t *dst, int16_t size, uint16_t ht)
#else
void I2S_RX_CallBack(int32_t *src, int32_t *dst, int16_t size, uint16_t ht)
#endif
{
	static bool to_rx = 0;	// used as a flag to clear the RX buffer
	static bool to_tx = 0;	// used as a flag to clear the TX buffer
	static uchar lcd_dim = 0, lcd_dim_prescale = 0;
	static ulong tcount = 0;

	if((ts.txrx_mode == TRX_MODE_RX))
	{
		if((to_rx) || (ts.buffer_clear))
		{										// the first time back to RX, clear the buffers to reduce the "crash"
			to_rx = 0;							// caused by the content of the buffers from TX - used on return from SSB TX
			arm_fill_q15(0, dst, size);
			arm_fill_q15(0, src, size);
		}

		if(!ts.dvmode)
		{
			audio_rx_processor(src,dst,size);

			switch(ts.stereo_mode)
			{
				// Simple reverb
				case AUDIO_REVERB:
					audio_proc_fake_stereo_reverb(dst, size);
					break;

				// Mono
				default:
					break;
			}
		}
		else
		{
			audio_dv_rx_processor(src,dst,size);
		}

		to_tx = 1;		// Set flag to indicate that we WERE receiving when we go back to transmit mode
	}
	else	{			// Transmit mode
		if((to_tx) || (ts.tx_audio_muting_flag))	{	// the first time back to RX, or TX audio muting timer still active - clear the buffers to reduce the "crash"
			to_tx = 0;							// caused by the content of the buffers from TX - used on return from SSB TX
			arm_fill_q15(0, dst, size);
			arm_fill_q15(0, src, size);
		}
		else	{
			if(!ts.dvmode)
				audio_tx_processor(src,dst,size);
			else
				audio_dv_tx_processor(src,dst,size);
		}
		//
		to_rx = 1;		// Set flag to indicate that we WERE transmitting when we eventually go back to receive mode
	}
	//
	if(ts.unmute_delay_count)		// this updates at 375 Hz - used to time TX->RX delay
		ts.unmute_delay_count--;
	//
//!	ks.debounce_time++;				// keyboard debounce timer
//!	if(ks.debounce_time > DEBOUNCE_TIME_MAX)
//!		ks.debounce_time = DEBOUNCE_TIME_MAX;

	#if 0
	// Perform LCD backlight PWM brightness function
	//
	if(!ts.lcd_blanking_flag)	{	// is LCD *NOT* blanked?
		if(!lcd_dim_prescale)	{	// Only update dimming PWM counter every fourth time through to reduce frequency below that of audible range
			if(lcd_dim < ts.lcd_backlight_brightness)
				LCD_BACKLIGHT_PIO->BSRRH = LCD_BACKLIGHT;	// LCD backlight off
			else
				LCD_BACKLIGHT_PIO->BSRRL = LCD_BACKLIGHT;	// LCD backlight on
			//
			lcd_dim++;
			lcd_dim &= 3;	// limit brightness PWM count to 0-3
		}
		lcd_dim_prescale++;
		lcd_dim_prescale &= 3;	// limit prescale count to 0-3
	}
	else if(!ts.menu_mode)	// LCD is to be blanked - if NOT in menu mode
		LCD_BACKLIGHT_PIO->BSRRH = LCD_BACKLIGHT;	// LCD backlight off
	#endif

	#if 0
	tcount+=CLOCKS_PER_DMA_CYCLE;		// add the number of clock cycles that would have passed between DMA cycles
	if(tcount > CLOCKS_PER_CENTISECOND)	{	// has enough clock cycles for 0.01 second passed?
		tcount -= CLOCKS_PER_CENTISECOND;	// yes - subtract that many clock cycles
		ts.sysclock++;	// this clock updates at PRECISELY 100 Hz over the long term
	}
	//
	ts.thread_timer = 0;	// used to trigger the UI Driver thread
	//
	if(ts.spectrum_scope_scheduler)		// update thread timer if non-zero
		ts.spectrum_scope_scheduler--;
	#endif
}

