// SPDX-FileCopyrightText: 2025 Scott Powell / rippleradios.com
// SPDX-FileCopyrightText: 2025 Nicolai Electronics
// SPDX-License-Identifier: MIT

#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

// Definitions

typedef struct {
    uint32_t crc;
} meshcore_ack_t;

// Functions

int meshcore_ack_serialize(const meshcore_ack_t* ack, uint8_t* out_payload, uint8_t* out_size);
int meshcore_ack_deserialize(uint8_t* payload, uint8_t size, meshcore_ack_t* out_ack);
