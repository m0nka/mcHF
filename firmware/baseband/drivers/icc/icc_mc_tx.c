/************************************************************************************
**                                                                                 **
**                                 mcHF QRP Transceiver                            **
**                         Krassi Atanassov - M0NKA, 2013-2026                     **
**                                                                                 **
**---------------------------------------------------------------------------------**
**                                                                                 **
**  File name:		icc_mc_tx.c                                                    **
**  Description:	MarsChat/WSPR 4-FSK symbol tx streamer. Owns a private        **
**					softdds instance and generates constant envelope IQ           **
**					straight into the tx chain (like the tune generator, so       **
**					no ALC/compressor in the path), stepping the tone per          **
**					32768 sample symbol. An optional CW id segment (keyed          **
**					tone, smoothstep edges) follows the symbols. The M4 core       **
**					keys the exciter itself via the icc idle thread                **
**  Licence:		https://github.com/m0nka/mcHF/blob/main/LICENSE                **
************************************************************************************/

// Compiled only for the STM32H747 CM4 baseband build
#ifdef H7_M4_CORE

#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <math.h>

#include "uhsdr_board.h"
#include "drivers/audio/softdds/softdds.h"

#include "icc_mc_tx.h"

// Wire format constants, duplicated from common/mchf_icc_def.h to keep
// this translation unit free of the wire protocol header (it conflicts
// with the UHSDR board headers - same approach as icc_wspr.c)
#define MC_TX_HDR_SIZE			6
#define MC_TX_MAX_SYMS			162
#define MC_TX_SYMS_BYTES		((MC_TX_MAX_SYMS + 3) / 4)
#define MC_TX_MAX_CW_ELEM		255

// The SAI tx path runs at a fixed 48 kHz on this radio - the symbol
// length below is only exact at that rate
#define MC_TX_FS				48000UL

// One WSPR symbol = 8192/12000 s = exactly 32768 samples @ 48 kHz,
// tone spacing 12000/8192 = 1.46484375 Hz
#define MC_SYM_SAMPLES			32768UL
#define MC_TONE_STEP_HZ			(12000.0f / 8192.0f)

// Silence between the last symbol and the CW id (also absorbs the
// carrier ramp out) and the envelope edge length (5 ms smoothstep)
#define MC_GAP_SAMPLES			24000UL
#define MC_RAMP_SAMPLES			240UL

// FT8 - 8-FSK, 79 symbols of 0.16 s = 7680 samples @ 48 kHz, 6.25 Hz tone
// spacing, GFSK (BT 2.0) like WSJT-X and ft8_lib: the frequency follows the
// tone sequence through a Gaussian smoothed pulse three symbols long, so
// the signal stays narrow instead of splattering at every tone change. The
// frequency is updated every FT8_CHUNK samples (1 ms) from a pulse table
#define FT8_NN					79
#define FT8_SYM_SAMPLES			7680UL
#define FT8_TONE_STEP_HZ		6.25f
#define FT8_GFSK_BT				2.0f
#define FT8_CHUNK				48UL								// 1 ms
#define FT8_CHUNKS_SYM			(FT8_SYM_SAMPLES / FT8_CHUNK)		// 160
#define FT8_PULSE_LEN			(3 * FT8_CHUNKS_SYM)				// 480

// Transmission kinds sharing this streamer
#define MC_MODE_WSPR			0		// MarsChat/WSPR 4-FSK (+ CW id)
#define MC_MODE_FT8				1

// Streamer phases
enum
{
	MC_PH_IDLE = 0,
	MC_PH_SYMS,				// 4-FSK symbol stream
	MC_PH_GAP,				// silence before the CW id / after the symbols
	MC_PH_CW,				// keyed tone CW id elements
	MC_PH_TAIL				// envelope run out, then unkey
};

typedef struct
{
	// Flags crossing the ICC superloop / SAI interrupt contexts
	volatile uint8_t	active;			// streamer owns the tx iq buffers
	volatile uint8_t	start_req;		// key the exciter (set on start cmd)
	volatile uint8_t	done_flag;		// stream complete (set in the irq)
	uint8_t				unkey_sent;		// unkey request already handed out

	// Transmission description from the M7 core
	uint16_t			tone_base;
	uint8_t				nsym;
	uint8_t				syms[MC_TX_SYMS_BYTES];
	uint8_t				cw_nelem;
	uint16_t			cw_elem_samples;
	uint8_t				cw_bits[(MC_TX_MAX_CW_ELEM + 7) / 8];

	// Playback position
	uint8_t				phase;
	uint16_t			seg_idx;		// symbol / cw element index
	uint32_t			seg_pos;		// sample position inside the segment
	uint32_t			seg_len;

	// Keying envelope, slewed per sample and smoothstep shaped
	float32_t			env;
	float32_t			env_target;

	// Private tone generator - deliberately NOT the shared tune/CW
	// instance, so the tune machinery can never fight the streamer
	soft_dds_t			dds;

	// FT8
	uint8_t				mode;			// MC_MODE_xxx
	uint8_t				ft8_tones[FT8_NN];

} icc_mc_tx_state_t;

static icc_mc_tx_state_t	mc;

// GFSK frequency pulse, sampled at the chunk centres over three symbols
static float32_t			ft8_pulse[FT8_PULSE_LEN];
static uint8_t				ft8_pulse_ok = 0;

//*----------------------------------------------------------------------------
//* Function Name       : ft8_pulse_init
//* Object              : ft8_lib gfsk_pulse(): the rectangular symbol pulse
//*						: through a Gaussian of bandwidth BT, spanning -1.5 to
//*						: +1.5 symbols. The three overlapping pulses of a
//*						: steady tone add up to 1
//* Context    			: CONTEXT_ICC (superloop, first FT8 start)
//*----------------------------------------------------------------------------
static void ft8_pulse_init(void)
{
	const float32_t	c = (float32_t)M_PI * sqrtf(2.0f / logf(2.0f));
	uint32_t		i;
	float32_t		t;

	for(i = 0; i < FT8_PULSE_LEN; i++)
	{
		t = (((float32_t)i + 0.5f) / (float32_t)FT8_CHUNKS_SYM) - 1.5f;

		ft8_pulse[i] = (erff(c * FT8_GFSK_BT * (t + 0.5f)) - erff(c * FT8_GFSK_BT * (t - 0.5f))) / 2.0f;
	}

	ft8_pulse_ok = 1;
}

//*----------------------------------------------------------------------------
//* Function Name       : ft8_chunk_freq
//* Object              : GFSK frequency for chunk 'j' of symbol 'k' - the
//*						: symbol itself plus the tails of its neighbours
//*						: (the first and last symbol stand in for the ones
//*						: before and after the frame, as ft8_lib does)
//* Context    			: CONTEXT_IRQ
//*----------------------------------------------------------------------------
static float32_t ft8_chunk_freq(uint16_t k, uint32_t j)
{
	float32_t	prev = mc.ft8_tones[(k > 0) ? (k - 1) : 0];
	float32_t	cur  = mc.ft8_tones[k];
	float32_t	next = mc.ft8_tones[(k < (FT8_NN - 1)) ? (k + 1) : (FT8_NN - 1)];
	float32_t	tone;

	tone = prev * ft8_pulse[2 * FT8_CHUNKS_SYM + j] +
		   cur  * ft8_pulse[FT8_CHUNKS_SYM + j] +
		   next * ft8_pulse[j];

	return (float32_t)mc.tone_base + tone * FT8_TONE_STEP_HZ;
}

// ------------------------------------------------------------------
// Packed field access (layouts in common/mchf_icc_def.h)

static uint8_t mc_sym(uint16_t n)
{
	return (mc.syms[n >> 2] >> ((n & 3) * 2)) & 3;
}

static uint8_t mc_cw_bit(uint16_t n)
{
	return (mc.cw_bits[n >> 3] >> (n & 7)) & 1;
}

static void mc_set_tone(uint16_t sym, uint8_t smooth)
{
	softdds_setFreqDDS(&mc.dds,
			(float32_t)mc.tone_base + (float32_t)sym * MC_TONE_STEP_HZ,
			MC_TX_FS, smooth);
}

//*----------------------------------------------------------------------------
//* Function Name       : icc_mc_tx_start
//* Object              : parse the wire payload, arm the streamer and
//*						: request the exciter key. 0 = accepted
//* Context    			: CONTEXT_ICC (superloop)
//*----------------------------------------------------------------------------
uint8_t icc_mc_tx_start(const uint8_t *payload)
{
	uint16_t	tone, cw_samps;
	uint8_t		nsym, cw_nelem;

	if(payload == NULL)
		return 1;

	// A transmission is running - the M7 core must stop it first
	if(mc.active)
		return 2;

	tone     = (uint16_t)(payload[0] | (payload[1] << 8));
	nsym     = payload[2];
	cw_nelem = payload[3];
	cw_samps = (uint16_t)(payload[4] | (payload[5] << 8));

	// Keep the tone inside the SSB tx filter passband and the symbol
	// count inside the wire layout
	if((tone < 500) || (tone > 3000))
		return 3;

	if((nsym == 0) || (nsym > MC_TX_MAX_SYMS))
		return 4;

	// A CW element must at least fit its own keying edges
	if((cw_nelem != 0) && (cw_samps < (2 * MC_RAMP_SAMPLES)))
		return 5;

	mc.tone_base       = tone;
	mc.nsym            = nsym;
	mc.cw_nelem        = cw_nelem;
	mc.cw_elem_samples = cw_samps;

	memcpy(mc.syms, payload + MC_TX_HDR_SIZE, (size_t)((nsym + 3) / 4));

	if(cw_nelem != 0)
		memcpy(mc.cw_bits, payload + MC_TX_HDR_SIZE + MC_TX_SYMS_BYTES,
				(size_t)((cw_nelem + 7) / 8));

	// Playback state - the envelope ramps in from zero on the first
	// generated block, right after the exciter keys up
	mc.phase      = MC_PH_SYMS;
	mc.seg_idx    = 0;
	mc.seg_pos    = 0;
	mc.seg_len    = MC_SYM_SAMPLES;
	mc.env        = 0.0f;
	mc.env_target = 1.0f;
	mc.done_flag  = 0;
	mc.unkey_sent = 0;
	mc.mode       = MC_MODE_WSPR;

	mc_set_tone(mc_sym(0), 0);

	mc.active    = 1;
	mc.start_req = 1;

	printf("mc tx start: tone %u Hz, %u syms, %u cw elem\r\n",
			tone, nsym, cw_nelem);

	return 0;
}

//*----------------------------------------------------------------------------
//* Function Name       : icc_mc_tx_start_ft8
//* Object              : FT8 frame - [0..1] audio tone of tone 0 in Hz LE,
//*						: [2] symbol count (79), [3..] one tone (0..7) per
//*						: byte. Arms the streamer and requests the key.
//*						: 0 = accepted
//* Context    			: CONTEXT_ICC (superloop)
//*----------------------------------------------------------------------------
uint8_t icc_mc_tx_start_ft8(const uint8_t *payload)
{
	uint16_t	tone;
	uint8_t		i;

	if(payload == NULL)
		return 1;

	if(mc.active)
		return 2;

	tone = (uint16_t)(payload[0] | (payload[1] << 8));

	// The top tone sits 7 x 6.25 = 44 Hz above the base - keep all of it
	// inside the SSB tx passband
	if((tone < 200) || (tone > 2900))
		return 3;

	if(payload[2] != FT8_NN)
		return 4;

	for(i = 0; i < FT8_NN; i++)
	{
		if(payload[3 + i] > 7)
			return 5;

		mc.ft8_tones[i] = payload[3 + i];
	}

	if(!ft8_pulse_ok)
		ft8_pulse_init();

	mc.mode       = MC_MODE_FT8;
	mc.tone_base  = tone;
	mc.nsym       = FT8_NN;
	mc.cw_nelem   = 0;

	mc.phase      = MC_PH_SYMS;
	mc.seg_idx    = 0;
	mc.seg_pos    = 0;
	mc.seg_len    = FT8_SYM_SAMPLES;
	mc.env        = 0.0f;
	mc.env_target = 1.0f;
	mc.done_flag  = 0;
	mc.unkey_sent = 0;

	softdds_setFreqDDS(&mc.dds, ft8_chunk_freq(0, 0), MC_TX_FS, 0);

	mc.active    = 1;
	mc.start_req = 1;

	printf("ft8 tx start: tone %u Hz\r\n", tone);

	return 0;
}

//*----------------------------------------------------------------------------
//* Function Name       : icc_mc_tx_stop
//* Object              : abort the transmission (M7 request or error)
//* Context    			: CONTEXT_ICC (superloop)
//*----------------------------------------------------------------------------
void icc_mc_tx_stop(void)
{
	if((!mc.active) && (!mc.start_req))
		return;

	mc.active    = 0;
	mc.start_req = 0;
	mc.phase     = MC_PH_IDLE;
	mc.done_flag = 1;						// drives the unkey request

	printf("mc tx stop\r\n");
}

//*----------------------------------------------------------------------------
//* Function Name       : icc_mc_tx_active
//* Object              : true while the streamer owns the tx iq buffers
//* Context    			: any
//*----------------------------------------------------------------------------
uint8_t icc_mc_tx_active(void)
{
	return mc.active;
}

//*----------------------------------------------------------------------------
//* Function Name       : icc_mc_tx_key_request
//* Object              : pending exciter key/unkey request, one-shot.
//*						: The flags are one way (start_req cleared here,
//*						: done_flag latched by unkey_sent), so the SAI
//*						: interrupt and this superloop caller cannot race
//* Context    			: CONTEXT_ICC (superloop idle thread)
//*----------------------------------------------------------------------------
uint8_t icc_mc_tx_key_request(void)
{
	if(mc.start_req)
	{
		mc.start_req = 0;
		return 1;
	}

	if((mc.done_flag) && (!mc.unkey_sent))
	{
		mc.unkey_sent = 1;
		return 2;
	}

	return 0;
}

// ------------------------------------------------------------------
// Segment sequencing - called at segment end from the generator

static void mc_next_segment(void)
{
	switch(mc.phase)
	{
		case MC_PH_SYMS:
		{
			mc.seg_idx++;

			// FT8 - the generator retunes per chunk, nothing to do here
			// but the length; no CW id, straight to the run out
			if(mc.mode == MC_MODE_FT8)
			{
				if(mc.seg_idx < mc.nsym)
				{
					mc.seg_len = FT8_SYM_SAMPLES;
					break;
				}

				mc.phase      = MC_PH_TAIL;
				mc.seg_len    = 2 * MC_RAMP_SAMPLES;
				mc.env_target = 0.0f;
				break;
			}

			if(mc.seg_idx < mc.nsym)
			{
				// Next symbol - smooth transition keeps the phase
				mc_set_tone(mc_sym(mc.seg_idx), 1);
				mc.seg_len = MC_SYM_SAMPLES;
				break;
			}

			// Symbols done - the carrier ramps out into the gap, the
			// last symbol keeps full amplitude to its exact end
			mc.phase      = MC_PH_GAP;
			mc.seg_len    = MC_GAP_SAMPLES;
			mc.env_target = 0.0f;
			break;
		}

		case MC_PH_GAP:
		{
			if(mc.cw_nelem != 0)
			{
				mc.phase   = MC_PH_CW;
				mc.seg_idx = 0;
				mc.seg_len = mc.cw_elem_samples;

				mc_set_tone(0, 1);			// CW id keys the base tone
				mc.env_target = mc_cw_bit(0) ? 1.0f : 0.0f;
			}
			else
			{
				mc.phase      = MC_PH_TAIL;
				mc.seg_len    = 2 * MC_RAMP_SAMPLES;
				mc.env_target = 0.0f;
			}
			break;
		}

		case MC_PH_CW:
		{
			mc.seg_idx++;

			if(mc.seg_idx < mc.cw_nelem)
			{
				mc.seg_len    = mc.cw_elem_samples;
				mc.env_target = mc_cw_bit(mc.seg_idx) ? 1.0f : 0.0f;
			}
			else
			{
				mc.phase      = MC_PH_TAIL;
				mc.seg_len    = 2 * MC_RAMP_SAMPLES;
				mc.env_target = 0.0f;
			}
			break;
		}

		case MC_PH_TAIL:
		default:
		{
			// Stream complete - release the tx path and ask the idle
			// thread to unkey the exciter
			mc.phase     = MC_PH_IDLE;
			mc.active    = 0;
			mc.done_flag = 1;

			printf("mc tx done\r\n");
			break;
		}
	}

	mc.seg_pos = 0;
}

//*----------------------------------------------------------------------------
//* Function Name       : icc_mc_tx_gen
//* Object              : generate one tx iq block. The tone runs through
//*						: every phase for continuity, the keying envelope
//*						: (per sample slew + smoothstep shaping) does the
//*						: gating. Symbol boundaries are block aligned
//*						: (32768 is a multiple of every block size used),
//*						: gap/CW boundaries are handled mid block
//* Context    			: CONTEXT_IRQ (SAI tx path via TxProcessor_Run)
//*----------------------------------------------------------------------------
uint8_t icc_mc_tx_gen(float *i_buff, float *q_buff, uint16_t block_size)
{
	uint16_t	n = 0;
	float32_t	slew = 1.0f / (float32_t)MC_RAMP_SAMPLES;

	if(!mc.active)
		return 0;

	while(n < block_size)
	{
		uint32_t	run = mc.seg_len - mc.seg_pos;
		uint16_t	k;

		// FT8 symbols - GFSK, retune at every chunk boundary and never
		// run across one
		if((mc.mode == MC_MODE_FT8) && (mc.phase == MC_PH_SYMS))
		{
			uint32_t in_chunk = mc.seg_pos % FT8_CHUNK;

			if(in_chunk == 0)
				softdds_setFreqDDS(&mc.dds, ft8_chunk_freq(mc.seg_idx, mc.seg_pos / FT8_CHUNK), MC_TX_FS, 1);

			if(run > (FT8_CHUNK - in_chunk))
				run = FT8_CHUNK - in_chunk;
		}

		if(run > (uint32_t)(block_size - n))
			run = (uint32_t)(block_size - n);

		// The dds keeps running through silence too - constant phase,
		// and the envelope alone decides what leaves the radio
		softdds_genIQSingleTone(&mc.dds, i_buff + n, q_buff + n, (uint16_t)run);

		for(k = 0; k < run; k++)
		{
			float32_t shape;

			if(mc.env < mc.env_target)
			{
				mc.env += slew;
				if(mc.env > mc.env_target)
					mc.env = mc.env_target;
			}
			else if(mc.env > mc.env_target)
			{
				mc.env -= slew;
				if(mc.env < mc.env_target)
					mc.env = mc.env_target;
			}

			// Smoothstep of the linear slew = raised cosine like edge
			shape = mc.env * mc.env * (3.0f - 2.0f * mc.env);

			i_buff[n + k] *= shape;
			q_buff[n + k] *= shape;
		}

		n          = (uint16_t)(n + run);
		mc.seg_pos += run;

		if(mc.seg_pos >= mc.seg_len)
		{
			mc_next_segment();

			if(!mc.active)
				break;
		}
	}

	// Stream ended mid block - pad with silence
	for(; n < block_size; n++)
	{
		i_buff[n] = 0.0f;
		q_buff[n] = 0.0f;
	}

	return 1;
}

#endif // H7_M4_CORE
