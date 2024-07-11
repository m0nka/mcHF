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

#ifdef CONTEXT_BMS

#include "bms_proc.h"

//#define USE_DMA

ADC_HandleTypeDef    Adc1Handle;
//ADC_HandleTypeDef    Adc2Handle;

#ifdef USE_DMA
DMA_HandleTypeDef         DmaHandle;
#endif

struct BMSState	bmss;
uchar bms_early_init_done = 0;

//ulong sampling_skip 	= NORMAL_PROC_REFRESH;
//ulong proc_refresh_rate = NORMAL_PROC_REFRESH;	// 4s
//ulong ulCH1 = 0,ulCH2 = 0,ulCH3 = 0,ulCH4 = 0,ulCH5 = 0, ulCH6 = 0, ulCH8 = 0, ulCH9 = 0, ulCH10 = 0, ulCH11 = 0;
//ulong usCV[4],usChargeValue,usLoadValue;
//ulong ulTx[4];
//ulong usBalID[4];
//ulong numb_of_meas = 0;
//uchar charger_on = 0;
//uchar h_prot_on  = 0;
//uchar bal = 0;

#ifdef USE_DMA
#define ADC_CONVERTED_DATA_BUFFER_SIZE   ((uint32_t)  32)   /* Size of array aADCxConvertedData[], Aligned on cache line size */

ALIGN_32BYTES (static uint16_t   aADCxConvertedData[ADC_CONVERTED_DATA_BUFFER_SIZE]);

ushort a_val = 0;

void DMA1_Stream6_IRQHandler(void)
{
	HAL_DMA_IRQHandler(Adc1Handle.DMA_Handle);
}

void HAL_ADC_ConvHalfCpltCallback(ADC_HandleTypeDef* hadc)
{
	/* Invalidate Data Cache to get the updated content of the SRAM on the first half of the ADC converted data buffer: 32 bytes */
	SCB_InvalidateDCache_by_Addr((uint32_t *) &aADCxConvertedData[0], ADC_CONVERTED_DATA_BUFFER_SIZE);
	a_val = aADCxConvertedData[0];
}

void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef* hadc)
{
	/* Invalidate Data Cache to get the updated content of the SRAM on the second half of the ADC converted data buffer: 32 bytes */
	SCB_InvalidateDCache_by_Addr((uint32_t *) &aADCxConvertedData[ADC_CONVERTED_DATA_BUFFER_SIZE/2], ADC_CONVERTED_DATA_BUFFER_SIZE);
}
#endif

// 10k NTC, https://www.skyeinstruments.com/wp-content/uploads/Steinhart-Hart-Eqn-for-10k-Thermistors.pdf
// R = 10K*ADC / (1023 - ADC), where 10K is the NTC voltage divider value, 1023(8-bit) is max ADC count
// https://learn.adafruit.com/thermistor/using-a-thermistor
static float bms_proc_steinhart_hart(float adc_cnt)
{
	const float SH_A = 0.001125308852122f;
	const float SH_B = 0.000234711863267f;
	const float SH_C = 0.000000085663516f;
	float res, logr;

	// ToDo: Vcc and Vref do not match, probably. Add compensation!
	//

	// ADC count to resistance (assumes NTC VCC = ADC VREF!)
	res = (10.0f * adc_cnt / ((float)ADC_RESOLUTION - adc_cnt)) * 1000.0f;

	// Steinhart-Hart
	logr = log(res);
	return (1.0f / (SH_A + SH_B * logr + SH_C * (pow(logr, 3.0))) - 273.15f);
}

//*----------------------------------------------------------------------------
//* Function Name       : HAL_ADC_MspInit
//* Object              :
//* Notes    			: HAL ADC Init callback
//* Notes   			:
//* Notes    			:
//* Context    			: CONTEXT_BMS
//*----------------------------------------------------------------------------
void HAL_ADC_MspInit(ADC_HandleTypeDef *hadc)
{
	#ifdef USE_DMA
	__HAL_RCC_DMA1_CLK_ENABLE();

	//--printf("init adc1 dma\r\n");
	DmaHandle.Instance                 = DMA1_Stream6;
	DmaHandle.Init.Request             = DMA_REQUEST_ADC1;
	DmaHandle.Init.Direction           = DMA_PERIPH_TO_MEMORY;
	DmaHandle.Init.PeriphInc           = DMA_PINC_DISABLE;
	DmaHandle.Init.MemInc              = DMA_MINC_ENABLE;
	DmaHandle.Init.PeriphDataAlignment = DMA_PDATAALIGN_HALFWORD;
	DmaHandle.Init.MemDataAlignment    = DMA_MDATAALIGN_HALFWORD;
	DmaHandle.Init.Mode                = DMA_NORMAL;
	DmaHandle.Init.Priority            = DMA_PRIORITY_MEDIUM;

	/* Deinitialize  & Initialize the DMA for new transfer */
	HAL_DMA_DeInit(&DmaHandle);
	HAL_DMA_Init(&DmaHandle);

	/* Associate the DMA handle */
	__HAL_LINKDMA(hadc, DMA_Handle, DmaHandle);

	// NVIC configuration for DMA Input data interrupt
	HAL_NVIC_SetPriority(DMA1_Stream6_IRQn, 1, 0);
	HAL_NVIC_EnableIRQ	(DMA1_Stream6_IRQn);
	#endif
}

#if 0
//*----------------------------------------------------------------------------
//* Function Name       : HAL_ADC_MspDeInit
//* Object              :
//* Notes    			: HAL ADC DeInit callback
//* Notes   			:
//* Notes    			:
//* Context    			: CONTEXT_BMS
//*----------------------------------------------------------------------------
void HAL_ADC_MspDeInit(ADC_HandleTypeDef *hadc)
{
#if 0
	ADCx_FORCE_RESET();
	ADCx_RELEASE_RESET();

	// ADC Periph clock disable(automatically reset all ADC's)
	__HAL_RCC_ADC12_CLK_DISABLE();

	HAL_GPIO_DeInit(ADCx_CELL1_GPIO_PORT, ADCx_CELL1_PIN);
	HAL_GPIO_DeInit(ADCx_CELL2_GPIO_PORT, ADCx_CELL2_PIN);
	HAL_GPIO_DeInit(ADCx_CELL3_GPIO_PORT, ADCx_CELL3_PIN);
	HAL_GPIO_DeInit(ADCx_CELL4_GPIO_PORT, ADCx_CELL4_PIN);

	HAL_GPIO_DeInit(ADCx_CHARGE_GPIO_PORT,ADCx_CHARGE_PIN);
	HAL_GPIO_DeInit(ADCx_LOAD_GPIO_PORT,  ADCx_LOAD_PIN);

	HAL_GPIO_DeInit(ADCx_TEMP1_GPIO_PORT, ADCx_TEMP1_PIN);
	HAL_GPIO_DeInit(ADCx_TEMP2_GPIO_PORT, ADCx_TEMP2_PIN);
	HAL_GPIO_DeInit(ADCx_TEMP3_GPIO_PORT, ADCx_TEMP3_PIN);
	HAL_GPIO_DeInit(ADCx_TEMP4_GPIO_PORT, ADCx_TEMP4_PIN);
#endif
}
#endif

static void bms_proc_gpio_init(void)
{
	GPIO_InitTypeDef          GPIO_InitStruct;
	RCC_PeriphCLKInitTypeDef  PeriphClkInitStruct;

	//printf("HAL_ADC_MspInit\r\n");

	PeriphClkInitStruct.PeriphClockSelection = RCC_PERIPHCLK_ADC;
	PeriphClkInitStruct.AdcClockSelection    = RCC_ADCCLKSOURCE_CLKP;
	PeriphClkInitStruct.PLL2.PLL2P           = 4;

	HAL_RCCEx_PeriphCLKConfig(&PeriphClkInitStruct);

	ADCx_CLK_ENABLE();

	__HAL_RCC_ADC_CONFIG(RCC_ADCCLKSOURCE_CLKP);

	//ADCx_CHANNEL_GPIO_CLK_ENABLE();

	GPIO_InitStruct.Mode 	= GPIO_MODE_ANALOG;
	GPIO_InitStruct.Pull	= GPIO_NOPULL;

	GPIO_InitStruct.Pin 	= ADCx_CELL1_PIN;
	HAL_GPIO_Init(ADCx_CELL1_GPIO_PORT, &GPIO_InitStruct);

	GPIO_InitStruct.Pin 	= ADCx_CELL2_PIN;
	HAL_GPIO_Init(ADCx_CELL2_GPIO_PORT, &GPIO_InitStruct);

	GPIO_InitStruct.Pin 	= ADCx_CELL3_PIN;
	HAL_GPIO_Init(ADCx_CELL3_GPIO_PORT, &GPIO_InitStruct);

	GPIO_InitStruct.Pin 	= ADCx_CELL4_PIN;
	HAL_GPIO_Init(ADCx_CELL4_GPIO_PORT, &GPIO_InitStruct);

	GPIO_InitStruct.Pin 	= ADCx_CHARGE_PIN;
	HAL_GPIO_Init(ADCx_CHARGE_GPIO_PORT, &GPIO_InitStruct);

	GPIO_InitStruct.Pin 	= ADCx_LOAD_PIN;
	HAL_GPIO_Init(ADCx_LOAD_GPIO_PORT, &GPIO_InitStruct);

	GPIO_InitStruct.Pin 	= ADCx_TEMP1_PIN;
	HAL_GPIO_Init(ADCx_TEMP1_GPIO_PORT, &GPIO_InitStruct);

	GPIO_InitStruct.Pin 	= ADCx_TEMP2_PIN;
	HAL_GPIO_Init(ADCx_TEMP2_GPIO_PORT, &GPIO_InitStruct);

	GPIO_InitStruct.Pin 	= ADCx_TEMP3_PIN;
	HAL_GPIO_Init(ADCx_TEMP3_GPIO_PORT, &GPIO_InitStruct);

	GPIO_InitStruct.Pin 	= ADCx_TEMP4_PIN;
	HAL_GPIO_Init(ADCx_TEMP4_GPIO_PORT, &GPIO_InitStruct);
}

static int bms_proc_adc1_init(void)
{
	ADC_ChannelConfTypeDef sConfig;

	/*##-1- Configure the ADC peripheral #######################################*/
	Adc1Handle.Instance          = ADC1;

	if (HAL_ADC_DeInit(&Adc1Handle) != HAL_OK)
		return 1;

	#ifndef USE_DMA
	Adc1Handle.Init.ClockPrescaler           			= ADC_CLOCK_ASYNC_DIV2;
	Adc1Handle.Init.Resolution               			= ADC_RESOLUTION_16B;
	Adc1Handle.Init.ScanConvMode             			= ENABLE;
	Adc1Handle.Init.EOCSelection             			= ADC_EOC_SINGLE_CONV;
	Adc1Handle.Init.LowPowerAutoWait         			= DISABLE;
	Adc1Handle.Init.ContinuousConvMode       			= DISABLE;
	Adc1Handle.Init.NbrOfConversion          			= ADC_CHANNELS_CNT;
	Adc1Handle.Init.DiscontinuousConvMode    			= DISABLE;
	Adc1Handle.Init.NbrOfDiscConversion      			= 1;
	Adc1Handle.Init.ExternalTrigConv         			= ADC_SOFTWARE_START;
	Adc1Handle.Init.ExternalTrigConvEdge     			= ADC_EXTERNALTRIGCONVEDGE_NONE;
	Adc1Handle.Init.ConversionDataManagement 			= ADC_CONVERSIONDATA_DR;
	Adc1Handle.Init.Overrun                  			= ADC_OVR_DATA_OVERWRITTEN;

	#ifndef USE_OVERSAMPLER
	Adc1Handle.Init.OversamplingMode         			= DISABLE;
	#else
	Adc1Handle.Init.OversamplingMode          		  	= ENABLE;
	Adc1Handle.Init.Oversampling.Ratio                 	= OVERSAMPLING_RATIO;
	Adc1Handle.Init.Oversampling.RightBitShift         	= RIGHTBITSHIFT;
	Adc1Handle.Init.Oversampling.TriggeredMode         	= TRIGGEREDMODE;
	Adc1Handle.Init.Oversampling.OversamplingStopReset 	= OVERSAMPLINGSTOPRESET;
	#endif
	#else
	Adc1Handle.Init.ClockPrescaler           = ADC_CLOCK_ASYNC_DIV2;            /* Asynchronous clock mode, input ADC clock divided by 2*/
	Adc1Handle.Init.Resolution               = ADC_RESOLUTION_16B;              /* 16-bit resolution for converted data */
	Adc1Handle.Init.ScanConvMode             = DISABLE;                         /* Sequencer disabled (ADC conversion on only 1 channel: channel set on rank 1) */
	Adc1Handle.Init.EOCSelection             = ADC_EOC_SINGLE_CONV;             /* EOC flag picked-up to indicate conversion end */
	Adc1Handle.Init.LowPowerAutoWait         = DISABLE;                         /* Auto-delayed conversion feature disabled */
	Adc1Handle.Init.ContinuousConvMode       = ENABLE;                          /* Continuous mode enabled (automatic conversion restart after each conversion) */
	Adc1Handle.Init.NbrOfConversion          = 1;                               /* Parameter discarded because sequencer is disabled */
	Adc1Handle.Init.DiscontinuousConvMode    = DISABLE;                         /* Parameter discarded because sequencer is disabled */
	Adc1Handle.Init.NbrOfDiscConversion      = 1;                               /* Parameter discarded because sequencer is disabled */
	Adc1Handle.Init.ExternalTrigConv         = ADC_SOFTWARE_START;              /* Software start to trig the 1st conversion manually, without external event */
	Adc1Handle.Init.ExternalTrigConvEdge     = ADC_EXTERNALTRIGCONVEDGE_NONE;   /* Parameter discarded because software trigger chosen */
	Adc1Handle.Init.ConversionDataManagement = ADC_CONVERSIONDATA_DMA_ONESHOT;  /* ADC DMA circular requested */
	Adc1Handle.Init.Overrun                  = ADC_OVR_DATA_OVERWRITTEN;        /* DR register is overwritten with the last conversion result in case of overrun */
	Adc1Handle.Init.OversamplingMode         = DISABLE;                         /* No oversampling */
	#endif

	if (HAL_ADC_Init(&Adc1Handle) != HAL_OK)
	    return 2;

	// Run the ADC calibration in single-ended mode
	if (HAL_ADCEx_Calibration_Start(&Adc1Handle, ADC_CALIB_OFFSET, ADC_SINGLE_ENDED) != HAL_OK)
		return 3;

	sConfig.SamplingTime = ADC_SAMPLINGTIME;
	sConfig.SingleDiff   = ADC_SINGLE_ENDED;
	sConfig.OffsetNumber = ADC_OFFSET_NONE;
	sConfig.Offset 		 = 0;
	sConfig.OffsetRightShift       = DISABLE;                    /* No Right Offset Shift */
	sConfig.OffsetSignedSaturation = DISABLE;                    /* No Signed Saturation */

	sConfig.Channel      = ADCx_CH_CELL1;
	sConfig.Rank         = ADC_REGULAR_RANK_1;
	if(HAL_ADC_ConfigChannel(&Adc1Handle, &sConfig) != HAL_OK)
		return 4;

	#ifndef USE_DMA
	sConfig.Channel      = ADCx_CH_CELL2;
	sConfig.Rank         = ADC_REGULAR_RANK_2;
	if(HAL_ADC_ConfigChannel(&Adc1Handle, &sConfig) != HAL_OK)
		return 5;

	sConfig.Channel      = ADCx_CH_CELL3;
	sConfig.Rank         = ADC_REGULAR_RANK_3;
	if (HAL_ADC_ConfigChannel(&Adc1Handle, &sConfig) != HAL_OK)
		return 6;

	sConfig.Channel      = ADCx_CH_CELL4;
	sConfig.Rank         = ADC_REGULAR_RANK_4;
	if (HAL_ADC_ConfigChannel(&Adc1Handle, &sConfig) != HAL_OK)
		return 7;

	#if 1
	sConfig.Channel      = ADCx_CH_CHARGE;
	sConfig.Rank         = ADC_REGULAR_RANK_5;
	if (HAL_ADC_ConfigChannel(&Adc1Handle, &sConfig) != HAL_OK)
		return 8;

	sConfig.Channel      = ADCx_CH_LOAD;
	sConfig.Rank         = ADC_REGULAR_RANK_6;
	if (HAL_ADC_ConfigChannel(&Adc1Handle, &sConfig) != HAL_OK)
		return 9;
	#endif

	sConfig.Channel      = ADCx_CH_TEMP1;
	sConfig.Rank         = ADC_REGULAR_RANK_7;
	if (HAL_ADC_ConfigChannel(&Adc1Handle, &sConfig) != HAL_OK)
		return 10;

	sConfig.Channel      = ADCx_CH_TEMP2;
	sConfig.Rank         = ADC_REGULAR_RANK_8;
	if (HAL_ADC_ConfigChannel(&Adc1Handle, &sConfig) != HAL_OK)
		return 11;

	sConfig.Channel      = ADCx_CH_TEMP3;
	sConfig.Rank         = ADC_REGULAR_RANK_9;
	if (HAL_ADC_ConfigChannel(&Adc1Handle, &sConfig) != HAL_OK)
		return 12;

	sConfig.Channel      = ADCx_CH_TEMP4;
	sConfig.Rank         = ADC_REGULAR_RANK_10;
	if (HAL_ADC_ConfigChannel(&Adc1Handle, &sConfig) != HAL_OK)
		return 13;
#endif

	#ifdef USE_DMA
	if (HAL_ADC_Start_DMA(&Adc1Handle, (uint32_t *)aADCxConvertedData, 1) != HAL_OK)
	    return 14;
	#endif

	return 0;
}

#if 0
static int bms_proc_adc2_init(void)
{
	ADC_ChannelConfTypeDef sConfig;

	printf("bms_proc_adc2_init\r\n");

	/*##-1- Configure the ADC peripheral #######################################*/
	Adc2Handle.Instance          = ADC2;

	if (HAL_ADC_DeInit(&Adc2Handle) != HAL_OK)
		return 1;

	Adc2Handle.Init.ClockPrescaler           = ADC_CLOCK_ASYNC_DIV2;          /* Asynchronous clock mode, input ADC clock divided by 2*/
	Adc2Handle.Init.Resolution               = ADC_RESOLUTION_16B;            /* 16-bit resolution for converted data */
	Adc2Handle.Init.ScanConvMode             = DISABLE;                        /* Sequencer disabled (ADC conversion on only 1 channel: channel set on rank 1) */
	Adc2Handle.Init.EOCSelection             = ADC_EOC_SINGLE_CONV;           /* EOC flag picked-up to indicate conversion end */
	Adc2Handle.Init.LowPowerAutoWait         = DISABLE;                       /* Auto-delayed conversion feature disabled */
	Adc2Handle.Init.ContinuousConvMode       = ENABLE;                        /* Continuous mode disabled to have only 1 conversion at each conversion trig */
	Adc2Handle.Init.NbrOfConversion          = 1;   				           /* Parameter discarded because sequencer is disabled */
	Adc2Handle.Init.DiscontinuousConvMode    = DISABLE;                       /* Parameter discarded because sequencer is disabled */
	Adc2Handle.Init.NbrOfDiscConversion      = 1;                             /* Parameter discarded because sequencer is disabled */
	Adc2Handle.Init.ExternalTrigConv         = ADC_SOFTWARE_START;            /* Software start to trig the 1st conversion manually, without external event */
	Adc2Handle.Init.ExternalTrigConvEdge     = ADC_EXTERNALTRIGCONVEDGE_NONE; /* Parameter discarded because software trigger chosen */
	Adc2Handle.Init.ConversionDataManagement = ADC_CONVERSIONDATA_DR;         /* Regular Conversion data stored in DR register only */
	Adc2Handle.Init.Overrun                  = ADC_OVR_DATA_OVERWRITTEN;      /* DR register is overwritten with the last conversion result in case of overrun */
	Adc2Handle.Init.OversamplingMode         = DISABLE;                       /* No oversampling */

	if (HAL_ADC_Init(&Adc2Handle) != HAL_OK)
	    return 2;

	// Run the ADC calibration in single-ended mode
	if (HAL_ADCEx_Calibration_Start(&Adc2Handle, ADC_CALIB_OFFSET, ADC_DIFFERENTIAL_ENDED) != HAL_OK)
		return 3;

	sConfig.SamplingTime = ADC_SAMPLINGTIME;
	sConfig.SingleDiff   = ADC_DIFFERENTIAL_ENDED;
	sConfig.OffsetNumber = ADC_OFFSET_NONE;
	sConfig.Offset 		 = 0;
	sConfig.OffsetRightShift       = DISABLE;                    /* No Right Offset Shift */
	sConfig.OffsetSignedSaturation = DISABLE;                    /* No Signed Saturation */

	#if 0
	sConfig.Channel      = ADCx_CH_CHARGE;
	sConfig.Rank         = ADC_REGULAR_RANK_1;
	if (HAL_ADC_ConfigChannel(&Adc2Handle, &sConfig) != HAL_OK)
		return 4;
	#else
	sConfig.Channel      = ADCx_CH_LOAD;
	sConfig.Rank         = ADC_REGULAR_RANK_1;
	if (HAL_ADC_ConfigChannel(&Adc2Handle, &sConfig) != HAL_OK)
		return 4;
	#endif

	if(HAL_ADC_Start(&Adc2Handle) != HAL_OK)
	{
		//printf("error sampling\r\n");
		return 5;
	}

	return 0;
}
#endif

static void bms_proc_pins_init(void)
{
	GPIO_InitTypeDef  gpio_init_structure;

	gpio_init_structure.Mode  = GPIO_MODE_OUTPUT_PP;
	gpio_init_structure.Pull  = GPIO_NOPULL;
	gpio_init_structure.Speed = GPIO_SPEED_FREQ_LOW;

	// BAL1 is PG6, BAL2 is PG3, BAL3 is PG2
	gpio_init_structure.Pin   = BAL1_ON|BAL2_ON|BAL3_ON;
	HAL_GPIO_Init(BAL13_PORT, &gpio_init_structure);

	// BAL4 is PD7
	gpio_init_structure.Pin   = BAL4_ON;
	HAL_GPIO_Init(BAL4_PORT, &gpio_init_structure);

	// Force OFF
	HAL_GPIO_WritePin(BAL13_PORT, BAL1_ON|BAL2_ON|BAL3_ON, 	GPIO_PIN_RESET);
	HAL_GPIO_WritePin(BAL4_PORT,  BAL4_ON, 					GPIO_PIN_RESET);

	// Power button (encoder switch line)
	gpio_init_structure.Pin   = POWER_BUTTON;
	gpio_init_structure.Mode  = GPIO_MODE_INPUT;
	HAL_GPIO_Init(POWER_BUTTON_PORT, &gpio_init_structure);
}

//*----------------------------------------------------------------------------
//* Function Name       : bms_proc_hw_init
//* Object              :
//* Notes    			:  call from main() on startup!
//* Notes   			:
//* Notes    			:
//* Context    			: CONTEXT_RESET
//*----------------------------------------------------------------------------
void bms_proc_hw_init(void)
{
	int res;

	//printf("bms_proc_hw_init\r\n");

	bms_proc_pins_init();
	bms_proc_gpio_init();

	#ifndef USE_DMA
	#if 1
	res = bms_proc_adc1_init();
	if(res != 0)
	{
		printf("err adc1 init (%d)\r\n", res);
		return;
	}
	#else
	res = bms_proc_adc2_init();
	if(res != 0)
	{
		printf("err adc2 init (%d)\r\n", res);
		return;
	}
	#endif
	#endif

	// Init success
	bms_early_init_done = 1;

	//printf("bms_proc_hw_init ok\r\n");
}

void bms_proc_power_cleanup(void)
{
	GPIO_InitTypeDef  GPIO_InitStruct;

	GPIO_InitStruct.Mode  = GPIO_MODE_OUTPUT_PP;
	GPIO_InitStruct.Pull  = GPIO_PULLDOWN;				//GPIO_PULLUP;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;

	GPIO_InitStruct.Pin  = VCC_5V_ON;
	HAL_GPIO_Init(VCC_5V_ON_PORT, &GPIO_InitStruct);

	GPIO_InitStruct.Pin  = CHGR_ON;
	HAL_GPIO_Init(CHGR_ON_PORT, &GPIO_InitStruct);
	GPIO_InitStruct.Pin  = CC_CV;
	HAL_GPIO_Init(CC_CV_PORT, &GPIO_InitStruct);

	GPIO_InitStruct.Pin  =  BAL1_ON|BAL2_ON|BAL3_ON;
	HAL_GPIO_Init(BAL13_PORT, &GPIO_InitStruct);
	GPIO_InitStruct.Pin  = BAL4_ON;
	HAL_GPIO_Init(BAL4_PORT, &GPIO_InitStruct);

	// 5V OFF
	HAL_GPIO_WritePin(VCC_5V_ON_PORT, VCC_5V_ON, GPIO_PIN_RESET);

	// Charger off
	HAL_GPIO_WritePin(CHGR_ON_PORT, CHGR_ON, 0);
	HAL_GPIO_WritePin(CC_CV_PORT,   CC_CV,   0);

	// Balancer off
	HAL_GPIO_WritePin(BAL13_PORT, BAL1_ON|BAL2_ON|BAL3_ON, 	GPIO_PIN_RESET);
	HAL_GPIO_WritePin(BAL4_PORT,  BAL4_ON, 					GPIO_PIN_RESET);
}

static void bms_proc_measure_adc1(void)
{
	uchar res = 0;

	if(HAL_ADC_Start(&Adc1Handle) != HAL_OK)
	{
		printf("error sampling\r\n");
		return;
	}

	// Read Cell1
	if (HAL_ADC_PollForConversion(&Adc1Handle, ADC_POLLING_WAIT) != HAL_OK)
	{
		(bmss.e[0])++;
		goto stop_adc;
	}
	bmss.ulCH1 = HAL_ADC_GetValue(&Adc1Handle);

	// Read Cell1 + Cell2
	if (HAL_ADC_PollForConversion(&Adc1Handle, ADC_POLLING_WAIT) != HAL_OK)
	{
		(bmss.e[1])++;
		goto stop_adc;;
	}
	bmss.ulCH2 = HAL_ADC_GetValue(&Adc1Handle);

	// Read Cell1 + Cell2 + Cell3
	if (HAL_ADC_PollForConversion(&Adc1Handle, ADC_POLLING_WAIT) != HAL_OK)
	{
		(bmss.e[2])++;
		goto stop_adc;;
	}
	bmss.ulCH3 = HAL_ADC_GetValue(&Adc1Handle);

	// Read Cell1 + Cell2 + Cell3 + Cell4
	if (HAL_ADC_PollForConversion(&Adc1Handle, ADC_POLLING_WAIT) != HAL_OK)
	{
		(bmss.e[3])++;
		goto stop_adc;
	}
	bmss.ulCH4 = HAL_ADC_GetValue(&Adc1Handle);

	// Read Charge port
	if (HAL_ADC_PollForConversion(&Adc1Handle, ADC_POLLING_WAIT) != HAL_OK)
		goto stop_adc;

	ulong x = HAL_ADC_GetValue(&Adc1Handle);
	//printf("chgr: %d\r\n", x);
	bmss.ulCH5 = x;

	// Read Load Voltage
	if (HAL_ADC_PollForConversion(&Adc1Handle, ADC_POLLING_WAIT) != HAL_OK)
		goto stop_adc;

	bmss.ulCH6 = HAL_ADC_GetValue(&Adc1Handle);

	// Cell1 temperature NTC
	if (HAL_ADC_PollForConversion(&Adc1Handle, ADC_POLLING_WAIT) != HAL_OK)
		goto stop_adc;

	bmss.ulCH8 = HAL_ADC_GetValue(&Adc1Handle);

	// Cell2 temperature NTC
	if (HAL_ADC_PollForConversion(&Adc1Handle, ADC_POLLING_WAIT) != HAL_OK)
		goto stop_adc;

	bmss.ulCH9 = HAL_ADC_GetValue(&Adc1Handle);

	// Cell3 temperature NTC
	if (HAL_ADC_PollForConversion(&Adc1Handle, ADC_POLLING_WAIT) != HAL_OK)
		goto stop_adc;

	bmss.ulCH10 = HAL_ADC_GetValue(&Adc1Handle);

	// Cell4 temperature NTC
	if (HAL_ADC_PollForConversion(&Adc1Handle, ADC_POLLING_WAIT) != HAL_OK)
		goto stop_adc;

	bmss.ulCH11 = HAL_ADC_GetValue(&Adc1Handle);

	res = 1;

stop_adc:
	if(!res) printf("adc conv err\r\n");
	HAL_ADC_Stop(&Adc1Handle);
}

#if 0
static void bms_proc_measure_adc2(void)
{
	//if(HAL_ADC_Start(&Adc2Handle) != HAL_OK)
	//{
	//	printf("error sampling\r\n");
	//	return;
	//}

	// Read Charge port
	if (HAL_ADC_PollForConversion(&Adc2Handle, ADC_POLLING_WAIT) != HAL_OK) return;
	ulong x = HAL_ADC_GetValue(&Adc2Handle);
	//printf("curr: %d\r\n", x);
	bmss.ulCH5 = x;

	// Read Load Voltage
	//if (HAL_ADC_PollForConversion(&Adc1Handle, ADC_POLLING_WAIT) != HAL_OK) return;
	//ulCH6 += HAL_ADC_GetValue(&Adc1Handle);

	//HAL_ADC_Stop(&Adc2Handle);

	//numb_of_meas++;
}
#endif

#if 0
static void bms_proc_calc_average(void)
{
	bmss.ulCH1 /= numb_of_meas;
	bmss.ulCH2 /= numb_of_meas;
	bmss.ulCH3 /= numb_of_meas;
	bmss.ulCH4 /= numb_of_meas;
	bmss.ulCH5 /= numb_of_meas;
	bmss.ulCH6 /= numb_of_meas;

	bmss.ulCH8 /= numb_of_meas;
	bmss.ulCH9 /= numb_of_meas;
	bmss.ulCH10 /= numb_of_meas;
	bmss.ulCH11 /= numb_of_meas;

	//printf("curr: %d\r\n", ulCH5);
}

static void bms_proc_reset_accumulator(void)
{
	bmss.ulCH1 = 0;
	bmss.ulCH2 = 0;
	bmss.ulCH3 = 0;
	bmss.ulCH4 = 0;
	bmss.ulCH5 = 0;
	bmss.ulCH6 = 0;

	bmss.ulCH8 = 0;
	bmss.ulCH9 = 0;
	bmss.ulCH10 = 0;
	bmss.ulCH11 = 0;

	numb_of_meas = 0;

	for(int i = 0; i < 4; i++)		// reset error count
		bmss.e[0] = 0;

	//printf("-----------\r\n");
}
#endif

static void bms_proc_calc_votages(void)
{
	// ---------------------------------------------
	// CELL1
	bmss.ulCH1 *= bmss.vref;
	bmss.ulCH1 /= ADC_RESOLUTION;

	if(bmss.ulCH1 > ADC_MIN_VALUE)
		bmss.ulCH1 += bmss.cal_adc[0];	// adc mV calibration
	else
		bmss.ulCH1 = 0;					// ignore noise/missing cells

	bmss.a[0] = bmss.ulCH1;

	//printf("cal0 %d\r\n", bmss.cal_adc[0]);

	// Ideal divider 56k/100k, ratio 50/32
	//bmss.usCV[0]   = (bmss.ulCH1 * 5000)/(3200 + bmss.cal_adc[0]);
	bmss.usCV[0] = ROUND_DIVIDE((bmss.ulCH1 * 5000), (3200 + bmss.cal_adc[0]));

	bmss.s[0] = bmss.usCV[0];
	bmss.c[0] = bmss.usCV[0];	// branch voltage same as measured

	// ---------------------------------------------
	// CELL2
	bmss.ulCH2 *= bmss.vref;
	bmss.ulCH2 /= ADC_RESOLUTION;

	if(bmss.ulCH2 > ADC_MIN_VALUE)
		bmss.ulCH2 += bmss.cal_adc[1];	// adc mV calibration
	else
		bmss.ulCH2 = 0;					// ignore noise/missing cells

	bmss.a[1] = bmss.ulCH2;

	// Ideal divider 220k/100k, ratio 100/31
	//bmss.usCV[1]   = (bmss.ulCH2 * 10000)/(3100 + bmss.cal_adc[1]);
	bmss.usCV[1] = ROUND_DIVIDE((bmss.ulCH2 * 10000), (3100 + bmss.cal_adc[1]));

	bmss.s[1] = bmss.usCV[1];
	bmss.usCV[1]	 -= bmss.usCV[0];	// remove bottom cell
	bmss.c[1] = bmss.usCV[1];

	// ---------------------------------------------
	// CELL3
	bmss.ulCH3 *= bmss.vref;
	bmss.ulCH3 /= ADC_RESOLUTION;

	if(bmss.ulCH3 > ADC_MIN_VALUE)
		bmss.ulCH3 += bmss.cal_adc[2];	// adc mV calibration
	else
		bmss.ulCH3 = 0;					// ignore noise/missing cells

	bmss.a[2] = bmss.ulCH3;

	// Ideal divider 390k/100k, ratio 150/31
	//bmss.usCV[2]   = (bmss.ulCH3 * 15000)/(3100 + bmss.cal_adc[2]);
	bmss.usCV[2] = ROUND_DIVIDE((bmss.ulCH3 * 15000), (3100 + bmss.cal_adc[2]));

	bmss.s[2] = bmss.usCV[2];
	bmss.usCV[2]  -= (bmss.usCV[0] + bmss.usCV[1]);	// Remove bottom cells
	bmss.c[2] = bmss.usCV[2];

	// ---------------------------------------------
	// CELL4
	bmss.ulCH4 *= bmss.vref;
	bmss.ulCH4 /= ADC_RESOLUTION;

	if(bmss.ulCH4 > ADC_MIN_VALUE)
		bmss.ulCH4 += bmss.cal_adc[3];	// adc mV calibration
	else
		bmss.ulCH4 = 0;					// ignore noise/missing cells

	bmss.a[3] = bmss.ulCH4;

	// Ideal divider 560k/100k, ratio 200/30
	//--bmss.ulCH4  = (bmss.ulCH4 * 20000)/3000);
	bmss.ulCH4 = ROUND_DIVIDE((bmss.ulCH4 * 20000), 3030);

	bmss.usCV[3]	  = bmss.ulCH4;
	bmss.s[3] = bmss.usCV[3];
	bmss.usCV[3]	 -= (bmss.usCV[0] + bmss.usCV[1] + bmss.usCV[2]);	// Remove bottom cells
	bmss.c[3] = bmss.usCV[3];

	// ---------------------------------------------
	// CHRG
	//bmss.curr  = ((ulCH5 * 6600) / ADC_RESOLUTION) - 3300;
	//bmss.curr = -bmss.curr;
	//bmss.curr /= 2;

	//printf("curr: %d\r\n", bmss.curr);
	bmss.ulCH5 *= bmss.vref;
	bmss.ulCH5 /= ADC_RESOLUTION;

	if(bmss.ulCH5 > ADC_MIN_VALUE)
		bmss.ulCH5 += bmss.cal_adc[4];	// adc mV calibration
	else
		bmss.ulCH5 = 0;					// ignore noise/missing cells

	bmss.a[4] = bmss.ulCH5;

	// Ideal divider 560k/100k, ratio 200/30
	//bmss.usCV     	   = (bmss.ulCH5 * 20000)/(3000 + bmss.cal_adc[4]);
	bmss.usCV[4] = ROUND_DIVIDE((bmss.ulCH5 * 20000), (3030 + bmss.cal_adc[4]));
	bmss.chgr = bmss.usCV[4];
	bmss.usChargeValue = bmss.usCV[4];

	// ---------------------------------------------
	// LOAD
	//printf("load: %d\r\n", ((ulCH6 * 6600) / 0xFFFF) - 3300);
	bmss.ulCH6 *= bmss.vref;
	bmss.ulCH6 /= ADC_RESOLUTION;

	if(bmss.ulCH6 > ADC_MIN_VALUE)
		bmss.ulCH6 += bmss.cal_adc[5];	// adc mV calibration
	else
		bmss.ulCH6 = 0;					// ignore noise/missing cells

	bmss.a[5] = bmss.ulCH6;

	// Ideal divider 560k/100k, ratio 200/30
	//--bmss.bmss.usLoadValue = (bmss.ulCH6 * 20000)/3000);
	bmss.usLoadValue = ROUND_DIVIDE((bmss.ulCH6 * 20000), 3030);
	bmss.load = bmss.usLoadValue;

	if(!bmss.charger_on)
	{
		#if 0
		printf("C1: %dmV, C2: %dmV, C3:%dmV, C4: %dmV (%dmA)\r\n",
				bmss.usCV[0],
				bmss.usCV[1],
				bmss.usCV[2],
				bmss.usCV[3],
				(bmss.ulCH4 - bmss.usLoadValue));// numb_of_meas);(NS: %d)
		#endif

		bmss.curr = (bmss.ulCH4 - bmss.usLoadValue);
		//bmss.curr = bmss.usLoadValue;
	}
	else
	{
		#if 0
		// (Pot diff - diode voltage drop)/ Rsense
		int current = (bmss.usChargeValue - bmss.ulCH4 - 650)/1;

		printf("C1: %d mV, C2: %d mV, C3: %d mV, C4: %d mV (CH: %d mV, CR: %d mA, %d-%d)\r\n", 	bmss.usCV[0],
																								bmss.usCV[1],
																								bmss.usCV[2],
																								bmss.usCV[3],
																								bmss.usChargeValue,
																								current,
																								numb_of_meas,
																								bmss.h_prot_on);
		#endif
	}

	#if 0
	// Wrong math!
	printf("Battery voltage: C1: %d.%d V, C2: %d.%d V, C3: %d.%d V, C4: %d.%d V (%d samp)\r\n", bmss.usCV[0]/1000, (bmss.usCV[0]%1000)/10,
																								bmss.usCV[1]/1000, (bmss.usCV[1]%1000)/10,
																								bmss.usCV[2]/1000, (bmss.usCV[2]%1000)/10,
																								bmss.usCV[3]/1000, (bmss.usCV[3]%1000)/10,
																								numb_of_meas);
	#endif

	bmss.t_err  = bmss.e[0] << 24;
	bmss.t_err |= bmss.e[1] << 16;
	bmss.t_err |= bmss.e[2] <<  8;
	bmss.t_err |= bmss.e[3] <<  0;

	if(bmss.t_err)
		printf("accum error: %d\r\n", bmss.t_err);

	// NTC voltages (debug)
	#if 0
	printf("T1: %d, T2: %d, T3: %d, T4: %d\r\n",
			((ulTx[0] * bmss.vref)/ADC_RESOLUTION),
			((ulTx[1] * bmss.vref)/ADC_RESOLUTION),
			((ulTx[2] * bmss.vref)/ADC_RESOLUTION),
			((ulTx[3] * bmss.vref)/ADC_RESOLUTION));
	#endif

	//int tt1,tt2,tt3,tt4;
	float t1;

	// ADC mV calibration
	bmss.ulCH8  += bmss.cal_adc[6];
	bmss.ulCH9  += bmss.cal_adc[7];
	bmss.ulCH10 += bmss.cal_adc[8];
	bmss.ulCH11 += bmss.cal_adc[9];

	bmss.a[6] = (bmss.ulCH8  * bmss.vref)/ADC_RESOLUTION;
	bmss.a[7] = (bmss.ulCH9  * bmss.vref)/ADC_RESOLUTION;
	bmss.a[8] = (bmss.ulCH10 * bmss.vref)/ADC_RESOLUTION;
	bmss.a[9] = (bmss.ulCH11 * bmss.vref)/ADC_RESOLUTION;

	t1 = bms_proc_steinhart_hart(bmss.ulCH8);
	bmss.t[0] = (ulong)(t1*100);

	t1 = bms_proc_steinhart_hart(bmss.ulCH9);
	bmss.t[1] = (ulong)(t1*100);

	t1 = bms_proc_steinhart_hart(bmss.ulCH10);
	bmss.t[2] = (ulong)(t1*100);

	t1 = bms_proc_steinhart_hart(bmss.ulCH11);
	bmss.t[3] = (ulong)(t1*100);

	// NTC as temperature
	#if 0
	printf("T1: %2d.%02dC, T2: %2d.%02dC, T3: %2d.%02dC, T4: %2d.%02dC\r\n",
			bmss.t[0]/100, bmss.t[0]%100,
			bmss.t[1]/100, bmss.t[1]%100,
			bmss.t[2]/100, bmss.t[2]%100,
			bmss.t[3]/100, bmss.t[3]%100);
	#endif

	// Notify UI
	bmss.rr = 1;
}

// Simple SOC tracking base on average
// pack voltage. ToDo: Coloumb counting
//
//  2600 -  4200, per cell safe range
// 10400 - 16800, pack range 6400 mV, 1% is 64 mV
//
static void bms_proc_calc_perc(void)
{
	static ulong aver_load = 0;
	static uchar load_acc = 0;

	// If missing cell, do not calculate percentage
	if((bmss.ulCH1 == 0)||(bmss.ulCH2 == 0)||(bmss.ulCH3 == 0)||(bmss.ulCH4 == 0))
	{
		bmss.perc = 100;
		return;
	}

	// Average to prevent UI control updating too often
	aver_load += bmss.load;
	load_acc++;

	if(load_acc < 10)
		return;

	aver_load /= 10;
	//printf("load %dmV\r\n", aver_load);

	aver_load -= 10400;			// minus base level
	bmss.perc  = aver_load/64;	// to %

	if(bmss.perc > 100)
		bmss.perc = 100;

	//printf("SOC %d%%\r\n", bmss.perc);

	// Reset accum
	load_acc  = 0;
	aver_load = 0;
}

#if 0
static void bms_proc_decide_balancer(void)
{
	static ulong max_val  = 0;
	static ulong bal_skip = 0;
	int i, j = 0;

	// No balancing on DC IN power
	if(bmss.run_on_dc)
	{
		// Balancer off
		if(HAL_GPIO_ReadPin(BAL13_PORT, BAL1_ON) == GPIO_PIN_SET)
		{
			printf("running on DC, will turn balancer off...\r\n");

			// Clear public flags
			for(i = 0; i < 4; i++)
				bmss.usBalID[i] = 0;

			// Route backlight to VCC_3V
			HAL_GPIO_WritePin(BAL13_PORT, BAL1_ON, GPIO_PIN_RESET);
		}

		return;
	}

	// Not too often
	bal_skip++;
	if(bal_skip < BALANCER_FREQ)
		return;

	bal_skip = 0;

	// Find the highest voltage cell in order to run the balancer
	for(i = 0; i < 4; i++)
	{
		if((bmss.c[i] + 0) > max_val)
		{
			max_val = bmss.c[i];				// save highest
			j = i;								// save index
		}
	}
	printf("highest is CELL%d = %d mV\r\n", (j + 1), max_val);

	// Already on, skip
	if(bmss.usBalID[j] != 0)
	{
		printf("already set to BAL CELL%d\r\n", (j + 1));
		return;
	}

	// Clear public flags
	for(i = 0; i < 4; i++)
		bmss.usBalID[i] = 0;

	printf("BAL CELL%d\r\n", (j + 1));

	// BAL1_ON signal needs to be on all the time (K5 on)
	HAL_GPIO_WritePin(BAL13_PORT, BAL1_ON, GPIO_PIN_SET);

	// Balancer on
	switch(j)
	{
		case 0:
		{
			// All other off
			HAL_GPIO_WritePin(BAL13_PORT, BAL2_ON|BAL3_ON, 	GPIO_PIN_RESET);
			HAL_GPIO_WritePin(BAL4_PORT,  BAL4_ON, 			GPIO_PIN_RESET);

			bmss.usBalID[0] = 1;
	   		break;
		}

		case 1:
		{
			// All other off
			HAL_GPIO_WritePin(BAL13_PORT, BAL3_ON, 			GPIO_PIN_RESET);
			HAL_GPIO_WritePin(BAL4_PORT,  BAL4_ON, 			GPIO_PIN_RESET);

			// K6 on
			HAL_GPIO_WritePin(BAL13_PORT, BAL2_ON, 			GPIO_PIN_SET);

			bmss.usBalID[1] = 1;
	   		break;
		}

		case 2:
		{
			// Save power on K6, route power to CELL3
			HAL_GPIO_WritePin(BAL13_PORT, BAL2_ON|BAL3_ON, 	GPIO_PIN_RESET);

			// K8 on (route to top cells)
			HAL_GPIO_WritePin(BAL4_PORT,  BAL4_ON, 			GPIO_PIN_SET);

			bmss.usBalID[2] = 1;
	   		break;
		}

	   	case 3:
	   	{
			// Save power
			HAL_GPIO_WritePin(BAL13_PORT, BAL2_ON, 			GPIO_PIN_RESET);

			// K8 on (route to top cells)
			HAL_GPIO_WritePin(BAL4_PORT,  BAL4_ON, 			GPIO_PIN_SET);

			// K7 on
			HAL_GPIO_WritePin(BAL13_PORT, BAL3_ON, 			GPIO_PIN_SET);

			bmss.usBalID[3] = 1;
	   		break;
	   	}

	   	default:
	   		break;
	}

	// Get min value
	//max_val = 0;
	for(i = 0; i < 4; i++)
	{
		if(bmss.c[i] < max_val)
			max_val = bmss.c[i];
	}
	//printf("min val = %d mV\r\n", max_val);
}
#else
static void bms_proc_decide_balancer(void)
{
	ulong max_val  = 0;
	static ulong bal_skip = 0;
	int i, j = 0;

	// No balancing on DC IN power
	if(bmss.run_on_dc)
	{
		// Balancer off
		if(HAL_GPIO_ReadPin(BAL13_PORT, BAL1_ON) == GPIO_PIN_SET)
		{
			printf("running on DC, will turn balancer off...\r\n");

			// Clear public flags
			for(i = 0; i < 4; i++)
				bmss.usBalID[i] = 0;

			// Route backlight to VCC_3V
			HAL_GPIO_WritePin(BAL13_PORT, BAL1_ON, GPIO_PIN_RESET);
		}

		return;
	}

	// Accumulate
	bal_skip++;
	if(bal_skip < BALANCER_FREQ)
	{
		for(i = 0; i < 4; i++)
			bmss.ba[i] += bmss.c[i];

		return;
	}
	bal_skip = 0;

	// Average
	for(i = 0; i < 4; i++)
		bmss.ba[i] /= BALANCER_FREQ;

	// Find the highest voltage cell in order to run the balancer
	for(i = 0; i < 4; i++)
	{
		if(bmss.ba[i] > max_val)
		{
			max_val = bmss.ba[i];				// save highest
			j = i;								// save index
		}
	}

	// Debug
	#if 1
	printf("-------------------------------------\r\n");
	for(i = 0; i < 4; i++)
	{
		if(i == j)
			printf("  C%d = %d mV <---           \r\n", i, (int)bmss.ba[i]);
		else
			printf("  C%d = %d mV                \r\n", i, (int)bmss.ba[i]);
	}
	#else
	printf("== highest is C%d = %d mV ==\r\n", (int)(j + 1), (int)max_val);
	#endif

	// Clear accumulators
	for(i = 0; i < 4; i++)
		bmss.ba[i] = 0;

	// Already on, skip
	if(bmss.usBalID[j] != 0)
	{
		printf(" already C%d\r\n", (j + 1));
		return;
	}
	else
	{
		printf(" use C%d\r\n", (j + 1));
	}

	// Clear public flags
	for(i = 0; i < 4; i++)
		bmss.usBalID[i] = 0;

	// BAL1_ON signal needs to be on all the time (K5 on)
	HAL_GPIO_WritePin(BAL13_PORT, BAL1_ON, GPIO_PIN_SET);

	// Balancer on
	switch(j)
	{
		case 0:
		{
			// All other off
			HAL_GPIO_WritePin(BAL13_PORT, BAL2_ON|BAL3_ON, 	GPIO_PIN_RESET);
			HAL_GPIO_WritePin(BAL4_PORT,  BAL4_ON, 			GPIO_PIN_RESET);

			bmss.usBalID[0] = 1;
	   		break;
		}

		case 1:
		{
			// All other off
			HAL_GPIO_WritePin(BAL13_PORT, BAL3_ON, 			GPIO_PIN_RESET);
			HAL_GPIO_WritePin(BAL4_PORT,  BAL4_ON, 			GPIO_PIN_RESET);

			// K6 on
			HAL_GPIO_WritePin(BAL13_PORT, BAL2_ON, 			GPIO_PIN_SET);

			bmss.usBalID[1] = 1;
	   		break;
		}

		case 2:
		{
			// Save power on K6, route power to CELL3
			HAL_GPIO_WritePin(BAL13_PORT, BAL2_ON|BAL3_ON, 	GPIO_PIN_RESET);

			// K8 on (route to top cells)
			HAL_GPIO_WritePin(BAL4_PORT,  BAL4_ON, 			GPIO_PIN_SET);

			bmss.usBalID[2] = 1;
	   		break;
		}

	   	case 3:
	   	{
			// Save power
			HAL_GPIO_WritePin(BAL13_PORT, BAL2_ON, 			GPIO_PIN_RESET);

			// K8 on (route to top cells)
			HAL_GPIO_WritePin(BAL4_PORT,  BAL4_ON, 			GPIO_PIN_SET);

			// K7 on
			HAL_GPIO_WritePin(BAL13_PORT, BAL3_ON, 			GPIO_PIN_SET);

			bmss.usBalID[3] = 1;
	   		break;
	   	}

	   	default:
	   		break;
	}
}
#endif

// Do we have external power plugged in ?
static int bms_proc_handle_charger(void)
{
	// Are we running on batteries or DC input ?
	if(bmss.usChargeValue > bmss.load)
		bmss.run_on_dc  = 1;
	else
		bmss.run_on_dc  = 0;

	//printf("Charger value: %d mV\r\n",bmss.usChargeValue);
	if(bmss.usChargeValue > CHARGER_MIN_VOLTAGE)
	{
		//printf("Charger voltage: %d mV(prot=%d)\r\n",bmss.usChargeValue, bmss.h_prot_on);

		if(bmss.h_prot_on)
			goto ch_off;

		// ToDo: implement check for charger max voltage !

		//HAL_GPIO_WritePin(GPIOE, GPIO_PIN_11, 1);	// Red led on(Charge)
//		proc_refresh_rate = CHARGE_PROC_REFRESH;	// 500 mS while charging
//		sampling_skip     = 0;						// Reset counter to prevent overflow

//		HAL_GPIO_WritePin(GPIOA,GPIO_PIN_12, 1); 	// CH on

		bmss.charger_on = 1;
		return 1;
	}

ch_off:

	//HAL_GPIO_WritePin(GPIOE, GPIO_PIN_11, 0);	// Red led off(Charge)
//	proc_refresh_rate = NORMAL_PROC_REFRESH;	// 4s normal operation
//	sampling_skip     = 0;						// Reset counter to prevent overflow

//	HAL_GPIO_WritePin(GPIOA,GPIO_PIN_12, 0); 	// CH off

	// Reset charger state
	bmss.charger_on = 0;
	bmss.h_prot_on  = 0;

	return 0;
}

static void bms_proc_low_voltage_protection(void)
{
	static uchar power_off_count = 0;

	if((bmss.usCV[0] < CELL_MIN_VALUE)||(bmss.usCV[1] < CELL_MIN_VALUE)||(bmss.usCV[2] < CELL_MIN_VALUE)||(bmss.usCV[3] < CELL_MIN_VALUE))
	{
		#if 0
		printf("cell1: %d mV\r\n", bmss.usCV[0]);
		printf("cell2: %d mV\r\n", bmss.usCV[1]);
		printf("cell3: %d mV\r\n", bmss.usCV[2]);
		printf("cell4: %d mV\r\n", bmss.usCV[3]);
		#endif

		power_off_count++;
		if(power_off_count < 10)
			return;

		printf("low voltage protection kicked in, will power off, bye!\r\n");
		vTaskDelay(200);

		//HAL_GPIO_WritePin(GPIOE,GPIO_PIN_10, 0); 	// led off
		//HAL_GPIO_WritePin(GPIOD,GPIO_PIN_8,  1);	// Power off

		power_off();
	}
	else
		power_off_count = 0; // reset accumulator
}

static void bms_proc_high_voltage_protection(void)
{
	//printf("A1: %d mV, A2: %d mV, A3: %d mV, A4: %d mV\r\n", bmss.usCV[0], bmss.usCV[1], bmss.usCV[2],bmss.usCV[3]);

	if((bmss.usCV[0] > CELL_MAX_VALUE)||(bmss.usCV[1] > CELL_MAX_VALUE)||(bmss.usCV[2] > CELL_MAX_VALUE)||(bmss.usCV[3] > CELL_MAX_VALUE))
	{
		printf("high voltage protection kicked in, will turn off charging!\r\n");
		vTaskDelay(200);

		bmss.h_prot_on = 1;	// how to prevent oscillaition ?, ToDo: implement trickle charging ?

		// Turn off charging
		bms_proc_handle_charger();
	}
}

// current BMS resolution is 500 mS!!
//
static void bms_proc_power_off(void)
{
	static uchar wait_radio_boot_up = 0;

	// Wait for full bootup, before allowing power off
	if(wait_radio_boot_up < 10)
	{
		wait_radio_boot_up++;
		return;
	}

	// Check for power off
	//
	// - power off is feeding VBAT voltage from Cell1
	//   into PC13(WKUP2). As part of D3 domain, it needs
	//   special init to use as GPIO
	//
	// 		== fixed by swapping PG11 and PC13! ==
	//
   	if(HAL_GPIO_ReadPin(POWER_BUTTON_PORT, POWER_BUTTON) == 1)
   	{
   		vTaskDelay(2000);// debounce
   		if(HAL_GPIO_ReadPin(POWER_BUTTON_PORT, POWER_BUTTON) == 1)
   		{
   			printf("user held button, will power off, bye!\r\n");

   			// Power off process
   			power_off();
   		}
   	}
}

//*----------------------------------------------------------------------------
//* Function Name       : bms_proc_worker
//* Object              :
//* Notes    			:
//* Notes   			:
//* Notes    			:
//* Context    			: CONTEXT_BMS
//*----------------------------------------------------------------------------
static void bms_proc_worker(void)
{
	#ifndef USE_DMA
	// Handle power off
	bms_proc_power_off();

	// Check for init success
	if(!bms_early_init_done)
		return;

	#ifdef USE_BALANCER
	// Balancer process
	bms_proc_exec_balancer();
	#endif

	// Do ADC conversion
	bms_proc_measure_adc1();
	//bms_proc_measure_adc2();
	bms_proc_calc_votages();
	//bms_proc_calc_filtered();
	bms_proc_calc_perc();

	// Handle charging
	//
	// ToDo: move charger detection in actual bms_proc_measure() for faster reaction
	//       and then we can have slower handling in the worker
	//
	bms_proc_handle_charger();

	if(bmss.run_on_dc == 0)
	{
		// Low voltage protection
		bms_proc_low_voltage_protection();
	}
	//else
	//{
		// High voltage protection
	//	bms_proc_high_voltage_protection();
	//}

	// ---------------------------------------------------------------------------------------------
	// ---------------------------------------------------------------------------------------------
	// ToDo: Remove this channel
	//int delta = (int)(ulCH4 - bmss.usLoadValue);
	//printf("Battery/Load: %d/%d mV, delta: %d mV\r\n",ulCH4, bmss.usLoadValue, delta);
	// ---------------------------------------------------------------------------------------------
	// ---------------------------------------------------------------------------------------------

	//#ifdef USE_BALANCER
	// Calculate balancer params
	bms_proc_decide_balancer();
	//#endif

	//HAL_PWREx_EnableMonitoring();
	//printf("Battery %d,Temp %d\r\n", HAL_PWREx_GetVBATLevel(), HAL_PWREx_GetTemperatureLevel());
	//HAL_PWREx_DisableMonitoring();
	#else

	// Check for init success
	if(!bms_early_init_done)
		return;

	printf("count %d\r\n", a_val);
	HAL_ADC_Start_DMA(&Adc1Handle, (uint32_t *)aADCxConvertedData, 1);

	#endif
}

//*----------------------------------------------------------------------------
//* Function Name       : bms_proc_task
//* Object              :
//* Notes    			: main process
//* Notes   			:
//* Notes    			:
//* Context    			: CONTEXT_BMS
//*----------------------------------------------------------------------------
void bms_proc_task(void const *arg)
{
	vTaskDelay(BMS_PROC_START_DELAY);
	//printf("bms process start\r\n");

	// Clear calibration
	for(int i = 0; i < 10; i++)
	{
		bmss.cal_adc[i] = 0;
		bmss.cal_res[i] = 0;

		if(i < 4)
			bmss.ba[i] = 0;
	}

	bmss.ulCH1 = 0;
	bmss.ulCH2 = 0;
	bmss.ulCH3 = 0;
	bmss.ulCH4 = 0;
	bmss.ulCH5 = 0;
	bmss.ulCH6 = 0;
	bmss.ulCH8 = 0;
	bmss.ulCH9 = 0;
	bmss.ulCH10 = 0;
	bmss.ulCH11 = 0;

	bmss.charger_on = 0;
	bmss.h_prot_on  = 0;
	bmss.run_on_dc  = 0;

	// Manual calibration, compensate for divider resistors tolerance
	//
	// ----------------------------
	// cell1
	bmss.cal_adc[0] = -60;
	//
	// ----------------------------
	// cell2
	bmss.cal_adc[1] = -60;
	//
	// ----------------------------
	// cell3
	bmss.cal_adc[2] = -60;
	//
	// ----------------------------
	// cell4
	bmss.cal_adc[3] = -60;
	//
	// ----------------------------
	// charger
	bmss.cal_adc[4] = -60;
	//
	// ----------------------------
	// load
	bmss.cal_adc[5] = -60;
	// ----------------------------
	// temp
	bmss.cal_adc[6] = -50;
	bmss.cal_adc[7] = -50;
	bmss.cal_adc[8] = -50;
	bmss.cal_adc[9] = -50;
	// ----------------------------
	//
	bmss.vref = ADC_EXT_VREF_mV;

	#ifdef USE_DMA
	int res = bms_proc_adc1_init();
	if(res != 0)
	{
		printf("err adc1 init (%d)\r\n", res);
	}
	#endif

bms_proc_loop:

	// Process
	bms_proc_worker();

	vTaskDelay(BMS_PROC_SLEEP_TIME);
	goto bms_proc_loop;
}

#endif




