// SPDX-FileCopyrightText: 2025 Scott Powell / rippleradios.com
// SPDX-FileCopyrightText: 2025 Nicolai Electronics
// SPDX-License-Identifier: MIT
#include "main.h"
#include "mchf_pro_board.h"

#if defined (CONTEXT_LORA) && defined(MESHCORE)

#include "packet.h"
#include <stdint.h>
#include <string.h>

#define member_size(type, member) (sizeof(((type*)0)->member))

// Header byte fields
#define PACKET_HEADER_ROUTE_SHIFT 0
#define PACKET_HEADER_ROUTE_MASK  0x03  // 2-bits
#define PACKET_HEADER_TYPE_SHIFT  2
#define PACKET_HEADER_TYPE_MASK   0x0F  // 4-bits
#define PACKET_HEADER_VER_SHIFT   6
#define PACKET_HEADER_VER_MASK    0x03  // 2-bits

typedef struct __attribute__((packed)) {
    uint8_t  header;
    uint16_t transport_codes[0];
} meshcore_line_header_t;

int meshcore_serialize(const meshcore_message_t* message, uint8_t* out_data, uint8_t* out_size) {
    if (out_data == NULL) {
        return -1;
    }

    memset(out_data, 0, MESHCORE_MAX_TRANS_UNIT);

    if (message->path_length > MESHCORE_MAX_PATH_SIZE || message->payload_length > MESHCORE_MAX_PAYLOAD_SIZE) {
        return -1;
    }

    uint8_t position = 0;

    meshcore_line_header_t* line_header  = (meshcore_line_header_t*)&out_data[position];
    position                            += sizeof(meshcore_line_header_t);

    line_header->header  = (message->route & PACKET_HEADER_ROUTE_MASK) << PACKET_HEADER_ROUTE_SHIFT;
    line_header->header += (message->type & PACKET_HEADER_TYPE_MASK) << PACKET_HEADER_TYPE_SHIFT;
    line_header->header += (message->version & PACKET_HEADER_VER_MASK) << PACKET_HEADER_VER_SHIFT;

    if (message->route == MESHCORE_ROUTE_TYPE_TRANSPORT_FLOOD ||
        message->route == MESHCORE_ROUTE_TYPE_TRANSPORT_DIRECT) {
        // The message has transport codes
        memcpy(&out_data[position], message->transport_codes, member_size(meshcore_line_header_t, transport_codes));
        position += sizeof(uint16_t);
    }

    out_data[position]  = message->path_length;
    position           += sizeof(uint8_t);

    memcpy(&out_data[position], message->path, message->path_length);
    position += message->path_length;

    memcpy(&out_data[position], message->payload, message->payload_length);
    position += message->payload_length;

    *out_size = position;

    return 0;
}

int meshcore_deserialize(uint8_t* data, uint8_t size, meshcore_message_t* out_message) {
    if (out_message == NULL || data == NULL) {
        return -1;
    }

    memset(out_message, 0, sizeof(meshcore_message_t));

    uint8_t position = 0;

    if (size < sizeof(meshcore_line_header_t) || size > MESHCORE_MAX_TRANS_UNIT) {
        return -1;
    }
    meshcore_line_header_t* line_header  = (meshcore_line_header_t*)&data[position];
    position                            += sizeof(meshcore_line_header_t);

    out_message->route   = (line_header->header >> PACKET_HEADER_ROUTE_SHIFT) & PACKET_HEADER_ROUTE_MASK;
    out_message->type    = (line_header->header >> PACKET_HEADER_TYPE_SHIFT) & PACKET_HEADER_TYPE_MASK;
    out_message->version = (line_header->header >> PACKET_HEADER_VER_SHIFT) & PACKET_HEADER_VER_MASK;

    if (out_message->route == MESHCORE_ROUTE_TYPE_TRANSPORT_FLOOD ||
        out_message->route == MESHCORE_ROUTE_TYPE_TRANSPORT_DIRECT) {
        // The message has transport codes
        if (size - position < sizeof(uint16_t)) {
            return -1;
        }
        memcpy(out_message->transport_codes, line_header->transport_codes, sizeof(uint16_t));
        position += sizeof(uint16_t);
    }

    if (size - position < sizeof(uint8_t)) {
        return -1;
    }

    out_message->path_length  = data[position];
    position                 += sizeof(uint8_t);

    uint8_t* path  = &data[position];
    position      += out_message->path_length;

    if (out_message->path_length > MESHCORE_MAX_PATH_SIZE) {
        return -1;
    }

    memcpy(out_message->path, path, out_message->path_length);

    out_message->payload_length = size - position;
    uint8_t* payload            = &data[position];

    if (out_message->payload_length > MESHCORE_MAX_PAYLOAD_SIZE) {
        return -1;
    }

    memcpy(out_message->payload, payload, out_message->payload_length);

    return 0;
}

#endif
