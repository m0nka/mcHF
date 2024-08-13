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
#ifndef __TOUCH_I2C_H
#define __TOUCH_I2C_H

#include "stm32h747i_discovery_conf.h"

#if (USE_HAL_I2C_REGISTER_CALLBACKS == 1)
typedef struct
{
  void (* pMspInitCb)(I2C_HandleTypeDef *);
  void (* pMspDeInitCb)(I2C_HandleTypeDef *);
}BSP_I2C_Cb_t;
#endif /* (USE_HAL_I2C_REGISTER_CALLBACKS == 1) */

#ifdef BOARD_MCHF_PRO
#define BUS_I2C4                              I2C1
#define BUS_I2C4_CLK_ENABLE()                  __HAL_RCC_C1_I2C1_CLK_ENABLE()
#define BUS_I2C4_CLK_DISABLE()                 __HAL_RCC_C1_I2C1_CLK_ENABLE()
//#define BUS_I2C4_SCL_GPIO_CLK_ENABLE()         __HAL_RCC_GPIOB_CLK_ENABLE()
//#define BUS_I2C4_SCL_GPIO_CLK_DISABLE()        __HAL_RCC_GPIOB_CLK_DISABLE()
//#define BUS_I2C4_SDA_GPIO_CLK_ENABLE()         __HAL_RCC_GPIOB_CLK_ENABLE()
//#define BUS_I2C4_SDA_GPIO_CLK_DISABLE()        __HAL_RCC_GPIOB_CLK_DISABLE()

#define BUS_I2C4_FORCE_RESET()                 __HAL_RCC_I2C1_FORCE_RESET()
#define BUS_I2C4_RELEASE_RESET()               __HAL_RCC_I2C1_RELEASE_RESET()

/* Definition for I2C4 Pins */
#define BUS_I2C4_SCL_PIN                       GPIO_PIN_6
#define BUS_I2C4_SDA_PIN                       GPIO_PIN_7
#define BUS_I2C4_SCL_GPIO_PORT                 GPIOB
#define BUS_I2C4_SDA_GPIO_PORT                 GPIOB
#define BUS_I2C4_SCL_AF                        GPIO_AF4_I2C4
#define BUS_I2C4_SDA_AF                        GPIO_AF4_I2C4
#endif

#ifndef BUS_I2C4_FREQUENCY
   #define BUS_I2C4_FREQUENCY  100000U /* Frequency of I2Cn = 100 KHz*/
#endif

extern I2C_HandleTypeDef hbus_i2c4;

int32_t BSP_I2C4_Init(void);
int32_t BSP_I2C4_DeInit(void);
int32_t BSP_I2C4_WriteReg(uint16_t DevAddr, uint16_t Reg, uint8_t *pData, uint16_t Length);
int32_t BSP_I2C4_ReadReg(uint16_t DevAddr, uint16_t Reg, uint8_t *pData, uint16_t Length);
int32_t BSP_I2C4_WriteReg16(uint16_t DevAddr, uint16_t Reg, uint8_t *pData, uint16_t Length);
int32_t BSP_I2C4_ReadReg16(uint16_t DevAddr, uint16_t Reg, uint8_t *pData, uint16_t Length);
int32_t BSP_I2C4_IsReady(uint16_t DevAddr, uint32_t Trials);
int32_t BSP_GetTick(void);
#if (USE_HAL_I2C_REGISTER_CALLBACKS == 1)
int32_t BSP_I2C4_RegisterDefaultMspCallbacks (void);
int32_t BSP_I2C4_RegisterMspCallbacks (BSP_I2C_Cb_t *Callback);
#endif /* USE_HAL_I2C_REGISTER_CALLBACKS */
HAL_StatusTypeDef MX_I2C4_Init(I2C_HandleTypeDef *phi2c, uint32_t timing);


#endif
