/************************************************************************************
**                                                                                 **
**                                 mcHF QRP Transceiver                            **
**                         Krassi Atanassov - M0NKA, 2013-2026                     **
**                                                                                 **
**---------------------------------------------------------------------------------**
**                                                                                 **
**  File name:		sha512.c                                                       **
**  Description:	SHA-512 (FIPS 180-4), required by Ed25519                      **
**  Licence:		https://github.com/m0nka/mcHF/blob/main/LICENSE                **
************************************************************************************/
//
// Plain reference implementation - Ed25519 hashes at most a few hundred
// bytes per signature here, so the unrolled/table variants would only
// cost flash. Built for the host test too (MC_CRYPTO_HOST_BUILD), which
// is where it is checked against the FIPS 180-4 vectors
//
#ifndef MC_CRYPTO_HOST_BUILD
#include "main.h"
#include "mchf_pro_board.h"
#endif

#if defined(MC_CRYPTO_HOST_BUILD) || (defined(CONTEXT_LORA) && defined(MESHCORE))

#include <string.h>

#include "sha512.h"

// First 64 bits of the fractional parts of the cube roots of the first
// 80 primes
static const uint64_t sha512_k[80] =
{
	0x428A2F98D728AE22ULL, 0x7137449123EF65CDULL,
	0xB5C0FBCFEC4D3B2FULL, 0xE9B5DBA58189DBBCULL,
	0x3956C25BF348B538ULL, 0x59F111F1B605D019ULL,
	0x923F82A4AF194F9BULL, 0xAB1C5ED5DA6D8118ULL,
	0xD807AA98A3030242ULL, 0x12835B0145706FBEULL,
	0x243185BE4EE4B28CULL, 0x550C7DC3D5FFB4E2ULL,
	0x72BE5D74F27B896FULL, 0x80DEB1FE3B1696B1ULL,
	0x9BDC06A725C71235ULL, 0xC19BF174CF692694ULL,
	0xE49B69C19EF14AD2ULL, 0xEFBE4786384F25E3ULL,
	0x0FC19DC68B8CD5B5ULL, 0x240CA1CC77AC9C65ULL,
	0x2DE92C6F592B0275ULL, 0x4A7484AA6EA6E483ULL,
	0x5CB0A9DCBD41FBD4ULL, 0x76F988DA831153B5ULL,
	0x983E5152EE66DFABULL, 0xA831C66D2DB43210ULL,
	0xB00327C898FB213FULL, 0xBF597FC7BEEF0EE4ULL,
	0xC6E00BF33DA88FC2ULL, 0xD5A79147930AA725ULL,
	0x06CA6351E003826FULL, 0x142929670A0E6E70ULL,
	0x27B70A8546D22FFCULL, 0x2E1B21385C26C926ULL,
	0x4D2C6DFC5AC42AEDULL, 0x53380D139D95B3DFULL,
	0x650A73548BAF63DEULL, 0x766A0ABB3C77B2A8ULL,
	0x81C2C92E47EDAEE6ULL, 0x92722C851482353BULL,
	0xA2BFE8A14CF10364ULL, 0xA81A664BBC423001ULL,
	0xC24B8B70D0F89791ULL, 0xC76C51A30654BE30ULL,
	0xD192E819D6EF5218ULL, 0xD69906245565A910ULL,
	0xF40E35855771202AULL, 0x106AA07032BBD1B8ULL,
	0x19A4C116B8D2D0C8ULL, 0x1E376C085141AB53ULL,
	0x2748774CDF8EEB99ULL, 0x34B0BCB5E19B48A8ULL,
	0x391C0CB3C5C95A63ULL, 0x4ED8AA4AE3418ACBULL,
	0x5B9CCA4F7763E373ULL, 0x682E6FF3D6B2B8A3ULL,
	0x748F82EE5DEFB2FCULL, 0x78A5636F43172F60ULL,
	0x84C87814A1F0AB72ULL, 0x8CC702081A6439ECULL,
	0x90BEFFFA23631E28ULL, 0xA4506CEBDE82BDE9ULL,
	0xBEF9A3F7B2C67915ULL, 0xC67178F2E372532BULL,
	0xCA273ECEEA26619CULL, 0xD186B8C721C0C207ULL,
	0xEADA7DD6CDE0EB1EULL, 0xF57D4F7FEE6ED178ULL,
	0x06F067AA72176FBAULL, 0x0A637DC5A2C898A6ULL,
	0x113F9804BEF90DAEULL, 0x1B710B35131C471BULL,
	0x28DB77F523047D84ULL, 0x32CAAB7B40C72493ULL,
	0x3C9EBE0A15C9BEBCULL, 0x431D67C49C100D4CULL,
	0x4CC5D4BECB3E42B6ULL, 0x597F299CFC657E2AULL,
	0x5FCB6FAB3AD6FAECULL, 0x6C44198C4A475817ULL
};

#define ROTR64(x, n)	(((x) >> (n)) | ((x) << (64 - (n))))

#define BIG_SIGMA0(x)	(ROTR64(x, 28) ^ ROTR64(x, 34) ^ ROTR64(x, 39))
#define BIG_SIGMA1(x)	(ROTR64(x, 14) ^ ROTR64(x, 18) ^ ROTR64(x, 41))
#define SMALL_SIGMA0(x)	(ROTR64(x,  1) ^ ROTR64(x,  8) ^ ((x) >>  7))
#define SMALL_SIGMA1(x)	(ROTR64(x, 19) ^ ROTR64(x, 61) ^ ((x) >>  6))

#define CH(x, y, z)		(((x) & (y)) ^ ((~(x)) & (z)))
#define MAJ(x, y, z)	(((x) & (y)) ^ ((x) & (z)) ^ ((y) & (z)))

static uint64_t sha512_load_be(const uint8_t *p)
{
	return ((uint64_t)p[0] << 56) | ((uint64_t)p[1] << 48) |
		   ((uint64_t)p[2] << 40) | ((uint64_t)p[3] << 32) |
		   ((uint64_t)p[4] << 24) | ((uint64_t)p[5] << 16) |
		   ((uint64_t)p[6] <<  8) | ((uint64_t)p[7]);
}

static void sha512_store_be(uint8_t *p, uint64_t v)
{
	p[0] = (uint8_t)(v >> 56);
	p[1] = (uint8_t)(v >> 48);
	p[2] = (uint8_t)(v >> 40);
	p[3] = (uint8_t)(v >> 32);
	p[4] = (uint8_t)(v >> 24);
	p[5] = (uint8_t)(v >> 16);
	p[6] = (uint8_t)(v >>  8);
	p[7] = (uint8_t)(v);
}

static void sha512_block(sha512_ctx *ctx, const uint8_t *block)
{
	uint64_t	w[80];
	uint64_t	a, b, c, d, e, f, g, h;
	uint64_t	t1, t2;
	int			i;

	for(i = 0; i < 16; i++)
		w[i] = sha512_load_be(block + (i * 8));

	for(i = 16; i < 80; i++)
		w[i] = SMALL_SIGMA1(w[i - 2]) + w[i - 7] + SMALL_SIGMA0(w[i - 15]) + w[i - 16];

	a = ctx->state[0];
	b = ctx->state[1];
	c = ctx->state[2];
	d = ctx->state[3];
	e = ctx->state[4];
	f = ctx->state[5];
	g = ctx->state[6];
	h = ctx->state[7];

	for(i = 0; i < 80; i++)
	{
		t1 = h + BIG_SIGMA1(e) + CH(e, f, g) + sha512_k[i] + w[i];
		t2 = BIG_SIGMA0(a) + MAJ(a, b, c);

		h = g;
		g = f;
		f = e;
		e = d + t1;
		d = c;
		c = b;
		b = a;
		a = t1 + t2;
	}

	ctx->state[0] += a;
	ctx->state[1] += b;
	ctx->state[2] += c;
	ctx->state[3] += d;
	ctx->state[4] += e;
	ctx->state[5] += f;
	ctx->state[6] += g;
	ctx->state[7] += h;
}

void sha512_init(sha512_ctx *ctx)
{
	// First 64 bits of the fractional parts of the square roots of the
	// first 8 primes
	ctx->state[0] = 0x6A09E667F3BCC908ULL;
	ctx->state[1] = 0xBB67AE8584CAA73BULL;
	ctx->state[2] = 0x3C6EF372FE94F82BULL;
	ctx->state[3] = 0xA54FF53A5F1D36F1ULL;
	ctx->state[4] = 0x510E527FADE682D1ULL;
	ctx->state[5] = 0x9B05688C2B3E6C1FULL;
	ctx->state[6] = 0x1F83D9ABFB41BD6BULL;
	ctx->state[7] = 0x5BE0CD19137E2179ULL;

	ctx->count	 = 0;
	ctx->buf_len = 0;
}

void sha512_update(sha512_ctx *ctx, const uint8_t *data, size_t len)
{
	size_t	take;

	ctx->count += (uint64_t)len;

	// Top up a part filled block first
	if(ctx->buf_len)
	{
		take = SHA512_BLOCK_SIZE - ctx->buf_len;

		if(take > len)
			take = len;

		memcpy(ctx->buf + ctx->buf_len, data, take);

		ctx->buf_len += take;
		data		 += take;
		len			 -= take;

		if(ctx->buf_len == SHA512_BLOCK_SIZE)
		{
			sha512_block(ctx, ctx->buf);
			ctx->buf_len = 0;
		}
	}

	while(len >= SHA512_BLOCK_SIZE)
	{
		sha512_block(ctx, data);

		data += SHA512_BLOCK_SIZE;
		len	 -= SHA512_BLOCK_SIZE;
	}

	if(len)
	{
		memcpy(ctx->buf, data, len);
		ctx->buf_len = len;
	}
}

void sha512_final(sha512_ctx *ctx, uint8_t out[SHA512_DIGEST_SIZE])
{
	uint64_t	bits = ctx->count * 8;
	int			i;

	// 0x80 then zeroes up to a 16 byte length field. Only the low 64
	// bits of the 128 bit length are ever non zero here
	ctx->buf[ctx->buf_len++] = 0x80;

	if(ctx->buf_len > (SHA512_BLOCK_SIZE - 16))
	{
		memset(ctx->buf + ctx->buf_len, 0, SHA512_BLOCK_SIZE - ctx->buf_len);
		sha512_block(ctx, ctx->buf);
		ctx->buf_len = 0;
	}

	memset(ctx->buf + ctx->buf_len, 0, (SHA512_BLOCK_SIZE - 8) - ctx->buf_len);
	sha512_store_be(ctx->buf + SHA512_BLOCK_SIZE - 8, bits);

	sha512_block(ctx, ctx->buf);

	for(i = 0; i < 8; i++)
		sha512_store_be(out + (i * 8), ctx->state[i]);

	memset(ctx, 0, sizeof(*ctx));
}

void sha512(const uint8_t *data, size_t len, uint8_t out[SHA512_DIGEST_SIZE])
{
	sha512_ctx	ctx;

	sha512_init(&ctx);
	sha512_update(&ctx, data, len);
	sha512_final(&ctx, out);
}

#endif
