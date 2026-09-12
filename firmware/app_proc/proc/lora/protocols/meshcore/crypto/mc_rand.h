/************************************************************************************
**                                                                                 **
**                                 mcHF QRP Transceiver                            **
**                         Krassi Atanassov - M0NKA, 2013-2026                     **
**                                                                                 **
**---------------------------------------------------------------------------------**
**                                                                                 **
**  File name:		mc_rand.h                                                      **
**  Description:	Random bytes for MeshCore key generation                       **
**  Licence:		https://github.com/m0nka/mcHF/blob/main/LICENSE                **
************************************************************************************/
#ifndef __MC_RAND_H
#define __MC_RAND_H

#include <stddef.h>
#include <stdint.h>

// Fill buf with random bytes. Returns 0 when the hardware TRNG supplied
// the entropy, 1 when it could not be started and the (much weaker)
// timing/UID fallback was used - the caller should warn in that case,
// since an identity key is only as good as the seed behind it
uint8_t	mc_rand_bytes(uint8_t *buf, size_t len);

#endif
