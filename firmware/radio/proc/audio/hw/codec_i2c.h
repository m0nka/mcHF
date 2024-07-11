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
#ifndef __CODEC_I2C_H
#define __CODEC_I2C_H

#include "stm32h747i_discovery_conf.h"

#if (USE_HAL_I2C_REGISTER_CALLBACKS == 1)
typedef struct
{
  void (* pMspInitCb)(I2C_HandleTypeDef *);
  void (* pMspDeInitCb)(I2C_HandleTypeDef *);
}BSP_I2C_Cb_t;
#endif /* (USE_HAL_I2C_REGISTER_CALLBACKS == 1) */

#ifdef BOARD_MCHF_PRO
#define BUS_I2C1                               I2C1
#define BUS_I2C1_CLK_ENABLE()                  __HAL_RCC_I2C1_CLK_ENABLE()
#define BUS_I2C1_CLK_DISABLE()                 __HAL_RCC_I2C1_CLK_DISABLE()
#define BUS_I2C1_SCL_GPIO_CLK_ENABLE()         __HAL_RCC_GPIOB_CLK_ENABLE()
#define BUS_I2C1_SCL_GPIO_CLK_DISABLE()        __HAL_RCC_GPIOB_CLK_DISABLE()
#define BUS_I2C1_SDA_GPIO_CLK_ENABLE()         __HAL_RCC_GPIOB_CLK_ENABLE()
#define BUS_I2C1_SDA_GPIO_CLK_DISABLE()        __HAL_RCC_GPIOB_CLK_DISABLE()

#define BUS_I2C1_FORCE_RESET()                 __HAL_RCC_I2C1_FORCE_RESET()
#define BUS_I2C1_RELEASE_RESET()               __HAL_RCC_I2C1_RELEASE_RESET()

/* Definition for I2C4 Pins */
#define BUS_I2C1_SCL_PIN                       GPIO_PIN_8
#define BUS_I2C1_SDA_PIN                       GPIO_PIN_9
#define BUS_I2C1_SCL_GPIO_PORT                 GPIOB
#define BUS_I2C1_SDA_GPIO_PORT                 GPIOB
#define BUS_I2C1_SCL_AF                        GPIO_AF4_I2C1
#define BUS_I2C1_SDA_AF                        GPIO_AF4_I2C1
#endif

#ifndef BUS_I2C1_FREQUENCY
   #define BUS_I2C1_FREQUENCY  100000U /* Frequency of I2Cn = 100 KHz*/
#endif

extern I2C_HandleTypeDef hbus_i2c4;

int32_t BSP_I2C1_Init(void);
int32_t BSP_I2C1_DeInit(void);
int32_t BSP_I2C1_WriteReg(uint16_t DevAddr, uint16_t Reg, uint8_t *pData, uint16_t Length);
int32_t BSP_I2C1_ReadReg(uint16_t DevAddr, uint16_t Reg, uint8_t *pData, uint16_t Length);
int32_t BSP_I2C1_WriteReg16(uint16_t DevAddr, uint16_t Reg, uint8_t *pData, uint16_t Length);
int32_t BSP_I2C1_ReadReg16(uint16_t DevAddr, uint16_t Reg, uint8_t *pData, uint16_t Length);
int32_t BSP_I2C1_IsReady(uint16_t DevAddr, uint32_t Trials);
//int32_t BSP_GetTick(void);
#if (USE_HAL_I2C_REGISTER_CALLBACKS == 1)
int32_t BSP_I2C4_RegisterDefaultMspCallbacks (void);
int32_t BSP_I2C4_RegisterMspCallbacks (BSP_I2C_Cb_t *Callback);
#endif /* USE_HAL_I2C_REGISTER_CALLBACKS */
HAL_StatusTypeDef MX_I2C1_Init(I2C_HandleTypeDef *phi2c, uint32_t timing);


#endif
