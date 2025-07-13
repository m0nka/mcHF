
#include "main.h"
#include "stm32f4xx_hal.h"
#include "adc.h"
#include "dac.h"
#include "dma.h"
#include "fatfs.h"
#include "i2c.h"
#include "i2s.h"
#include "rtc.h"
#include "spi.h"
#include "tim.h"
#include "usb_device.h"
#include "usb_host.h"
#include "gpio.h"
#include "fsmc.h"

void SystemClock_Config(void);
void Error_Handler(void);

#ifdef BOOTLOADER_BUILD
void MX_USB_HOST_Process(void);
#include "bootloader/bootloader_main.h"
#else

#include "uhsdr_main.h"

#endif

//
// ToDo: 1. change startup file
//		 2. replace HAL and LL
//		 3. adjust linker script
//		 4. add core notifications
//		 5. add debug print
//
int main(void)
{
	#ifdef BOOTLOADER_BUILD
    Bootloader_CheckAndGoForBootTarget();
    //  we need to do this as early as possible
	#endif

	#ifdef H7_M4_CORE
    // Stall, wait core notification...
    // ToDo: ..
	#endif

    HAL_Init();
    HAL_RCC_DeInit();

    // Clock init already done by M7 core
	#ifndef H7_M4_CORE
    SystemClock_Config();
	#endif

	#ifdef BOOTLOADER_BUILD
    Bootloader_Main();
	#endif

    // Initialize all configured peripherals
	#if !defined(BOOTLOADER_BUILD) && !defined(H7_M4_CORE)
    MX_GPIO_Init();
    MX_DMA_Init();
    MX_ADC1_Init();
    MX_ADC2_Init();
    MX_ADC3_Init();
    MX_DAC_Init();
    MX_I2C1_Init();
    MX_I2C2_Init();
    MX_I2S3_Init();
    MX_RTC_Init();
    MX_SPI2_Init();
    MX_TIM3_Init();
    MX_TIM5_Init();
    MX_TIM8_Init();
    MX_USB_DEVICE_Init();
	#endif

    // Initialize all configured peripherals
	#ifdef H7_M4_CORE
    // ... driver specific init
    // ToDo: ...
	#endif

    #if defined(USE_USBHOST) || defined(BOOTLOADER_BUILD)
//!    MX_USB_HOST_Init();
	#if defined(USE_USBDRIVE) || defined(BOOTLOADER_BUILD)
    MX_FATFS_Init();
	#endif  // MX_FSMC_Init();
	#endif

    #ifndef BOOTLOADER_BUILD
//!    MX_TIM4_Init();
	#endif

	#ifndef BOOTLOADER_BUILD
//!    mchfMain();
    while(1);
	#else
    while (1)
    {
    	MX_USB_HOST_Process();
    	Bootloader_UsbHostApplication();
    }
	#endif
}

/** System Clock Configuration
*/
void SystemClock_Config(void)
{

  RCC_OscInitTypeDef RCC_OscInitStruct;
  RCC_ClkInitTypeDef RCC_ClkInitStruct;
  RCC_PeriphCLKInitTypeDef PeriphClkInitStruct;

    /**Configure the main internal regulator output voltage 
    */
  __HAL_RCC_PWR_CLK_ENABLE();

  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

    /**Initializes the CPU, AHB and APB busses clocks 
    */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.LSEState = RCC_LSE_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = 10;
  RCC_OscInitStruct.PLL.PLLN = 210;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 7;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

    /**Initializes the CPU, AHB and APB busses clocks 
    */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_5) != HAL_OK)
  {
    Error_Handler();
  }

  PeriphClkInitStruct.PeriphClockSelection = RCC_PERIPHCLK_I2S|RCC_PERIPHCLK_I2S
                              |RCC_PERIPHCLK_RTC;
  PeriphClkInitStruct.PLLI2S.PLLI2SN = 192;
  PeriphClkInitStruct.PLLI2S.PLLI2SR = 5;
  PeriphClkInitStruct.RTCClockSelection = RCC_RTCCLKSOURCE_LSE;
  if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  HAL_RCC_MCOConfig(RCC_MCO2, RCC_MCO2SOURCE_PLLI2SCLK, RCC_MCODIV_5);

    /**Configure the Systick interrupt time 
    */
  HAL_SYSTICK_Config(HAL_RCC_GetHCLKFreq()/1000);

    /**Configure the Systick 
    */
  HAL_SYSTICK_CLKSourceConfig(SYSTICK_CLKSOURCE_HCLK);

  /* SysTick_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(SysTick_IRQn, 0, 0);
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @param  None
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler */
  /* User can add his own implementation to report the HAL error return state */
  while(1) 
  {
  }
  /* USER CODE END Error_Handler */ 
}

#ifdef USE_FULL_ASSERT

/**
   * @brief Reports the name of the source file and the source line number
   * where the assert_param error has occurred.
   * @param file: pointer to the source file name
   * @param line: assert_param error line source number
   * @retval None
   */
void assert_failed(uint8_t* file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
    ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
    for(;;); // We stuck here as long as we do not have any debug interface...
  /* USER CODE END 6 */
}

#endif
