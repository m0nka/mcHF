// SPDX-FileCopyrightText: 2025 Scott Powell / rippleradios.com
// SPDX-FileCopyrightText: 2025 Nicolai Electronics
// SPDX-License-Identifier: MIT
#include "main.h"
#include "mchf_pro_board.h"

#if defined (CONTEXT_LORA) && defined(MESHCORE)

#include "grp_txt.h"
#include <stdint.h>
#include <string.h>
#include "packet.h"

#define member_size(type, member) (sizeof(((type*)0)->member))

int meshcore_grp_txt_serialize(const meshcore_grp_txt_t* grp_txt, uint8_t* out_payload, uint8_t* out_size) {
    if (out_payload == NULL) {
        return -1;
    }

    memset(out_payload, 0, MESHCORE_MAX_PAYLOAD_SIZE);

    uint8_t position = 0;

    memcpy(&out_payload[position], &grp_txt->channel_hash, sizeof(uint8_t));
    position += sizeof(uint8_t);

    memcpy(&out_payload[position], grp_txt->mac, MESHCORE_CIPHER_MAC_SIZE);
    position += MESHCORE_CIPHER_MAC_SIZE;

    memcpy(&out_payload[position], grp_txt->data, grp_txt->data_length);
    position += grp_txt->data_length;

    *out_size = position;

    return 0;
}

int meshcore_grp_txt_deserialize(uint8_t* data, uint8_t size, meshcore_grp_txt_t* out_grp_txt) {
    if (out_grp_txt == NULL || data == NULL) {
        return -1;
    }

    memset(out_grp_txt, 0, sizeof(meshcore_grp_txt_t));

    uint8_t position = 0;

    memcpy(&out_grp_txt->channel_hash, &data[position], sizeof(uint8_t));
    position += sizeof(uint8_t);

    memcpy(out_grp_txt->mac, &data[position], MESHCORE_CIPHER_MAC_SIZE);
    position += MESHCORE_CIPHER_MAC_SIZE;

    out_grp_txt->data_length = size - position;
    memcpy(out_grp_txt->data, &data[position], out_grp_txt->data_length);
    position += out_grp_txt->data_length;

    return 0;
}

#endif
