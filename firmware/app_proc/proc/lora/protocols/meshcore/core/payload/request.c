// SPDX-FileCopyrightText: 2025 Scott Powell / rippleradios.com
// SPDX-FileCopyrightText: 2025 Nicolai Electronics
// SPDX-License-Identifier: MIT
#include "main.h"
#include "mchf_pro_board.h"

#if defined (CONTEXT_LORA) && defined(MESHCORE)

#include "request.h"
#include <stdint.h>
#include <string.h>

#define member_size(type, member) (sizeof(((type*)0)->member))

int meshcore_request_serialize(const meshcore_request_t* request, uint8_t* out_payload, uint8_t* out_size) {
    if (out_payload == NULL) {
        return -1;
    }

    memset(out_payload, 0, MESHCORE_MAX_PAYLOAD_SIZE);

    uint8_t position = 0;

    out_payload[position]  = request->destination_hash;
    position              += sizeof(uint8_t);

    out_payload[position]  = request->source_hash;
    position              += sizeof(uint8_t);

    memcpy(&out_payload[position], request->ciphher_mac, MESHCORE_CIPHER_MAC_SIZE);
    position += MESHCORE_CIPHER_MAC_SIZE;

    if (request->ciphertext_length > MESHCORE_MAX_PAYLOAD_SIZE - MESHCORE_CIPHER_MAC_SIZE - sizeof(uint8_t) * 2) {
        return -1;
    }

    memcpy(&out_payload[position], request->ciphertext, request->ciphertext_length);
    position += request->ciphertext_length;

    *out_size = position;

    return 0;
}

int meshcore_request_deserialize(uint8_t* data, uint8_t size, meshcore_request_t* out_request) {
    if (out_request == NULL || data == NULL) {
        return -1;
    }

    memset(out_request, 0, sizeof(meshcore_request_t));

    uint8_t position = 0;

    out_request->destination_hash  = data[position];
    position                      += sizeof(uint8_t);

    out_request->source_hash  = data[position];
    position                 += sizeof(uint8_t);

    memcpy(out_request->ciphher_mac, &data[position], MESHCORE_CIPHER_MAC_SIZE);
    position += MESHCORE_CIPHER_MAC_SIZE;

    out_request->ciphertext_length = size - position;
    if (out_request->ciphertext_length > MESHCORE_MAX_PAYLOAD_SIZE - MESHCORE_CIPHER_MAC_SIZE - sizeof(uint8_t) * 2) {
        return -1;
    }

    memcpy(out_request->ciphertext, &data[position], out_request->ciphertext_length);
    position += out_request->ciphertext_length;

    return 0;
}

#endif
