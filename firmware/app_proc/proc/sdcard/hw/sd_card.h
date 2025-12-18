#ifndef __SD_CARD_H
#define __SD_CARD_H

// Use DMA version
#define SD_USE_DMA

// Use PLL2 as clock source
//#define SD_USE_PLL2

/*
 * when using cachable memory region, it may be needed to maintain the cache
 * validity. Enable the define below to activate a cache maintenance at each
 * read and write operation.
 * Notice: This is applicable only for cortex M7 based platform.
 */
#ifdef SD_USE_DMA
#define ENABLE_SD_DMA_CACHE_MAINTENANCE  1
#endif

#define BSP_SD_CardInfo 		HAL_SD_CardInfoTypeDef

#ifndef SD_WRITE_TIMEOUT
#define SD_WRITE_TIMEOUT         100U
#endif

#ifndef SD_READ_TIMEOUT
#define SD_READ_TIMEOUT          5000U
#endif

#define   SD_TRANSFER_OK         0U
#define   SD_TRANSFER_BUSY       1U

#define SD_PRESENT               1UL
#define SD_NOT_PRESENT           0UL

#define SD_DETECT_EXTI_IRQn      EXTI0_IRQn
#define SD_DETECT_EXTI_LINE      EXTI_LINE_0

// ----------------------------------------------------------------------------------------------
//
int32_t sd_card_init(void);
int32_t sd_card_set_exti_irq(void);
int32_t sd_card_read_blocks(uint32_t *pData, uint32_t BlockIdx, uint32_t BlocksNbr);
int32_t sd_card_write_blocks(uint32_t *pData, uint32_t BlockIdx, uint32_t NbrOfBlocks);
int32_t sd_card_read_blocks_dma(uint32_t *pData, uint32_t BlockIdx, uint32_t NbrOfBlocks);
int32_t sd_card_write_blocks_dma(uint32_t *pData, uint32_t BlockIdx, uint32_t NbrOfBlocks);
int32_t sd_card_erase(uint32_t BlockIdx, uint32_t BlocksNbr);
int32_t sd_card_get_card_state(void);
int32_t ad_card_get_card_info(BSP_SD_CardInfo *CardInfo);
uchar 	sd_card_is_detected(void);
void 	sd_card_reset_driver(void);
void 	sd_card_low_level_init(void);
void 	sd_card_power(uchar state);

#endif

