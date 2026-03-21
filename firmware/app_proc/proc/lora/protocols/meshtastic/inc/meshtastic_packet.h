/**
 * meshtastic_packet.h — On-wire packet header and flag parsing.
 *
 * Extracted from RadioInterface.h:30-68 and MeshTypes.h.
 * The PacketHeader is sent as raw bytes (NOT protobuf), 16 bytes total.
 *
 * Part of meshtastic-lite.
 */
#pragma once

#include <stdint.h>
#include <string.h>

// ─── Flag Masks (RadioInterface.h:24-28) ───────────────────────────────────────

#define MESH_FLAGS_HOP_LIMIT_MASK   0x07
#define MESH_FLAGS_WANT_ACK_MASK    0x08
#define MESH_FLAGS_VIA_MQTT_MASK    0x10
#define MESH_FLAGS_HOP_START_MASK   0xE0
#define MESH_FLAGS_HOP_START_SHIFT  5

// ─── Special Addresses ─────────────────────────────────────────────────────────

#define MESH_ADDR_BROADCAST  0xFFFFFFFF

// ─── Packet Header (16 bytes, matches RadioInterface.h:34-54) ──────────────────

/**
 * On-wire packet header. Packed, little-endian.
 * This is NOT a protobuf — it is raw bytes at the start of every LoRa frame.
 *
 * Layout (16 bytes):
 *   [0..3]   to          destination NodeNum (LE)
 *   [4..7]   from        source NodeNum (LE)
 *   [8..11]  id          packet ID (LE)
 *   [12]     flags       hop_limit[2:0], want_ack[3], via_mqtt[4], hop_start[7:5]
 *   [13]     channel     channel hash (XOR of name + PSK bytes)
 *   [14]     next_hop    last byte of next-hop NodeNum
 *   [15]     relay_node  last byte of relaying NodeNum
 */
typedef struct __attribute__((packed)) {
    uint32_t to;
    uint32_t from;
    uint32_t id;
    uint8_t  flags;
    uint8_t  channel;
    uint8_t  next_hop;
    uint8_t  relay_node;
} MeshPacketHeader;

static_assert(sizeof(MeshPacketHeader) == 16, "PacketHeader must be exactly 16 bytes");

// ─── Decoded Packet ────────────────────────────────────────────────────────────

/**
 * A received Meshtastic packet after header parsing and (optionally) decryption.
 */
struct MeshRxPacket {
    // Header fields
    uint32_t to;
    uint32_t from;
    uint32_t id;
    uint8_t  hop_limit;
    uint8_t  hop_start;
    bool     want_ack;
    bool     via_mqtt;
    uint8_t  channel_hash;
    uint8_t  next_hop;
    uint8_t  relay_node;

    // Payload (after header, before decryption this is ciphertext)
    uint8_t  payload[255 - 16];
    size_t   payload_len;

    // Radio metadata
    float    rssi;
    float    snr;

    // Set after successful decryption
    int8_t   channel_index;  // -1 if not yet matched
};

/**
 * Parse raw LoRa frame bytes into a MeshRxPacket.
 * Returns false if frame is too short to contain a valid header.
 */
static inline bool meshParsePacket(const uint8_t *raw, size_t len, MeshRxPacket *pkt) {
    if (len < sizeof(MeshPacketHeader))
        return false;

    const MeshPacketHeader *hdr = (const MeshPacketHeader *)raw;

    pkt->to           = hdr->to;
    pkt->from         = hdr->from;
    pkt->id           = hdr->id;
    pkt->hop_limit    = hdr->flags & MESH_FLAGS_HOP_LIMIT_MASK;
    pkt->want_ack     = (hdr->flags & MESH_FLAGS_WANT_ACK_MASK) != 0;
    pkt->via_mqtt     = (hdr->flags & MESH_FLAGS_VIA_MQTT_MASK) != 0;
    pkt->hop_start    = (hdr->flags & MESH_FLAGS_HOP_START_MASK) >> MESH_FLAGS_HOP_START_SHIFT;
    pkt->channel_hash = hdr->channel;
    pkt->next_hop     = hdr->next_hop;
    pkt->relay_node   = hdr->relay_node;

    pkt->payload_len  = len - sizeof(MeshPacketHeader);
    if (pkt->payload_len > sizeof(pkt->payload))
        pkt->payload_len = sizeof(pkt->payload);
    memcpy(pkt->payload, raw + sizeof(MeshPacketHeader), pkt->payload_len);

    pkt->rssi = 0;
    pkt->snr  = 0;
    pkt->channel_index = -1;

    return true;
}

/**
 * Build a raw LoRa frame from components.
 * Returns total frame size (header + payload_len).
 * `out` must be at least 16 + payload_len bytes.
 */
static inline size_t meshBuildPacket(uint8_t *out,
                                      uint32_t to, uint32_t from, uint32_t id,
                                      uint8_t hop_limit, uint8_t hop_start,
                                      bool want_ack, bool via_mqtt,
                                      uint8_t channel_hash,
                                      uint8_t next_hop, uint8_t relay_node,
                                      const uint8_t *payload, size_t payload_len)
{
    MeshPacketHeader *hdr = (MeshPacketHeader *)out;
    hdr->to         = to;
    hdr->from       = from;
    hdr->id         = id;
    hdr->flags      = (hop_limit & MESH_FLAGS_HOP_LIMIT_MASK)
                    | (want_ack ? MESH_FLAGS_WANT_ACK_MASK : 0)
                    | (via_mqtt ? MESH_FLAGS_VIA_MQTT_MASK : 0)
                    | ((hop_start << MESH_FLAGS_HOP_START_SHIFT) & MESH_FLAGS_HOP_START_MASK);
    hdr->channel    = channel_hash;
    hdr->next_hop   = next_hop;
    hdr->relay_node = relay_node;

    memcpy(out + sizeof(MeshPacketHeader), payload, payload_len);
    return sizeof(MeshPacketHeader) + payload_len;
}
