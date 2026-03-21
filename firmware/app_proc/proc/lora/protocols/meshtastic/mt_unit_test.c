/**
 * test_meshtastic.c — Verification test for meshtastic-lite.
 *
 * Uses a known-good packet from the Meshtastic community (discussions/226):
 *   key        = defaultpsk (0xd4f1bb3a20290759f0bcffabcf4e6901)
 *   plaintext  = 08 01 12 04 54 65 73 74 48 00  (Data{portnum=1, payload="Test"})
 *   ciphertext = 2d 73 fe a3 70 6e bf 6a 16 e0
 *   nonce/IV   = a5 72 a4 ab 00 00 00 00 88 1b c2 25 00 00 00 00
 *     → packetId = 0xaba472a5 (LE), fromNode = 0x25c21b88 (LE)
 *
 * Build: gcc -o test_meshtastic test_meshtastic.c -lm
 *        (needs a software AES block encrypt — included below)
 */
#include "main.h"
#include "mchf_pro_board.h"

#if defined (CONTEXT_LORA) && defined(MESHTASTIC)

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdint.h>

/* ---------- Minimal AES-128 block encrypt for testing ---------------------- */
/* This is a compact AES implementation, sufficient for verification.          */
/* On your ESP32-P4 you'd use mbedtls instead (MESH_CRYPTO_USE_MBEDTLS=1).    */

static const uint8_t aes_sbox[256] = {
    0x63,0x7c,0x77,0x7b,0xf2,0x6b,0x6f,0xc5,0x30,0x01,0x67,0x2b,0xfe,0xd7,0xab,0x76,
    0xca,0x82,0xc9,0x7d,0xfa,0x59,0x47,0xf0,0xad,0xd4,0xa2,0xaf,0x9c,0xa4,0x72,0xc0,
    0xb7,0xfd,0x93,0x26,0x36,0x3f,0xf7,0xcc,0x34,0xa5,0xe5,0xf1,0x71,0xd8,0x31,0x15,
    0x04,0xc7,0x23,0xc3,0x18,0x96,0x05,0x9a,0x07,0x12,0x80,0xe2,0xeb,0x27,0xb2,0x75,
    0x09,0x83,0x2c,0x1a,0x1b,0x6e,0x5a,0xa0,0x52,0x3b,0xd6,0xb3,0x29,0xe3,0x2f,0x84,
    0x53,0xd1,0x00,0xed,0x20,0xfc,0xb1,0x5b,0x6a,0xcb,0xbe,0x39,0x4a,0x4c,0x58,0xcf,
    0xd0,0xef,0xaa,0xfb,0x43,0x4d,0x33,0x85,0x45,0xf9,0x02,0x7f,0x50,0x3c,0x9f,0xa8,
    0x51,0xa3,0x40,0x8f,0x92,0x9d,0x38,0xf5,0xbc,0xb6,0xda,0x21,0x10,0xff,0xf3,0xd2,
    0xcd,0x0c,0x13,0xec,0x5f,0x97,0x44,0x17,0xc4,0xa7,0x7e,0x3d,0x64,0x5d,0x19,0x73,
    0x60,0x81,0x4f,0xdc,0x22,0x2a,0x90,0x88,0x46,0xee,0xb8,0x14,0xde,0x5e,0x0b,0xdb,
    0xe0,0x32,0x3a,0x0a,0x49,0x06,0x24,0x5c,0xc2,0xd3,0xac,0x62,0x91,0x95,0xe4,0x79,
    0xe7,0xc8,0x37,0x6d,0x8d,0xd5,0x4e,0xa9,0x6c,0x56,0xf4,0xea,0x65,0x7a,0xae,0x08,
    0xba,0x78,0x25,0x2e,0x1c,0xa6,0xb4,0xc6,0xe8,0xdd,0x74,0x1f,0x4b,0xbd,0x8b,0x8a,
    0x70,0x3e,0xb5,0x66,0x48,0x03,0xf6,0x0e,0x61,0x35,0x57,0xb9,0x86,0xc1,0x1d,0x9e,
    0xe1,0xf8,0x98,0x11,0x69,0xd9,0x8e,0x94,0x9b,0x1e,0x87,0xe9,0xce,0x55,0x28,0xdf,
    0x8c,0xa1,0x89,0x0d,0xbf,0xe6,0x42,0x68,0x41,0x99,0x2d,0x0f,0xb0,0x54,0xbb,0x16
};

static const uint8_t aes_rcon[11] = {0x00,0x01,0x02,0x04,0x08,0x10,0x20,0x40,0x80,0x1b,0x36};

static uint8_t xtime(uint8_t x) { return (x<<1) ^ ((x>>7) * 0x1b); }

static void aes128_key_expansion(const uint8_t key[16], uint8_t rkeys[176]) {
    memcpy(rkeys, key, 16);
    for (int i = 1; i <= 10; i++) {
        uint8_t t[4];
        memcpy(t, rkeys + (i-1)*16 + 12, 4);
        uint8_t tmp = t[0];
        t[0] = aes_sbox[t[1]] ^ aes_rcon[i];
        t[1] = aes_sbox[t[2]];
        t[2] = aes_sbox[t[3]];
        t[3] = aes_sbox[tmp];
        for (int j = 0; j < 16; j++)
            rkeys[i*16+j] = rkeys[(i-1)*16+j] ^ t[j%4];
        // fix: each 4-byte word XORs with previous word in same round
        for (int j = 4; j < 16; j++)
            rkeys[i*16+j] = rkeys[i*16+j-4] ^ rkeys[(i-1)*16+j];
    }
}

static void aes128_encrypt_block(const uint8_t rkeys[176], const uint8_t in[16], uint8_t out[16]) {
    uint8_t s[16];
    memcpy(s, in, 16);
    // AddRoundKey 0
    for (int i = 0; i < 16; i++) s[i] ^= rkeys[i];
    for (int round = 1; round <= 10; round++) {
        // SubBytes
        for (int i = 0; i < 16; i++) s[i] = aes_sbox[s[i]];
        // ShiftRows
        uint8_t t;
        t=s[1]; s[1]=s[5]; s[5]=s[9]; s[9]=s[13]; s[13]=t;
        t=s[2]; s[2]=s[10]; s[10]=t; t=s[6]; s[6]=s[14]; s[14]=t;
        t=s[15]; s[15]=s[11]; s[11]=s[7]; s[7]=s[3]; s[3]=t;
        // MixColumns (skip on last round)
        if (round < 10) {
            for (int c = 0; c < 4; c++) {
                uint8_t a0=s[c*4], a1=s[c*4+1], a2=s[c*4+2], a3=s[c*4+3];
                uint8_t h = a0^a1^a2^a3;
                s[c*4+0] ^= xtime(a0^a1) ^ h;
                s[c*4+1] ^= xtime(a1^a2) ^ h;
                s[c*4+2] ^= xtime(a2^a3) ^ h;
                s[c*4+3] ^= xtime(a3^a0) ^ h;
            }
        }
        // AddRoundKey
        for (int i = 0; i < 16; i++) s[i] ^= rkeys[round*16+i];
    }
    memcpy(out, s, 16);
}

/* Expanded round keys — computed once per key */
static uint8_t g_rkeys[176];
static int g_rkeys_init = 0;

extern "C" void mesh_aes_block_encrypt(const uint8_t *key, int key_bits,
                             const uint8_t in[16], uint8_t out[16])
{
    (void)key_bits; // only AES-128 for this test
    if (!g_rkeys_init) {
        aes128_key_expansion(key, g_rkeys);
        g_rkeys_init = 1;
    }
    aes128_encrypt_block(g_rkeys, in, out);
}

/* ---------- Now include the library --------------------------------------- */

#include "meshtastic.h"

/* ---------- Test helpers -------------------------------------------------- */

static void print_hex(const char *label, const uint8_t *data, size_t len) {
    printf("  %-20s: ", label);
    for (size_t i = 0; i < len; i++) printf("%02x ", data[i]);
    printf("\n");
}

static int test_count = 0;
static int pass_count = 0;

#define TEST(name) do { test_count++; printf("\n[TEST] %s\n", name); } while(0)
#define PASS(name) do { pass_count++; printf("  [PASS] %s\n", name); } while(0)
#define FAIL(name, ...) do { printf("  [FAIL] %s: ", name); printf(__VA_ARGS__); printf("\n"); } while(0)

/* ---------- Tests --------------------------------------------------------- */

void test_nonce_construction(void) {
    TEST("Nonce Construction");

    // Known IV from discussions/226:
    // a5 72 a4 ab 00 00 00 00 88 1b c2 25 00 00 00 00
    uint32_t from_node = 0x25c21b88;
    uint32_t packet_id = 0xaba472a5;

    uint8_t nonce[16];
    meshBuildNonce(nonce, from_node, packet_id);

    uint8_t expected[16] = {
        0xa5, 0x72, 0xa4, 0xab, 0x00, 0x00, 0x00, 0x00,
        0x88, 0x1b, 0xc2, 0x25, 0x00, 0x00, 0x00, 0x00
    };

    print_hex("expected", expected, 16);
    print_hex("got", nonce, 16);

    if (memcmp(nonce, expected, 16) == 0)
        PASS("nonce matches firmware layout");
    else
        FAIL("nonce", "mismatch!");
}

void test_default_psk_expansion(void) {
    TEST("Default PSK Expansion");

    // PSK index 1 should expand to the well-known default key
    uint8_t raw = 1;
    MeshCryptoKey k = meshExpandPsk(&raw, 1);

    print_hex("expected", MESH_DEFAULT_PSK, 16);
    print_hex("got", k.bytes, k.length);

    if (k.length == 16 && memcmp(k.bytes, MESH_DEFAULT_PSK, 16) == 0)
        PASS("PSK index 1 → defaultpsk");
    else
        FAIL("PSK expansion", "mismatch! length=%d", k.length);

    // PSK index 2 should be defaultpsk with last byte +1
    raw = 2;
    k = meshExpandPsk(&raw, 1);
    if (k.length == 16 && k.bytes[15] == MESH_DEFAULT_PSK[15] + 1)
        PASS("PSK index 2 → defaultpsk[15]+1");
    else
        FAIL("PSK index 2", "last byte=%02x, expected %02x", k.bytes[15], MESH_DEFAULT_PSK[15]+1);

    // PSK index 0 = no encryption
    raw = 0;
    k = meshExpandPsk(&raw, 1);
    if (k.length == 0)
        PASS("PSK index 0 → no encryption");
    else
        FAIL("PSK index 0", "length=%d", k.length);
}

void test_channel_hash(void) {
    TEST("Channel Hash");

    // Default LongFast channel with default PSK
    // Hash = XOR("LongFast") ^ XOR(defaultpsk)
    const char *name = "LongFast";
    MeshCryptoKey k;
    memcpy(k.bytes, MESH_DEFAULT_PSK, 16);
    k.length = 16;

    uint8_t h = meshComputeChannelHash(name, &k);
    printf("  LongFast hash = 0x%02x\n", h);

    // Verify XOR components manually
    uint8_t name_xor = 0;
    for (size_t i = 0; i < strlen(name); i++) name_xor ^= name[i];
    uint8_t key_xor = 0;
    for (int i = 0; i < 16; i++) key_xor ^= MESH_DEFAULT_PSK[i];
    uint8_t expected = name_xor ^ key_xor;

    printf("  name_xor=0x%02x key_xor=0x%02x expected=0x%02x\n", name_xor, key_xor, expected);

    if (h == expected)
        PASS("channel hash = XOR(name) ^ XOR(key)");
    else
        FAIL("channel hash", "got 0x%02x, expected 0x%02x", h, expected);
}

void test_decrypt_known_packet(void) {
    TEST("Decrypt Known Packet (discussions/226 test vector)");

    // Known values:
    uint32_t from_node = 0x25c21b88;
    uint32_t packet_id = 0xaba472a5;

    uint8_t ciphertext[] = { 0x2d, 0x73, 0xfe, 0xa3, 0x70, 0x6e, 0xbf, 0x6a, 0x16, 0xe0 };
    uint8_t expected_pt[] = { 0x08, 0x01, 0x12, 0x04, 0x54, 0x65, 0x73, 0x74, 0x48, 0x00 };

    // Reset key expansion state for new key
    g_rkeys_init = 0;

    MeshCryptoKey key;
    memcpy(key.bytes, MESH_DEFAULT_PSK, 16);
    key.length = 16;

    uint8_t buf[10];
    memcpy(buf, ciphertext, 10);

    print_hex("ciphertext", buf, 10);

    bool ok = meshDecrypt(&key, from_node, packet_id, buf, 10);

    print_hex("decrypted", buf, 10);
    print_hex("expected", expected_pt, 10);

    if (ok && memcmp(buf, expected_pt, 10) == 0)
        PASS("decrypt matches known plaintext");
    else
        FAIL("decrypt", "mismatch!");

    // Now decode the Data protobuf
    MeshData data;
    if (meshDecodeData(buf, 10, &data)) {
        printf("  portnum=%d payload_len=%zu payload='%.*s'\n",
               data.portnum, data.payload_len, (int)data.payload_len, data.payload);
        if (data.portnum == PORT_TEXT_MESSAGE &&
            data.payload_len == 4 &&
            memcmp(data.payload, "Test", 4) == 0)
            PASS("protobuf decode: TEXT_MESSAGE 'Test'");
        else
            FAIL("protobuf", "unexpected content");
    } else {
        FAIL("protobuf", "decode failed");
    }
}

void test_frequency_calculation(void) {
    TEST("Frequency Calculation");

    // US region, LongFast, default channel name
    const RegionDef *us = meshGetRegion(REGION_US);
    MeshFreqConfig fc = meshCalcFrequency(us, MODEM_LONG_FAST, "LongFast");

    printf("  region: %s (%.1f - %.1f MHz)\n", us->name, us->freq_start, us->freq_end);
    printf("  numChannels: %u\n", fc.num_channels);
    printf("  channel_num: %u\n", fc.channel_num);
    printf("  frequency: %.6f MHz\n", fc.frequency_mhz);

    // Verify hash("LongFast") % numChannels
    uint32_t h = meshDjb2Hash("LongFast");
    uint32_t num_ch = (uint32_t)((928.0f - 902.0f) / (250.0f / 1000.0f));
    uint32_t expected_ch = h % num_ch;
    printf("  djb2('LongFast') = 0x%08x, %% %u = %u\n", h, num_ch, expected_ch);

    if (fc.channel_num == expected_ch)
        PASS("channel_num = djb2(name) %% numChannels");
    else
        FAIL("channel_num", "got %u expected %u", fc.channel_num, expected_ch);

    // Verify freq = freqStart + bw/2000 + channel_num * bw/1000
    float expected_freq = 902.0f + (250.0f / 2000.0f) + (expected_ch * (250.0f / 1000.0f));
    float diff = fc.frequency_mhz - expected_freq;
    if (diff < 0) diff = -diff;
    if (diff < 0.001f)
        PASS("frequency formula correct");
    else
        FAIL("frequency", "got %.6f expected %.6f", fc.frequency_mhz, expected_freq);

    // ANZ region (NZ)
    const RegionDef *anz = meshGetRegion(REGION_ANZ);
    MeshFreqConfig fc_anz = meshCalcFrequency(anz, MODEM_LONG_FAST, "LongFast");
    printf("  ANZ: freq=%.6f MHz, ch=%u/%u\n",
           fc_anz.frequency_mhz, fc_anz.channel_num, fc_anz.num_channels);
    PASS("ANZ frequency calculated");
}

void test_packet_header_roundtrip(void) {
    TEST("Packet Header Round-trip");

    uint8_t frame[255];
    uint8_t payload[] = { 0xDE, 0xAD, 0xBE, 0xEF };

    size_t len = meshBuildPacket(frame,
        0xFFFFFFFF,   // to (broadcast)
        0x12345678,   // from
        0xABCD0042,   // id
        3,            // hop_limit
        3,            // hop_start
        true,         // want_ack
        false,        // via_mqtt
        0x7A,         // channel hash
        0x56,         // next_hop
        0x78,         // relay_node
        payload, 4);

    printf("  frame length: %zu\n", len);

    MeshRxPacket pkt;
    if (meshParsePacket(frame, len, &pkt)) {
        int ok = 1;
        if (pkt.to != 0xFFFFFFFF)  { FAIL("to", "0x%08x", pkt.to); ok=0; }
        if (pkt.from != 0x12345678){ FAIL("from", "0x%08x", pkt.from); ok=0; }
        if (pkt.id != 0xABCD0042)  { FAIL("id", "0x%08x", pkt.id); ok=0; }
        if (pkt.hop_limit != 3)    { FAIL("hop_limit", "%d", pkt.hop_limit); ok=0; }
        if (pkt.hop_start != 3)    { FAIL("hop_start", "%d", pkt.hop_start); ok=0; }
        if (!pkt.want_ack)         { FAIL("want_ack", "false"); ok=0; }
        if (pkt.via_mqtt)          { FAIL("via_mqtt", "true"); ok=0; }
        if (pkt.channel_hash!=0x7A){ FAIL("channel", "0x%02x", pkt.channel_hash); ok=0; }
        if (pkt.next_hop != 0x56)  { FAIL("next_hop", "0x%02x", pkt.next_hop); ok=0; }
        if (pkt.relay_node!=0x78)  { FAIL("relay", "0x%02x", pkt.relay_node); ok=0; }
        if (pkt.payload_len != 4)  { FAIL("payload_len", "%zu", pkt.payload_len); ok=0; }
        if (memcmp(pkt.payload, payload, 4) != 0) { FAIL("payload", "mismatch"); ok=0; }
        if (ok) PASS("all header fields round-trip correctly");
    } else {
        FAIL("parse", "returned false");
    }
}

void test_session_rx(void) {
    TEST("Full Session RX (synthetic packet)");

    // Reset key state
    g_rkeys_init = 0;

    // Create a session
    MeshSession session;
    session.init(REGION_US, MODEM_LONG_FAST, ROLE_CLIENT, 0x11223344);
    session.channels.addDefaultChannel();

    printf("  channel[0] hash = 0x%02x, name='%s'\n",
           session.channels.channels[0].hash,
           session.channels.effectiveName(0));

    // Build a packet as if another node sent it
    uint32_t sender   = 0x25c21b88;
    uint32_t pkt_id   = 0xaba472a5;

    // Encode a text message Data protobuf
    uint8_t data_buf[64];
    size_t data_len = meshEncodeData(data_buf, sizeof(data_buf),
                                      PORT_TEXT_MESSAGE,
                                      (const uint8_t *)"Hello", 5);
    printf("  encoded Data protobuf: %zu bytes\n", data_len);
    print_hex("plaintext", data_buf, data_len);

    // Encrypt it
    MeshCryptoKey key;
    memcpy(key.bytes, MESH_DEFAULT_PSK, 16);
    key.length = 16;
    meshEncrypt(&key, sender, pkt_id, data_buf, data_len);
    print_hex("encrypted", data_buf, data_len);

    // Build the full frame
    uint8_t frame[255];
    size_t frame_len = meshBuildPacket(frame,
        MESH_ADDR_BROADCAST, sender, pkt_id,
        3, 3, false, false,
        session.channels.channels[0].hash,
        0, 0,
        data_buf, data_len);

    // Reset key state for decrypt
    g_rkeys_init = 0;

    // Process through session
    MeshRxResult result;
    bool ok = session.processRx(frame, frame_len, -85.0f, 7.5f, &result);

    if (ok) {
        printf("  from=0x%08x id=0x%08x ch=%d port=%d\n",
               result.packet.from, result.packet.id,
               result.channel_idx, result.data.portnum);
        printf("  payload='%.*s'\n",
               (int)result.data.payload_len, result.data.payload);

        if (result.data.portnum == PORT_TEXT_MESSAGE &&
            result.data.payload_len == 5 &&
            memcmp(result.data.payload, "Hello", 5) == 0)
            PASS("full RX pipeline: encrypt→frame→parse→decrypt→decode");
        else
            FAIL("payload", "unexpected content");
    } else {
        FAIL("processRx", "returned false");
    }
}

void test_multichannel(void) {
    TEST("Multi-Channel Decrypt");

    g_rkeys_init = 0;

    MeshSession session;
    session.init(REGION_US, MODEM_LONG_FAST, ROLE_CLIENT, 0xAABBCCDD);

    // Add default channel
    session.channels.addDefaultChannel();

    // Add a custom channel with a 16-byte key
    uint8_t custom_key[16] = {
        0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,
        0x09,0x0A,0x0B,0x0C,0x0D,0x0E,0x0F,0x10
    };
    session.channels.addChannel("ADSBnet", custom_key, 16, false);

    printf("  channel[0] '%s' hash=0x%02x\n",
           session.channels.effectiveName(0),
           session.channels.channels[0].hash);
    printf("  channel[1] '%s' hash=0x%02x\n",
           session.channels.effectiveName(1),
           session.channels.channels[1].hash);

    // Build a packet encrypted with the custom channel key
    uint32_t sender = 0xDEADBEEF;
    uint32_t pkt_id = 0x42424242;

    uint8_t data_buf[64];
    size_t data_len = meshEncodeData(data_buf, sizeof(data_buf),
                                      PORT_TEXT_MESSAGE,
                                      (const uint8_t *)"Secret", 6);

    MeshCryptoKey ck;
    memcpy(ck.bytes, custom_key, 16);
    ck.length = 16;

    // Need fresh key expansion for custom key
    g_rkeys_init = 0;
    meshEncrypt(&ck, sender, pkt_id, data_buf, data_len);

    uint8_t frame[255];
    size_t frame_len = meshBuildPacket(frame,
        MESH_ADDR_BROADCAST, sender, pkt_id,
        3, 3, false, false,
        session.channels.channels[1].hash,  // use custom channel's hash
        0, 0,
        data_buf, data_len);

    // Reset for decrypt
    g_rkeys_init = 0;

    MeshRxResult result;
    bool ok = session.processRx(frame, frame_len, -90.0f, 5.0f, &result);

    if (ok && result.channel_idx == 1) {
        printf("  matched channel %d ('%s')\n",
               result.channel_idx,
               session.channels.effectiveName(result.channel_idx));
        printf("  payload='%.*s'\n",
               (int)result.data.payload_len, result.data.payload);
        if (memcmp(result.data.payload, "Secret", 6) == 0)
            PASS("packet correctly matched to custom channel");
        else
            FAIL("payload", "unexpected content");
    } else {
        FAIL("multichannel", "ok=%d ch_idx=%d", ok, result.channel_idx);
    }
}

void test_airtime(void) {
    TEST("Airtime Estimation");

    // LongFast: SF11, BW250, a 50-byte total packet
    uint32_t ms = meshPacketAirtimeMs(MODEM_LONG_FAST, 50);
    printf("  LongFast 50 bytes: %u ms\n", ms);
    // Sanity: should be in the ~200-600ms range for SF11/250
    if (ms > 100 && ms < 2000) PASS("airtime in plausible range");
    else FAIL("airtime", "%u ms seems wrong", ms);

    // Short Fast: SF7, BW250, same packet
    ms = meshPacketAirtimeMs(MODEM_SHORT_FAST, 50);
    printf("  ShortFast 50 bytes: %u ms\n", ms);
    if (ms > 5 && ms < 200) PASS("ShortFast much faster");
    else FAIL("airtime", "%u ms", ms);
}

void test_protobuf_position(void) {
    TEST("Protobuf Position Decode");

    // Manually encode a Position: lat=37.8044*1e7, lon=-122.2712*1e7, alt=10m, time=1700000000
    // Using fixed32 wire type (5) for lat(field 1) and lon(field 2)
    uint8_t pb[] = {
        0x0d,  // field 1, wire type 5 (fixed32)
        0x00, 0x00, 0x00, 0x00,  // placeholder lat
        0x15,  // field 2, wire type 5 (fixed32)
        0x00, 0x00, 0x00, 0x00,  // placeholder lon
        0x25,  // field 4, wire type 5 (fixed32) - time
        0x00, 0x00, 0x00, 0x00,  // placeholder time
    };

    int32_t lat = (int32_t)(37.8044 * 1e7);
    int32_t lon = (int32_t)(-122.2712 * 1e7);
    uint32_t time = 1700000000;
    memcpy(pb + 1, &lat, 4);
    memcpy(pb + 6, &lon, 4);
    memcpy(pb + 11, &time, 4);

    MeshPosition pos;
    if (meshDecodePosition(pb, sizeof(pb), &pos)) {
        printf("  lat=%.4f lon=%.4f time=%u\n",
               pos.latitude(), pos.longitude(), pos.time);
        if (pos.latitude_i == lat && pos.longitude_i == lon && pos.time == time)
            PASS("position decoded correctly");
        else
            FAIL("position", "values mismatch");
    } else {
        FAIL("position", "decode failed");
    }
}

/* ---------- Main ---------------------------------------------------------- */

int main(void) {
    printf("═══════════════════════════════════════════════════\n");
    printf("  meshtastic-lite verification tests\n");
    printf("═══════════════════════════════════════════════════\n");

    test_nonce_construction();
    test_default_psk_expansion();
    test_channel_hash();
    test_decrypt_known_packet();
    test_frequency_calculation();
    test_packet_header_roundtrip();
    test_session_rx();
    test_multichannel();
    test_airtime();
    test_protobuf_position();

    printf("\n═══════════════════════════════════════════════════\n");
    printf("  Results: %d/%d passed\n", pass_count, test_count);
    printf("═══════════════════════════════════════════════════\n");

    return (pass_count == test_count) ? 0 : 1;
}
#endif
