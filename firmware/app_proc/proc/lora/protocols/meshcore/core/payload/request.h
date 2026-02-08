// SPDX-FileCopyrightText: 2025 Scott Powell / rippleradios.com
// SPDX-FileCopyrightText: 2025 Nicolai Electronics
// SPDX-License-Identifier: MIT

#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "packet.h"

// Definitions

typedef struct {
    uint8_t destination_hash;
    uint8_t source_hash;
    uint8_t ciphher_mac[MESHCORE_CIPHER_MAC_SIZE];
    uint8_t ciphertext_length;
    uint8_t ciphertext[MESHCORE_MAX_PAYLOAD_SIZE - MESHCORE_CIPHER_MAC_SIZE - sizeof(uint8_t) * 2];
} meshcore_request_t;

// Functions

int meshcore_request_serialize(const meshcore_request_t* request, uint8_t* out_payload, uint8_t* out_size);
int meshcore_request_deserialize(uint8_t* payload, uint8_t size, meshcore_request_t* out_request);
