//
// Implementation provided by:
// https://github.com/Nicolai-Electronics/esp32-component-sx126x
//
#include "main.h"
#include "mchf_pro_board.h"

#ifdef CONTEXT_LORA

#include <iso646.h>
#include <stdint.h>
//#include "driver/spi_common.h"
//#include "driver/spi_master.h"
//#include "esp_check.h"
//#include "esp_err.h"
//#include "esp_log.h"
#include "lora_spi.h"

#include "sx126x.h"

static const char* TAG = "sx126x";

// Interrupt handlers

//IRAM_ATTR
static void sx1262_busy_handler(void* pvParameters)
{
    sx126x_handle_t* handle = (sx126x_handle_t*)pvParameters;
    if (handle == NULL) return;
#if 0
    if (gpio_get_level(handle->busy))
    {
        xSemaphoreTakeFromISR(handle->busy_semaphore, NULL);
    } else
    {
        xSemaphoreGiveFromISR(handle->busy_semaphore, NULL);
    }
#endif
}

//IRAM_ATTR
static void sx1262_dio1_handler(void* pvParameters)
{
    sx126x_handle_t* handle = (sx126x_handle_t*)pvParameters;

    if(handle == NULL)
    	return;

    xSemaphoreGiveFromISR(handle->interrupt_semaphore, NULL);
}

// Private functions

static esp_err_t sx126x_busy_wait(sx126x_handle_t* handle)
{
    if (handle == NULL)
    	return ESP_ERR_INVALID_ARG;
#if 0
    if(!gpio_get_level(handle->busy))
    {
        return ESP_OK;  // Don't take semaphore if not busy
    }
#endif
    if(xSemaphoreTake(handle->busy_semaphore, handle->timeout) == pdTRUE) {
        return ESP_OK;
    }
    else
    {
        return ESP_ERR_TIMEOUT;
    }
}

static esp_err_t sx126x_command(sx126x_handle_t* handle, uint8_t command, const uint8_t* tx_data, uint8_t* rx_data,size_t length)
{
    if (handle == NULL)
    	return ESP_ERR_INVALID_ARG;
#if 0
    spi_transaction_t t = {
        .flags     = 0,
        .cmd       = command,
        .length    = length * 8,
        .tx_buffer = tx_data,
        .rx_buffer = rx_data,
    };

    esp_err_t res = spi_device_transmit(handle->device, &t);
    if (res != ESP_OK)
    	return res;
#endif

    return sx126x_busy_wait(handle);
}

// Public functions - commands

esp_err_t sx126x_set_op_mode_sleep(sx126x_handle_t* handle, bool warm_start, bool rtc_timeout_disable) {
    if (handle == NULL) return ESP_ERR_INVALID_ARG;
    uint8_t sleep_config  = 0;
    sleep_config         |= (warm_start ? 0x01 : 0x00) << 2;                            // Warm start
    sleep_config         |= ((!warm_start && rtc_timeout_disable) ? 0x01 : 0x00) << 0;  // RTC timeout disable
    return sx126x_command(handle, SX126X_CMD_SET_SLEEP, &sleep_config, NULL, 1);
}

esp_err_t sx126x_set_op_mode_standby(sx126x_handle_t* handle, bool use_xosc) {
    if (handle == NULL) return ESP_ERR_INVALID_ARG;
    uint8_t standby_config  = 0;
    standby_config         |= (use_xosc ? 0x01 : 0x00) << 0;
    return sx126x_command(handle, SX126X_CMD_SET_STANDBY, &standby_config, NULL, 1);
}

esp_err_t sx126x_set_op_mode_fs(sx126x_handle_t* handle) {
    if (handle == NULL) return ESP_ERR_INVALID_ARG;
    return sx126x_command(handle, SX126X_CMD_SET_FS, NULL, NULL, 0);
}

esp_err_t sx126x_set_op_mode_tx(sx126x_handle_t* handle) {
    if (handle == NULL) return ESP_ERR_INVALID_ARG;
    return sx126x_command(handle, SX126X_CMD_SET_TX, NULL, NULL, 0);
}

esp_err_t sx126x_set_op_mode_rx(sx126x_handle_t* handle) {
    if (handle == NULL) return ESP_ERR_INVALID_ARG;
    uint8_t timeout[3] = {0, 0, 0};  // Continuous RX
    return sx126x_command(handle, SX126X_CMD_SET_RX, timeout, NULL, 3);
}

esp_err_t sx126x_stop_timer_on_preamble(sx126x_handle_t* handle, bool stop) {
    if (handle == NULL) return ESP_ERR_INVALID_ARG;
    uint8_t parameter = stop ? 0x01 : 0x00;
    return sx126x_command(handle, SX126X_CMD_STOP_TIMER_ON_PREAMBLE, &parameter, NULL, 1);
}

esp_err_t sx126x_set_rx_duty_cycle(sx126x_handle_t* handle, uint32_t rx_period, uint32_t sleep_period) {
    if (handle == NULL || rx_period > 0x00FFFFFF || sleep_period > 0x00FFFFFF) return ESP_ERR_INVALID_ARG;
    uint8_t parameters[6];
    parameters[0] = (rx_period >> 16) & 0xFF;
    parameters[1] = (rx_period >> 8) & 0xFF;
    parameters[2] = rx_period & 0xFF;
    parameters[3] = (sleep_period >> 16) & 0xFF;
    parameters[4] = (sleep_period >> 8) & 0xFF;
    parameters[5] = sleep_period & 0xFF;
    return sx126x_command(handle, SX126X_CMD_SET_RX_DUTY_CYCLE, parameters, NULL, sizeof(parameters));
}

esp_err_t sx126x_set_cad(sx126x_handle_t* handle) {
    if (handle == NULL) return ESP_ERR_INVALID_ARG;
    return sx126x_command(handle, SX126X_CMD_SET_CAD, NULL, NULL, 0);
}

esp_err_t sx126x_tx_continuous_wave(sx126x_handle_t* handle) {
    if (handle == NULL) return ESP_ERR_INVALID_ARG;
    return sx126x_command(handle, SX126X_CMD_SET_TX_CONTINUOUS_WAVE, NULL, NULL, 0);
}

esp_err_t sx126x_set_tx_infinite_preamble(sx126x_handle_t* handle) {
    if (handle == NULL) return ESP_ERR_INVALID_ARG;
    return sx126x_command(handle, SX126X_CMD_SET_TX_INFINITE_PREAMBLE, NULL, NULL, 0);
}

esp_err_t sx126x_set_regulator_mode(sx126x_handle_t* handle, bool use_dc_dc) {
    if (handle == NULL) return ESP_ERR_INVALID_ARG;
    uint8_t parameter = use_dc_dc ? 0x01 : 0x00;
    return sx126x_command(handle, SX126X_CMD_SET_REGULATOR_MODE, &parameter, NULL, 1);
}

esp_err_t sx126x_calibrate(sx126x_handle_t* handle, bool rc64k, bool rc13m, bool pll, bool adc_pulse, bool adc_bulk_n,
                           bool adc_bulk_p, bool image) {
    if (handle == NULL) return ESP_ERR_INVALID_ARG;
    uint8_t parameter = (rc64k & 1) | ((rc13m & 1) << 1) | ((pll & 1) << 2) | ((adc_pulse & 1) << 3) |
                        ((adc_bulk_n & 1) << 4) | ((adc_bulk_p & 1) << 5) | ((image & 1) << 6);
    return sx126x_command(handle, SX126X_CMD_CALIBRATE, &parameter, NULL, 1);
}

esp_err_t sx126x_calibrate_image(sx126x_handle_t* handle, uint8_t frequency1, uint8_t frequency2) {
    if (handle == NULL) return ESP_ERR_INVALID_ARG;
    uint8_t parameters[2] = {frequency1, frequency2};
    return sx126x_command(handle, SX126X_CMD_CALIBRATE_IMAGE, parameters, NULL, sizeof(parameters));
}

esp_err_t sx126x_set_pa_config(sx126x_handle_t* handle, uint8_t pa_duty_cycle, uint8_t hp_max, bool is_sx1261) {
    if (handle == NULL || hp_max > 0x07) return ESP_ERR_INVALID_ARG;
    uint8_t pa_lut        = 0x01;  // Reserved byte, always 0x01
    uint8_t device_sel    = is_sx1261 ? 0x01 : 0x00;
    uint8_t parameters[4] = {pa_duty_cycle, hp_max, device_sel, pa_lut};
    return sx126x_command(handle, SX126X_CMD_SET_PA_CONFIG, parameters, NULL, sizeof(parameters));
}

esp_err_t sx126x_set_rx_tx_fallback_mode(sx126x_handle_t* handle, sx126x_fallback_mode_t fallback_mode) {
    if (handle == NULL) return ESP_ERR_INVALID_ARG;
    uint8_t parameter = (uint8_t)fallback_mode;
    return sx126x_command(handle, SX126X_CMD_SET_RX_TX_FALLBACK_MODE, &parameter, NULL, 1);
}

esp_err_t sx126x_set_dio_irq_params(sx126x_handle_t* handle, uint16_t irq_mask, uint16_t dio1_mask, uint16_t dio2_mask,
                                    uint16_t dio3_mask) {
    if (handle == NULL) return ESP_ERR_INVALID_ARG;

    uint8_t parameters[8];
    parameters[0] = (irq_mask >> 8) & 0xFF;
    parameters[1] = irq_mask & 0xFF;
    parameters[2] = (dio1_mask >> 8) & 0xFF;
    parameters[3] = dio1_mask & 0xFF;
    parameters[4] = (dio2_mask >> 8) & 0xFF;
    parameters[5] = dio2_mask & 0xFF;
    parameters[6] = (dio3_mask >> 8) & 0xFF;
    parameters[7] = dio3_mask & 0xFF;

    return sx126x_command(handle, SX126X_CMD_SET_DIO_IRQ_PARAMS, parameters, NULL, sizeof(parameters));
}

esp_err_t sx126x_get_irq_status(sx126x_handle_t* handle, uint16_t* out_irq_status, uint8_t* out_command_status,
                                uint8_t* out_chip_mode)
{
    if (handle == NULL)
    	return ESP_ERR_INVALID_ARG;

    uint8_t result[3];
#if 0
    spi_transaction_t t = {
        .flags     = 0,
        .cmd       = SX126X_CMD_GET_IRQ_STATUS,
        .length    = 8 * sizeof(result),
        .tx_buffer = NULL,
        .rx_buffer = result,
    };
    esp_err_t res = spi_device_transmit(handle->device, &t);
    if (res != ESP_OK) return res;

    if (out_irq_status) {
        *out_irq_status = (result[1] << 8) | result[2];
    }

    if (res == ESP_OK && out_command_status) {
        *out_command_status = (result[0] >> 1) & 0x07;
    }
    if (res == ESP_OK && out_chip_mode) {
        *out_chip_mode = (result[0] >> 4) & 0x07;
    }
#endif
    return ESP_OK;
}

esp_err_t sx126x_clear_irq_status(sx126x_handle_t* handle, uint16_t irq_mask) {
    if (handle == NULL) return ESP_ERR_INVALID_ARG;

    uint8_t parameters[2];
    parameters[0] = (irq_mask >> 8) & 0xFF;
    parameters[1] = irq_mask & 0xFF;
#if 0
    spi_transaction_t t = {
        .flags     = 0,
        .cmd       = SX126X_CMD_CLEAR_IRQ_STATUS,
        .length    = 8 * sizeof(parameters),
        .tx_buffer = parameters,
        .rx_buffer = NULL,
    };
    return spi_device_transmit(handle->device, &t);
#endif
    return 0;
}

esp_err_t sx126x_set_dio2_as_rf_switch_ctrl(sx126x_handle_t* handle, bool enable) {
    if (handle == NULL) return ESP_ERR_INVALID_ARG;
    uint8_t parameter = enable ? 0x01 : 0x00;
    return sx126x_command(handle, SX126X_CMD_SET_DIO2_AS_RF_SWITCH_CTRL, &parameter, NULL, 1);
}

esp_err_t sx126x_set_dio3_as_txco_ctrl(sx126x_handle_t* handle, float voltage, float delay_us) {
    if (handle == NULL) return ESP_ERR_INVALID_ARG;
    uint8_t voltage_value = 0;
    if (voltage >= 1.6) voltage_value = 0x01;
    if (voltage >= 1.7) voltage_value = 0x02;
    if (voltage >= 1.8) voltage_value = 0x03;
    if (voltage >= 2.2) voltage_value = 0x04;
    if (voltage >= 2.7) voltage_value = 0x05;
    if (voltage >= 3.0) voltage_value = 0x06;
    if (voltage >= 3.3) voltage_value = 0x07;
    uint32_t delay_value = (uint32_t)(delay_us / 15.62f);
    if (delay_value > 0x00FFFFFF) return ESP_ERR_INVALID_ARG;  // Delay exceeds maximum value
    uint8_t parameters[4];
    parameters[0] = voltage_value;
    parameters[1] = (delay_value >> 16) & 0xFF;
    parameters[2] = (delay_value >> 8) & 0xFF;
    parameters[3] = delay_value & 0xFF;
    return sx126x_command(handle, SX126X_CMD_SET_DIO3_AS_TXCO_CTRL, parameters, NULL, sizeof(parameters));
}

esp_err_t sx126x_set_rf_frequency(sx126x_handle_t* handle, float frequency) {
    if (handle == NULL) return ESP_ERR_INVALID_ARG;
    const double xtal_frequency  = 32.0f;
    uint32_t     frequency_value = (frequency * ((uint32_t)(1) << 25)) / xtal_frequency;
    uint8_t      frequency_bytes[4];
    frequency_bytes[0] = (frequency_value >> 24) & 0xFF;
    frequency_bytes[1] = (frequency_value >> 16) & 0xFF;
    frequency_bytes[2] = (frequency_value >> 8) & 0xFF;
    frequency_bytes[3] = frequency_value & 0xFF;
    return sx126x_command(handle, SX126X_CMD_SET_RF_FREQUENCY, frequency_bytes, NULL, sizeof(frequency_bytes));
}

esp_err_t sx126x_set_packet_type(sx126x_handle_t* handle, sx126x_packet_type_t packet_type) {
    if (handle == NULL) return ESP_ERR_INVALID_ARG;
    uint8_t packet_type_value = (uint8_t)packet_type;
    return sx126x_command(handle, SX126X_CMD_SET_PACKET_TYPE, &packet_type_value, NULL, 1);
}

esp_err_t sx126x_get_packet_type(sx126x_handle_t* handle, sx126x_packet_type_t* out_packet_type) {
    if (handle == NULL) return ESP_ERR_INVALID_ARG;
    uint8_t   data[2];
    esp_err_t res = sx126x_command(handle, SX126X_CMD_GET_PACKET_TYPE, NULL, data, sizeof(data));
    if (out_packet_type && res == ESP_OK) {
        *out_packet_type = data[1];
    }
    return res;
}

esp_err_t sx126x_set_tx_params(sx126x_handle_t* handle, int8_t power, bool pa_is_high_power, uint32_t ramp_time_us) {
    // You can set the PA mode using the SetPaConfig command, make sure to set the pa_is_high_power flag correctly
    if (handle == NULL) return ESP_ERR_INVALID_ARG;
    if (pa_is_high_power) {
        if (power < -9 || power > 22) {
            return ESP_ERR_INVALID_ARG;  // Power out of range for high power mode
        }
    } else if (power < -17 || power > 14) {
        return ESP_ERR_INVALID_ARG;  // Power out of range for low power mode
    }
    uint8_t ramp_time_value = 0;
    if (ramp_time_us >= 20) ramp_time_value = 0x01;    // 20 us
    if (ramp_time_us >= 40) ramp_time_value = 0x02;    // 40 us
    if (ramp_time_us >= 80) ramp_time_value = 0x03;    // 80 us
    if (ramp_time_us >= 200) ramp_time_value = 0x04;   // 200 us
    if (ramp_time_us >= 800) ramp_time_value = 0x05;   // 800 us
    if (ramp_time_us >= 1700) ramp_time_value = 0x06;  // 1700 us
    if (ramp_time_us >= 3400) ramp_time_value = 0x07;  // 3400 us
    uint8_t parameters[2] = {(uint8_t)(power), ramp_time_value};
    return sx126x_command(handle, SX126X_CMD_SET_TX_PARAMS, parameters, NULL, sizeof(parameters));
}

esp_err_t sx126x_set_modulation_params_lora(sx126x_handle_t* handle, sx126x_lora_spreading_factor_t spreading_factor,
                                            sx126x_lora_bandwidth_t bandwidth, sx126x_lora_coding_rate_t coding_rate,
                                            bool low_data_rate_optimization) {
    if (handle == NULL) return ESP_ERR_INVALID_ARG;
    uint8_t parameters[4] = {(uint8_t)spreading_factor, (uint8_t)bandwidth, (uint8_t)coding_rate,
                             low_data_rate_optimization ? 0x01 : 0x00};
    return sx126x_command(handle, SX126X_CMD_SET_MODULATION_PARAMS, parameters, NULL, sizeof(parameters));
}

esp_err_t sx126x_set_modulation_params_gfsk(sx126x_handle_t* handle, uint32_t bit_rate,
                                            sx1262_gfsk_pulse_shape_t pulse_shape, sx126x_gfsk_bandwidth_t bandwidth,
                                            uint32_t frequency_deviation) {
    if (handle == NULL) return ESP_ERR_INVALID_ARG;
    const float xtal_frequency            = 32.0f;
    uint32_t    bit_rate_value            = (32 * xtal_frequency) / bit_rate;
    uint32_t    frequency_deviation_value = (uint32_t)((frequency_deviation * (1 << 25))) / xtal_frequency;
    uint8_t     parameters[8]             = {(bit_rate_value >> 16) & 0xFF,
                                             (bit_rate_value >> 8) & 0xFF,
                                             bit_rate_value & 0xFF,
                                             (uint8_t)pulse_shape,
                                             (uint8_t)bandwidth,
                                             (frequency_deviation_value >> 16) & 0xFF,
                                             (frequency_deviation_value >> 8) & 0xFF,
                                             frequency_deviation_value & 0xFF};
    return sx126x_command(handle, SX126X_CMD_SET_MODULATION_PARAMS, parameters, NULL, sizeof(parameters));
}

esp_err_t sx126x_set_packet_params_lora(sx126x_handle_t* handle, uint16_t preamble_length, bool fixed_packet_length,
                                        uint8_t payload_length, bool crc_enabled, bool inverted_iq) {
    if (handle == NULL) return ESP_ERR_INVALID_ARG;
    uint8_t parameters[6] = {
        (preamble_length >> 8) & 0xFF,      // Preamble length MSB
        preamble_length & 0xFF,             // Preamble length LSB
        fixed_packet_length ? 0x01 : 0x00,  // Header type (0: variable, 1: fixed)
        payload_length,                     // Payload length
        crc_enabled ? 0x01 : 0x00,          // CRC enabled flag
        inverted_iq ? 0x01 : 0x00,          // Inverted IQ flag
    };
    return sx126x_command(handle, SX126X_CMD_SET_PACKET_PARAMS, parameters, NULL, sizeof(parameters));
}

esp_err_t sx126x_set_packet_params_lora_variable_length(sx126x_handle_t* handle, uint16_t preamble_length,
                                                        bool crc_enabled, bool inverted_iq) {
    return sx126x_set_packet_params_lora(handle, preamble_length, false, 0xFF, crc_enabled, inverted_iq);
}

esp_err_t sx126x_set_packet_params_gfsk(sx126x_handle_t* handle, uint16_t preamble_length,
                                        uint8_t preamble_detector_length, uint8_t sync_word_length,
                                        uint8_t address_compensation, bool packet_length_variable,
                                        uint8_t payload_length, uint8_t crc_type, bool whitening) {
    if (handle == NULL) return ESP_ERR_INVALID_ARG;
    uint8_t parameters[9] = {
        (preamble_length >> 8) & 0xFF,         // Preamble length MSB
        preamble_length & 0xFF,                // Preamble length LSB
        preamble_detector_length,              // Preamble detector length
        sync_word_length,                      // Sync word length
        address_compensation,                  // Address compensation
        packet_length_variable ? 0x01 : 0x00,  // Packet length variable flag
        payload_length,                        // Payload length
        crc_type,                              // CRC type
        whitening ? 0x01 : 0x00                // Whitening flag
    };
    return sx126x_command(handle, SX126X_CMD_SET_PACKET_PARAMS, parameters, NULL, sizeof(parameters));
}

esp_err_t sx126x_set_cad_params(sx126x_handle_t* handle, sx126x_cad_symbol_num_t symbol_num, uint8_t det_peak,
                                uint8_t det_min, bool exit_mode, uint32_t timeout) {
    if (handle == NULL) return ESP_ERR_INVALID_ARG;
    uint8_t parameters[7] = {
        (uint8_t)symbol_num,      //
        det_peak,                 //
        det_min,                  //
        exit_mode ? 0x01 : 0x00,  //
        (timeout >> 16) & 0xFF,   //
        (timeout >> 8) & 0xFF,    //
        timeout & 0xFF,           //
    };
    return sx126x_command(handle, SX126X_CMD_SET_CAD_PARAMS, parameters, NULL, sizeof(parameters));
}

esp_err_t sx126x_set_buffer_base_address(sx126x_handle_t* handle, uint8_t tx_base_address, uint8_t rx_base_address) {
    if (handle == NULL) return ESP_ERR_INVALID_ARG;
    uint8_t parameters[2] = {tx_base_address, rx_base_address};
    return sx126x_command(handle, SX126X_CMD_SET_BUFFER_BASE_ADDRESS, parameters, NULL, sizeof(parameters));
}

esp_err_t sx126x_set_lora_symb_num_timeout(sx126x_handle_t* handle, uint8_t symb_num) {
    if (handle == NULL) return ESP_ERR_INVALID_ARG;
    return sx126x_command(handle, SX126X_CMD_SET_LORA_SYMB_NUM_TIMEOUT, &symb_num, NULL, 1);
}

esp_err_t sx126x_get_status(sx126x_handle_t* handle, uint8_t* out_command_status, uint8_t* out_chip_mode) {
    if (handle == NULL) return ESP_ERR_INVALID_ARG;
    uint8_t   status = 0;
    esp_err_t res    = sx126x_command(handle, SX126X_CMD_GET_STATUS, NULL, &status, 1);
    if (res == ESP_OK && out_command_status) *out_command_status = (status >> 1) & 0x07;
    if (res == ESP_OK && out_chip_mode) *out_chip_mode = (status >> 4) & 0x07;
    return res;
}

esp_err_t sx126x_get_rx_buffer_status(sx126x_handle_t* handle, uint8_t* out_payload_length,
                                      uint8_t* out_start_pointer) {
    if (handle == NULL) return ESP_ERR_INVALID_ARG;
    uint8_t   result[3];
    esp_err_t res = sx126x_command(handle, SX126X_CMD_GET_RX_BUFFER_STATUS, NULL, result, sizeof(result));
    if (res == ESP_OK && out_payload_length) *out_payload_length = result[1];
    if (res == ESP_OK && out_start_pointer) *out_start_pointer = result[2];
    return res;
}

esp_err_t sx126x_get_packet_status_lora(sx126x_handle_t* handle, uint8_t* out_rx_status, uint8_t* out_rssi_sync,
                                        uint8_t* out_rssi_avg) {
    if (handle == NULL) return ESP_ERR_INVALID_ARG;
    uint8_t   result[4] = {0};
    esp_err_t res       = sx126x_command(handle, SX126X_CMD_GET_PACKET_STATUS, NULL, result, sizeof(result));
    if (res == ESP_OK && out_rx_status) *out_rx_status = result[1];
    if (res == ESP_OK && out_rssi_sync) *out_rssi_sync = result[2];
    if (res == ESP_OK && out_rssi_avg) *out_rssi_avg = result[3];
    return res;
}

esp_err_t sx126x_get_packet_status_gfsk(sx126x_handle_t* handle, uint8_t* out_rx_status, uint8_t* out_rssi_sync,
                                        uint8_t* out_rssi_avg) {
    if (handle == NULL) return ESP_ERR_INVALID_ARG;
    uint8_t   result[4] = {0};
    esp_err_t res       = sx126x_command(handle, SX126X_CMD_GET_PACKET_STATUS, NULL, result, sizeof(result));
    if (res == ESP_OK && out_rx_status) *out_rx_status = result[1];
    if (res == ESP_OK && out_rssi_sync) *out_rssi_sync = result[2];
    if (res == ESP_OK && out_rssi_avg) *out_rssi_avg = result[3];
    return res;
}

esp_err_t sx126x_get_rssi_inst(sx126x_handle_t* handle, float* out_signal_power) {
    if (handle == NULL) return ESP_ERR_INVALID_ARG;
    uint8_t   result[2];
    esp_err_t res = sx126x_command(handle, SX126X_CMD_GET_RSSI_INST, NULL, result, sizeof(result));
    if (res == ESP_OK && out_signal_power) *out_signal_power = -result[1] / 2.0f;
    return res;
}

esp_err_t sx126x_get_stats_lora(sx126x_handle_t* handle, uint16_t* out_nb_pkt_received, uint16_t* out_nb_pkt_crc_error,
                                uint16_t* out_nb_pkt_header_error) {
    if (handle == NULL) return ESP_ERR_INVALID_ARG;
    uint8_t   result[7];
    esp_err_t res = sx126x_command(handle, SX126X_CMD_GET_STATS, NULL, result, sizeof(result));
    if (res == ESP_OK && out_nb_pkt_received) *out_nb_pkt_received = (result[1] << 8) | result[2];
    if (res == ESP_OK && out_nb_pkt_crc_error) *out_nb_pkt_crc_error = (result[3] << 8) | result[4];
    if (res == ESP_OK && out_nb_pkt_header_error) *out_nb_pkt_header_error = (result[5] << 8) | result[6];
    return res;
}

esp_err_t sx126x_get_stats_gfsk(sx126x_handle_t* handle, uint16_t* out_nb_pkt_received, uint16_t* out_nb_pkt_crc_error,
                                uint16_t* out_nb_pkt_length_error) {
    if (handle == NULL) return ESP_ERR_INVALID_ARG;
    uint8_t   result[7];
    esp_err_t res = sx126x_command(handle, SX126X_CMD_GET_STATS, NULL, result, sizeof(result));
    if (res == ESP_OK && out_nb_pkt_received) *out_nb_pkt_received = (result[1] << 8) | result[2];
    if (res == ESP_OK && out_nb_pkt_crc_error) *out_nb_pkt_crc_error = (result[3] << 8) | result[4];
    if (res == ESP_OK && out_nb_pkt_length_error) *out_nb_pkt_length_error = (result[5] << 8) | result[6];
    return res;
}

esp_err_t sx126x_reset_stats(sx126x_handle_t* handle) {
    if (handle == NULL) return ESP_ERR_INVALID_ARG;
    const uint8_t dummy[6] = {0};
    return sx126x_command(handle, SX126X_CMD_RESET_STATS, dummy, NULL, sizeof(dummy));
}

esp_err_t sx126x_get_device_errors(sx126x_handle_t* handle, uint16_t* out_errors) {
    if (handle == NULL) return ESP_ERR_INVALID_ARG;
    uint8_t   result[3];
    esp_err_t res = sx126x_command(handle, SX126X_CMD_GET_DEVICE_ERRORS, NULL, result, sizeof(result));
    if (res == ESP_OK && out_errors) {
        *out_errors = (result[1] << 8) | result[2];
    }
    return res;
}

esp_err_t sx126x_clear_device_errors(sx126x_handle_t* handle, uint8_t* out_command_status, uint8_t* out_chip_mode)
{
    if (handle == NULL) return ESP_ERR_INVALID_ARG;

    uint8_t result[2] = {0};
#if 0
    spi_transaction_t t = {
        .flags     = 0,
        .cmd       = SX126X_CMD_CLEAR_DEVICE_ERRORS,
        .length    = 8 * 2,
        .tx_buffer = NULL,
        .rx_buffer = result,
    };

    esp_err_t res = spi_device_transmit(handle->device, &t);
    if (res != ESP_OK) {
        return res;
    }

    if (res == ESP_OK && out_command_status) *out_command_status = (result[0] >> 1) & 0x07;
    if (res == ESP_OK && out_chip_mode) *out_chip_mode = (result[0] >> 4) & 0x07;
#endif
    return ESP_OK;
}

esp_err_t sx126x_write_register(sx126x_handle_t* handle, uint16_t address, const uint8_t* values, size_t length)
{
    if (handle == NULL) return ESP_ERR_INVALID_ARG;
#if 0
    spi_transaction_ext_t t = {
        .base =
            {
                .flags     = SPI_TRANS_VARIABLE_ADDR,
                .cmd       = SX126X_CMD_WRITE_REGISTER,
                .addr      = address,
                .length    = length * 8,
                .tx_buffer = values,
                .rx_buffer = NULL,
            },
        .address_bits = 16,
    };
    return spi_device_transmit(handle->device, (spi_transaction_t*)&t);
#endif
    return 0;
}

esp_err_t sx126x_read_register(sx126x_handle_t* handle, uint16_t address, uint8_t* out_values, size_t length)
{
    if (handle == NULL)
    	return ESP_ERR_INVALID_ARG;

#if 0
    spi_transaction_ext_t t = {
        .base =
            {
                .flags     = SPI_TRANS_VARIABLE_ADDR | SPI_TRANS_VARIABLE_DUMMY,
                .cmd       = SX126X_CMD_READ_REGISTER,
                .addr      = address,
                .length    = length * 8,
                .tx_buffer = NULL,
                .rx_buffer = out_values,
            },
        .address_bits = 16,
        .dummy_bits   = 8,
    };
    return spi_device_transmit(handle->device, (spi_transaction_t*)&t);
#endif
    return 0;
}

esp_err_t sx126x_write_buffer(sx126x_handle_t* handle, uint8_t offset, const uint8_t* values, size_t length)
{
    if (handle == NULL)
    	return ESP_ERR_INVALID_ARG;
#if 0
    spi_transaction_ext_t t = {
        .base =
            {
                .flags     = SPI_TRANS_VARIABLE_ADDR,
                .cmd       = SX126X_CMD_WRITE_BUFFER,
                .addr      = offset,
                .length    = length * 8,
                .tx_buffer = values,
                .rx_buffer = NULL,
            },
        .address_bits = 8,
    };
    return spi_device_transmit(handle->device, (spi_transaction_t*)&t);
#endif
}

esp_err_t sx126x_read_buffer(sx126x_handle_t* handle, uint8_t offset, uint8_t* out_values, size_t length)
{
    if (handle == NULL)
    	return ESP_ERR_INVALID_ARG;
#if 0
    spi_transaction_ext_t t = {
        .base =
            {
                .flags     = SPI_TRANS_VARIABLE_ADDR | SPI_TRANS_VARIABLE_DUMMY,
                .cmd       = SX126X_CMD_READ_BUFFER,
                .addr      = offset,
                .length    = length * 8,
                .tx_buffer = NULL,
                .rx_buffer = out_values,
            },
        .address_bits = 8,
        .dummy_bits   = 8,
    };
    return spi_device_transmit(handle->device, (spi_transaction_t*)&t);
#endif
    return 0;
}

// Public functions - registers

esp_err_t sx126x_read_version_string(sx126x_handle_t* handle, char* out_buffer, size_t buffer_size) {
    if (out_buffer == NULL || buffer_size < 1) {
        return ESP_ERR_INVALID_ARG;
    }
    memset(out_buffer, '\0', buffer_size);
    if (buffer_size > SX126X_VERSION_STRING_LENGTH) {
        buffer_size = SX126X_VERSION_STRING_LENGTH;
    }
    return sx126x_read_register(handle, SX126X_REG_VERSION_STRING, (uint8_t*)out_buffer, buffer_size);
}

esp_err_t sx126x_set_sync_word_adv(sx126x_handle_t* handle, uint8_t sync_word, uint8_t control_bits) {
    uint8_t sync_word_buffer[2] = {(uint8_t)((sync_word & 0xF0) | ((control_bits & 0xF0) >> 4)),
                                   (uint8_t)(((sync_word & 0x0F) << 4) | (control_bits & 0x0F))};
    return sx126x_write_register(handle, SX126X_REG_LORA_SYNC_WORD_MSB, sync_word_buffer, 2);
}

esp_err_t sx126x_set_sync_word(sx126x_handle_t* handle, uint8_t sync_word) {
    return sx126x_set_sync_word_adv(handle, sync_word, SX126X_DEFAULT_SYNC_WORD_CONTROL_BITS);
}

// Public functions - management & control

esp_err_t sx1262_reset(sx126x_handle_t* handle) {
    if (handle == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
#if 0
    esp_err_t res = gpio_set_level(handle->reset, 0);
    if (res != ESP_OK) {
        return res;
    }
    vTaskDelay(pdMS_TO_TICKS(10));
    res = gpio_set_level(handle->reset, 1);
    if (res != ESP_OK) {
        return res;
    }
    vTaskDelay(pdMS_TO_TICKS(10));
#endif
    return ESP_OK;
}

esp_err_t sx126x_init(sx126x_handle_t* handle, int spi_host_id, int nss, int reset,
                      int dio1, int busy) {
    esp_err_t res;

    if (!handle)
    	return ESP_ERR_INVALID_ARG;

    handle->timeout = pdMS_TO_TICKS(1000);

    handle->busy_semaphore = xSemaphoreCreateBinary();
    if (handle->busy_semaphore == NULL) {
        return ESP_ERR_NO_MEM;
    }
    handle->interrupt_semaphore = xSemaphoreCreateBinary();
    if (handle->interrupt_semaphore == NULL) {
        vSemaphoreDelete(handle->busy_semaphore);
        return ESP_ERR_NO_MEM;
    }
#if 0
    spi_device_interface_config_t devcfg = {
        .command_bits   = 8,        // SX1262 uses 8-bit commands
        .clock_speed_hz = 1000000,  // SX1262 support maximum 16MHz SPI clock
        .mode           = 0,
        .spics_io_num   = nss,
        .queue_size     = 8,
    };
    res = spi_bus_add_device(spi_host_id, &devcfg, &handle->device);
    if (res != ESP_OK) {
        return res;
    }

    gpio_config_t gpio_reset_conf = {
        .intr_type    = GPIO_INTR_DISABLE,
        .mode         = GPIO_MODE_OUTPUT,
        .pin_bit_mask = BIT64(reset),
        .pull_down_en = 0,
        .pull_up_en   = 0,
    };
    ESP_RETURN_ON_ERROR(gpio_config(&gpio_reset_conf), TAG, "Failed to configure reset pin");

    handle->reset = reset;

    gpio_config_t gpio_dio1_conf = {
        .intr_type    = GPIO_INTR_POSEDGE,
        .mode         = GPIO_MODE_INPUT,
        .pin_bit_mask = BIT64(dio1),
        .pull_down_en = 1,
        .pull_up_en   = 0,
    };
    ESP_RETURN_ON_ERROR(gpio_config(&gpio_dio1_conf), TAG, "Failed to configure dio1 pin");

    ESP_RETURN_ON_ERROR(gpio_isr_handler_add(dio1, sx1262_dio1_handler, (void*)handle), TAG,
                        "Failed to add interrupt handler for dio1 pin");

    handle->dio1 = dio1;

    gpio_config_t gpio_busy_conf = {
        .intr_type    = GPIO_INTR_ANYEDGE,
        .mode         = GPIO_MODE_INPUT,
        .pin_bit_mask = BIT64(busy),
        .pull_down_en = 0,
        .pull_up_en   = 1,
    };
    ESP_RETURN_ON_ERROR(gpio_config(&gpio_busy_conf), TAG, "Failed to configure busy pin");
#endif
    ESP_RETURN_ON_ERROR(gpio_isr_handler_add(busy, sx1262_busy_handler, (void*)handle), TAG,
                        "Failed to add interrupt handler for busy pin");

    handle->busy = busy;

    res = sx1262_reset(handle);

    return res;
}

bool sx126x_is_busy(sx126x_handle_t* handle)
{
    if (handle == NULL)
    	return false;

//!    return gpio_get_level(handle->busy) == 1;
    return 0;
}

esp_err_t sx126x_irq_wait(sx126x_handle_t* handle, TickType_t timeout)
{
    if (handle == NULL)
    	return ESP_ERR_INVALID_ARG;

    if (sx126x_get_irq_state(handle))
    {
        return ESP_OK;  // Interrupt line already low
    }

    if (xSemaphoreTake(handle->interrupt_semaphore, timeout) == pdTRUE)
    {
        return ESP_OK;
    }
    else
    {
        return ESP_ERR_TIMEOUT;
    }
}

bool sx126x_get_irq_state(sx126x_handle_t* handle)
{
    if (handle == NULL)
    	return false;

//!    int level = gpio_get_level(handle->dio1);

//!    return level == 1;
    return 0;
}

#endif
