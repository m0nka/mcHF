#ifndef __SD_CARD_H
#define __SD_CARD_H

#include "stm32h747i_discovery_conf.h"
#include "stm32h747i_discovery_errno.h"

#define BSP_SD_CardInfo HAL_SD_CardInfoTypeDef

#define SD_INSTANCES_NBR         1UL

#ifndef SD_WRITE_TIMEOUT
#define SD_WRITE_TIMEOUT         100U
#endif

#ifndef SD_READ_TIMEOUT
#define SD_READ_TIMEOUT          100U
#endif

#define   SD_TRANSFER_OK         0U
#define   SD_TRANSFER_BUSY       1U

#define SD_PRESENT               1UL
#define SD_NOT_PRESENT           0UL

#define SD_DETECT_EXTI_IRQn      EXTI0_IRQn
#define SD_DETECT_EXTI_LINE      EXTI_LINE_0

//#define SD_DetectIRQHandler()    HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_0)

extern SD_HandleTypeDef    hsd_sdmmc[];
extern EXTI_HandleTypeDef  hsd_exti[];

int32_t sd_card_init(uint32_t Instance);
int32_t BSP_SD_DeInit(uint32_t Instance);

int32_t sd_card_set_exti_irq(uint32_t Instance);
int32_t BSP_SD_ReadBlocks(uint32_t Instance, uint32_t *pData, uint32_t BlockIdx, uint32_t BlocksNbr);
int32_t BSP_SD_WriteBlocks(uint32_t Instance, uint32_t *pData, uint32_t BlockIdx, uint32_t NbrOfBlocks);
int32_t BSP_SD_ReadBlocks_DMA(uint32_t Instance, uint32_t *pData, uint32_t BlockIdx, uint32_t NbrOfBlocks);
int32_t BSP_SD_WriteBlocks_DMA(uint32_t Instance, uint32_t *pData, uint32_t BlockIdx, uint32_t NbrOfBlocks);
int32_t BSP_SD_ReadBlocks_IT(uint32_t Instance, uint32_t *pData, uint32_t BlockIdx, uint32_t NbrOfBlocks);
int32_t BSP_SD_WriteBlocks_IT(uint32_t Instance, uint32_t *pData, uint32_t BlockIdx, uint32_t NbrOfBlocks);
int32_t BSP_SD_Erase(uint32_t Instance, uint32_t BlockIdx, uint32_t BlocksNbr);
int32_t BSP_SD_GetCardState(uint32_t Instance);
int32_t BSP_SD_GetCardInfo(uint32_t Instance, BSP_SD_CardInfo *CardInfo);
int32_t BSP_SD_IsDetected(void);

void    BSP_SD_DETECT_IRQHandler(uint32_t Instance);
void    BSP_SD_IRQHandler(uint32_t Instance);

/* These functions can be modified in case the current settings (e.g. DMA stream ot IT)
   need to be changed for specific application needs */
void BSP_SD_AbortCallback(uint32_t Instance);
void BSP_SD_WriteCpltCallback(uint32_t Instance);
void BSP_SD_ReadCpltCallback(uint32_t Instance);
void BSP_SD_DetectCallback(uint32_t Instance, uint32_t Status);
void HAL_SD_DriveTransciver_1_8V_Callback(FlagStatus status);
HAL_StatusTypeDef MX_SDMMC1_SD_Init(SD_HandleTypeDef *hsd);

#endif

