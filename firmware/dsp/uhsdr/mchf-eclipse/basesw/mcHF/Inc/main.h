#ifndef __MAIN_H
#define __MAIN_H

#ifdef H7_M4_CORE
#include "stm32h7xx_hal.h"
#include <stdio.h>
#include <stdint.h>

// -------------------------------------------------------------------
// Hardware semaphores for core to core comms
//
// OpenAMP
#define HSEM_ID_0           0 			// CM7 to CM4 Notification
#define HSEM_ID_1           1 			// CM4 to CM7 Notification

#define	MCHF_D_VER_MAJOR			0
#define	MCHF_D_VER_MINOR			2
#define	MCHF_D_VER_RELEASE			12
#define	MCHF_D_VER_BUILD			4

#endif

/* Private define ------------------------------------------------------------*/
#ifndef H7_M4_CORE
#define BUTTON_PWR_Pin GPIO_PIN_13
#define BUTTON_PWR_GPIO_Port GPIOC
#define PADDLE_DAH_Pin GPIO_PIN_0
#define PADDLE_DAH_GPIO_Port GPIOE
#define PADDLE_DIT_Pin GPIO_PIN_1
#define PADDLE_DIT_GPIO_Port GPIOE
#endif
/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

/**
  * @}
  */ 

/**
  * @}
*/ 

#endif /* __MAIN_H */
/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
