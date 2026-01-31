/************************************************************************************
**                                                                                 **
**                                 mcHF QRP Transceiver                            **
**                         Krassi Atanassov - M0NKA, 2013-2026                     **
**                                                                                 **
**---------------------------------------------------------------------------------**
**                                                                                 **
**  File name:                                                                     **
**  Description:                                                                   **
**  Last Modified:                                                                 **
**  Licence:			https://github.com/m0nka/mcHF/blob/main/LICENSE            **
************************************************************************************/
#include "main.h"
#include "mchf_pro_board.h"

#if defined (CONTEXT_LORA) && defined(MESHCORE)

#include <inttypes.h>
#include <stdio.h>
#include <string.h>
#include "aes.h"
#include "hmac_sha256.h"
#include "packet.h"
#include "advert.h"
#include "grp_txt.h"

#include "client.h"

#ifdef MESHCORE_UNIT_TEST
unsigned char packet_bin[] = {0x11, 0x00, 0x7e, 0x76, 0x62, 0x67, 0x6f, 0x7f, 0x08, 0x50, 0xa8, 0xa3, 0x55, 0xba, 0xaf,
                              0xbf, 0xc1, 0xeb, 0x7b, 0x41, 0x74, 0xc3, 0x40, 0x44, 0x2d, 0x7d, 0x71, 0x61, 0xc9, 0x47,
                              0x4a, 0x2c, 0x94, 0x00, 0x6c, 0xe7, 0xcf, 0x68, 0x2e, 0x58, 0x40, 0x8d, 0xd8, 0xfc, 0xc5,
                              0x19, 0x06, 0xec, 0xa9, 0x8e, 0xbf, 0x94, 0xa0, 0x37, 0x88, 0x6b, 0xda, 0xde, 0x7e, 0xcd,
                              0x09, 0xfd, 0x92, 0xb8, 0x39, 0x49, 0x1d, 0xf3, 0x80, 0x9c, 0x94, 0x54, 0xf5, 0x28, 0x6d,
                              0x1d, 0x33, 0x70, 0xac, 0x31, 0xa3, 0x45, 0x93, 0xd5, 0x69, 0xe9, 0xa0, 0x42, 0xa3, 0xb4,
                              0x1f, 0xd3, 0x31, 0xdf, 0xfb, 0x7e, 0x18, 0x59, 0x9c, 0xe1, 0xe6, 0x09, 0x92, 0xa0, 0x76,
                              0xd5, 0x02, 0x38, 0xc5, 0xb8, 0xf8, 0x57, 0x57, 0x37, 0x53, 0x54, 0x52, 0x2f, 0x50, 0x75,
                              0x67, 0x65, 0x74, 0x4d, 0x65, 0x73, 0x68, 0x20, 0x43, 0x6f, 0x75, 0x67, 0x61, 0x72};
unsigned int  packet_bin_len = 134;

unsigned char test_message_rx_bin[]   = {0x15, 0x00, 0x11, 0x61, 0x72, 0xfb, 0x24, 0x6c, 0x57, 0x9e, 0x01, 0x0a, 0x18,
                                         0x18, 0x6f, 0xfb, 0x79, 0x3a, 0x18, 0x81, 0xec, 0x7c, 0x2d, 0x64, 0xda, 0x78,
                                         0xbb, 0xaf, 0x0c, 0x05, 0x71, 0xae, 0x29, 0xa1, 0x2d, 0xf5, 0xc4};
unsigned int  test_message_rx_bin_len = 37;

unsigned char advert_bin[] = {
    0x12, 0x00, 0x2b, 0xdc, 0x0e, 0x54, 0x51, 0xb7, 0xdb, 0x3b, 0x2a, 0xc2, 0xc7, 0x8c, 0x8f, 0xdd, 0xaa, 0x76,
    0x38, 0xb6, 0x4a, 0xbf, 0x56, 0xe5, 0xcd, 0xff, 0xdf, 0xd8, 0xaf, 0x09, 0x62, 0x47, 0xe8, 0x4b, 0x50, 0xd7,
    0x0b, 0x69, 0xe9, 0xaf, 0x04, 0xf9, 0x68, 0x15, 0xeb, 0x37, 0xc5, 0x6a, 0x45, 0xb9, 0x81, 0x7e, 0x15, 0x43,
    0x07, 0x50, 0x5d, 0x8e, 0x01, 0x22, 0x47, 0x7e, 0xd8, 0x59, 0x80, 0x0c, 0x9c, 0x19, 0x2b, 0x53, 0x20, 0x60,
    0x8d, 0xf4, 0x86, 0x40, 0x6e, 0x27, 0x2f, 0x2b, 0x7b, 0x87, 0xda, 0x29, 0x9b, 0x79, 0xaf, 0x83, 0xed, 0x46,
    0x5c, 0x8e, 0x19, 0x03, 0x10, 0x50, 0xff, 0x97, 0xf3, 0x24, 0x61, 0x00, 0x81, 0x52, 0x65, 0x6e, 0x7a, 0x65};
unsigned int advert_bin_len = 108;
uint8_t encoded_packet[256] = {0};
#endif

uint8_t key_pb[16] = {0x8b, 0x33, 0x87, 0xe9, 0xc5, 0xcd, 0xea, 0x6a, 0xc9, 0xe5, 0xed, 0xba, 0xa1, 0x15, 0xcd, 0x72};
uint8_t key_d9[16] = {0x9C, 0xD8, 0xFC, 0xF2, 0x2A, 0x47, 0x33, 0x3B, 0x59, 0x1D, 0x96, 0xA2, 0xB8, 0x48, 0xB7, 0x3F};

const char* type_to_string(meshcore_payload_type_t type) {
    switch (type) {
        case MESHCORE_PAYLOAD_TYPE_REQ:
            return "Request";
        case MESHCORE_PAYLOAD_TYPE_RESPONSE:
            return "Response";
        case MESHCORE_PAYLOAD_TYPE_TXT_MSG:
            return "Plain text message";
        case MESHCORE_PAYLOAD_TYPE_ACK:
            return "Acknowledgement";
        case MESHCORE_PAYLOAD_TYPE_ADVERT:
            return "Node advertisement";
        case MESHCORE_PAYLOAD_TYPE_GRP_TXT:
            return "Group text message (unverified)";
        case MESHCORE_PAYLOAD_TYPE_GRP_DATA:
            return "Group data message (unverified)";
        case MESHCORE_PAYLOAD_TYPE_ANON_REQ:
            return "Anonymous request";
        case MESHCORE_PAYLOAD_TYPE_PATH:
            return "Returned path";
        case MESHCORE_PAYLOAD_TYPE_TRACE:
            return "Trace";
        case MESHCORE_PAYLOAD_TYPE_MULTIPART:
            return "Multipart";
        case MESHCORE_PAYLOAD_TYPE_RAW_CUSTOM:
            return "Custom raw";
        default:
            return "UNKNOWN";
    }
}

const char* route_to_string(meshcore_route_type_t route) {
    switch (route) {
        case MESHCORE_ROUTE_TYPE_TRANSPORT_FLOOD:
            return "Transport flood";
        case MESHCORE_ROUTE_TYPE_FLOOD:
            return "Flood";
        case MESHCORE_ROUTE_TYPE_DIRECT:
            return "Direct";
        case MESHCORE_ROUTE_TYPE_TRANSPORT_DIRECT:
            return "Transport direct";
        default:
            return "Unknown";
    }
}

const char* role_to_string(meshcore_device_role_t role) {
    switch (role) {
        case MESHCORE_DEVICE_ROLE_CHAT_NODE:
            return "Chat Node";
        case MESHCORE_DEVICE_ROLE_REPEATER:
            return "Repeater";
        case MESHCORE_DEVICE_ROLE_ROOM_SERVER:
            return "Room Server";
        case MESHCORE_DEVICE_ROLE_SENSOR:
            return "Sensor";
        default:
            return "Unknown";
    }
}

#ifdef MESHCORE_UNIT_TEST
void client_unit_test(void)
{
    printf("Input packet binary data [%d]:\r\n", test_message_rx_bin_len);
    print_hex_array(test_message_rx_bin, test_message_rx_bin_len);

    meshcore_message_t message;
    if (meshcore_deserialize(test_message_rx_bin, test_message_rx_bin_len, &message) >= 0) {
        printf("Decoded message:\r\n");
        printf("Type: %s [%d]\r\n", type_to_string(message.type), message.type);
        printf("Route: %s [%d]\r\n", route_to_string(message.route), message.route);
        printf("Version: %d\r\n", message.version);
        printf("Path Length: %d\r\n", message.path_length);
        printf("Payload Length: %d\r\n", message.payload_length);

        if (message.path_length > 0)
        {
            printf("Path: \r\n");
            print_hex_array(message.path, message.path_length);
        }

        if (message.payload_length > 0)
        {
            printf("Payload [%d]: \r\n", message.payload_length);
            print_hex_array(message.payload, message.payload_length);
        }

        if (message.type == MESHCORE_PAYLOAD_TYPE_ADVERT)
        {
            meshcore_advert_t advert;
            if (meshcore_advert_deserialize(message.payload, message.payload_length, &advert) >= 0)
            {
                printf("Decoded node advertisement:\r\n");
                printf("Public Key: \r\n");
                print_hex_array(advert.pub_key, MESHCORE_PUB_KEY_SIZE);

                printf("Timestamp: %x\r\n", advert.timestamp);
                printf("Signature: \r\n");
                print_hex_array(advert.signature, MESHCORE_SIGNATURE_SIZE);

                printf("Role: %s\r\n", role_to_string(advert.role));
                if (advert.position_valid) {
                    printf("Position: lat=%d, lon=%d\r\n", advert.position_lat, advert.position_lon);
                } else {
                    printf("Position: (not available)\r\n");
                }
                if (advert.extra1_valid) {
                    printf("Extra1: %u\r\n", advert.extra1);
                } else {
                    printf("Extra1: (not available)\r\n");
                }
                if (advert.extra2_valid) {
                    printf("Extra2: %u\r\n", advert.extra2);
                } else {
                    printf("Extra2: (not available)\r\n");
                }
                if (advert.name_valid) {
                    printf("Name: %s\r\n", advert.name);
                } else {
                    printf("Name: (not available)\r\n");
                }

                if (meshcore_advert_serialize(&advert, message.payload, &message.payload_length) < 0) {
                    printf("Failed to serialize node advertisement payload. \r\n");
                    return;
                }

            } else {
                printf("Failed to decode node advertisement payload. \r\n");
                return;
            }
        }
        else if (message.type == MESHCORE_PAYLOAD_TYPE_GRP_TXT)
        {
            meshcore_grp_txt_t grp_txt;
            if (meshcore_grp_txt_deserialize(message.payload, message.payload_length, &grp_txt) >= 0) {
                printf("Decoded group text message:\r\n");
                printf("Channel Hash: %02X\r\n", grp_txt.channel_hash);
                printf("Data Length: %d\r\n", grp_txt.data_length);

                printf("Received MAC: \r\n", grp_txt.data_length);
                print_hex_array(grp_txt.mac, MESHCORE_CIPHER_MAC_SIZE);

                printf("Data [%d]: \r\n", grp_txt.data_length);
                print_hex_array(grp_txt.data, grp_txt.data_length);

                // TO-DO: all of this MAC verification and decryption should be moved somewhere else

                uint8_t out[128];
                size_t  out_len =
                    hmac_sha256(key, sizeof(key), grp_txt.data, grp_txt.data_length, out, MESHCORE_CIPHER_MAC_SIZE);

                printf("Calculated MAC [%d]: \r\n", out_len);
                print_hex_array(out, out_len);

                if (memcmp(out, grp_txt.mac, MESHCORE_CIPHER_MAC_SIZE) == 0)
                {
                    printf("MAC verification: SUCCESS\r\n");

                    // Copy encrypted data to buffer for decryption, AES works in-place
                    grp_txt.decrypted.data_length = grp_txt.data_length;
                    memcpy(grp_txt.decrypted.data, grp_txt.data, grp_txt.data_length);

                    struct AES_ctx ctx;
                    AES_init_ctx(&ctx, key);
                    for (uint8_t i = 0; i < (grp_txt.decrypted.data_length / 16); i++) {
                        AES_ECB_decrypt(&ctx, &grp_txt.decrypted.data[i * 16]);
                    }

                    printf("Data [%d]: \r\n", grp_txt.decrypted.data_length);
                    print_hex_array(grp_txt.decrypted.data, grp_txt.decrypted.data_length);

                    uint8_t position = 0;
                    memcpy(&grp_txt.decrypted.timestamp, grp_txt.decrypted.data, sizeof(uint32_t));
                    position                            += sizeof(uint32_t);
                    grp_txt.decrypted.text_type          = grp_txt.decrypted.data[position];
                    position                            += sizeof(uint8_t);
                    size_t text_length                   = grp_txt.decrypted.data_length - position;
                    grp_txt.decrypted.text               = (char*)&grp_txt.decrypted.data[position];
                    grp_txt.decrypted.text[text_length]  = '\0';

                    printf("Timestamp: %x \r\n", (int)(grp_txt.decrypted.timestamp));
                    printf("Text Type: %d \r\n", grp_txt.decrypted.text_type);
                    printf("Message: '%s' \r\n", grp_txt.decrypted.text);

                } else {
                    printf("MAC verification: FAILURE\r\n");
                }

                if (meshcore_grp_txt_serialize(&grp_txt, message.payload, &message.payload_length) < 0) {
                    printf("Failed to serialize group text message payload.\r\n");
                    return;
                }

            } else {
                printf("Failed to decode group text message payload.\r\n");
                return;
            }
        }
    } else {
        printf("Failed to decode message.\r\n");
        return;
    }

    // Verify
	#if 0
    uint8_t encoded_packet_len = 0;
    if (meshcore_serialize(&message, encoded_packet, &encoded_packet_len) >= 0)
    {
        printf("Serialized packet binary data [%d]: \r\n", encoded_packet_len);
        print_hex_array(encoded_packet, encoded_packet_len);

        if (encoded_packet_len != test_message_rx_bin_len ||
            memcmp(encoded_packet, test_message_rx_bin, test_message_rx_bin_len) != 0)
        {
            printf("Serialized packet does not match original input! \r\n");
            return;
        }
        else
        {
            printf("Serialized packet matches original input. \r\n");
        }
    } else {
        printf("Failed to serialize message. \r\n");
        return;
    }
	#endif
}
#endif

void client_decode(uchar *msg, ushort size, char *notif)
{
	meshcore_message_t message;

	//printf("size: %d \r\n", size);
	//print_hex_array(msg, size);

	if((msg == NULL)||(notif == NULL)||(size == 0))
		return;

	*notif = 0;

    if(meshcore_deserialize(msg, size, &message) >= 0)
    {
        //printf("Decoded message:\r\n");
        printf("Type: %s [%d]\r\n", type_to_string(message.type), message.type);
        printf("Route: %s [%d]\r\n", route_to_string(message.route), message.route);
        printf("Version: %d\r\n", message.version);
        printf("Path Length: %d\r\n", message.path_length);
        printf("Payload Length: %d\r\n", message.payload_length);

		#if 0
        if (message.path_length > 0)
        {
            printf("Path: \r\n");
            print_hex_array(message.path, message.path_length);
        }

        if (message.payload_length > 0)
        {
            printf("Payload [%d]: \r\n", message.payload_length);
            print_hex_array(message.payload, message.payload_length);
        }
		#endif

        if (message.type == MESHCORE_PAYLOAD_TYPE_ADVERT)
        {
            meshcore_advert_t advert;
            if (meshcore_advert_deserialize(message.payload, message.payload_length, &advert) >= 0)
            {
                printf("Decoded node advertisement:\r\n");
                printf("Public Key: \r\n");
                print_hex_array(advert.pub_key, MESHCORE_PUB_KEY_SIZE);

                //printf("Timestamp: %x\r\n", advert.timestamp);
                //printf("Signature: \r\n");
                //print_hex_array(advert.signature, MESHCORE_SIGNATURE_SIZE);

                printf("Role: %s\r\n", role_to_string(advert.role));
                if (advert.position_valid) {
                    printf("Position: lat=%d, lon=%d\r\n", advert.position_lat, advert.position_lon);
                } else {
                    printf("Position: (not available)\r\n");
                }
                if (advert.extra1_valid) {
                    printf("Extra1: %u\r\n", advert.extra1);
                } else {
                    printf("Extra1: (not available)\r\n");
                }
                if (advert.extra2_valid) {
                    printf("Extra2: %u\r\n", advert.extra2);
                } else {
                    printf("Extra2: (not available)\r\n");
                }
                if (advert.name_valid) {
                    printf("Name: %s\r\n", advert.name);

                    strcpy(notif, advert.name);
                }
                else
                {
                    printf("Name: (not available)\r\n");
                }

                //if (meshcore_advert_serialize(&advert, message.payload, &message.payload_length) < 0)
                //{
                 //   printf("Failed to serialize node advertisement payload. \r\n");
                  //  return;
                //}

            }
            else
            {
                printf("Failed to decode node advertisement payload. \r\n");
                return;
            }
        }
        else if (message.type == MESHCORE_PAYLOAD_TYPE_GRP_TXT)
        {
            meshcore_grp_txt_t grp_txt;
            if (meshcore_grp_txt_deserialize(message.payload, message.payload_length, &grp_txt) >= 0) {
                printf("Decoded group text message:\r\n");
                printf("Channel Hash: %02X\r\n", grp_txt.channel_hash);
                printf("Data Length: %d\r\n", grp_txt.data_length);

                //printf("Received MAC: \r\n", grp_txt.data_length);
                //print_hex_array(grp_txt.mac, MESHCORE_CIPHER_MAC_SIZE);

                //printf("Data [%d]: \r\n", grp_txt.data_length);
                //print_hex_array(grp_txt.data, grp_txt.data_length);

                // ToDo: all of this MAC verification and decryption should be moved somewhere else

                uchar *pkey;

                switch(grp_txt.channel_hash)
                {
                	case 0xD9:
                		pkey = key_d9;
                		break;
                	default:
                		pkey = key_pb;
                		break;
                }


                uint8_t out[128];
                size_t  out_len =
                    hmac_sha256(pkey, 16, grp_txt.data, grp_txt.data_length, out, MESHCORE_CIPHER_MAC_SIZE);

                //printf("Calculated MAC [%d]: \r\n", out_len);
                //print_hex_array(out, out_len);

                if (memcmp(out, grp_txt.mac, MESHCORE_CIPHER_MAC_SIZE) == 0)
                {
                    printf("MAC verification: SUCCESS\r\n");

                    // Copy encrypted data to buffer for decryption, AES works in-place
                    grp_txt.decrypted.data_length = grp_txt.data_length;
                    memcpy(grp_txt.decrypted.data, grp_txt.data, grp_txt.data_length);

                    struct AES_ctx ctx;
                    AES_init_ctx(&ctx, pkey);
                    for (uint8_t i = 0; i < (grp_txt.decrypted.data_length / 16); i++) {
                        AES_ECB_decrypt(&ctx, &grp_txt.decrypted.data[i * 16]);
                    }

                    //printf("Data [%d]: \r\n", grp_txt.decrypted.data_length);
                    //print_hex_array(grp_txt.decrypted.data, grp_txt.decrypted.data_length);

                    uint8_t position = 0;
                    memcpy(&grp_txt.decrypted.timestamp, grp_txt.decrypted.data, sizeof(uint32_t));
                    position                            += sizeof(uint32_t);
                    grp_txt.decrypted.text_type          = grp_txt.decrypted.data[position];
                    position                            += sizeof(uint8_t);
                    size_t text_length                   = grp_txt.decrypted.data_length - position;
                    grp_txt.decrypted.text               = (char*)&grp_txt.decrypted.data[position];
                    grp_txt.decrypted.text[text_length]  = '\0';

                    //printf("Timestamp: %x \r\n", (int)(grp_txt.decrypted.timestamp));
                    printf("Text Type: %d \r\n", grp_txt.decrypted.text_type);
                    printf("Message: '%s' \r\n", grp_txt.decrypted.text);

                    strcpy(notif, grp_txt.decrypted.text);

                } else {
                    printf("MAC verification: FAILURE\r\n");
                }

                //if (meshcore_grp_txt_serialize(&grp_txt, message.payload, &message.payload_length) < 0) {
                //    printf("Failed to serialize group text message payload.\r\n");
                //    return;
                //}

            }
            else
            {
                printf("Failed to decode group text message payload.\r\n");
                return;
            }
        }
    } else {
        printf("Failed to decode message.\r\n");
        return;
    }

}

#endif
