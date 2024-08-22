/************************************************************************************
**                                                                                 **
**                             mcHF Pro QRP Transceiver                            **
**                         Krassi Atanassov - M0NKA 2012-2020                      **
**                            mail: djchrismarc@gmail.com                          **
**                                 twitter: @bph_co                                **
**---------------------------------------------------------------------------------**
**                                                                                 **
**  File name:                                                                     **
**  Description:                                                                   **
**  Last Modified:                                                                 **
**  Licence:                                                                       **
**          The mcHF project is released for radio amateurs experimentation,       **
**          non-commercial use only. All source files under GPL-3.0, unless        **
**          third party drivers specifies otherwise. Thank you!                    **
************************************************************************************/
#ifndef __VFO_I2C_H
#define __VFO_I2C_H

#include "stm32h747i_discovery_conf.h"

#if (USE_HAL_I2C_REGISTER_CALLBACKS == 1)
typedef struct
{
  void (* pMspInitCb)(I2C_HandleTypeDef *);
  void (* pMspDeInitCb)(I2C_HandleTypeDef *);
}BSP_I2C_Cb_t;
#endif /* (USE_HAL_I2C_REGISTER_CALLBACKS == 1) */

#ifdef BOARD_MCHF_PRO
#define BUS_I2C2                               I2C2
#define BUS_I2C2_CLK_ENABLE()                  __HAL_RCC_I2C2_CLK_ENABLE()
#define BUS_I2C2_CLK_DISABLE()                 __HAL_RCC_I2C2_CLK_DISABLE()
#define BUS_I2C2_SCL_GPIO_CLK_ENABLE()         __HAL_RCC_GPIOH_CLK_ENABLE()
#define BUS_I2C2_SCL_GPIO_CLK_DISABLE()        __HAL_RCC_GPIOH_CLK_DISABLE()
#define BUS_I2C2_SDA_GPIO_CLK_ENABLE()         __HAL_RCC_GPIOH_CLK_ENABLE()
#define BUS_I2C2_SDA_GPIO_CLK_DISABLE()        __HAL_RCC_GPIOH_CLK_DISABLE()

#define BUS_I2C2_FORCE_RESET()                 __HAL_RCC_I2C2_FORCE_RESET()
#define BUS_I2C2_RELEASE_RESET()               __HAL_RCC_I2C2_RELEASE_RESET()

/* Definition for I2C4 Pins */
#define BUS_I2C2_SCL_PIN                       GPIO_PIN_4
#define BUS_I2C2_SDA_PIN                       GPIO_PIN_5
#define BUS_I2C2_SCL_GPIO_PORT                 GPIOH
#define BUS_I2C2_SDA_GPIO_PORT                 GPIOH
#define BUS_I2C2_SCL_AF                        GPIO_AF4_I2C2
#define BUS_I2C2_SDA_AF                        GPIO_AF4_I2C2
#endif

#ifndef BUS_I2C2_FREQUENCY
   #define BUS_I2C2_FREQUENCY  400000U /* Frequency of I2Cn = 100 KHz*/
#endif

//extern I2C_HandleTypeDef hbus_i2c4;

int32_t BSP_I2C2_Init(void);
int32_t BSP_I2C2_DeInit(void);
int32_t BSP_I2C2_WriteReg(uint16_t DevAddr, uint16_t Reg, uint8_t *pData, uint16_t Length);
int32_t BSP_I2C2_ReadReg(uint16_t DevAddr, uint16_t Reg, uint8_t *pData, uint16_t Length);
int32_t BSP_I2C2_WriteReg16(uint16_t DevAddr, uint16_t Reg, uint8_t *pData, uint16_t Length);
int32_t BSP_I2C2_ReadReg16(uint16_t DevAddr, uint16_t Reg, uint8_t *pData, uint16_t Length);
int32_t BSP_I2C2_IsReady(uint16_t DevAddr, uint32_t Trials);
//int32_t BSP_GetTick(void);
#if (USE_HAL_I2C_REGISTER_CALLBACKS == 1)
int32_t BSP_I2C4_RegisterDefaultMspCallbacks (void);
int32_t BSP_I2C4_RegisterMspCallbacks (BSP_I2C_Cb_t *Callback);
#endif /* USE_HAL_I2C_REGISTER_CALLBACKS */
HAL_StatusTypeDef MX_I2C2_Init(I2C_HandleTypeDef *phi2c, uint32_t timing);


#endif
