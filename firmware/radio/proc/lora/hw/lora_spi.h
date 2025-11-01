/************************************************************************************
**                                                                                 **
**                             mcHF Pro QRP Transceiver                            **
**                         Krassi Atanassov - M0NKA, 2013-2025                     **
**                                                                                 **
**---------------------------------------------------------------------------------**
**                                                                                 **
**  File name:                                                                     **
**  Description:                                                                   **
**  Last Modified:                                                                 **
**  Licence:                                                                       **
************************************************************************************/
#ifndef __LORA_SPI_H
#define __LORA_SPI_H

#define SPI1_CLK_ENABLE()                __HAL_RCC_SPI1_CLK_ENABLE()
#define DMA1_CLK_ENABLE()                __HAL_RCC_DMA1_CLK_ENABLE()
#define SPI1_SCK_GPIO_CLK_ENABLE()       __HAL_RCC_GPIOA_CLK_ENABLE()
#define SPI1_MISO_GPIO_CLK_ENABLE()      __HAL_RCC_GPIOA_CLK_ENABLE()
#define SPI1_MOSI_GPIO_CLK_ENABLE()      __HAL_RCC_GPIOB_CLK_ENABLE()

#define SPI1_FORCE_RESET()               __HAL_RCC_SPI1_FORCE_RESET()
#define SPI1_RELEASE_RESET()             __HAL_RCC_SPI1_RELEASE_RESET()

#define SPI1_TX_DMA_STREAM               DMA1_Stream3
#define SPI1_RX_DMA_STREAM               DMA1_Stream2

#define SPI1_TX_DMA_REQUEST              DMA_REQUEST_SPI1_TX
#define SPI1_RX_DMA_REQUEST              DMA_REQUEST_SPI1_RX

#define SPI1_DMA_TX_IRQn                 DMA1_Stream3_IRQn
#define SPI1_DMA_RX_IRQn                 DMA1_Stream2_IRQn

#define SPI1_DMA_TX_IRQHandler           DMA1_Stream3_IRQHandler
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

// -----------------------------------------------------------------------

uchar lora_spi_init(void);


#endif
