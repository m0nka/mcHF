/************************************************************************************
**                                                                                 **
**                                 mcHF QRP Transceiver                            **
**                         Krassi Atanassov - M0NKA, 2013-2026                     **
**                                                                                 **
**---------------------------------------------------------------------------------**
**                                                                                 **
**  File name:		wspr_encoder.h                                                 **
**  Description:	WSPR encoder core - public API                                 **
**  Last Modified:                                                                 **
**  Licence:		https://github.com/m0nka/mcHF/blob/main/LICENSE                **
************************************************************************************/
//
// Pure C WSPR channel encoder, mirror of the decode chain in wspr_decoder.c
//
// Raw 50 bit payload -> +31 zero tail bits -> K=32 r=1/2 convolutional
// encode -> bit reversal interleave -> +sync vector -> 162 4-ary symbols
//
// Personality-free by design: the payload packers (WSPR type 1 here,
// MarsChat frame layer elsewhere) are peer producers of the raw bits.
// No OS or HAL dependencies - builds on a PC with -DWSPR_HOST_BUILD
//
#ifndef __WSPR_ENCODER_H
#define __WSPR_ENCODER_H

#include <stdint.h>

// Raw 50 bit payload (packed 7 bytes, MSB first, low 6 bits of bits50[6]
// ignored) -> 162 channel symbols, values 0..3
void	wspr_encode_raw	(const uint8_t bits50[7], uint8_t sym162[162]);

// Pack a standard WSPR type 1 message into the raw 50 bit payload
// call: 3..6 chars, one digit in the first three positions (e.g. "M0NKA")
// grid: 4 char locator, AA00..RR99 (e.g. "IO92")
// dbm:  reported tx power, 0..60
// Returns 0 ok, 1 invalid input
int		wspr_pack_type1	(const char *call, const char *grid, int dbm, uint8_t bits50[7]);

#endif
