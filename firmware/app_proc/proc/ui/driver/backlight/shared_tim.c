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
#include "mchf_pro_board.h"
#include "main.h"

#include "shared_tim.h"

#ifdef USE_LL_VERSION

#define TIM_DUTY_CYCLES_NB 11

static uint32_t aDutyCycle[TIM_DUTY_CYCLES_NB] = {
  0,    /*  0% */
  10,   /* 10% */
  20,   /* 20% */
  30,   /* 30% */
  40,   /* 40% */
  50,   /* 50% */
  60,   /* 60% */
  70,   /* 70% */
  80,   /* 80% */
  90,   /* 90% */
  100,  /* 100% */
};

static uint8_t iDutyCycle = 5;

__IO uint32_t uwMeasuredDutyCycle = 0;

static uint32_t TimOutClock = 1;

void TimerCaptureCompare_Callback(void)
{
	uint32_t CNT, ARR;

	CNT = LL_TIM_GetCounter(TIM1);
	ARR = LL_TIM_GetAutoReload(TIM1);

	if(LL_TIM_OC_GetCompareCH2(TIM1) > ARR )
	{
    /* If capture/compare setting is greater than autoreload,
     * there is a counter overflow and counter restarts from 0.
       Need to add full period to counter value (ARR+1)  */
		CNT = CNT + ARR + 1;
	}

	uwMeasuredDutyCycle = (CNT * 100) / ( ARR + 1 );
}

void Configure_DutyCycle(uint32_t D)
{
	uint32_t P;    /* Pulse duration */
	uint32_t T;    /* PWM signal period */

	/* PWM signal period is determined by the value of the auto-reload register */
	T = LL_TIM_GetAutoReload(TIM1) + 1;

	/* Pulse duration is determined by the value of the compare register.       */
	/* Its value is calculated in order to match the requested duty cycle.      */
	P = (D*T)/100;
	LL_TIM_OC_SetCompareCH2(TIM1, P);
}

void TIM1_CC_IRQHandler(void)
{
	if(LL_TIM_IsActiveFlag_CC2(TIM1) == 1)
	{
		LL_TIM_ClearFlag_CC2(TIM1);
		TimerCaptureCompare_Callback();
	}
}

void Configure_TIMPWMOutput(void)
{
  /*************************/
  /* GPIO AF configuration */
  /*************************/
  /* Enable the peripheral clock of GPIOs */
  //LL_AHB4_GRP1_EnableClock(LL_AHB4_GRP1_PERIPH_GPIOB);

  /* GPIO TIM1_CH2 configuration */
  LL_GPIO_SetPinMode	(LCD_BL_CTRL_GPIO_PORT, LCD_BL_CTRL_PIN, LL_GPIO_MODE_ALTERNATE);
  LL_GPIO_SetPinPull	(LCD_BL_CTRL_GPIO_PORT, LCD_BL_CTRL_PIN, LL_GPIO_PULL_DOWN);
  LL_GPIO_SetPinSpeed	(LCD_BL_CTRL_GPIO_PORT, LCD_BL_CTRL_PIN, LL_GPIO_SPEED_FREQ_HIGH);
  LL_GPIO_SetAFPin_8_15	(LCD_BL_CTRL_GPIO_PORT, LCD_BL_CTRL_PIN, LCD_BL_CTRL_AF);

  /***********************************************/
  /* Configure the NVIC to handle TIM3 interrupt */
  /***********************************************/
  NVIC_SetPriority	(TIM1_CC_IRQn, 0);
  NVIC_EnableIRQ	(TIM1_CC_IRQn);

  /******************************/
  /* Peripheral clocks enabling */
  /******************************/
  /* Enable the timer peripheral clock */
  LL_APB1_GRP1_EnableClock(LL_APB1_GRP1_PERIPH_TIM12);

  /***************************/
  /* Time base configuration */
  /***************************/
  /* Set counter mode */
  /* Reset value is LL_TIM_COUNTERMODE_UP */
  //LL_TIM_SetCounterMode(TIM3, LL_TIM_COUNTERMODE_UP);

  /* Set the pre-scaler value to have TIM3 counter clock equal to 10 kHz */
  LL_TIM_SetPrescaler(TIM1, __LL_TIM_CALC_PSC(SystemCoreClock, 10000));

  /* Enable TIM3_ARR register preload. Writing to or reading from the         */
  /* auto-reload register accesses the preload register. The content of the   */
  /* preload register are transferred into the shadow register at each update */
  /* event (UEV).                                                             */
  LL_TIM_EnableARRPreload(TIM1);

  /* Set the auto-reload value to have a counter frequency of 100 Hz */
  /* TIM3CLK = SystemCoreClock / (APB prescaler & multiplier)               */
  TimOutClock = SystemCoreClock/1;
  LL_TIM_SetAutoReload(TIM1, __LL_TIM_CALC_ARR(TimOutClock, LL_TIM_GetPrescaler(TIM1), 100));

  /*********************************/
  /* Output waveform configuration */
  /*********************************/
  /* Set output mode */
  /* Reset value is LL_TIM_OCMODE_FROZEN */
  LL_TIM_OC_SetMode(TIM1, LCD_BL_CTRL_TIM_CH, LL_TIM_OCMODE_PWM1);

  /* Set output channel polarity */
  /* Reset value is LL_TIM_OCPOLARITY_HIGH */
  //LL_TIM_OC_SetPolarity(TIM3, LL_TIM_CHANNEL_CH3, LL_TIM_OCPOLARITY_HIGH);

  /* Set compare value to half of the counter period (50% duty cycle ) */
  LL_TIM_OC_SetCompareCH2(TIM1, ( (LL_TIM_GetAutoReload(TIM1) + 1 ) / 2));

  /* Enable TIM3_CCR3 register preload. Read/Write operations access the      */
  /* preload register. TIM3_CCR3 preload value is loaded in the active        */
  /* at each update event.                                                    */
  LL_TIM_OC_EnablePreload(TIM1, LCD_BL_CTRL_TIM_CH);

  /**************************/
  /* TIM3 interrupts set-up */
  /**************************/
  /* Enable the capture/compare interrupt for channel 1*/
  LL_TIM_EnableIT_CC2(TIM1);

  /**********************************/
  /* Start output signal generation */
  /**********************************/
  /* Enable output channel 3 */
  LL_TIM_CC_EnableChannel(TIM1, LCD_BL_CTRL_TIM_CH);

  /* Enable counter */
  LL_TIM_EnableCounter(TIM1);

  /* Force update generation */
  LL_TIM_GenerateEvent_UPDATE(TIM1);
}
#else
#define  PERIOD_VALUE       (uint32_t)(1000 - 1)  /* Period Value  */

//#define  PULSE1_VALUE       (uint32_t)(PERIOD_VALUE*99.5/100)
#define  PULSE2_VALUE       (uint32_t)(PERIOD_VALUE*80/100)

//#define  PULSE3_VALUE       (uint32_t)(PERIOD_VALUE/4)        /* Capture Compare 3 Value  */
//#define  PULSE4_VALUE       (uint32_t)(PERIOD_VALUE*12.5/100) /* Capture Compare 4 Value  */

TIM_HandleTypeDef	TimHandle;
TIM_OC_InitTypeDef 	sConfig;
uint32_t 			uhPrescalerValue = 0;

void HAL_TIM_PWM_MspInit(TIM_HandleTypeDef *htim)
{
  GPIO_InitTypeDef   GPIO_InitStruct;
  /*##-1- Enable peripherals and GPIO Clocks #################################*/
  /* TIMx Peripheral clock enable */
  __HAL_RCC_TIM1_CLK_ENABLE();

  /* Common configuration for all channels */
  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;

  // Charging only in bootloader!
  //GPIO_InitStruct.Alternate = GPIO_AF1_TIM1;
  //GPIO_InitStruct.Pin = BMS_PWM_PIN;
  //HAL_GPIO_Init(BMS_PWM_PORT, &GPIO_InitStruct);

  GPIO_InitStruct.Alternate = GPIO_AF1_TIM1;
  GPIO_InitStruct.Pin = LCD_BL_CTRL_PIN;
  HAL_GPIO_Init(LCD_BL_CTRL_GPIO_PORT, &GPIO_InitStruct);
}

#endif

void shared_tim_change(uchar val)
{
#ifndef REV_0_8_4_PATCH
	if(val > 100)
		val = 100;
	else if(val < 5)
		val = 5;

	// Set the pulse value for channel 2
	sConfig.Pulse = (uint32_t)(PERIOD_VALUE*val/100);

	HAL_TIM_PWM_Stop(&TimHandle, TIM_CHANNEL_2);
	HAL_TIM_PWM_ConfigChannel(&TimHandle, &sConfig, TIM_CHANNEL_2);
	HAL_TIM_PWM_Start(&TimHandle, TIM_CHANNEL_2);
#endif
}

#ifndef REV_0_8_4_PATCH
static void shared_tim_init_a(void)
{
	#ifdef USE_LL_VERSION
	/* Configure the timer in output compare mode */
	Configure_TIMPWMOutput();

	/* Change PWM signal duty cycle */
	Configure_DutyCycle(aDutyCycle[iDutyCycle]);
	#else

	/* Compute the prescaler value to have TIM1 counter clock equal to 20000000 Hz */
	uhPrescalerValue = (uint32_t)(SystemCoreClock / (2*20000000)) - 1;

	  /*##-1- Configure the TIM peripheral #######################################*/
	  /* -----------------------------------------------------------------------
	  TIM1 Configuration: generate 4 PWM signals with 4 different duty cycles.

	    In this example TIM1 input clock (TIM1CLK) is set to APB2 clock (PCLK2),
	    since APB2 prescaler is equal to 2.
	      TIM1CLK = 2*PCLK2
	      PCLK2 = HCLK/2 as AHB Clock divider is set to RCC_HCLK_DIV2
	      => TIM1CLK = HCLK = SystemCoreClock/2

	    To get TIM1 counter clock at 20 MHz, the prescaler is computed as follows:
	       Prescaler = (TIM1CLK / TIM1 counter clock) - 1
	       Prescaler = ((SystemCoreClock) /(2*20 MHz)) - 1

	    To get TIM1 output clock at 20 KHz, the period (ARR)) is computed as follows:
	       ARR = (TIM1 counter clock / TIM1 output clock) - 1
	           = 999

	    TIM1 Channel1 duty cycle = (TIM1_CCR1/ TIM1_ARR + 1)* 100 = 50%
	    TIM1 Channel2 duty cycle = (TIM1_CCR2/ TIM1_ARR + 1)* 100 = 37.5%
	    TIM1 Channel3 duty cycle = (TIM1_CCR3/ TIM1_ARR + 1)* 100 = 25%
	    TIM1 Channel4 duty cycle = (TIM1_CCR4/ TIM1_ARR + 1)* 100 = 12.5%

	    Note:
	     SystemCoreClock variable holds HCLK frequency and is defined in system_stm32h7xx.c file.
	     Each time the core clock (HCLK) changes, user had to update SystemCoreClock
	     variable value. Otherwise, any configuration based on this variable will be incorrect.
	     This variable is updated in three ways:
	      1) by calling CMSIS function SystemCoreClockUpdate()
	      2) by calling HAL API function HAL_RCC_GetSysClockFreq()
	      3) each time HAL_RCC_ClockConfig() is called to configure the system clock frequency
	  ----------------------------------------------------------------------- */

	  /* Initialize TIMx peripheral as follows:
	       + Prescaler = (SystemCoreClock / (2*20000000)) - 1
	       + Period = (1000 - 1)
	       + ClockDivision = 0
	       + Counter direction = Up
	  */

	  TimHandle.Instance = TIM1;

	  TimHandle.Init.Prescaler         = uhPrescalerValue;
	  TimHandle.Init.Period            = PERIOD_VALUE;
	  TimHandle.Init.ClockDivision     = 0;
	  TimHandle.Init.CounterMode       = TIM_COUNTERMODE_UP;
	  TimHandle.Init.RepetitionCounter = 0;
	  if (HAL_TIM_PWM_Init(&TimHandle) != HAL_OK)
	  {
	    /* Initialization Error */
	    //Error_Handler();
	  }

	  sConfig.OCMode       = TIM_OCMODE_PWM1;
	  sConfig.OCPolarity   = TIM_OCPOLARITY_HIGH;
	  sConfig.OCFastMode   = TIM_OCFAST_DISABLE;
	  sConfig.OCNPolarity  = TIM_OCNPOLARITY_HIGH;
	  sConfig.OCNIdleState = TIM_OCNIDLESTATE_RESET;

	  sConfig.OCIdleState  = TIM_OCIDLESTATE_RESET;
#if 0
	  /* Set the pulse value for channel 1 */
	  sConfig.Pulse = PULSE1_VALUE;
	  if (HAL_TIM_PWM_ConfigChannel(&TimHandle, &sConfig, TIM_CHANNEL_1) != HAL_OK)
	  {
	    /* Configuration Error */
	    //Error_Handler();
	  }
#endif
	  /* Set the pulse value for channel 2 */
	  sConfig.Pulse = PULSE2_VALUE;
	  if (HAL_TIM_PWM_ConfigChannel(&TimHandle, &sConfig, TIM_CHANNEL_2) != HAL_OK)
	  {
	    /* Configuration Error */
	    //Error_Handler();
	  }
#if 0
	  if (HAL_TIM_PWM_Start(&TimHandle, TIM_CHANNEL_1) != HAL_OK)
	  {
	    /* PWM Generation Error */
	    //Error_Handler();
	  }
#endif
	  /* Start channel 2 */
	  if (HAL_TIM_PWM_Start(&TimHandle, TIM_CHANNEL_2) != HAL_OK)
	  {
	    /* PWM Generation Error */
	    //Error_Handler();
	  }

	#endif
}
#endif

void shared_tim_init(void)
{
	#ifndef REV_0_8_4_PATCH
	shared_tim_init_a();
	#else
	GPIO_InitTypeDef   GPIO_InitStruct;

	  /* Common configuration for all channels */
	  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
	  //GPIO_InitStruct.Pull = GPIO_PULLUP;
	  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;

	  GPIO_InitStruct.Pin = LCD_BL_CTRL_PIN;
	  HAL_GPIO_Init(LCD_BL_CTRL_GPIO_PORT, &GPIO_InitStruct);

	  HAL_GPIO_WritePin(LCD_BL_CTRL_GPIO_PORT, LCD_BL_CTRL_PIN, GPIO_PIN_SET);
	#endif
}
