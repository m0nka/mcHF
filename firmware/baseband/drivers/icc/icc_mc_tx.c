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

} icc_mc_tx_state_t;

static icc_mc_tx_state_t	mc;

// Bench debug - trace of the first tone changes (sym value and the dds
// step that resulted) plus generator call stats, dumped after unkey
#define MC_DBG_TRACE			12
static volatile uint16_t	dbg_sym[MC_DBG_TRACE];
static volatile uint32_t	dbg_step[MC_DBG_TRACE];
static volatile uint8_t		dbg_n = 0;
static volatile uint32_t	dbg_gen_calls = 0;
static volatile uint32_t	dbg_gen_samples = 0;

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

	// Bench debug - record what the dds was actually told
	if(dbg_n < MC_DBG_TRACE)
	{
		dbg_sym [dbg_n] = sym;
		dbg_step[dbg_n] = mc.dds.step;
		dbg_n++;
	}
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

	mc_set_tone(mc_sym(0), 0);

	mc.active    = 1;
	mc.start_req = 1;

	printf("mc tx start: tone %u Hz, %u syms, %u cw elem\r\n",
			tone, nsym, cw_nelem);

	// Bench debug - first received symbol bytes, diff against the CM7
	// "mc: payload" line and the host dump_payload ground truth
	printf("mc tx syms: %02x %02x %02x %02x %02x %02x %02x %02x\r\n",
			mc.syms[0], mc.syms[1], mc.syms[2], mc.syms[3],
			mc.syms[4], mc.syms[5], mc.syms[6], mc.syms[7]);

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
		uint8_t	i;

		mc.unkey_sent = 1;

		// Bench debug - dump the tone trace, generator stats and the
		// tx_processor signal snapshots
		{
			extern void tx_processor_mc_dbg_dump(void);
			tx_processor_mc_dbg_dump();
		}

		printf("mc dbg: gen calls %u samples %u\r\n",
				dbg_gen_calls, dbg_gen_samples);
		for(i = 0; i < dbg_n; i++)
			printf("mc dbg: seg %u sym %u step %u\r\n",
					i, dbg_sym[i], dbg_step[i]);

		dbg_n           = 0;
		dbg_gen_calls   = 0;
		dbg_gen_samples = 0;

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

	// Bench debug - prove this generator actually feeds the tx path
	dbg_gen_calls++;
	dbg_gen_samples += block_size;

	while(n < block_size)
	{
		uint32_t	run = mc.seg_len - mc.seg_pos;
		uint16_t	k;

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
