/************************************************************************************
**                                                                                 **
**                                 mcHF QRP Transceiver                            **
**                         Krassi Atanassov - M0NKA, 2013-2026                     **
**                                                                                 **
**---------------------------------------------------------------------------------**
**                                                                                 **
**  File name:		uhsdr_hw_sai_m4.h                                              **
**  Description:	SAI1 audio streaming hw driver for the H747 CM4 core,          **
**					same hw setup as the CLINT baseband project (audio_sai)        **
**  Licence:		https://github.com/m0nka/mcHF/blob/main/LICENSE                **
************************************************************************************/
#ifndef __UHSDR_HW_SAI_M4_H
#define __UHSDR_HW_SAI_M4_H

#ifdef H7_M4_CORE

#include "stm32h7xx_hal.h"

#define AUDIO_OUT_SAIx                           SAI1_Block_B
#define AUDIO_OUT_SAIx_CLK_ENABLE()              __HAL_RCC_SAI1_CLK_ENABLE()
#define AUDIO_OUT_SAIx_CLK_DISABLE()             __HAL_RCC_SAI1_CLK_DISABLE()

#define AUDIO_OUT_SAIx_SCK_GPIO_PORT             GPIOF
#define AUDIO_OUT_SAIx_SCK_PIN                   GPIO_PIN_8
#define AUDIO_OUT_SAIx_SCK_AF                    GPIO_AF6_SAI1

#define AUDIO_OUT_SAIx_SD_GPIO_PORT              GPIOE
#define AUDIO_OUT_SAIx_SD_PIN                    GPIO_PIN_3
#define AUDIO_OUT_SAIx_SD_AF                     GPIO_AF6_SAI1

#define AUDIO_OUT_SAIx_FS_GPIO_PORT              GPIOF
#define AUDIO_OUT_SAIx_FS_PIN                    GPIO_PIN_9
#define AUDIO_OUT_SAIx_FS_AF                     GPIO_AF6_SAI1

// SAI DMA Stream definitions
#define AUDIO_OUT_SAIx_DMAx_CLK_ENABLE()         __HAL_RCC_DMA2_CLK_ENABLE()
#define AUDIO_OUT_SAIx_DMAx_STREAM               DMA2_Stream4
#define AUDIO_OUT_SAIx_DMAx_REQUEST              DMA_REQUEST_SAI1_B
#define AUDIO_OUT_SAIx_DMAx_IRQ                  DMA2_Stream4_IRQn
#define AUDIO_OUT_SAIx_DMAx_IRQHandler           DMA2_Stream4_IRQHandler

#define AUDIO_IN_SAIx                            SAI1_Block_A
#define AUDIO_IN_SAIx_CLK_ENABLE()               __HAL_RCC_SAI1_CLK_ENABLE()
#define AUDIO_IN_SAIx_CLK_DISABLE()              __HAL_RCC_SAI1_CLK_DISABLE()

#define AUDIO_IN_SAIx_MCLK_GPIO_PORT             GPIOG
#define AUDIO_IN_SAIx_MCLK_PIN                   GPIO_PIN_7
#define AUDIO_IN_SAIx_MCLK_AF                    GPIO_AF6_SAI1

#define AUDIO_IN_SAIx_SCK_GPIO_PORT              GPIOE
#define AUDIO_IN_SAIx_SCK_PIN                    GPIO_PIN_5
#define AUDIO_IN_SAIx_SCK_AF                     GPIO_AF6_SAI1

#define AUDIO_IN_SAIx_SD_GPIO_PORT               GPIOE
#define AUDIO_IN_SAIx_SD_PIN                     GPIO_PIN_6
#define AUDIO_IN_SAIx_SD_AF                      GPIO_AF6_SAI1

#define AUDIO_IN_SAIx_FS_GPIO_PORT               GPIOE
#define AUDIO_IN_SAIx_FS_PIN                     GPIO_PIN_4
#define AUDIO_IN_SAIx_FS_AF                      GPIO_AF6_SAI1

#define AUDIO_IN_SAIx_DMAx_CLK_ENABLE()          __HAL_RCC_DMA2_CLK_ENABLE()
#define AUDIO_IN_SAIx_DMAx_STREAM                DMA2_Stream1
#define AUDIO_IN_SAIx_DMAx_REQUEST               DMA_REQUEST_SAI1_A
#define AUDIO_IN_SAIx_DMAx_IRQ                   DMA2_Stream1_IRQn
#define AUDIO_IN_SAIx_DMAx_IRQHandler            DMA2_Stream1_IRQHandler

// SAI handles (RX = Block A master, TX = Block B)
extern SAI_HandleTypeDef	haudio_out_sai;
extern SAI_HandleTypeDef	haudio_in_sai;

// Init SAI1 blocks + DMA and start circular streaming.
// rx_buf/tx_buf point to 16 bit interleaved L/R sample buffers,
// buf_len_halfwords is the total number of 16 bit words per buffer
uint8_t uhsdr_sai_m4_start(int16_t *rx_buf, int16_t *tx_buf, uint32_t buf_len_halfwords);

// Stop streaming
void uhsdr_sai_m4_stop(void);

#endif // H7_M4_CORE

#endif
