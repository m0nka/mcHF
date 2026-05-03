/************************************************************************************
**                                                                                 **
**                                 mcHF QRP Transceiver                            **
**                         Krassi Atanassov - M0NKA, 2013-2026                     **
**                                                                                 **
**---------------------------------------------------------------------------------**
**                                                                                 **
**  File name:                                                                     **
**  Description:                                                                   **
**  Last Modified:                                                                 **
**  Licence:			https://github.com/m0nka/mcHF/blob/main/LICENSE            **
************************************************************************************/
#include "mchf_pro_board.h"
#include "main.h"

#include "board.h"
#include "sdram.h"

#define PWR_CFG_SMPS    0xCAFECAFE
#define PWR_CFG_LDO     0x5ACAFE5A

typedef struct pwr_db
{
  __IO uint32_t t[0x30/4];
  __IO uint32_t PDR1;

}PWDDBG_TypeDef;

/* Private macro -------------------------------------------------------------*/
#define PWDDBG                          ((PWDDBG_TypeDef*)PWR)
#define DEVICE_IS_CUT_2_1()             (HAL_GetREVID() & 0x21ff) ? 1 : 0

// Core unique regs loaded to RAM
struct	CM7_CORE_DETAILS		ccd;

// Public radio state
struct	TRANSCEIVER_STATE_UI	tsu;

__IO  uint32_t SystemClock_MHz 		= M7_CLOCK;
__IO  uint32_t SystemClock_changed 	= 0;

/**
  * @brief  System Clock Configuration to 400MHz
  *         The system Clock is configured as follow :
  *            System Clock source            = PLL (HSE)
  *            SYSCLK(Hz)                     = 400000000 (CM7 CPU Clock)
  *            HCLK(Hz)                       = 200000000 (CM4 CPU, AXI and AHBs Clock)
  *            AHB Prescaler                  = 2
  *            D1 APB3 Prescaler              = 2 (APB3 Clock  100MHz)
  *            D2 APB1 Prescaler              = 2 (APB1 Clock  100MHz)
  *            D2 APB2 Prescaler              = 2 (APB2 Clock  100MHz)
  *            D3 APB4 Prescaler              = 2 (APB4 Clock  100MHz)
  *            HSE Frequency(Hz)              = 25000000
  *            PLL_M                          = 5
  *            PLL_N                          = 160
  *            PLL_P                          = 2
  *            PLL_Q                          = 4
  *            PLL_R                          = 2
  * @param  None
  * @retval None
  */
static void SystemClock_Config_400MHz(void)
{
	RCC_OscInitTypeDef RCC_OscInitStruct;
	HAL_StatusTypeDef ret = HAL_OK;

	/* Enable HSE Oscillator and activate PLL with HSE as source */
	RCC_OscInitStruct.OscillatorType 	= RCC_OSCILLATORTYPE_HSE;
	RCC_OscInitStruct.HSEState 			= RCC_HSE_BYPASS;			//RCC_HSE_ON, changed to bypass, so can use PH1 as BAND3 GPIO
	RCC_OscInitStruct.HSIState 			= RCC_HSI_OFF;
	RCC_OscInitStruct.CSIState 			= RCC_CSI_OFF;
	RCC_OscInitStruct.PLL.PLLState 		= RCC_PLL_ON;
	RCC_OscInitStruct.PLL.PLLSource		= RCC_PLLSOURCE_HSE;

	RCC_OscInitStruct.PLL.PLLM 			= 5;
	RCC_OscInitStruct.PLL.PLLN 			= 160;
	RCC_OscInitStruct.PLL.PLLFRACN 		= 0;
	RCC_OscInitStruct.PLL.PLLP 			= 2;
	RCC_OscInitStruct.PLL.PLLR 			= 2;
	RCC_OscInitStruct.PLL.PLLQ 			= 4;

	RCC_OscInitStruct.PLL.PLLVCOSEL 	= RCC_PLL1VCOWIDE;
	RCC_OscInitStruct.PLL.PLLRGE 		= RCC_PLL1VCIRANGE_2;

	ret = HAL_RCC_OscConfig(&RCC_OscInitStruct);
	if(ret != HAL_OK)
	{
		Error_Handler(181);
	}
}

/**
  * @brief  System Clock Configuration to 480MHz
  *         The system Clock is configured as follow :
  *            System Clock source            = PLL (HSE)
  *            SYSCLK(Hz)                     = 480000000 (CM7 CPU Clock)
  *            HCLK(Hz)                       = 240000000 (CM4 CPU, AXI and AHBs Clock)
  *            AHB Prescaler                  = 2
  *            D1 APB3 Prescaler              = 2 (APB3 Clock  120MHz)
  *            D2 APB1 Prescaler              = 2 (APB1 Clock  120MHz)
  *            D2 APB2 Prescaler              = 2 (APB2 Clock  120MHz)
  *            D3 APB4 Prescaler              = 2 (APB4 Clock  120MHz)
  *            HSE Frequency(Hz)              = 25000000
  *            PLL_M                          = 5
  *            PLL_N                          = 192
  *            PLL_P                          = 2
  *            PLL_Q                          = 4
  *            PLL_R                          = 2
  * @param  None
  * @retval None
  */
static void SystemClock_Config_480MHz(void)
{
	RCC_OscInitTypeDef RCC_OscInitStruct;
	HAL_StatusTypeDef ret = HAL_OK;

	/* Enable HSE Oscillator and activate PLL with HSE as source */
	RCC_OscInitStruct.OscillatorType	= RCC_OSCILLATORTYPE_HSE;
	RCC_OscInitStruct.HSEState 			= RCC_HSE_BYPASS;
	RCC_OscInitStruct.HSIState 			= RCC_HSI_OFF;
	RCC_OscInitStruct.CSIState 			= RCC_CSI_OFF;
	RCC_OscInitStruct.PLL.PLLState 		= RCC_PLL_ON;
	RCC_OscInitStruct.PLL.PLLSource 	= RCC_PLLSOURCE_HSE;

	RCC_OscInitStruct.PLL.PLLM 			= 5;
	RCC_OscInitStruct.PLL.PLLN 			= 192;
	RCC_OscInitStruct.PLL.PLLFRACN 		= 0;
	RCC_OscInitStruct.PLL.PLLP 			= 2;
	RCC_OscInitStruct.PLL.PLLR 			= 2;
	RCC_OscInitStruct.PLL.PLLQ 			= 4;

	RCC_OscInitStruct.PLL.PLLVCOSEL = RCC_PLL1VCOWIDE;
	RCC_OscInitStruct.PLL.PLLRGE = RCC_PLL1VCIRANGE_2;

	ret = HAL_RCC_OscConfig(&RCC_OscInitStruct);
	if(ret != HAL_OK)
	{
		Error_Handler(180);
	}
}

void SystemClockChange_Handler(void)
{
	RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

	if(SystemClock_changed != 0)
	{
		/* Select HSE  as system clock source to allow modification of the PLL configuration */
		RCC_ClkInitStruct.ClockType       = RCC_CLOCKTYPE_SYSCLK;
		RCC_ClkInitStruct.SYSCLKSource    = RCC_SYSCLKSOURCE_HSE;
		if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_1) != HAL_OK)
		{
			/* Initialization Error */
			Error_Handler(178);
		}

		if(SystemClock_MHz == 400)
		{
			__HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE0);
			while(!__HAL_PWR_GET_FLAG(PWR_FLAG_VOSRDY)) {}

			SystemClock_Config_480MHz();
			SystemClock_MHz = 480;
		}
		else
		{
			SystemClock_Config_400MHz();

			__HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);
			while(!__HAL_PWR_GET_FLAG(PWR_FLAG_VOSRDY)) {}

			SystemClock_MHz = 400;
		}

		/* PLL1  as system clock source to allow modification of the PLL configuration */
		RCC_ClkInitStruct.ClockType       = RCC_CLOCKTYPE_SYSCLK;
		RCC_ClkInitStruct.SYSCLKSource    = RCC_SYSCLKSOURCE_PLLCLK;
		if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_4) != HAL_OK)
		{
			/* Initialization Error */
			Error_Handler(179);
		}

		SystemClock_changed = 0;
	}
}

void SystemClock_Config(void)
{
	RCC_ClkInitTypeDef RCC_ClkInitStruct;
	//RCC_PeriphCLKInitTypeDef RCC_PeriphClkInit;
	HAL_StatusTypeDef ret = HAL_OK;

	//#if (USE_VOS0_480MHZ_OVERCLOCK == 1)
	//#if (BOARD_HW_CONFIG_IS_LDO == 1) && (BOARD_HW_CONFIG_IS_DIRECT_SMPS == 0)
	#if defined(USE_PWR_LDO_SUPPLY)
	/*!< Supply configuration update enable */
	HAL_PWREx_ConfigSupply(PWR_LDO_SUPPLY);
	//#else
	//  #error "Please make sure that the STM32H747I-DISCO Board has been modified to match the LDO configuration then set the define BOARD_HW_CONFIG_IS_LDO to 1 and BOARD_HW_CONFIG_IS_DIRECT_SMPS set to 0 to confirm the HW config"
	//#endif  /* (BOARD_HW_CONFIG_IS_LDO == 1) && (BOARD_HW_CONFIG_IS_DIRECT_SMPS == 0) */
	#else
	//#if (BOARD_HW_CONFIG_IS_LDO == 0) && (BOARD_HW_CONFIG_IS_DIRECT_SMPS == 1)
	/*!< Supply configuration update enable */
	HAL_PWREx_ConfigSupply(PWR_DIRECT_SMPS_SUPPLY);
	//#else
	//  #error "Please make sure that the STM32H747I-DISCO Board has been modified to match the Direct SMPS configuration then set the define BOARD_HW_CONFIG_IS_LDO to 0 and BOARD_HW_CONFIG_IS_DIRECT_SMPS set to 1 to confirm the HW config"
	#endif  /* (BOARD_HW_CONFIG_IS_LDO == 0) && (BOARD_HW_CONFIG_IS_DIRECT_SMPS == 1) */
	//#endif /* USE_VOS0_480MHZ_OVERCLOCK */

	/* The voltage scaling allows optimizing the power consumption when the device is
     clocked below the maximum system frequency, to update the voltage scaling value
     regarding system frequency refer to product datasheet.  */
	__HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

	while(!__HAL_PWR_GET_FLAG(PWR_FLAG_VOSRDY)) {}

	//#if (USE_VOS0_480MHZ_OVERCLOCK == 1)
	if(SystemClock_MHz == 480)
	{
		__HAL_RCC_SYSCFG_CLK_ENABLE();
		__HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE0);
		while(!__HAL_PWR_GET_FLAG(PWR_FLAG_VOSRDY)) {}

		SystemClock_Config_480MHz();
	}
	else
	{
		//#else
		SystemClock_Config_400MHz();
		//#endif /* (USE_VOS0_480MHZ_OVERCLOCK == 1) */
	}

	/* Select PLL as system clock source and configure  bus clocks dividers */
	RCC_ClkInitStruct.ClockType = (RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_D1PCLK1 | RCC_CLOCKTYPE_PCLK1 | \
								   RCC_CLOCKTYPE_PCLK2  | RCC_CLOCKTYPE_D3PCLK1);

	RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
	RCC_ClkInitStruct.SYSCLKDivider = RCC_SYSCLK_DIV1;
	RCC_ClkInitStruct.AHBCLKDivider = RCC_HCLK_DIV2;
	RCC_ClkInitStruct.APB3CLKDivider = RCC_APB3_DIV2;
	RCC_ClkInitStruct.APB1CLKDivider = RCC_APB1_DIV2;
	RCC_ClkInitStruct.APB2CLKDivider = RCC_APB2_DIV2;
	RCC_ClkInitStruct.APB4CLKDivider = RCC_APB4_DIV2;
	ret = HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_4);
	if(ret != HAL_OK)
	{
		Error_Handler(177);
	}

	// Select SysClk as source of USART clocks
	#if 0
	RCC_PeriphClkInit.PeriphClockSelection  	= RCC_PERIPHCLK_USART1|RCC_PERIPHCLK_USART234578;
	RCC_PeriphClkInit.Usart234578ClockSelection = RCC_USART234578CLKSOURCE_D2PCLK1;
	RCC_PeriphClkInit.Usart16ClockSelection     = RCC_USART1CLKSOURCE_D2PCLK2;
	HAL_RCCEx_PeriphCLKConfig(&RCC_PeriphClkInit);
	#else
	//RCC_PeriphClkInit.PeriphClockSelection  	= RCC_PERIPHCLK_USART1;
	//RCC_PeriphClkInit.Usart16ClockSelection     = RCC_USART1CLKSOURCE_D2PCLK2;
	//HAL_RCCEx_PeriphCLKConfig(&RCC_PeriphClkInit);

	// Moved to low level driver init
	//RCC_PeriphClkInit.PeriphClockSelection  	= RCC_PERIPHCLK_USART234578;
	//RCC_PeriphClkInit.Usart234578ClockSelection = RCC_USART234578CLKSOURCE_D2PCLK1;
	//HAL_RCCEx_PeriphCLKConfig(&RCC_PeriphClkInit);
	#endif

	/*
  	  Note : The activation of the I/O Compensation Cell is recommended with communication  interfaces
          (GPIO, SPI, FMC, QSPI ...)  when  operating at  high frequencies(please refer to product datasheet)
          The I/O Compensation Cell activation  procedure requires :
        - The activation of the CSI clock
        - The activation of the SYSCFG clock
        - Enabling the I/O Compensation Cell : setting bit[0] of register SYSCFG_CCCSR
	 */

	/*activate CSI clock mondatory for I/O Compensation Cell*/
	__HAL_RCC_CSI_ENABLE() ;

	/* Enable SYSCFG clock mondatory for I/O Compensation Cell */
	__HAL_RCC_SYSCFG_CLK_ENABLE() ;

	/* Enables the I/O Compensation Cell */
	HAL_EnableCompensationCell();
}

void PeriphCommonClock_Config(void)
{
	LL_RCC_PLL2P_Enable();
	LL_RCC_PLL2_SetVCOInputRange(LL_RCC_PLLINPUTRANGE_8_16);
	LL_RCC_PLL2_SetVCOOutputRange(LL_RCC_PLLVCORANGE_MEDIUM);
	LL_RCC_PLL2_SetM(2);
	LL_RCC_PLL2_SetN(12);
	LL_RCC_PLL2_SetP(2);
	LL_RCC_PLL2_SetQ(2);
	LL_RCC_PLL2_SetR(2);
	LL_RCC_PLL2_Enable();

	// Wait till PLL is ready
	while(LL_RCC_PLL2_IsReady() != 1)
	{
	}
}

#if 0
/**
  * @brief  System Clock Configuration
  *         The system Clock is configured as follow :
  *            System Clock source            = PLL (HSE)
  *            SYSCLK(Hz)                     = 400000000 (Cortex-M7 CPU Clock)
  *            HCLK(Hz)                       = 200000000 (Cortex-M4 CPU, Bus matrix Clocks)
  *            AHB Prescaler                  = 2
  *            D1 APB3 Prescaler              = 2 (APB3 Clock  100MHz)
  *            D2 APB1 Prescaler              = 2 (APB1 Clock  100MHz)
  *            D2 APB2 Prescaler              = 2 (APB2 Clock  100MHz)
  *            D3 APB4 Prescaler              = 2 (APB4 Clock  100MHz)
  *            HSE Frequency(Hz)              = 25000000
  *            PLL_M                          = 5
  *            PLL_N                          = 160
  *            PLL_P                          = 2
  *            PLL_Q                          = 4
  *            PLL_R                          = 2
  *            VDD(V)                         = 3.3
  *            Flash Latency(WS)              = 4
  * @param  None
  * @retval None
  */
static void SystemClock_Config(void)
{
  RCC_ClkInitTypeDef RCC_ClkInitStruct;
  RCC_OscInitTypeDef RCC_OscInitStruct;
  RCC_PeriphCLKInitTypeDef PeriphClkInitStruct;
  HAL_StatusTypeDef ret = HAL_OK;

  /*!< Supply configuration update enable */
#if defined(USE_PWR_LDO_SUPPLY)
  HAL_PWREx_ConfigSupply(PWR_LDO_SUPPLY);
#else
  if(DEVICE_IS_CUT_2_1() == 0)
  {
    /* WA to avoid loosing SMPS regulation in run mode */
    PWDDBG->PDR1 = 0xCAFECAFE;
    __DSB();
    PWDDBG->PDR1 |= (1<<5 | 1<<3);
    __DSB();
  }
  HAL_PWREx_ConfigSupply(PWR_DIRECT_SMPS_SUPPLY);
#endif /* USE_PWR_LDO_SUPPLY */

  /* The voltage scaling allows optimizing the power consumption when the device is
     clocked below the maximum system frequency, to update the voltage scaling value
     regarding system frequency refer to product datasheet.  */
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  while(!__HAL_PWR_GET_FLAG(PWR_FLAG_VOSRDY)) {}

  /* Enable HSE Oscillator and activate PLL with HSE as source */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.HSIState = RCC_HSI_OFF;
  RCC_OscInitStruct.CSIState = RCC_CSI_OFF;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;

  RCC_OscInitStruct.PLL.PLLM = 5;
  RCC_OscInitStruct.PLL.PLLN = 160;
  RCC_OscInitStruct.PLL.PLLFRACN = 0;
  RCC_OscInitStruct.PLL.PLLP = 2;
  RCC_OscInitStruct.PLL.PLLR = 2;
  RCC_OscInitStruct.PLL.PLLQ = 4;

  RCC_OscInitStruct.PLL.PLLVCOSEL = RCC_PLL1VCOWIDE;
  RCC_OscInitStruct.PLL.PLLRGE = RCC_PLL1VCIRANGE_2;
  ret = HAL_RCC_OscConfig(&RCC_OscInitStruct);
  if(ret != HAL_OK)
  {
    Error_Handler(7);
  }

  /* Select PLL as system clock source and configure  bus clocks dividers */
  RCC_ClkInitStruct.ClockType = (RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_D1PCLK1 | RCC_CLOCKTYPE_PCLK1 | \
                                 RCC_CLOCKTYPE_PCLK2  | RCC_CLOCKTYPE_D3PCLK1);

  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.SYSCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_APB1_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_APB2_DIV2;
  RCC_ClkInitStruct.APB3CLKDivider = RCC_APB3_DIV2;
  RCC_ClkInitStruct.APB4CLKDivider = RCC_APB4_DIV2;
  ret = HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_4);
  if(ret != HAL_OK)
  {
    Error_Handler(8);
  }

  /* Configures the External Low Speed oscillator (LSE) drive capability */
  __HAL_RCC_LSEDRIVE_CONFIG(RCC_LSEDRIVE_HIGH);

  /*##-1- Configure LSE as RTC clock source ##################################*/
  RCC_OscInitStruct.OscillatorType =  RCC_OSCILLATORTYPE_LSI | RCC_OSCILLATORTYPE_LSE;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_NONE;
  RCC_OscInitStruct.LSEState = RCC_LSE_ON;
  RCC_OscInitStruct.LSIState = RCC_LSI_OFF;
  if(HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler(9);
  }

  PeriphClkInitStruct.PeriphClockSelection = RCC_PERIPHCLK_RTC;
  PeriphClkInitStruct.RTCClockSelection = RCC_RTCCLKSOURCE_LSE;
  if(HAL_RCCEx_PeriphCLKConfig(&PeriphClkInitStruct) != HAL_OK)
  {
    Error_Handler(10);
  }

  /*##-2- Enable RTC peripheral Clocks #######################################*/
  /* Enable RTC Clock */
  __HAL_RCC_RTC_ENABLE();

  /*
  Note : The activation of the I/O Compensation Cell is recommended with communication  interfaces
          (GPIO, SPI, FMC, QSPI ...)  when  operating at  high frequencies(please refer to product datasheet)
          The I/O Compensation Cell activation  procedure requires :
        - The activation of the CSI clock
        - The activation of the SYSCFG clock
        - Enabling the I/O Compensation Cell : setting bit[0] of register SYSCFG_CCCSR
  */

  __HAL_RCC_CSI_ENABLE() ;

  __HAL_RCC_SYSCFG_CLK_ENABLE() ;

  HAL_EnableCompensationCell();
}
#endif

void MPU_Config(void)
{
	MPU_Region_InitTypeDef MPU_InitStruct;

	// Disable the MPU
	HAL_MPU_Disable();

	// Setup Flash - launcher and radio code execution
	MPU_InitStruct.Enable           = MPU_REGION_ENABLE;
	MPU_InitStruct.BaseAddress      = FLASH_BASE;					// 0x08000000
	MPU_InitStruct.Size             = MPU_REGION_SIZE_2MB;			// 2MB
	MPU_InitStruct.AccessPermission = MPU_REGION_FULL_ACCESS;
	MPU_InitStruct.IsBufferable     = MPU_ACCESS_BUFFERABLE;
	MPU_InitStruct.IsCacheable      = MPU_ACCESS_CACHEABLE;
	MPU_InitStruct.IsShareable      = MPU_ACCESS_NOT_SHAREABLE;
	MPU_InitStruct.Number           = MPU_REGION_NUMBER0;
	MPU_InitStruct.TypeExtField     = MPU_TEX_LEVEL0;
	MPU_InitStruct.SubRegionDisable = 0x00;
	MPU_InitStruct.DisableExec      = MPU_INSTRUCTION_ACCESS_ENABLE;
	HAL_MPU_ConfigRegion(&MPU_InitStruct);

	#ifndef PCB_V9_REV_A
	MPU_InitStruct.Size             = MPU_REGION_SIZE_4MB;
	#else
	MPU_InitStruct.Size             = MPU_REGION_SIZE_16MB;
	#endif

	// Setup SDRAM - emWin video buffers + app region(rev 9)
	MPU_InitStruct.Enable           = MPU_REGION_ENABLE;
	MPU_InitStruct.BaseAddress      = SDRAM_DEVICE_ADDR;
	MPU_InitStruct.AccessPermission = MPU_REGION_FULL_ACCESS;
	MPU_InitStruct.IsBufferable     = MPU_ACCESS_NOT_BUFFERABLE;
	MPU_InitStruct.IsCacheable      = MPU_ACCESS_CACHEABLE;
	MPU_InitStruct.IsShareable      = MPU_ACCESS_NOT_SHAREABLE;
	MPU_InitStruct.Number           = MPU_REGION_NUMBER1;
	MPU_InitStruct.TypeExtField     = MPU_TEX_LEVEL0;
	MPU_InitStruct.SubRegionDisable = 0x00;
	MPU_InitStruct.DisableExec      = MPU_INSTRUCTION_ACCESS_DISABLE;
	HAL_MPU_ConfigRegion(&MPU_InitStruct);

	// Upper 8MB of ext SDRAM as app execution space
	#ifdef PCB_V9_REV_A
	MPU_InitStruct.Enable           = MPU_REGION_ENABLE;
	MPU_InitStruct.BaseAddress      = SDRAM_APP_ADDR;
	MPU_InitStruct.Size             = MPU_REGION_SIZE_8MB;
	MPU_InitStruct.AccessPermission = MPU_REGION_FULL_ACCESS;
	MPU_InitStruct.IsBufferable     = MPU_ACCESS_NOT_BUFFERABLE;
	MPU_InitStruct.IsCacheable      = MPU_ACCESS_CACHEABLE;
	MPU_InitStruct.IsShareable      = MPU_ACCESS_NOT_SHAREABLE;
	MPU_InitStruct.Number           = MPU_REGION_NUMBER2;
	MPU_InitStruct.TypeExtField     = MPU_TEX_LEVEL0;
	MPU_InitStruct.SubRegionDisable = 0x00;
	MPU_InitStruct.DisableExec      = MPU_INSTRUCTION_ACCESS_ENABLE;
	HAL_MPU_ConfigRegion(&MPU_InitStruct);
	#endif

	#ifndef PCB_V9_REV_A
	MPU_InitStruct.Number           = MPU_REGION_NUMBER2;
	#else
	MPU_InitStruct.Number 			= MPU_REGION_NUMBER3;
	#endif

	// Setup D3 SRAM - OpenAMP core to core comms
	MPU_InitStruct.Enable 			= MPU_REGION_ENABLE;
	MPU_InitStruct.BaseAddress 		= D3_SRAM_BASE;					// 0x38000000
	MPU_InitStruct.Size 			= MPU_REGION_SIZE_64KB;			// 64KB
	MPU_InitStruct.AccessPermission = MPU_REGION_FULL_ACCESS;
	MPU_InitStruct.IsBufferable 	= MPU_ACCESS_NOT_BUFFERABLE;
	MPU_InitStruct.IsCacheable 		= MPU_ACCESS_CACHEABLE;
	MPU_InitStruct.IsShareable 		= MPU_ACCESS_SHAREABLE;
	MPU_InitStruct.TypeExtField 	= MPU_TEX_LEVEL0;
	MPU_InitStruct.SubRegionDisable = 0x00;
	MPU_InitStruct.DisableExec 		= MPU_INSTRUCTION_ACCESS_DISABLE;
	HAL_MPU_ConfigRegion(&MPU_InitStruct);

	#ifndef PCB_V9_REV_A
	MPU_InitStruct.Number           = MPU_REGION_NUMBER3;
	#else
	MPU_InitStruct.Number 			= MPU_REGION_NUMBER4;
	#endif

	// Setup AXI SRAM - OS heap
	MPU_InitStruct.Enable           = MPU_REGION_ENABLE;
	MPU_InitStruct.BaseAddress      = D1_AXISRAM_BASE;				// 0x24000000
	MPU_InitStruct.Size             = MPU_REGION_SIZE_512KB;		// 512KB
	MPU_InitStruct.AccessPermission = MPU_REGION_FULL_ACCESS;
	MPU_InitStruct.IsBufferable     = MPU_ACCESS_NOT_BUFFERABLE;
	MPU_InitStruct.IsCacheable      = MPU_ACCESS_CACHEABLE;
	MPU_InitStruct.IsShareable      = MPU_ACCESS_NOT_SHAREABLE;
	MPU_InitStruct.TypeExtField     = MPU_TEX_LEVEL0;
	MPU_InitStruct.SubRegionDisable = 0x00;
	MPU_InitStruct.DisableExec      = MPU_INSTRUCTION_ACCESS_DISABLE;
	HAL_MPU_ConfigRegion(&MPU_InitStruct);

	#ifndef PCB_V9_REV_A
	MPU_InitStruct.Number           = MPU_REGION_NUMBER4;
	#else
	MPU_InitStruct.Number 			= MPU_REGION_NUMBER5;
	#endif

	// Setup SRAM1 + SRAM2, DSP executable code (code + data)
	MPU_InitStruct.Enable           = MPU_REGION_ENABLE;
	MPU_InitStruct.BaseAddress      = D2_AHBSRAM_BASE;				// 0x30000000
	MPU_InitStruct.Size             = MPU_REGION_SIZE_256KB;		// 256KB
	MPU_InitStruct.AccessPermission = MPU_REGION_FULL_ACCESS;
	MPU_InitStruct.IsBufferable     = MPU_ACCESS_NOT_BUFFERABLE;
	MPU_InitStruct.IsCacheable      = MPU_ACCESS_CACHEABLE;
	MPU_InitStruct.IsShareable      = MPU_ACCESS_NOT_SHAREABLE;
	MPU_InitStruct.TypeExtField     = MPU_TEX_LEVEL0;
	MPU_InitStruct.SubRegionDisable = 0x00;
	MPU_InitStruct.DisableExec      = MPU_INSTRUCTION_ACCESS_DISABLE;
	HAL_MPU_ConfigRegion(&MPU_InitStruct);

	#ifndef PCB_V9_REV_A
	MPU_InitStruct.Number           = MPU_REGION_NUMBER5;
	#else
	MPU_InitStruct.Number 			= MPU_REGION_NUMBER6;
	#endif

	// Setup SRAM3, D2 domain, HW peripherals DMA buffers
	MPU_InitStruct.Enable           = MPU_REGION_ENABLE;
	MPU_InitStruct.BaseAddress      = D2_AHBSRAM_BASE + 0x40000;	// 0x30040000
	MPU_InitStruct.Size             = MPU_REGION_SIZE_32KB;			// 32KB
	MPU_InitStruct.AccessPermission = MPU_REGION_FULL_ACCESS;
	MPU_InitStruct.IsBufferable     = MPU_ACCESS_NOT_BUFFERABLE;
	MPU_InitStruct.IsCacheable      = MPU_ACCESS_CACHEABLE;
	MPU_InitStruct.IsShareable      = MPU_ACCESS_NOT_SHAREABLE;
	MPU_InitStruct.TypeExtField     = MPU_TEX_LEVEL0;
	MPU_InitStruct.SubRegionDisable = 0x00;
	MPU_InitStruct.DisableExec      = MPU_INSTRUCTION_ACCESS_DISABLE;
	HAL_MPU_ConfigRegion(&MPU_InitStruct);

	#if 0
	// Setup ITCM RAM - OS code, interrupt handlers
	MPU_InitStruct.Enable           = MPU_REGION_ENABLE;
	MPU_InitStruct.BaseAddress      = D1_ITCMRAM_BASE;				// 0x00000000
	MPU_InitStruct.Size             = MPU_REGION_SIZE_64KB;			// 64KB
	MPU_InitStruct.AccessPermission = MPU_REGION_FULL_ACCESS;
	MPU_InitStruct.IsBufferable     = MPU_ACCESS_NOT_BUFFERABLE;
	MPU_InitStruct.IsCacheable      = MPU_ACCESS_CACHEABLE;
	MPU_InitStruct.IsShareable      = MPU_ACCESS_NOT_SHAREABLE;
	MPU_InitStruct.Number           = MPU_REGION_NUMBER6;
	MPU_InitStruct.TypeExtField     = MPU_TEX_LEVEL0;
	MPU_InitStruct.SubRegionDisable = 0x00;
	MPU_InitStruct.DisableExec      = MPU_INSTRUCTION_ACCESS_ENABLE;
	HAL_MPU_ConfigRegion(&MPU_InitStruct);

	// Setup DTCM RAM - stack, heap, RTOS heap ?
	MPU_InitStruct.Enable           = MPU_REGION_ENABLE;
	MPU_InitStruct.BaseAddress      = D1_DTCMRAM_BASE;				// 0x20000000
	MPU_InitStruct.Size             = MPU_REGION_SIZE_128KB;		// 128KB
	MPU_InitStruct.AccessPermission = MPU_REGION_FULL_ACCESS;
	MPU_InitStruct.IsBufferable     = MPU_ACCESS_NOT_BUFFERABLE;
	MPU_InitStruct.IsCacheable      = MPU_ACCESS_CACHEABLE;
	MPU_InitStruct.IsShareable      = MPU_ACCESS_NOT_SHAREABLE;
	MPU_InitStruct.Number           = MPU_REGION_NUMBER7;
	MPU_InitStruct.TypeExtField     = MPU_TEX_LEVEL0;
	MPU_InitStruct.SubRegionDisable = 0x00;
	MPU_InitStruct.DisableExec      = MPU_INSTRUCTION_ACCESS_DISABLE;
	HAL_MPU_ConfigRegion(&MPU_InitStruct);
	#endif

	// RTC Domain SRAM - not a great idea, needs manual flush on write
	#if 0
	MPU_InitStruct.Enable 			= MPU_REGION_ENABLE;
	MPU_InitStruct.BaseAddress 		= D3_BKPSRAM_BASE;					// 0x38800000
	MPU_InitStruct.Size 			= MPU_REGION_SIZE_4KB;				// 4KB
	MPU_InitStruct.AccessPermission = MPU_REGION_FULL_ACCESS;
	MPU_InitStruct.IsBufferable 	= MPU_ACCESS_NOT_BUFFERABLE;
	MPU_InitStruct.IsCacheable 		= MPU_ACCESS_CACHEABLE;
	MPU_InitStruct.IsShareable 		= MPU_ACCESS_SHAREABLE;
	MPU_InitStruct.Number 			= MPU_REGION_NUMBER6;
	MPU_InitStruct.TypeExtField 	= MPU_TEX_LEVEL0;
	MPU_InitStruct.SubRegionDisable = 0x00;
	MPU_InitStruct.DisableExec 		= MPU_INSTRUCTION_ACCESS_DISABLE;
	HAL_MPU_ConfigRegion(&MPU_InitStruct);
	#endif

    HAL_MPU_Enable(MPU_PRIVILEGED_DEFAULT);
}

void CPU_CACHE_Enable(void)
{
	/* Enable I-Cache */
	SCB_EnableICache();

	/* Enable D-Cache */
	SCB_EnableDCache();
}

static void bsp_backlight_init(void)
{
	  GPIO_InitTypeDef  gpio_init_structure;

	  /* LCD_BL_CTRL GPIO configuration */
	  //LCD_BL_CTRL_GPIO_CLK_ENABLE();

	  gpio_init_structure.Pin       = LCD_BL_CTRL_PIN;
	  gpio_init_structure.Mode      = GPIO_MODE_OUTPUT_PP;
	  gpio_init_structure.Speed     = GPIO_SPEED_FREQ_HIGH;

	  HAL_GPIO_Init(LCD_BL_CTRL_GPIO_PORT, &gpio_init_structure);

	  /* Assert back-light LCD_BL_CTRL pin */
	  //HAL_GPIO_WritePin(LCD_BL_CTRL_GPIO_PORT, LCD_BL_CTRL_PIN, GPIO_PIN_SET);
}

#ifdef CONTEXT_ICC
// M4 core Keyer IRQ setup
static void EXTI23_IRQHandler_Config(void)
{
	GPIO_InitTypeDef   GPIO_InitStructure;

	// Configure PC.13 pin as the EXTI input event line in interrupt mode for both CPU1 and CPU2
	GPIO_InitStructure.Mode 	= GPIO_MODE_IT_FALLING;
	GPIO_InitStructure.Pull 	= GPIO_PULLUP;
	GPIO_InitStructure.Speed 	= GPIO_SPEED_FREQ_VERY_HIGH;

	GPIO_InitStructure.Pin 		= PADDLE_DIT_PIN;
	HAL_GPIO_Init(PADDLE_DIT_PORT, &GPIO_InitStructure);

	GPIO_InitStructure.Pin 		= PADDLE_DAH;
	HAL_GPIO_Init(PADDLE_DAH_PIO, &GPIO_InitStructure);

	// Configure the second CPU (CM4) EXTI line for IT
	HAL_EXTI_D2_EventInputConfig(EXTI_LINE2 , EXTI_MODE_IT,  ENABLE);
	HAL_EXTI_D2_EventInputConfig(EXTI_LINE3 , EXTI_MODE_IT,  ENABLE);
}
#endif

// 5V, 8V
static void power_cntr_init(void)
{
	GPIO_InitTypeDef  gpio_init_structure;

	gpio_init_structure.Mode  = GPIO_MODE_OUTPUT_PP;
	gpio_init_structure.Pull  = GPIO_PULLDOWN;
	gpio_init_structure.Speed = GPIO_SPEED_FREQ_LOW;

	// Not needed because of RF board power mod!
	#if 0
	// -----------------------------------------------------
	// -----------------------------------------------------
	// When no batteries installed, there will be missing
	// LOAD_16V rail. We need to test and enable the charging
	// regulator (BMS should do that!!!)
	//
	// Temporary put it on, for testing, but with batteries
	// has to be removed !!!
	//
	// CHGR_ON is PD4, active low
	gpio_init_structure.Pin   = GPIO_PIN_4;
	HAL_GPIO_Init(GPIOD, &gpio_init_structure);
	// ON
	HAL_GPIO_WritePin(GPIOD, GPIO_PIN_4, GPIO_PIN_RESET);
	printf("######## CHARGER IS ON !!! ########\r\n");
	// -----------------------------------------------------
	// -----------------------------------------------------
	#endif

	// 5V on is PG10
	gpio_init_structure.Pin   = VCC_5V_ON;
	HAL_GPIO_Init(VCC_5V_ON_PORT, &gpio_init_structure);

	// 5V ON on start
	#if 0
	HAL_GPIO_WritePin(VCC_5V_ON_PORT, VCC_5V_ON, GPIO_PIN_RESET);
	#else
	HAL_GPIO_WritePin(VCC_5V_ON_PORT, VCC_5V_ON, GPIO_PIN_SET);
	#endif

	#if 0
	// 8V on is PG9
	gpio_init_structure.Pin   = GPIO_PIN_9;
	HAL_GPIO_Init(GPIOG, &gpio_init_structure);
	// 8V ON on start (actually 6V after mod)
	HAL_GPIO_WritePin(GPIOG, GPIO_PIN_9, GPIO_PIN_RESET);
	#endif
}

// Via stop mode
void bsp_power_off(void)
{
	//printf("power off in\r\n");

	// Stop all repaints
	#ifdef CONTEXT_VIDEO
	ui_proc_power_cleanup();
	#endif

	// Safely stop OS
	portDISABLE_INTERRUPTS();

	// Tasks hw cleanup
	#ifdef CONTEXT_AUDIO
	audio_proc_power_cleanup();
	#endif

	band_proc_power_cleanup();

	#ifdef CONTEXT_ROTARY
	rotary_proc_power_cleanup();
	#endif

	#ifdef CONTEXT_TOUCH
	touch_proc_power_cleanup();
	#endif

	#ifdef CONTEXT_TRX
	trx_proc_power_clean_up();
	#endif

	#ifdef CONTEXT_FAN
	fan_proc_power_clean_up();
	#endif

	#ifdef CONTEXT_VFO
	vfo_proc_power_cleanup();
	#endif

	radio_init_save_before_off();

	#ifdef CONTEXT_BMS
	bms_proc_power_cleanup();
	#endif

	#ifdef CONTEXT_LORA
	lora_proc_power_cleanup();
	#endif

	HAL_Delay(3000);

	#if 0
	// Enter reason for reset, so the bootloader doesn't power back on the radio
	WRITE_REG(BKP_REG_RESET_REASON, RESET_POWER_OFF);
	HAL_PWR_DisableBkUpAccess();
	// Restart to bootloader
	NVIC_SystemReset();
	#else
	HAL_GPIO_WritePin(LCD_BL_CTRL_GPIO_PORT, LCD_BL_CTRL_PIN, GPIO_PIN_RESET);
	LL_GPIO_ResetOutputPin(POWER_HOLD_PORT, POWER_HOLD);
	#endif
}

static void ptt_init(void)
{
	GPIO_InitTypeDef  gpio_init_structure;

	gpio_init_structure.Mode  = GPIO_MODE_OUTPUT_PP;
	gpio_init_structure.Pull  = GPIO_NOPULL;
	gpio_init_structure.Speed = GPIO_SPEED_FREQ_LOW;

	// PTT line
	gpio_init_structure.Pin   = PTT_PIN;
	HAL_GPIO_Init(PTT_PIN_PORT, &gpio_init_structure);

	// RX on start
	HAL_GPIO_WritePin(PTT_PIN_PORT, PTT_PIN, GPIO_PIN_RESET);
}

static void power_led_init(void)
{
	GPIO_InitTypeDef  gpio_init_structure;

	gpio_init_structure.Mode  = GPIO_MODE_OUTPUT_PP;
	gpio_init_structure.Pull  = GPIO_NOPULL;
	gpio_init_structure.Speed = GPIO_SPEED_FREQ_LOW;

	// PTT line
	gpio_init_structure.Pin   = ON_LED;
	HAL_GPIO_Init(ON_LED_PORT, &gpio_init_structure);

	// On
	HAL_GPIO_WritePin(ON_LED_PORT, ON_LED, GPIO_PIN_SET);
}

void bsp_hold_power(void)
{
#if 0
	GPIO_InitTypeDef  GPIO_InitStruct;

	__HAL_RCC_GPIOC_CLK_ENABLE();

	HAL_GPIO_WritePin(POWER_HOLD_PORT,POWER_HOLD, 1);	// hold power

	GPIO_InitStruct.Pin   = POWER_HOLD;
	GPIO_InitStruct.Mode  = GPIO_MODE_OUTPUT_PP;
	GPIO_InitStruct.Pull  = GPIO_PULLDOWN;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;

	HAL_GPIO_Init(POWER_HOLD_PORT, &GPIO_InitStruct);
#else
	LL_GPIO_InitTypeDef 		GPIO_InitStruct = {0};

	// This is first ever call, so enable gpio clock
	LL_AHB4_GRP1_EnableClock(LL_AHB4_GRP1_PERIPH_GPIOC);

	// Hold the regulator line
	LL_GPIO_SetOutputPin(POWER_HOLD_PORT, POWER_HOLD);

	GPIO_InitStruct.Pin 	= POWER_HOLD;
	GPIO_InitStruct.Mode 	= LL_GPIO_MODE_OUTPUT;
	GPIO_InitStruct.Pull 	= LL_GPIO_PULL_DOWN;
	LL_GPIO_Init(POWER_HOLD_PORT, &GPIO_InitStruct);


#endif
}

void bsp_gpio_clocks_on(void)
{
	// All GPIO clocks on
	__HAL_RCC_GPIOA_CLK_ENABLE();
	__HAL_RCC_GPIOB_CLK_ENABLE();
	__HAL_RCC_GPIOC_CLK_ENABLE();
	__HAL_RCC_GPIOD_CLK_ENABLE();
	__HAL_RCC_GPIOE_CLK_ENABLE();
	__HAL_RCC_GPIOF_CLK_ENABLE();
	__HAL_RCC_GPIOG_CLK_ENABLE();
	__HAL_RCC_GPIOH_CLK_ENABLE();
	__HAL_RCC_GPIOI_CLK_ENABLE();

	// GPS early off
	LL_GPIO_SetPinMode		(GPS_EN_PORT, GPS_EN_PIN, LL_GPIO_MODE_OUTPUT);
	LL_GPIO_ResetOutputPin	(GPS_EN_PORT, GPS_EN_PIN);
}

uint8_t bsp_config(void)
{
	//LL_GPIO_InitTypeDef 		GPIO_InitStruct = {0};

	// Enable CRC to Unlock GUI
	__HAL_RCC_CRC_CLK_ENABLE();
	//--MX_CRC_Init();

	printf_init(1);
	printf("---------------------------------  \r\n");
	printf("-->%s v: %d.%d.%d\r\n", DEVICE_STRING, MCHF_R_VER_MINOR, MCHF_R_VER_RELEASE, MCHF_R_VER_BUILD);

	// Useful during ushdr port
	#ifndef REV_0_8_4_PATCH__
	printf("== allow m4 core to take control and stall application processor == \r\n");
	HAL_Delay(500);
	bsp_wake_second_core();
	while(1);
	#endif

	power_cntr_init();

	power_led_init();

	ptt_init();

	bsp_backlight_init();

	// DSP core Keyer IRQ
	#ifdef CONTEXT_ICC
	EXTI23_IRQHandler_Config();
	#endif

	// Task hw basic init (after LCD Reset!)
	//--tasks_pre_os_init();

	// Print Clock configuration
	#if 0
	printf( "CPU clcok %dMHz, Periph %dMHz/%dMHz \r\n" , (HAL_RCCEx_GetD1SysClockFreq()/1000000U)
	                                                   , (HAL_RCC_GetPCLK1Freq()/1000000U)
													   , (HAL_RCC_GetPCLK2Freq()/1000000U) );
	#endif

	return 0;
}

