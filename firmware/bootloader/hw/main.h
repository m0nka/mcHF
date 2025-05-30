/**
  ******************************************************************************
  * @file    main.h
  * @author  MCD Application Team
  * @brief   Header for main.c file
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
#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
 extern "C" {
#endif

//#include "mchf_types.h"

//#define CONTEXT_IPC_PROC
//#define GUI_TASK
//#define TS_TASK

#define IPC_PROC_START_DELAY 	500

#define NEXT_LINE_POS	25

/* Includes ------------------------------------------------------------------*/
#ifndef WIN32
#include "stm32h7xx_hal.h"

#include "stm32h747i_discovery.h"
//#include "stm32h747i_discovery_sd.h"
//#include "stm32h747i_discovery_ts.h"
//#include "stm32h747i_discovery_qspi.h"
//#include "stm32h747i_discovery_sdram.h"
//#include "stm32h747i_discovery_bus.h"
//#include "otm8009a.h"
//#include "cmsis_os.h"

#include "stm32h7xx_ll_bus.h"
#include "stm32h7xx_ll_rcc.h"
#include "stm32h7xx_ll_system.h"
#include "stm32h7xx_ll_utils.h"
#include "stm32h7xx_ll_gpio.h"
#include "stm32h7xx_ll_exti.h"
#include "stm32h7xx_ll_spi.h"
#include "stm32h7xx_ll_pwr.h"
#include "stm32h7xx_ll_adc.h"
#include "stm32h7xx_ll_tim.h"
//#include "stm32h7xx_ll_dma.h"
//#include "stm32h7xx_ll_bdma.h"

/* FatFs includes component */
//#include "ff_gen_drv.h"

#endif /* !WIN32 */

 typedef  void (*pFunc)(void);

 #define PWR_CFG_SMPS    0xCAFECAFE
 #define PWR_CFG_LDO     0x5ACAFE5A

 typedef struct pwr_db
 {
   __IO uint32_t t[0x30/4];
   __IO uint32_t PDR1;
 } PWDDBG_TypeDef;

 #define PWDDBG                          ((PWDDBG_TypeDef*)PWR)
 #define DEVICE_IS_CUT_2_1()             (HAL_GetREVID() & 0x21ff) ? 1 : 0

//#define BKP_REG_PWR_CFG           (RTC->BKP28R)
//#define BKP_REG_SW_CFG            (RTC->BKP27R)
//#define BKP_REG_SUBDEMO_ADDRESS   (RTC->BKP26R)
//#define BKP_REG_CALIB_DR0         (RTC->BKP25R)
//#define BKP_REG_CALIB_DR1         (RTC->BKP24R)

/* GUI includes components */
//#include "GUI.h"
//#include "DIALOG.h"
//#include "LCDConf.h"
#ifndef WIN32
//#include "ST_GUI_Addons.h"
#endif /* !WIN32 */

#ifndef WIN32
/* Kernel includes components */
//#include "storage.h"
//#include "calibration.h"
//#include "ui_proc.h"
#endif /* !WIN32 */

/* standard includes components */
#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#if 0
/* Exported types ------------------------------------------------------------*/
typedef struct Bitmap_Res_s
{
  uint16_t       x;
  uint16_t       y;
} Bitmap_Res_t;

typedef struct Demo_Header_s
{
  uint32_t       demo_id;
  uint32_t       icon_addr;
  Bitmap_Res_t   icon_res;
  uint32_t       logo_addr;
  Bitmap_Res_t   logo_res;
  uint32_t       sw_flags;
  uint32_t       reserved[2];
  unsigned char  demo_descr[32];
} Demo_Header_t;

typedef enum Demo_Id_e
{
  DEMO_ID_0,
  DEMO_ID_1,
  DEMO_ID_2,
  DEMO_ID_3,
  DEMO_ID_4,
  DEMO_ID_INVALID,
} Demo_Id_t;

typedef enum AutoDemoEvent_e
{
  AUTO_DEMO_DISABLE,
  AUTO_DEMO_ENABLE,
  AUTO_DEMO_RESET
} AutoDemoEvent_t;

/* Exported variables --------------------------------------------------------*/
/* Exported constants --------------------------------------------------------*/
#define SW_FLAG_AUTO_DEMO         (1L << 0)
#endif

#if 0
#define	TASK_PROC_IDLE				0
#define	TASK_PROC_WORK				1
#define	TASK_PROC_DONE				2

struct ESPMessage {

	uchar 	ucMessageID;
	uchar	ucProcStatus;
	uchar	ucDataReady;
	uchar	ucExecResult;

	uchar 	ucData[128];

} ESPMessage;
#endif

/* Exported macros -----------------------------------------------------------*/
/* Exported functions ------------------------------------------------------- */
void Error_Handler(int err);
void BSP_ErrorHandler(void);

//void printf_init(uchar is_shared);

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
