/************************************************************************************
**                                                                                 **
**                                 mcHF QRP Transceiver                            **
**                         Krassi Atanassov - M0NKA, 2013-2026                     **
**                                                                                 **
**---------------------------------------------------------------------------------**
**                                                                                 **
**  File name:		wspr_fano.h                                                    **
**  Description:	Fano sequential decoder for the WSPR convolutional code        **
**  Last Modified:                                                                 **
**  Licence:		https://github.com/m0nka/mcHF/blob/main/LICENSE                **
************************************************************************************/
#ifndef __WSPR_FANO_H
#define __WSPR_FANO_H

#include <stdint.h>

// Layland-Lushbaugh rate 1/2, K=32 convolutional code (as used by WSPR/JT65)
#define WSPR_POLY1				0xf2d05351u
#define WSPR_POLY2				0xe4613c47u

// Static node array dimensioning (WSPR needs 81)
#define WSPR_FANO_MAX_NBITS		81

// Sequential decode of nbits info bits from 2*nbits soft channel symbols
//
// symbols:		soft coded bits, 0..255, one per channel symbol (deinterleaved,
//				two consecutive symbols per info bit, POLY1 output first)
// data:		output buffer, (nbits+7)/8 bytes, info bits packed MSB first
// mettab:		metric table [expected bit][received soft symbol]
// delta:		Fano threshold step
// maxcycles:	iteration limit per info bit
//
// Returns 0 on success, negative on timeout/failure
int wspr_fano(const uint8_t *symbols, uint8_t *data, int nbits,
              int mettab[2][256], int delta, uint32_t maxcycles);

#endif
