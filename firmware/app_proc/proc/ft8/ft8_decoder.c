/************************************************************************************
**                                                                                 **
**                                 mcHF QRP Transceiver                            **
**                         Krassi Atanassov - M0NKA, 2013-2026                     **
**                                                                                 **
**---------------------------------------------------------------------------------**
**                                                                                 **
**  File name:		ft8_decoder.c                                                  **
**  Description:	FT8 slot decoder (ft8_lib host), portable - also builds on PC  **
**  Last Modified:                                                                 **
**  Licence:		https://github.com/m0nka/mcHF/blob/main/LICENSE                **
************************************************************************************/
//
// The decode loop of ft8_lib's demo/decode_ft8.c without the PC plumbing:
// no malloc, no stdio, no float printing, and each stage timed on its own
// so the M7 decode budget (~1.5 s per slot) can be measured stage by stage.
//
// Builds unchanged on the host (claude/FT8/host) against the same waterfall
// files, so a decode missing on the target is a port problem and not an
// ft8_lib limitation.
//
#include <string.h>

#include "ft8/decode.h"
#include "ft8/encode.h"
#include "ft8/message.h"

#include "ft8_decoder.h"

// Callsign hash table - resolves the 10/12/22 bit hashed callsigns in
// messages to calls seen in full earlier. Persists across slots, ageing
// entries out after FT8_HASH_MAX_AGE slots
#define FT8_HASH_SIZE			256
#define FT8_HASH_MAX_AGE		10

static struct
{
	char		callsign[12];		// up to 11 chars + terminator
	uint32_t	hash;				// 8 MSBs age, 22 LSBs hash

} ft8_hash_tbl[FT8_HASH_SIZE];

static int				ft8_hash_count;
static ft8_clock_us_t	ft8_clock;

// Candidate list and duplicate check tables - static, too big for a stack
static ftx_candidate_t	ft8_cand[FT8_MAX_CANDIDATES];
static ftx_message_t	ft8_decoded[FT8_MAX_DECODES];
static ftx_message_t	*ft8_decoded_tbl[FT8_MAX_DECODES];

static void ft8_hash_add(const char *callsign, uint32_t hash)
{
	uint16_t	hash10 = (hash >> 12) & 0x3FFu;
	int			idx = (hash10 * 23) % FT8_HASH_SIZE;

	while(ft8_hash_tbl[idx].callsign[0] != '\0')
	{
		if(((ft8_hash_tbl[idx].hash & 0x3FFFFFu) == hash) && (strcmp(ft8_hash_tbl[idx].callsign, callsign) == 0))
		{
			// Seen again, reset the age
			ft8_hash_tbl[idx].hash &= 0x3FFFFFu;
			return;
		}

		idx = (idx + 1) % FT8_HASH_SIZE;
	}

	// Never fill the last slot, lookups rely on finding an empty one
	if(ft8_hash_count >= (FT8_HASH_SIZE - 1))
		return;

	ft8_hash_count++;
	strncpy(ft8_hash_tbl[idx].callsign, callsign, 11);
	ft8_hash_tbl[idx].callsign[11] = '\0';
	ft8_hash_tbl[idx].hash = hash;
}

static bool ft8_hash_lookup(ftx_callsign_hash_type_t hash_type, uint32_t hash, char *callsign)
{
	uint8_t		shift = (hash_type == FTX_CALLSIGN_HASH_10_BITS) ? 12 : ((hash_type == FTX_CALLSIGN_HASH_12_BITS) ? 10 : 0);
	uint16_t	hash10 = (hash >> (12 - shift)) & 0x3FFu;
	int			idx = (hash10 * 23) % FT8_HASH_SIZE;

	while(ft8_hash_tbl[idx].callsign[0] != '\0')
	{
		if(((ft8_hash_tbl[idx].hash & 0x3FFFFFu) >> shift) == hash)
		{
			strcpy(callsign, ft8_hash_tbl[idx].callsign);
			return true;
		}

		idx = (idx + 1) % FT8_HASH_SIZE;
	}

	callsign[0] = '\0';
	return false;
}

static void ft8_hash_age(void)
{
	int		i;
	uint8_t	age;

	for(i = 0; i < FT8_HASH_SIZE; i++)
	{
		if(ft8_hash_tbl[i].callsign[0] == '\0')
			continue;

		age = (uint8_t)(ft8_hash_tbl[i].hash >> 24);
		if(age > FT8_HASH_MAX_AGE)
		{
			ft8_hash_tbl[i].callsign[0] = '\0';
			ft8_hash_tbl[i].hash = 0;
			ft8_hash_count--;
		}
		else
			ft8_hash_tbl[i].hash = (((uint32_t)age + 1u) << 24) | (ft8_hash_tbl[i].hash & 0x3FFFFFu);
	}
}

static ftx_callsign_hash_interface_t ft8_hash_if =
{
	.lookup_hash	= ft8_hash_lookup,
	.save_hash		= ft8_hash_add
};

int ft8_decoder_encode(const char *text, uint8_t *tones)
{
	ftx_message_t		msg;
	ftx_message_rc_t	rc;

	rc = ftx_message_encode(&msg, &ft8_hash_if, text);
	if(rc != FTX_MESSAGE_RC_OK)
		return (int)rc;

	ft8_encode(msg.payload, tones);
	return 0;
}

void ft8_decoder_init(ft8_clock_us_t clock_us)
{
	ft8_clock = clock_us;
	ft8_hash_count = 0;
	memset(ft8_hash_tbl, 0, sizeof(ft8_hash_tbl));
}

void ft8_decoder_wf_init(ftx_waterfall_t *wf, uint8_t *mag, int num_blocks)
{
	wf->max_blocks		= FT8_MAX_BLOCKS;
	wf->num_blocks		= (num_blocks > FT8_MAX_BLOCKS) ? FT8_MAX_BLOCKS : num_blocks;
	wf->num_bins		= FT8_NUM_BINS;
	wf->time_osr		= FT8_TIME_OSR;
	wf->freq_osr		= FT8_FREQ_OSR;
	wf->block_stride	= FT8_TIME_OSR * FT8_FREQ_OSR * FT8_NUM_BINS;
	wf->mag				= mag;
	wf->protocol		= FTX_PROTOCOL_FT8;
}

int ft8_decoder_run(const ftx_waterfall_t *wf, FT8_DECODE_RESULT *res, int max_res,
					FT8_DECODE_STATS *stats)
{
	int						i, idx, n_res = 0;
	uint32_t				t0, t1, t_start;
	const ftx_candidate_t	*cand;
	ftx_message_t			msg;
	ftx_decode_status_t		status;
	ftx_message_offsets_t	offsets;
	ftx_message_rc_t		rc;
	FT8_DECODE_RESULT		*r;

	memset(stats, 0, sizeof(FT8_DECODE_STATS));

	t_start = ft8_clock();

	// Sync search - top candidates by Costas score
	stats->candidates = (uint16_t)ftx_find_candidates(wf, FT8_MAX_CANDIDATES, ft8_cand, FT8_MIN_SCORE);

	t1 = ft8_clock();
	stats->us_sync = t1 - t_start;

	for(i = 0; i < FT8_MAX_DECODES; i++)
		ft8_decoded_tbl[i] = NULL;

	for(i = 0; i < stats->candidates; i++)
	{
		cand = &ft8_cand[i];

		// LLR extraction, LDPC, CRC
		t0 = ft8_clock();
		if(!ftx_decode_candidate(wf, cand, FT8_LDPC_ITERATIONS, &msg, &status))
		{
			stats->us_ldpc += ft8_clock() - t0;
			stats->ldpc_fail++;
			continue;
		}
		stats->us_ldpc += ft8_clock() - t0;

		// Same payload found via a neighbouring candidate?
		idx = msg.hash % FT8_MAX_DECODES;
		while(ft8_decoded_tbl[idx] != NULL)
		{
			if((ft8_decoded_tbl[idx]->hash == msg.hash) && (memcmp(ft8_decoded_tbl[idx]->payload, msg.payload, sizeof(msg.payload)) == 0))
				break;

			idx = (idx + 1) % FT8_MAX_DECODES;
		}

		if(ft8_decoded_tbl[idx] != NULL)
		{
			stats->duplicates++;
			continue;
		}

		memcpy(&ft8_decoded[idx], &msg, sizeof(msg));
		ft8_decoded_tbl[idx] = &ft8_decoded[idx];
		stats->decodes++;

		// Out of result slots - keep counting, stop unpacking
		if(n_res >= max_res)
			continue;

		r = &res[n_res++];

		t0 = ft8_clock();
		rc = ftx_message_decode(&msg, &ft8_hash_if, r->text, &offsets);
		stats->us_unpack += ft8_clock() - t0;

		if(rc != FTX_MESSAGE_RC_OK)
		{
			strcpy(r->text, "?unpack ");
			r->text[7] = (char)('0' + ((int)rc % 10));
		}

		r->score	= cand->score;
		r->snr		= (int16_t)(cand->score / 2);
		r->freq_hz	= (uint16_t)((FT8_MIN_BIN + cand->freq_offset + (float)cand->freq_sub / wf->freq_osr) / FT8_SYMBOL_PERIOD + 0.5f);
		r->dt_10	= (int16_t)((cand->time_offset + (float)cand->time_sub / wf->time_osr) * FT8_SYMBOL_PERIOD * 10.0f);
	}

	ft8_hash_age();

	stats->us_total = ft8_clock() - t_start;

	return n_res;
}
