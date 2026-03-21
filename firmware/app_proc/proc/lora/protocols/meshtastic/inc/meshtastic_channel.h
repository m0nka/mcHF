/**
 * meshtastic_channel.h — Channel management: PSK key expansion, channel hashing,
 * and multi-channel decrypt matching.
 *
 * Extracted from:
 *   - Channels.h:144-145 (defaultpsk)
 *   - Channels.cpp:27-52 (xorHash, generateHash)
 *   - Channels.cpp:208-256 (getKey / PSK expansion)
 *   - Router.cpp:485-517 (multi-channel decrypt walk)
 *
 * Part of meshtastic-lite.
 */
#pragma once

#include "meshtastic_crypto.h"
#include "meshtastic_config.h"
#include <string.h>

// ─── Constants ─────────────────────────────────────────────────────────────────

#define MESH_MAX_CHANNELS 8

/**
 * The well-known 16-byte AES-128 default PSK (Channels.h:144).
 * This is the expanded form of the single-byte PSK index 1 ("AQ==").
 * All nodes on the default "LongFast" channel share this key.
 */
static const uint8_t MESH_DEFAULT_PSK[16] = {
    0xd4, 0xf1, 0xbb, 0x3a, 0x20, 0x29, 0x07, 0x59,
    0xf0, 0xbc, 0xff, 0xab, 0xcf, 0x4e, 0x69, 0x01
};

// ─── Channel Configuration ─────────────────────────────────────────────────────

struct MeshChannel {
    char         name[32];       // Empty string "" means use preset name
    uint8_t      psk_raw[34];    // Raw PSK as configured (may be 0, 1, 16, or 32 bytes)
    uint8_t      psk_raw_len;
    bool         enabled;
    bool         is_primary;

    // Computed at init:
    MeshCryptoKey key;           // Expanded AES key
    uint8_t       hash;          // Channel hash for wire matching
};

// ─── XOR Hash (Channels.cpp:27-33) ────────────────────────────────────────────

static inline uint8_t meshXorHash(const uint8_t *p, size_t len) {
    uint8_t code = 0;
    for (size_t i = 0; i < len; i++)
        code ^= p[i];
    return code;
}

// ─── PSK Expansion (Channels.cpp:208-256) ──────────────────────────────────────

/**
 * Expand a raw PSK into a full AES key, following Meshtastic's rules:
 *   - 0 bytes  → no encryption (key.length = 0)
 *   - 1 byte   → "short PSK index": 0 = no encrypt, 1+ = defaultpsk with last byte bumped
 *   - 2-15     → pad to 16 bytes with zeros (AES-128)
 *   - 16 bytes → AES-128 as-is
 *   - 17-31    → pad to 32 bytes with zeros (AES-256)
 *   - 32 bytes → AES-256 as-is
 */
static inline MeshCryptoKey meshExpandPsk(const uint8_t *raw, uint8_t raw_len) {
    MeshCryptoKey k;
    memset(k.bytes, 0, sizeof(k.bytes));

    if (raw_len == 0) {
        k.length = 0;   // no encryption
    } else if (raw_len == 1) {
        uint8_t idx = raw[0];
        if (idx == 0) {
            k.length = 0;  // explicitly disabled
        } else {
            memcpy(k.bytes, MESH_DEFAULT_PSK, 16);
            k.bytes[15] = MESH_DEFAULT_PSK[15] + idx - 1;
            k.length = 16;
        }
    } else if (raw_len <= 16) {
        memcpy(k.bytes, raw, raw_len);
        k.length = 16;
    } else {
        memcpy(k.bytes, raw, (raw_len > 32) ? 32 : raw_len);
        k.length = 32;
    }
    return k;
}

// ─── Channel Hash Computation (Channels.cpp:39-52) ─────────────────────────────

/**
 * Compute the single-byte channel hash.
 * hash = XOR(name_bytes) ^ XOR(expanded_key_bytes)
 *
 * `name` is the effective channel name: if the channel name is empty "",
 * use the modem preset name (e.g., "LongFast").
 */
static inline uint8_t meshComputeChannelHash(const char *effective_name,
                                              const MeshCryptoKey *key) {
    uint8_t h = meshXorHash((const uint8_t *)effective_name, strlen(effective_name));
    if (key->length > 0)
        h ^= meshXorHash(key->bytes, key->length);
    return h;
}

// ─── Channel Table ─────────────────────────────────────────────────────────────

struct MeshChannelTable {
    MeshChannel   channels[MESH_MAX_CHANNELS];
    uint8_t       count;
    MeshModemPreset preset;  // needed for default channel name resolution

    /**
     * Initialize with zero channels.
     */
    void init(MeshModemPreset p) {
        memset(channels, 0, sizeof(channels));
        count = 0;
        preset = p;
    }

    /**
     * Get the effective name for a channel (empty → preset name).
     */
    const char* effectiveName(uint8_t idx) const {
        if (idx >= count) return "";
        const char *n = channels[idx].name;
        if (n[0] == '\0')
            return meshPresetName(preset);
        return n;
    }

    /**
     * Add a channel. Returns the channel index, or -1 if full.
     *
     * `name`: channel name ("" for default)
     * `psk`: raw PSK bytes (1 byte = short index, 16/32 bytes = full key)
     * `psk_len`: length of psk
     * `primary`: true if this is the primary channel
     */
    int addChannel(const char *name, const uint8_t *psk, uint8_t psk_len, bool primary) {
        if (count >= MESH_MAX_CHANNELS) return -1;

        MeshChannel *ch = &channels[count];
        memset(ch, 0, sizeof(MeshChannel));

        strncpy(ch->name, name ? name : "", sizeof(ch->name) - 1);
        if (psk && psk_len > 0 && psk_len <= sizeof(ch->psk_raw)) {
            memcpy(ch->psk_raw, psk, psk_len);
            ch->psk_raw_len = psk_len;
        } else {
            ch->psk_raw_len = 0;
        }
        ch->enabled = true;
        ch->is_primary = primary;

        // Expand PSK
        ch->key = meshExpandPsk(ch->psk_raw, ch->psk_raw_len);

        // If secondary with no PSK, inherit from primary
        if (!primary && ch->key.length <= 0) {
            for (uint8_t i = 0; i < count; i++) {
                if (channels[i].is_primary && channels[i].enabled) {
                    ch->key = channels[i].key;
                    break;
                }
            }
        }

        // Compute hash
        const char *eff = (ch->name[0] == '\0') ? meshPresetName(preset) : ch->name;
        ch->hash = meshComputeChannelHash(eff, &ch->key);

        return count++;
    }

    /**
     * Convenience: add the default LongFast channel (PSK index 1).
     */
    int addDefaultChannel() {
        uint8_t default_psk_idx = 1;
        return addChannel("", &default_psk_idx, 1, true);
    }

    /**
     * Try to find a channel matching a given wire hash, and return its key.
     * Used during RX to attempt decryption against all configured channels.
     *
     * Returns the channel index if found, or -1 if no match.
     * On success, `out_key` is populated with the matching channel's key.
     */
    int findByHash(uint8_t wire_hash, MeshCryptoKey *out_key) const {
        for (uint8_t i = 0; i < count; i++) {
            if (channels[i].enabled && channels[i].hash == wire_hash) {
                if (out_key) *out_key = channels[i].key;
                return i;
            }
        }
        return -1;
    }

    /**
     * Full multi-channel decrypt attempt for a received packet.
     *
     * Tries each channel whose hash matches the wire hash. For each match,
     * copies the ciphertext, decrypts, and attempts protobuf decode validation.
     * This mirrors Router.cpp:485-517.
     *
     * `payload`: encrypted payload bytes (will NOT be modified)
     * `payload_len`: length of encrypted payload
     * `wire_hash`: channel hash from the packet header
     * `from_node`: sender NodeNum (for nonce)
     * `packet_id`: packet ID (for nonce)
     * `out_plaintext`: buffer to receive decrypted bytes (must be >= payload_len)
     * `out_key`: receives the matching key
     *
     * Returns channel index on success, -1 on failure.
     * `validate_fn` is an optional callback to validate decoded protobuf data.
     * If NULL, any channel hash match is accepted (first-match).
     * Signature: bool validate(const uint8_t *plaintext, size_t len)
     */
    int tryDecrypt(const uint8_t *payload, size_t payload_len,
                   uint8_t wire_hash,
                   uint32_t from_node, uint32_t packet_id,
                   uint8_t *out_plaintext,
                   MeshCryptoKey *out_key,
                   bool (*validate_fn)(const uint8_t *, size_t) = nullptr) const
    {
        for (uint8_t i = 0; i < count; i++) {
            if (!channels[i].enabled) continue;
            if (channels[i].hash != wire_hash) continue;

            // Copy ciphertext (fresh copy per attempt, matching firmware behavior)
            memcpy(out_plaintext, payload, payload_len);

            // Decrypt in-place
            if (!meshDecrypt(&channels[i].key, from_node, packet_id,
                             out_plaintext, payload_len))
                continue;

            // Validate if callback provided
            if (validate_fn && !validate_fn(out_plaintext, payload_len))
                continue;

            if (out_key) *out_key = channels[i].key;
            return i;
        }
        return -1;
    }
};
