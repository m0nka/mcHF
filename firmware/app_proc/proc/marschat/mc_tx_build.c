/************************************************************************************
**                                                                                 **
**                                 mcHF QRP Transceiver                            **
**                         Krassi Atanassov - M0NKA, 2013-2026                     **
**                                                                                 **
**---------------------------------------------------------------------------------**
**                                                                                 **
**  File name:		mc_tx_build.c                                                  **
**  Description:	ICC_MC_TX_START payload builder (see mc_tx_build.h)            **
**  Licence:		https://github.com/m0nka/mcHF/blob/main/LICENSE                **
************************************************************************************/

#include <stdint.h>
#include <string.h>

#include "wspr_encoder.h"
#include "mc_tx_build.h"

// Wire format constants, duplicated from common/mchf_icc_def.h (this
// translation unit stays free of the wire protocol header so the host
// test rig can compile it - same approach as the M4 side icc_mc_tx.c)
#define MC_TX_HDR_SIZE			6
#define MC_TX_MAX_SYMS			162
#define MC_TX_SYMS_BYTES		((MC_TX_MAX_SYMS + 3) / 4)
#define MC_TX_MAX_CW_ELEM		255

// Morse code table, A-Z 0-9 and '/' (portable/mobile callsigns)
typedef struct
{
	char		c;
	const char	*sym;						// "." and "-" string
} mc_morse_t;

static const mc_morse_t morse_tab[] =
{
	{ 'A', ".-"    }, { 'B', "-..."  }, { 'C', "-.-."  }, { 'D', "-.."   },
	{ 'E', "."     }, { 'F', "..-."  }, { 'G', "--."   }, { 'H', "...."  },
	{ 'I', ".."    }, { 'J', ".---"  }, { 'K', "-.-"   }, { 'L', ".-.."  },
	{ 'M', "--"    }, { 'N', "-."    }, { 'O', "---"   }, { 'P', ".--."  },
	{ 'Q', "--.-"  }, { 'R', ".-."   }, { 'S', "..."   }, { 'T', "-"     },
	{ 'U', "..-"   }, { 'V', "...-"  }, { 'W', ".--"   }, { 'X', "-..-"  },
	{ 'Y', "-.--"  }, { 'Z', "--.."  },
	{ '0', "-----" }, { '1', ".----" }, { '2', "..---" }, { '3', "...--" },
	{ '4', "....-" }, { '5', "....." }, { '6', "-...." }, { '7', "--..." },
	{ '8', "---.." }, { '9', "----." },
	{ '/', "-..-." },
};

static const char *mc_morse_lookup(char c)
{
	uint32_t i;

	// Fold lower case letters
	if((c >= 'a') && (c <= 'z'))
		c = (char)(c - 'a' + 'A');

	for(i = 0; i < (sizeof(morse_tab) / sizeof(morse_tab[0])); i++)
	{
		if(morse_tab[i].c == c)
			return morse_tab[i].sym;
	}

	return 0;
}

//*----------------------------------------------------------------------------
//* Function Name       : mc_tx_cw_elements
//* Object              : morse element bitstream, see header
//* Context    			: any (pure function)
//*----------------------------------------------------------------------------
int mc_tx_cw_elements(const char *text, uint8_t *bits, int max_elem)
{
	int	n = 0;
	int	first_char = 1;

	if((text == 0) || (bits == 0))
		return -1;

	memset(bits, 0, (size_t)((max_elem + 7) / 8));

	for(; *text != 0; text++)
	{
		const char	*sym = mc_morse_lookup(*text);
		int			first_sym = 1;

		if(sym == 0)
			return -1;

		// Three off units between letters (one is implied by the
		// element grid, add all three explicitly by skipping)
		if(!first_char)
			n += 3;
		first_char = 0;

		for(; *sym != 0; sym++)
		{
			int	on = (*sym == '-') ? 3 : 1;

			// One off unit between the symbols of a letter
			if(!first_sym)
				n += 1;
			first_sym = 0;

			if((n + on) > max_elem)
				return -1;

			for(; on > 0; on--)
			{
				bits[n >> 3] |= (uint8_t)(1u << (n & 7));
				n++;
			}
		}
	}

	return n;
}

//*----------------------------------------------------------------------------
//* Function Name       : mc_tx_build_payload
//* Object              : raw 50 bit frame -> ICC_MC_TX_START wire payload
//* Context    			: any (pure function)
//*----------------------------------------------------------------------------
int mc_tx_build_payload(const uint8_t bits50[7], uint16_t tone_hz,
						const char *cw_call, uint8_t cw_wpm,
						uint8_t *out, uint16_t *out_len)
{
	uint8_t		syms[MC_TX_MAX_SYMS];
	uint8_t		cw_bits[(MC_TX_MAX_CW_ELEM + 7) / 8];
	int			nelem = 0;
	uint16_t	unit_samples = 0;
	int			i;

	if((bits50 == 0) || (out == 0) || (out_len == 0))
		return 1;

	if((tone_hz < 500) || (tone_hz > 3000))
		return 1;

	// Optional CW id segment
	if((cw_call != 0) && (cw_call[0] != 0))
	{
		if(cw_wpm == 0)
			return 1;

		nelem = mc_tx_cw_elements(cw_call, cw_bits, MC_TX_MAX_CW_ELEM);
		if(nelem < 0)
			return 2;

		// One element unit is 1.2/wpm seconds at the 48 kHz tx rate
		unit_samples = (uint16_t)(57600UL / cw_wpm);
	}

	// 50 bits -> 162 channel symbols
	wspr_encode_raw(bits50, syms);

	// Header
	out[0] = (uint8_t)(tone_hz >> 0);
	out[1] = (uint8_t)(tone_hz >> 8);
	out[2] = MC_TX_MAX_SYMS;
	out[3] = (uint8_t)nelem;
	out[4] = (uint8_t)(unit_samples >> 0);
	out[5] = (uint8_t)(unit_samples >> 8);

	// Symbol stream, 2 bits each, LSBs first
	memset(out + MC_TX_HDR_SIZE, 0, MC_TX_SYMS_BYTES);
	for(i = 0; i < MC_TX_MAX_SYMS; i++)
		out[MC_TX_HDR_SIZE + (i >> 2)] |= (uint8_t)((syms[i] & 3) << ((i & 3) * 2));

	*out_len = (uint16_t)(MC_TX_HDR_SIZE + MC_TX_SYMS_BYTES);

	if(nelem != 0)
	{
		memcpy(out + MC_TX_HDR_SIZE + MC_TX_SYMS_BYTES, cw_bits,
				(size_t)((nelem + 7) / 8));
		*out_len = (uint16_t)(*out_len + (uint16_t)((nelem + 7) / 8));
	}

	return 0;
}

// Wire-format timing constants, matching baseband/drivers/icc/icc_mc_tx.c
#define MC_TX_FS			48000UL
#define MC_TX_SYM_SAMPLES	32768UL				// one WSPR symbol
#define MC_TX_GAP_SAMPLES	24000UL				// silence before CW id / tail
#define MC_TX_RAMP_SAMPLES	240UL				// envelope in + out

//*----------------------------------------------------------------------------
//* Function Name       : mc_tx_build_duration_ms
//* Object              : see header
//* Context    			: any (pure function)
//*----------------------------------------------------------------------------
uint32_t mc_tx_build_duration_ms(const uint8_t *payload)
{
	uint8_t		nsym, nelem;
	uint16_t	unit_samples;
	uint64_t	samples;

	if(payload == 0)
		return 0;

	nsym         = payload[2];
	nelem        = payload[3];
	unit_samples = (uint16_t)(payload[4] | (payload[5] << 8));

	samples = (uint64_t)nsym * MC_TX_SYM_SAMPLES + MC_TX_GAP_SAMPLES + 2 * MC_TX_RAMP_SAMPLES;

	if(nelem != 0)
		samples += (uint64_t)nelem * unit_samples;

	return (uint32_t)((samples * 1000) / MC_TX_FS);
}
