// SPDX-FileCopyrightText: 2025 Scott Powell / rippleradios.com
// SPDX-FileCopyrightText: 2025 Nicolai Electronics
// SPDX-License-Identifier: MIT

#pragma once

#include <stddef.h>
#include <stdint.h>

// Definitions

#define MESHCORE_MAX_HASH_SIZE        8
#define MESHCORE_PUB_KEY_SIZE         32
#define MESHCORE_PRV_KEY_SIZE         64
#define MESHCORE_SEED_SIZE            32
#define MESHCORE_SIGNATURE_SIZE       64
#define MESHCORE_MAX_ADVERT_DATA_SIZE 32
#define MESHCORE_CIPHER_KEY_SIZE      16
#define MESHCORE_CIPHER_BLOCK_SIZE    16
#define MESHCORE_CIPHER_MAC_SIZE      2
#define MESHCORE_PATH_HASH_SIZE       1
#define MESHCORE_MAX_PAYLOAD_SIZE     184
#define MESHCORE_MAX_PATH_SIZE        64
#define MESHCORE_MAX_TRANS_UNIT       255

typedef enum {
    MESHCORE_PAYLOAD_TYPE_REQ        = 0x0,
    MESHCORE_PAYLOAD_TYPE_RESPONSE   = 0x1,
    MESHCORE_PAYLOAD_TYPE_TXT_MSG    = 0x2,
    MESHCORE_PAYLOAD_TYPE_ACK        = 0x3,
    MESHCORE_PAYLOAD_TYPE_ADVERT     = 0x4,
    MESHCORE_PAYLOAD_TYPE_GRP_TXT    = 0x5,
    MESHCORE_PAYLOAD_TYPE_GRP_DATA   = 0x6,
    MESHCORE_PAYLOAD_TYPE_ANON_REQ   = 0x7,
    MESHCORE_PAYLOAD_TYPE_PATH       = 0x8,
    MESHCORE_PAYLOAD_TYPE_TRACE      = 0x9,
    MESHCORE_PAYLOAD_TYPE_MULTIPART  = 0xA,
    MESHCORE_PAYLOAD_TYPE_RAW_CUSTOM = 0xF,
} meshcore_payload_type_t;

/*
Payload types:

REQ: request (prefixed with dest/src hashes, MAC) (enc data: timestamp, blob)
RESPONSE: response to REQ or ANON_REQ (prefixed with dest/src hashes, MAC) (enc data: timestamp, blob)
TXT_MSG: a plain text message (prefixed with dest/src hashes, MAC) (enc data: timestamp, text)
ACK: a simple ack
ADVERT: a node advertising its Identity
GRP_TXT: an (unverified) group text message (prefixed with channel hash, MAC) (enc data: timestamp, "name: msg")
GRP_DATA: an (unverified) group datagram (prefixed with channel hash, MAC) (enc data: timestamp, blob)
ANON_REQ: generic request (prefixed with dest_hash, ephemeral pub_key, MAC) (enc data: ...)
PATH: returned path (prefixed with dest/src hashes, MAC) (enc data: path, extra)
TRACE: trace a path, collecting SNI for each hop
MULTIPART: packet is one of a set of packets
CUSTOM: custom packet as raw bytes, for applications with custom encryption, payloads, etc
*/

typedef enum {
    MESHCORE_ROUTE_TYPE_TRANSPORT_FLOOD  = 0x0,
    MESHCORE_ROUTE_TYPE_FLOOD            = 0x1,
    MESHCORE_ROUTE_TYPE_DIRECT           = 0x2,
    MESHCORE_ROUTE_TYPE_TRANSPORT_DIRECT = 0x3,
} meshcore_route_type_t;

// TRANSPORT_FLOOD: flood mode + transport codes
// FLOOD: flood mode, needs 'path' to be built up (max 64 bytes)
// DIRECT: direct route, 'path' is supplied
// TRANSPORT_DIRECT: direct route + transport codes

typedef struct {
    meshcore_payload_type_t type;
    meshcore_route_type_t   route;
    uint8_t                 version;
    uint16_t                transport_codes[2];
    uint8_t                 path_length;
    uint8_t                 path[MESHCORE_MAX_PATH_SIZE];
    uint8_t                 payload_length;
    uint8_t                 payload[MESHCORE_MAX_PAYLOAD_SIZE];
} meshcore_message_t;

// Functions

/// Serialize a meshcore_message_t to a binary format for transmission
int meshcore_serialize(const meshcore_message_t* message, uint8_t* out_data, uint8_t* out_size);

/// Deserialize a raw binary message into a meshcore_message_t
int meshcore_deserialize(uint8_t* data, uint8_t size, meshcore_message_t* out_message);
