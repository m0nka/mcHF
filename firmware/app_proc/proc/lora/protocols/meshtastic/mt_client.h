/**
 * meshtastic_task.h — FreeRTOS Meshtastic RX/TX task for T-Display-P4.
 *
 * Bridges the Cpp_Bus_Driver SX1262 with meshtastic-lite.
 * SPI1 (GPIO 2/3/4) is dedicated to SX1262 — no SPI mutex needed.
 * DIO1 routed through XL9535 IO expander — polled, not ISR.
 *
 * Public API:
 *   meshy_start()          — configure SX1262 and start RX/TX tasks
 *   meshy_stop()           — stop tasks and release radio
 *   meshy_send_text()      — queue a text message for TX
 *   meshy_get_messages()   — drain received message queue
 *   meshy_get_nodes()      — get known node table
 *
 * Part of ADS-B Receiver / Meshy — T-Display-P4.
 */
#pragma once

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// ─── Inbound message (pushed to UI) ─────────────────────────────────────────

#define MESHY_MAX_TEXT 240
#define MESHY_MAX_NODES 512

typedef struct {
    uint32_t from;                    // sender NodeNum
    uint32_t to;                      // destination (0xFFFFFFFF = broadcast)
    uint32_t id;                      // packet ID
    int8_t   channel_idx;             // which channel matched
    float    rssi;
    float    snr;
    uint8_t  hop_limit;
    uint8_t  hop_start;

    // Decoded content (port-specific)
    uint16_t portnum;                 // MeshPortNum
    char     text[MESHY_MAX_TEXT];     // for PORT_TEXT_MESSAGE
    size_t   text_len;

    // Position (for PORT_POSITION)
    bool     has_position;
    double   lat;
    double   lon;
    int32_t  altitude;

    // Node info (for PORT_NODEINFO)
    bool     has_nodeinfo;
    char     long_name[40];
    char     short_name[5];

    // Telemetry (for PORT_TELEMETRY)
    bool     has_telemetry;
    uint32_t battery_level;           // 0-100
    float    voltage;
    float    channel_util;
    float    air_util_tx;

    // Timestamp (boot-relative ms)
    uint32_t rx_time_ms;
    uint16_t rx_count;                // number of times this packet was received
} meshy_msg_t;

// ─── Known node entry ────────────────────────────────────────────────────────

typedef struct {
    uint32_t node_num;
    char     long_name[40];
    char     short_name[5];
    float    last_rssi;
    float    last_snr;
    uint32_t last_seen_ms;            // esp_log_timestamp() of last packet
    bool     has_position;
    double   lat;
    double   lon;
    int32_t  altitude;
    uint32_t battery_level;           // 0-100, from telemetry
    uint16_t hw_model;
} meshy_node_t;

// ─── Stats ───────────────────────────────────────────────────────────────────

typedef struct {
    uint32_t rx_packets;              // total packets received
    uint32_t rx_decoded;              // successfully decrypted
    uint32_t tx_packets;              // total packets sent
    uint32_t known_nodes;             // nodes in table
    float    freq_mhz;                // current frequency
    bool     running;                 // true if tasks are active
} meshy_stats_t;

// ─── Public API ──────────────────────────────────────────────────────────────

/**
 * Set hardware references before calling meshy_start().
 * Pass SX1262.get() and XL9535.get() from main.cpp.
 */
void meshy_set_hw(void *sx1262_ptr, void *xl9535_ptr);

/**
 * Start Meshtastic tasks. Reconfigures SX1262 with Meshtastic params.
 * Call after SX1262 is initialized (sx1262 begin success).
 * Returns true on success.
 */
bool meshy_start(void);

/**
 * Stop Meshtastic tasks. Returns SX1262 to idle.
 */
void meshy_stop(void);

/**
 * Queue a broadcast text message on channel 0 (default).
 * Non-blocking — TX task handles CSMA/CA and transmission.
 */
bool meshy_send_text(const char *text);

/**
 * Queue a direct text message to a specific node.
 */
bool meshy_send_dm(uint32_t dest_node, const char *text);

/**
 * Get next received message (non-blocking).
 * Returns true if a message was available, populates *msg.
 */
bool meshy_recv(meshy_msg_t *msg);

/**
 * Get the current node table. Caller provides array and max count.
 * Returns number of nodes copied.
 */
int meshy_get_nodes(meshy_node_t *out, int max_count);

/**
 * Get current stats.
 */
meshy_stats_t meshy_get_stats(void);

/**
 * Format node table into text buffer for display.
 * Returns number of nodes written.
 */
int meshy_format_nodes(char *buf, int bufsize);

/**
 * Format recent messages into text buffer for display.
 * Returns number of messages written.
 */
int meshy_format_messages(char *buf, int bufsize);

#ifdef __cplusplus
}
#endif
