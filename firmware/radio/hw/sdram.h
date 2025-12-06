
#ifndef __SDRAM_H
#define __SDRAM_H

#ifdef __cplusplus
 extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32h747i_discovery_conf.h"
#include "stm32h747i_discovery_errno.h"
//#include "../Components/is42s32800j/is42s32800j.h"

#if (USE_HAL_SDRAM_REGISTER_CALLBACKS == 1)
typedef struct
{
  void (* pMspInitCb)(SDRAM_HandleTypeDef *);
  void (* pMspDeInitCb)(SDRAM_HandleTypeDef *);
}BSP_SDRAM_Cb_t;
#endif /* (USE_HAL_SDRAM_REGISTER_CALLBACKS == 1) */

// SDRAM refresh counter (100Mhz SD clock)
#define REFRESH_COUNT                    		((uint32_t)0x0603)

#define SDRAM_TIMEOUT                    		((uint32_t)0xFFFF)

#define SDRAM_MODEREG_BURST_LENGTH_1             ((uint16_t)0x0000)
#define SDRAM_MODEREG_BURST_LENGTH_2             ((uint16_t)0x0001)
#define SDRAM_MODEREG_BURST_LENGTH_4             ((uint16_t)0x0002)
#define SDRAM_MODEREG_BURST_LENGTH_8             ((uint16_t)0x0004)
#define SDRAM_MODEREG_BURST_TYPE_SEQUENTIAL      ((uint16_t)0x0000)
#define SDRAM_MODEREG_BURST_TYPE_INTERLEAVED     ((uint16_t)0x0008)
#define SDRAM_MODEREG_CAS_LATENCY_2              ((uint16_t)0x0020)
#define SDRAM_MODEREG_CAS_LATENCY_3              ((uint16_t)0x0030)
#define SDRAM_MODEREG_OPERATING_MODE_STANDARD    ((uint16_t)0x0000)
#define SDRAM_MODEREG_WRITEBURST_MODE_PROGRAMMED ((uint16_t)0x0000)
#define SDRAM_MODEREG_WRITEBURST_MODE_SINGLE     ((uint16_t)0x0200)

#define SDRAM_INSTANCES_NBR       				1U
#define SDRAM_DEVICE_ADDR         				0xC0000000

// Executable region
#define SDRAM_APP_ADDR         					0xC0800000

#ifndef PCB_V9_REV_A
#define SDRAM_DEVICE_SIZE         				0x00400000
#else
#define SDRAM_DEVICE_SIZE         				0x01000000
#endif

/* MDMA definitions for SDRAM DMA transfer */
#define SDRAM_MDMAx_CLK_ENABLE             __HAL_RCC_MDMA_CLK_ENABLE
#define SDRAM_MDMAx_CLK_DISABLE            __HAL_RCC_MDMA_CLK_DISABLE
#define SDRAM_MDMAx_CHANNEL                MDMA_Channel0
#define SDRAM_MDMAx_IRQn                   MDMA_IRQn
#define SDRAM_MDMA_IRQHandler              MDMA_IRQHandler

extern SDRAM_HandleTypeDef hsdram[];

int32_t BSP_SDRAM_Init(uint32_t Instance);
int32_t BSP_SDRAM_DeInit(uint32_t Instance);
#if (USE_HAL_SDRAM_REGISTER_CALLBACKS == 1)
int32_t BSP_SDRAM_RegisterDefaultMspCallbacks (uint32_t Instance);
int32_t BSP_SDRAM_RegisterMspCallbacks (uint32_t Instance, BSP_SDRAM_Cb_t *CallBacks);
#endif /* (USE_HAL_SDRAM_REGISTER_CALLBACKS == 1)  */
int32_t BSP_SDRAM_SendCmd(uint32_t Instance, FMC_SDRAM_CommandTypeDef *SdramCmd);

void BSP_SDRAM_IRQHandler(uint32_t Instance);

/* These functions can be modified in case the current settings need to be
   changed for specific application needs */
HAL_StatusTypeDef MX_SDRAM_Init(SDRAM_HandleTypeDef *hSdram);



#ifdef __cplusplus
}
#endif

#endif
