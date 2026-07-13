/************************************************************************************
**                                                                                 **
**                                 mcHF QRP Transceiver                            **
**                         Krassi Atanassov - M0NKA, 2013-2026                     **
**                                                                                 **
**---------------------------------------------------------------------------------**
**                                                                                 **
**  File name:		wspr_encoder.c                                                 **
**  Description:	WSPR encoder core                                              **
**  Last Modified:                                                                 **
**  Licence:		https://github.com/m0nka/mcHF/blob/main/LICENSE                **
************************************************************************************/
//
// Mirror of the decode chain in wspr_decoder.c - constants (polynomials,
// interleaver, sync vector, type 1 packing) follow K9AN/KA9Q wsprd (GPLv3)
//
// Validated on host against the unmodified decoder (claude/wspr_test):
// pack -> encode -> synthesize -> decode must round-trip exactly
//
#include <string.h>

#include "wspr_decoder.h"
#include "wspr_fano.h"
#include "wspr_encoder.h"

//*----------------------------------------------------------------------------
//* Function Name       : parity32
//* Object              : even parity of a 32 bit word
//* Context    			: any (pure)
//*----------------------------------------------------------------------------
static uint32_t parity32(uint32_t v)
{
	v ^= v >> 16;
	v ^= v >> 8;
	v ^= v >> 4;
	v ^= v >> 2;
	v ^= v >> 1;

	return v & 1;
}

//*----------------------------------------------------------------------------
//* Function Name       : char_idx
//* Object              : char -> WSPR alphabet index (0-9, A-Z, space)
//* Notes    			: returns -1 on invalid char
//* Context    			: any (pure)
//*----------------------------------------------------------------------------
static int char_idx(char c)
{
	if((c >= '0') && (c <= '9'))
		return c - '0';

	if((c >= 'A') && (c <= 'Z'))
		return c - 'A' + 10;

	if(c == ' ')
		return 36;

	return -1;
}

//*----------------------------------------------------------------------------
//* Function Name       : wspr_encode_raw
//* Object              : raw 50 bit payload -> 162 channel symbols (0..3)
//* Notes    			: tail bits, convolutional code, interleaver and sync
//* Notes    			: vector exactly mirror the decoder
//* Context    			: CONTEXT_WSPR / MarsChat / host build
//*----------------------------------------------------------------------------
void wspr_encode_raw(const uint8_t bits50[7], uint8_t sym162[162])
{
	uint8_t		conv[WSPR_NSYM];
	uint32_t	state;
	int			i, k;

	// K=32 r=1/2 non recursive convolutional encode over 50 payload bits
	// plus 31 zero tail bits (the tail flushes the shift register)
	state	= 0;
	k		= 0;

	for(i = 0; i < WSPR_NBITS; i++)
	{
		uint32_t bit = 0;

		if(i < 50)
			bit = (bits50[i >> 3] >> (7 - (i & 7))) & 1;

		state = (state << 1) | bit;

		conv[k++] = (uint8_t)parity32(state & WSPR_POLY1);
		conv[k++] = (uint8_t)parity32(state & WSPR_POLY2);
	}

	// Interleave by 8 bit reversal of the destination index,
	// exact inverse of deinterleave() in wspr_decoder.c
	k = 0;

	for(i = 0; i < 256; i++)
	{
		uint32_t	u = (uint32_t)i;
		int			j = (int)(((((u * 0x0802u) & 0x22110u) | ((u * 0x8020u) & 0x88440u)) * 0x10101u >> 16) & 0xffu);

		if(j < WSPR_NSYM)
			sym162[j] = conv[k++];
	}

	// Channel symbol = sync bit + 2 * data bit
	for(i = 0; i < WSPR_NSYM; i++)
		sym162[i] = wspr_pr3[i] + 2 * sym162[i];
}

//*----------------------------------------------------------------------------
//* Function Name       : wspr_pack_type1
//* Object              : "CALL GRID dBm" -> raw 50 bit payload
//* Notes    			: inverse of unpack_message() in wspr_decoder.c,
//* Notes    			: returns 0 ok, 1 invalid input
//* Context    			: CONTEXT_WSPR / host build
//*----------------------------------------------------------------------------
int wspr_pack_type1(const char *call, const char *grid, int dbm, uint8_t bits50[7])
{
	char		c6[6];
	int			len, i, digit_pos;
	uint32_t	n1, ng, n2;

	if((call == NULL) || (grid == NULL))
		return 1;

	len = (int)strlen(call);
	if((len < 3) || (len > 6))
		return 1;

	// Normalize callsign to 6 chars with the digit in third position
	digit_pos = -1;
	for(i = 1; (i < len) && (i < 3); i++)
	{
		if((call[i] >= '0') && (call[i] <= '9'))
			digit_pos = i;
	}
	if(digit_pos < 0)
		return 1;

	if((len + (2 - digit_pos)) > 6)
		return 1;

	memset(c6, ' ', sizeof(c6));
	memcpy(&c6[2 - digit_pos], call, len);

	// Callsign field, 28 bits
	// (positions 0-1 alphanumeric/space, 2 digit, 3-5 letters/space)
	{
		int v0 = char_idx(c6[0]);
		int v1 = char_idx(c6[1]);
		int v2 = char_idx(c6[2]);
		int v3 = char_idx(c6[3]);
		int v4 = char_idx(c6[4]);
		int v5 = char_idx(c6[5]);

		if((v0 < 0) || (v1 < 0) || (v1 == 36) || (v2 < 0) || (v2 > 9))
			return 1;
		if((v3 < 10) || (v4 < 10) || (v5 < 10))
			return 1;

		n1 = (uint32_t)v0;
		n1 = n1 * 36 + (uint32_t)v1;
		n1 = n1 * 10 + (uint32_t)v2;
		n1 = n1 * 27 + (uint32_t)(v3 - 10);
		n1 = n1 * 27 + (uint32_t)(v4 - 10);
		n1 = n1 * 27 + (uint32_t)(v5 - 10);
	}

	// Grid locator, 15 bits
	if((grid[0] < 'A') || (grid[0] > 'R') || (grid[1] < 'A') || (grid[1] > 'R'))
		return 1;
	if((grid[2] < '0') || (grid[2] > '9') || (grid[3] < '0') || (grid[3] > '9'))
		return 1;

	ng = (uint32_t)((179 - 10 * (grid[0] - 'A') - (grid[2] - '0')) * 180
	     + 10 * (grid[1] - 'A') + (grid[3] - '0'));

	// Power field, 7 bits
	if((dbm < 0) || (dbm > 60))
		return 1;

	n2 = ng * 128 + (uint32_t)(dbm + 64);

	// Pack n1 (28 bits) + n2 (22 bits) MSB first into 7 bytes,
	// low 6 bits of the last byte zero (canonical raw form)
	bits50[0] = (uint8_t)(n1 >> 20);
	bits50[1] = (uint8_t)(n1 >> 12);
	bits50[2] = (uint8_t)(n1 >> 4);
	bits50[3] = (uint8_t)(((n1 & 0x0F) << 4) | ((n2 >> 18) & 0x0F));
	bits50[4] = (uint8_t)(n2 >> 10);
	bits50[5] = (uint8_t)(n2 >> 2);
	bits50[6] = (uint8_t)((n2 & 0x03) << 6);

	return 0;
}
