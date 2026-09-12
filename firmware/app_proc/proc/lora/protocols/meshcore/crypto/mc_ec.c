/************************************************************************************
**                                                                                 **
**                                 mcHF QRP Transceiver                            **
**                         Krassi Atanassov - M0NKA, 2013-2026                     **
**                                                                                 **
**---------------------------------------------------------------------------------**
**                                                                                 **
**  File name:		mc_ec.c                                                        **
**  Description:	Curve25519 - X25519 key agreement and Ed25519 signatures       **
**  Licence:		https://github.com/m0nka/mcHF/blob/main/LICENSE                **
************************************************************************************/
//
// Derived from TweetNaCl (Bernstein, van Gent, Lange, Schwabe, public
// domain). The field element representation is TweetNaCl's: 16 signed
// 64 bit limbs in radix 2^16, which is slow next to the 32 bit radix
// 2^25.5 implementations but is a fraction of the code, and a handful of
// scalar multiplications per chat message is nothing on a 480 MHz M7.
//
// Every curve constant in here was regenerated from its definition
// rather than copied, and the whole module is checked against the RFC
// 7748 / RFC 8032 test vectors by claude/meshchat_test - build it with
// MC_CRYPTO_HOST_BUILD after touching anything below
//
#ifndef MC_CRYPTO_HOST_BUILD
#include "main.h"
#include "mchf_pro_board.h"
#endif

#if defined(MC_CRYPTO_HOST_BUILD) || (defined(CONTEXT_LORA) && defined(MESHCORE))

#include <string.h>

#include "sha512.h"
#include "mc_ec.h"

// Field element: 16 limbs, radix 2^16, signed so intermediate results
// can go negative before the carry pass folds them back
typedef int64_t	gf[16];

static const gf	mc_gf_0 = {0};
static const gf	mc_gf_1 = {1};

// a24 = 121665, the Montgomery ladder constant
static const gf	mc_gf_121665 = {0xDB41, 1};

// d = -121665 / 121666 (mod 2^255 - 19), the Edwards curve constant
static const gf	mc_gf_d =
{
	0x78A3, 0x1359, 0x4DCA, 0x75EB, 0xD8AB, 0x4141, 0x0A4D, 0x0070,
	0xE898, 0x7779, 0x4079, 0x8CC7, 0xFE73, 0x2B6F, 0x6CEE, 0x5203
};

// 2 * d
static const gf	mc_gf_d2 =
{
	0xF159, 0x26B2, 0x9B94, 0xEBD6, 0xB156, 0x8283, 0x149A, 0x00E0,
	0xD130, 0xEEF3, 0x80F2, 0x198E, 0xFCE7, 0x56DF, 0xD9DC, 0x2406
};

// Base point, x then y (y = 4/5, x recovered even)
static const gf	mc_gf_bx =
{
	0xD51A, 0x8F25, 0x2D60, 0xC956, 0xA7B2, 0x9525, 0xC760, 0x692C,
	0xDC5C, 0xFDD6, 0xE231, 0xC0A4, 0x53FE, 0xCD6E, 0x36D3, 0x2169
};

static const gf	mc_gf_by =
{
	0x6658, 0x6666, 0x6666, 0x6666, 0x6666, 0x6666, 0x6666, 0x6666,
	0x6666, 0x6666, 0x6666, 0x6666, 0x6666, 0x6666, 0x6666, 0x6666
};

// sqrt(-1) = 2^((p-1)/4)
static const gf	mc_gf_sqrtm1 =
{
	0xA0B0, 0x4A0E, 0x1B27, 0xC4EE, 0xE478, 0xAD2F, 0x1806, 0x2F43,
	0xD7A7, 0x3DFB, 0x0099, 0x2B4D, 0xDF0B, 0x4FC1, 0x2480, 0x2B83
};

// L = 2^252 + 27742317777372353535851937790883648493, the group order,
// little endian
static const uint64_t	mc_ec_l[32] =
{
	0xED, 0xD3, 0xF5, 0x5C, 0x1A, 0x63, 0x12, 0x58,
	0xD6, 0x9C, 0xF7, 0xA2, 0xDE, 0xF9, 0xDE, 0x14,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x10
};

// ---------------------------------------------------------------------
// Field arithmetic

static void mc_gf_set(gf r, const gf a)
{
	int	i;

	for(i = 0; i < 16; i++)
		r[i] = a[i];
}

//*----------------------------------------------------------------------------
//* Function Name       : mc_gf_carry
//* Object              : propagate the limb carries and fold the overflow
//*						: back in via 2^256 = 38 (mod 2^255 - 19)
//* Notes    			: the +2^16 / -1 dance keeps it branch free for
//*						: negative limbs
//*----------------------------------------------------------------------------
static void mc_gf_carry(gf o)
{
	int		i;
	int64_t	c;

	for(i = 0; i < 16; i++)
	{
		o[i] += (1LL << 16);
		c	  = o[i] >> 16;

		if(i < 15)
			o[i + 1] += c - 1;
		else
			o[0] += 38 * (c - 1);

		o[i] -= c << 16;
	}
}

// Constant time conditional swap - b must be 0 or 1
static void mc_gf_cswap(gf p, gf q, int b)
{
	int64_t	t, c = ~((int64_t)b - 1);
	int		i;

	for(i = 0; i < 16; i++)
	{
		t	  = c & (p[i] ^ q[i]);
		p[i] ^= t;
		q[i] ^= t;
	}
}

//*----------------------------------------------------------------------------
//* Function Name       : mc_gf_pack
//* Object              : field element to 32 little endian bytes, fully
//*						: reduced. Two conditional subtractions of p pull
//*						: the value into [0, p)
//*----------------------------------------------------------------------------
static void mc_gf_pack(uint8_t *o, const gf n)
{
	gf	m, t;
	int	i, j, b;

	mc_gf_set(t, n);

	mc_gf_carry(t);
	mc_gf_carry(t);
	mc_gf_carry(t);

	for(j = 0; j < 2; j++)
	{
		m[0] = t[0] - 0xFFED;

		for(i = 1; i < 15; i++)
		{
			m[i]	  = t[i] - 0xFFFF - ((m[i - 1] >> 16) & 1);
			m[i - 1] &= 0xFFFF;
		}

		m[15]  = t[15] - 0x7FFF - ((m[14] >> 16) & 1);
		b	   = (m[15] >> 16) & 1;
		m[14] &= 0xFFFF;

		mc_gf_cswap(t, m, 1 - b);
	}

	for(i = 0; i < 16; i++)
	{
		o[2 * i]	 = (uint8_t)(t[i] & 0xFF);
		o[2 * i + 1] = (uint8_t)(t[i] >> 8);
	}
}

// 32 little endian bytes to a field element. The top bit is dropped -
// for Ed25519 keys that bit carries the x sign, not field data
static void mc_gf_unpack(gf o, const uint8_t *n)
{
	int	i;

	for(i = 0; i < 16; i++)
		o[i] = n[2 * i] + ((int64_t)n[2 * i + 1] << 8);

	o[15] &= 0x7FFF;
}

static void mc_gf_add(gf o, const gf a, const gf b)
{
	int	i;

	for(i = 0; i < 16; i++)
		o[i] = a[i] + b[i];
}

static void mc_gf_sub(gf o, const gf a, const gf b)
{
	int	i;

	for(i = 0; i < 16; i++)
		o[i] = a[i] - b[i];
}

static void mc_gf_mul(gf o, const gf a, const gf b)
{
	int64_t	t[31];
	int		i, j;

	for(i = 0; i < 31; i++)
		t[i] = 0;

	for(i = 0; i < 16; i++)
		for(j = 0; j < 16; j++)
			t[i + j] += a[i] * b[j];

	// Fold the upper half down: limb i+16 has weight 2^256 = 38
	for(i = 0; i < 15; i++)
		t[i] += 38 * t[i + 16];

	for(i = 0; i < 16; i++)
		o[i] = t[i];

	mc_gf_carry(o);
	mc_gf_carry(o);
}

static void mc_gf_sq(gf o, const gf a)
{
	mc_gf_mul(o, a, a);
}

// o = 1/i, by Fermat: i^(p-2)
static void mc_gf_inv(gf o, const gf i)
{
	gf	c;
	int	a;

	mc_gf_set(c, i);

	for(a = 253; a >= 0; a--)
	{
		mc_gf_sq(c, c);

		if((a != 2) && (a != 4))
			mc_gf_mul(c, c, i);
	}

	mc_gf_set(o, c);
}

// o = i^((p-5)/8), the square root helper
static void mc_gf_pow2523(gf o, const gf i)
{
	gf	c;
	int	a;

	mc_gf_set(c, i);

	for(a = 250; a >= 0; a--)
	{
		mc_gf_sq(c, c);

		if(a != 1)
			mc_gf_mul(c, c, i);
	}

	mc_gf_set(o, c);
}

static int mc_gf_parity(const gf a)
{
	uint8_t	d[32];

	mc_gf_pack(d, a);

	return d[0] & 1;
}

static int mc_gf_neq(const gf a, const gf b)
{
	uint8_t	c[32], d[32];

	mc_gf_pack(c, a);
	mc_gf_pack(d, b);

	return (memcmp(c, d, 32) != 0) ? 1 : 0;
}

// Constant time 32 byte compare - 0 when equal
static int mc_ec_verify_32(const uint8_t *x, const uint8_t *y)
{
	uint32_t	d = 0;
	int			i;

	for(i = 0; i < 32; i++)
		d |= (uint32_t)(x[i] ^ y[i]);

	return (int)((1 & ((d - 1) >> 8)) - 1);
}

// ---------------------------------------------------------------------
// X25519

//*----------------------------------------------------------------------------
//* Function Name       : mc_ec_x25519
//* Object              : Montgomery ladder scalar multiplication
//* Notes    			: the scalar is clamped as RFC 7748 requires, so
//*						: callers may pass a raw 32 byte value
//*----------------------------------------------------------------------------
int mc_ec_x25519(uint8_t out[MC_EC_KEY_SIZE],
				 const uint8_t scalar[MC_EC_KEY_SIZE],
				 const uint8_t point[MC_EC_KEY_SIZE])
{
	uint8_t	z[32];
	int64_t	x[80];
	int64_t	r;
	gf		a, b, c, d, e, f;
	int		i;

	for(i = 0; i < 31; i++)
		z[i] = scalar[i];

	z[31] = (scalar[31] & 127) | 64;
	z[0] &= 248;

	mc_gf_unpack(x, point);

	for(i = 0; i < 16; i++)
	{
		b[i] = x[i];
		a[i] = 0;
		c[i] = 0;
		d[i] = 0;
	}

	a[0] = 1;
	d[0] = 1;

	for(i = 254; i >= 0; i--)
	{
		r = (z[i >> 3] >> (i & 7)) & 1;

		mc_gf_cswap(a, b, (int)r);
		mc_gf_cswap(c, d, (int)r);

		mc_gf_add(e, a, c);
		mc_gf_sub(a, a, c);
		mc_gf_add(c, b, d);
		mc_gf_sub(b, b, d);
		mc_gf_sq (d, e);
		mc_gf_sq (f, a);
		mc_gf_mul(a, c, a);
		mc_gf_mul(c, b, e);
		mc_gf_add(e, a, c);
		mc_gf_sub(a, a, c);
		mc_gf_sq (b, a);
		mc_gf_sub(c, d, f);
		mc_gf_mul(a, c, mc_gf_121665);
		mc_gf_add(a, a, d);
		mc_gf_mul(c, c, a);
		mc_gf_mul(a, d, f);
		mc_gf_mul(d, b, x);
		mc_gf_sq (b, e);

		mc_gf_cswap(a, b, (int)r);
		mc_gf_cswap(c, d, (int)r);
	}

	for(i = 0; i < 16; i++)
	{
		x[i + 16] = a[i];
		x[i + 32] = c[i];
		x[i + 48] = b[i];
		x[i + 64] = d[i];
	}

	mc_gf_inv(x + 32, x + 32);
	mc_gf_mul(x + 16, x + 16, x + 32);
	mc_gf_pack(out, x + 16);

	return 0;
}

int mc_ec_x25519_base(uint8_t out[MC_EC_KEY_SIZE], const uint8_t scalar[MC_EC_KEY_SIZE])
{
	static const uint8_t	base[32] = { 9 };

	return mc_ec_x25519(out, scalar, base);
}

// ---------------------------------------------------------------------
// Ed25519

//*----------------------------------------------------------------------------
//* Function Name       : mc_ec_mod_l
//* Object              : reduce a 64 byte little endian value mod L into
//*						: 32 bytes - Barrett style, as in TweetNaCl
//*----------------------------------------------------------------------------
static void mc_ec_mod_l(uint8_t *r, int64_t x[64])
{
	int64_t	carry;
	int		i, j;

	for(i = 63; i >= 32; i--)
	{
		carry = 0;

		for(j = i - 32; j < (i - 12); j++)
		{
			x[j]  += carry - 16 * x[i] * (int64_t)mc_ec_l[j - (i - 32)];
			carry  = (x[j] + 128) >> 8;
			x[j]  -= carry << 8;
		}

		x[j] += carry;
		x[i]  = 0;
	}

	carry = 0;

	for(j = 0; j < 32; j++)
	{
		x[j]  += carry - (x[31] >> 4) * (int64_t)mc_ec_l[j];
		carry  = x[j] >> 8;
		x[j]  &= 255;
	}

	for(j = 0; j < 32; j++)
		x[j] -= carry * (int64_t)mc_ec_l[j];

	for(i = 0; i < 32; i++)
	{
		x[i + 1] += x[i] >> 8;
		r[i]	  = (uint8_t)(x[i] & 255);
	}
}

static void mc_ec_reduce(uint8_t r[64])
{
	int64_t	x[64];
	int		i;

	for(i = 0; i < 64; i++)
		x[i] = (int64_t)r[i];

	memset(r, 0, 64);

	mc_ec_mod_l(r, x);
}

// Extended Edwards point: x, y, z, t
static void mc_ec_pt_add(gf p[4], gf q[4])
{
	gf	a, b, c, d, t, e, f, g, h;

	mc_gf_sub(a, p[1], p[0]);
	mc_gf_sub(t, q[1], q[0]);
	mc_gf_mul(a, a, t);

	mc_gf_add(b, p[0], p[1]);
	mc_gf_add(t, q[0], q[1]);
	mc_gf_mul(b, b, t);

	mc_gf_mul(c, p[3], q[3]);
	mc_gf_mul(c, c, mc_gf_d2);

	mc_gf_mul(d, p[2], q[2]);
	mc_gf_add(d, d, d);

	mc_gf_sub(e, b, a);
	mc_gf_sub(f, d, c);
	mc_gf_add(g, d, c);
	mc_gf_add(h, b, a);

	mc_gf_mul(p[0], e, f);
	mc_gf_mul(p[1], h, g);
	mc_gf_mul(p[2], g, f);
	mc_gf_mul(p[3], e, h);
}

static void mc_ec_pt_cswap(gf p[4], gf q[4], uint8_t b)
{
	int	i;

	for(i = 0; i < 4; i++)
		mc_gf_cswap(p[i], q[i], b);
}

static void mc_ec_pt_pack(uint8_t *r, gf p[4])
{
	gf	tx, ty, zi;

	mc_gf_inv(zi, p[2]);
	mc_gf_mul(tx, p[0], zi);
	mc_gf_mul(ty, p[1], zi);

	mc_gf_pack(r, ty);

	r[31] ^= (uint8_t)(mc_gf_parity(tx) << 7);
}

static void mc_ec_scalarmult(gf p[4], gf q[4], const uint8_t *s)
{
	int	i;

	mc_gf_set(p[0], mc_gf_0);
	mc_gf_set(p[1], mc_gf_1);
	mc_gf_set(p[2], mc_gf_1);
	mc_gf_set(p[3], mc_gf_0);

	for(i = 255; i >= 0; i--)
	{
		uint8_t	b = (s[i / 8] >> (i & 7)) & 1;

		mc_ec_pt_cswap(p, q, b);
		mc_ec_pt_add(q, p);
		mc_ec_pt_add(p, p);
		mc_ec_pt_cswap(p, q, b);
	}
}

static void mc_ec_scalarbase(gf p[4], const uint8_t *s)
{
	gf	q[4];

	mc_gf_set(q[0], mc_gf_bx);
	mc_gf_set(q[1], mc_gf_by);
	mc_gf_set(q[2], mc_gf_1);
	mc_gf_mul(q[3], mc_gf_bx, mc_gf_by);

	mc_ec_scalarmult(p, q, s);
}

// Expand a seed into the clamped scalar (low half) and the prefix used
// for deterministic nonces (high half)
static void mc_ec_expand_seed(uint8_t d[64], const uint8_t seed[MC_EC_SEED_SIZE])
{
	sha512(seed, MC_EC_SEED_SIZE, d);

	d[0]  &= 248;
	d[31] &= 127;
	d[31] |= 64;
}

void mc_ec_ed25519_pubkey(uint8_t pub[MC_EC_KEY_SIZE], const uint8_t seed[MC_EC_SEED_SIZE])
{
	uint8_t	d[64];
	gf		p[4];

	mc_ec_expand_seed(d, seed);
	mc_ec_scalarbase(p, d);
	mc_ec_pt_pack(pub, p);

	memset(d, 0, sizeof(d));
}

//*----------------------------------------------------------------------------
//* Function Name       : mc_ec_ed25519_sign
//* Object              : detached Ed25519 signature
//* Notes    			: the two hashes are streamed rather than built in
//*						: one buffer, so a signature costs no scratch
//*						: proportional to the message
//*----------------------------------------------------------------------------
void mc_ec_ed25519_sign(uint8_t sig[MC_EC_SIG_SIZE],
						const uint8_t *msg, size_t msg_len,
						const uint8_t seed[MC_EC_SEED_SIZE],
						const uint8_t pub[MC_EC_KEY_SIZE])
{
	uint8_t		d[64], r[64], h[64];
	int64_t		x[64];
	gf			p[4];
	sha512_ctx	ctx;
	int			i, j;

	mc_ec_expand_seed(d, seed);

	// r = H(prefix || msg) mod L
	sha512_init(&ctx);
	sha512_update(&ctx, d + 32, 32);
	sha512_update(&ctx, msg, msg_len);
	sha512_final(&ctx, r);

	mc_ec_reduce(r);

	// R = [r] * B
	mc_ec_scalarbase(p, r);
	mc_ec_pt_pack(sig, p);

	// h = H(R || A || msg) mod L
	sha512_init(&ctx);
	sha512_update(&ctx, sig, 32);
	sha512_update(&ctx, pub, 32);
	sha512_update(&ctx, msg, msg_len);
	sha512_final(&ctx, h);

	mc_ec_reduce(h);

	// S = r + h * a  (mod L)
	for(i = 0; i < 64; i++)
		x[i] = 0;

	for(i = 0; i < 32; i++)
		x[i] = (int64_t)r[i];

	for(i = 0; i < 32; i++)
		for(j = 0; j < 32; j++)
			x[i + j] += (int64_t)h[i] * (int64_t)d[j];

	mc_ec_mod_l(sig + 32, x);

	memset(d, 0, sizeof(d));
	memset(r, 0, sizeof(r));
}

//*----------------------------------------------------------------------------
//* Function Name       : mc_ec_unpack_neg
//* Object              : decompress an Ed25519 public key into -A, the
//*						: form the verification equation wants
//*----------------------------------------------------------------------------
static int mc_ec_unpack_neg(gf r[4], const uint8_t p[32])
{
	gf	t, chk, num, den, den2, den4, den6;

	mc_gf_set(r[2], mc_gf_1);
	mc_gf_unpack(r[1], p);

	// x^2 = (y^2 - 1) / (d * y^2 + 1)
	mc_gf_sq (num, r[1]);
	mc_gf_mul(den, num, mc_gf_d);
	mc_gf_sub(num, num, r[2]);
	mc_gf_add(den, r[2], den);

	mc_gf_sq (den2, den);
	mc_gf_sq (den4, den2);
	mc_gf_mul(den6, den4, den2);
	mc_gf_mul(t, den6, num);
	mc_gf_mul(t, t, den);

	mc_gf_pow2523(t, t);
	mc_gf_mul(t, t, num);
	mc_gf_mul(t, t, den);
	mc_gf_mul(t, t, den);
	mc_gf_mul(r[0], t, den);

	mc_gf_sq (chk, r[0]);
	mc_gf_mul(chk, chk, den);

	if(mc_gf_neq(chk, num))
		mc_gf_mul(r[0], r[0], mc_gf_sqrtm1);

	mc_gf_sq (chk, r[0]);
	mc_gf_mul(chk, chk, den);

	if(mc_gf_neq(chk, num))
		return -1;

	// Negate, so the caller can add instead of subtract
	if(mc_gf_parity(r[0]) == (p[31] >> 7))
		mc_gf_sub(r[0], mc_gf_0, r[0]);

	mc_gf_mul(r[3], r[0], r[1]);

	return 0;
}

int mc_ec_ed25519_verify(const uint8_t sig[MC_EC_SIG_SIZE],
						 const uint8_t *msg, size_t msg_len,
						 const uint8_t pub[MC_EC_KEY_SIZE])
{
	uint8_t		h[64], t[32];
	gf			p[4], q[4];
	sha512_ctx	ctx;

	if(mc_ec_unpack_neg(q, pub))
		return -1;

	// S must be reduced - an unreduced one would let a signature be
	// mauled into a second valid encoding
	if(sig[63] & 0xE0)
		return -1;

	sha512_init(&ctx);
	sha512_update(&ctx, sig, 32);
	sha512_update(&ctx, pub, 32);
	sha512_update(&ctx, msg, msg_len);
	sha512_final(&ctx, h);

	mc_ec_reduce(h);

	// [h] * (-A) + [S] * B should land back on R
	mc_ec_scalarmult(p, q, h);
	mc_ec_scalarbase(q, sig + 32);
	mc_ec_pt_add(p, q);
	mc_ec_pt_pack(t, p);

	return mc_ec_verify_32(sig, t);
}

// ---------------------------------------------------------------------
// Ed25519 -> Curve25519

void mc_ec_ed25519_to_x25519_priv(uint8_t out[MC_EC_KEY_SIZE],
								  const uint8_t seed[MC_EC_SEED_SIZE])
{
	uint8_t	d[64];

	mc_ec_expand_seed(d, seed);

	memcpy(out, d, 32);
	memset(d, 0, sizeof(d));
}

void mc_ec_ed25519_to_x25519_pub(uint8_t out[MC_EC_KEY_SIZE],
								 const uint8_t ed_pub[MC_EC_KEY_SIZE])
{
	gf	y, num, den, inv, u;

	// u = (1 + y) / (1 - y). The sign bit in ed_pub[31] is not field
	// data and mc_gf_unpack already drops it
	mc_gf_unpack(y, ed_pub);

	mc_gf_add(num, mc_gf_1, y);
	mc_gf_sub(den, mc_gf_1, y);

	mc_gf_inv(inv, den);
	mc_gf_mul(u, num, inv);

	mc_gf_pack(out, u);
}

int mc_ec_shared_secret(uint8_t out[MC_EC_KEY_SIZE],
						const uint8_t our_seed[MC_EC_SEED_SIZE],
						const uint8_t their_ed_pub[MC_EC_KEY_SIZE])
{
	uint8_t	priv[32], pub[32];
	int		res;

	mc_ec_ed25519_to_x25519_priv(priv, our_seed);
	mc_ec_ed25519_to_x25519_pub (pub,  their_ed_pub);

	res = mc_ec_x25519(out, priv, pub);

	memset(priv, 0, sizeof(priv));

	return res;
}

#endif
