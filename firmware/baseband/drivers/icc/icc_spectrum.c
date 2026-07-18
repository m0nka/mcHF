/************************************************************************************
**                                                                                 **
**                                 mcHF QRP Transceiver                            **
**                         Krassi Atanassov - M0NKA, 2013-2026                     **
**                                                                                 **
**---------------------------------------------------------------------------------**
**                                                                                 **
**  File name:		icc_spectrum.c                                                 **
**  Description:	self contained 2048 point FFT spectrum/waterfall processor,    **
**					direct port of the CLINT dsp_idle spectrum path so the        **
**					M7 UI receives byte identical data                             **
**  Licence:		https://github.com/m0nka/mcHF/blob/main/LICENSE                **
************************************************************************************/

// Compiled only for the STM32H747 CM4 baseband build
#ifdef H7_M4_CORE

#include <stdio.h>
#include <string.h>
#include <math.h>

#include "arm_math.h"

#include "icc_spectrum.h"
#include "icc_proc.h"

// Same sizes as the CLINT project in DSP_MODE
#define FFT_IQ_BUFF_LEN1				2048
#define FFT_IQ_BUFF_M1_HALF				((FFT_IQ_BUFF_LEN1-1)/2)
#define FFT_QUADRATURE_PROC				1

#define SPECTRUM_HEIGHT1				110
#define SPECTRUM_SCOPE_ADJUST_OFFSET	100
#define SPECTRUM_AGC_SCALING			25
#define SCOPE_PREAMP_GAIN				1000
#define INIT_SPEC_AGC_LEVEL				-80

// dB/division scaling factors (from common/mchf_icc_def.h, duplicated here
// to keep this translation unit free of the wire protocol header)
#define DB_SCALING_5					63.2456
#define DB_SCALING_7					42.1637
#define DB_SCALING_10					31.6228
#define DB_SCALING_15					21.0819
#define DB_SCALING_20					15.8114
#define DB_SCALING_S1					52.7046
#define DB_SCALING_S2					26.3523
#define DB_SCALING_S3					17.5682

// values of the wire protocol spectrum_db_scale field
enum
{
	ICC_DB_DIV_DEFAULT = 0,
	ICC_DB_DIV_5,
	ICC_DB_DIV_7,
	ICC_DB_DIV_10,
	ICC_DB_DIV_15,
	ICC_DB_DIV_20,
	ICC_S_1_DIV,
	ICC_S_2_DIV,
	ICC_S_3_DIV
};

// values of the wire protocol fft_window_type field
enum
{
	ICC_FFT_WINDOW_RECTANGULAR = 0,
	ICC_FFT_WINDOW_COSINE,
	ICC_FFT_WINDOW_BARTLETT,
	ICC_FFT_WINDOW_WELCH,
	ICC_FFT_WINDOW_HANN,
	ICC_FFT_WINDOW_HAMMING,
	ICC_FFT_WINDOW_BLACKMAN,
	ICC_FFT_WINDOW_NUTTALL
};

// Spectrum processor state
typedef struct
{
	arm_rfft_instance_f32			S;
	arm_cfft_radix4_instance_f32	S_CFFT;

	// Collected IQ + FFT output (rfft writes 2*N values)
	float32_t	FFT_Samples[FFT_IQ_BUFF_LEN1*2];

	volatile uint32_t	samp_ptr;
	volatile uint8_t	state;			// 0 = collecting, 1 = ready for FFT
	uint8_t				enabled;

	float		display_offset;
	float		agc_rate;
	float		db_scale;

	// display processing settings from the M7 core
	uint8_t		fft_window_type;
	uint8_t		scope_filter;
	uint8_t		spectrum_scope_nosig_adjust;
	uint8_t		rf_codec_gain;

} icc_spectrum_state_t;

static icc_spectrum_state_t		isd;

static float32_t	sd_FFT_AVGData[FFT_IQ_BUFF_LEN1/2];		// IIR low-pass filtered FFT buffer data
static q15_t		sd_FFT_DspData[FFT_IQ_BUFF_LEN1];		// Rescaled and de-linearized display data
static float32_t	sd_FFT_MagData[FFT_IQ_BUFF_LEN1/2];
static float32_t	sd_FFT_Windat[FFT_IQ_BUFF_LEN1];

static uint8_t		ou_svalue = 1;

//*----------------------------------------------------------------------------
//* Function Name       : icc_spectrum_apply_settings
//* Object              : load display processing settings from the wire state
//* Context    			: CONTEXT_ICC
//*----------------------------------------------------------------------------
void icc_spectrum_apply_settings(const icc_radio_settings_t *st)
{
	isd.fft_window_type				= st->fft_window_type;
	isd.scope_filter				= st->scope_filter;
	isd.spectrum_scope_nosig_adjust	= st->spectrum_scope_nosig_adjust;
	isd.rf_codec_gain				= st->rf_codec_gain;

	// Filter setting sanity - used as divider
	if(isd.scope_filter == 0)
		isd.scope_filter = 4;

	// AGC rate
	isd.agc_rate = (float)st->scope_agc_rate;
	if(isd.agc_rate < 1)
		isd.agc_rate = 1;
	isd.agc_rate = isd.agc_rate/SPECTRUM_AGC_SCALING;

	// dB/division scaling
	switch(st->spectrum_db_scale)
	{
		case ICC_DB_DIV_5:
			isd.db_scale = DB_SCALING_5;
			break;
		case ICC_DB_DIV_7:
			isd.db_scale = DB_SCALING_7;
			break;
		case ICC_DB_DIV_15:
			isd.db_scale = DB_SCALING_15;
			break;
		case ICC_DB_DIV_20:
			isd.db_scale = DB_SCALING_20;
			break;
		case ICC_S_1_DIV:
			isd.db_scale = DB_SCALING_S1;
			break;
		case ICC_S_2_DIV:
			isd.db_scale = DB_SCALING_S2;
			break;
		case ICC_S_3_DIV:
			isd.db_scale = DB_SCALING_S3;
			break;
		case ICC_DB_DIV_10:
		default:
			isd.db_scale = DB_SCALING_10;
			break;
	}
}

//*----------------------------------------------------------------------------
//* Function Name       : icc_spectrum_init
//* Object              : init FFT instances and publics
//* Context    			: CONTEXT_ICC
//*----------------------------------------------------------------------------
void icc_spectrum_init(void)
{
	arm_status	a;

	isd.state			= 0;
	isd.samp_ptr		= 0;
	isd.display_offset	= INIT_SPEC_AGC_LEVEL;

	// Defaults if the M7 core did not upload the state yet
	if(isd.db_scale == 0)
		isd.db_scale = DB_SCALING_10;

	if(isd.agc_rate == 0)
		isd.agc_rate = 25.0/SPECTRUM_AGC_SCALING;

	if(isd.scope_filter == 0)
		isd.scope_filter = 4;

	if(isd.spectrum_scope_nosig_adjust == 0)
		isd.spectrum_scope_nosig_adjust = 20;

	if(isd.fft_window_type == 0)
		isd.fft_window_type = ICC_FFT_WINDOW_BLACKMAN;

	// Init the FFT instances, same setup as the CLINT project
	a = arm_rfft_init_f32((arm_rfft_instance_f32 *)&isd.S,
						  (arm_cfft_radix4_instance_f32 *)&isd.S_CFFT,
						  FFT_IQ_BUFF_LEN1,
						  FFT_QUADRATURE_PROC,
						  1);
	if(a != ARM_MATH_SUCCESS)
	{
		printf("fft init err %d\r\n", a);
		return;
	}

	isd.enabled = 1;
}

//*----------------------------------------------------------------------------
//* Function Name       : icc_spectrum_collect
//* Object              : collect raw IQ samples for the FFT
//* Context    			: CONTEXT_IRQ (audio DMA block handler)
//*----------------------------------------------------------------------------
void icc_spectrum_collect(volatile int16_t *src, uint32_t num_frames)
{
	uint32_t i;

	if((!isd.enabled) || (isd.state != 0))
		return;

	for(i = 0; i < num_frames; i++)
	{
		// get floating point data for FFT for spectrum scope/waterfall display.
		// Channel order swapped vs the CLINT project - the UHSDR RX processor
		// uses the opposite I/Q assignment, this keeps the display unmirrored
		isd.FFT_Samples[isd.samp_ptr] = (float32_t)(*(src));
		isd.samp_ptr++;
		isd.FFT_Samples[isd.samp_ptr] = (float32_t)(*(src + 1));
		isd.samp_ptr++;

		src += 2;

		// Obtain samples needed for FFT
		if(isd.samp_ptr >= FFT_IQ_BUFF_LEN1*2)
		{
			isd.samp_ptr = 0;
			isd.state    = 1;
			return;
		}
	}
}

void icc_spectrum_set_smeter(uint8_t s_value)
{
	if(s_value == 0)
		s_value = 1;

	ou_svalue = s_value;
}

//*----------------------------------------------------------------------------
//* Function Name       : icc_spectrum_get_buffer
//* Object              : fill the ICC broadcast packet for the M7 core
//* Context    			: CONTEXT_ICC
//*----------------------------------------------------------------------------
uint16_t icc_spectrum_get_buffer(uint8_t *buffer)
{
	uint32_t k;

	if(buffer == NULL)
		return 0;

	// Header - 10 bytes of meta data
	//
	buffer[0] = 0x9F;		// sig
	buffer[1] = ou_svalue;	// S-meter

	// Then 1024 bytes of Spectrum data
	buffer += 10;

	// Left part of screen
	for(k = 0; k < 512; k++)
		buffer[k +   0] = (uint8_t)*(sd_FFT_DspData + k + 512);

	// Right part of screen
	for(k = 0; k < 512; k++)
		buffer[k + 512] = (uint8_t)*(sd_FFT_DspData + k + 0);

	return 1034;
}

//*----------------------------------------------------------------------------
//* Function Name       : icc_spectrum_codec_gain_factor
//* Object              : voltage gain factor of the codec A/D input stage
//* Context    			: CONTEXT_ICC
//*----------------------------------------------------------------------------
static float32_t icc_spectrum_codec_gain_factor(void)
{
	float32_t gcalc;

	// codec has 1.5 dB/step, offset by 34.5db (full gain = 12dB)
	gcalc = (float32_t)isd.rf_codec_gain;
	if(gcalc > 31)
		gcalc = 31;

	gcalc *= 1.5;
	gcalc -= 34.5;
	gcalc = powf(10.0f, gcalc/10.0f);	// convert to power ratio

	return sqrtf(gcalc);				// convert to voltage ratio
}

//*----------------------------------------------------------------------------
//* Function Name       : icc_spectrum_thread
//* Object              : do the FFT/mag/average/rescale processing when a
//* Object              : full sample buffer was collected. Direct port of
//* Object              : the CLINT UiDriverReDrawSpectrumDisplay()
//* Context    			: CONTEXT_ICC (superloop)
//*----------------------------------------------------------------------------
void icc_spectrum_thread(void)
{
	uint32_t 	i, spec_width;
	uint32_t	max_ptr;	// throw-away pointer for ARM maxval and minval functions
	float32_t	gcalc;

	q15_t   	sd_FFT_TempData[FFT_IQ_BUFF_LEN1];

	if(!isd.enabled)
		return;

	// enough samples collected ?
	if(isd.state != 1)
		return;

	gcalc = icc_spectrum_codec_gain_factor();
	gcalc = 1/gcalc;				// Get gain setting of codec and convert to multiplier factor

	// scale input according to A/D gain
	arm_scale_f32((float32_t *)isd.FFT_Samples, (float32_t)(gcalc * SCOPE_PREAMP_GAIN), (float32_t *)isd.FFT_Samples, FFT_IQ_BUFF_LEN1);

	// Do windowing function on input data to get less "Bin Leakage" on FFT data
	switch(isd.fft_window_type)
	{
		case ICC_FFT_WINDOW_RECTANGULAR:	// No processing at all
			arm_copy_f32((float32_t *)isd.FFT_Samples, (float32_t *)sd_FFT_Windat, FFT_IQ_BUFF_LEN1);
			break;
		case ICC_FFT_WINDOW_COSINE:			// Sine window function (a.k.a. "Cosine Window")
			for(i = 0; i < FFT_IQ_BUFF_LEN1; i++)
			{
				sd_FFT_Windat[i] = arm_sin_f32((PI * (float32_t)i)/FFT_IQ_BUFF_LEN1 - 1) * isd.FFT_Samples[i];
			}
			break;
		case ICC_FFT_WINDOW_BARTLETT:		// a.k.a. "Triangular" window
			for(i = 0; i < FFT_IQ_BUFF_LEN1; i++)
			{
				sd_FFT_Windat[i] = (1 - fabs(i - ((float32_t)FFT_IQ_BUFF_M1_HALF))/(float32_t)FFT_IQ_BUFF_M1_HALF) * isd.FFT_Samples[i];
			}
			break;
		case ICC_FFT_WINDOW_WELCH:			// Parabolic window function
			for(i = 0; i < FFT_IQ_BUFF_LEN1; i++)
			{
				sd_FFT_Windat[i] = (1 - ((i - ((float32_t)FFT_IQ_BUFF_M1_HALF))/(float32_t)FFT_IQ_BUFF_M1_HALF)*((i - ((float32_t)FFT_IQ_BUFF_M1_HALF))/(float32_t)FFT_IQ_BUFF_M1_HALF)) * isd.FFT_Samples[i];
			}
			break;
		case ICC_FFT_WINDOW_HANN:			// Raised Cosine Window (non zero-phase version)
			for(i = 0; i < FFT_IQ_BUFF_LEN1; i++)
			{
				sd_FFT_Windat[i] = 0.5 * (float32_t)((1 - (arm_cos_f32(PI*2 * (float32_t)i / (float32_t)(FFT_IQ_BUFF_LEN1-1)))) * isd.FFT_Samples[i]);
			}
			break;
		case ICC_FFT_WINDOW_HAMMING:		// Another Raised Cosine window
			for(i = 0; i < FFT_IQ_BUFF_LEN1; i++)
			{
				sd_FFT_Windat[i] = (float32_t)((0.53836 - (0.46164 * arm_cos_f32(PI*2 * (float32_t)i / (float32_t)(FFT_IQ_BUFF_LEN1-1)))) * isd.FFT_Samples[i]);
			}
			break;
		case ICC_FFT_WINDOW_NUTTALL:		// Slightly wider than Blackman, comparable sidelobe rejection
			for(i = 0; i < FFT_IQ_BUFF_LEN1; i++)
			{
				sd_FFT_Windat[i] = (0.355768 - (0.487396*arm_cos_f32((2*PI*(float32_t)i)/(float32_t)FFT_IQ_BUFF_LEN1-1)) + (0.144232*arm_cos_f32((4*PI*(float32_t)i)/(float32_t)FFT_IQ_BUFF_LEN1-1)) - (0.012604*arm_cos_f32((6*PI*(float32_t)i)/(float32_t)FFT_IQ_BUFF_LEN1-1))) * isd.FFT_Samples[i];
			}
			break;
		case ICC_FFT_WINDOW_BLACKMAN:		// probably best for "default" use
		default:
			for(i = 0; i < FFT_IQ_BUFF_LEN1; i++)
			{
				sd_FFT_Windat[i] = (0.42659 - (0.49656*arm_cos_f32((2*PI*(float32_t)i)/(float32_t)FFT_IQ_BUFF_LEN1-1)) + (0.076849*arm_cos_f32((4*PI*(float32_t)i)/(float32_t)FFT_IQ_BUFF_LEN1-1))) * isd.FFT_Samples[i];
			}
			break;
	}

	// Do FFT
	arm_rfft_f32((arm_rfft_instance_f32 *)&isd.S, (float32_t *)(sd_FFT_Windat), (float32_t *)(isd.FFT_Samples));

	// Calculate magnitude
	arm_cmplx_mag_f32((float32_t *)(isd.FFT_Samples), (float32_t *)(sd_FFT_MagData), (FFT_IQ_BUFF_LEN1/2));

	// IIR lowpass filtering to "smooth" display
	float32_t	filt_factor;

	filt_factor = (float)isd.scope_filter;		// use stored filter setting
	filt_factor = 1/filt_factor;				// invert filter factor to allow multiplication

	arm_scale_f32((float32_t *)sd_FFT_AVGData, (float32_t)filt_factor, (float32_t *)isd.FFT_Samples, FFT_IQ_BUFF_LEN1/2);	// get scaled version of previous data
	arm_sub_f32  ((float32_t *)sd_FFT_AVGData, (float32_t *)isd.FFT_Samples, (float32_t *)sd_FFT_AVGData, FFT_IQ_BUFF_LEN1/2);	// subtract scaled information from old, average data
	arm_scale_f32((float32_t *)sd_FFT_MagData, (float32_t)filt_factor, (float32_t *)isd.FFT_Samples, FFT_IQ_BUFF_LEN1/2);	// get scaled version of new, input data
	arm_add_f32  ((float32_t *)isd.FFT_Samples, (float32_t *)sd_FFT_AVGData, (float32_t *)sd_FFT_AVGData, FFT_IQ_BUFF_LEN1/2);	// add portion new, input data into average

	for(i = 0; i < FFT_IQ_BUFF_LEN1/2; i++)
	{
		// guarantee that the result will always be >= 0
		if(sd_FFT_AVGData[i] < 1)
			sd_FFT_AVGData[i] = 1;
	}

	q15_t		max1, min1;
	q15_t		mean1;
	float32_t	sig;

	// De-linearize data with dB/division
	for(i = 0; i < (FFT_IQ_BUFF_LEN1/2); i++)
	{
		sig = log10(sd_FFT_AVGData[i]) * isd.db_scale;		// take FFT data, do a log10 and multiply it to scale it to get desired dB/division
		sig += isd.display_offset;							// apply "AGC", vertical "sliding" offset (or brightness for waterfall)
		if(sig > 1)											// is the value greater than 1?
			sd_FFT_DspData[i] = (q15_t)sig;					// it was a useful value - save it
		else
			sd_FFT_DspData[i] = 1;							// not greater than 1 - assign it to a base value of 1 for sanity's sake
	}

	arm_copy_q15((q15_t *)sd_FFT_DspData, (q15_t *)sd_FFT_TempData, FFT_IQ_BUFF_LEN1/2);

	// Find peak and average to vertically adjust display
	spec_width = FFT_IQ_BUFF_LEN1/2;
	arm_max_q15 ((q15_t *)sd_FFT_TempData, spec_width, &max1, &max_ptr);	// find maximum element
	arm_min_q15 ((q15_t *)sd_FFT_TempData, spec_width, &min1, &max_ptr);	// find minimum element
	arm_mean_q15((q15_t *)sd_FFT_TempData, spec_width, &mean1);				// find mean value

	if(mean1 == 0)
		mean1 = 1;

	// Vertically adjust spectrum scope so that the strongest signals are adjusted to the top
	if(max1 > SPECTRUM_HEIGHT1)
	{
		// is result higher than display
		isd.display_offset -= isd.agc_rate;		// yes, adjust downwards quickly
	}
	// Prevent "empty" spectrum display from filling with "noise"
	else if(((max1*10/mean1) <= (q15_t)isd.spectrum_scope_nosig_adjust) && (max1 < SPECTRUM_HEIGHT1+(SPECTRUM_HEIGHT1/2)))
	{
		// was "average" signal ratio below set threshold and average is not insanely strong??
		if((min1 > 2) && (max1 > 2))
		{
			// prevent the adjustment from going downwards, "into the weeds"
			isd.display_offset -= isd.agc_rate;	// yes, adjust downwards
			if(isd.display_offset < (-(SPECTRUM_HEIGHT1 + SPECTRUM_SCOPE_ADJUST_OFFSET)))
				isd.display_offset = (-(SPECTRUM_HEIGHT1 + SPECTRUM_SCOPE_ADJUST_OFFSET));
		}
	}
	else
		isd.display_offset += (isd.agc_rate/3);	// no, adjust upwards more slowly

	if((min1 <= 2) && (max1 <= 2))
	{
		// We must already be in the weeds, below the bottom - adjust upwards quickly
		isd.display_offset += isd.agc_rate*10;
	}

	// Crude S-meter estimate from the strongest bin of the averaged FFT data.
	// ToDo: calibrate ICC_SMETER_CAL on real hardware and restrict the search
	// to the actual filter passband around the RX carrier
	{
		#define ICC_SMETER_CAL		(-110.0f)

		float32_t	max_avg;
		uint32_t	max_idx;
		float32_t	dbm;
		int32_t		s_val;

		arm_max_f32((float32_t *)sd_FFT_AVGData, FFT_IQ_BUFF_LEN1/2, &max_avg, &max_idx);
		dbm = 10.0f * log10f(max_avg) + ICC_SMETER_CAL;

		if(dbm <= -121.0f)
			s_val = 1;								// below S1
		else if(dbm <= -73.0f)
			s_val = 1 + (int32_t)((dbm + 121.0f)/6.0f);		// S1..S9, 6dB per unit
		else
			s_val = 9 + (int32_t)((dbm + 73.0f)/10.0f);		// above S9, 10dB steps

		if(s_val > 34)
			s_val = 34;

		ou_svalue = (uint8_t)s_val;
	}

	// Notify M7 core
	icc_proc_notify_fft_ready();

	// Allow collection in the audio driver
	isd.state = 0;
}

#endif // H7_M4_CORE
