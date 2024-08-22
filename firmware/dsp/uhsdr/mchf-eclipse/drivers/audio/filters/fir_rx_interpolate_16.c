/*  -*-  mode: c; tab-width: 4; indent-tabs-mode: t; c-basic-offset: 4; coding: utf-8  -*-  */
/************************************************************************************
**                                                                                 **
**                               mcHF QRP Transceiver                              **
**                             K Atanassov - M0NKA 2014                            **
**                                                                                 **
**---------------------------------------------------------------------------------**
**                                                                                 **
**  File name:                                                                     **
**  Description:                                                                   **
**  Last Modified:                                                                 **
**  Licence:		GNU GPLv3                                                      **
************************************************************************************/

#include "filters.h"

/**************************************************************

KA7OEI, November, 2014

IMPORTANT NOTE:

This low-pass filter is designed to remove aliasing artifacts from the interpolated output.

Response (15th order FIR, Least Pth-norm:  Fpass = 4000 Hz, Fstop = 8600 Hz Fsamp = 12000 Hz):

	-6dB:  5075 Hz
	-10dB: 5841 Hz
	-20dB: 7034 Hz
	-40dB: 8270 Hz
	-50dB:  >8550 Hz

Because the Nyquist Frequency is 6 kHz with the 12 kHz input sample rate, with filtered
audio limited to <3 kHz by the normal SSB audio filters, there will be NO aliased
content <9 kHz so the response of this low-pass filter may be relaxed to provide
a higher degree of filtering (e.g. >50dB) >8550 Hz.

In the case of the 3.6 kHz filter, the aliasing extends down no farther than 8200 Hz where the
attenuation is still at least 40dB where this, plus the effects of psychoacoustic masking
should make these artifacts inaudible.

***************************************************************/

const arm_fir_interpolate_instance_f32 FirRxInterpolate=
{
    .phaseLength = 16,
    .pCoeffs = (float*) (const float[])
    {
        0.0064096284297950057,
        0.017769515865099398,
        0.032257990537557804,
        0.040774242726362854,
        0.031402652968355385,
        -0.0044545611189248744,
        -0.065457689067971975,
        -0.13802671944933273,
        -0.20064935251937976,
        -0.23328570256042488,
        -0.22670802442881149,
        -0.18633542013951426,
        -0.1286861967088595,
        -0.072726968058963559,
        -0.031602889405696735,
        -0.009023372039440496
    },
    .pState = NULL
};

/*#####################################################
 *
 * Interpolation filter with 4 taps
 * 4 taps minimum for the ARM function to work
 * two versions:
 * 1.) for filter bandwidths between 3k4 and 4k8
 * 		--> 5kHz stop frequency
 * 2.) for filter bandwidths between 8k0 and 10k
 * 		--> 10kHz stop frequency
 *
 * these "filters" do not filter much with 4 taps . . .
 * FIR 4 taps, Fstop 5kHz / 10kHz, Rectangular
 * designed with Iowa Hills FIR Filter Designer
 * DD4WH 2016_03_11
 *
 * ####################################################
 */

const arm_fir_interpolate_instance_f32 FirRxInterpolate_4_5k=
{
    .phaseLength = 4,
    .pCoeffs = (float*) (const float[])
    {
        0.210149639888675743,
        0.292367020487055873,
        0.292367020487055873,
        0.210149639888675743
    },
    .pState = NULL
};

const arm_fir_interpolate_instance_f32 FirRxInterpolate_4_10k=
{
    .phaseLength = 4,
    .pCoeffs = (float*) (const float[])
    {
        0.139302961114601442,
        0.460435107346899464,
        0.460435107346899464,
        0.139302961114601442
    },
    .pState = NULL
};

// FIXME: Is this the right file for a FreeDV TX filter?
// this is meant to be an interpolation filter for FreeDV
// cutoff 2.4kHz (30 Taps)

const arm_fir_instance_f32 Fir_TxFreeDV_Interpolate =
{
    .numTaps = FIR_TX_FREEDV_INTERPOLATE_NUM_TAPS,
    .pCoeffs = (float*) (const float[])
    {
    	-0.001351058376639693,
    	-0.003185783059285283,
    	-0.005575655191465650,
    	-0.007911042143847070,
    	-0.009195803961917746,
    	-0.008176776032790899,
    	-0.003590261795373249,
    	 0.005518212389136743,
    	 0.019475233722896626,
    	 0.037749136460739165,
    	 0.058883910956281677,
    	 0.080638648048914893,
    	 0.100323507980679211,
    	 0.115270746696726323,
    	 0.123339678809895686,
    	 0.123339678809895686,
    	 0.115270746696726323,
    	 0.100323507980679211,
    	 0.080638648048914893,
    	 0.058883910956281677,
    	 0.037749136460739165,
    	 0.019475233722896626,
    	 0.005518212389136743,
    	-0.003590261795373249,
    	-0.008176776032790899,
    	-0.009195803961917746,
    	-0.007911042143847070,
    	-0.005575655191465650,
    	-0.003185783059285283,
    	-0.001351058376639693
    },
    .pState = NULL
};
/*

// And here for you to try the same filter with 60 taps
// this is meant to be an interpolation filter for FreeDV
// cutoff 2.4kHz

const arm_fir_instance_f32 FirFreeDVInterpolate=
{
    .numTaps = 30,
    .pCoeffs = (float*) (const float[])
    {
 -325.7026576032276350E-6,
-371.9001810615538940E-6,
-297.4863280206030250E-6,
-42.52080605332919560E-6,
 430.2851136718793440E-6,
 0.001114933511882014,
 0.001945133332895070,
 0.002786351459208297,
 0.003441665727387134,
 0.003674210056274392,
 0.003245972098811801,
 0.001969072608194520,
-237.8878302134766050E-6,
-0.003298503514874146,
-0.006943439896700603,
-0.010701717349100807,
-0.013925451717764645,
-0.015850079401508791,
-0.015685944805004001,
-0.012730898974573298,
-0.006488430580536590,
 0.003227089128632603,
 0.016216448931132731,
 0.031868083873548676,
 0.049186001642367742,
 0.066873952747489043,
 0.083467736074663401,
 0.097499421337832640,
 0.107671414315587272,
 0.113015797883873961,
 0.113015797883873961,
 0.107671414315587272,
 0.097499421337832640,
 0.083467736074663401,
 0.066873952747489043,
 0.049186001642367742,
 0.031868083873548676,
 0.016216448931132731,
 0.003227089128632603,
-0.006488430580536590,
-0.012730898974573298,
-0.015685944805004001,
-0.015850079401508791,
-0.013925451717764645,
-0.010701717349100807,
-0.006943439896700603,
-0.003298503514874146,
-237.8878302134766050E-6
 0.001969072608194520,
 0.003245972098811801,
 0.003674210056274392,
 0.003441665727387134,
 0.002786351459208297,
 0.001945133332895070,
 0.001114933511882014,
 430.2851136718793440E-6,
-42.52080605332919560E-6,
-297.4863280206030250E-6,
-371.9001810615538940E-6,
-325.7026576032276350E-6
    }
};
 */

const float32_t Fir_Rx_FreeDV_Interpolate_Coeffs[24] =
{
    -0.017405444562935534,
	-0.017108768884742161,
	-0.013120768604810904,
	-0.004804294719432716,
	 0.007973629566960488,
	 0.024740722876424703,
	 0.044407570327122774,
	 0.065355761534977047,
	 0.085619214114458830,
	 0.103134409504618857,
	 0.116021478863062078,
	 0.122850816384744946,
	 0.122850816384744946,
	 0.116021478863062078,
	 0.103134409504618857,
	 0.085619214114458830,
	 0.065355761534977047,
	 0.044407570327122774,
	 0.024740722876424703,
	 0.007973629566960488,
	-0.004804294719432716,
	-0.013120768604810904,
	-0.017108768884742161,
	-0.017405444562935534
};
