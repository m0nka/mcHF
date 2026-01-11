//
// Implementation provided by:
// https://github.com/Nicolai-Electronics/esp32-component-sx126x
//
#ifndef __SX126X_H
#define __SX126X_H
//#pragma once
//#include <stddef.h>
//#include <stdint.h>
//#include "driver/gpio.h"
//#include "driver/spi_common.h"
//#include "driver/spi_master.h"
#include "esp_err.h"

//#ifndef esp_err_t
//typedef	int	esp_err_t;
//#endif

// Commands
#define SX126X_CMD_SET_SLEEP                  0x84
#define SX126X_CMD_SET_STANDBY                0x80
#define SX126X_CMD_SET_FS                     0xC1
#define SX126X_CMD_SET_TX                     0x83
#define SX126X_CMD_SET_RX                     0x82
#define SX126X_CMD_STOP_TIMER_ON_PREAMBLE     0x9F
#define SX126X_CMD_SET_RX_DUTY_CYCLE          0x94
#define SX126X_CMD_SET_CAD                    0xC5
#define SX126X_CMD_SET_TX_CONTINUOUS_WAVE     0xD1
#define SX126X_CMD_SET_TX_INFINITE_PREAMBLE   0xD2
#define SX126X_CMD_SET_REGULATOR_MODE         0x96
#define SX126X_CMD_CALIBRATE                  0x89
#define SX126X_CMD_CALIBRATE_IMAGE            0x98
#define SX126X_CMD_SET_PA_CONFIG              0x95
#define SX126X_CMD_SET_RX_TX_FALLBACK_MODE    0x93
#define SX126X_CMD_WRITE_REGISTER             0x0D
#define SX126X_CMD_READ_REGISTER              0x1D
#define SX126X_CMD_WRITE_BUFFER               0x0E
#define SX126X_CMD_READ_BUFFER                0x1E
#define SX126X_CMD_SET_DIO_IRQ_PARAMS         0x08
#define SX126X_CMD_GET_IRQ_STATUS             0x12
#define SX126X_CMD_CLEAR_IRQ_STATUS           0x02
#define SX126X_CMD_SET_DIO2_AS_RF_SWITCH_CTRL 0x9D
#define SX126X_CMD_SET_DIO3_AS_TXCO_CTRL      0x97
#define SX126X_CMD_SET_RF_FREQUENCY           0x86
#define SX126X_CMD_SET_PACKET_TYPE            0x8A
#define SX126X_CMD_GET_PACKET_TYPE            0x11
#define SX126X_CMD_SET_TX_PARAMS              0x8E
#define SX126X_CMD_SET_MODULATION_PARAMS      0x8B
#define SX126X_CMD_SET_PACKET_PARAMS          0x8C
#define SX126X_CMD_SET_CAD_PARAMS             0x88
#define SX126X_CMD_SET_LORA_SYMB_NUM_TIMEOUT  0xA0
#define SX126X_CMD_GET_STATUS                 0xC0
#define SX126X_CMD_GET_RX_BUFFER_STATUS       0x13
#define SX126X_CMD_GET_PACKET_STATUS          0x14
#define SX126X_CMD_GET_RSSI_INST              0x15
#define SX126X_CMD_GET_STATS                  0x10
#define SX126X_CMD_RESET_STATS                0x00
#define SX126X_CMD_GET_DEVICE_ERRORS          0x17
#define SX126X_CMD_CLEAR_DEVICE_ERRORS        0x07
#define SX126X_CMD_SET_BUFFER_BASE_ADDRESS    0x8F

// Registers
#define SX126X_REG_RX_GAIN_RETENTION(X)             (0x029F + (X))  // 0-2
#define SX126X_REG_VERSION_STRING                   0x0320
#define SX126X_REG_HOPPING_ENABLE                   0x0385
#define SX126X_REG_LR_FHSS_PACKET_LENGTH            0x0386
#define SX126X_REG_LR_FHSS_NUM_HOPPING_BLOCKS       0x0387
#define SX126X_REG_LR_FHSS_NUM_SYMBOLS_FREQX_MSB(X) (0x0388 + (X) * 6)
#define SX126X_REG_LR_FHSS_NUM_SYMBOLS_FREQX_LSB(X) (0x0389 + (X) * 6)
#define SX126X_REG_LR_FHSS_FREQX_0(X)               (0x038A + (X) * 6)
#define SX126X_REG_LR_FHSS_FREQX_1(X)               (0x038B + (X) * 6)
#define SX126X_REG_LR_FHSS_FREQX_2(X)               (0x038C + (X) * 6)
#define SX126X_REG_LR_FHSS_FREQX_3(X)               (0x038D + (X) * 6)
#define SX126X_REG_SPECTRAL_SCAN_RESULT             0x0401
#define SX126X_REG_DIOX_OUT_ENABLE                  0x0580
#define SX126X_REG_DIOX_DRIVE_STRENGTH              0x0582
#define SX126X_REG_DIOX_IN_ENABLE                   0x0583
#define SX126X_REG_DIOX_PULL_UP_CTRL                0x0584
#define SX126X_REG_DIOX_PULL_DOWN_CTRL              0x0585
#define SX126X_REG_TX_BITBANG_ENABLE_0              0x0587
#define SX126X_REG_PATCH_UPDATE_ENABLE              0x0610
#define SX126X_REG_TX_BITBANG_ENABLE_1              0x0680
#define SX126X_REG_WHITENING_INITIAL_MSB            0x06B8
#define SX126X_REG_WHITENING_INITIAL_LSB            0x06B9
#define SX126X_REG_RX_TX_PLD_LEN                    0x06BB
#define SX126X_REG_CRC_INITIAL_MSB                  0x06BC
#define SX126X_REG_CRC_INITIAL_LSB                  0x06BD
#define SX126X_REG_CRC_POLYNOMIAL_MSB               0x06BE
#define SX126X_REG_CRC_POLYNOMIAL_LSB               0x06BF
#define SX126X_REG_SYNC_WORD(X)                     (0x06C0 + (X))  // 0-6
#define SX126X_REG_NODE_ADDRESS                     0x06CD
#define SX126X_REG_BROADCAST_ADDRESS                0x06CE
#define SX126X_REG_PAYLOAD_LENGTH                   0x0702
#define SX126X_REG_PACKET_PARAMS                    0x0704
#define SX126X_REG_LORA_SYNC_TIMEOUT                0x0706
#define SX126X_REG_IQ_CONFIG                        0x0736
#define SX126X_REG_LORA_SYNC_WORD_MSB               0x0740
#define SX126X_REG_LORA_SYNC_WORD_LSB               0x0741
#define SX126X_REG_FREQ_ERROR                       0x076B
#define SX126X_REG_SPECTRAL_SCAN_STATUS             0x07CD
#define SX126X_REG_RX_ADDR_PTR                      0x0803
#define SX126X_REG_RANDOM_NUMBER_0                  0x0819
#define SX126X_REG_RANDOM_NUMBER_1                  0x081A
#define SX126X_REG_RANDOM_NUMBER_2                  0x081B
#define SX126X_REG_RANDOM_NUMBER_3                  0x081C
#define SX126X_REG_SENSITIVITY_CONFIG               0x0889
#define SX126X_REG_RF_FREQUENCY(X)                  (0x088B + (X))  // 0-3
#define SX126X_REG_RSSI_AVG_WINDOW                  0x089B
#define SX126X_REG_RX_GAIN                          0x08AC
#define SX126X_REG_TX_CLAMP_CONFIG                  0x08D8
#define SX126X_REG_ANA_LNA                          0x08E2
#define SX126X_REG_LNA_CAP_TUNE_N                   0x08E3
#define SX126X_REG_LNA_CAP_TUNE_P                   0x08E4
#define SX126X_REG_ANA_MIXER                        0x08E5
#define SX126X_REG_OCP_CONFIGURATION                0x08E7
#define SX126X_REG_RTC_CTRL                         0x0902
#define SX126X_REG_XTA_TRIM                         0x0911
#define SX126X_REG_XTB_TRIM                         0x0912
#define SX126X_REG_DIO3_OUT_VOLTAGE_CTRL            0x0920
#define SX126X_REG_EVENT_MASK                       0x0944
#define SX126X_REG_PATCH_MEMORY_BASE                0x8000

#define SX126X_VERSION_STRING_LENGTH 16

//RADIOLIB_SX126X_REG_LORA_SYNC_WORD_MSB + LSB
#define RADIOLIB_SX126X_SYNC_WORD_PUBLIC                        0x34        // actually 0x3444  NOTE: The low nibbles in each byte (0x_4_4) are masked out since apparently, they're reserved.
#define RADIOLIB_SX126X_SYNC_WORD_PRIVATE                       0x12        // actually 0x1424        You couldn't make this up if you tried.


// Chip mode (get status)
#define SX126X_CHIP_MODE_STDBY_RC   0x02
#define SX126X_CHIP_MODE_STDBY_XOSC 0x03
#define SX126X_CHIP_MODE_FS         0x04
#define SX126X_CHIP_MODE_RX         0x05
#define SX126X_CHIP_MODE_TX         0x06

// Command status (get status)
#define SX126X_COMMAND_STATUS_DATA_AVAILABLE 0x02
#define SX126X_COMMAND_STATUS_TIMEOUT        0x03
#define SX126X_COMMAND_STATUS_INVALID        0x04
#define SX126X_COMMAND_STATUS_FAILED         0x05
#define SX126X_COMMAND_STATUS_TX_DONE        0x06

// Sync word
#define SX126X_DEFAULT_SYNC_WORD_CONTROL_BITS 0x44

// IRQ

#define SX126X_IRQ_TX_DONE           (1 << 0)
#define SX126X_IRQ_RX_DONE           (1 << 1)
#define SX126X_IRQ_PREAMBLE_DETECTED (1 << 2)
#define SX126X_IRQ_SYNC_WORD_VALID   (1 << 3)
#define SX126X_IRQ_HEADER_VALID      (1 << 4)
#define SX126X_IRQ_HEADER_ERROR      (1 << 5)
#define SX126X_IRQ_CRC_ERROR         (1 << 6)
#define SX126X_IRQ_CAD_DONE          (1 << 7)
#define SX126X_IRQ_CAD_DETECTED      (1 << 8)
#define SX126X_IRQ_TIMEOUT           (1 << 9)
#define SX126X_IRQ_LRFHSSHOP         (1 << 14)
#define SX126X_IRQ_ALL               0xFFFF

// these macros are usually defined by Arduino, but some platforms undef them, so its safer to use our own
#define RADIOLIB_MIN(a,b)			((a)<(b)?(a):(b))
#define RADIOLIB_MAX(a,b)			((a)>(b)?(a):(b))
#define RADIOLIB_ABS(x)         	((x)>0?(x):-(x))

typedef struct {
    //spi_device_handle_t
	int 		 device;
    int          reset;
    int          dio1;
    int          busy;
    TickType_t          timeout;
    SemaphoreHandle_t   busy_semaphore;
    SemaphoreHandle_t   interrupt_semaphore;
} sx126x_handle_t;

typedef enum {
    SX126X_PACKET_TYPE_GFSK    = 0x00,
    SX126X_PACKET_TYPE_LORA    = 0x01,
    SX126X_PACKET_TYPE_LR_FHSS = 0x03,
} sx126x_packet_type_t;

typedef enum {
    SX126X_FALLBACK_MODE_FS         = 0x40,
    SX126X_FALLBACK_MODE_STDBY_XOSC = 0x30,
    SX126X_FALLBACK_MODE_STDBY_RC   = 0x20,
} sx126x_fallback_mode_t;

typedef enum {
    SX126X_CAD_ON_1_SYMB  = 0x00,
    SX126X_CAD_ON_2_SYMB  = 0x01,
    SX126X_CAD_ON_4_SYMB  = 0x02,
    SX126X_CAD_ON_8_SYMB  = 0x03,
    SX126X_CAD_ON_16_SYMB = 0x04,
} sx126x_cad_symbol_num_t;

typedef enum {
    SK126X_GFSK_PULSE_SHAPE_NO_FILTER       = 0x00,
    SX126X_GFSK_PULSE_SHAPE_GAUSSIAN_BT_0_3 = 0x08,
    SX126X_GFSK_PULSE_SHAPE_GAUSSIAN_BT_0_5 = 0x09,
    SX126X_GFSK_PULSE_SHAPE_GAUSSIAN_BT_0_7 = 0x0A,
    SX126X_GFSK_PULSE_SHAPE_GAUSSIAN_BT_1_0 = 0x0B,
} sx1262_gfsk_pulse_shape_t;

typedef enum {
    SX126X_GFSK_BANDWIDTH_4800   = 0x1F,
    SX126X_GFSK_BANDWIDTH_5800   = 0x17,
    SX126X_GFSK_BANDWIDTH_7300   = 0x0F,
    SX126X_GFSK_BANDWIDTH_9700   = 0x1E,
    SX126X_GFSK_BANDWIDTH_11700  = 0x16,
    SX126X_GFSK_BANDWIDTH_14600  = 0x0E,
    SX126X_GFSK_BANDWIDTH_19500  = 0x1D,
    SX126X_GFSK_BANDWIDTH_23400  = 0x15,
    SX126X_GFSK_BANDWIDTH_29300  = 0x0D,
    SX126X_GFSK_BANDWIDTH_39000  = 0x1C,
    SX126X_GFSK_BANDWIDTH_46900  = 0x14,
    SX126X_GFSK_BANDWIDTH_58600  = 0x0C,
    SX126X_GFSK_BANDWIDTH_78200  = 0x1B,
    SX126X_GFSK_BANDWIDTH_93800  = 0x13,
    SX126X_GFSK_BANDWIDTH_117300 = 0x0B,
    SX126X_GFSK_BANDWIDTH_156200 = 0x1A,
    SX126X_GFSK_BANDWIDTH_187200 = 0x12,
    SX126X_GFSK_BANDWIDTH_234300 = 0x0A,
    SX126X_GFSK_BANDWIDTH_312000 = 0x19,
    SX126X_GFSK_BANDWIDTH_373600 = 0x11,
    SX126X_GFSK_BANDWIDTH_467000 = 0x09,
} sx126x_gfsk_bandwidth_t;

typedef enum {
    SX126X_LORA_SPREADING_FACTOR_5  = 0x05,
    SX126X_LORA_SPREADING_FACTOR_6  = 0x06,
    SX126X_LORA_SPREADING_FACTOR_7  = 0x07,
    SX126X_LORA_SPREADING_FACTOR_8  = 0x08,
    SX126X_LORA_SPREADING_FACTOR_9  = 0x09,
    SX126X_LORA_SPREADING_FACTOR_10 = 0x0A,
    SX126X_LORA_SPREADING_FACTOR_11 = 0x0B,
    SX126X_LORA_SPREADING_FACTOR_12 = 0x0C,
} sx126x_lora_spreading_factor_t;

typedef enum {
    SX126X_LORA_BANDWIDTH_7   = 0x00,
    SX126X_LORA_BANDWIDTH_10  = 0x08,
    SX126X_LORA_BANDWIDTH_15  = 0x01,
    SX126X_LORA_BANDWIDTH_20  = 0x09,
    SX126X_LORA_BANDWIDTH_31  = 0x02,
    SX126X_LORA_BANDWIDTH_41  = 0x0A,
    SX126X_LORA_BANDWIDTH_62  = 0x03,
    SX126X_LORA_BANDWIDTH_125 = 0x04,
    SX126X_LORA_BANDWIDTH_250 = 0x05,
    SX126X_LORA_BANDWIDTH_500 = 0x06,
} sx126x_lora_bandwidth_t;

typedef enum {
    SX126X_LORA_CODING_RATE_4_5    = 0x01,
    SX126X_LORA_CODING_RATE_4_6    = 0x02,
    SX126X_LORA_CODING_RATE_4_7    = 0x03,
    SX126X_LORA_CODING_RATE_4_8    = 0x04,
    SX126X_LORA_CODING_RATE_4_5_LI = 0x05,
    SX126X_LORA_CODING_RATE_4_6_LI = 0x06,
    SX126X_LORA_CODING_RATE_4_8_LI = 0x07,
} sx126x_lora_coding_rate_t;

typedef enum {
    SX126X_ERROR_RC64K_CALIB_ERR = 0x0001,
    SX126X_ERROR_RC13M_CALIB_ERR = 0x0002,
    SX126X_ERROR_PLL_CALIB_ERR   = 0x0004,
    SX126X_ERROR_ADC_CALIB_ERR   = 0x0008,
    SX126X_ERROR_IMG_CALIB_ERR   = 0x0010,
    SX126X_XOSC_START_ERR        = 0x0020,
    SX126X_PLL_LOCK_ERR          = 0x0040,
    SX126X_PA_RAMP_ERR           = 0x0100,
} sx126x_error_t;

// Exti IRQs
void sx1262_busy_handler(void* pvParameters);
void sx1262_dio1_handler(void* pvParameters);

// Commands
esp_err_t sx126x_set_op_mode_sleep(sx126x_handle_t* handle, bool warm_start, bool rtc_timeout_disable);
esp_err_t sx126x_set_op_mode_standby(sx126x_handle_t* handle, bool use_xosc);
esp_err_t sx126x_set_op_mode_fs(sx126x_handle_t* handle);
esp_err_t sx126x_set_op_mode_tx(sx126x_handle_t* handle);
esp_err_t sx126x_set_op_mode_rx(sx126x_handle_t* handle);
esp_err_t sx126x_stop_timer_on_preamble(sx126x_handle_t* handle, bool stop);
esp_err_t sx126x_set_rx_duty_cycle(sx126x_handle_t* handle, uint32_t rx_period, uint32_t sleep_period);
esp_err_t sx126x_set_cad(sx126x_handle_t* handle);
esp_err_t sx126x_tx_continuous_wave(sx126x_handle_t* handle);
esp_err_t sx126x_set_tx_infinite_preamble(sx126x_handle_t* handle);
esp_err_t sx126x_set_regulator_mode(sx126x_handle_t* handle, bool use_dc_dc);
esp_err_t sx126x_calibrate(sx126x_handle_t* handle, bool rc64k, bool rc13m, bool pll, bool adc_pulse, bool adc_bulk_n,
                           bool adc_bulk_p, bool image);
esp_err_t sx126x_calibrate_image(sx126x_handle_t* handle, uint8_t frequency1, uint8_t frequency2);
esp_err_t sx126x_set_pa_config(sx126x_handle_t* handle, uint8_t pa_duty_cycle, uint8_t hp_max, bool is_sx1261);
esp_err_t sx126x_set_rx_tx_fallback_mode(sx126x_handle_t* handle, sx126x_fallback_mode_t fallback_mode);
esp_err_t sx126x_write_register(sx126x_handle_t* handle, uint16_t address, const uint8_t* values, size_t length);
esp_err_t sx126x_read_register(sx126x_handle_t* handle, uint16_t address, uint8_t* out_values, size_t length);
esp_err_t sx126x_write_buffer(sx126x_handle_t* handle, uint8_t offset, const uint8_t* values, size_t length);
esp_err_t sx126x_read_buffer(sx126x_handle_t* handle, uint8_t offset, uint8_t* out_values, size_t length);
esp_err_t sx126x_set_dio_irq_params(sx126x_handle_t* handle, uint16_t irq_mask, uint16_t dio1_mask, uint16_t dio2_mask,
                                    uint16_t dio3_mask);
esp_err_t sx126x_get_irq_status(sx126x_handle_t* handle, uint16_t* out_irq_status, uint8_t* out_command_status,
                                uint8_t* out_chip_mode);
esp_err_t sx126x_clear_irq_status(sx126x_handle_t* handle, uint16_t irq_mask);
esp_err_t sx126x_set_dio2_as_rf_switch_ctrl(sx126x_handle_t* handle, bool enable);
esp_err_t sx126x_set_dio3_as_txco_ctrl(sx126x_handle_t* handle, float voltage, float delay_us);
esp_err_t sx126x_set_rf_frequency(sx126x_handle_t* handle, float frequency);
esp_err_t sx126x_set_packet_type(sx126x_handle_t* handle, sx126x_packet_type_t packet_type);
esp_err_t sx126x_get_packet_type(sx126x_handle_t* handle, sx126x_packet_type_t* out_packet_type);
esp_err_t sx126x_set_tx_params(sx126x_handle_t* handle, int8_t power, bool pa_is_high_power, uint32_t ramp_time_us);
esp_err_t sx126x_set_modulation_params_lora(sx126x_handle_t* handle, sx126x_lora_spreading_factor_t spreading_factor,
                                            sx126x_lora_bandwidth_t bandwidth, sx126x_lora_coding_rate_t coding_rate,
                                            bool low_data_rate_optimization);
esp_err_t sx126x_set_modulation_params_gfsk(sx126x_handle_t* handle, uint32_t bit_rate,
                                            sx1262_gfsk_pulse_shape_t pulse_shape, sx126x_gfsk_bandwidth_t bandwidth,
                                            uint32_t frequency_deviation);
esp_err_t sx126x_set_packet_params_lora(sx126x_handle_t* handle, uint16_t preamble_length, bool fixed_packet_length,
                                        uint8_t payload_length, bool crc_enabled, bool inverted_iq);
esp_err_t sx126x_set_packet_params_lora_variable_length(sx126x_handle_t* handle, uint16_t preamble_length,
                                                        bool crc_enabled, bool inverted_iq);
esp_err_t sx126x_set_packet_params_gfsk(sx126x_handle_t* handle, uint16_t preamble_length,
                                        uint8_t preamble_detector_length, uint8_t sync_word_length,
                                        uint8_t address_compensation, bool packet_length_variable,
                                        uint8_t payload_length, uint8_t crc_type, bool whitening);
esp_err_t sx126x_set_cad_params(sx126x_handle_t* handle, sx126x_cad_symbol_num_t symbol_num, uint8_t det_peak,
                                uint8_t det_min, bool exit_mode, uint32_t timeout);
esp_err_t sx126x_set_buffer_base_address(sx126x_handle_t* handle, uint8_t tx_base_address, uint8_t rx_base_address);
esp_err_t sx126x_set_lora_symb_num_timeout(sx126x_handle_t* handle, uint8_t symb_num);
esp_err_t sx126x_get_status(sx126x_handle_t* handle, uint8_t* out_command_status, uint8_t* out_chip_mode);
esp_err_t sx126x_get_rx_buffer_status(sx126x_handle_t* handle, uint8_t* out_payload_length, uint8_t* out_start_pointer);
esp_err_t sx126x_get_packet_status_lora(sx126x_handle_t* handle, uint8_t* out_rx_status, uint8_t* out_rssi_sync,
                                        uint8_t* out_rssi_avg);
esp_err_t sx126x_get_packet_status_gfsk(sx126x_handle_t* handle, uint8_t* out_rx_status, uint8_t* out_rssi_sync,
                                        uint8_t* out_rssi_avg);
esp_err_t sx126x_get_rssi_inst(sx126x_handle_t* handle, float* out_signal_power);
esp_err_t sx126x_get_stats_lora(sx126x_handle_t* handle, uint16_t* out_nb_pkt_received, uint16_t* out_nb_pkt_crc_error,
                                uint16_t* out_nb_pkt_header_error);
esp_err_t sx126x_get_stats_gfsk(sx126x_handle_t* handle, uint16_t* out_nb_pkt_received, uint16_t* out_nb_pkt_crc_error,
                                uint16_t* out_nb_pkt_length_error);
esp_err_t sx126x_reset_stats(sx126x_handle_t* handle);
esp_err_t sx126x_get_device_errors(sx126x_handle_t* handle, uint16_t* out_errors);
esp_err_t sx126x_clear_device_errors(sx126x_handle_t* handle, uint8_t* out_command_status, uint8_t* out_chip_mode);

// Registers
esp_err_t sx126x_read_version_string(sx126x_handle_t* handle, char* out_buffer, size_t buffer_size);
esp_err_t sx126x_set_sync_word_adv(sx126x_handle_t* handle, uint8_t sync_word, uint8_t control_bits);
esp_err_t sx126x_set_sync_word(sx126x_handle_t* handle, uint8_t sync_word);

// Management & control
esp_err_t sx1262_reset(sx126x_handle_t* handle);
esp_err_t sx126x_init(sx126x_handle_t* handle, int spi_host_id, int nss, int reset,
                      int dio1, int busy);
bool      sx126x_is_busy(sx126x_handle_t* handle);
esp_err_t sx126x_irq_wait(sx126x_handle_t* handle, TickType_t timeout);
bool      sx126x_get_irq_state(sx126x_handle_t* handle);

#endif
