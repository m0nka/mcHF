/**
 * meshtastic_radio.h — RadioLib integration for Meshtastic.
 *
 * Handles frequency selection (DJB2 hash), CSMA/CA, and provides a clean
 * interface between RadioLib's SX1262 driver and our protocol layer.
 *
 * Extracted from:
 *   - RadioInterface.cpp:732-741  (DJB2 hash for freq selection)
 *   - RadioInterface.cpp:842-867  (frequency calculation)
 *   - RadioInterface.cpp:571-633  (CSMA/CA delays)
 *   - SX126xInterface.cpp:94      (RadioLib init params)
 *
 * Part of meshtastic-lite.
 */
#pragma once

#include "meshtastic_config.h"
#include "meshtastic_packet.h"
#include "meshtastic_channel.h"
#include "meshtastic_crypto.h"
#include "meshtastic_pb.h"

#include <stdint.h>
#include <stdlib.h>  // for rand()
#include <math.h>

// ─── DJB2 Hash (RadioInterface.cpp:732-741) ───────────────────────────────────

/**
 * DJB2 hash used for channel→frequency mapping.
 * MUST match the firmware's implementation exactly for interop.
 */
static inline uint32_t meshDjb2Hash(const char *str) {
    uint32_t hash = 5381;
    int c;
    while ((c = *str++) != 0)
        hash = ((hash << 5) + hash) + (unsigned char)c;
    return hash;
}

// ─── Frequency Calculation ─────────────────────────────────────────────────────

struct MeshFreqConfig {
    float    frequency_mhz;    // center frequency
    uint32_t channel_num;      // which slot within the region
    uint32_t num_channels;     // total channel slots in region
};

/**
 * Calculate the LoRa center frequency for a given region, preset, and channel name.
 * Matches RadioInterface.cpp:842-867 exactly.
 *
 * `channel_name`: the effective channel name (preset name if default, e.g. "LongFast")
 * `slot_override`: if >= 0, use this slot instead of hash-based calculation
 *                  (matches Meshtastic's config.lora.channel_num)
 */
static inline MeshFreqConfig meshCalcFrequency(const RegionDef *region,
                                                 MeshModemPreset preset,
                                                 const char *channel_name,
                                                 int32_t slot_override = -1)
{
    ModemParams mp = meshPresetParams(preset);
    float bw = mp.bw_khz;

    uint32_t numChannels = (uint32_t)floor(
        (region->freq_end - region->freq_start) / (bw / 1000.0f));
    if (numChannels == 0) numChannels = 1;

    uint32_t channel_num;
    if (slot_override >= 0 && (uint32_t)slot_override < numChannels) {
        channel_num = (uint32_t)slot_override;
    } else {
        channel_num = meshDjb2Hash(channel_name) % numChannels;
    }

    // freq = freqStart + (bw/2000) + (channel_num * (bw/1000))
    float freq = region->freq_start + (bw / 2000.0f) + (channel_num * (bw / 1000.0f));

    return { freq, channel_num, numChannels };
}

// ─── Slot Time Calculation ─────────────────────────────────────────────────────

/**
 * Compute slot time in ms for CSMA/CA.
 * From RadioInterface.cpp:888-899 (sub-GHz path).
 */
static inline uint32_t meshSlotTimeMs(MeshModemPreset preset) {
    ModemParams mp = meshPresetParams(preset);
    float symbol_time_ms = (float)(1 << mp.sf) / mp.bw_khz;  // pow_of_2(sf) / bw
    float propagation_turnaround_mac = 0.2f + 0.4f + 7.0f;

    // CAD duration for SX126x: NUM_SYM_CAD + 0.5 symbols (min 2.25)
    float cad_symbols = (MESH_NUM_SYM_CAD + 0.5f);
    if (cad_symbols < 2.25f) cad_symbols = 2.25f;

    return (uint32_t)(cad_symbols * symbol_time_ms + propagation_turnaround_mac);
}

// ─── CSMA/CA Delays ────────────────────────────────────────────────────────────

/**
 * Random TX delay for CSMA/CA before sending.
 * From RadioInterface.cpp:571-581.
 *
 * `channel_util_pct`: current channel utilization 0-100
 */
static inline uint32_t meshTxDelayMs(MeshModemPreset preset, float channel_util_pct = 0) {
    uint32_t slot = meshSlotTimeMs(preset);
    // CW size scales with channel utilization
    uint8_t cw_size = MESH_CW_MIN +
        (uint8_t)((MESH_CW_MAX - MESH_CW_MIN) * (channel_util_pct / 100.0f));
    if (cw_size > MESH_CW_MAX) cw_size = MESH_CW_MAX;
    uint32_t max_slots = 1u << cw_size;
    return (rand() % max_slots) * slot;
}

/**
 * SNR-weighted rebroadcast delay for flooding.
 * From RadioInterface.cpp:614-633.
 *
 * Lower SNR = shorter delay (farther nodes flood first).
 * ROUTER role: shorter delay (random within 2*CWsize slots).
 * CLIENT/ROUTER_LATE: offset by 2*CWmax*slot + random within CWsize slots.
 */
static inline uint32_t meshRebroadcastDelayMs(MeshModemPreset preset, MeshRole role, float snr) {
    uint32_t slot = meshSlotTimeMs(preset);

    // Map SNR [-20, 10] → CW [CWmin, CWmax]
    float snr_clamped = snr;
    if (snr_clamped < -20.0f) snr_clamped = -20.0f;
    if (snr_clamped > 10.0f)  snr_clamped = 10.0f;
    uint8_t cw_size = MESH_CW_MIN +
        (uint8_t)((MESH_CW_MAX - MESH_CW_MIN) * ((snr_clamped - (-20.0f)) / 30.0f));

    if (role == ROLE_CLIENT_MUTE) {
        return UINT32_MAX; // never rebroadcast
    }

    // ROUTER_LATE gets offset like a regular client (non-router path)
    // Regular ROUTER would use the short-delay path, but we only support ROUTER_LATE
    uint32_t offset = (2 * MESH_CW_MAX * slot);
    uint32_t random_part = (rand() % (1u << cw_size)) * slot;
    return offset + random_part;
}

// ─── Packet Time Calculation ───────────────────────────────────────────────────

/**
 * Estimate airtime in ms for a LoRa packet of `total_bytes` length.
 * Uses the SX1276/SX1262 time-on-air formula from Semtech AN1200.13.
 */
static inline uint32_t meshPacketAirtimeMs(MeshModemPreset preset, size_t total_bytes) {
    ModemParams mp = meshPresetParams(preset);
    float t_sym = (float)(1 << mp.sf) / mp.bw_khz; // ms per symbol
    float t_preamble = (MESH_PREAMBLE_LENGTH + 4.25f) * t_sym;

    // payload symbol count
    float de = (mp.bw_khz <= 125.0f && mp.sf >= 11) ? 1.0f : 0.0f; // low data rate opt
    int pl = (int)total_bytes;
    float num = 8.0f * pl - 4.0f * mp.sf + 28.0f + 16.0f; // +16 for CRC
    float denom = 4.0f * (mp.sf - 2.0f * de);
    float payload_symbols = 8.0f + fmaxf(ceilf(num / denom) * mp.cr, 0.0f);

    float t_payload = payload_symbols * t_sym;
    return (uint32_t)(t_preamble + t_payload + 0.5f);
}

// ─── RadioLib Configuration Helper ─────────────────────────────────────────────

/**
 * Parameters to pass to RadioLib's SX1262::begin() or equivalent.
 * Matches the call in SX126xInterface.cpp:94.
 */
struct MeshRadioConfig {
    float    frequency_mhz;
    float    bandwidth_khz;
    uint8_t  spreading_factor;
    uint8_t  coding_rate;       // denominator (5 = 4/5, 8 = 4/8)
    uint8_t  sync_word;
    int8_t   tx_power_dbm;
    uint16_t preamble_length;
};

/**
 * Build a complete radio config from region + preset + channel name.
 * `slot_override`: if >= 0, use this frequency slot instead of hash-based
 */
static inline MeshRadioConfig meshBuildRadioConfig(MeshRegion region,
                                                     MeshModemPreset preset,
                                                     const char *channel_name,
                                                     int8_t tx_power = 0,
                                                     int32_t slot_override = -1)
{
    const RegionDef *rd = meshGetRegion(region);
    ModemParams mp = meshPresetParams(preset);
    MeshFreqConfig fc = meshCalcFrequency(rd, preset, channel_name, slot_override);

    int8_t power = tx_power;
    if (power == 0 || power > (int8_t)rd->power_limit)
        power = rd->power_limit;
    if (power == 0) power = 17; // fallback default

    return {
        .frequency_mhz   = fc.frequency_mhz,
        .bandwidth_khz    = mp.bw_khz,
        .spreading_factor = mp.sf,
        .coding_rate      = mp.cr,
        .sync_word        = MESH_SYNC_WORD,
        .tx_power_dbm     = power,
        .preamble_length  = MESH_PREAMBLE_LENGTH,
    };
}

// ─── Packet ID Generation ──────────────────────────────────────────────────────

/**
 * Generate a Meshtastic-compatible packet ID.
 * From mesh.proto and the firmware: lower 10 bits are a sequential counter,
 * upper 22 bits are random. Both combine to form a 32-bit ID.
 */
struct MeshPacketIdGen {
    uint32_t counter;
    uint32_t random_base;

    void init() {
        counter = 0;
        random_base = (uint32_t)rand() & 0xFFFFFC00u; // upper 22 bits
    }

    uint32_t next() {
        uint32_t id = random_base | (counter & 0x3FF);
        counter++;
        if ((counter & 0x3FF) == 0) {
            // Wrap: pick new random base
            random_base = (uint32_t)rand() & 0xFFFFFC00u;
        }
        return id;
    }
};

// ─── Complete RX/TX Session ────────────────────────────────────────────────────

/**
 * High-level Meshtastic session state.
 * Ties together channels, radio config, role, and packet ID generation.
 *
 * Usage:
 *   MeshSession session;
 *   session.init(REGION_US, MODEM_LONG_FAST, ROLE_CLIENT, myNodeNum);
 *   session.channels.addDefaultChannel();
 *   MeshRadioConfig rc = session.radioConfig();
 *   // ... configure RadioLib with rc ...
 *   // On RX:
 *   MeshRxResult result;
 *   if (session.processRx(raw_bytes, raw_len, rssi, snr, &result)) { ... }
 */

struct MeshRxResult {
    MeshRxPacket packet;      // parsed header + raw/decrypted payload
    MeshData     data;        // decoded Data envelope
    int8_t       channel_idx; // which channel matched (-1 if none/PKI)
    bool         decrypted;   // true if decryption succeeded
    bool         pki_encrypted; // true if decrypted via PKI (DM)
};

struct MeshSession {
    MeshRegion        region;
    MeshModemPreset   preset;
    MeshRole          role;
    uint32_t          node_num;     // our node address (bottom 4 bytes of MAC)
    int32_t           freq_slot;    // -1 = hash-based, >= 0 = explicit slot override
    MeshChannelTable  channels;
    MeshPacketIdGen   id_gen;

    // PKI state
    MeshPkiIdentity   pki;          // our x25519 keypair
    MeshNodeKeyStore  node_keys;    // known remote node public keys

    void init(MeshRegion r, MeshModemPreset p, MeshRole rl, uint32_t node,
              int32_t slot = -1) {
        region    = r;
        preset    = p;
        role      = rl;
        node_num  = node;
        freq_slot = slot;
        channels.init(p);
        id_gen.init();
        pki.init();
        node_keys.init();
    }

    /**
     * Get the radio config for the primary channel.
     */
    MeshRadioConfig radioConfig(int8_t tx_power = 0) const {
        const char *name = channels.effectiveName(0);
        return meshBuildRadioConfig(region, preset, name, tx_power, freq_slot);
    }

    /**
     * Process a received raw LoRa frame.
     * Mirrors Router.cpp:440-518 — attempts PKI decryption first for unicast
     * DMs addressed to us, then falls back to channel PSK decryption.
     * Auto-learns public keys from NODEINFO_APP broadcasts.
     *
     * Returns true if the packet was successfully parsed and decrypted.
     */
    bool processRx(const uint8_t *raw, size_t raw_len,
                    float rssi, float snr,
                    MeshRxResult *result)
    {
        memset(result, 0, sizeof(MeshRxResult));

        // Parse header
        if (!meshParsePacket(raw, raw_len, &result->packet))
            return false;

        result->packet.rssi = rssi;
        result->packet.snr  = snr;

        uint8_t plaintext[240];
        bool decrypted = false;

        // ── Step 1: Try PKI decryption first (Router.cpp:451-481) ──
        // Conditions: channel_hash==0, unicast to us, sender key known,
        //             payload > 12 bytes overhead
        if (pki.initialized && meshIsPkiCandidate(&result->packet, node_num)) {
            size_t pt_len = meshTryPkiDecrypt(&result->packet, &pki,
                                                &node_keys, plaintext);
            if (pt_len > 0) {
                // Validate protobuf decode
                if (meshDecodeData(plaintext, pt_len, &result->data) &&
                    result->data.portnum != PORT_UNKNOWN)
                {
                    decrypted = true;
                    result->pki_encrypted = true;
                    result->channel_idx = -1; // not a channel-based message
                }
            }
        }

        // ── Step 2: Fall back to channel PSK decryption (Router.cpp:485-517) ──
        if (!decrypted) {
            MeshCryptoKey matched_key;
            int ch_idx = channels.tryDecrypt(
                result->packet.payload, result->packet.payload_len,
                result->packet.channel_hash,
                result->packet.from, result->packet.id,
                plaintext, &matched_key,
                meshValidateData);

            if (ch_idx >= 0) {
                if (meshDecodeData(plaintext, result->packet.payload_len, &result->data)) {
                    decrypted = true;
                    result->channel_idx = ch_idx;
                    result->packet.channel_index = ch_idx;
                }
            }
        }

        if (!decrypted) {
            result->decrypted = false;
            return false;
        }

        result->decrypted = true;

        // ── Auto-learn public keys from NODEINFO broadcasts ──
        meshLearnNodeKey(&result->packet, &result->data, &node_keys);

        return true;
    }

    /**
     * Build a channel-encrypted TX frame (broadcast or group message).
     * Returns total frame length, or 0 on failure.
     */
    size_t buildTx(uint8_t channel_idx,
                    uint32_t to,
                    MeshPortNum portnum,
                    const uint8_t *payload, size_t payload_len,
                    bool want_ack,
                    uint8_t *out)
    {
        if (role == ROLE_CLIENT_MUTE) return 0;
        if (channel_idx >= channels.count) return 0;

        // Encode Data protobuf
        uint8_t data_buf[240];
        size_t data_len = meshEncodeData(data_buf, sizeof(data_buf),
                                          portnum, payload, payload_len);
        if (data_len == 0) return 0;

        // Encrypt with channel PSK
        uint32_t pkt_id = id_gen.next();
        const MeshChannel *ch = &channels.channels[channel_idx];

        if (ch->key.length > 0) {
            if (!meshEncrypt(&ch->key, node_num, pkt_id, data_buf, data_len))
                return 0;
        }

        uint8_t hop_limit = MESH_HOP_RELIABLE;
        return meshBuildPacket(out,
                                to, node_num, pkt_id,
                                hop_limit, hop_limit,
                                want_ack, false,
                                ch->hash,
                                0, 0,
                                data_buf, data_len);
    }

    /**
     * Build a PKI-encrypted DM frame (direct message to specific node).
     * Uses x25519 DH + AES-256-CCM. Requires recipient's public key
     * in node_keys.
     *
     * Returns total frame length, or 0 on failure.
     */
    size_t buildDmTx(uint32_t to_node,
                      MeshPortNum portnum,
                      const uint8_t *payload, size_t payload_len,
                      bool want_ack,
                      uint8_t *out,
                      uint32_t (*rand_fn)(void))
    {
        if (role == ROLE_CLIENT_MUTE) return 0;
        if (!pki.initialized) return 0;

        // Look up recipient's public key
        const uint8_t *their_key = node_keys.getKey(to_node);
        if (!their_key) return 0;

        // Encode Data protobuf
        uint8_t data_buf[240];
        size_t data_len = meshEncodeData(data_buf, sizeof(data_buf),
                                          portnum, payload, payload_len);
        if (data_len == 0) return 0;

        // PKI encrypt (produces ciphertext + 12 bytes overhead)
        uint8_t encrypted[256];
        uint32_t pkt_id = id_gen.next();
        size_t enc_len = meshPkiEncrypt(pki.private_key, their_key,
                                         node_num, pkt_id,
                                         data_buf, data_len,
                                         encrypted, rand_fn);
        if (enc_len == 0) return 0;

        // PKI DMs use channel_hash=0 (Router.cpp:453)
        uint8_t hop_limit = MESH_HOP_RELIABLE;
        return meshBuildPacket(out,
                                to_node, node_num, pkt_id,
                                hop_limit, hop_limit,
                                want_ack, false,
                                0,  // channel_hash = 0 for PKI DMs
                                0, 0,
                                encrypted, enc_len);
    }

    /**
     * Convenience: send a text DM via PKI.
     */
    size_t buildTextDm(uint32_t to_node, const char *text, bool want_ack,
                        uint8_t *out, uint32_t (*rand_fn)(void))
    {
        return buildDmTx(to_node, PORT_TEXT_MESSAGE,
                          (const uint8_t *)text, strlen(text),
                          want_ack, out, rand_fn);
    }

    /**
     * Convenience: build a text broadcast on a channel.
     */
    size_t buildTextTx(uint8_t channel_idx, uint32_t to,
                        const char *text, bool want_ack,
                        uint8_t *out)
    {
        return buildTx(channel_idx, to, PORT_TEXT_MESSAGE,
                        (const uint8_t *)text, strlen(text), want_ack, out);
    }
};
