/************************************************************************************
**                                                                                 **
**                                 mcHF QRP Transceiver                            **
**                         Krassi Atanassov - M0NKA, 2013-2026                     **
**                                                                                 **
**---------------------------------------------------------------------------------**
**                                                                                 **
**  File name:		mc_ec.h                                                        **
**  Description:	Curve25519 - X25519 key agreement and Ed25519 signatures       **
**  Licence:		https://github.com/m0nka/mcHF/blob/main/LICENSE                **
************************************************************************************/
//
// What MeshCore needs from us:
//
//   - a node identity, which is an Ed25519 keypair. The public half is
//     what travels in an ADVERT and what other stations store as the
//     contact's "public key"
//   - signatures over adverts, so other nodes accept ours
//   - a shared secret with a contact for direct messages. MeshCore
//     derives it by mapping both Ed25519 keys onto Curve25519 and
//     running X25519 over the result
//
#ifndef __MC_EC_H
#define __MC_EC_H

#include <stddef.h>
#include <stdint.h>

#define MC_EC_KEY_SIZE			32
#define MC_EC_SEED_SIZE			32
#define MC_EC_SIG_SIZE			64

// ---------------------------------------------------------------------
// X25519 (RFC 7748)

// out = scalar * point. Returns 0 on success
int		mc_ec_x25519(uint8_t out[MC_EC_KEY_SIZE],
					 const uint8_t scalar[MC_EC_KEY_SIZE],
					 const uint8_t point[MC_EC_KEY_SIZE]);

// out = scalar * basepoint (u = 9)
int		mc_ec_x25519_base(uint8_t out[MC_EC_KEY_SIZE],
						  const uint8_t scalar[MC_EC_KEY_SIZE]);

// ---------------------------------------------------------------------
// Ed25519 (RFC 8032). Signatures are detached - the 64 byte signature is
// produced and consumed on its own, the message is never copied

// Derive the public key from a 32 byte seed (the private half we store)
void	mc_ec_ed25519_pubkey(uint8_t pub[MC_EC_KEY_SIZE],
							 const uint8_t seed[MC_EC_SEED_SIZE]);

// Sign. pub must be the key mc_ec_ed25519_pubkey() derived from seed -
// it is passed in rather than recomputed, since the caller already has it
void	mc_ec_ed25519_sign(uint8_t sig[MC_EC_SIG_SIZE],
						   const uint8_t *msg, size_t msg_len,
						   const uint8_t seed[MC_EC_SEED_SIZE],
						   const uint8_t pub[MC_EC_KEY_SIZE]);

// Returns 0 when the signature is good, -1 otherwise
int		mc_ec_ed25519_verify(const uint8_t sig[MC_EC_SIG_SIZE],
							 const uint8_t *msg, size_t msg_len,
							 const uint8_t pub[MC_EC_KEY_SIZE]);

// ---------------------------------------------------------------------
// Ed25519 -> Curve25519, the bridge MeshCore's direct messages need.
// The private side is just the clamped low half of SHA-512(seed); the
// public side is the birational map u = (1 + y) / (1 - y)

void	mc_ec_ed25519_to_x25519_priv(uint8_t out[MC_EC_KEY_SIZE],
									 const uint8_t seed[MC_EC_SEED_SIZE]);

void	mc_ec_ed25519_to_x25519_pub(uint8_t out[MC_EC_KEY_SIZE],
									const uint8_t ed_pub[MC_EC_KEY_SIZE]);

// Convenience: the raw X25519 shared secret between our identity seed and
// a contact's Ed25519 public key. Returns 0 on success
int		mc_ec_shared_secret(uint8_t out[MC_EC_KEY_SIZE],
							const uint8_t our_seed[MC_EC_SEED_SIZE],
							const uint8_t their_ed_pub[MC_EC_KEY_SIZE]);

#endif
