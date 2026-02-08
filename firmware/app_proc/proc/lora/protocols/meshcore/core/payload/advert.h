// SPDX-FileCopyrightText: 2025 Scott Powell / rippleradios.com
// SPDX-FileCopyrightText: 2025 Nicolai Electronics
// SPDX-License-Identifier: MIT

#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

// Definitions

#define MESHCORE_PUB_KEY_SIZE         32
#define MESHCORE_SIGNATURE_SIZE       64
#define MESHCORE_MAX_ADVERT_DATA_SIZE 32
#define MESHCORE_MAX_NAME_SIZE        32

typedef enum {
    MESHCORE_DEVICE_ROLE_UNKNOWN     = 0,
    MESHCORE_DEVICE_ROLE_CHAT_NODE   = 1,
    MESHCORE_DEVICE_ROLE_REPEATER    = 2,
    MESHCORE_DEVICE_ROLE_ROOM_SERVER = 3,
    MESHCORE_DEVICE_ROLE_SENSOR      = 4,
} meshcore_device_role_t;

typedef struct {
    uint8_t                pub_key[MESHCORE_PUB_KEY_SIZE];
    uint32_t               timestamp;
    uint8_t                signature[MESHCORE_SIGNATURE_SIZE];
    meshcore_device_role_t role;
    bool                   name_valid;
    char                   name[MESHCORE_MAX_NAME_SIZE + sizeof('\0')];
    bool                   position_valid;
    int32_t                position_lat;
    int32_t                position_lon;
    bool                   extra1_valid;
    uint16_t               extra1;
    bool                   extra2_valid;
    uint16_t               extra2;
} meshcore_advert_t;

// Functions

int meshcore_advert_serialize(const meshcore_advert_t* advert, uint8_t* out_payload, uint8_t* out_size);
int meshcore_advert_deserialize(uint8_t* payload, uint8_t size, meshcore_advert_t* out_advert);
