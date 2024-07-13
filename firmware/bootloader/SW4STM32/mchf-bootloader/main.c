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
#include "mchf_pro_board.h"
#include "main.h"
#include "version.h"

//#include "bsp.h"

#include "lcd_low.h"
#include "lcd_high.h"

#include "ipc_proc.h"

#include "hw_lcd.h"
#include "hw_sdram.h"

#include "hw_sd.h"
#include "ff_gen_drv.h"
#include "sd_diskio.h"

#include "hw_flash.h"

//uint32_t wakeup_pressed = 0; /* wakeup_pressed = 1 ==> User request calibration */

static uint32_t LCD_X_Size = 0;
static uint32_t LCD_Y_Size = 0;

uint32_t 	thirdPartyAdress = 0;
RTC_HandleTypeDef RtcHandle;

CRC_HandleTypeDef   CrcHandle;

extern LTDC_HandleTypeDef hltdc;

int 	dsp_core_stat;
ulong	reset_reason;
uchar   gen_boot_reason_err = 0;

extern const unsigned char dsp_idle[816];

extern SD_HandleTypeDef hsd_sdmmc[1];
FATFS SDFatFs;  						/* File system object for SD card logical drive */
FIL MyFile;     						/* File object */
char SDPath[4]; 						/* SD card logical drive path */

void NMI_Handler(void)
{
  Error_Handler(11);
}

void HardFault_Handler(void)
{
	printf( "== HARD FAULT ==\n");
	while(1);
}

void MemManage_Handler(void)
{
  Error_Handler(13);
}

void BusFault_Handler(void)
{
  Error_Handler(14);
}

void UsageFault_Handler(void)
{
  Error_Handler(15);
}

void DebugMon_Handler(void)
{
}

void SysTick_Handler(void)
{
	HAL_IncTick();
}

#if 0
#define USE_RTC
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
  if (GPIO_Pin == GPIO_PIN_13)
  {
	  printf("yeah\r\n");
  }
}
void EXTI15_10_IRQHandler(void)
{
  HAL_GPIO_EXTI_IRQHandler(BUTTON_WAKEUP_PIN);
}
static void TS_EXTI_Callback(void)
{
	//BSP_TS_Callback(0);
	printf("hehe\r\n");
}
//EXTI_HandleTypeDef hts_exti;
EXTI_HandleTypeDef hts_exti[1] = {0};
#ifdef USE_RTC
RTC_HandleTypeDef RtcHandle;
#define RTC_ASYNCH_PREDIV  0x7F   /* LSE as RTC clock */
#define RTC_SYNCH_PREDIV   0x00FF /* LSE as RTC clock */
#endif
static void EXTI15_10_IRQHandler_Config(void)
{
	GPIO_InitTypeDef   GPIO_InitStructure;

	#ifdef USE_RTC
	RtcHandle.Instance 				= RTC;
	RtcHandle.Init.HourFormat 		= RTC_HOURFORMAT_24;
	RtcHandle.Init.AsynchPrediv 	= RTC_ASYNCH_PREDIV;
	RtcHandle.Init.SynchPrediv 		= RTC_SYNCH_PREDIV;
	RtcHandle.Init.OutPut 			= RTC_OUTPUT_DISABLE;
	RtcHandle.Init.OutPutPolarity 	= RTC_OUTPUT_POLARITY_HIGH;
	RtcHandle.Init.OutPutType 		= RTC_OUTPUT_TYPE_OPENDRAIN;

	// Disable the write protection for RTC registers
	__HAL_RTC_WRITEPROTECTION_DISABLE	(&RtcHandle);

	// Reference manual, page 2081
	RtcHandle.Instance->CR		&= ~RTC_CR_OSEL_0;
	RtcHandle.Instance->CR 		&= ~RTC_CR_OSEL_1;
	RtcHandle.Instance->CR 		&= ~(RTC_CR_COE);
	RtcHandle.Instance->TAMPCR  |= (RTC_TAMPCR_TAMP1E);
	RtcHandle.Instance->CR 	    &= ~(RTC_CR_TSE);

	// Enable the write protection for RTC registers
	__HAL_RTC_WRITEPROTECTION_ENABLE(&RtcHandle);

	if(HAL_RTC_Init(&RtcHandle) != HAL_OK)
	{
	}
	#endif

	/* Configure PC.13 pin as the EXTI input event line in interrupt mode for both CPU1 and CPU2*/
	GPIO_InitStructure.Pin 		= GPIO_PIN_13;
	GPIO_InitStructure.Mode 	= GPIO_MODE_IT_RISING;
	GPIO_InitStructure.Pull 	= GPIO_PULLDOWN;
	//GPIO_InitStructure.Speed	= GPIO_SPEED_FREQ_HIGH;
	HAL_GPIO_Init(GPIOC, &GPIO_InitStructure);

	HAL_EXTI_D1_EventInputConfig	(EXTI_LINE13 , EXTI_MODE_IT,  ENABLE);
	(void)HAL_EXTI_GetHandle		(&hts_exti[0], EXTI_LINE_13);
	(void)HAL_EXTI_RegisterCallback	(&hts_exti[0], HAL_EXTI_COMMON_CB_ID, TS_EXTI_Callback);

	/* Enable and set EXTI lines 15 to 10 Interrupt to the lowest priority */
	HAL_NVIC_SetPriority(EXTI15_10_IRQn, 15, 0);
	HAL_NVIC_EnableIRQ(EXTI15_10_IRQn);
}
#endif

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
void SystemClock_Config(void)
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
  RCC_OscInitStruct.HSEState 		= RCC_HSE_ON;
  RCC_OscInitStruct.HSIState 		= RCC_HSI_OFF;
  RCC_OscInitStruct.CSIState 		= RCC_CSI_OFF;
  RCC_OscInitStruct.PLL.PLLState 	= RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource 	= RCC_PLLSOURCE_HSE;

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
  RCC_ClkInitStruct.ClockType = ( RCC_CLOCKTYPE_SYSCLK 	|\
		  	  	  	  	  	  	  RCC_CLOCKTYPE_HCLK	|\
		  	  	  	  	  	  	  RCC_CLOCKTYPE_D1PCLK1 |\
								  RCC_CLOCKTYPE_PCLK1 	|\
								  RCC_CLOCKTYPE_PCLK2	|\
								  RCC_CLOCKTYPE_D3PCLK1
  	  	  	  	  	  	  	  	  );

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

	#ifdef USE_LSE
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
#else
  /*##-1- Configure LSE as RTC clock source ##################################*/
  RCC_OscInitStruct.OscillatorType =  RCC_OSCILLATORTYPE_LSI | RCC_OSCILLATORTYPE_LSE;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_NONE;
  RCC_OscInitStruct.LSEState = RCC_LSE_OFF;
  RCC_OscInitStruct.LSIState = RCC_LSI_ON;
  if(HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler(9);
  }

  PeriphClkInitStruct.PeriphClockSelection = RCC_PERIPHCLK_RTC;
  PeriphClkInitStruct.RTCClockSelection = RCC_RTCCLKSOURCE_LSI;
  if(HAL_RCCEx_PeriphCLKConfig(&PeriphClkInitStruct) != HAL_OK)
  {
    Error_Handler(10);
  }

  /*##-2- Enable RTC peripheral Clocks #######################################*/
  /* Enable RTC Clock */
  __HAL_RCC_RTC_ENABLE();
#endif
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

void MPU_Config(void)
{
	MPU_Region_InitTypeDef MPU_InitStruct;

	// Disable the MPU
	HAL_MPU_Disable();
#if 0
	// Configure the MPU attributes as WB for Flash
	MPU_InitStruct.Enable           = MPU_REGION_ENABLE;
	MPU_InitStruct.BaseAddress      = FLASH_BASE;
	MPU_InitStruct.Size             = MPU_REGION_SIZE_2MB;
	MPU_InitStruct.AccessPermission = MPU_REGION_FULL_ACCESS;
	MPU_InitStruct.IsBufferable     = MPU_ACCESS_BUFFERABLE;
	MPU_InitStruct.IsCacheable      = MPU_ACCESS_CACHEABLE;
	MPU_InitStruct.IsShareable      = MPU_ACCESS_NOT_SHAREABLE;
	MPU_InitStruct.Number           = MPU_REGION_NUMBER0;
	MPU_InitStruct.TypeExtField     = MPU_TEX_LEVEL0;
	MPU_InitStruct.SubRegionDisable = 0x00;
	MPU_InitStruct.DisableExec      = MPU_INSTRUCTION_ACCESS_ENABLE;
	HAL_MPU_ConfigRegion(&MPU_InitStruct);

	// Setup SDRAM in Write-through (framebuffer)
	MPU_InitStruct.Enable           = MPU_REGION_ENABLE;
	MPU_InitStruct.BaseAddress      = SDRAM_DEVICE_ADDR;
	MPU_InitStruct.Size             = MPU_REGION_SIZE_32MB;
	MPU_InitStruct.AccessPermission = MPU_REGION_FULL_ACCESS;
	MPU_InitStruct.IsBufferable     = MPU_ACCESS_NOT_BUFFERABLE;
	MPU_InitStruct.IsCacheable      = MPU_ACCESS_CACHEABLE;
	MPU_InitStruct.IsShareable      = MPU_ACCESS_NOT_SHAREABLE;
	MPU_InitStruct.Number           = MPU_REGION_NUMBER1;
	MPU_InitStruct.TypeExtField     = MPU_TEX_LEVEL0;
	MPU_InitStruct.SubRegionDisable = 0x00;
	MPU_InitStruct.DisableExec      = MPU_INSTRUCTION_ACCESS_DISABLE;
	HAL_MPU_ConfigRegion(&MPU_InitStruct);

	/* Setup AXI SRAM, SRAM1 and SRAM2 in Write-through */
	MPU_InitStruct.Enable           = MPU_REGION_ENABLE;
	MPU_InitStruct.BaseAddress      = D1_AXISRAM_BASE;
	MPU_InitStruct.Size             = MPU_REGION_SIZE_512KB;
	MPU_InitStruct.AccessPermission = MPU_REGION_FULL_ACCESS;
	MPU_InitStruct.IsBufferable     = MPU_ACCESS_NOT_BUFFERABLE;
	MPU_InitStruct.IsCacheable      = MPU_ACCESS_CACHEABLE;
	MPU_InitStruct.IsShareable      = MPU_ACCESS_NOT_SHAREABLE;
	MPU_InitStruct.Number           = MPU_REGION_NUMBER2;
	MPU_InitStruct.TypeExtField     = MPU_TEX_LEVEL0;
	MPU_InitStruct.SubRegionDisable = 0x00;
	MPU_InitStruct.DisableExec      = MPU_INSTRUCTION_ACCESS_DISABLE;
	HAL_MPU_ConfigRegion(&MPU_InitStruct);

	/* Setup D2 SRAM1 & SRAM2 in Write-through */
	MPU_InitStruct.Enable           = MPU_REGION_ENABLE;
	MPU_InitStruct.BaseAddress      = D2_AXISRAM_BASE;
	MPU_InitStruct.Size             = MPU_REGION_SIZE_256KB;
	MPU_InitStruct.AccessPermission = MPU_REGION_FULL_ACCESS;
	MPU_InitStruct.IsBufferable     = MPU_ACCESS_NOT_BUFFERABLE;
	MPU_InitStruct.IsCacheable      = MPU_ACCESS_CACHEABLE;
	MPU_InitStruct.IsShareable      = MPU_ACCESS_NOT_SHAREABLE;
	MPU_InitStruct.Number           = MPU_REGION_NUMBER3;
	MPU_InitStruct.TypeExtField     = MPU_TEX_LEVEL0;
	MPU_InitStruct.SubRegionDisable = 0x00;
	MPU_InitStruct.DisableExec      = MPU_INSTRUCTION_ACCESS_DISABLE;
	HAL_MPU_ConfigRegion(&MPU_InitStruct);
#endif


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

	// Setup SDRAM - emWin video buffers
	MPU_InitStruct.Enable           = MPU_REGION_ENABLE;
	MPU_InitStruct.BaseAddress      = SDRAM_DEVICE_ADDR;			// 0xD0000000
	MPU_InitStruct.Size             = MPU_REGION_SIZE_4MB;			// 32MB
	MPU_InitStruct.AccessPermission = MPU_REGION_FULL_ACCESS;
	MPU_InitStruct.IsBufferable     = MPU_ACCESS_NOT_BUFFERABLE;
	MPU_InitStruct.IsCacheable      = MPU_ACCESS_CACHEABLE;
	MPU_InitStruct.IsShareable      = MPU_ACCESS_NOT_SHAREABLE;
	MPU_InitStruct.Number           = MPU_REGION_NUMBER1;
	MPU_InitStruct.TypeExtField     = MPU_TEX_LEVEL0;
	MPU_InitStruct.SubRegionDisable = 0x00;
	MPU_InitStruct.DisableExec      = MPU_INSTRUCTION_ACCESS_DISABLE;
	HAL_MPU_ConfigRegion(&MPU_InitStruct);

#if 0	// in radio!!
	// Setup D3 SRAM - OpenAMP core to core comms
	MPU_InitStruct.Enable 			= MPU_REGION_ENABLE;
	MPU_InitStruct.BaseAddress 		= D3_SRAM_BASE;					// 0x38000000
	MPU_InitStruct.Size 			= MPU_REGION_SIZE_64KB;			// 64KB
	MPU_InitStruct.AccessPermission = MPU_REGION_FULL_ACCESS;
	MPU_InitStruct.IsBufferable 	= MPU_ACCESS_NOT_BUFFERABLE;
	MPU_InitStruct.IsCacheable 		= MPU_ACCESS_CACHEABLE;
	MPU_InitStruct.IsShareable 		= MPU_ACCESS_SHAREABLE;
	MPU_InitStruct.Number 			= MPU_REGION_NUMBER2;
	MPU_InitStruct.TypeExtField 	= MPU_TEX_LEVEL0;
	MPU_InitStruct.SubRegionDisable = 0x00;
	MPU_InitStruct.DisableExec 		= MPU_INSTRUCTION_ACCESS_DISABLE;
	HAL_MPU_ConfigRegion(&MPU_InitStruct);
#endif

	// Setup AXI SRAM - OS heap
	MPU_InitStruct.Enable           = MPU_REGION_ENABLE;
	MPU_InitStruct.BaseAddress      = D1_AXISRAM_BASE;				// 0x24000000
	MPU_InitStruct.Size             = MPU_REGION_SIZE_512KB;		// 512KB
	MPU_InitStruct.AccessPermission = MPU_REGION_FULL_ACCESS;
	MPU_InitStruct.IsBufferable     = MPU_ACCESS_NOT_BUFFERABLE;
	MPU_InitStruct.IsCacheable      = MPU_ACCESS_CACHEABLE;
	MPU_InitStruct.IsShareable      = MPU_ACCESS_NOT_SHAREABLE;
	MPU_InitStruct.Number           = MPU_REGION_NUMBER2;
	MPU_InitStruct.TypeExtField     = MPU_TEX_LEVEL0;
	MPU_InitStruct.SubRegionDisable = 0x00;
	MPU_InitStruct.DisableExec      = MPU_INSTRUCTION_ACCESS_DISABLE;
	HAL_MPU_ConfigRegion(&MPU_InitStruct);

	// Setup SRAM1 + SRAM2, DSP executable code (code + data)
	MPU_InitStruct.Enable           = MPU_REGION_ENABLE;
	MPU_InitStruct.BaseAddress      = D2_AHBSRAM_BASE;				// 0x30000000
	MPU_InitStruct.Size             = MPU_REGION_SIZE_256KB;		// 256KB
	MPU_InitStruct.AccessPermission = MPU_REGION_FULL_ACCESS;
	MPU_InitStruct.IsBufferable     = MPU_ACCESS_NOT_BUFFERABLE;
	MPU_InitStruct.IsCacheable      = MPU_ACCESS_CACHEABLE;
	MPU_InitStruct.IsShareable      = MPU_ACCESS_NOT_SHAREABLE;
	MPU_InitStruct.Number           = MPU_REGION_NUMBER3;
	MPU_InitStruct.TypeExtField     = MPU_TEX_LEVEL0;
	MPU_InitStruct.SubRegionDisable = 0x00;
	MPU_InitStruct.DisableExec      = MPU_INSTRUCTION_ACCESS_ENABLE;
	HAL_MPU_ConfigRegion(&MPU_InitStruct);

	#if 0
	// Setup ITCM RAM - OS code, interrupt handlers
	MPU_InitStruct.Enable           = MPU_REGION_ENABLE;
	MPU_InitStruct.BaseAddress      = D1_ITCMRAM_BASE;				// 0x00000000
	MPU_InitStruct.Size             = MPU_REGION_SIZE_64KB;			// 64KB
	MPU_InitStruct.AccessPermission = MPU_REGION_FULL_ACCESS;
	MPU_InitStruct.IsBufferable     = MPU_ACCESS_NOT_BUFFERABLE;
	MPU_InitStruct.IsCacheable      = MPU_ACCESS_NOT_CACHEABLE;
	MPU_InitStruct.IsShareable      = MPU_ACCESS_NOT_SHAREABLE;
	MPU_InitStruct.Number           = MPU_REGION_NUMBER4;
	MPU_InitStruct.TypeExtField     = MPU_TEX_LEVEL0;
	MPU_InitStruct.SubRegionDisable = 0x00;
	MPU_InitStruct.DisableExec      = MPU_INSTRUCTION_ACCESS_ENABLE;
	HAL_MPU_ConfigRegion(&MPU_InitStruct);

	// Setup DTCM RAM - DMA buffers
	MPU_InitStruct.Enable           = MPU_REGION_ENABLE;
	MPU_InitStruct.BaseAddress      = D1_DTCMRAM_BASE;				// 0x20000000
	MPU_InitStruct.Size             = MPU_REGION_SIZE_128KB;		// 128KB
	MPU_InitStruct.AccessPermission = MPU_REGION_FULL_ACCESS;
	MPU_InitStruct.IsBufferable     = MPU_ACCESS_NOT_BUFFERABLE;
	MPU_InitStruct.IsCacheable      = MPU_ACCESS_NOT_CACHEABLE;
	MPU_InitStruct.IsShareable      = MPU_ACCESS_NOT_SHAREABLE;
	MPU_InitStruct.Number           = MPU_REGION_NUMBER5;
	MPU_InitStruct.TypeExtField     = MPU_TEX_LEVEL0;
	MPU_InitStruct.SubRegionDisable = 0x00;
	MPU_InitStruct.DisableExec      = MPU_INSTRUCTION_ACCESS_DISABLE;
	HAL_MPU_ConfigRegion(&MPU_InitStruct);
	#endif


	HAL_MPU_Enable(MPU_PRIVILEGED_DEFAULT);
}

void CPU_CACHE_Enable(void)
{
	SCB_EnableICache();
	SCB_EnableDCache();
}

void Error_Handler(int err)
{
	gen_boot_reason_err = err;
}

ulong is_firmware_valid(void)
{
	//return 5;

	#if 0
	// Is there valid jump to signature block (always const)
	if(*(ulong *)(RADIO_FIRM_ADDR + 0x004) != (RADIO_FIRM_ADDR + 0x299))
	{
		return 1;
	}

	// Signature1 valid ?
	if(*(ulong *)(RADIO_FIRM_ADDR + 0x2A0) != 0x77777777)
	{
		return 2;
	}

	// Signature2 valid ?
	if(*(ulong *)(RADIO_FIRM_ADDR + 0x2A4) != 0x88888888)
	{
		return 2;
	}
	#else
	if(*(ulong *)(RADIO_FIRM_ADDR + 0x004) == 0xFFFFFFFF)
	{
		return 1;
	}
	#endif

	// ToDo: test CRC
	// ...

	return 0;
}

static void early_backup_domain_init(void)
{
	/* Enable Back up SRAM */
	/* Enable write access to Backup domain */
	PWR->CR1 |= PWR_CR1_DBP;
	while((PWR->CR1 & PWR_CR1_DBP) == RESET)
	{
		__asm("nop");
	}

	// Enable BKPRAM clock
	__HAL_RCC_BKPRAM_CLK_ENABLE();
}

static void bsp_config(void)
{
	// Use sharing, as DSP core might be running after reset
	printf_init(1);
	printf("\r\n");
	printf("%s v: %d.%d  \r\n", DEVICE_STRING, MCHF_L_VER_RELEASE, MCHF_L_VER_BUILD);

	// Initialise the screen
	hw_lcd_gpio_init();
	hw_lcd_reset();

	#ifdef CONTEXT_IPC_PROC
	ipc_proc_init();
	#endif

	// Seems to be important
	early_backup_domain_init();
}

static void jump_to_fw(uint32_t SubDemoAddress)
{
	//printf("jump\r\n");

	/* Store the address of the Sub Demo binary */
	HAL_PWR_EnableBkUpAccess();
	WRITE_REG(BKP_REG_RESET_REASON, RESET_JUMP_TO_FW);
	HAL_PWR_DisableBkUpAccess();

	/* Disable LCD */
	#ifdef USE_EM_WIN
	LCD_Off();
	#endif

	//GUI_Delay(200);		// causes re-entrance
	HAL_Delay(200);

	/* Disable the MPU */
	HAL_MPU_Disable();

	/* DeInit Storage */
	//Storage_DeInit();

	// ToDo: stop uart driver..
	//       other hw ?

	printf("Jump to radio(0x%08x)...\r\n", (int)SubDemoAddress);

	/* Disable and Invalidate I-Cache */
	SCB_DisableICache();
	SCB_InvalidateICache();

	/* Disable, Clean and Invalidate D-Cache */
	SCB_DisableDCache();
	SCB_CleanInvalidateDCache();

	HAL_Delay(50);

	//printf("reset\r\n");
	HAL_NVIC_SystemReset();
}

static int sdram_test(void)
{
	ulong *ram_ptr = (ulong *)SDRAM_DEVICE_ADDR;
	ulong temp, i;
	ulong num_err = 0;

	// Connect SDRAM
	if(BSP_SDRAM_Init(0) != BSP_ERROR_NONE)
	{
		printf("SDRAM init error!\r\n");
		return 1;
	}

	// Write data
	for(i = 0; i < SDRAM_DEVICE_SIZE/4; i++)
	{
		temp = (i << 24)|(i << 16)|(i << 8)|i;
		*ram_ptr++ = temp;
	}

	// Reset ptr
	ram_ptr = (ulong *)SDRAM_DEVICE_ADDR;

	// Compare
	for(i = 0; i < SDRAM_DEVICE_SIZE/4; i++)
	{
		//if(i < 16)
		//	printf("%08x\r\n", *ram_ptr);

		temp = (i << 24)|(i << 16)|(i << 8)|i;
		if(*ram_ptr++ != temp)
			num_err++;
	}

	// Disconnect SDRAM
	BSP_SDRAM_DeInit(0);

	if(num_err)
	{
		printf("SDRAM test res: %d (%04x-%04x)\r\n", num_err, SDRAM_DEVICE_ADDR, ram_ptr);
		return 2;
	}

	return 0;
}

#if 0
void power_off(void)
{
	GPIO_InitTypeDef  GPIO_InitStruct;

	printf("...power off\r\n");
	HAL_Delay(300);

	#ifndef REV_8_2
	// 5V OFF
	HAL_GPIO_WritePin(GPIOG, GPIO_PIN_10, GPIO_PIN_SET);

	// 8V OFF
	HAL_GPIO_WritePin(GPIOG, GPIO_PIN_9, GPIO_PIN_SET);

	// PG11 is power hold
	GPIO_InitStruct.Pin   = GPIO_PIN_11;
	GPIO_InitStruct.Mode  = GPIO_MODE_INPUT;	//GPIO_MODE_OUTPUT_PP;
	//GPIO_InitStruct.Pull  = GPIO_PULLUP;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
	HAL_GPIO_Init(GPIOG, &GPIO_InitStruct);
	//HAL_GPIO_WritePin(GPIOG, GPIO_PIN_11, 1);	// drop power
	#else
	// 5V OFF
	HAL_GPIO_WritePin(VCC_5V_ON_PORT, VCC_5V_ON, GPIO_PIN_RESET);

	// Drop power
	HAL_GPIO_WritePin(POWER_HOLD_PORT, POWER_HOLD, 0);
	#endif

	// Shouldn't be here!
	while(1);
}
#endif

#if 0
// Via standby mode
static void power_off_x(uchar reset_reason)
{
	PWREx_WakeupPinTypeDef sPinParams;

	/* Disable used wakeup source: PWR_WAKEUP_PIN4 */
	HAL_PWR_DisableWakeUpPin(PWR_WAKEUP_PIN2);

	/* Clear all related wakeup flags */
	HAL_PWREx_ClearWakeupFlag(PWR_WAKEUP_PIN_FLAGS);

	/* Enable WakeUp Pin PWR_WAKEUP_PIN4 connected to PC.13 User Button */
	sPinParams.WakeUpPin    = PWR_WAKEUP_PIN2;
	sPinParams.PinPolarity  = PWR_PIN_POLARITY_LOW;
	sPinParams.PinPull      = PWR_PIN_NO_PULL;
	HAL_PWREx_EnableWakeUpPin(&sPinParams);

	if(reset_reason != RESET_POWER_OFF)
	{
		HAL_GPIO_WritePin(POWER_LED_PORT,POWER_LED, 0);									// LED Off
		HAL_GPIO_WritePin(LCD_BL_CTRL_GPIO_PORT, LCD_BL_CTRL_PIN, GPIO_PIN_RESET);		// backlight on
	}

	/* Enter the Standby mode */
	HAL_PWR_EnterSTANDBYMode();
}
#else
static void power_off_x(uchar reset_reason)
{
	// LED off
	HAL_GPIO_WritePin(POWER_LED_PORT, POWER_LED, 0);

	// Backlight off
	HAL_GPIO_WritePin(LCD_BL_CTRL_GPIO_PORT, LCD_BL_CTRL_PIN, GPIO_PIN_RESET);

	// Regulator off
	HAL_GPIO_WritePin(POWER_HOLD_PORT, POWER_HOLD, 0);

	// Stall
	while(1);
}
#endif

static void draw_atlas_circle(ushort c2x,ushort c2y, uchar dir)
{
	short factorX = -12;
	short factorY = -12;

	if(dir)
	{
		//factorX = 14;
		factorY = 12;
	}

	lcd_low_DrawCircle(c2x + factorX, 	c2y + factorY, 		17, 0x403079B0);
	lcd_low_DrawCircle(c2x + factorX, 	c2y + factorY, 		18, 0x403079B0);
	lcd_low_FillCircle(c2x + factorX/2, c2y + factorY/2, 	16, lcd_low_COLOR_BLACK);
	lcd_low_DrawCircle(c2x + factorX/2, c2y + factorY/2, 	17, 0x803b0bc0);
	lcd_low_DrawCircle(c2x + factorX/2, c2y + factorY/2, 	18, 0x803b0bc0);
	lcd_low_FillCircle(c2x, 			c2y, 				16, lcd_low_COLOR_BLACK);
	lcd_low_DrawCircle(c2x, 			c2y, 				17, 0xff3c8bc7);
	lcd_low_DrawCircle(c2x, 			c2y, 				18, 0xff3c8bc7);
	lcd_low_FillCircle(c2x, 			c2y, 			 	 9, 0xffffce23);
}

static void draw_atlas_ui(void)
{
	uchar i, j, k;
	ulong col;

	// Draw a gradient line
	for(j = 0, k = 0; j < 6; j++)
	{
		col = lcd_low_COLOR_DARKGRAY;
		for(i = 0; i < 6; i++)
		{
			if(i)
				lcd_low_FillRect(460, k + 45 + j*72 + + i*12, 4, 2, col);
			else
				lcd_low_FillRect(460, k + 45 + j*72 + + i*12, 6, 4, col);
			col += 0x101010;
		}

		if(j == 2) k = 20;
	}

	// Mid point vertical blue line
	lcd_low_DrawHLine(450, 265, 20, 0xff3b6a97);
	lcd_low_DrawHLine(450, 266, 20, 0xff3b6a97);
	lcd_low_DrawHLine(450, 267, 20, 0xff3b6a97);

	// Side circles
	draw_atlas_circle(430,450, 0);
	draw_atlas_circle(430, 60, 1);

	// Top text
	lcd_low_SetFont(&Font24);
	lcd_low_SetTextColor(lcd_low_COLOR_WHITE);
	lcd_low_DisplayStringAt(50, 160, (uint8_t *)"BOOTLOADER", LEFT_MODE);
}

void bare_lcd_init(void)
{
	if(BSP_LCD_Init(0, LCD_ORIENTATION_PORTRAIT) != BSP_ERROR_NONE)
	{
		printf("== lcd init error ==\r\n");
		return;
	}

	lcd_low_SetFuncDriver(&LCD_Driver);
	lcd_low_SetLayer(0);

	BSP_LCD_GetXSize(0, &LCD_X_Size);
	BSP_LCD_GetYSize(0, &LCD_Y_Size);

	HAL_LTDC_ProgramLineEvent(&hltdc, 0);
	HAL_GPIO_WritePin(LCD_BL_CTRL_GPIO_PORT, LCD_BL_CTRL_PIN, GPIO_PIN_SET); 			// backlight on

	lcd_low_Clear(lcd_low_COLOR_BLACK);

	//draw_atlas_ui();

	// Right side info bar
	//lcd_low_DrawRect(1, 345, 479, 134, lcd_low_COLOR_WHITE);							// rect outline
	//
	//lcd_low_SetBackColor(lcd_low_COLOR_WHITE);
	//lcd_low_SetTextColor(lcd_low_COLOR_BLACK);
	//lcd_low_DisplayStringAt(LINE(0) + 1, 335, (uint8_t *)"boot version", LEFT_MODE);	// label
	//lcd_low_DisplayStringAt(LINE(2) + 1, 335, (uint8_t *)"coop version", LEFT_MODE);	// label
}

#if 0
static int boot_dsp_core(ulong *checksum)
{
	int32_t timeout = 0xFFFF;
	int 	i, res = 0;
	ulong  chk;

	// Checksum of header
	for(i = 0, chk = 0; i < sizeof(dsp_idle); i++)
	{
		chk += dsp_idle[i];
	}

	if(checksum)
		*checksum = chk;

	// Copy CM4 code to D2_SRAM memory
	memcpy((void *)D2_AXISRAM_BASE, dsp_idle, sizeof(dsp_idle));

	// Remap M4 core boot address (overwrites fuses)
	HAL_RCCEx_EnableBootCore(RCC_BOOT_C2);

	// Wait until CPU2 boots and enters in stop mode or timeout
	while((__HAL_RCC_GET_FLAG(RCC_FLAG_D2CKRDY) != RESET) && (timeout-- > 0))
	{
		__asm("nop");
	}

	if(timeout < 0)
		res = 1;

	// Markup in memory blank DSP Firmware
	WRITE_REG(BKP_REG_DSP_ID, DSP_IDLE);

	return res;
}
#endif

static int test_sd_card(void)
{
	GPIO_InitTypeDef  GPIO_InitStruct;

	// SD_DET - PC2 after 0.8.3 mod
	GPIO_InitStruct.Pin   = SD_DET;
	GPIO_InitStruct.Mode  = GPIO_MODE_INPUT;
	GPIO_InitStruct.Pull  = GPIO_PULLUP;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
	HAL_GPIO_Init(SD_DET_PORT, &GPIO_InitStruct);

	if(HAL_GPIO_ReadPin(SD_DET_PORT, SD_DET))
	{
		//printf("sd card na\r\n");
		return 1;
	}
	//printf("sd card in\r\n");

	// SD_PWR_CNTR - PB13 after 0.8.3 mod
	// Note: IO15 and IO33 on ESP32 can interfere
	//       with this line(with current mod), so
	//       make sure chip is on and those are inputs
	GPIO_InitStruct.Pin   = SD_PWR_CNTR;
	GPIO_InitStruct.Mode  = GPIO_MODE_OUTPUT_PP;
	GPIO_InitStruct.Pull  = GPIO_PULLUP;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
	HAL_GPIO_Init(SD_PWR_CNTR_PORT, &GPIO_InitStruct);

	// Power on
	HAL_GPIO_WritePin(SD_PWR_CNTR_PORT, SD_PWR_CNTR, GPIO_PIN_RESET);

	// Init low level driver
	if(BSP_SD_Init(0) != 0)
	{
		//printf("sd low level driver init err!\r\n");
		return 2;
	}

	//printf("sd block size: %d\r\n", (int)hsd_sdmmc->SdCard.BlockSize);
	//printf("sd num blocks: %d\r\n", (int)hsd_sdmmc->SdCard.BlockNbr);
	//printf("sd card size : %d\r\n", (hsd_sdmmc->SdCard.BlockNbr*hsd_sdmmc->SdCard.BlockNbr)/1024);

	uchar boot[512];

	// Read boot sector
	if(BSP_SD_ReadBlocks(0, (ulong *)boot, 0, 1) != 0)
	{
		//printf("sd unable to read boot sector!\r\n");
		return 3;
	}
	//print_hex_array((uchar *)(&boot[0] + 512 - 32), 32);

	// Check signature
	if((boot[510] != 0x55) || (boot[511] != 0xAA))
	{
		//printf("sd bad boot sector signature!\r\n");
		return 4;
	}

	// Init FatFS
	if(FATFS_LinkDriver(&SD_Driver, SDPath) != 0)
	{
		//printf("sd unable to init FS!\r\n");
		return 5;
	}

	if(f_mount(&SDFatFs, (TCHAR const*)SDPath, 0) != FR_OK)
	{
		//printf("sd unable to mount FS!\r\n");
		return 6;
	}

	#if 0
	FRESULT res;
	uint32_t bytesread;
	uint8_t rtext[100];

	/* Open the text file object with read access */
	if(f_open(&MyFile, "STM32.TXT", FA_READ) == FR_OK)
	{
		memset(rtext, 0, sizeof(rtext));
		res = f_read(&MyFile, rtext, sizeof(rtext), (void *)&bytesread);

		if((bytesread > 0) && (res == FR_OK))
		{
			//printf((char *)rtext);
			f_close(&MyFile);
		}
		//else
		//	printf("error read file!\r\n");
	}
	//else
	//	printf("error open file!\r\n");
	#endif

	return 0;
}

#if 0
static uchar load_default_dsp_core(uchar is_default)
{
	FRESULT 	res  = 0;
	uint32_t	read = 0;
	ulong 		curr_addr = 0;
	char		dsp_name[32];
	ulong 		curr_id;

	if(is_default)
		curr_id = DSP_LOADED_CLINT;
	else
		curr_id = READ_REG(BKP_REG_DSP_ID);

	switch(curr_id)
	{
		case DSP_LOADED_CLINT:
			strcpy(dsp_name, "clint.bin");
			//curr_id = 0;
			break;

		case DSP_LOADED_UHSDR:
			strcpy(dsp_name, "uhsdr.bin");
			//curr_id = 0;
			break;

		default:
			strcpy(dsp_name, "clint.bin");
			break;
	}
	printf("will use file: %s\r\n", dsp_name);

	// Open the file
	if(f_open(&MyFile, dsp_name, FA_READ) != FR_OK)
	{
		printf("error open file!\r\n");
		return 1;
	}

#if 1
	uchar *temp = malloc(512);
	if(temp == NULL)
	{
		printf("error alloc temp block!\r\n");
		res = 2;
		goto dsp_upd_clean_up;
	}

	// Get size
	ulong fs = f_size(&MyFile);

	fs -= 512;
	ulong cnt = fs/512;
	ulong lef = fs%512;

	if((cnt == 0) && (lef > 0))
		goto last_chunk;

	while(cnt)
	{
		// Next
		if(f_read(&MyFile, temp, 512, (void *)&read) != FR_OK)
		{
			printf("error chunk read!\r\n");
			res = 3;
			goto dsp_upd_clean_up;
		}

		if(read != 512)
		{
			printf("error first chunk size!\r\n");
			res = 4;
			goto dsp_upd_clean_up;
		}

		// Copy first chunk
		memcpy((void *)(D2_AXISRAM_BASE + curr_addr), (void *)temp, 512);
		curr_addr += 512;
		cnt--;
	}

last_chunk:

	if(lef == 0)
		goto dsp_upd_clean_up;

	// Last
	if(f_read(&MyFile, temp, lef, (void *)&read) != FR_OK)
	{
		printf("error chunk read!\r\n");
		res = 5;
		goto dsp_upd_clean_up;
	}

	if(lef != read)
	{
		printf("error first chunk size!\r\n");
		res = 6;
		goto dsp_upd_clean_up;
	}

	// Copy first chunk
	memcpy((void *)(D2_AXISRAM_BASE + curr_addr), (void *)temp, lef);

	// ToDo: Checksum test
	//..

    #else
	if(curr_id == 0)
	{
		// 0x081D0000, 0x30000
		if(hw_flash_program_file(&MyFile, 0x081D0000) != 0)
		{
			printf("error flashing dsp file!\r\n");
		}
	}
	else
	{
		// 0x081A0000, 0x30000
		if(hw_flash_program_file(&MyFile, 0x081A0000) != 0)
		{
			printf("error flashing dsp file!\r\n");
		}
	}
    #endif

	printf("write dsp id: %d\r\n", (int)curr_id);

	// Markup in memory that is valid
	WRITE_REG(BKP_REG_DSP_ID, curr_id);

dsp_upd_clean_up:
	free(temp);
	f_close(&MyFile);
	return res;
}
#endif

#if 0
HAL_StatusTypeDef HAL_CRCEx_Polynomial_Set(CRC_HandleTypeDef *hcrc, uint32_t Pol, uint32_t PolyLength)
{
	return HAL_OK;
}
#endif


// Critical HW init on start
//
// - missing pullups or pulldowns ? Well, deal with it here
//
static void critical_hw_init_and_run_fw(void)
{
	GPIO_InitTypeDef  GPIO_InitStruct;

	// Enable RTC back-up registers access
	__HAL_RCC_RTC_ENABLE();
	__HAL_RCC_RTC_CLK_ENABLE();
	HAL_PWR_EnableBkUpAccess();

    reset_reason = READ_REG(BKP_REG_RESET_REASON);
	WRITE_REG(BKP_REG_RESET_REASON, RESET_CLEAR);

	// PG11 is power hold
	GPIO_InitStruct.Pin   = POWER_HOLD;
	GPIO_InitStruct.Mode  = GPIO_MODE_OUTPUT_PP;
	GPIO_InitStruct.Pull  = GPIO_PULLDOWN;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
	HAL_GPIO_Init(POWER_HOLD_PORT, &GPIO_InitStruct);

	// Power off request from firmware
	if(reset_reason == RESET_POWER_OFF)
	{
		printf("power off request from radio...\r\n");
		HAL_Delay(300);
		power_off_x(RESET_POWER_OFF);
	}

	// PF6 is LED - ack boot up, in firmware should be ambient sensor, not GPIO!
	GPIO_InitStruct.Pin   = POWER_LED;
	HAL_GPIO_Init(POWER_LED_PORT, &GPIO_InitStruct);

	// Hold power
	HAL_GPIO_WritePin(POWER_LED_PORT,POWER_LED, 1);		// led on
	HAL_GPIO_WritePin(POWER_HOLD_PORT,POWER_HOLD, 1);	// hold power, high

	// Change DSP code
	#if 0
	if(reset_reason == RESET_DSP_RELOAD)
	{
		printf("dsp reload request from radio...\r\n");

		if(test_sd_card() == 0)
		{
			printf("sd card detected\r\n");
			if(load_default_dsp_core(0) == 0)
			{
				printf("dsp core loaded to D2 RAM\r\n");

				// Reinitialize the Stack pointer
				__set_MSP(*(__IO uint32_t*) RADIO_FIRM_ADDR);

				// Jump to application address
				((pFunc) (*(__IO uint32_t*) (RADIO_FIRM_ADDR + 4)))();
			}
		}
	}
	#endif

	// 5V control via logic high
	//  - using isolation MOSFET to prevent idle leak battery->regulator inhibit->CPU gpio
	GPIO_InitStruct.Pin   = VCC_5V_ON;
	HAL_GPIO_Init(VCC_5V_ON_PORT, &GPIO_InitStruct);
	HAL_GPIO_WritePin(VCC_5V_ON_PORT, VCC_5V_ON, 1);

	// Disable charger
	#if 0
	GPIO_InitStruct.Pin   = CHGR_ON;
	HAL_GPIO_Init(CHGR_ON_PORT, &GPIO_InitStruct);
	//
	GPIO_InitStruct.Pin   = CC_CV;
	HAL_GPIO_Init(CC_CV_PORT, &GPIO_InitStruct);
	//
	HAL_GPIO_WritePin(CHGR_ON_PORT, CHGR_ON, 0);
	HAL_GPIO_WritePin(CC_CV_PORT,   CC_CV,   0);
	#endif

	// Test only
	#if 0
	// Power is PI11
	__HAL_RCC_GPIOI_CLK_ENABLE();
	GPIO_InitStruct.Pin   = ESP_POWER;
	GPIO_InitStruct.Mode  = GPIO_MODE_OUTPUT_PP;
	HAL_GPIO_Init(ESP_POWER_PORT, &GPIO_InitStruct);

	// Power on
	HAL_GPIO_WritePin(ESP_POWER_PORT, ESP_POWER, GPIO_PIN_RESET);
	#endif

	//HAL_PWR_DisableBkUpAccess();

	if(reset_reason == RESET_JUMP_TO_FW)
	{
		// Reinitialize the Stack pointer
		__set_MSP(*(__IO uint32_t*) RADIO_FIRM_ADDR);

		// Jump to application address
		((pFunc) (*(__IO uint32_t*) (RADIO_FIRM_ADDR + 4)))();
	}
}

static uchar update_radio(void)
{
	uchar res = 0;

	__HAL_RCC_CRC_CLK_ENABLE();

	CrcHandle.Instance = CRC;
	CrcHandle.Init.DefaultPolynomialUse    = DEFAULT_POLYNOMIAL_DISABLE;
	CrcHandle.Init.DefaultInitValueUse     = DEFAULT_INIT_VALUE_ENABLE;
	CrcHandle.Init.InputDataInversionMode  = CRC_INPUTDATA_INVERSION_NONE;
	CrcHandle.Init.OutputDataInversionMode = CRC_OUTPUTDATA_INVERSION_DISABLE;
	CrcHandle.InputDataFormat              = CRC_INPUTDATA_FORMAT_WORDS;

	// Init CRC unit
	if(HAL_CRC_Init(&CrcHandle) != HAL_OK)
	{
		//printf("error crc unit!\r\n");
		return 1;
	}

	// Open flash file from SD card
	if(f_open(&MyFile, "radio.bin", FA_READ) != FR_OK)
	{
		//printf("error open file!\r\n");
		return 2;
	}

	ulong fs = f_size(&MyFile);
	//printf("file size: %d bytes\r\n", (int)fs);

	// Remove checksum
	fs -= 4;

	if(f_lseek(&MyFile, fs) != FR_OK)
	{
		//printf("error chksum location!\r\n");
		res = 3;
		goto fw_upd_clean_up;
	}

	ulong chk  = 0;
	ulong read = 0;

	if(f_read(&MyFile, &chk, 4, (void *)&read) != FR_OK)
	{
		//printf("error chksum read!\r\n");
		res = 4;
		goto fw_upd_clean_up;
	}

	if(read != 4)
	{
		//printf("error chksum size!\r\n");
		res = 5;
		goto fw_upd_clean_up;
	}
	//printf("file crc: 0x%x\r\n", chk);

	// Roll back
	if(f_lseek(&MyFile, 0) != FR_OK)
	{
		//printf("error rollback!\r\n");
		res = 6;
		goto fw_upd_clean_up;
	}

	ulong blocks = fs/512;
	ulong leftov = fs%512;
	ulong calc_crc = 0;

	//printf("chunks count: %d\r\n", blocks);
	//printf("extra bytes: %d\r\n", leftov);

	uchar *temp = malloc(512);
	if(temp == NULL)
	{
		//printf("error alloc temp block!\r\n");
		res = 7;
		goto fw_upd_clean_up;
	}

	// First
	if(f_read(&MyFile, temp, 512, (void *)&read) != FR_OK)
	{
		//printf("error chunk read!\r\n");
		free(temp);
		res = 8;
		goto fw_upd_clean_up;
	}

	if(read != 512)
	{
		//printf("error first chunk size!\r\n");
		free(temp);
		res = 9;
		goto fw_upd_clean_up;
	}

	calc_crc = HAL_CRC_Calculate(&CrcHandle, (uint32_t*)temp, 512/4);
	blocks--;

	// All chunks
	for(ulong i = 0; i < blocks; i++)
	{
		if(f_read(&MyFile, temp, 512, (void *)&read) != FR_OK)
		{
			//printf("error chunk read!\r\n");
			free(temp);
			res = 10;
			goto fw_upd_clean_up;
		}

		if(read != 512)
		{
			//printf("error next(%d) chunk size!\r\n", i);
			free(temp);
			res = 11;
			goto fw_upd_clean_up;
		}

		calc_crc = HAL_CRC_Accumulate(&CrcHandle, (uint32_t *)temp, 512/4);
	}

	// Leftovers
	if(leftov)
	{
		if(f_read(&MyFile, temp, leftov, (void *)&read) != FR_OK)
		{
			//printf("error leftover read!\r\n");
			free(temp);
			res = 12;
			goto fw_upd_clean_up;
		}

		if(read != leftov)
		{
			//printf("error last chunk size!\r\n");
			free(temp);
			res = 13;
			goto fw_upd_clean_up;
		}

		calc_crc = HAL_CRC_Accumulate(&CrcHandle, (uint32_t *)temp, leftov/4);
	}
	free(temp);
	//printf("calc checksum: 0x%x\r\n", calc_crc);

	// Test CRC
	if(chk != calc_crc)
	{
		//printf("crc mismatch!\r\n");
		res = 14;
		goto fw_upd_clean_up;
	}

	// Flash the file
	if(hw_flash_program_file(&MyFile, RADIO_FIRM_ADDR) != 0)
	{
		//printf("error writing file!\r\n");
		res = 15;
	}

fw_upd_clean_up:
	f_close(&MyFile);
	return res;
}

static void fs_cleanup(void)
{
	// Clean up after file operation (close FS, de-init SD driver, card power off, etc..)
	f_mount(NULL, (TCHAR const*)SDPath, 0);
	FATFS_UnLinkDriverEx(SDPath, 0);
	BSP_SD_DeInit(0);
	HAL_GPIO_WritePin(SD_PWR_CNTR_PORT, SD_PWR_CNTR, GPIO_PIN_SET);
}

void gpio_clocks_on(void)
{
	__HAL_RCC_GPIOA_CLK_ENABLE();
	__HAL_RCC_GPIOB_CLK_ENABLE();
	__HAL_RCC_GPIOC_CLK_ENABLE();
	__HAL_RCC_GPIOD_CLK_ENABLE();
	__HAL_RCC_GPIOE_CLK_ENABLE();
	__HAL_RCC_GPIOF_CLK_ENABLE();
	__HAL_RCC_GPIOG_CLK_ENABLE();
	__HAL_RCC_GPIOH_CLK_ENABLE();
	__HAL_RCC_GPIOI_CLK_ENABLE();
}

void boot_process(void)
{
	char 	buff[200];
	int 	line = 10;

	// Init LCD
    bare_lcd_init();

    // Text attributes
	lcd_low_SetBackColor(lcd_low_COLOR_BLACK);
	lcd_low_SetTextColor(lcd_low_COLOR_WHITE);
	lcd_low_SetFont(&Font16);

	// -----------------------------------------------------------------------------------------------
	// -----------------------------------------------------------------------------------------------
	sprintf(buff, "%s", DEVICE_STRING);
//!	lcd_low_DisplayStringAt(LINE(line), 10, (uchar *)buff, LEFT_MODE);
	line += 2;
	sprintf(buff, "%d.%d.%d.%d", MCHF_L_VER_MAJOR, MCHF_L_VER_MINOR, MCHF_L_VER_RELEASE, MCHF_L_VER_BUILD);
//|!	lcd_low_DisplayStringAt(LINE(1) + 1, 350, (uint8_t *)buff, LEFT_MODE);	// label

	// -----------------------------------------------------------------------------------------------
	// -----------------------------------------------------------------------------------------------
	// Test for general boot error (clocks, lcd, etc)
//!	lcd_low_DisplayStringAt(LINE(line), 10, (uchar *)"Testing BOOT err...", LEFT_MODE);

	if(gen_boot_reason_err == 0)
		lcd_low_DisplayStringAt(LINE(line), 40, (uchar *)"Testing BOOT err...PASS", LEFT_MODE);
	else
	{
		sprintf(buff, "Update Firmware....FAIL(%d)", gen_boot_reason_err);
		lcd_low_DisplayStringAt(LINE(line), 40, (uchar *)buff, LEFT_MODE);
	}
	line++;

	// -----------------------------------------------------------------------------------------------
	// -----------------------------------------------------------------------------------------------
	// Test if ESP32 alive
	#ifdef CONTEXT_IPC_PROC
	lcd_low_DisplayStringAt(LINE(line), 10, (uchar *)"Testing Baseband...", LEFT_MODE);

	if(ipc_proc_establish_link(buff) == 0)
	{
		lcd_low_DisplayStringAt(LINE(line), 10, (uchar *)"Testing Baseband...PASS", LEFT_MODE);
		line++;

		lcd_low_DisplayStringAt(LINE(3) + 1, 350, (uint8_t *)buff, LEFT_MODE);
	}
	else
	{
		lcd_low_DisplayStringAt(LINE(line), 10, (uchar *)"Testing Baseband...FAIL", LEFT_MODE);
		line++;
	}
	#endif

#if 0
	// -----------------------------------------------------------------------------------------------
	// -----------------------------------------------------------------------------------------------
	// DSP test(already done on reset, so just showing result)
	lcd_low_DisplayStringAt(LINE(line), 10, (uchar *)"Testing DSP core...", LEFT_MODE);

	if(dsp_core_stat != 0)
		lcd_low_DisplayStringAt(LINE(line), 10, (uchar *)"Testing DSP core...FAIL", LEFT_MODE);
	else
		lcd_low_DisplayStringAt(LINE(line), 10, (uchar *)"Testing DSP core...PASS", LEFT_MODE);
	line++;
#endif

	// -----------------------------------------------------------------------------------------------
	// -----------------------------------------------------------------------------------------------
	// SDRAM test
	lcd_low_DisplayStringAt(LINE(line), 40, (uchar *)"Testing SDRAM......", LEFT_MODE);

	if(sdram_test() != 0)
	{
		lcd_low_DisplayStringAt(LINE(line), 40, (uchar *)"Testing SDRAM......FAIL.", LEFT_MODE);
	}
	else
	{
		lcd_low_DisplayStringAt(LINE(line), 40, (uchar *)"Testing SDRAM......PASS", LEFT_MODE);
	}
	line++;

	// -----------------------------------------------------------------------------------------------
	// -----------------------------------------------------------------------------------------------
	// SD Card test
	lcd_low_DisplayStringAt(LINE(line), 40, (uchar *)"Testing SD Card....", LEFT_MODE);

	if(test_sd_card() == 0)
	{
		lcd_low_DisplayStringAt(LINE(line), 40, (uchar *)"Testing SD Card....PASS", LEFT_MODE);
		line++;

		if(reset_reason == RESET_UPDATE_FW)
		{
			lcd_low_DisplayStringAt(LINE(line), 40, (uchar *)"Update Firmware....", LEFT_MODE);

			uchar res = update_radio();
			if(res == 0)
				lcd_low_DisplayStringAt(LINE(line), 40, (uchar *)"Update Firmware....PASS", LEFT_MODE);
			else
			{
				sprintf(buff, "Update Firmware....FAIL(%d)", res);
				lcd_low_DisplayStringAt(LINE(line), 40, (uchar *)buff, LEFT_MODE);
			}
			line++;
		}

		#if 0
		lcd_low_DisplayStringAt(LINE(line), 40, (uchar *)"Upload DSP Core....", LEFT_MODE);
		uchar d_res = load_default_dsp_core(1);
		if(d_res == 0)
			lcd_low_DisplayStringAt(LINE(line), 40, (uchar *)"Upload DSP Core....PASS", LEFT_MODE);
		else
		{
			sprintf(buff, "Upload DSP Core....FAIL(%d)", d_res);
			lcd_low_DisplayStringAt(LINE(line), 40, (uchar *)buff, LEFT_MODE);
		}
		line++;
		#endif

		// Clean up anyway, do we need on fail sd test as well ?
		fs_cleanup();
	}
	else
	{
		lcd_low_DisplayStringAt(LINE(line), 40, (uchar *)"Testing SD Card....FAIL", LEFT_MODE);
		line++;
	}

	// -----------------------------------------------------------------------------------------------
	// -----------------------------------------------------------------------------------------------
	// Firmware test
	lcd_low_DisplayStringAt(LINE(line), 40, (uchar *)"Testing Firmware...", LEFT_MODE);

	if(is_firmware_valid() == 0)
	{
		lcd_low_DisplayStringAt(LINE(line), 40, (uchar *)"Testing Firmware...PASS", LEFT_MODE);
		line++;

		HAL_Delay(1000);

		// Jump
		jump_to_fw(RADIO_FIRM_ADDR);
	}

	lcd_low_DisplayStringAt(LINE(line), 40, (uchar *)"Testing Firmware...FAIL", LEFT_MODE);
	line++;

	// Test - chip blank programming
	#if 0
	HAL_PWR_EnableBkUpAccess();
	WRITE_REG(BKP_REG_RESET_REASON, RESET_UPDATE_FW);
	HAL_PWR_DisableBkUpAccess();
	NVIC_SystemReset();
	#endif

	// -----------------------------------------------------------------------------------------------
	// -----------------------------------------------------------------------------------------------
	// Stall, and power off eventually
	#if 0
	ulong pc = 0;
	while(1)
	{
		HAL_Delay(1000);
		pc++;

		// On timeout go to sleep, to prevent draining of batteries
		if(pc > 25)
		{
			lcd_low_DisplayStringAt(LINE(line), 10, (uchar *)"Testing Power Off...", LEFT_MODE);
			line++;

			printf("== will power off ==\r\n");
			power_off_x(0);
		}
	}
	#endif
}

int main(void)
{
	// Disable FMC Bank1 to avoid speculative/cache accesses
	FMC_Bank1_R->BTCR[0] &= ~FMC_BCRx_MBKEN;

	// Remap M4 core boot address (overwrites fuses)
	//----HAL_SYSCFG_CM4BootAddConfig(SYSCFG_BOOT_ADDR0, D2_AXISRAM_BASE);

	// All GPIOs ready
    gpio_clocks_on();

    // Configure the MPU attributes as Write Through
    MPU_Config();

    // Enable the CPU Cache
    CPU_CACHE_Enable();

	// Needs to be first thing (after MMU init ?)
	//dsp_core_stat = boot_dsp_core(NULL);

    // Init hal
    HAL_Init();

    // Configure the system clock to 400 MHz
    SystemClock_Config();

	// Init hw
    critical_hw_init_and_run_fw();

    //EXTI15_10_IRQHandler_Config();

    // Proc init
    bsp_config();

    // Actual bootloader sequencing
    boot_process();

    while(1)
    {
    	HAL_Delay(300);
    	HAL_GPIO_TogglePin(POWER_LED_PORT, POWER_LED);
    }
}
