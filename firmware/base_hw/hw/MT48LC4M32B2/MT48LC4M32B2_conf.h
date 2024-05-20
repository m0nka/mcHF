/**
  ******************************************************************************
  * @file    MT48LC4M32B2_conf.h
  * @author  MCD Application Team
  * @brief   This file contains some configurations required for the
  *          MT48LC4M32B2 SDRAM memory.
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2018 STMicroelectronics.
  * All rights reserved.</center></h2>
  *
  * This software component is licensed by ST under BSD 3-Clause license,
  * the "License"; You may not use this file except in compliance with the
  * License. You may obtain a copy of the License at:
  *                        opensource.org/licenses/BSD-3-Clause
  *
  ******************************************************************************
  */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __MT48LC4M32B2_CONF_H
#define __MT48LC4M32B2_CONF_H

/* Includes ------------------------------------------------------------------*/
#include "stm32h7xx_hal.h"

#define REFRESH_COUNT                    ((uint32_t)0x0603)   /* SDRAM refresh counter (100Mhz SD clock) */
#define MT48LC4M32B2_TIMEOUT             ((uint32_t)0xFFFF)

#endif /* MT48LC4M32B2_CONF_H */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
