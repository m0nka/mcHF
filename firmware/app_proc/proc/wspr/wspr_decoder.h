/************************************************************************************
**                                                                                 **
**                                 mcHF QRP Transceiver                            **
**                         Krassi Atanassov - M0NKA, 2013-2026                     **
**                                                                                 **
**---------------------------------------------------------------------------------**
**                                                                                 **
**  File name:		wspr_decoder.h                                                 **
**  Description:	WSPR decoder core - public API                                 **
**  Last Modified:                                                                 **
**  Licence:		https://github.com/m0nka/mcHF/blob/main/LICENSE                **
************************************************************************************/
//
// Pure C WSPR (Weak Signal Propagation Reporter) decoder
//
// Algorithm constants (sync vector, interleaver, convolutional code, message
// packing) verified against K9AN/KA9Q wsprd (WSJT-X / rtlsdr-wsprd, GPLv3)
//
// Input:	16 bit signed mono PCM, 12000 Hz, up to 120 seconds of one rx cycle,
//			signal expected somewhere in the 1400..1600 Hz audio passband
// Output:	list of decoded type 1 messages (callsign, grid, power)
//
// No OS or HAL dependencies - core can be built and tested on a PC
// with -DWSPR_HOST_BUILD (large buffers then go to the normal heap/bss
// instead of the dedicated SDRAM section)
//
#ifndef __WSPR_DECODER_H
#define __WSPR_DECODER_H

#include <stdint.h>

// Sample rates
#define WSPR_FS_IN				12000						// input PCM rate, Hz
#define WSPR_FS_BB				375							// baseband rate, Hz
#define WSPR_CAPTURE_SEC		120							// max capture length, s

// WSPR signal structure
#define WSPR_NSYM				162							// channel symbols
#define WSPR_NBITS				81							// 50 msg + 31 tail bits
#define WSPR_SPS				256							// samples/symbol @ 375 Hz
#define WSPR_DF					(375.0f / 256.0f)			// tone spacing, Hz

// Audio passband
#define WSPR_CENTER_HZ			1500.0f						// nominal signal center
#define WSPR_SEARCH_HZ			110.0f						// search +/- around center

// Decoder dimensioning
#define WSPR_MAX_BB_SAMPLES		(WSPR_CAPTURE_SEC * WSPR_FS_BB)
#define WSPR_FFT_SIZE			512
#define WSPR_FFT_STEP			128
#define WSPR_MAX_FRAMES			(((WSPR_MAX_BB_SAMPLES - WSPR_FFT_SIZE) / WSPR_FFT_STEP) + 1)
#define WSPR_MAX_CAND			25							// candidates per cycle
#define WSPR_MAX_DECODES		16							// reported decodes per cycle

// One decoded transmission
typedef struct
{
	float	freq_hz;										// audio freq of tone group center
	float	snr_db;											// SNR in 2500 Hz ref bandwidth
	float	dt_sec;											// time offset vs nominal +1s start
	float	drift_hz;										// freq drift over transmission
	char	call[8];										// callsign (type 1)
	char	grid[6];										// 4 char locator
	int		dbm;											// reported tx power
	char	message[24];									// printable message text

} WSPR_DECODE;

// Reset internal state, call before feeding a new capture
void	wspr_decoder_reset	(void);

// Stream in PCM samples (any chunk size), 12 kHz mono, returns samples accepted
int		wspr_decoder_feed	(const int16_t *pcm, int num_samples);

// Run full decode pass over fed samples, returns number of decodes
int		wspr_decoder_run	(WSPR_DECODE *out, int max_out);

#endif
