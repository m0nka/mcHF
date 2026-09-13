/************************************************************************************
**                                                                                 **
**                                 mcHF QRP Transceiver                            **
**                         Krassi Atanassov - M0NKA, 2013-2026                     **
**                                                                                 **
**---------------------------------------------------------------------------------**
**                                                                                 **
**  File name:		sha512.h                                                       **
**  Description:	SHA-512, needed by the Ed25519 signature scheme                **
**  Licence:		https://github.com/m0nka/mcHF/blob/main/LICENSE                **
************************************************************************************/
#ifndef __MC_SHA512_H
#define __MC_SHA512_H

#include <stddef.h>
#include <stdint.h>

#define SHA512_DIGEST_SIZE		64
#define SHA512_BLOCK_SIZE		128

typedef struct
{
	uint64_t	state[8];
	uint64_t	count;							// message length in bytes
	uint8_t		buf[SHA512_BLOCK_SIZE];
	size_t		buf_len;

} sha512_ctx;

void	sha512_init  (sha512_ctx *ctx);
void	sha512_update(sha512_ctx *ctx, const uint8_t *data, size_t len);
void	sha512_final (sha512_ctx *ctx, uint8_t out[SHA512_DIGEST_SIZE]);

// One shot
void	sha512(const uint8_t *data, size_t len, uint8_t out[SHA512_DIGEST_SIZE]);

#endif
