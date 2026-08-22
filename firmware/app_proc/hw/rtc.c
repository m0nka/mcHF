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
#include "main.h"

#include "rtc.h"

static k_AlarmCallback AlarmCallback;
  
// The board has a 32.768 kHz crystal and SystemClock_Config already
// selects it (board.c, and it hangs in Error_Handler(9) if the LSE fails
// to start - so a booting radio proves the crystal runs). Without this
// define the MspInit below quietly switched the RTC back to the LSI
// while keeping the prescalers below, which divide by 32768: the LSI
// runs near 28 kHz, so the wall clock lost about one second in six
// (measured 110 RTC seconds against 128.9 real ones, 2026-07-21). That
// is what made the RTC look like a badly drifting crystal
#define USE_LSE

#define RTC_ASYNCH_PREDIV  0x7F   /* LSE as RTC clock */
#define RTC_SYNCH_PREDIV   0x00FF /* LSE as RTC clock */

// -----------------------------------------------------------------------------
// LSE trim - RTC smooth digital calibration (RM0433, RTC_CALR)
//
// With the LSE fix above in place the wall clock still drifts by a few ppm.
// That is the crystal and its load capacitors, not anything software does.
// The RTC trims it continuously in the backup domain: over a 32 s window it
// adds 512 pulses when CALP is set and subtracts CALM, one step being
// 1/2^20 = 0.954 ppm, range -487..+488 ppm.
//
// PER UNIT VALUE. This compensates THIS board's crystal, so it does not
// belong in a shared header. Positive = the clock runs slow and is sped up.
// To re-measure: set the clock against a reference, leave the radio running
// for several days, then
//
//   ppm = seconds_lost * 1000000 / seconds_elapsed
//
// Note the sign - a clock that GAINS needs a negative number here.
//
// The first value tried was +29 ppm, from a single 2-3 s/24 h observation
// on 2026-07-22 that read the drift the wrong way round. A 6-day run with
// that trim active (claude/rtc/time_log.txt, 23.07 -> 29.07.2026) gained
// 26 s over 514020 s, a least-squares rate of +49.9 ppm fast. Backing out
// the +28.6 ppm the trim was adding leaves the bare crystal at +21.3 ppm,
// so it runs fast and must be slowed down.
//
// -21 ppm lands on -22 steps = -20.98 ppm, leaving about +0.3 ppm, i.e.
// under a second a month. What limits the result from here is temperature,
// not this number: a 32.768 kHz tuning fork is parabolic at about
// -0.034 ppm/degC^2 either side of a +25 degC peak, so a warm chassis costs
// several ppm on its own
//
// This is only the FACTORY DEFAULT now. proc/gps/gps_calib.c measures the
// real per unit figure against GPS PPS and stores it in the backup domain,
// which wins at boot when present - see rtc_calib_ppm_load() below
#define RTC_CALIB_PPM		-21

// Backup domain home of the measured per unit trim. These survive reset and
// power off (the pack feeds the RTC), so a calibrated radio stays calibrated
// without touching the SD card. DR6 is taken by the virt_eeprom test
#define RTC_CALIB_BKP_MAGIC_REG		RTC_BKP_DR10
#define RTC_CALIB_BKP_PPM_REG		RTC_BKP_DR11
#define RTC_CALIB_BKP_MAGIC			0x43414C42		// 'CALB'

// Trim actually in force, ppm. Positive = clock runs slow and is sped up
static int32_t	rtc_calib_ppm = RTC_CALIB_PPM;

#define BUTTON_WAKEUP_PIN                   GPIO_PIN_13
RTC_HandleTypeDef RtcHandle;

#if 1
// Fix insane Date on startup
void check_date_sanity(void)
{
	RTC_DateTypeDef sdatestructureget;
	RTC_TimeTypeDef stimestructureget;

	k_GetTime(&stimestructureget);
	k_GetDate(&sdatestructureget);

	if(sdatestructureget.Year < 26)
	{
		sdatestructureget.Year = 26;		// Can't be travelling back in time, can we ?
	}
	else if(sdatestructureget.Year > 31)
		sdatestructureget.Year = 31;		// Am i still alive to update this code, yay! Open the JD!

	k_SetDate(&sdatestructureget);
}
#endif

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

	// Apply the LSE trim, measured value first. Safe to re-run on every boot
	// - CALR lives in the backup domain and this only rewrites it
	rtc_calib_ppm_apply(rtc_calib_ppm_load());

	// Fix crazy dates
	check_date_sanity();
}

//*----------------------------------------------------------------------------
//* Function Name       : rtc_calib_ppm_apply
//* Object              : write a ppm trim into RTC_CALR. Positive ppm speeds
//*						: the clock up (it was running slow). 0 = applied
//* Notes    			: no calendar disturbance, so this is safe to call at
//*						: any time - including from a running calibration
//*----------------------------------------------------------------------------
int rtc_calib_ppm_apply(int32_t ppm)
{
	int32_t		steps;
	uint32_t	plus, minus;

	// One step is 1/2^20 = 0.954 ppm. Round to nearest, away from zero
	steps = (ppm * 1048576 + ((ppm >= 0) ? 500000 : -500000)) / 1000000;

	// CALM is 9 bits, and the only way to a net gain is to add the
	// full 512 and take the difference back off again
	if(steps > 512)
		steps = 512;
	else if(steps < -511)
		steps = -511;

	if(steps > 0)
	{
		plus  = RTC_SMOOTHCALIB_PLUSPULSES_SET;
		minus = (uint32_t)(512 - steps);
	}
	else
	{
		plus  = RTC_SMOOTHCALIB_PLUSPULSES_RESET;
		minus = (uint32_t)(-steps);
	}

	if(HAL_RTCEx_SetSmoothCalib(&RtcHandle, RTC_SMOOTHCALIB_PERIOD_32SEC, plus, minus) != HAL_OK)
	{
		printf("rtc: lse trim failed\r\n");
		return 1;
	}

	rtc_calib_ppm = ppm;

	printf("rtc: lse trim %d ppm (calp %d, calm %d)\r\n",
			(int)ppm, (plus != 0) ? 1 : 0, (int)minus);

	return 0;
}

//*----------------------------------------------------------------------------
//* Function Name       : rtc_calib_ppm_get
//* Object              : trim currently in force, ppm. A measurement made
//*						: while this is applied is a RESIDUAL - the new trim
//*						: is (this - residual), see gps_calib.c
//*----------------------------------------------------------------------------
int32_t rtc_calib_ppm_get(void)
{
	return rtc_calib_ppm;
}

//*----------------------------------------------------------------------------
//* Function Name       : rtc_calib_ppm_load
//* Object              : measured per unit trim from the backup domain, or
//*						: the compiled in factory default when never measured
//*----------------------------------------------------------------------------
int32_t rtc_calib_ppm_load(void)
{
	if(k_BkupRestoreParameter(RTC_CALIB_BKP_MAGIC_REG) != RTC_CALIB_BKP_MAGIC)
		return RTC_CALIB_PPM;

	return (int32_t)k_BkupRestoreParameter(RTC_CALIB_BKP_PPM_REG);
}

//*----------------------------------------------------------------------------
//* Function Name       : rtc_calib_ppm_save
//* Object              : apply a measured trim and make it survive power off
//*----------------------------------------------------------------------------
int rtc_calib_ppm_save(int32_t ppm)
{
	// Well outside the CALR range means the caller computed nonsense - a
	// bad trim is worse than none, so refuse rather than clamp silently
	if((ppm < -400) || (ppm > 400))
	{
		printf("rtc: refusing insane trim %d ppm\r\n", (int)ppm);
		return 1;
	}

	if(rtc_calib_ppm_apply(ppm) != 0)
		return 1;

	k_BkupSaveParameter(RTC_CALIB_BKP_PPM_REG,   (uint32_t)ppm);
	k_BkupSaveParameter(RTC_CALIB_BKP_MAGIC_REG, RTC_CALIB_BKP_MAGIC);

	printf("rtc: trim %d ppm stored\r\n", (int)ppm);

	return 0;
}

//*----------------------------------------------------------------------------
//* Function Name       : rtc_calib_ppm_clear
//* Object              : forget the measured trim, back to the factory default
//*----------------------------------------------------------------------------
void rtc_calib_ppm_clear(void)
{
	k_BkupSaveParameter(RTC_CALIB_BKP_MAGIC_REG, 0);
	rtc_calib_ppm_apply(RTC_CALIB_PPM);
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

	#ifdef USE_LSE
  	// Configure LSE as RTC clock source
  	RCC_OscInitStruct.OscillatorType 	= RCC_OSCILLATORTYPE_LSI | RCC_OSCILLATORTYPE_LSE;
  	RCC_OscInitStruct.PLL.PLLState 		= RCC_PLL_NONE;
  	RCC_OscInitStruct.LSEState 			= RCC_LSE_ON;
  	RCC_OscInitStruct.LSIState 			= RCC_LSI_OFF;
  	if(HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  	{
  		Error_Handler(520);
  		return;
  	}
  
  	PeriphClkInitStruct.PeriphClockSelection 	= RCC_PERIPHCLK_RTC;
  	PeriphClkInitStruct.RTCClockSelection 		= RCC_RTCCLKSOURCE_LSE;
  	if(HAL_RCCEx_PeriphCLKConfig(&PeriphClkInitStruct) != HAL_OK)
  	{
  		Error_Handler(521);
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
