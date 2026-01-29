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
    uint8_t  data_length;
    uint8_t  data[MESHCORE_MAX_PAYLOAD_SIZE - sizeof(uint8_t) - MESHCORE_CIPHER_MAC_SIZE];
    uint32_t timestamp;
    uint8_t  text_type;
    char*    text;
} meshcore_grp_txt_decrypted_t;

typedef struct {
    uint8_t                      channel_hash;
    uint8_t                      mac[MESHCORE_CIPHER_MAC_SIZE];
    uint8_t                      data_length;
    uint8_t                      data[MESHCORE_MAX_PAYLOAD_SIZE - sizeof(uint8_t) - MESHCORE_CIPHER_MAC_SIZE];
    meshcore_grp_txt_decrypted_t decrypted;
} meshcore_grp_txt_t;

// Functions

int meshcore_grp_txt_serialize(const meshcore_grp_txt_t* grp_text, uint8_t* out_payload, uint8_t* out_size);
int meshcore_grp_txt_deserialize(uint8_t* payload, uint8_t size, meshcore_grp_txt_t* out_grp_text);
