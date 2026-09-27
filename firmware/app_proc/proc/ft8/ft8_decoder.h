/************************************************************************************
**                                                                                 **
**                                 mcHF QRP Transceiver                            **
**                         Krassi Atanassov - M0NKA, 2013-2026                     **
**                                                                                 **
**---------------------------------------------------------------------------------**
**                                                                                 **
**  File name:		ft8_decoder.h                                                  **
**  Description:	FT8 slot decoder (ft8_lib host), portable - also builds on PC  **
**  Last Modified:                                                                 **
**  Licence:		https://github.com/m0nka/mcHF/blob/main/LICENSE                **
************************************************************************************/
#ifndef __FT8_DECODER_H
#define __FT8_DECODER_H

#include <stdint.h>

#include "ft8/decode.h"

// Waterfall geometry - must match whoever produces the magnitudes (the host
// wf_gen.py tool now, the M4 FFT front end later). ft8_lib reference values:
// 12 kHz audio, 200..3000 Hz analysed, 2x time and 2x frequency oversampling
#define FT8_SAMPLE_RATE				12000
#define FT8_TIME_OSR				2
#define FT8_FREQ_OSR				2
#define FT8_MIN_BIN					32				// 200 Hz / 6.25 Hz
#define FT8_NUM_BINS				449				// (3000 Hz / 6.25 Hz + 1) - FT8_MIN_BIN
#define FT8_MAX_BLOCKS				93				// 15 s / 0.16 s
#define FT8_WF_BYTES				(FT8_MAX_BLOCKS * FT8_TIME_OSR * FT8_FREQ_OSR * FT8_NUM_BINS)

// Decoder limits (ft8_lib demo values)
#define FT8_MAX_CANDIDATES			140
#define FT8_MIN_SCORE				10
#define FT8_LDPC_ITERATIONS			25
#define FT8_MAX_DECODES				50

// Waterfall file (.wf) - what the M4 will eventually stream over ICC, stored
// to disk so the M7 decoder can be benched without the M4 in the loop.
// Little endian, 32 byte header followed by num_blocks * block_stride bytes
// laid out as ftx_waterfall_t.mag: [block][time_sub][freq_sub][bin]
#define FT8_WF_MAGIC				0x57385446		// "FT8W"
#define FT8_WF_VERSION				1

typedef struct
{
	uint32_t	magic;
	uint16_t	version;
	uint16_t	hdr_size;			// sizeof(FT8_WF_HEADER)
	uint16_t	num_blocks;
	uint16_t	num_bins;
	uint8_t		time_osr;
	uint8_t		freq_osr;
	uint16_t	min_bin;
	uint32_t	slot_hhmmss;		// slot start time, informational
	uint8_t		reserved[12];

} FT8_WF_HEADER;

// One decoded message. No floats - the target printf cannot print them
typedef struct
{
	int16_t		snr;				// dB, ft8_lib estimate (score / 2)
	int16_t		dt_10;				// time offset, tenths of a second
	uint16_t	freq_hz;			// audio frequency
	int16_t		score;				// sync score
	char		text[FTX_MAX_MESSAGE_LENGTH];

} FT8_DECODE_RESULT;

// Per slot statistics, times in microseconds
typedef struct
{
	uint32_t	us_sync;			// ftx_find_candidates
	uint32_t	us_ldpc;			// ftx_decode_candidate - LLR extraction, LDPC, CRC
	uint32_t	us_unpack;			// ftx_message_decode
	uint32_t	us_total;
	uint16_t	candidates;
	uint16_t	ldpc_fail;			// candidates that did not converge / failed CRC
	uint16_t	duplicates;
	uint16_t	decodes;

} FT8_DECODE_STATS;

// Free running microsecond clock, supplied by the platform (DWT on target)
typedef uint32_t (*ft8_clock_us_t)(void);

void	ft8_decoder_init(ft8_clock_us_t clock_us);

// Bind a waterfall to a magnitude buffer of FT8_WF_BYTES and set the geometry
void	ft8_decoder_wf_init(ftx_waterfall_t *wf, uint8_t *mag, int num_blocks);

// Decode one slot. Returns the number of results written (<= max_res)
int		ft8_decoder_run(const ftx_waterfall_t *wf, FT8_DECODE_RESULT *res, int max_res,
						FT8_DECODE_STATS *stats);

// Message text -> the 79 channel tones (0..7) of an FT8 frame, using the
// same callsign hash table as the decoder. Returns 0 on success, else the
// ftx_message_rc_t of the failed pack
#define FT8_TONES					79

int		ft8_decoder_encode(const char *text, uint8_t *tones);

#endif
