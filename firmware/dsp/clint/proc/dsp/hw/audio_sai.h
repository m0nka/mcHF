/************************************************************************************
**                                                                                 **
**                             mcHF Pro QRP Transceiver                            **
**                         Krassi Atanassov - M0NKA, 2013-2021                     **
**                                                                                 **
**---------------------------------------------------------------------------------**
**                                                                                 **
**  File name:                                                                     **
**  Description:                                                                   **
**  Last Modified:                                                                 **
**  Licence:       The mcHF project is released for radio amateurs experimentation **
**               and non-commercial use only.Check 3rd party drivers for licensing **
************************************************************************************/
#ifndef __AUDIO_SAI_H
#define __AUDIO_SAI_H

#define DMA_BUFF_SIZE       	128

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

/* SAI DMA Stream definitions */
#define AUDIO_OUT_SAIx_DMAx_CLK_ENABLE()         __HAL_RCC_DMA2_CLK_ENABLE()
#define AUDIO_OUT_SAIx_DMAx_STREAM               DMA2_Stream4
#define AUDIO_OUT_SAIx_DMAx_REQUEST              DMA_REQUEST_SAI1_B
#define AUDIO_OUT_SAIx_DMAx_IRQ                  DMA2_Stream4_IRQn
#define AUDIO_OUT_SAIx_DMAx_PERIPH_DATA_SIZE     DMA_PDATAALIGN_HALFWORD
#define AUDIO_OUT_SAIx_DMAx_MEM_DATA_SIZE        DMA_MDATAALIGN_HALFWORD
#define AUDIO_OUT_SAIx_DMAx_IRQHandler           DMA2_Stream4_IRQHandler
/*------------------------------------------------------------------------------
                        AUDIO IN defines parameters
------------------------------------------------------------------------------*/
#define AUDIO_IN_SAIx                           SAI1_Block_A
#define AUDIO_IN_SAIx_CLK_ENABLE()              __HAL_RCC_SAI1_CLK_ENABLE()
#define AUDIO_IN_SAIx_CLK_DISABLE()             __HAL_RCC_SAI1_CLK_DISABLE()

#define AUDIO_IN_SAIx_MCLK_GPIO_PORT            GPIOG
#define AUDIO_IN_SAIx_MCLK_PIN                  GPIO_PIN_7
#define AUDIO_IN_SAIx_MCLK_AF                   GPIO_AF6_SAI1

#define AUDIO_IN_SAIx_SCK_GPIO_PORT             GPIOE
#define AUDIO_IN_SAIx_SCK_PIN                   GPIO_PIN_5
#define AUDIO_IN_SAIx_SCK_AF                    GPIO_AF6_SAI1

#define AUDIO_IN_SAIx_SD_GPIO_PORT              GPIOE
#define AUDIO_IN_SAIx_SD_PIN                    GPIO_PIN_6
#define AUDIO_IN_SAIx_SD_AF                     GPIO_AF6_SAI1

#define AUDIO_IN_SAIx_FS_GPIO_PORT              GPIOE
#define AUDIO_IN_SAIx_FS_PIN                    GPIO_PIN_4
#define AUDIO_IN_SAIx_FS_AF                     GPIO_AF6_SAI1

#define AUDIO_IN_SAIx_DMAx_CLK_ENABLE()         __HAL_RCC_DMA2_CLK_ENABLE()
#define AUDIO_IN_SAIx_DMAx_STREAM               DMA2_Stream1
#define AUDIO_IN_SAIx_DMAx_REQUEST              DMA_REQUEST_SAI1_A
#define AUDIO_IN_SAIx_DMAx_IRQ                  DMA2_Stream1_IRQn
//#define AUDIO_IN_SAIx_DMAx_PERIPH_DATA_SIZE     DMA_PDATAALIGN_HALFWORD
//#define AUDIO_IN_SAIx_DMAx_MEM_DATA_SIZE        DMA_MDATAALIGN_HALFWORD
#define AUDIO_IN_SAIx_DMAx_IRQHandler           DMA2_Stream1_IRQHandler
/*------------------------------------------------------------------------------
                        --------------------------
------------------------------------------------------------------------------*/

/* AUDIO FREQUENCY */
#define AUDIO_FREQUENCY_192K     192000U
#define AUDIO_FREQUENCY_176K     176400U
#define AUDIO_FREQUENCY_96K       96000U
#define AUDIO_FREQUENCY_88K       88200U
#define AUDIO_FREQUENCY_48K       48000U
#define AUDIO_FREQUENCY_44K       44100U
#define AUDIO_FREQUENCY_32K       32000U
#define AUDIO_FREQUENCY_22K       22050U
#define AUDIO_FREQUENCY_16K       16000U
#define AUDIO_FREQUENCY_11K       11025U
#define AUDIO_FREQUENCY_8K         8000U

/* AUDIO RESOLUTION */
#define AUDIO_RESOLUTION_16B                16U
#define AUDIO_RESOLUTION_32B                32U

uchar audio_sai_hw_init(void);
void audio_sai_get_buffer(uchar *buffer);

#endif
