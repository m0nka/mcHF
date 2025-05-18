/************************************************************************************
**                                                                                 **
**                             mcHF Pro QRP Transceiver                            **
**                         Krassi Atanassov - M0NKA, 2013-2025                     **
**                                                                                 **
**---------------------------------------------------------------------------------**
**                                                                                 **
**  File name:                                                                     **
**  Description:                                                                   **
**  Last Modified:                                                                 **
**  Licence:               GNU GPLv3                                               **
************************************************************************************/
#include "main.h"

#include "rtc.h"

static k_AlarmCallback AlarmCallback;
  
#define RTC_ASYNCH_PREDIV  0x7F   /* LSE as RTC clock */
#define RTC_SYNCH_PREDIV   0x00FF /* LSE as RTC clock */

#define BUTTON_WAKEUP_PIN                   GPIO_PIN_13
RTC_HandleTypeDef RtcHandle;

// Fix insane Date on startup
void check_date_sanity(void)
{
	RTC_DateTypeDef sdatestructureget;
	RTC_TimeTypeDef stimestructureget;

	k_GetTime(&stimestructureget);
	k_GetDate(&sdatestructureget);

	if(sdatestructureget.Year < 25)
	{
		sdatestructureget.Year = 25;		// Can't be travelling back in time, can we ?
	}
	else if(sdatestructureget.Year > 31)
		sdatestructureget.Year = 31;		// Am i still alive to update this code, yay! Open the JD!

	k_SetDate(&sdatestructureget);
}

void k_CalendarBkupInit(void)
{
	// Remove PC13 from RTC domain
	//wake_irq_setup();

	/*##-1- Configure the RTC peripheral #######################################*/
	/* Configure RTC prescaler and RTC data registers */
	/* RTC configured as follow:
  	  - Hour Format    = Format 24
  	  - Asynch Prediv  = Value according to source clock
  	  - Synch Prediv   = Value according to source clock
  	  - OutPut         = Output Disable
  	  - OutPutPolarity = High Polarity
  	  - OutPutType     = Open Drain */
	RtcHandle.Instance 				= RTC;
	RtcHandle.Init.HourFormat 		= RTC_HOURFORMAT_24;
	RtcHandle.Init.AsynchPrediv 	= RTC_ASYNCH_PREDIV;
	RtcHandle.Init.SynchPrediv 		= RTC_SYNCH_PREDIV;
	RtcHandle.Init.OutPut 			= RTC_OUTPUT_DISABLE;
	RtcHandle.Init.OutPutPolarity 	= RTC_OUTPUT_POLARITY_HIGH;
	RtcHandle.Init.OutPutType 		= RTC_OUTPUT_TYPE_OPENDRAIN;

	#if 0
	// Disable the write protection for RTC registers
	__HAL_RTC_WRITEPROTECTION_DISABLE	(&RtcHandle);

	// Reference manual, page 2081
	RtcHandle.Instance->CR		&= ~RTC_CR_OSEL_0;
	RtcHandle.Instance->CR 		&= ~RTC_CR_OSEL_1;
	RtcHandle.Instance->CR 		&= ~(RTC_CR_COE);
	RtcHandle.Instance->TAMPCR  &= ~(RTC_TAMPCR_TAMP1E);
	RtcHandle.Instance->CR 	    &= ~(RTC_CR_TSE);

	// Enable the write protection for RTC registers
	__HAL_RTC_WRITEPROTECTION_ENABLE(&RtcHandle);
	#endif

	if(HAL_RTC_Init(&RtcHandle) != HAL_OK)
	{
		return;
	}

	// Fix crazy dates
//!	check_date_sanity();
}

/**
  * @brief RTC MSP Initialization 
  *        This function configures the hardware resources used in this example: 
  *           - Peripheral's clock enable
  * @param  hrtc: RTC handle pointer
  * @retval None
  */
// normal
void HAL_RTC_MspInit(RTC_HandleTypeDef *hrtc)
{
	RCC_OscInitTypeDef        RCC_OscInitStruct;
	RCC_PeriphCLKInitTypeDef  PeriphClkInitStruct;
  
	// Enables access to the backup domain
	// To enable access on RTC registers
	PWR->CR1 |= PWR_CR1_DBP;
  	while((PWR->CR1 & PWR_CR1_DBP) == RESET)
  	{
  		__asm("nop");
  	}

	#ifdef USE_LSE
  	// Configure LSE as RTC clock source
  	RCC_OscInitStruct.OscillatorType 	= RCC_OSCILLATORTYPE_LSI | RCC_OSCILLATORTYPE_LSE;
  	RCC_OscInitStruct.PLL.PLLState 		= RCC_PLL_NONE;
  	RCC_OscInitStruct.LSEState 			= RCC_LSE_ON;
  	RCC_OscInitStruct.LSIState 			= RCC_LSI_OFF;
  	if(HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  	{
  		return;
  	}
  
  	PeriphClkInitStruct.PeriphClockSelection 	= RCC_PERIPHCLK_RTC;
  	PeriphClkInitStruct.RTCClockSelection 		= RCC_RTCCLKSOURCE_LSE;
  	if(HAL_RCCEx_PeriphCLKConfig(&PeriphClkInitStruct) != HAL_OK)
  	{
  		return;
  	}
  
	// Configures the External Low Speed oscillator (LSE) drive capability
	__HAL_RCC_LSEDRIVE_CONFIG(RCC_LSEDRIVE_HIGH);
	#else
	// Configure the RTC clock source
	// -a- Enable LSI Oscillator
	RCC_OscInitStruct.OscillatorType =  RCC_OSCILLATORTYPE_LSI;
	RCC_OscInitStruct.PLL.PLLState = RCC_PLL_NONE;
	RCC_OscInitStruct.LSIState = RCC_LSI_ON;
	if(HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
	{
		return;
	}

	// -b- Select LSI as RTC clock source
	PeriphClkInitStruct.PeriphClockSelection = RCC_PERIPHCLK_RTC;
	PeriphClkInitStruct.RTCClockSelection = RCC_RTCCLKSOURCE_LSI;
	if(HAL_RCCEx_PeriphCLKConfig(&PeriphClkInitStruct) != HAL_OK)
	{
		return;
	}
	#endif

	// Enable the RTC peripheral Clock
	__HAL_RCC_RTC_ENABLE();
	__HAL_RCC_RTC_CLK_ENABLE();
	__HAL_RCC_BKPRAM_CLK_ENABLE();
}


/**
  * @brief RTC MSP De-Initialization 
  *        This function frees the hardware resources used in this example:
  *          - Disable the Peripheral's clock
  * @param  hrtc: RTC handle pointer
  * @retval None
  */
void HAL_RTC_MspDeInit(RTC_HandleTypeDef *hrtc)
{
  /*##-1- Reset peripherals ##################################################*/
   __HAL_RCC_RTC_DISABLE();
}


/**
  * @brief  Backup save parameter 
  * @param  address: RTC Backup data Register number.
  *                  This parameter can be: RTC_BKP_DRx where x can be from 0 to 19 to 
  *                                         specify the register.
  * @param  Data:    Data to be written in the specified RTC Backup data register.
  * @retval None
  */
void k_BkupSaveParameter(uint32_t address, uint32_t data)
{
  HAL_RTCEx_BKUPWrite(&RtcHandle,address,data);  
}

/**
  * @brief  Backup restore parameter. 
  * @param  address: RTC Backup data Register number.
  *                  This parameter can be: RTC_BKP_DRx where x can be from 0 to 19 to 
  *                                         specify the register. 
  * @retval None
  */
uint32_t k_BkupRestoreParameter(uint32_t address)
{
   return HAL_RTCEx_BKUPRead(&RtcHandle,address);  
}

/**
  * @brief  RTC Get time. 
  * @param  Time: Pointer to Time structure
  * @retval None
  */
void k_GetTime(RTC_TimeTypeDef *Time)
{
   HAL_RTC_GetTime(&RtcHandle, Time, RTC_FORMAT_BIN);
   RTC_DateTypeDef dummy;

   /* We need to get Date after getting Time
    * in order to unlock the updata of the RTC Calendar
    */
   HAL_RTC_GetDate(&RtcHandle, &dummy, RTC_FORMAT_BIN);
}

/**
  * @brief  RTC Set time. 
  * @param  Time: Pointer to Time structure
  * @retval None
  */
void k_SetTime(RTC_TimeTypeDef *Time)
{
   Time->StoreOperation = 0;
   Time->SubSeconds = 0;
   Time->DayLightSaving = 0;
   HAL_RTC_SetTime(&RtcHandle, Time, RTC_FORMAT_BIN);
}

/**
  * @brief  RTC Get date
  * @param  Date: Pointer to Date structure
  * @retval None
  */
void k_GetDate(  RTC_DateTypeDef *Date)
{
   HAL_RTC_GetDate(&RtcHandle, Date, RTC_FORMAT_BIN);
   
   if((Date->Date == 0) || (Date->Month == 0))
   {
     Date->Date = Date->Month = 1;
   }    

   //printf("year get: %d\r\n", Date->Year);
}

/**
  * @brief  RTC Set alarm
  * @param  Alarm: Pointer to Alarm structure
  * @retval None
  */
void k_SetAlarm(RTC_AlarmTypeDef *Alarm)
{
  HAL_RTC_SetAlarm_IT(&RtcHandle, Alarm, RTC_FORMAT_BIN);
}

/**
  * @brief  RTC Set date
  * @param  Date: Pointer to Date structure
  * @retval None
  */
void k_SetDate(RTC_DateTypeDef *Date)
{
   //printf("year set: %d\r\n", Date->Year);
   HAL_RTC_SetDate(&RtcHandle, Date, RTC_FORMAT_BIN);
}

/**
  * @brief  Alarm callback
  * @param  hrtc : RTC handle
  * @retval None
  */
void HAL_RTC_AlarmAEventCallback(RTC_HandleTypeDef *hrtc)
{
  /* Turn LED1 on: Alarm generation */
  //BSP_LED_On(LED1);

  if (AlarmCallback != NULL)
  {
    AlarmCallback();
  }
}

void k_SetAlarmCallback (k_AlarmCallback alarmCallback)
{
  AlarmCallback = alarmCallback;
}
