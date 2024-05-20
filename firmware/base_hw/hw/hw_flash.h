/**
  ******************************************************************************
  * @file    bsp.h
  * @author  MCD Application Team
  * @brief   Header for bsp.c file
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2019 STMicroelectronics.
  * All rights reserved.</center></h2>
  *
  * This software component is licensed by ST under Ultimate Liberty license
  * SLA0044, the "License"; You may not use this file except in compliance with
  * the License. You may obtain a copy of the License at:
  *                             www.st.com/SLA0044
  *
  ******************************************************************************
  */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __HW_FLASH_H
#define __HW_FLASH_H

/* Includes ------------------------------------------------------------------*/
//#include "main.h"

/* Exported types ------------------------------------------------------------*/
/* Exported constants --------------------------------------------------------*/

/* Exported macros -----------------------------------------------------------*/
/* Exported functions ------------------------------------------------------- */
//uint8_t BSP_SuspendCPU2( void );
//uint8_t BSP_ResumeCPU2( void );
//uint8_t BSP_TouchUpdate(void);

//int BSP_ResourcesCopy(WM_HWIN hItem, FIL * pResFile, uint32_t Address);
int hw_flash_program_file(FIL * pResFile, uint32_t Address);
//int BSP_FlashUpdate(uint32_t Address, uint8_t *pData, uint32_t Size);

#endif /*__BSP_H */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
