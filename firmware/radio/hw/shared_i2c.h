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
#ifndef __SHARED_I2C_H
#define __SHARED_I2C_H

//#include "stm32h747i_discovery_conf.h"
#include "mchf_pro_pinmap.h"

#if (USE_HAL_I2C_REGISTER_CALLBACKS == 1)
typedef struct
{
  void (* pMspInitCb)(I2C_HandleTypeDef *);
  void (* pMspDeInitCb)(I2C_HandleTypeDef *);
}BSP_I2C_Cb_t;
#endif /* (USE_HAL_I2C_REGISTER_CALLBACKS == 1) */

#ifdef BOARD_MCHF_PRO
#define BUS_I2C1                               I2C4
#define BUS_I2C1_CLK_ENABLE()                  __HAL_RCC_I2C4_CLK_ENABLE()
#define BUS_I2C1_CLK_DISABLE()                 __HAL_RCC_I2C4_CLK_DISABLE()
#define BUS_I2C1_SCL_GPIO_CLK_ENABLE()         __HAL_RCC_GPIOB_CLK_ENABLE()
#define BUS_I2C1_SCL_GPIO_CLK_DISABLE()        __HAL_RCC_GPIOB_CLK_DISABLE()
#define BUS_I2C1_SDA_GPIO_CLK_ENABLE()         __HAL_RCC_GPIOB_CLK_ENABLE()
#define BUS_I2C1_SDA_GPIO_CLK_DISABLE()        __HAL_RCC_GPIOB_CLK_DISABLE()

#define BUS_I2C1_FORCE_RESET()                 __HAL_RCC_I2C4_FORCE_RESET()
#define BUS_I2C1_RELEASE_RESET()               __HAL_RCC_I2C4_RELEASE_RESET()

/* Definition for I2C1 Pins */
#define BUS_I2C1_SCL_PIN                       CODEC_SCL_I2C4_PIN
#define BUS_I2C1_SDA_PIN                       CODEC_SDA_I2C4_PIN
#define BUS_I2C1_SCL_GPIO_PORT                 CODEC_SCL_I2C4_PORT
#define BUS_I2C1_SDA_GPIO_PORT                 CODEC_SDA_I2C4_PORT
#define BUS_I2C1_SCL_AF                        CODEC_SCL_I2C4_AF
#define BUS_I2C1_SDA_AF                        CODEC_SDA_I2C4_AF
#endif

#ifndef BUS_I2C1_FREQUENCY
   #define BUS_I2C1_FREQUENCY  100000U /* Frequency of I2Cn = 100 KHz*/
#endif

#ifndef I2C_VALID_TIMING_NBR
#define I2C_VALID_TIMING_NBR                 128U
#endif
#define I2C_SPEED_FREQ_STANDARD                0U    /* 100 kHz */
#define I2C_SPEED_FREQ_FAST                    1U    /* 400 kHz */
#define I2C_SPEED_FREQ_FAST_PLUS               2U    /* 1 MHz */
#define I2C_ANALOG_FILTER_DELAY_MIN            50U   /* ns */
#define I2C_ANALOG_FILTER_DELAY_MAX            260U  /* ns */
#define I2C_USE_ANALOG_FILTER                  1U
#define I2C_DIGITAL_FILTER_COEF                0U
#define I2C_PRESC_MAX                          16U
#define I2C_SCLDEL_MAX                         16U
#define I2C_SDADEL_MAX                         16U
#define I2C_SCLH_MAX                           256U
#define I2C_SCLL_MAX                           256U
#define SEC2NSEC                               1000000000UL

typedef struct
{
  uint32_t freq;       /* Frequency in Hz */
  uint32_t freq_min;   /* Minimum frequency in Hz */
  uint32_t freq_max;   /* Maximum frequency in Hz */
  uint32_t hddat_min;  /* Minimum data hold time in ns */
  uint32_t vddat_max;  /* Maximum data valid time in ns */
  uint32_t sudat_min;  /* Minimum data setup time in ns */
  uint32_t lscl_min;   /* Minimum low period of the SCL clock in ns */
  uint32_t hscl_min;   /* Minimum high period of SCL clock in ns */
  uint32_t trise;      /* Rise time in ns */
  uint32_t tfall;      /* Fall time in ns */
  uint32_t dnf;        /* Digital noise filter coefficient */
} I2C_Charac_t;

typedef struct
{
  uint32_t presc;      /* Timing prescaler */
  uint32_t tscldel;    /* SCL delay */
  uint32_t tsdadel;    /* SDA delay */
  uint32_t sclh;       /* SCL high period */
  uint32_t scll;       /* SCL low period */
} I2C_Timings_t;


extern I2C_HandleTypeDef hbus_i2c4;

int32_t shared_i2c_init(void);
int32_t shared_i2c_deinit(void);
int32_t shared_i2c_write_reg(uint16_t DevAddr, uint16_t Reg, uint8_t *pData, uint16_t Length);
int32_t shared_i2c_read_reg(uint16_t DevAddr, uint16_t Reg, uint8_t *pData, uint16_t Length);
//int32_t BSP_I2C1_WriteReg16(uint16_t DevAddr, uint16_t Reg, uint8_t *pData, uint16_t Length);
//int32_t BSP_I2C1_ReadReg16(uint16_t DevAddr, uint16_t Reg, uint8_t *pData, uint16_t Length);
//int32_t BSP_I2C1_IsReady(uint16_t DevAddr, uint32_t Trials);
//int32_t BSP_GetTick(void);

#if (USE_HAL_I2C_REGISTER_CALLBACKS == 1)
int32_t BSP_I2C4_RegisterDefaultMspCallbacks (void);
int32_t BSP_I2C4_RegisterMspCallbacks (BSP_I2C_Cb_t *Callback);
#endif /* USE_HAL_I2C_REGISTER_CALLBACKS */
HAL_StatusTypeDef MX_I2C1_Init(I2C_HandleTypeDef *phi2c, uint32_t timing);


#endif
