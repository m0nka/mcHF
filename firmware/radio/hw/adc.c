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
**  Licence:       The mcHF project is released for radio amateurs experimentation **
**               and non-commercial use only.Check 3rd party drivers for licensing **
************************************************************************************/

#include "main.h"
#include "mchf_pro_board.h"

#include "adc.h"

/* Definitions of ADC hardware constraints delays */
/* Note: Only ADC IP HW delays are defined in ADC LL driver driver,           */
/*       not timeout values:                                                  */
/*       Timeout values for ADC operations are dependent to device clock      */
/*       configuration (system clock versus ADC clock),                       */
/*       and therefore must be defined in user application.                   */
/*       Refer to @ref ADC_LL_EC_HW_DELAYS for description of ADC timeout     */
/*       values definition.                                                   */

  /* Timeout values for ADC operations. */
  /* (calibration, enable settling time, disable settling time, ...)          */
  /* Values defined to be higher than worst cases: low clock frequency,       */
  /* maximum prescalers.                                                      */
/* Timeout to wait for current conversion on going to be completed.           */
/* Timeout fixed to worst case, for 1 channel.                                */
/*   - maximum sampling time (830.5 adc_clk)                                  */
/*   - ADC resolution (Tsar 16 bits= 16.5 adc_clk)                            */
/*   - ADC clock with prescaler 256                                           */
/*     823 * 256 = 210688 clock cycles max                                    */
/* Unit: cycles of CPU clock.                                                 */
#define ADC_CALIBRATION_TIMEOUT_MS      (1320UL)
#define ADC_ENABLE_TIMEOUT_MS           (2UL)    /*!< ADC enable time-out value  */
#define ADC_DISABLE_TIMEOUT_MS          (2UL)    /*!< ADC disable time-out value */
#define ADC_STOP_CONVERSION_TIMEOUT_MS  (2UL)
#define ADC_CONVERSION_TIMEOUT_MS       (50UL)
  /* Delay between ADC end of calibration and ADC enable.                     */
  /* Delay estimation in CPU cycles: Case of ADC enable done                  */
  /* immediately after ADC calibration, ADC clock setting slow                */
  /* (LL_ADC_CLOCK_ASYNC_DIV32). Use a higher delay if ratio                  */
  /* (CPU clock / ADC clock) is above 32.                                     */
#define ADC_DELAY_CALIB_ENABLE_CPU_CYCLES  (LL_ADC_DELAY_CALIB_ENABLE_ADC_CYCLES * 32)

/* Definitions of environment analog values */
  /* Value of analog reference voltage (Vref+), connected to analog voltage   */
  /* supply Vdda (unit: mV).                                                  */
#define VDDA_APPLI                       (3300U)

/* Definitions of data related to this example */
  /* Definition of ADCx analog watchdog window thresholds */
  /* Value of ADC analog watchdog threshold high */
#define ADC_AWD_THRESHOLD_HIGH           (__LL_ADC_DIGITAL_SCALE(LL_ADC_RESOLUTION_16B) / 2)

/* Value of ADC analog watchdog threshold low  */
#define ADC_AWD_THRESHOLD_LOW            (   0U)

static void Configure_ADC(void)
{
	/* Enable GPIO Clock */
	//LL_AHB4_GRP1_EnableClock(LL_AHB4_GRP1_PERIPH_GPIOA);

	// ADC3_INP1, Reflected Power (PC3)
	LL_GPIO_SetPinMode(ADC3_INP1_PORT, ADC3_INP1, LL_GPIO_MODE_ANALOG);

	// ADC3_INP8, Ambient light sensor (PF6)
	LL_GPIO_SetPinMode(POWER_LED_PORT, POWER_LED, LL_GPIO_MODE_ANALOG);

	// ADC3_INP3, PA Temperature (PF7)
	LL_GPIO_SetPinMode(ADC3_INP3_PORT, ADC3_INP3, LL_GPIO_MODE_ANALOG);

	// ADC3_INP6, Forward Power (PF10)
	LL_GPIO_SetPinMode(ADC3_INP6_PORT, ADC3_INP6, LL_GPIO_MODE_ANALOG);

	/*## Configuration of NVIC #################################################*/
	/* Configure NVIC to enable ADC1 interruptions */
	NVIC_SetPriority(ADC_IRQn, 4);
	NVIC_EnableIRQ	(ADC_IRQn);

	/*## Configuration of ADC ##################################################*/
	/*## Configuration of ADC hierarchical scope: common to several ADC ########*/

	/* Enable ADC clock (core clock) */
	LL_AHB1_GRP1_EnableClock(LL_AHB1_GRP1_PERIPH_ADC12);

	/* Note: Hardware constraint (refer to description of the functions         */
	/*       below):                                                            */
	/*       On this STM32 series, setting of these features is conditioned to   */
	/*       ADC state:                                                         */
	/*       All ADC instances of the ADC common group must be disabled.        */
	/* Note: In this example, all these checks are not necessary but are        */
	/*       implemented anyway to show the best practice usages                */
	/*       corresponding to reference manual procedure.                       */
	/*       Software can be optimized by removing some of these checks, if     */
	/*       they are not relevant considering previous settings and actions    */
	/*       in user application.                                               */
	if(__LL_ADC_IS_ENABLED_ALL_COMMON_INSTANCE(__LL_ADC_COMMON_INSTANCE(ADC1)) == 0)
	{
		/* Note: Call of the functions below are commented because they are       */
		/*       useless in this example:                                         */
		/*       setting corresponding to default configuration from reset state. */

		/* Set ADC clock (conversion clock) common to several ADC instances */
		LL_ADC_SetCommonClock(__LL_ADC_COMMON_INSTANCE(ADC1), LL_ADC_CLOCK_SYNC_PCLK_DIV4);

		/* Set ADC measurement path to internal channels */
		// LL_ADC_SetCommonPathInternalCh(__LL_ADC_COMMON_INSTANCE(ADC1), LL_ADC_PATH_INTERNAL_NONE);


		/*## Configuration of ADC hierarchical scope: multimode ####################*/

		/* Set ADC multimode configuration */
		// LL_ADC_SetMultimode(__LL_ADC_COMMON_INSTANCE(ADC1), LL_ADC_MULTI_INDEPENDENT);

		/* Set ADC multimode DMA transfer */
		// LL_ADC_SetMultiDMATransfer(__LL_ADC_COMMON_INSTANCE(ADC1), LL_ADC_MULTI_REG_DMA_EACH_ADC);

		/* Set ADC multimode: delay between 2 sampling phases */
		// LL_ADC_SetMultiTwoSamplingDelay(__LL_ADC_COMMON_INSTANCE(ADC1), LL_ADC_MULTI_TWOSMP_DELAY_1CYCLE);
	}

	/*## Configuration of ADC hierarchical scope: ADC instance #################*/

	/* Note: Hardware constraint (refer to description of the functions         */
	/*       below):                                                            */
	/*       On this STM32 series, setting of these features is conditioned to   */
	/*       ADC state:                                                         */
	/*       ADC must be disabled.                                              */
	if (LL_ADC_IsEnabled(ADC1) == 0)
	{
		/* Note: Call of the functions below are commented because they are       */
		/*       useless in this example:                                         */
		/*       setting corresponding to default configuration from reset state. */

		/* Set ADC data resolution */
		LL_ADC_SetResolution(ADC1, LL_ADC_RESOLUTION_16B);

		/* Set ADC conversion data alignment */
		//LL_ADC_SetResolution(ADC1, LL_ADC_DATA_ALIGN_RIGHT);

		/* Set ADC low power mode */
		//LL_ADC_SetLowPowerMode(ADC1, LL_ADC_LP_MODE_NONE);

		/* Set ADC selected offset number: channel and offset level */
		// LL_ADC_SetOffset(ADC1, LL_ADC_OFFSET_1, LL_ADC_CHANNEL_15, 0x000);
	}

	/*## Configuration of ADC hierarchical scope: ADC group regular ############*/

	/* Note: Hardware constraint (refer to description of the functions         */
	/*       below):                                                            */
	/*       On this STM32 series, setting of these features is conditioned to   */
	/*       ADC state:                                                         */
	/*       ADC must be disabled or enabled without conversion on going        */
	/*       on group regular.                                                  */
	if ((LL_ADC_IsEnabled(ADC1) == 0)               ||
		(LL_ADC_REG_IsConversionOngoing(ADC1) == 0)   )
	{
		/* Set ADC group regular trigger source */
		LL_ADC_REG_SetTriggerSource(ADC1, LL_ADC_REG_TRIG_SOFTWARE);

		/* Set ADC group regular trigger polarity */
		// LL_ADC_REG_SetTriggerEdge(ADC1, LL_ADC_REG_TRIG_EXT_RISING);

		/* Set ADC group regular continuous mode */
		LL_ADC_REG_SetContinuousMode(ADC1, LL_ADC_REG_CONV_CONTINUOUS);

		/* Set ADC group regular conversion data transfer */
		// LL_ADC_REG_SetDMATransfer(ADC1, LL_ADC_REG_DMA_TRANSFER_NONE);

		/* Set ADC group regular overrun behavior */
		LL_ADC_REG_SetOverrun(ADC1, LL_ADC_REG_OVR_DATA_OVERWRITTEN);

		/* Set ADC group regular sequencer */
		/* Note: On this STM32 series, ADC group regular sequencer is              */
		/*       fully configurable: sequencer length and each rank               */
		/*       affectation to a channel are configurable.                       */
		/*       Refer to description of function                                 */
		/*       "LL_ADC_REG_SetSequencerLength()".                               */

		/* Preselect ADC1 channel 15 */
//!    LL_ADC_SetChannelPreSelection(ADC1, LL_ADC_CHANNEL_15);
		LL_ADC_SetChannelSingleDiff(ADC1, LL_ADC_CHANNEL_1, LL_ADC_SINGLE_ENDED);
		LL_ADC_SetChannelSingleDiff(ADC1, LL_ADC_CHANNEL_3, LL_ADC_SINGLE_ENDED);
		LL_ADC_SetChannelSingleDiff(ADC1, LL_ADC_CHANNEL_6, LL_ADC_SINGLE_ENDED);
		LL_ADC_SetChannelSingleDiff(ADC1, LL_ADC_CHANNEL_8, LL_ADC_SINGLE_ENDED);

		/* Set ADC group regular sequencer length and scan direction */
		LL_ADC_REG_SetSequencerLength(ADC1, LL_ADC_REG_SEQ_SCAN_ENABLE_4RANKS);

		/* Set ADC group regular sequencer discontinuous mode */
		// LL_ADC_REG_SetSequencerDiscont(ADC1, LL_ADC_REG_SEQ_DISCONT_DISABLE);

		/* Set ADC group regular sequence: channel on the selected sequence rank. */
		LL_ADC_REG_SetSequencerRanks(ADC1, LL_ADC_REG_RANK_1, LL_ADC_CHANNEL_1);
		LL_ADC_REG_SetSequencerRanks(ADC1, LL_ADC_REG_RANK_1, LL_ADC_CHANNEL_3);
		LL_ADC_REG_SetSequencerRanks(ADC1, LL_ADC_REG_RANK_1, LL_ADC_CHANNEL_6);
		LL_ADC_REG_SetSequencerRanks(ADC1, LL_ADC_REG_RANK_1, LL_ADC_CHANNEL_8);
	}

	/*## Configuration of ADC hierarchical scope: ADC group injected ###########*/

	/* Note: Hardware constraint (refer to description of the functions         */
	/*       below):                                                            */
	/*       On this STM32 series, setting of these features is conditioned to   */
	/*       ADC state:                                                         */
	/*       ADC must be disabled or enabled without conversion on going        */
	/*       on group injected.                                                 */
	if ((LL_ADC_IsEnabled(ADC1) == 0)               ||
		(LL_ADC_INJ_IsConversionOngoing(ADC1) == 0)   )
	{
		/* Note: Call of the functions below are commented because they are       */
		/*       useless in this example:                                         */
		/*       setting corresponding to default configuration from reset state. */

		/* Set ADC group injected trigger source */
		// LL_ADC_INJ_SetTriggerSource(ADC1, LL_ADC_INJ_TRIG_SOFTWARE);

		/* Set ADC group injected trigger polarity */
		// LL_ADC_INJ_SetTriggerEdge(ADC1, LL_ADC_INJ_TRIG_EXT_RISING);

		/* Set ADC group injected conversion trigger  */
		// LL_ADC_INJ_SetTrigAuto(ADC1, LL_ADC_INJ_TRIG_INDEPENDENT);

		/* Set ADC group injected contexts queue mode */
		/* Note: If ADC group injected contexts queue are enabled, configure      */
		/*       contexts using function "LL_ADC_INJ_ConfigQueueContext()".       */
		// LL_ADC_INJ_SetQueueMode(ADC1, LL_ADC_INJ_QUEUE_DISABLE);

		/* Set ADC group injected sequencer */
		/* Note: On this STM32 series, ADC group injected sequencer is             */
		/*       fully configurable: sequencer length and each rank               */
		/*       affectation to a channel are configurable.                       */
		/*       Refer to description of function                                 */
		/*       "LL_ADC_INJ_SetSequencerLength()".                               */

		/* Set ADC group injected sequencer length and scan direction */
		// LL_ADC_INJ_SetSequencerLength(ADC1, LL_ADC_INJ_SEQ_SCAN_DISABLE);

		/* Set ADC group injected sequencer discontinuous mode */
		// LL_ADC_INJ_SetSequencerDiscont(ADC1, LL_ADC_INJ_SEQ_DISCONT_DISABLE);

		/* Set ADC group injected sequence: channel on the selected sequence rank. */
		// LL_ADC_INJ_SetSequencerRanks(ADC1, LL_ADC_INJ_RANK_1, LL_ADC_CHANNEL_15);
	}

	/*## Configuration of ADC hierarchical scope: channels #####################*/

	/* Note: Hardware constraint (refer to description of the functions         */
	/*       below):                                                            */
	/*       On this STM32 series, setting of these features is conditioned to   */
	/*       ADC state:                                                         */
	/*       ADC must be disabled or enabled without conversion on going        */
	/*       on either groups regular or injected.                              */
	if ((LL_ADC_IsEnabled(ADC1) == 0)                    ||
		((LL_ADC_REG_IsConversionOngoing(ADC1) == 0) &&
		(LL_ADC_INJ_IsConversionOngoing(ADC1) == 0)   )   )
	{
		/* Set ADC channels sampling time */
		/* Note: Considering interruption occurring after each ADC conversion     */
		/*       when ADC conversion is out of the analog watchdog window         */
		/*       selected (IT from ADC analog watchdog),                          */
		/*       select sampling time and ADC clock with sufficient               */
		/*       duration to not create an overhead situation in IRQHandler.      */
//!		LL_ADC_SetChannelSamplingTime(ADC1, LL_ADC_CHANNEL_15, LL_ADC_SAMPLINGTIME_810CYCLES_5);
		LL_ADC_SetChannelSamplingTime(ADC1, LL_ADC_CHANNEL_1, LL_ADC_SAMPLINGTIME_810CYCLES_5);
		LL_ADC_SetChannelSamplingTime(ADC1, LL_ADC_CHANNEL_3, LL_ADC_SAMPLINGTIME_810CYCLES_5);
		LL_ADC_SetChannelSamplingTime(ADC1, LL_ADC_CHANNEL_6, LL_ADC_SAMPLINGTIME_810CYCLES_5);
		LL_ADC_SetChannelSamplingTime(ADC1, LL_ADC_CHANNEL_8, LL_ADC_SAMPLINGTIME_810CYCLES_5);


		/* Set mode single-ended or differential input of the selected            */
		/* ADC channel.                                                           */
		// LL_ADC_SetChannelSingleDiff(ADC1, LL_ADC_CHANNEL_15, LL_ADC_SINGLE_ENDED);
	}

	/*## Configuration of ADC transversal scope: analog watchdog ###############*/

	/* Set ADC analog watchdog channels to be monitored */
//!	LL_ADC_SetAnalogWDMonitChannels(ADC1, LL_ADC_AWD1, LL_ADC_AWD_ALL_CHANNELS_REG);

	/* Set ADC analog watchdog thresholds */
//!  LL_ADC_ConfigAnalogWDThresholds(ADC1, LL_ADC_AWD1, ADC_AWD_THRESHOLD_HIGH, ADC_AWD_THRESHOLD_LOW);

	/*## Configuration of ADC transversal scope: oversampling ##################*/

	/* Note: Feature not available on this STM32 series */


	/*## Configuration of ADC interruptions ####################################*/
	/* Enable ADC analog watchdog 1 interruption */
	LL_ADC_EnableIT_EOC(ADC1);

	printf("adc conf \r\n");
}

static void Activate_ADC(void)
{
  __IO uint32_t wait_loop_index = 0U;

  #if (USE_TIMEOUT == 1)
  uint32_t Timeout = 0U; /* Variable used for timeout management */
  #endif /* USE_TIMEOUT */

  /*## Operation on ADC hierarchical scope: ADC instance #####################*/

  /* Note: Hardware constraint (refer to description of the functions         */
  /*       below):                                                            */
  /*       On this STM32 series, setting of these features is conditioned to   */
  /*       ADC state:                                                         */
  /*       ADC must be disabled.                                              */
  /* Note: In this example, all these checks are not necessary but are        */
  /*       implemented anyway to show the best practice usages                */
  /*       corresponding to reference manual procedure.                       */
  /*       Software can be optimized by removing some of these checks, if     */
  /*       they are not relevant considering previous settings and actions    */
  /*       in user application.                                               */
  if (LL_ADC_IsEnabled(ADC1) == 0)
  {
    /* Disable ADC deep power down (enabled by default after reset state) */
    LL_ADC_DisableDeepPowerDown(ADC1);

    /* Enable ADC internal voltage regulator */
    LL_ADC_EnableInternalRegulator(ADC1);

    /* Delay for ADC internal voltage regulator stabilization.                */
    /* Compute number of CPU cycles to wait for, from delay in us.            */
    /* Note: Variable divided by 2 to compensate partially                    */
    /*       CPU processing cycles (depends on compilation optimization).     */
    /* Note: If system core clock frequency is below 200kHz, wait time        */
    /*       is only a few CPU processing cycles.                             */
    wait_loop_index = ((LL_ADC_DELAY_INTERNAL_REGUL_STAB_US * (SystemCoreClock / (100000 * 2))) / 10);
    while(wait_loop_index != 0)
    {
      wait_loop_index--;
    }

    /* Run ADC self calibration */
    // Need to define which calib parameter has to be used
    // LL_ADC_StartCalibration(ADC1, LL_ADC_CALIB_OFFSET, LL_ADC_SINGLE_ENDED);
    // LL_ADC_StartCalibration(ADC1, LL_ADC_CALIB_OFFSET_LINEARITY, LL_ADC_SINGLE_ENDED);

    /* Poll for ADC effectively calibrated */
    #if (USE_TIMEOUT == 1)
    Timeout = ADC_CALIBRATION_TIMEOUT_MS;
    #endif /* USE_TIMEOUT */

    while (LL_ADC_IsCalibrationOnGoing(ADC1) != 0)
    {
    #if (USE_TIMEOUT == 1)
      /* Check Systick counter flag to decrement the time-out value */
      if (LL_SYSTICK_IsActiveCounterFlag())
      {
        if(Timeout-- == 0)
        {
        /* Time-out occurred. Set LED to blinking mode */
        LED_Blinking(LED_BLINK_ERROR);
        }
      }
    #endif /* USE_TIMEOUT */
    }

    /* Delay between ADC end of calibration and ADC enable.                   */
    /* Note: Variable divided by 2 to compensate partially                    */
    /*       CPU processing cycles (depends on compilation optimization).     */
    wait_loop_index = (ADC_DELAY_CALIB_ENABLE_CPU_CYCLES >> 1);
    while(wait_loop_index != 0)
    {
      wait_loop_index--;
    }

    /* Enable ADC */
    LL_ADC_Enable(ADC1);

    /* Poll for ADC ready to convert */
    #if (USE_TIMEOUT == 1)
    Timeout = ADC_ENABLE_TIMEOUT_MS;
    #endif /* USE_TIMEOUT */

    while (LL_ADC_IsActiveFlag_ADRDY(ADC1) == 0)
    {
    #if (USE_TIMEOUT == 1)
      /* Check Systick counter flag to decrement the time-out value */
      if (LL_SYSTICK_IsActiveCounterFlag())
      {
        if(Timeout-- == 0)
        {
        /* Time-out occurred. Set LED to blinking mode */
        LED_Blinking(LED_BLINK_ERROR);
        }
      }
    #endif /* USE_TIMEOUT */
    }

    /* Note: ADC flag ADRDY is not cleared here to be able to check ADC       */
    /*       status afterwards.                                               */
    /*       This flag should be cleared at ADC Deactivation, before a new    */
    /*       ADC activation, using function "LL_ADC_ClearFlag_ADRDY()".       */
  }

  /*## Operation on ADC hierarchical scope: ADC group regular ################*/
  /* Note: No operation on ADC group regular performed here.                  */
  /*       ADC group regular conversions to be performed after this function  */
  /*       using function:                                                    */
  /*       "LL_ADC_REG_StartConversion();"                                    */

  /*## Operation on ADC hierarchical scope: ADC group injected ###############*/
  /* Note: No operation on ADC group injected performed here.                 */
  /*       ADC group injected conversions to be performed after this function */
  /*       using function:                                                    */
  /*       "LL_ADC_INJ_StartConversion();"
   *                              */
  printf("adc act \r\n");
}

ushort ch1;
ushort ch3;
ushort ch6;
ushort ch8;

static void adc_proc_task(void *arg)
{
	for(;;)
	{
		vTaskDelay(5000);
		printf("ch1: %d, ch3: %d, ch6: %d, ch8: %d\r\n", ch1, ch3, ch6, ch8);
	}
}

void adc_callback(void)
{
	/* Disable ADC analog watchdog 1 interruption */
	//LL_ADC_DisableIT_EOC(ADC1);

	//printf("adc irq\r\n");
	ch1 = LL_ADC_REG_ReadConversionData16(ADC1);
	ch3 = LL_ADC_REG_ReadConversionData16(ADC1);
	ch6 = LL_ADC_REG_ReadConversionData16(ADC1);
	ch8 = LL_ADC_REG_ReadConversionData16(ADC1);
}

void adc_init(void)
{
	Configure_ADC();
	Activate_ADC();

	if((LL_ADC_IsEnabled(ADC1) == 1)&&(LL_ADC_IsDisableOngoing(ADC1) == 0)&&(LL_ADC_REG_IsConversionOngoing(ADC1) == 0))
	{
	    LL_ADC_REG_StartConversion(ADC1);
	    printf("adc start \r\n");
	}
	else
	{
		return;
	}

	// ADC processor
    xTaskCreate((TaskFunction_t)adc_proc_task, "adc_proc", 256, NULL, osPriorityNormal, NULL);
}
