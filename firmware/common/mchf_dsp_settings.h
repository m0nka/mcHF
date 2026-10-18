/************************************************************************************
**                                                                                 **
**                                 mcHF QRP Transceiver                            **
**                         Krassi Atanassov - M0NKA, 2013-2026                     **
**                                                                                 **
**---------------------------------------------------------------------------------**
**                                                                                 **
**  File name:		mchf_dsp_settings.h                                            **
**  Description:	UHSDR DSP settings, edited in the Baseband menu on the M7    **
**					M7 core and applied by the baseband on the M4 core. Pure      **
**					macros, no types - safe to include on both sides (the M4       **
**					UHSDR code can not include mchf_icc_def.h)                     **
**  Licence:		https://github.com/m0nka/mcHF/blob/main/LICENSE                **
************************************************************************************/
#ifndef __MCHF_DSP_SETTINGS_H
#define __MCHF_DSP_SETTINGS_H

// Wire/storage layout version - bump when the list below is reordered or an
// entry changes meaning (appending is fine, older data just misses the tail)
#define DSP_SET_VERSION					1

// ICC_SET_DSP_SETTINGS payload:
// [0]      DSP_SET_VERSION
// [1]      number of values that follow
// [2.. ]   values, int16 little endian, in the order of the list below
#define DSP_SET_HDR_SIZE				2

// All settings, X(id, default, min, max). Values are in the units the menu
// shows, the M4 side does any scaling. Units/meaning:
//
// AGC_SLOPE           dB (UHSDR stores dB*10)
// AGC_DECAY_xxx       ms, per AGC mode - LONG is what the CUSTOM mode runs
// AGC_HANG_MODE       0 auto (per mode preset), 1 off, 2 on (AGC_HANG_TIME)
// AGC_HANG_THRESH     dB
// NB_LEVEL            1..15, the blanker only runs at the 12 kHz decimation
// MNOTCH/MPEAK_FREQ   Hz
// NR_BETA             x1000
// SAM_ENABLE          the AM mode is demodulated as synchronous AM
// SAM_PLL_ZETA        x100
// RX/TX_BASS/TREBLE   dB
// FM_DEV_5K           0 = 2.5 kHz, 1 = 5 kHz deviation
// FM_SQUELCH          0 = open
// FM_TONE_BURST       0 off, 1 1750 Hz, 2 2135 Hz
// FM_CTCSS_xxx        index into the UHSDR tone table, 0 = off
// TX_COMP_LEVEL       -1 off, 0..12, 13 custom (TX_ALC_RELEASE/TX_ALC_GAIN)
// TX_SSB_FILTER       1 soprano, 2 tenor, 3 bass
// TX_TUNE_TONE        0 single, 1 two tone
// CW_WEIGHT           dit/space ratio x100
// CW_RX_DELAY         x10 ms
//
// The RF gain (= WDSP AGC threshold) is not here, it lives on the on-screen
// AGC dialog together with the AGC mode
#define DSP_SET_LIST(X) \
	X(AGC_SLOPE,			7,		0,		20)		\
	X(AGC_DECAY_LONG,		2000,	10,		5000)	\
	X(AGC_DECAY_SLOW,		500,	10,		5000)	\
	X(AGC_DECAY_MED,		250,	10,		5000)	\
	X(AGC_DECAY_FAST,		50,		10,		5000)	\
	X(AGC_HANG_MODE,		0,		0,		2)		\
	X(AGC_HANG_TIME,		250,	10,		5000)	\
	X(AGC_HANG_THRESH,		45,		-20,	120)	\
	X(AGC_HANG_DECAY,		500,	100,	5000)	\
	X(NR_ENABLE,			0,		0,		1)		\
	X(NR_STRENGTH,			160,	1,		200)	\
	X(NB_ENABLE,			0,		0,		1)		\
	X(NB_LEVEL,				10,		1,		15)		\
	X(ANOTCH_ENABLE,		0,		0,		1)		\
	X(ANOTCH_RATE,			10,		0,		40)		\
	X(MNOTCH_ENABLE,		0,		0,		1)		\
	X(MNOTCH_FREQ,			800,	200,	5000)	\
	X(MPEAK_ENABLE,			0,		0,		1)		\
	X(MPEAK_FREQ,			750,	200,	5000)	\
	X(NR_BETA,				960,	700,	999)	\
	X(NR_ASNR,				30,		2,		30)		\
	X(NR_SMOOTH_WIDTH,		4,		1,		5)		\
	X(NR_SMOOTH_THRESH,		40,		10,		100)	\
	X(SAM_ENABLE,			0,		0,		1)		\
	X(SAM_PLL_RANGE,		500,	50,		8000)	\
	X(SAM_PLL_ZETA,			65,		1,		100)	\
	X(SAM_PLL_BW,			250,	25,		1000)	\
	X(SAM_FADE_LEVELER,		0,		0,		1)		\
	X(IQ_AUTO_CORR,			1,		0,		1)		\
	X(RX_BASS,				0,		-20,	20)		\
	X(RX_TREBLE,			0,		-20,	20)		\
	X(FM_DEV_5K,			0,		0,		1)		\
	X(FM_SQUELCH,			0,		0,		20)		\
	X(FM_TONE_BURST,		0,		0,		2)		\
	X(FM_CTCSS_GEN,			0,		0,		55)		\
	X(FM_CTCSS_DET,			0,		0,		55)		\
	X(TX_MIC_GAIN,			15,		2,		99)		\
	X(TX_COMP_LEVEL,		4,		-1,		13)		\
	X(TX_ALC_RELEASE,		10,		0,		20)		\
	X(TX_ALC_GAIN,			1,		1,		25)		\
	X(TX_SSB_FILTER,		1,		1,		3)		\
	X(TX_AM_FILTER,			1,		0,		1)		\
	X(TX_TUNE_TONE,			0,		0,		1)		\
	X(TX_BASS,				0,		-20,	5)		\
	X(TX_TREBLE,			0,		-20,	5)		\
	X(CW_SPEED,				12,		5,		48)		\
	X(CW_WEIGHT,			100,	50,		150)	\
	X(CW_SIDETONE,			750,	400,	1000)	\
	X(CW_PADDLE_REV,		0,		0,		1)		\
	X(CW_RX_DELAY,			8,		0,		50)

#define DSP_SET_ENUM(id, def, min, max)		DSP_SET_##id,

enum
{
	DSP_SET_LIST(DSP_SET_ENUM)
	DSP_SET_COUNT
};

#endif
