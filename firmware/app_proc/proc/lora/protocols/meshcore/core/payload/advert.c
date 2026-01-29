// SPDX-FileCopyrightText: 2025 Scott Powell / rippleradios.com
// SPDX-FileCopyrightText: 2025 Nicolai Electronics
// SPDX-License-Identifier: MIT
#include "main.h"
#include "mchf_pro_board.h"

#if defined (CONTEXT_LORA) && defined(MESHCORE)

#include "advert.h"
#include <stdint.h>
#include <string.h>
#include "packet.h"

#define member_size(type, member) (sizeof(((type*)0)->member))

#define ADVERT_FLAG_HAS_POSITION 0x10
#define ADVERT_FLAG_FEAT1        0x20
#define ADVERT_FLAG_FEAT2        0x40
#define ADVERT_FLAG_NAME         0x80

int meshcore_advert_serialize(const meshcore_advert_t* advert, uint8_t* out_payload, uint8_t* out_size) {
    if (out_payload == NULL) {
        return -1;
    }

    memset(out_payload, 0, MESHCORE_MAX_PAYLOAD_SIZE);

    uint8_t position = 0;

    memcpy(&out_payload[position], advert->pub_key, MESHCORE_PUB_KEY_SIZE);
    position += MESHCORE_PUB_KEY_SIZE;

    memcpy(&out_payload[position], &advert->timestamp, sizeof(uint32_t));
    position += sizeof(uint32_t);

    memcpy(&out_payload[position], advert->signature, MESHCORE_SIGNATURE_SIZE);
    position += MESHCORE_SIGNATURE_SIZE;

    uint8_t flags = (uint8_t)advert->role & 0x0F;
    if (advert->position_valid) {
        flags |= ADVERT_FLAG_HAS_POSITION;
    }
    if (advert->extra1_valid) {
        flags |= ADVERT_FLAG_FEAT1;
    }
    if (advert->extra2_valid) {
        flags |= ADVERT_FLAG_FEAT2;
    }
    if (advert->name_valid) {
        flags |= ADVERT_FLAG_NAME;
    }

    out_payload[position]  = flags;
    position              += sizeof(uint8_t);

    if (advert->position_valid) {
        memcpy(&out_payload[position], &advert->position_lat, sizeof(int32_t));
        position += sizeof(int32_t);
        memcpy(&out_payload[position], &advert->position_lon, sizeof(int32_t));
        position += sizeof(int32_t);
    }

    if (advert->extra1_valid) {
        memcpy(&out_payload[position], &advert->extra1, sizeof(uint16_t));
        position += sizeof(uint16_t);
    }

    if (advert->extra2_valid) {
        memcpy(&out_payload[position], &advert->extra2, sizeof(uint16_t));
        position += sizeof(uint16_t);
    }

    if (advert->name_valid) {
        size_t name_len = strnlen(advert->name, MESHCORE_MAX_NAME_SIZE);
        memcpy(&out_payload[position], advert->name, name_len);
        position += name_len;
    }

    *out_size = position;

    return 0;
}

int meshcore_advert_deserialize(uint8_t* data, uint8_t size, meshcore_advert_t* out_advert) {
    if (out_advert == NULL || data == NULL) {
        return -1;
    }

    memset(out_advert, 0, sizeof(meshcore_advert_t));

    uint8_t position = 0;

    memcpy(out_advert->pub_key, &data[position], MESHCORE_PUB_KEY_SIZE);
    position += MESHCORE_PUB_KEY_SIZE;

    memcpy(&out_advert->timestamp, &data[position], sizeof(uint32_t));
    position += sizeof(uint32_t);

    memcpy(out_advert->signature, &data[position], MESHCORE_SIGNATURE_SIZE);
    position += MESHCORE_SIGNATURE_SIZE;

    uint8_t app_data_len = size - position;

    if (app_data_len > 0) {
        uint8_t flags  = data[position];
        position      += sizeof(uint8_t);

        out_advert->role = (meshcore_device_role_t)(flags & 0x0F);

        if (flags & ADVERT_FLAG_HAS_POSITION) {
            if (size - position < sizeof(int32_t) * 2) {
                return -1;
            }
            memcpy(&out_advert->position_lat, &data[position], sizeof(int32_t));
            position += sizeof(int32_t);
            memcpy(&out_advert->position_lon, &data[position], sizeof(int32_t));
            position += sizeof(int32_t);

            out_advert->position_valid = true;
        }

        if (flags & ADVERT_FLAG_FEAT1) {
            if (size - position < sizeof(uint16_t)) {
                return -1;
            }
            memcpy(&out_advert->extra1, &data[position], sizeof(uint16_t));
            position += sizeof(uint16_t);

            out_advert->extra1_valid = true;
        }

        if (flags & ADVERT_FLAG_FEAT2) {
            if (size - position < sizeof(uint16_t)) {
                return -1;
            }
            memcpy(&out_advert->extra2, &data[position], sizeof(uint16_t));
            position += sizeof(uint16_t);

            out_advert->extra2_valid = true;
        }

        if (flags & ADVERT_FLAG_NAME) {
            uint8_t name_len = size - position;
            if (name_len > MESHCORE_MAX_NAME_SIZE) {
                return -1;
            }
            memcpy(out_advert->name, &data[position], name_len);
            out_advert->name[name_len]  = '\0';  // Null-terminate
            position                   += name_len;

            out_advert->name_valid = true;
        }
    }

    return 0;
}

#endif
