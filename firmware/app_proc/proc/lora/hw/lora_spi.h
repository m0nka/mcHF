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
#ifndef __LORA_SPI_H
#define __LORA_SPI_H

// Test wiring to back board
//#define SPI_GPIO_TEST

// Defines a default timeout delay in milliseconds for the SPI transfer
#ifndef SPI_TRANSFER_TIMEOUT
#define SPI_TRANSFER_TIMEOUT 1000
#elif SPI_TRANSFER_TIMEOUT <= 0
#error "SPI_TRANSFER_TIMEOUT cannot be less or equal to 0!"
#endif

#if 0
#define SPI1_CLK_ENABLE()                __HAL_RCC_SPI1_CLK_ENABLE()
#define DMA1_CLK_ENABLE()                __HAL_RCC_DMA1_CLK_ENABLE()
#define SPI1_SCK_GPIO_CLK_ENABLE()       __HAL_RCC_GPIOA_CLK_ENABLE()
#define SPI1_MISO_GPIO_CLK_ENABLE()      __HAL_RCC_GPIOA_CLK_ENABLE()
#define SPI1_MOSI_GPIO_CLK_ENABLE()      __HAL_RCC_GPIOB_CLK_ENABLE()

#define SPI1_FORCE_RESET()               __HAL_RCC_SPI1_FORCE_RESET()
#define SPI1_RELEASE_RESET()             __HAL_RCC_SPI1_RELEASE_RESET()

#define SPI1_TX_DMA_STREAM               DMA1_Stream7
#define SPI1_RX_DMA_STREAM               DMA1_Stream2

#define SPI1_TX_DMA_REQUEST              DMA_REQUEST_SPI1_TX
#define SPI1_RX_DMA_REQUEST              DMA_REQUEST_SPI1_RX

#define SPI1_DMA_TX_IRQn                 DMA1_Stream7_IRQn
#define SPI1_DMA_RX_IRQn                 DMA1_Stream2_IRQn

#define SPI1_DMA_TX_IRQHandler           DMA1_Stream7_IRQHandler
#define SPI1_DMA_RX_IRQHandler           DMA1_Stream2_IRQHandler

#define SPI1_IRQn                        SPI1_IRQn
#define SPI1_IRQHandler                  SPI1_IRQHandler

#define BUFFERSIZE                       (COUNTOF(LoraTxBuffer) - 1)
#define COUNTOF(__BUFFER__)   (sizeof(__BUFFER__) / sizeof(*(__BUFFER__)))

enum {
  TRANSFER_WAIT,
  TRANSFER_COMPLETE,
  TRANSFER_ERROR
};
#endif

// Input flags
#define SPI_TRANS_MODE_DIO            (1<<0)  ///< Transmit/receive data in 2-bit mode
#define SPI_TRANS_MODE_QIO            (1<<1)  ///< Transmit/receive data in 4-bit mode
#define SPI_TRANS_USE_RXDATA          (1<<2)  ///< Receive into rx_data member of spi_transaction_t instead into memory at rx_buffer.
#define SPI_TRANS_USE_TXDATA          (1<<3)  ///< Transmit tx_data member of spi_transaction_t instead of data at tx_buffer. Do not set tx_buffer when using this.
#define SPI_TRANS_MODE_DIOQIO_ADDR    (1<<4)  ///< Also transmit address in mode selected by SPI_MODE_DIO/SPI_MODE_QIO
#define SPI_TRANS_VARIABLE_CMD        (1<<5)  ///< Use the ``command_bits`` in ``spi_transaction_ext_t`` rather than default value in ``spi_device_interface_config_t``.
#define SPI_TRANS_VARIABLE_ADDR       (1<<6)  ///< Use the ``address_bits`` in ``spi_transaction_ext_t`` rather than default value in ``spi_device_interface_config_t``.
#define SPI_TRANS_VARIABLE_DUMMY      (1<<7)  ///< Use the ``dummy_bits`` in ``spi_transaction_ext_t`` rather than default value in ``spi_device_interface_config_t``.
#define SPI_TRANS_CS_KEEP_ACTIVE      (1<<8)  ///< Keep CS active after data transfer
#define SPI_TRANS_MULTILINE_CMD       (1<<9)  ///< The data lines used at command phase is the same as data phase (otherwise, only one data line is used at command phase)
#define SPI_TRANS_MODE_OCT            (1<<10) ///< Transmit/receive data in 8-bit mode
#define SPI_TRANS_MULTILINE_ADDR      SPI_TRANS_MODE_DIOQIO_ADDR ///< The data lines used at address phase is the same as data phase (otherwise, only one data line is used at address phase)
#define SPI_TRANS_DMA_BUFFER_ALIGN_MANUAL   (1<<11) ///< By default driver will automatically re-alloc dma buffer if it doesn't meet hardware alignment or dma_capable requirements, this flag is for you to disable this feature, you will need to take care of the alignment otherwise driver will return you error ESP_ERR_INVALID_ARG
#define SPI_TRANS_DMA_USE_PSRAM       (1<<12) ///< Use PSRAM for DMA buffer directly, has speed limit, but no temp buffer and save memory


/**
 * This structure describes one SPI transaction. The descriptor should not be modified until the transaction finishes.
 */
typedef struct spi_transaction_t {
    uint32_t flags;                 ///< Bitwise OR of SPI_TRANS_* flags
    uint16_t cmd;                   /**< Command data, of which the length is set in the ``command_bits`` of spi_device_interface_config_t.
                                      *
                                      *  <b>NOTE: this field, used to be "command" in ESP-IDF 2.1 and before, is re-written to be used in a new way in ESP-IDF 3.0.</b>
                                      *
                                      *  Example: write 0x0123 and command_bits=12 to send command 0x12, 0x3_ (in previous version, you may have to write 0x3_12).
                                      */
    uint64_t addr;                  /**< Address data, of which the length is set in the ``address_bits`` of spi_device_interface_config_t.
                                      *
                                      *  <b>NOTE: this field, used to be "address" in ESP-IDF 2.1 and before, is re-written to be used in a new way in ESP-IDF3.0.</b>
                                      *
                                      *  Example: write 0x123400 and address_bits=24 to send address of 0x12, 0x34, 0x00 (in previous version, you may have to write 0x12340000).
                                      */
    size_t length;                  ///< Total data length, in bits
    size_t rxlength;                ///< Total data length received, should be not greater than ``length`` in full-duplex mode (0 defaults this to the value of ``length``).
    uint32_t override_freq_hz;      ///< New freq speed value will override for current device, remain `0` to skip update. Need about 30us each time
    void *user;                     ///< User-defined variable. Can be used to store eg transaction ID.
    union {
        const void *tx_buffer;      ///< Pointer to transmit buffer, or NULL for no MOSI phase
        uint8_t tx_data[4];         ///< If SPI_TRANS_USE_TXDATA is set, data set here is sent directly from this variable.
    };
    union {
        void *rx_buffer;            ///< Pointer to receive buffer, or NULL for no MISO phase. Written by 4 bytes-unit if DMA is used.
        uint8_t rx_data[4];         ///< If SPI_TRANS_USE_RXDATA is set, data is received directly to this variable
    };
} spi_transaction_t;        //the rx data should start from a 32-bit aligned address to get around dma issue.

/**
 * This struct is for SPI transactions which may change their address and command length.
 * Please do set the flags in base to ``SPI_TRANS_VARIABLE_CMD_ADR`` to use the bit length here.
 */
typedef struct {
    struct spi_transaction_t base;  ///< Transaction data, so that pointer to spi_transaction_t can be converted into spi_transaction_ext_t
    uint8_t command_bits;           ///< The command length in this transaction, in bits.
    uint8_t address_bits;           ///< The address length in this transaction, in bits.
    uint8_t dummy_bits;             ///< The dummy length in this transaction, in bits.
} spi_transaction_ext_t ;

// -----------------------------------------------------------------------

void lora_spi_activate_exti_irq(void);
void lora_spi_init(void);

int spi_device_transmit(int device, spi_transaction_t *t);

void lora_gpio_init(void);
void lora_spi_power_state(uchar on);


#endif
