/**
 * meshtastic_crypto.h — AES-CTR encryption/decryption for Meshtastic channel PSK.
 *
 * Nonce layout from CryptoEngine.cpp:253-261:
 *   [0..7]   packetId    (uint64_t LE — upper 32 bits are always 0 for current firmware)
 *   [8..11]  fromNode    (uint32_t LE)
 *   [12..15] blockCounter (starts at 0, incremented per 16-byte AES block)
 *
 * AES-CTR works identically for encrypt and decrypt (XOR with keystream).
 *
 * Part of meshtastic-lite.
 */
#pragma once

#include <stdint.h>
#include <stddef.h>
#include <string.h>

// ─── Key Structures ────────────────────────────────────────────────────────────

struct MeshCryptoKey {
    uint8_t bytes[32];
    int8_t  length;     // 0 = no encryption, 16 = AES-128, 32 = AES-256, -1 = invalid
};

// ─── Nonce Construction ────────────────────────────────────────────────────────

/**
 * Build the 16-byte nonce/IV for AES-CTR.
 * Matches CryptoEngine::initNonce() exactly.
 */
static inline void meshBuildNonce(uint8_t nonce[16], uint32_t fromNode, uint32_t packetId) {
    memset(nonce, 0, 16);
    // packetId as uint64_t LE in bytes [0..7]
    uint64_t pid64 = (uint64_t)packetId;
    memcpy(nonce, &pid64, sizeof(uint64_t));
    // fromNode as uint32_t LE in bytes [8..11]
    memcpy(nonce + 8, &fromNode, sizeof(uint32_t));
    // bytes [12..15] = block counter, starts at 0 (handled by CTR mode)
}

// ─── Platform-specific AES-CTR ─────────────────────────────────────────────────
//
// We provide two implementations:
//   1. ESP-IDF mbedtls (hardware-accelerated on ESP32-P4)
//   2. Portable software fallback using a tiny AES implementation
//
// Define MESH_CRYPTO_USE_MBEDTLS=1 to use mbedtls (recommended on ESP-IDF).
// Otherwise a software AES-CTR is used.

#if defined(MESH_CRYPTO_USE_MBEDTLS) && MESH_CRYPTO_USE_MBEDTLS

#include "mbedtls/aes.h"

/**
 * AES-CTR encrypt/decrypt in-place.  Uses ESP-IDF mbedtls with HW acceleration.
 */
static inline bool meshCryptCtr(const MeshCryptoKey *key,
                                 uint8_t nonce[16],
                                 uint8_t *data, size_t len)
{
    if (key->length <= 0 || len == 0) return true; // no-op

    mbedtls_aes_context ctx;
    mbedtls_aes_init(&ctx);

    int ret = mbedtls_aes_setkey_enc(&ctx, key->bytes, key->length * 8);
    if (ret != 0) {
        mbedtls_aes_free(&ctx);
        return false;
    }

    // mbedtls CTR mode needs a stream block and offset counter
    uint8_t stream_block[16] = {0};
    size_t nc_off = 0;
    uint8_t nonce_counter[16];
    memcpy(nonce_counter, nonce, 16);

    ret = mbedtls_aes_crypt_ctr(&ctx, len, &nc_off, nonce_counter, stream_block, data, data);

    mbedtls_aes_free(&ctx);
    return ret == 0;
}

#else // Software AES-CTR fallback

// Minimal AES implementation for non-ESP platforms.
// You can replace this with your own AES block cipher.
// For now, we declare the interface and expect the user to link an AES impl.

#ifdef __cplusplus
extern "C" {
#endif

/**
 * External AES block encrypt function.
 * Must encrypt a single 16-byte block: out = AES_encrypt(key, in).
 * key_bits is 128 or 256.
 */
extern void mesh_aes_block_encrypt(const uint8_t *key, int key_bits,
                                    const uint8_t in[16], uint8_t out[16]);

#ifdef __cplusplus
}
#endif

/**
 * Software AES-CTR encrypt/decrypt in-place.
 */
static inline bool meshCryptCtr(const MeshCryptoKey *key,
                                 uint8_t nonce[16],
                                 uint8_t *data, size_t len)
{
    if (key->length <= 0 || len == 0) return true;

    int key_bits = key->length * 8;
    uint8_t counter_block[16];
    uint8_t keystream[16];
    memcpy(counter_block, nonce, 16);

    for (size_t offset = 0; offset < len; offset += 16) {
        mesh_aes_block_encrypt(key->bytes, key_bits, counter_block, keystream);

        size_t block_len = (len - offset < 16) ? (len - offset) : 16;
        for (size_t i = 0; i < block_len; i++) {
            data[offset + i] ^= keystream[i];
        }

        // Increment the 32-bit counter in bytes [12..15] (little-endian)
        for (int i = 12; i < 16; i++) {
            if (++counter_block[i] != 0) break;
        }
    }
    return true;
}

#endif // MESH_CRYPTO_USE_MBEDTLS

// ─── Convenience Wrappers ──────────────────────────────────────────────────────

/**
 * Decrypt a Meshtastic payload in-place.
 * `data` points to the encrypted payload (after the 16-byte header).
 */
static inline bool meshDecrypt(const MeshCryptoKey *key,
                                uint32_t fromNode, uint32_t packetId,
                                uint8_t *data, size_t len)
{
    uint8_t nonce[16];
    meshBuildNonce(nonce, fromNode, packetId);
    return meshCryptCtr(key, nonce, data, len);
}

/**
 * Encrypt a Meshtastic payload in-place. (CTR is symmetric.)
 */
static inline bool meshEncrypt(const MeshCryptoKey *key,
                                uint32_t fromNode, uint32_t packetId,
                                uint8_t *data, size_t len)
{
    return meshDecrypt(key, fromNode, packetId, data, len);
}
