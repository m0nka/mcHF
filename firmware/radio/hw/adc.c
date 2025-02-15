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
//ushort dma_rx_buffer[2];
__attribute__((section("dma_mem"))) __attribute__ ((aligned (32))) ushort dma_rx_buffer[2];
#endif

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
	LL_GPIO_SetPinMode(POWER_LED_PORT, LL_GPIO_PIN_6, LL_GPIO_MODE_ANALOG);

	// -----------------------------------------------------------------
	// ADC3_INP3, PA Temperature (PF7)
	LL_GPIO_SetPinMode(ADC3_INP3_PORT, ADC3_INP3, LL_GPIO_MODE_ANALOG);

	// -----------------------------------------------------------------
	// ADC3_INP6, Forward Power (PF10)
	LL_GPIO_SetPinMode(ADC3_INP6_PORT, ADC3_INP6, LL_GPIO_MODE_ANALOG);

	LL_AHB4_GRP1_EnableClock(LL_AHB4_GRP1_PERIPH_ADC3);

	LL_ADC_SetCommonClock	(__LL_ADC_COMMON_INSTANCE(ADC3), LL_ADC_CLOCK_SYNC_PCLK_DIV4);

	//LL_RCC_SetADCClockSource(LL_RCC_ADC_CLKSOURCE_PLL2P);
	//LL_ADC_SetCommonClock	(__LL_ADC_COMMON_INSTANCE(ADC3), LL_ADC_CLOCK_ASYNC_DIV1);

	#ifdef LL_ADC_USE_DMA
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
	ADC_Config_DMA.NbData 					= 2;
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
	//LL_ADC_SetMultiTwoSamplingDelay(__LL_ADC_COMMON_INSTANCE(ADC3), LL_ADC_MULTI_TWOSMP_DELAY_1CYCLE);
	//LL_ADC_SetResolution(ADC3, LL_ADC_DATA_ALIGN_RIGHT);
	//LL_ADC_SetLowPowerMode(ADC3, LL_ADC_LP_MODE_NONE);
	//LL_ADC_SetOffset(ADC3, LL_ADC_OFFSET_1, LL_ADC_CHANNEL_15, 0x000);
	//LL_ADC_REG_SetDMATransfer(ADC3, LL_ADC_REG_DMA_TRANSFER_NONE);
	//LL_ADC_REG_SetTriggerEdge(ADC3, LL_ADC_REG_TRIG_EXT_RISING);
	//LL_ADC_REG_SetSequencerDiscont(ADC3, LL_ADC_REG_SEQ_DISCONT_DISABLE);

	LL_ADC_SetMultimode				(__LL_ADC_COMMON_INSTANCE(ADC3), LL_ADC_MULTI_INDEPENDENT);
	LL_ADC_SetResolution			(ADC3, LL_ADC_RESOLUTION_16B);
	LL_ADC_REG_SetTriggerSource		(ADC3, LL_ADC_REG_TRIG_SOFTWARE);
	//LL_ADC_REG_SetOverrun			(ADC3, LL_ADC_REG_OVR_DATA_OVERWRITTEN);
	LL_ADC_REG_SetOverrun			(ADC3, LL_ADC_REG_OVR_DATA_PRESERVED);

	#if 0
	LL_ADC_REG_SetSequencerLength	(ADC3, LL_ADC_REG_SEQ_SCAN_DISABLE);
	LL_ADC_REG_SetContinuousMode	(ADC3, LL_ADC_REG_CONV_SINGLE);
	#else
	LL_ADC_REG_SetSequencerLength	(ADC3, LL_ADC_REG_SEQ_SCAN_ENABLE_2RANKS);
	LL_ADC_REG_SetContinuousMode	(ADC3, LL_ADC_REG_CONV_CONTINUOUS);
	#endif

	LL_ADC_SetChannelPreSelection(ADC3, LL_ADC_CHANNEL_VREFINT);
	LL_ADC_SetChannelPreSelection(ADC3, LL_ADC_CHANNEL_TEMPSENSOR);

	#if 1
	// VREF internal channel
	LL_ADC_SetCommonPathInternalCh	(__LL_ADC_COMMON_INSTANCE(ADC3), LL_ADC_PATH_INTERNAL_VREFINT);
	LL_ADC_SetChannelSamplingTime	(ADC3, LL_ADC_CHANNEL_VREFINT, LL_ADC_SAMPLINGTIME_810CYCLES_5);
	LL_ADC_REG_SetSequencerRanks	(ADC3, LL_ADC_REG_RANK_1, LL_ADC_CHANNEL_VREFINT);
	#endif

	#if 0
	// VBAT internal channel
	LL_ADC_SetCommonPathInternalCh	(__LL_ADC_COMMON_INSTANCE(ADC3), LL_ADC_PATH_INTERNAL_VBAT);
	LL_ADC_SetChannelSamplingTime	(ADC3, LL_ADC_CHANNEL_VBAT, LL_ADC_SAMPLINGTIME_810CYCLES_5);
	LL_ADC_REG_SetSequencerRanks	(ADC3, LL_ADC_REG_RANK_1, LL_ADC_CHANNEL_VBAT);
	#endif

	#if 1
	// Temp internal channel
	LL_ADC_SetCommonPathInternalCh	(__LL_ADC_COMMON_INSTANCE(ADC3), LL_ADC_PATH_INTERNAL_TEMPSENSOR);
	LL_ADC_SetChannelSamplingTime	(ADC3, LL_ADC_CHANNEL_TEMPSENSOR, LL_ADC_SAMPLINGTIME_810CYCLES_5);
	LL_ADC_REG_SetSequencerRanks	(ADC3, LL_ADC_REG_RANK_2, LL_ADC_CHANNEL_TEMPSENSOR);
	#endif

	#if 0
	// CH1, Reflected Power (PC3)
	LL_ADC_SetChannelSingleDiff		(ADC3, LL_ADC_CHANNEL_1, LL_ADC_SINGLE_ENDED);
	LL_ADC_SetChannelSamplingTime	(ADC3, LL_ADC_CHANNEL_1, LL_ADC_SAMPLINGTIME_810CYCLES_5);
	LL_ADC_REG_SetSequencerRanks	(ADC3, LL_ADC_REG_RANK_1, LL_ADC_CHANNEL_1);
	#endif

	#if 0
	// CH3, PA Temperature (PF7) - not working, pin multiplexer ? ToDo:
	LL_ADC_SetChannelSingleDiff		(ADC3, LL_ADC_CHANNEL_3, LL_ADC_SINGLE_ENDED);
	LL_ADC_SetChannelSamplingTime	(ADC3, LL_ADC_CHANNEL_3, LL_ADC_SAMPLINGTIME_810CYCLES_5);
	LL_ADC_REG_SetSequencerRanks	(ADC3, LL_ADC_REG_RANK_1, LL_ADC_CHANNEL_3);
	#endif

	#if 0
	// CH6, Forward Power (PF10) - not working, pin multiplexer ? ToDo:
	LL_ADC_SetChannelSingleDiff		(ADC3, LL_ADC_CHANNEL_6, LL_ADC_SINGLE_ENDED);
	LL_ADC_SetChannelSamplingTime	(ADC3, LL_ADC_CHANNEL_6, LL_ADC_SAMPLINGTIME_810CYCLES_5);
	LL_ADC_REG_SetSequencerRanks	(ADC3, LL_ADC_REG_RANK_1,LL_ADC_CHANNEL_6);
	#endif

	#if 0
	// CH8, Ambient light sensor (PF6) - not working, pin multiplexer ? ToDo:
	LL_ADC_SetChannelSingleDiff		(ADC3, LL_ADC_CHANNEL_8, LL_ADC_SINGLE_ENDED);
	LL_ADC_SetChannelSamplingTime	(ADC3, LL_ADC_CHANNEL_8, LL_ADC_SAMPLINGTIME_810CYCLES_5);
	LL_ADC_REG_SetSequencerRanks	(ADC3, LL_ADC_REG_RANK_1, LL_ADC_CHANNEL_8);
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
	}

	printf("adc act \r\n");
}

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

	for(;;)
	{
		vTaskDelay(2000);

		#ifdef LL_ADC_USE_IRQ
		if(samp_done)
		{
			//printf("ch1: %d, ch3: %d, ch6: %d, ch8: %d\r\n", ch1, ch3, ch6, ch8);

			ushort ch1v =  __LL_ADC_CALC_DATA_TO_VOLTAGE(VDDA_APPLI, ch1, LL_ADC_RESOLUTION_16B);
			ushort ch3v =  __LL_ADC_CALC_DATA_TO_VOLTAGE(VDDA_APPLI, ch3, LL_ADC_RESOLUTION_16B);
			//printf("ch1: %d.%dV\r\n",v/1000, v%1000);
			printf("ch1: %dmV(%d), ch3: %dmV(%d)\r\n", ch1v, ch1, ch3v, ch3);

			LL_ADC_REG_StartConversion(ADC3);
			samp_done = 0;
		}
		#endif

		#ifdef LL_ADC_USE_DMA
		if(LL_DMA_IsActiveFlag_TC1(DMA1))
		{
			LL_DMA_ClearFlag_TC1(DMA1);

			ushort v =  __LL_ADC_CALC_DATA_TO_VOLTAGE(VDDA_APPLI, dma_rx_buffer[0], LL_ADC_RESOLUTION_16B);
			printf("ch1: %d.%dV\r\n",v/1000, v%1000);

			//tempSensor = (TEMPSENSOR_CAL2_TEMP - TEMPSENSOR_CAL1_TEMP)/(*TEMPSENSOR_CAL2_ADDR - *TEMPSENSOR_CAL1_ADDR) * dataStructure->getRxBuffer()[4] + 30;

		}
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

	ch3 = LL_ADC_REG_ReadConversionData16(ADC3);
	//ch6 = LL_ADC_REG_ReadConversionData16(ADC3);
	//ch8 = LL_ADC_REG_ReadConversionData16(ADC3);

	samp_done = 1;
}
#endif

void adc_init(void)
{
	adc_configure();
	adc_activate();

	//#ifdef LL_ADC_USE_IRQ
	if((LL_ADC_IsEnabled(ADC3) == 1)&&(LL_ADC_IsDisableOngoing(ADC3) == 0)&&(LL_ADC_REG_IsConversionOngoing(ADC3) == 0))
	{
	    LL_ADC_REG_StartConversion(ADC3);
	    printf("adc start \r\n");
	}
	else
	{
		return;
	}
	//#endif

	// ADC processor (only register, will start with scheduler)
    xTaskCreate((TaskFunction_t)adc_proc_task, "adc_proc", 256, NULL, osPriorityNormal, NULL);
}
