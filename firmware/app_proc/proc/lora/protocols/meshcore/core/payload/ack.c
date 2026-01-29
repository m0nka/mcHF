// SPDX-FileCopyrightText: 2025 Scott Powell / rippleradios.com
// SPDX-FileCopyrightText: 2025 Nicolai Electronics
// SPDX-License-Identifier: MIT
#include "main.h"
#include "mchf_pro_board.h"

#if defined (CONTEXT_LORA) && defined(MESHCORE)

#include "ack.h"
#include <stdint.h>
#include <string.h>
#include "packet.h"

#define member_size(type, member) (sizeof(((type*)0)->member))

int meshcore_ack_serialize(const meshcore_ack_t* ack, uint8_t* out_payload, uint8_t* out_size) {
    if (out_payload == NULL) {
        return -1;
    }

    memset(out_payload, 0, MESHCORE_MAX_PAYLOAD_SIZE);

    uint8_t position = 0;

    memcpy(&out_payload[position], &ack->crc, sizeof(uint32_t));
    position += sizeof(uint32_t);

    *out_size = position;

    return 0;
}

int meshcore_ack_deserialize(uint8_t* data, uint8_t size, meshcore_ack_t* out_ack) {
    if (out_ack == NULL || data == NULL) {
        return -1;
    }

    memset(out_ack, 0, sizeof(meshcore_ack_t));

    uint8_t position = 0;

    memcpy(&out_ack->crc, &data[position], sizeof(uint32_t));
    position += sizeof(uint32_t);

    return 0;
}
#endif
