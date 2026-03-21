/**
 * meshtastic_task.cpp — FreeRTOS Meshtastic RX/TX for T-Display-P4.
 *
 * Uses the Cpp_Bus_Driver SX1262 API (not RadioLib directly) to interface
 * with the HPD16A LoRa module. Meshtastic protocol handling via meshtastic-lite.
 *
 * Hardware notes:
 *   - SPI1 (GPIO 2/3/4) is dedicated to SX1262 — no SPI mutex needed
 *   - DIO1 routed through XL9535 I2C expander (XL9535_SX1262_DIO1) — polled
 *   - DIO2 used as RF switch internally by HPD16A (set via chip config)
 *   - SKY13453 RF switch: VCTL HIGH = TX path, LOW = RX path
 *
 * Part of ADS-B Receiver / Meshy — T-Display-P4.
 */
#include "main.h"
#include "mchf_pro_board.h"

#if defined (CONTEXT_LORA) && defined(MESHTASTIC)

#include "client.h"

#include <stdio.h>
#include <string.h>
#include <math.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "esp_mac.h"
#include "esp_heap_caps.h"

// Cpp_Bus_Driver includes
#include "cpp_bus_driver_library.h"
#include "t_display_p4_config.h"

// Meshtastic protocol library
#define MESH_CRYPTO_USE_MBEDTLS 1
#include "meshtastic.h"

static const char *TAG = "MESHY";

// ─── Hardware References (set by meshy_init_hw) ─────────────────────────────

static Cpp_Bus_Driver::Sx126x  *s_sx1262  = nullptr;
static Cpp_Bus_Driver::Xl95x5  *s_xl9535  = nullptr;

// ─── Meshtastic Session ─────────────────────────────────────────────────────

static MeshSession s_session;
static bool s_running = false;

// ─── Task Handles ───────────────────────────────────────────────────────────

static TaskHandle_t s_rx_task_hdl = nullptr;
static TaskHandle_t s_tx_task_hdl = nullptr;

// ─── Queues ─────────────────────────────────────────────────────────────────

static QueueHandle_t s_rx_queue  = nullptr;  // meshy_msg_t → UI
static QueueHandle_t s_tx_queue  = nullptr;  // tx_request_t → TX task

typedef struct {
    uint32_t to;
    uint8_t  channel_idx;
    char     text[MESHY_MAX_TEXT];
} tx_request_t;

// ─── Node Table ─────────────────────────────────────────────────────────────

static meshy_node_t *s_nodes = nullptr;  // allocated in PSRAM by meshy_start()
static int s_node_count = 0;
static SemaphoreHandle_t s_node_mutex = nullptr;

// ─── Stats ──────────────────────────────────────────────────────────────────

static meshy_stats_t s_stats;

// ─── Message History (ring buffer for UI display, PSRAM-backed) ─────────────

#define MESHY_MSG_HISTORY 1000
static meshy_msg_t *s_msg_history = nullptr;  // allocated in PSRAM by meshy_start()
static int s_msg_write_idx = 0;
static int s_msg_count = 0;

// ─── Node Table Helpers ─────────────────────────────────────────────────────

static meshy_node_t* find_or_create_node(uint32_t node_num) {
    // Find existing
    for (int i = 0; i < s_node_count; i++) {
        if (s_nodes[i].node_num == node_num) return &s_nodes[i];
    }
    // Create new
    if (s_node_count < MESHY_MAX_NODES) {
        meshy_node_t *n = &s_nodes[s_node_count++];
        memset(n, 0, sizeof(meshy_node_t));
        n->node_num = node_num;
        return n;
    }
    // Table full — evict oldest
    int oldest_idx = 0;
    uint32_t oldest_time = UINT32_MAX;
    for (int i = 0; i < s_node_count; i++) {
        if (s_nodes[i].last_seen_ms < oldest_time) {
            oldest_time = s_nodes[i].last_seen_ms;
            oldest_idx = i;
        }
    }
    meshy_node_t *n = &s_nodes[oldest_idx];
    memset(n, 0, sizeof(meshy_node_t));
    n->node_num = node_num;
    return n;
}

static void update_node_from_packet(const MeshRxResult *result) {
    xSemaphoreTake(s_node_mutex, portMAX_DELAY);

    meshy_node_t *node = find_or_create_node(result->packet.from);
    node->last_rssi = result->packet.rssi;
    node->last_snr  = result->packet.snr;
    node->last_seen_ms = esp_log_timestamp();

    // Update from decoded content
    if (result->data.portnum == PORT_NODEINFO) {
        MeshUser user;
        if (meshDecodeUser(result->data.payload, result->data.payload_len, &user)) {
            strncpy(node->long_name, user.long_name, sizeof(node->long_name) - 1);
            strncpy(node->short_name, user.short_name, sizeof(node->short_name) - 1);
            node->hw_model = user.hw_model;
            ESP_LOGI(TAG, "Node !%08lx: %s (%s)", (unsigned long)node->node_num,
                     node->long_name, node->short_name);
        }
    } else if (result->data.portnum == PORT_POSITION) {
        MeshPosition pos;
        if (meshDecodePosition(result->data.payload, result->data.payload_len, &pos)) {
            if (pos.latitude_i != 0 || pos.longitude_i != 0) {
                node->has_position = true;
                node->lat = pos.latitude();
                node->lon = pos.longitude();
                node->altitude = pos.altitude;
            }
        }
    } else if (result->data.portnum == PORT_TELEMETRY) {
        MeshTelemetry tel;
        if (meshDecodeTelemetry(result->data.payload, result->data.payload_len, &tel)) {
            if (tel.has_device_metrics) {
                node->battery_level = tel.device_metrics.battery_level;
            }
        }
    }

    s_stats.known_nodes = s_node_count;
    xSemaphoreGive(s_node_mutex);
}

// ─── Build meshy_msg_t from MeshRxResult ────────────────────────────────────

static void build_msg(const MeshRxResult *result, meshy_msg_t *msg) {
    memset(msg, 0, sizeof(meshy_msg_t));

    msg->from        = result->packet.from;
    msg->to          = result->packet.to;
    msg->id          = result->packet.id;
    msg->channel_idx = result->channel_idx;
    msg->rssi        = result->packet.rssi;
    msg->snr         = result->packet.snr;
    msg->hop_limit   = result->packet.hop_limit;
    msg->hop_start   = result->packet.hop_start;
    msg->portnum     = result->data.portnum;
    msg->rx_time_ms  = esp_log_timestamp();
    msg->rx_count    = 1;

    switch (result->data.portnum) {
        case PORT_TEXT_MESSAGE: {
            size_t tlen = result->data.payload_len;
            if (tlen >= MESHY_MAX_TEXT) tlen = MESHY_MAX_TEXT - 1;
            memcpy(msg->text, result->data.payload, tlen);
            msg->text[tlen] = '\0';
            msg->text_len = tlen;
            break;
        }
        case PORT_POSITION: {
            MeshPosition pos;
            if (meshDecodePosition(result->data.payload, result->data.payload_len, &pos)) {
                msg->has_position = true;
                msg->lat = pos.latitude();
                msg->lon = pos.longitude();
                msg->altitude = pos.altitude;
            }
            break;
        }
        case PORT_NODEINFO: {
            MeshUser user;
            if (meshDecodeUser(result->data.payload, result->data.payload_len, &user)) {
                msg->has_nodeinfo = true;
                strncpy(msg->long_name, user.long_name, sizeof(msg->long_name) - 1);
                strncpy(msg->short_name, user.short_name, sizeof(msg->short_name) - 1);
            }
            break;
        }
        case PORT_TELEMETRY: {
            MeshTelemetry tel;
            if (meshDecodeTelemetry(result->data.payload, result->data.payload_len, &tel)) {
                if (tel.has_device_metrics) {
                    msg->has_telemetry = true;
                    msg->battery_level = tel.device_metrics.battery_level;
                    msg->voltage = tel.device_metrics.voltage;
                    msg->channel_util = tel.device_metrics.channel_utilization;
                    msg->air_util_tx = tel.device_metrics.air_util_tx;
                }
            }
            break;
        }
        default:
            break;
    }
}

// ─── SX1262 Configuration for Meshtastic ────────────────────────────────────

// Map meshtastic BW (kHz) → Cpp_Bus_Driver enum
static Cpp_Bus_Driver::Sx126x::Lora_Bw meshBwToEnum(float bw_khz) {
    if (bw_khz >= 500.0f) return Cpp_Bus_Driver::Sx126x::Lora_Bw::BW_500000HZ;
    if (bw_khz >= 250.0f) return Cpp_Bus_Driver::Sx126x::Lora_Bw::BW_250000HZ;
    return Cpp_Bus_Driver::Sx126x::Lora_Bw::BW_125000HZ;
}

// Map meshtastic SF (7-12) → Cpp_Bus_Driver enum
static Cpp_Bus_Driver::Sx126x::Sf meshSfToEnum(uint8_t sf) {
    switch (sf) {
        case 7:  return Cpp_Bus_Driver::Sx126x::Sf::SF7;
        case 8:  return Cpp_Bus_Driver::Sx126x::Sf::SF8;
        case 9:  return Cpp_Bus_Driver::Sx126x::Sf::SF9;
        case 10: return Cpp_Bus_Driver::Sx126x::Sf::SF10;
        case 11: return Cpp_Bus_Driver::Sx126x::Sf::SF11;
        case 12: return Cpp_Bus_Driver::Sx126x::Sf::SF12;
        default: return Cpp_Bus_Driver::Sx126x::Sf::SF9;
    }
}

// Map meshtastic CR denominator (5-8) → Cpp_Bus_Driver enum
static Cpp_Bus_Driver::Sx126x::Cr meshCrToEnum(uint8_t cr) {
    switch (cr) {
        case 5:  return Cpp_Bus_Driver::Sx126x::Cr::CR_4_5;
        case 6:  return Cpp_Bus_Driver::Sx126x::Cr::CR_4_6;
        case 7:  return Cpp_Bus_Driver::Sx126x::Cr::CR_4_7;
        case 8:  return Cpp_Bus_Driver::Sx126x::Cr::CR_4_8;
        default: return Cpp_Bus_Driver::Sx126x::Cr::CR_4_5;
    }
}

// Convert single-byte LoRa sync word to SX126x 2-byte register format
static uint16_t meshSyncWordToSx126x(uint8_t sw) {
    uint8_t hi = (sw & 0xF0) | 0x04;
    uint8_t lo = ((sw & 0x0F) << 4) | 0x04;
    return ((uint16_t)hi << 8) | lo;
}

static bool configure_radio(void) {
    if (!s_sx1262 || !s_xl9535) return false;

    const char *ch_name = s_session.channels.effectiveName(0);
    MeshRadioConfig rc = s_session.radioConfig();

    ESP_LOGI(TAG, "Configuring SX1262: %.3f MHz, BW%.0f, SF%d, CR4/%d, SW 0x%02X, %ddBm",
             rc.frequency_mhz, rc.bandwidth_khz, rc.spreading_factor,
             rc.coding_rate, rc.sync_word, rc.tx_power_dbm);

    // RF switch: LOW = RX path for the SKY13453
    s_xl9535->pin_write(XL9535_SKY13453_VCTL, Cpp_Bus_Driver::Xl95x5::Value::LOW);

    // Configure LoRa parameters via Cpp_Bus_Driver API
    if (!s_sx1262->config_lora_params(
            rc.frequency_mhz,
            meshBwToEnum(rc.bandwidth_khz),
            140.0f,                    // current limit mA
            rc.tx_power_dbm,
            meshSfToEnum(rc.spreading_factor),
            meshCrToEnum(rc.coding_rate),
            Cpp_Bus_Driver::Sx126x::Lora_Crc_Type::ON,
            rc.preamble_length,
            meshSyncWordToSx126x(rc.sync_word)))
    {
        ESP_LOGE(TAG, "config_lora_params failed");
        return false;
    }

    // Put radio into continuous RX
    s_sx1262->clear_buffer();
    s_sx1262->start_lora_transmit(Cpp_Bus_Driver::Sx126x::Chip_Mode::RX);
    s_sx1262->set_irq_pin_mode(Cpp_Bus_Driver::Sx126x::Irq_Mask_Flag::RX_DONE);
    s_sx1262->clear_irq_flag(Cpp_Bus_Driver::Sx126x::Irq_Mask_Flag::RX_DONE);

    s_stats.freq_mhz = rc.frequency_mhz;
    ESP_LOGI(TAG, "Radio configured, listening on %.3f MHz (ch: %s)", rc.frequency_mhz, ch_name);
    return true;
}

// ─── RX Task ────────────────────────────────────────────────────────────────

static void meshy_rx_task(void *arg) {
    ESP_LOGI(TAG, "RX task started");

    while (s_running) {
        // Poll DIO1 through XL9535 GPIO expander
        if (s_xl9535->pin_read(XL9535_SX1262_DIO1) == 1) {
            Cpp_Bus_Driver::Sx126x::Irq_Status irq;
            if (!s_sx1262->parse_irq_status(s_sx1262->get_irq_flag(), irq)) {
                s_sx1262->clear_irq_flag(Cpp_Bus_Driver::Sx126x::Irq_Mask_Flag::RX_DONE);
                vTaskDelay(pdMS_TO_TICKS(10));
                continue;
            }

            if (irq.all_flag.crc_error) {
                s_sx1262->clear_irq_flag(Cpp_Bus_Driver::Sx126x::Irq_Mask_Flag::CRC_ERROR);
                vTaskDelay(pdMS_TO_TICKS(10));
                continue;
            }

            if (irq.all_flag.tx_rx_timeout) {
                s_sx1262->clear_irq_flag(Cpp_Bus_Driver::Sx126x::Irq_Mask_Flag::TIMEOUT);
                // Re-enter RX
                s_sx1262->start_lora_transmit(Cpp_Bus_Driver::Sx126x::Chip_Mode::RX);
                s_sx1262->set_irq_pin_mode(Cpp_Bus_Driver::Sx126x::Irq_Mask_Flag::RX_DONE);
                s_sx1262->clear_irq_flag(Cpp_Bus_Driver::Sx126x::Irq_Mask_Flag::RX_DONE);
                vTaskDelay(pdMS_TO_TICKS(10));
                continue;
            }

            if (irq.lora_reg_flag.header_error) {
                s_sx1262->clear_irq_flag(Cpp_Bus_Driver::Sx126x::Irq_Mask_Flag::HEADER_ERROR);
                vTaskDelay(pdMS_TO_TICKS(10));
                continue;
            }

            // Read received data
            uint8_t raw[256];
            uint8_t len = s_sx1262->receive_data(raw);

            // Clear RX_DONE IRQ — must happen after read, or DIO1 stays asserted
            // and we re-read the same buffer every poll cycle
            s_sx1262->clear_irq_flag(Cpp_Bus_Driver::Sx126x::Irq_Mask_Flag::RX_DONE);

            if (len == 0) {
                vTaskDelay(pdMS_TO_TICKS(10));
                continue;
            }

            // Get RSSI/SNR
            float rssi = 0, snr = 0;
            Cpp_Bus_Driver::Sx126x::Packet_Metrics pm;
            if (s_sx1262->get_lora_packet_metrics(pm)) {
                rssi = pm.lora.rssi_instantaneous;
                snr  = pm.lora.snr;
            }

            s_stats.rx_packets++;

            // Process through Meshtastic protocol
            MeshRxResult result;
            if (s_session.processRx(raw, len, rssi, snr, &result)) {
                s_stats.rx_decoded++;

                // Update node table
                update_node_from_packet(&result);

                // Build message for UI
                meshy_msg_t msg;
                build_msg(&result, &msg);

                // Push to queue (non-blocking — drop if full)
                xQueueSend(s_rx_queue, &msg, 0);

                // Store in ring buffer — dedup by packet ID
                bool is_dup = false;
                for (int i = 0; i < s_msg_count; i++) {
                    int idx = (s_msg_count < MESHY_MSG_HISTORY)
                            ? i
                            : (s_msg_write_idx + i) % MESHY_MSG_HISTORY;
                    if (s_msg_history[idx].id == msg.id && s_msg_history[idx].from == msg.from) {
                        s_msg_history[idx].rx_count++;
                        is_dup = true;
                        break;
                    }
                }
                if (!is_dup) {
                    s_msg_history[s_msg_write_idx] = msg;
                    s_msg_write_idx = (s_msg_write_idx + 1) % MESHY_MSG_HISTORY;
                    if (s_msg_count < MESHY_MSG_HISTORY) s_msg_count++;
                }

                // Log decoded messages
                const char *port_name = "?";
                switch (result.data.portnum) {
                    case PORT_TEXT_MESSAGE: port_name = "TEXT"; break;
                    case PORT_POSITION:    port_name = "POS";  break;
                    case PORT_NODEINFO:    port_name = "NODE"; break;
                    case PORT_TELEMETRY:   port_name = "TELEM"; break;
                    case PORT_ROUTING:     port_name = "ROUTE"; break;
                    default: break;
                }

                // Look up sender name
                const char *sender = "?";
                xSemaphoreTake(s_node_mutex, portMAX_DELAY);
                for (int i = 0; i < s_node_count; i++) {
                    if (s_nodes[i].node_num == result.packet.from && s_nodes[i].short_name[0]) {
                        sender = s_nodes[i].short_name;
                        break;
                    }
                }
                xSemaphoreGive(s_node_mutex);

                ESP_LOGI(TAG, "[%s] !%08lx (%s) RSSI:%.0f SNR:%.1f hop:%d/%d ch:%d",
                         port_name, (unsigned long)result.packet.from, sender,
                         rssi, snr, result.packet.hop_limit, result.packet.hop_start,
                         result.channel_idx);

                if (result.data.portnum == PORT_TEXT_MESSAGE) {
                    ESP_LOGI(TAG, "  \"%.*s\"", (int)msg.text_len, msg.text);
                } else if (result.data.portnum == PORT_POSITION && msg.has_position) {
                    ESP_LOGI(TAG, "  pos:%.6f,%.6f alt:%d", msg.lat, msg.lon, (int)msg.altitude);
                } else if (result.data.portnum == PORT_TELEMETRY && msg.has_telemetry) {
                    ESP_LOGI(TAG, "  bat:%lu%% %.2fV chUtil:%.1f%% airTx:%.1f%%",
                             (unsigned long)msg.battery_level, msg.voltage,
                             msg.channel_util, msg.air_util_tx);
                }
            }
        }

        vTaskDelay(pdMS_TO_TICKS(50));  // 50ms poll — SF11/BW250 min airtime is ~230ms
    }

    ESP_LOGI(TAG, "RX task exiting");
    s_rx_task_hdl = nullptr;
    vTaskDelete(NULL);
}

// ─── TX Task ────────────────────────────────────────────────────────────────

static void meshy_tx_task(void *arg) {
    ESP_LOGI(TAG, "TX task started");

    while (s_running) {
        tx_request_t req;
        if (xQueueReceive(s_tx_queue, &req, pdMS_TO_TICKS(200)) == pdTRUE) {
            // CSMA/CA random backoff
            uint32_t delay = meshTxDelayMs(s_session.preset);
            if (delay > 0) {
                ESP_LOGI(TAG, "CSMA/CA backoff: %lu ms", (unsigned long)delay);
                vTaskDelay(pdMS_TO_TICKS(delay));
            }

            // Build encrypted TX frame
            uint8_t frame[256];
            size_t frame_len = s_session.buildTextTx(
                req.channel_idx, req.to, req.text, false, frame);

            if (frame_len == 0) {
                ESP_LOGE(TAG, "Failed to build TX frame");
                continue;
            }

            // Switch to TX
            s_xl9535->pin_write(XL9535_SKY13453_VCTL, Cpp_Bus_Driver::Xl95x5::Value::HIGH);

            s_sx1262->start_lora_transmit(Cpp_Bus_Driver::Sx126x::Chip_Mode::TX, 0,
                                           Cpp_Bus_Driver::Sx126x::Fallback_Mode::FS);
            s_sx1262->set_irq_pin_mode(Cpp_Bus_Driver::Sx126x::Irq_Mask_Flag::TX_DONE);
            s_sx1262->clear_irq_flag(Cpp_Bus_Driver::Sx126x::Irq_Mask_Flag::TX_DONE);

            if (s_sx1262->send_data(frame, frame_len)) {
                // Wait for TX done (poll DIO1)
                uint16_t timeout = 0;
                bool tx_ok = false;
                while (timeout < 500) {  // 5 second max
                    if (s_xl9535->pin_read(XL9535_SX1262_DIO1) == 1) {
                        Cpp_Bus_Driver::Sx126x::Irq_Status irq;
                        if (s_sx1262->parse_irq_status(s_sx1262->get_irq_flag(), irq)) {
                            if (irq.all_flag.tx_done) {
                                tx_ok = true;
                                break;
                            }
                        }
                    }
                    timeout++;
                    vTaskDelay(pdMS_TO_TICKS(10));
                }

                if (tx_ok) {
                    s_stats.tx_packets++;
                    ESP_LOGI(TAG, "TX success: %zu bytes to %s",
                             frame_len, req.to == MESH_ADDR_BROADCAST ? "broadcast" : "DM");
                } else {
                    ESP_LOGE(TAG, "TX timeout");
                }
            } else {
                ESP_LOGE(TAG, "send_data failed");
            }

            // Return to RX
            s_xl9535->pin_write(XL9535_SKY13453_VCTL, Cpp_Bus_Driver::Xl95x5::Value::LOW);
            s_sx1262->start_lora_transmit(Cpp_Bus_Driver::Sx126x::Chip_Mode::RX);
            s_sx1262->set_irq_pin_mode(Cpp_Bus_Driver::Sx126x::Irq_Mask_Flag::RX_DONE);
            s_sx1262->clear_irq_flag(Cpp_Bus_Driver::Sx126x::Irq_Mask_Flag::RX_DONE);
        }
    }

    ESP_LOGI(TAG, "TX task exiting");
    s_tx_task_hdl = nullptr;
    vTaskDelete(NULL);
}

// ─── Public API ─────────────────────────────────────────────────────────────

// These must be set before calling meshy_start().
// Call from main.cpp: meshy_set_hw(SX1262.get(), XL9535.get());
extern "C" void meshy_set_hw(void *sx1262_ptr, void *xl9535_ptr) {
    s_sx1262 = static_cast<Cpp_Bus_Driver::Sx126x *>(sx1262_ptr);
    s_xl9535 = static_cast<Cpp_Bus_Driver::Xl95x5 *>(xl9535_ptr);
}

extern "C" bool meshy_start(void) {
    if (s_running) return true;
    if (!s_sx1262 || !s_xl9535) {
        ESP_LOGE(TAG, "Hardware not initialized — call meshy_set_hw() first");
        return false;
    }

    // Initialize Meshtastic session
    // Node number: use lower 4 bytes of base MAC
    uint8_t mac[8];
    esp_read_mac(mac, ESP_MAC_EFUSE_FACTORY);
    uint32_t node_num = ((uint32_t)mac[2] << 24) | ((uint32_t)mac[3] << 16) |
                        ((uint32_t)mac[4] << 8) | mac[5];

    s_session.init(REGION_US, MODEM_MEDIUM_FAST, ROLE_CLIENT, node_num);
    s_session.channels.addDefaultChannel();

    ESP_LOGI(TAG, "Session initialized: node=!%08lx region=US preset=MediumFast",
             (unsigned long)node_num);

    // Create queues and allocate message history in PSRAM
    if (!s_rx_queue) s_rx_queue = xQueueCreate(16, sizeof(meshy_msg_t));
    if (!s_tx_queue) s_tx_queue = xQueueCreate(8, sizeof(tx_request_t));
    if (!s_node_mutex) s_node_mutex = xSemaphoreCreateMutex();
    if (!s_msg_history) {
        s_msg_history = (meshy_msg_t *)heap_caps_calloc(MESHY_MSG_HISTORY, sizeof(meshy_msg_t), MALLOC_CAP_SPIRAM);
        if (!s_msg_history) {
            ESP_LOGE(TAG, "Failed to allocate message history in PSRAM (%zu bytes)",
                     MESHY_MSG_HISTORY * sizeof(meshy_msg_t));
            return false;
        }
        ESP_LOGI(TAG, "Message history: %d slots in PSRAM (%zu bytes)",
                 MESHY_MSG_HISTORY, MESHY_MSG_HISTORY * sizeof(meshy_msg_t));
    }
    if (!s_nodes) {
        s_nodes = (meshy_node_t *)heap_caps_calloc(MESHY_MAX_NODES, sizeof(meshy_node_t), MALLOC_CAP_SPIRAM);
        if (!s_nodes) {
            ESP_LOGE(TAG, "Failed to allocate node table in PSRAM (%zu bytes)",
                     MESHY_MAX_NODES * sizeof(meshy_node_t));
            return false;
        }
        ESP_LOGI(TAG, "Node table: %d slots in PSRAM (%zu bytes)",
                 MESHY_MAX_NODES, MESHY_MAX_NODES * sizeof(meshy_node_t));
    }

    // Reset stats
    memset(&s_stats, 0, sizeof(s_stats));

    // Configure radio
    if (!configure_radio()) {
        ESP_LOGE(TAG, "Radio configuration failed");
        return false;
    }

    s_running = true;
    s_stats.running = true;

    // Start tasks (in PSRAM if available — no DMA from stack)
    BaseType_t ret;
    ret = xTaskCreateWithCaps(meshy_rx_task, "meshy_rx", 8192, NULL, 3, &s_rx_task_hdl, MALLOC_CAP_SPIRAM);
    if (ret != pdPASS) {
        xTaskCreate(meshy_rx_task, "meshy_rx", 8192, NULL, 3, &s_rx_task_hdl);
    }

    ret = xTaskCreateWithCaps(meshy_tx_task, "meshy_tx", 8192, NULL, 2, &s_tx_task_hdl, MALLOC_CAP_SPIRAM);
    if (ret != pdPASS) {
        xTaskCreate(meshy_tx_task, "meshy_tx", 8192, NULL, 2, &s_tx_task_hdl);
    }

    ESP_LOGI(TAG, "Meshy started — listening on %.3f MHz", s_stats.freq_mhz);
    return true;
}

extern "C" void meshy_stop(void) {
    if (!s_running) return;
    s_running = false;
    s_stats.running = false;

    // Wait for tasks to exit
    for (int i = 0; i < 20 && (s_rx_task_hdl || s_tx_task_hdl); i++)
        vTaskDelay(pdMS_TO_TICKS(100));

    // Put radio to standby
    if (s_sx1262) {
        s_sx1262->start_lora_transmit(Cpp_Bus_Driver::Sx126x::Chip_Mode::RX);
    }

    ESP_LOGI(TAG, "Meshy stopped");
}

extern "C" bool meshy_send_text(const char *text) {
    if (!s_running || !s_tx_queue) return false;

    tx_request_t req;
    req.to = MESH_ADDR_BROADCAST;
    req.channel_idx = 0;
    strncpy(req.text, text, MESHY_MAX_TEXT - 1);
    req.text[MESHY_MAX_TEXT - 1] = '\0';

    return xQueueSend(s_tx_queue, &req, pdMS_TO_TICKS(100)) == pdTRUE;
}

extern "C" bool meshy_send_dm(uint32_t dest_node, const char *text) {
    if (!s_running || !s_tx_queue) return false;

    tx_request_t req;
    req.to = dest_node;
    req.channel_idx = 0;
    strncpy(req.text, text, MESHY_MAX_TEXT - 1);
    req.text[MESHY_MAX_TEXT - 1] = '\0';

    return xQueueSend(s_tx_queue, &req, pdMS_TO_TICKS(100)) == pdTRUE;
}

extern "C" bool meshy_recv(meshy_msg_t *msg) {
    if (!s_rx_queue) return false;
    return xQueueReceive(s_rx_queue, msg, 0) == pdTRUE;
}

extern "C" int meshy_get_nodes(meshy_node_t *out, int max_count) {
    if (!s_node_mutex) return 0;
    xSemaphoreTake(s_node_mutex, portMAX_DELAY);
    int count = (s_node_count < max_count) ? s_node_count : max_count;
    memcpy(out, s_nodes, count * sizeof(meshy_node_t));
    xSemaphoreGive(s_node_mutex);
    return count;
}

extern "C" meshy_stats_t meshy_get_stats(void) {
    return s_stats;
}

extern "C" int meshy_format_nodes(char *buf, int bufsize) {
    int pos = 0;
    if (!s_node_mutex) return 0;

    xSemaphoreTake(s_node_mutex, portMAX_DELAY);
    for (int i = 0; i < s_node_count && pos < bufsize - 80; i++) {
        meshy_node_t *n = &s_nodes[i];
        const char *name = n->short_name[0] ? n->short_name : "???";
        uint32_t age_s = (esp_log_timestamp() - n->last_seen_ms) / 1000;

        pos += snprintf(buf + pos, bufsize - pos,
            "!%08lX %-4s %-16s RSSI:%.0f %lus ago\n",
            (unsigned long)n->node_num, name,
            n->long_name[0] ? n->long_name : "",
            n->last_rssi, (unsigned long)age_s);
    }
    xSemaphoreGive(s_node_mutex);

    if (pos == 0) pos += snprintf(buf + pos, bufsize - pos, "No nodes heard");
    return s_node_count;
}

extern "C" int meshy_format_messages(char *buf, int bufsize) {
    int pos = 0;
    int count = 0;

    if (!s_msg_history || s_msg_count == 0) {
        pos += snprintf(buf + pos, bufsize - pos, "No messages yet\nListening on %.3f MHz...", s_stats.freq_mhz);
        return 0;
    }

    // Walk ring buffer from oldest to newest
    int start = (s_msg_count < MESHY_MSG_HISTORY)
              ? 0
              : s_msg_write_idx;

    for (int i = 0; i < s_msg_count && pos < bufsize - 120; i++) {
        int idx = (start + i) % MESHY_MSG_HISTORY;
        meshy_msg_t *m = &s_msg_history[idx];

        // Look up sender name
        const char *name = "???";
        if (s_node_mutex) {
            xSemaphoreTake(s_node_mutex, portMAX_DELAY);
            for (int j = 0; j < s_node_count; j++) {
                if (s_nodes[j].node_num == m->from && s_nodes[j].short_name[0]) {
                    name = s_nodes[j].short_name;
                    break;
                }
            }
            xSemaphoreGive(s_node_mutex);
        }

        // Repeat indicator suffix
        const char *rx_suffix = "";
        char rx_buf[12] = "";
        if (m->rx_count > 1) {
            snprintf(rx_buf, sizeof(rx_buf), " x%d", m->rx_count);
            rx_suffix = rx_buf;
        }

        switch (m->portnum) {
            case PORT_TEXT_MESSAGE:
                pos += snprintf(buf + pos, bufsize - pos,
                    "[%s] %s%s\n", name, m->text, rx_suffix);
                count++;
                break;
            case PORT_POSITION:
                if (m->has_position) {
                    pos += snprintf(buf + pos, bufsize - pos,
                        "[%s] @ %.4f,%.4f %dm%s\n",
                        name, m->lat, m->lon, (int)m->altitude, rx_suffix);
                    count++;
                }
                break;
            case PORT_NODEINFO:
                if (m->has_nodeinfo) {
                    pos += snprintf(buf + pos, bufsize - pos,
                        "[%s] joined: %s%s\n", m->short_name, m->long_name, rx_suffix);
                    count++;
                }
                break;
            case PORT_TELEMETRY:
                if (m->has_telemetry) {
                    pos += snprintf(buf + pos, bufsize - pos,
                        "[%s] bat:%lu%% %.1fV ch:%.0f%% air:%.0f%%%s\n",
                        name, (unsigned long)m->battery_level, m->voltage,
                        m->channel_util, m->air_util_tx, rx_suffix);
                    count++;
                }
                break;
            default:
                break;
        }
    }

    if (pos == 0) pos += snprintf(buf + pos, bufsize - pos, "No messages yet\nListening on %.3f MHz...", s_stats.freq_mhz);
    return count;
}
#endif
