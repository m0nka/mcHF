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

#ifdef LL_ADC_USE_IRQ
ushort ch1;
ushort ch3;
ushort ch6;
ushort ch8;
uchar samp_done = 0;
#endif

#ifdef LL_ADC_USE_DMA
__attribute__((section("dma_mem"))) __attribute__ ((aligned (32))) ushort dma_rx_buffer[10];
//__attribute__((section("heap_mem"))) __attribute__ ((aligned (32))) ushort dma_rx_buffer[2];
#endif

ushort 	ADC_Val[NUMBER_OF_ADC3_CHANNELS + 1];
uchar	adc_init_done = 0;

#ifndef LL_ADC_USE_BDMA
static void adc_configure(void)
{
	#ifdef LL_ADC_USE_DMA
	LL_DMA_InitTypeDef       ADC_Config_DMA;
	#endif

	// -----------------------------------------------------------------
	// ADC3_INP1, Reflected Power (PC3)
	LL_GPIO_SetPinMode(ADC3_INP1_PORT, ADC3_INP1, LL_GPIO_MODE_ANALOG);

	// -----------------------------------------------------------------
	// ADC3_INP8, Ambient light sensor (PF6)
	LL_GPIO_SetPinMode(POWER_LED_PORT, POWER_LED, LL_GPIO_MODE_ANALOG);

	// -----------------------------------------------------------------
	// ADC3_INP3, PA Temperature (PF7)
	LL_GPIO_SetPinMode(ADC3_INP3_PORT, ADC3_INP3, LL_GPIO_MODE_ANALOG);

	// -----------------------------------------------------------------
	// ADC3_INP6, Forward Power (PF10)
	LL_GPIO_SetPinMode(ADC3_INP6_PORT, ADC3_INP6, LL_GPIO_MODE_ANALOG);

	LL_RCC_SetADCClockSource(LL_RCC_ADC_CLKSOURCE_PLL2P); // PLL init in main.c
	LL_AHB4_GRP1_EnableClock(LL_AHB4_GRP1_PERIPH_ADC3);

	#ifndef LL_ADC_USE_DMA
	//LL_ADC_SetCommonClock	(__LL_ADC_COMMON_INSTANCE(ADC3), LL_ADC_CLOCK_ASYNC_DIV4);
	#else
	#if 0
	#else
	LL_ADC_SetCommonClock	(__LL_ADC_COMMON_INSTANCE(ADC3), LL_ADC_CLOCK_ASYNC_DIV8);
	#endif
	//
	LL_AHB1_GRP1_EnableClock(LL_AHB1_GRP1_PERIPH_DMA1);
	//
	ADC_Config_DMA.Direction 				= LL_DMA_DIRECTION_PERIPH_TO_MEMORY;
	ADC_Config_DMA.FIFOMode 				= LL_DMA_FIFOMODE_DISABLE;
	ADC_Config_DMA.MemBurst 				= LL_DMA_MBURST_SINGLE;
	ADC_Config_DMA.MemoryOrM2MDstAddress 	= (uint32_t)dma_rx_buffer;
	ADC_Config_DMA.MemoryOrM2MDstDataSize 	= LL_DMA_MDATAALIGN_HALFWORD;
	ADC_Config_DMA.MemoryOrM2MDstIncMode 	= LL_DMA_MEMORY_INCREMENT;
	ADC_Config_DMA.Mode 					= LL_DMA_MODE_CIRCULAR;
	ADC_Config_DMA.NbData 					= NUMBER_OF_ADC3_CHANNELS;
	ADC_Config_DMA.PeriphBurst 				= LL_DMA_PBURST_SINGLE;
	ADC_Config_DMA.PeriphOrM2MSrcAddress 	= (uint32_t)&ADC3->DR;
	ADC_Config_DMA.PeriphOrM2MSrcDataSize 	= LL_DMA_PDATAALIGN_HALFWORD;
	ADC_Config_DMA.PeriphOrM2MSrcIncMode 	= LL_DMA_PERIPH_NOINCREMENT;
	ADC_Config_DMA.PeriphRequest 			= LL_DMAMUX1_REQ_ADC3;
	ADC_Config_DMA.Priority 				= LL_DMA_PRIORITY_LOW;

	LL_DMA_Init(DMA1, LL_DMA_STREAM_1, &ADC_Config_DMA);

	LL_ADC_SetMultiDMATransfer(__LL_ADC_COMMON_INSTANCE(ADC3), LL_ADC_MULTI_REG_DMA_EACH_ADC);
	#endif

	//LL_SYSCFG_OpenAnalogSwitch(LL_SYSCFG_ANALOG_SWITCH_PC2|LL_SYSCFG_ANALOG_SWITCH_PC3);

	LL_ADC_SetMultimode				(__LL_ADC_COMMON_INSTANCE(ADC3), LL_ADC_MULTI_INDEPENDENT);
	LL_ADC_SetResolution			(ADC3, LL_ADC_RESOLUTION_16B);
	LL_ADC_REG_SetTriggerSource		(ADC3, LL_ADC_REG_TRIG_SOFTWARE);
	LL_ADC_REG_SetOverrun			(ADC3, LL_ADC_REG_OVR_DATA_OVERWRITTEN);
	LL_ADC_REG_SetContinuousMode	(ADC3, LL_ADC_REG_CONV_SINGLE);

	#if (NUMBER_OF_ADC3_CHANNELS == 1)
	LL_ADC_REG_SetSequencerLength	(ADC3, LL_ADC_REG_SEQ_SCAN_DISABLE);
	#endif

	#ifdef LL_ADC_USE_DMA
	LL_ADC_REG_SetContinuousMode	(ADC3, LL_ADC_REG_CONV_CONTINUOUS);
	#endif

	// Connect internal channels
	LL_ADC_SetCommonPathInternalCh(__LL_ADC_COMMON_INSTANCE(ADC3),LL_ADC_PATH_INTERNAL_VREFINT|\
																  LL_ADC_PATH_INTERNAL_TEMPSENSOR|\
																  LL_ADC_PATH_INTERNAL_VBAT);

	// Connect channels on portF
	//LL_ADC_SetChannelPreSelection(ADC3, LL_ADC_CHANNEL_3);
	LL_ADC_SetChannelPreSelection(ADC3, LL_ADC_CHANNEL_6);
	//LL_ADC_SetChannelPreSelection(ADC3, LL_ADC_CHANNEL_8);
	//LL_ADC_SetChannelPreSelection(ADC3, LL_ADC_CHANNEL_1);
	//LL_ADC_SetChannelPreSelection(ADC3, LL_ADC_CHANNEL_VREFINT);
	//LL_ADC_SetChannelPreSelection(ADC3, LL_ADC_CHANNEL_VBAT);
	//LL_ADC_SetChannelPreSelection(ADC3, LL_ADC_CHANNEL_TEMPSENSOR);

	// VREF internal channel
	LL_ADC_SetChannelSamplingTime	(ADC3, LL_ADC_CHANNEL_VREFINT, ADC_SAMP_TIME);
	LL_ADC_REG_SetSequencerRanks	(ADC3, LL_ADC_REG_RANK_1, LL_ADC_CHANNEL_VREFINT);

	// CH1, Reflected Power (PC3)
	#if (NUMBER_OF_ADC3_CHANNELS > 1)
	LL_ADC_SetChannelSingleDiff		(ADC3, LL_ADC_CHANNEL_1, LL_ADC_SINGLE_ENDED);
	LL_ADC_SetChannelSamplingTime	(ADC3, LL_ADC_CHANNEL_1, ADC_SAMP_TIME);
	LL_ADC_REG_SetSequencerRanks	(ADC3, LL_ADC_REG_RANK_2, LL_ADC_CHANNEL_1);
	LL_ADC_REG_SetSequencerLength	(ADC3, LL_ADC_REG_SEQ_SCAN_ENABLE_2RANKS);
	#endif

	// VBAT internal channel
	#if (NUMBER_OF_ADC3_CHANNELS > 2)
	LL_ADC_SetChannelSamplingTime	(ADC3, LL_ADC_CHANNEL_VBAT, ADC_SAMP_TIME);
	LL_ADC_REG_SetSequencerRanks	(ADC3, LL_ADC_REG_RANK_3, LL_ADC_CHANNEL_VBAT);
	LL_ADC_REG_SetSequencerLength	(ADC3, LL_ADC_REG_SEQ_SCAN_ENABLE_3RANKS);
	#endif

	// Temp internal channel
	#if (NUMBER_OF_ADC3_CHANNELS > 3)
	LL_ADC_SetChannelSamplingTime	(ADC3, LL_ADC_CHANNEL_TEMPSENSOR, ADC_SAMP_TIME);
	LL_ADC_REG_SetSequencerRanks	(ADC3, LL_ADC_REG_RANK_4, LL_ADC_CHANNEL_TEMPSENSOR);
	LL_ADC_REG_SetSequencerLength	(ADC3, LL_ADC_REG_SEQ_SCAN_ENABLE_4RANKS);
	#endif

	// CH6, Forward Power (PF10)
	#if (NUMBER_OF_ADC3_CHANNELS > 4)
	LL_ADC_SetChannelSingleDiff		(ADC3, LL_ADC_CHANNEL_6, LL_ADC_SINGLE_ENDED);
	LL_ADC_SetChannelSamplingTime	(ADC3, LL_ADC_CHANNEL_6, ADC_SAMP_TIME);
	LL_ADC_REG_SetSequencerRanks	(ADC3, LL_ADC_REG_RANK_5,LL_ADC_CHANNEL_6);
	LL_ADC_REG_SetSequencerLength	(ADC3, LL_ADC_REG_SEQ_SCAN_ENABLE_5RANKS);
	#endif

	// CH3, PA Temperature (PF7)
	#if (NUMBER_OF_ADC3_CHANNELS > 5)
	LL_ADC_SetChannelSingleDiff		(ADC3, LL_ADC_CHANNEL_3, LL_ADC_SINGLE_ENDED);
	LL_ADC_SetChannelSamplingTime	(ADC3, LL_ADC_CHANNEL_3, ADC_SAMP_TIME);
	LL_ADC_REG_SetSequencerRanks	(ADC3, LL_ADC_REG_RANK_6, LL_ADC_CHANNEL_3);
	LL_ADC_REG_SetSequencerLength	(ADC3, LL_ADC_REG_SEQ_SCAN_ENABLE_6RANKS);
	#endif

	// CH8, Ambient light sensor (PF6)
	#if (NUMBER_OF_ADC3_CHANNELS > 6)
	LL_ADC_SetChannelSingleDiff		(ADC3, LL_ADC_CHANNEL_8, LL_ADC_SINGLE_ENDED);
	LL_ADC_SetChannelSamplingTime	(ADC3, LL_ADC_CHANNEL_8, ADC_SAMP_TIME);
	LL_ADC_REG_SetSequencerRanks	(ADC3, LL_ADC_REG_RANK_7, LL_ADC_CHANNEL_8);
	LL_ADC_REG_SetSequencerLength	(ADC3, LL_ADC_REG_SEQ_SCAN_ENABLE_7RANKS);
	#endif

	#ifdef LL_ADC_USE_IRQ
	NVIC_SetPriority(ADC3_IRQn, 3);

	// Set correct IRQ
	LL_ADC_EnableIT_EOC(ADC3);
	//LL_ADC_EnableIT_EOSMP(ADC3);
	LL_ADC_EnableIT_OVR(ADC3);
	#endif

	printf("adc conf \r\n");
}
#endif

static void adc_activate(void)
{
	__IO uint32_t wait_loop_index = 0U;

	if(LL_ADC_IsEnabled(ADC3) == 0)
	{
		LL_ADC_DisableDeepPowerDown(ADC3);
		LL_ADC_EnableInternalRegulator(ADC3);

		wait_loop_index = ((LL_ADC_DELAY_INTERNAL_REGUL_STAB_US * (SystemCoreClock / (100000 * 2))) / 10);
		while(wait_loop_index != 0)
		{
			wait_loop_index--;
		}

		// Run ADC self calibration
		LL_ADC_StartCalibration(ADC3, LL_ADC_CALIB_OFFSET, LL_ADC_SINGLE_ENDED);
		//LL_ADC_StartCalibration(ADC3, LL_ADC_CALIB_OFFSET_LINEARITY, LL_ADC_SINGLE_ENDED);

		while(LL_ADC_IsCalibrationOnGoing(ADC3) != 0);

		wait_loop_index = (ADC_DELAY_CALIB_ENABLE_CPU_CYCLES >> 1);
		while(wait_loop_index != 0)
		{
			wait_loop_index--;
		}

		LL_ADC_Enable(ADC3);
		while(LL_ADC_IsActiveFlag_ADRDY(ADC3) == 0);

		printf("adc activated \r\n");
	}
}

#if 0
static void adc_switch_channel(uchar id)
{
	LL_ADC_Disable(ADC3);

	if(id == 0)
	{
		// VREF internal channel
		LL_ADC_SetCommonPathInternalCh	(__LL_ADC_COMMON_INSTANCE(ADC3), LL_ADC_PATH_INTERNAL_VREFINT);
		LL_ADC_SetChannelSamplingTime	(ADC3, LL_ADC_CHANNEL_VREFINT, LL_ADC_SAMPLINGTIME_64CYCLES_5);
		LL_ADC_REG_SetSequencerRanks	(ADC3, LL_ADC_REG_RANK_1, LL_ADC_CHANNEL_VBAT);
	}
	else
	{
		// VBAT internal channel
		LL_ADC_SetCommonPathInternalCh	(__LL_ADC_COMMON_INSTANCE(ADC3), LL_ADC_PATH_INTERNAL_VBAT);
		LL_ADC_SetChannelSamplingTime	(ADC3, LL_ADC_CHANNEL_VBAT, LL_ADC_SAMPLINGTIME_64CYCLES_5);
		LL_ADC_REG_SetSequencerRanks	(ADC3, LL_ADC_REG_RANK_1, LL_ADC_CHANNEL_VBAT);
	}

	LL_ADC_Enable(ADC3);
	while(LL_ADC_IsActiveFlag_ADRDY(ADC3) == 0);
}
#endif

static void adc_proc_task(void *arg)
{
	vTaskDelay(4000);
	printf("adc sampling start \r\n");

	#ifdef LL_ADC_USE_IRQ
	NVIC_EnableIRQ	(ADC3_IRQn);
	#endif

	#ifdef LL_ADC_USE_DMA
	LL_DMA_EnableStream(DMA1, LL_DMA_STREAM_1);
	#endif

	#ifdef LL_ADC_USE_BDMA
	//--LL_ADC_REG_StartConversion(ADC3);
	#endif

	 adc_init_done = 1;

	for(;;)
	{
		vTaskDelay(100);

		#ifdef LL_ADC_USE_IRQ
		if(samp_done)
		{
			//printf("ch1: %d, ch3: %d, ch6: %d, ch8: %d\r\n", ch1, ch3, ch6, ch8);

			ushort ch1v =  __LL_ADC_CALC_DATA_TO_VOLTAGE(VDDA_APPLI, ch1, LL_ADC_RESOLUTION_16B);
			//ushort ch3v =  __LL_ADC_CALC_DATA_TO_VOLTAGE(VDDA_APPLI, ch3, LL_ADC_RESOLUTION_16B);
			printf("ch1: %d.%dV\r\n",ch1v/1000, ch1v%1000);
			//printf("ch1: %dmV(%d), ch3: %dmV(%d)\r\n", ch1v, ch1, ch3v, ch3);

			LL_ADC_REG_StartConversion(ADC3);
			samp_done = 0;
		}
		#endif

		#ifdef LL_ADC_USE_DMA

		SCB_InvalidateDCache_by_Addr((uint32_t *)dma_rx_buffer, 6);

		ushort ch1v =  __LL_ADC_CALC_DATA_TO_VOLTAGE(VDDA_APPLI, dma_rx_buffer[0], LL_ADC_RESOLUTION_16B);
		ushort ch2v =  __LL_ADC_CALC_DATA_TO_VOLTAGE(VDDA_APPLI, dma_rx_buffer[1], LL_ADC_RESOLUTION_16B);
		ushort ch3v =  __LL_ADC_CALC_DATA_TO_VOLTAGE(VDDA_APPLI, dma_rx_buffer[2], LL_ADC_RESOLUTION_16B);
		printf("ch1: %dmV, ch2: %dmV, ch2: %dmV\r\n", ch1v, ch2v, ch3v);

		//tempSensor = (TEMPSENSOR_CAL2_TEMP - TEMPSENSOR_CAL1_TEMP)/(*TEMPSENSOR_CAL2_ADDR - *TEMPSENSOR_CAL1_ADDR) * dataStructure->getRxBuffer()[4] + 30;
		#endif

		#ifdef LL_ADC_USE_POLLING

		LL_ADC_REG_StartConversion(ADC3);

		for (uint8_t i = 0; i < NUMBER_OF_ADC3_CHANNELS; i++)
		{
			while(LL_ADC_IsActiveFlag_EOC(ADC3) == 0);
			LL_ADC_ClearFlag_EOC(ADC3);

			ADC_Val[i] = LL_ADC_REG_ReadConversionData16(ADC3);
		}
		//LL_ADC_REG_StopConversion(ADC3);

		#if 0
		for (uint8_t i = 0; i < NUMBER_OF_ADC3_CHANNELS; i++)
		{
			ushort chv =  __LL_ADC_CALC_DATA_TO_VOLTAGE(VDDA_APPLI, ADC_Val[i], LL_ADC_RESOLUTION_16B);

			if(i == VIRT_CH3_CPU_TEMP)
			{
				ulong temp = (TEMPSENSOR_CAL2_TEMP - TEMPSENSOR_CAL1_TEMP);
				temp = (temp * (ADC_Val[i] - *TEMPSENSOR_CAL1_ADDR))/(*TEMPSENSOR_CAL2_ADDR - *TEMPSENSOR_CAL1_ADDR);
				temp += 30;
				printf("ch%d: %dC   (int)\r\n", i, (int)temp);
			}
			else if(i == VIRT_CH5_PA_TEMP)
			{
				printf("ch%d: %dC   (pa )\r\n", i, chv/1500 + 20);
			}
			else if(i == VIRT_CH2_VBAT)
			{
				chv *= 4;
				printf("ch%d: %d.%2dV (vbat)\r\n", i, chv/1000, (chv%1000)/10);
			}
			else if(i == VIRT_CH1_REF_PWR)
			{
				printf("ch%d: %4dmV(ref)\r\n", i, chv);
			}
			else if(i == VIRT_CH4_FWD_PWR)
			{
				printf("ch%d: %4dmV(fwd)\r\n", i, chv);
			}
			else
				printf("ch%d: %dmV \r\n", i, chv);
		}
		printf(" \r\n");
		#endif

		//printf("ch%d: %4dmV(fwd)\r\n", VIRT_CH4_FWD_PWR, __LL_ADC_CALC_DATA_TO_VOLTAGE(VDDA_APPLI, ADC_Val[VIRT_CH4_FWD_PWR], LL_ADC_RESOLUTION_16B));

		#endif

		#ifdef LL_ADC_USE_BDMA

		SCB_InvalidateDCache_by_Addr((uint32_t *)dma_rx_buffer, sizeof(dma_rx_buffer));

		ushort ch1v =  __LL_ADC_CALC_DATA_TO_VOLTAGE(VDDA_APPLI, dma_rx_buffer[0], LL_ADC_RESOLUTION_16B);
		ushort ch2v =  __LL_ADC_CALC_DATA_TO_VOLTAGE(VDDA_APPLI, dma_rx_buffer[1], LL_ADC_RESOLUTION_16B);
		ushort ch3v =  __LL_ADC_CALC_DATA_TO_VOLTAGE(VDDA_APPLI, dma_rx_buffer[2], LL_ADC_RESOLUTION_16B);
		if((ch1v)||(ch2v)||(ch3v))
			printf("ch1: %dmV, ch2: %dmV, ch2: %dmV\r\n", ch1v, ch2v, ch3v);
		#endif

	}
}

#ifdef LL_ADC_USE_IRQ
void adc_callback(void)
{
	//LL_ADC_DisableIT_EOC(ADC3);
	//LL_ADC_DisableIT_EOSMP(ADC3);

	//--printf("adc irq\r\n");
	ch1 = LL_ADC_REG_ReadConversionData16(ADC3);

	//ch3 = LL_ADC_REG_ReadConversionData16(ADC3);
	//ch6 = LL_ADC_REG_ReadConversionData16(ADC3);
	//ch8 = LL_ADC_REG_ReadConversionData16(ADC3);

	samp_done = 1;
}
#endif

#ifdef LL_ADC_USE_BDMA

void adc_callback(void)
{
	printf("bdma irq\r\n");
}

static void MX_BDMA_Init(void)
{
	/* DMA controller clock enable */
	LL_AHB4_GRP1_EnableClock(LL_AHB4_GRP1_PERIPH_BDMA);

	/* BDMA_Channel0_IRQn interrupt configuration */
	NVIC_SetPriority(BDMA_Channel0_IRQn, NVIC_EncodePriority(NVIC_GetPriorityGrouping(),3, 0));
	NVIC_EnableIRQ	(BDMA_Channel0_IRQn);
}

static void MX_ADC3_Init(void)
{
	LL_ADC_InitTypeDef 			ADC_InitStruct = {0};
	LL_ADC_REG_InitTypeDef 		ADC_REG_InitStruct = {0};
	LL_ADC_CommonInitTypeDef	ADC_CommonInitStruct = {0};
	LL_GPIO_InitTypeDef 		GPIO_InitStruct = {0};

	LL_RCC_SetADCClockSource(LL_RCC_ADC_CLKSOURCE_PLL2P);
	LL_AHB4_GRP1_EnableClock(LL_AHB4_GRP1_PERIPH_ADC3);

	LL_AHB4_GRP1_EnableClock(LL_AHB4_GRP1_PERIPH_GPIOF);
	LL_AHB4_GRP1_EnableClock(LL_AHB4_GRP1_PERIPH_GPIOC);

	/**ADC3 GPIO Configuration
  	  PF6   ------> ADC3_INP8
  	  PF7   ------> ADC3_INP3
  	  PF10   ------> ADC3_INP6
  	  PC3_C   ------> ADC3_INP1
	 */
	GPIO_InitStruct.Pin = LL_GPIO_PIN_6|LL_GPIO_PIN_7|LL_GPIO_PIN_10;
	GPIO_InitStruct.Mode = LL_GPIO_MODE_ANALOG;
	GPIO_InitStruct.Pull = LL_GPIO_PULL_NO;
	LL_GPIO_Init(GPIOF, &GPIO_InitStruct);

	GPIO_InitStruct.Pin = LL_GPIO_PIN_3;
	GPIO_InitStruct.Mode = LL_GPIO_MODE_ANALOG;
	GPIO_InitStruct.Pull = LL_GPIO_PULL_NO;
	LL_GPIO_Init(GPIOC, &GPIO_InitStruct);

	// ADC3 DMA Init
	LL_BDMA_SetDataTransferDirection(BDMA, LL_BDMA_CHANNEL_0, LL_BDMA_DIRECTION_PERIPH_TO_MEMORY);
	LL_BDMA_SetChannelPriorityLevel	(BDMA, LL_BDMA_CHANNEL_0, LL_BDMA_PRIORITY_LOW);
	LL_BDMA_SetMode					(BDMA, LL_BDMA_CHANNEL_0, LL_BDMA_MODE_CIRCULAR);
	LL_BDMA_SetPeriphIncMode		(BDMA, LL_BDMA_CHANNEL_0, LL_BDMA_PERIPH_NOINCREMENT);
	LL_BDMA_SetMemoryIncMode		(BDMA, LL_BDMA_CHANNEL_0, LL_BDMA_MEMORY_INCREMENT);
	LL_BDMA_SetPeriphSize			(BDMA, LL_BDMA_CHANNEL_0, LL_BDMA_PDATAALIGN_HALFWORD);
	LL_BDMA_SetMemorySize			(BDMA, LL_BDMA_CHANNEL_0, LL_BDMA_MDATAALIGN_HALFWORD);
	LL_BDMA_SetPeriphAddress		(BDMA, LL_BDMA_CHANNEL_0, (uint32_t)&(ADC3->DR));
	LL_BDMA_SetMemoryAddress		(BDMA, LL_BDMA_CHANNEL_0, (uint32_t)dma_rx_buffer);
  	LL_BDMA_SetDataLength			(BDMA, LL_BDMA_CHANNEL_0, 3);
  	LL_BDMA_EnableChannel			(BDMA, LL_BDMA_CHANNEL_0);
  	LL_BDMA_SetPeriphRequest		(BDMA, LL_BDMA_CHANNEL_0, LL_DMAMUX2_REQ_ADC3);

  	//LL_ADC_REG_SetDataTransferMode(ADC3, ADC_CFGR_DMACONTREQ(LL_ADC_REG_DMA_TRANSFER_UNLIMITED));
  	LL_ADC_REG_SetDataTransferMode(ADC3, LL_ADC_REG_DMA_TRANSFER_UNLIMITED);

  	// Common config
  	LL_ADC_SetOverSamplingScope(ADC3, LL_ADC_OVS_DISABLE);
  	ADC_InitStruct.Resolution 			= LL_ADC_RESOLUTION_16B;
  	ADC_InitStruct.LowPowerMode 		= LL_ADC_LP_MODE_NONE;
  	LL_ADC_Init(ADC3, &ADC_InitStruct);

  	ADC_REG_InitStruct.TriggerSource 	= LL_ADC_REG_TRIG_SOFTWARE;
  	ADC_REG_InitStruct.SequencerLength	= LL_ADC_REG_SEQ_SCAN_ENABLE_3RANKS;
  	ADC_REG_InitStruct.SequencerDiscont = DISABLE;
  	ADC_REG_InitStruct.ContinuousMode 	= LL_ADC_REG_CONV_CONTINUOUS;
  	ADC_REG_InitStruct.Overrun 			= LL_ADC_REG_OVR_DATA_OVERWRITTEN;
  	LL_ADC_REG_Init(ADC3, &ADC_REG_InitStruct);

  	ADC_CommonInitStruct.Multimode = LL_ADC_MULTI_INDEPENDENT;
  	LL_ADC_CommonInit(__LL_ADC_COMMON_INSTANCE(ADC3), &ADC_CommonInitStruct);

	#if 0
  	/* Disable ADC deep power down (enabled by default after reset state) */
  	LL_ADC_DisableDeepPowerDown(ADC3);

  	/* Enable ADC internal voltage regulator */
  	LL_ADC_EnableInternalRegulator(ADC3);

  	/* Delay for ADC internal voltage regulator stabilization. */
  	/* Compute number of CPU cycles to wait for, from delay in us. */
  	/* Note: Variable divided by 2 to compensate partially */
  	/* CPU processing cycles (depends on compilation optimization). */
  	/* Note: If system core clock frequency is below 200kHz, wait time */
  	/* is only a few CPU processing cycles. */
  	__IO uint32_t wait_loop_index;
  	wait_loop_index = ((LL_ADC_DELAY_INTERNAL_REGUL_STAB_US * (SystemCoreClock / (100000 * 2))) / 10);
  	while(wait_loop_index != 0)
  	{
  		wait_loop_index--;
  	}
	#endif

  	// Connect internal channels
  	LL_ADC_SetCommonPathInternalCh(__LL_ADC_COMMON_INSTANCE(ADC3),LL_ADC_PATH_INTERNAL_VREFINT|\
  																	  LL_ADC_PATH_INTERNAL_TEMPSENSOR|\
  																	  LL_ADC_PATH_INTERNAL_VBAT);


  	LL_ADC_REG_SetSequencerRanks(ADC3, LL_ADC_REG_RANK_1, LL_ADC_CHANNEL_VBAT);
  	LL_ADC_SetChannelSamplingTime(ADC3, LL_ADC_CHANNEL_VBAT, ADC_SAMP_TIME);
  	LL_ADC_SetChannelSingleDiff(ADC3, LL_ADC_CHANNEL_VBAT, LL_ADC_SINGLE_ENDED);

  	LL_ADC_REG_SetSequencerRanks(ADC3, LL_ADC_REG_RANK_2, LL_ADC_CHANNEL_TEMPSENSOR);
  	LL_ADC_SetChannelSamplingTime(ADC3, LL_ADC_CHANNEL_TEMPSENSOR, ADC_SAMP_TIME);
  	LL_ADC_SetChannelSingleDiff(ADC3, LL_ADC_CHANNEL_TEMPSENSOR, LL_ADC_SINGLE_ENDED);

  	LL_ADC_REG_SetSequencerRanks(ADC3, LL_ADC_REG_RANK_3, LL_ADC_CHANNEL_VREFINT);
  	LL_ADC_SetChannelSamplingTime(ADC3, LL_ADC_CHANNEL_VREFINT, ADC_SAMP_TIME);
  	LL_ADC_SetChannelSingleDiff(ADC3, LL_ADC_CHANNEL_VREFINT, LL_ADC_SINGLE_ENDED);

  	//LL_ADC_REG_SetSequencerRanks(ADC3, LL_ADC_REG_RANK_1, LL_ADC_CHANNEL_6);
  	//LL_ADC_SetChannelSamplingTime(ADC3, LL_ADC_CHANNEL_6, ADC_SAMP_TIME);
  	//LL_ADC_SetChannelSingleDiff(ADC3, LL_ADC_CHANNEL_6, LL_ADC_SINGLE_ENDED);

  	/** Configure Regular Channel
  	 */
  	//LL_ADC_REG_SetSequencerRanks(ADC3, LL_ADC_REG_RANK_2, LL_ADC_CHANNEL_8);
  	//LL_ADC_SetChannelSamplingTime(ADC3, LL_ADC_CHANNEL_8, ADC_SAMP_TIME);
  	//LL_ADC_SetChannelSingleDiff(ADC3, LL_ADC_CHANNEL_8, LL_ADC_SINGLE_ENDED);

  	/** Configure Regular Channel
  	 */
  	//LL_ADC_REG_SetSequencerRanks(ADC3, LL_ADC_REG_RANK_6, LL_ADC_CHANNEL_1);
  	//LL_ADC_SetChannelSamplingTime(ADC3, LL_ADC_CHANNEL_1, ADC_SAMP_TIME);
  	//LL_ADC_SetChannelSingleDiff(ADC3, LL_ADC_CHANNEL_1, LL_ADC_SINGLE_ENDED);

  	/** Configure Regular Channel
  	 */
  	//LL_ADC_REG_SetSequencerRanks(ADC3, LL_ADC_REG_RANK_7, LL_ADC_CHANNEL_1);
  	//LL_ADC_SetChannelSamplingTime(ADC3, LL_ADC_CHANNEL_1, ADC_SAMP_TIME);
  	//LL_ADC_SetChannelSingleDiff(ADC3, LL_ADC_CHANNEL_1, LL_ADC_SINGLE_ENDED);


	LL_BDMA_EnableIT_TC(BDMA, LL_BDMA_CHANNEL_0);
	LL_BDMA_EnableIT_HT(BDMA, LL_BDMA_CHANNEL_0);
	LL_BDMA_EnableIT_TE(BDMA, LL_BDMA_CHANNEL_0);

}

#endif

uchar adc_init(void)
{
	int i;

	// Clear DMA buffer
	#ifdef LL_ADC_USE_DMA
	for(i = 0; i < sizeof(dma_rx_buffer); i++)
		dma_rx_buffer[i] = 0;
	#endif

	for(i = 0; i < sizeof(ADC_Val); i++)
		ADC_Val[i] = 0;

	#ifndef LL_ADC_USE_BDMA
	adc_configure();
	adc_activate();

	#ifndef LL_ADC_USE_POLLING
	if((LL_ADC_IsEnabled(ADC3) == 1)&&(LL_ADC_IsDisableOngoing(ADC3) == 0)&&(LL_ADC_REG_IsConversionOngoing(ADC3) == 0))
	{
	    LL_ADC_REG_StartConversion(ADC3);
	    printf("adc start \r\n");
	}
	else
	{
		return;
	}
	#endif
	#endif

	// BDMA
	#ifdef LL_ADC_USE_BDMA
	MX_BDMA_Init();
	MX_ADC3_Init();
	adc_activate();
	LL_ADC_REG_StartConversion(ADC3);
	#endif

	// ADC processor (only register, will start with scheduler)
    xTaskCreate((TaskFunction_t)adc_proc_task, "adc_proc", 256, NULL, osPriorityNormal, NULL);

    return 0;
}

ushort adc_read_ref_power(void)
{
	if(!adc_init_done)
		return 0xFFFF;

	return __LL_ADC_CALC_DATA_TO_VOLTAGE(VDDA_APPLI, ADC_Val[VIRT_CH1_REF_PWR], LL_ADC_RESOLUTION_16B);
}

ushort adc_read_fwd_power(void)
{
	if(!adc_init_done)
		return 0xFFFF;

	return __LL_ADC_CALC_DATA_TO_VOLTAGE(VDDA_APPLI, ADC_Val[VIRT_CH4_FWD_PWR], LL_ADC_RESOLUTION_16B);
}
