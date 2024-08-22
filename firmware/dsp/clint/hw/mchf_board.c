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
#include "mchf_board.h"

#include <stdio.h>

#if 1

#include "mchf_hw_i2c.h"
#include "mchf_hw_i2c2.h"

#include "ui_rotary.h"
#include "ui_lcd_hy28.h"
//
#include "ui_driver.h"
//
#include "codec.h"
//
#include "ui_si570.h"
//
// Eeprom items
#include "eeprom.h"
extern uint16_t VirtAddVarTab[NB_OF_VAR];
//
extern const ButtonMap	bm[];

// Transceiver state public structure
extern __IO TransceiverState ts;

//
__IO	FilterCoeffs		fc;

// ------------------------------------------------
// Frequency public
__IO DialFrequency 				df;

void mchf_board_swo_init(uint32_t portBits, uint32_t cpuCoreFreqHz)
{
  uint32_t SWOSpeed = 2000000; /* default 64k baud rate */
  uint32_t SWOPrescaler = (cpuCoreFreqHz / SWOSpeed) - 1; /* SWOSpeed in Hz, note that cpuCoreFreqHz is expected to be match the CPU core clock */

  CoreDebug->DEMCR = CoreDebug_DEMCR_TRCENA_Msk; /* enable trace in core debug */
  *((volatile unsigned *)(ITM_BASE + 0x400F0)) = 0x00000002; /* "Selected PIN Protocol Register": Select which protocol to use for trace output (2: SWO NRZ, 1: SWO Manchester encoding) */
  *((volatile unsigned *)(ITM_BASE + 0x40010)) = SWOPrescaler; /* "Async Clock Prescaler Register". Scale the baud rate of the asynchronous output */
  *((volatile unsigned *)(ITM_BASE + 0x00FB0)) = 0xC5ACCE55; /* ITM Lock Access Register, C5ACCE55 enables more write access to Control Register 0xE00 :: 0xFFC */
  ITM->TCR = ITM_TCR_TraceBusID_Msk | ITM_TCR_SWOENA_Msk | ITM_TCR_SYNCENA_Msk | ITM_TCR_ITMENA_Msk; /* ITM Trace Control Register */
  ITM->TPR = ITM_TPR_PRIVMASK_Msk; /* ITM Trace Privilege Register */
  ITM->TER = portBits; /* ITM Trace Enable Register. Enabled tracing on stimulus ports. One bit per stimulus port. */
  *((volatile unsigned *)(ITM_BASE + 0x01000)) = 0x400003FE; /* DWT_CTRL */
  *((volatile unsigned *)(ITM_BASE + 0x40304)) = 0x00000100; /* Formatter and Flush Control Register */
}

//*----------------------------------------------------------------------------
//* Function Name       : mchf_board_led_init
//* Object              :
//* Object              :
//* Input Parameters    :
//* Output Parameters   :
//* Functions called    :
//*----------------------------------------------------------------------------
static void mchf_board_led_init(void)
{
	GPIO_InitTypeDef  GPIO_InitStructure;

	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;

	GPIO_InitStructure.GPIO_Pin = GREEN_LED;
	GPIO_Init(GREEN_LED_PIO, &GPIO_InitStructure);

	GPIO_InitStructure.GPIO_Pin = RED_LED;
	GPIO_Init(RED_LED_PIO, &GPIO_InitStructure);
}

//*----------------------------------------------------------------------------
//* Function Name       : mchf_board_debug_init
//* Object              :
//* Object              :
//* Input Parameters    :
//* Output Parameters   :
//* Functions called    :
//*----------------------------------------------------------------------------
static void mchf_board_debug_init(void)
{
#ifdef DEBUG_BUILD

	USART_InitTypeDef USART_InitStructure;
	GPIO_InitTypeDef GPIO_InitStructure;

	USART_InitStructure.USART_BaudRate = 921600;//230400;
	USART_InitStructure.USART_WordLength = USART_WordLength_8b;
	USART_InitStructure.USART_StopBits = USART_StopBits_1;
	USART_InitStructure.USART_Parity = USART_Parity_No;
	USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
	USART_InitStructure.USART_Mode = USART_Mode_Tx;

	// Enable UART clock
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1, ENABLE);

	// Connect PXx to USARTx_Tx
	GPIO_PinAFConfig(DEBUG_PRINT_PIO, DEBUG_PRINT_SOURCE, GPIO_AF_USART1);

	// Configure USART Tx as alternate function
	GPIO_InitStructure.GPIO_OType 	= GPIO_OType_PP;
	GPIO_InitStructure.GPIO_PuPd 	= GPIO_PuPd_UP;
	GPIO_InitStructure.GPIO_Mode 	= GPIO_Mode_AF;

	GPIO_InitStructure.GPIO_Pin 	= DEBUG_PRINT;
	GPIO_InitStructure.GPIO_Speed 	= GPIO_Speed_50MHz;
	GPIO_Init(DEBUG_PRINT_PIO, &GPIO_InitStructure);

	// USART configuration
	USART_Init(USART1, &USART_InitStructure);

	// Enable USART
	USART_Cmd(USART1, ENABLE);

	// Wait tx ready
	while (USART_GetFlagStatus(DEBUG_COM, USART_FLAG_TC) == RESET);

	// Debug print details
	printf("-----------------------------------------------------\n\r");
	printf("%s v %d.%d.%d.%d\n\r",DEVICE_STRING,TRX4M_VER_MAJOR,TRX4M_VER_MINOR,TRX4M_VER_RELEASE,TRX4M_VER_BUILD);
	printf("= %s =\n\r",AUTHOR_STRING);

#endif

	// Make sure PB3 is not allocated to keypad!!!
	//
	mchf_board_swo_init(0x0, 168000000);
}

//*----------------------------------------------------------------------------
//* Function Name       : mchf_board_keypad_init
//* Object              :
//* Object              :
//* Input Parameters    :
//* Output Parameters   :
//* Functions called    :
//*----------------------------------------------------------------------------
static void mchf_board_keypad_init(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;
	ulong i;

	// Common init
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;

	// Init all from public struct declaration (ui driver)
	for(i = 0; i < 16; i++)
	{
		GPIO_InitStructure.GPIO_Pin = bm[i].button;
		GPIO_Init(bm[i].port, &GPIO_InitStructure);
	}
}

//*----------------------------------------------------------------------------
//* Function Name       : mchf_board_ptt_init
//* Object              :
//* Object              :
//* Input Parameters    :
//* Output Parameters   :
//* Functions called    :
//*----------------------------------------------------------------------------
static void mchf_board_ptt_init(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;

	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_DOWN;

	// RX/TX control pin init
	GPIO_InitStructure.GPIO_Pin = PTT_CNTR;
	GPIO_Init(PTT_CNTR_PIO, &GPIO_InitStructure);

#ifdef REV_08
	if(get_pcb_rev() == 0x08)
	{
	// RX/TX N control pin init
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
	GPIO_InitStructure.GPIO_Pin = PTTN_CNTR;
	GPIO_Init(PTTN_CNTR_PIO, &GPIO_InitStructure);

	PTTN_CNTR_PIO->BSRRL = PTTN_CNTR;
	}
#endif

	// RX on
	PTT_CNTR_PIO->BSRRH = PTT_CNTR;

	//#ifdef REV_08
	//PTTN_CNTR_PIO->BSRRL = PTTN_CNTR;
	//#endif
}

//*----------------------------------------------------------------------------
//* Function Name       : mchf_board_keyer_irq_init
//* Object              :
//* Object              :
//* Input Parameters    :
//* Output Parameters   :
//* Functions called    :
//*----------------------------------------------------------------------------
static void mchf_board_keyer_irq_init(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;
	EXTI_InitTypeDef EXTI_InitStructure;
	NVIC_InitTypeDef NVIC_InitStructure;

	// Enable the BUTTON Clock
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_SYSCFG, ENABLE);

	// Configure PADDLE_DASH pin as input
	GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_IN;
	GPIO_InitStructure.GPIO_PuPd  = GPIO_PuPd_UP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;

	GPIO_InitStructure.GPIO_Pin   = PADDLE_DAH;
	GPIO_Init(PADDLE_DAH_PIO, &GPIO_InitStructure);

	// Connect Button EXTI Line to PADDLE_DASH GPIO Pin
	SYSCFG_EXTILineConfig(EXTI_PortSourceGPIOE,EXTI_PinSource0);

	// Configure PADDLE_DASH EXTI line
	EXTI_InitStructure.EXTI_Line    = EXTI_Line0;
	EXTI_InitStructure.EXTI_Mode    = EXTI_Mode_Interrupt;
	EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Falling;
	EXTI_InitStructure.EXTI_LineCmd = ENABLE;
	EXTI_Init(&EXTI_InitStructure);

	// Enable and set PADDLE_DASH EXTI Interrupt to the lowest priority
	NVIC_InitStructure.NVIC_IRQChannel = EXTI0_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0x0F;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0x0F;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStructure);

	// Configure PADDLE_DOT pin as input
	GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_IN;
	GPIO_InitStructure.GPIO_PuPd  = GPIO_PuPd_UP;
   	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;

   	GPIO_InitStructure.GPIO_Pin   = PADDLE_DIT;
   	GPIO_Init(PADDLE_DIT_PIO, &GPIO_InitStructure);

    // Connect Button EXTI Line to PADDLE_DOT GPIO Pin
    SYSCFG_EXTILineConfig(EXTI_PortSourceGPIOE,EXTI_PinSource1);

    // Configure PADDLE_DOT EXTI line
    EXTI_InitStructure.EXTI_Line    = EXTI_Line1;
    EXTI_InitStructure.EXTI_Mode    = EXTI_Mode_Interrupt;
    EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Falling;
    EXTI_InitStructure.EXTI_LineCmd = ENABLE;
    EXTI_Init(&EXTI_InitStructure);

    // Enable and set PADDLE_DOT EXTI Interrupt to the lowest priority
    NVIC_InitStructure.NVIC_IRQChannel = EXTI1_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0x0F;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0x0F;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);
}

void mchf_board_power_button_input_init(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;

	// Configure pin as input
	GPIO_InitStructure.GPIO_Pin   = BUTTON_PWR;
	GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_IN;
	GPIO_InitStructure.GPIO_PuPd  = GPIO_PuPd_UP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;
	GPIO_Init(BUTTON_PWR_PIO, &GPIO_InitStructure);
}
//*----------------------------------------------------------------------------
//* Function Name       : mchf_board_power_button_irq_init
//* Object              :
//* Object              :
//* Input Parameters    :
//* Output Parameters   :
//* Functions called    :
//*----------------------------------------------------------------------------
static void mchf_board_power_button_irq_init(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;
	EXTI_InitTypeDef EXTI_InitStructure;
	NVIC_InitTypeDef NVIC_InitStructure;

	// Enable the BUTTON Clock
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_SYSCFG, ENABLE);

	// Configure pin as input
	GPIO_InitStructure.GPIO_Pin   = BUTTON_PWR;
	GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_IN;
	GPIO_InitStructure.GPIO_PuPd  = GPIO_PuPd_UP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;
	GPIO_Init(BUTTON_PWR_PIO, &GPIO_InitStructure);

	// Connect Button EXTI Line to GPIO Pin
	SYSCFG_EXTILineConfig(EXTI_PortSourceGPIOC,EXTI_PinSource13);

	// Configure PADDLE_DASH EXTI line
	EXTI_InitStructure.EXTI_Line    = EXTI_Line13;
	EXTI_InitStructure.EXTI_Mode    = EXTI_Mode_Interrupt;
	EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Falling;
	EXTI_InitStructure.EXTI_LineCmd = ENABLE;
	EXTI_Init(&EXTI_InitStructure);

	// Enable and set PADDLE_DASH EXTI Interrupt to the lowest priority
	NVIC_InitStructure.NVIC_IRQChannel = EXTI15_10_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0x0F;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0x0F;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStructure);
}

//*----------------------------------------------------------------------------
//* Function Name       : mchf_board_dac0_init
//* Object              :
//* Object              :
//* Input Parameters    :
//* Output Parameters   :
//* Functions called    :
//*----------------------------------------------------------------------------
static void mchf_board_dac0_init(void)
{
	 GPIO_InitTypeDef GPIO_InitStructure;
	 DAC_InitTypeDef  DAC_InitStructure;

	 // DAC Periph clock enable
	 RCC_APB1PeriphClockCmd(RCC_APB1Periph_DAC, ENABLE);

	 // DAC channel 1 (DAC_OUT1 = PA.4)
	 GPIO_InitStructure.GPIO_Pin  = DAC0;
	 GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AN;
	 GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
	 GPIO_Init(DAC0_PIO, &GPIO_InitStructure);

	 // DAC channel1 Configuration
	 DAC_InitStructure.DAC_Trigger 						= DAC_Trigger_None;
	 DAC_InitStructure.DAC_WaveGeneration 				= DAC_WaveGeneration_None;
	 DAC_InitStructure.DAC_LFSRUnmask_TriangleAmplitude = DAC_LFSRUnmask_Bits7_0;//DAC_TriangleAmplitude_4095;
	 DAC_InitStructure.DAC_OutputBuffer 				= DAC_OutputBuffer_Enable;
	 DAC_Init(DAC_Channel_1, &DAC_InitStructure);

	 // Enable DAC Channel1
	 DAC_Cmd(DAC_Channel_1, ENABLE);

	 // Set DAC Channel1 DHR12L register - JFET attenuator off (0V)
	 DAC_SetChannel1Data(DAC_Align_8b_R, 0x00);
}

//*----------------------------------------------------------------------------
//* Function Name       : mchf_board_dac1_init
//* Object              :
//* Object              :
//* Input Parameters    :
//* Output Parameters   :
//* Functions called    :
//*----------------------------------------------------------------------------
static void mchf_board_dac1_init(void)
{
	 GPIO_InitTypeDef GPIO_InitStructure;
	 DAC_InitTypeDef  DAC_InitStructure;

	 // DAC Periph clock enable
	 RCC_APB1PeriphClockCmd(RCC_APB1Periph_DAC, ENABLE);

	 // DAC channel 1 (DAC_OUT2 = PA.5)
	 GPIO_InitStructure.GPIO_Pin  = DAC1;
	 GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AN;
	 GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
	 GPIO_Init(DAC0_PIO, &GPIO_InitStructure);

	 // DAC channel1 Configuration
	 DAC_InitStructure.DAC_Trigger 						= DAC_Trigger_None;
	 DAC_InitStructure.DAC_WaveGeneration 				= DAC_WaveGeneration_None;
	 DAC_InitStructure.DAC_LFSRUnmask_TriangleAmplitude = DAC_LFSRUnmask_Bits7_0;//DAC_TriangleAmplitude_4095;
	 DAC_InitStructure.DAC_OutputBuffer 				= DAC_OutputBuffer_Enable;
	 DAC_Init(DAC_Channel_2, &DAC_InitStructure);

	 // Enable DAC Channel2
	 DAC_Cmd(DAC_Channel_2, ENABLE);

	 // Set DAC Channel2 DHR12L register - PA Bias (3.80 V)
	 DAC_SetChannel2Data(DAC_Align_8b_R, 220);
}

//*----------------------------------------------------------------------------
//* Function Name       : mchf_board_adc1_init
//* Object              : ADC1 used for power supply measurements
//* Input Parameters    :
//* Output Parameters   :
//* Functions called    :
//*----------------------------------------------------------------------------
static void mchf_board_adc1_init(void)
{
	ADC_InitTypeDef 		ADC_InitStructure;
	ADC_CommonInitTypeDef 	ADC_CommonInitStructure;
	GPIO_InitTypeDef 		GPIO_InitStructure;

	// Enable ADC3 clock
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_ADC1, ENABLE);

	// Configure ADC Channel 6 as analog input
	GPIO_InitStructure.GPIO_Pin 					= ADC1_PWR;
	GPIO_InitStructure.GPIO_Mode 					= GPIO_Mode_AIN;
	GPIO_InitStructure.GPIO_PuPd 					= GPIO_PuPd_NOPULL ;
	GPIO_Init(ADC1_PWR_PIO, &GPIO_InitStructure);

	// Common Init
	ADC_CommonInitStructure.ADC_Mode 				= ADC_Mode_Independent;
	ADC_CommonInitStructure.ADC_Prescaler 			= ADC_Prescaler_Div8;
	ADC_CommonInitStructure.ADC_DMAAccessMode 		= ADC_DMAAccessMode_Disabled;
	ADC_CommonInitStructure.ADC_TwoSamplingDelay	= ADC_TwoSamplingDelay_5Cycles;
	ADC_CommonInit(&ADC_CommonInitStructure);

	// Configuration
	ADC_StructInit(&ADC_InitStructure);
	ADC_InitStructure.ADC_Resolution 				= ADC_Resolution_12b;
	ADC_InitStructure.ADC_ScanConvMode 				= DISABLE;
	ADC_InitStructure.ADC_ContinuousConvMode 		= ENABLE;
	ADC_InitStructure.ADC_ExternalTrigConvEdge 		= ADC_ExternalTrigConvEdge_None;
	ADC_InitStructure.ADC_DataAlign 				= ADC_DataAlign_Right;
	ADC_InitStructure.ADC_NbrOfConversion 			= 1;
	ADC_Init(ADC1,&ADC_InitStructure);

	// Regular Channel Config
	ADC_RegularChannelConfig(ADC1, ADC_Channel_6, 1, ADC_SampleTime_3Cycles);

	// Enable
	ADC_Cmd(ADC1, ENABLE);

	// ADC2 regular Software Start Conv
	ADC_SoftwareStartConv(ADC1);
}

//*----------------------------------------------------------------------------
//* Function Name       : mchf_board_adc2_init
//* Object              : ADC2 used for forward antenna power
//* Input Parameters    :
//* Output Parameters   :
//* Functions called    :
//*----------------------------------------------------------------------------
static void mchf_board_adc2_init(void)
{
	ADC_InitTypeDef 		ADC_InitStructure;
	ADC_CommonInitTypeDef 	ADC_CommonInitStructure;
	GPIO_InitTypeDef 		GPIO_InitStructure;

	// Enable ADC3 clock
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_ADC2, ENABLE);

	// Configure ADC Channel 6 as analog input
	GPIO_InitStructure.GPIO_Pin 					= ADC2_RET;
	GPIO_InitStructure.GPIO_Mode 					= GPIO_Mode_AIN;
	GPIO_InitStructure.GPIO_PuPd 					= GPIO_PuPd_NOPULL ;
	GPIO_Init(ADC2_RET_PIO, &GPIO_InitStructure);

	// Common Init
	ADC_CommonInitStructure.ADC_Mode 				= ADC_Mode_Independent;
	ADC_CommonInitStructure.ADC_Prescaler 			= ADC_Prescaler_Div8;
	ADC_CommonInitStructure.ADC_DMAAccessMode 		= ADC_DMAAccessMode_Disabled;
	ADC_CommonInitStructure.ADC_TwoSamplingDelay	= ADC_TwoSamplingDelay_5Cycles;
	ADC_CommonInit(&ADC_CommonInitStructure);

	// Configuration
	ADC_StructInit(&ADC_InitStructure);
	ADC_InitStructure.ADC_Resolution 				= ADC_Resolution_12b;
	ADC_InitStructure.ADC_ScanConvMode 				= DISABLE;
	ADC_InitStructure.ADC_ContinuousConvMode 		= ENABLE;
	ADC_InitStructure.ADC_ExternalTrigConvEdge 		= ADC_ExternalTrigConvEdge_None;
	ADC_InitStructure.ADC_DataAlign 				= ADC_DataAlign_Right;
	ADC_InitStructure.ADC_NbrOfConversion 			= 1;
	ADC_Init(ADC2,&ADC_InitStructure);

	// Regular Channel Config
	ADC_RegularChannelConfig(ADC2, ADC_Channel_3, 1, ADC_SampleTime_3Cycles);

	// Enable
	ADC_Cmd(ADC2, ENABLE);

	// ADC2 regular Software Start Conv
	ADC_SoftwareStartConv(ADC2);
}

//*----------------------------------------------------------------------------
//* Function Name       : mchf_board_adc3_init
//* Object              : ADC3 used for return antenna power
//* Input Parameters    :
//* Output Parameters   :
//* Functions called    :
//*----------------------------------------------------------------------------
static void mchf_board_adc3_init(void)
{
	ADC_InitTypeDef 		ADC_InitStructure;
	ADC_CommonInitTypeDef 	ADC_CommonInitStructure;
	GPIO_InitTypeDef 		GPIO_InitStructure;

	// Enable ADC3 clock
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_ADC3, ENABLE);

	// Configure ADC Channel 6 as analog input
	GPIO_InitStructure.GPIO_Pin 					= ADC3_FWD;
	GPIO_InitStructure.GPIO_Mode 					= GPIO_Mode_AIN;
	GPIO_InitStructure.GPIO_PuPd 					= GPIO_PuPd_NOPULL ;
	GPIO_Init(ADC3_FWD_PIO, &GPIO_InitStructure);

	// Common Init
	ADC_CommonInitStructure.ADC_Mode 				= ADC_Mode_Independent;
	ADC_CommonInitStructure.ADC_Prescaler 			= ADC_Prescaler_Div8;
	ADC_CommonInitStructure.ADC_DMAAccessMode 		= ADC_DMAAccessMode_Disabled;
	ADC_CommonInitStructure.ADC_TwoSamplingDelay	= ADC_TwoSamplingDelay_5Cycles;
	ADC_CommonInit(&ADC_CommonInitStructure);

	// Configuration
	ADC_StructInit(&ADC_InitStructure);
	ADC_InitStructure.ADC_Resolution 				= ADC_Resolution_12b;
	ADC_InitStructure.ADC_ScanConvMode 				= DISABLE;
	ADC_InitStructure.ADC_ContinuousConvMode 		= ENABLE;
	ADC_InitStructure.ADC_ExternalTrigConvEdge 		= ADC_ExternalTrigConvEdge_None;
	ADC_InitStructure.ADC_DataAlign 				= ADC_DataAlign_Right;
	ADC_InitStructure.ADC_NbrOfConversion 			= 1;
	ADC_Init(ADC3,&ADC_InitStructure);

	// Regular Channel Config
	ADC_RegularChannelConfig(ADC3, ADC_Channel_2, 1, ADC_SampleTime_3Cycles);

	// Enable
	ADC_Cmd(ADC3, ENABLE);

	// ADC3 regular Software Start Conv
	ADC_SoftwareStartConv(ADC3);
}

//*----------------------------------------------------------------------------
//* Function Name       : mchf_board_power_down_init
//* Object              :
//* Object              :
//* Input Parameters    :
//* Output Parameters   :
//* Functions called    :
//*----------------------------------------------------------------------------
static void mchf_board_power_down_init(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;

	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;

	GPIO_InitStructure.GPIO_Pin = POWER_DOWN;
	GPIO_Init(POWER_DOWN_PIO, &GPIO_InitStructure);

	// Set initial state - low to enable main regulator
	POWER_DOWN_PIO->BSRRH = POWER_DOWN;
}

// Band control GPIOs setup
//
// -------------------------------------------
// 	 BAND		BAND0		BAND1		BAND2
//
//	 80m		1			1			x
//	 40m		1			0			x
//	 20/30m		0			0			x
//	 15-10m		0			1			x
//
// -------------------------------------------
//
static void mchf_board_band_cntr_init(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;

	GPIO_InitStructure.GPIO_Mode 	= GPIO_Mode_OUT;
	GPIO_InitStructure.GPIO_OType 	= GPIO_OType_PP;
	GPIO_InitStructure.GPIO_PuPd 	= GPIO_PuPd_NOPULL;
	GPIO_InitStructure.GPIO_Speed 	= GPIO_Speed_2MHz;

	GPIO_InitStructure.GPIO_Pin = BAND0|BAND1|BAND2;
	GPIO_Init(BAND0_PIO, &GPIO_InitStructure);

	// Set initial state - low (20m band)
	BAND0_PIO->BSRRH = BAND0;
	BAND1_PIO->BSRRH = BAND1;

	// Pulse the latch relays line, active low, so set high to disable
	BAND2_PIO->BSRRL = BAND2;
}

//*----------------------------------------------------------------------------
//* Function Name       : mchf_board_watchdog_init
//* Object              :
//* Object              :
//* Input Parameters    :
//* Output Parameters   :
//* Functions called    :
//*----------------------------------------------------------------------------
static void mchf_board_watchdog_init(void)
{
	// Enable WWDG clock
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_WWDG, ENABLE);

	// WWDG clock counter = (PCLK1 (42MHz)/4096)/8 = 1281 Hz (~780 us)
	WWDG_SetPrescaler(WWDG_Prescaler_8);

	// Set Window value to 80; WWDG counter should be refreshed only when the counter
	//    is below 80 (and greater than 64) otherwise a reset will be generated
	WWDG_SetWindowValue(WD_REFRESH_WINDOW);

	// Enable WWDG and set counter value to 127, WWDG timeout = ~780 us * 64 = 49.92 ms
	// In this case the refresh window is: ~780 * (127-80) = 36.6ms < refresh window < ~780 * 64 = 49.9ms
	// -- so wd reset is every 40 mS --
	// --WWDG_Enable(WD_REFRESH_COUNTER);
}

//*----------------------------------------------------------------------------
//* Function Name       : mchf_board_set_system_tick_value
//* Object              :
//* Object              :
//* Input Parameters    :
//* Output Parameters   :
//* Functions called    :
//*----------------------------------------------------------------------------
static void mchf_board_set_system_tick_value(void)
{
	RCC_ClocksTypeDef 	RCC_Clocks;
//	NVIC_InitTypeDef 	NVIC_InitStructure;

	// Configure Systick clock source as HCLK
	SysTick_CLKSourceConfig(SysTick_CLKSource_HCLK);

	// SystTick configuration
	RCC_GetClocksFreq(&RCC_Clocks);

	// Need 1mS tick for responsive UI
	SysTick_Config(RCC_Clocks.HCLK_Frequency / 1000);

//	NVIC_InitStructure.NVIC_IRQChannel = SysTick_IRQn;
//	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0x0E;
//	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
//	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
//	NVIC_Init(&NVIC_InitStructure);
}

//*----------------------------------------------------------------------------
//* Function Name       : mchf_board_green_led
//* Object              :
//* Object              :
//* Input Parameters    :
//* Output Parameters   :
//* Functions called    :
//*----------------------------------------------------------------------------
void mchf_board_green_led(int state)
{
	switch(state)
	{
		case 1:
			GREEN_LED_PIO->BSRRL = GREEN_LED;
			break;
		case 0:
			GREEN_LED_PIO->BSRRH = GREEN_LED;
			break;
		default:
			GREEN_LED_PIO->ODR ^= GREEN_LED;
			break;
	}
}

//*----------------------------------------------------------------------------
//* Function Name       : mchf_board_red_led
//* Object              :
//* Object              :
//* Input Parameters    :
//* Output Parameters   :
//* Functions called    :
//*----------------------------------------------------------------------------
void mchf_board_red_led(int state)
{
	switch(state)
	{
		case 1:
			RED_LED_PIO->BSRRL = RED_LED;
			break;
		case 0:
			RED_LED_PIO->BSRRH = RED_LED;
			break;
		default:
			RED_LED_PIO->ODR ^= RED_LED;
			break;
	}
}

//*----------------------------------------------------------------------------
//* Function Name       : mchf_board_switch_tx
//* Object              :
//* Object              :
//* Input Parameters    :
//* Output Parameters   :
//* Functions called    :
//*----------------------------------------------------------------------------
void mchf_board_switch_tx(char mode)
{
	if(mode)
	{
		mchf_board_green_led(0);
		mchf_board_red_led(1);

		// TX on (softrock control)
		PTT_CNTR_PIO->BSRRL = PTT_CNTR;

		#ifdef REV_08
		if(get_pcb_rev() == 0x08) PTTN_CNTR_PIO->BSRRH = PTTN_CNTR;
		#endif
	}
	else
	{
		mchf_board_red_led(0);
		mchf_board_green_led(1);

		// RX on
		PTT_CNTR_PIO->BSRRH = PTT_CNTR;

		#ifdef REV_08
		if(get_pcb_rev() == 0x08) PTTN_CNTR_PIO->BSRRL = PTTN_CNTR;
		#endif
	}
}

//*----------------------------------------------------------------------------
//* Function Name       : mchf_board_power_off
//* Object              :
//* Object              :
//* Input Parameters    :
//* Output Parameters   :
//* Functions called    :
//*----------------------------------------------------------------------------
void mchf_board_power_off(void)
{
	ulong i;
	char	tx[32];
	// Power off all - high to disable main regulator
	//

//	Write_VirtEEPROM(EEPROM_FREQ_HIGH,(df.tune_new >> 16));						// Save frequency
//	Write_VirtEEPROM(EEPROM_FREQ_LOW,(df.tune_new & 0xFFFF));					//
//
	UiDriverClearSpectrumDisplay();	// clear display under spectrum scope

	for(i = 0; i < 2; i++)	// Slight delay before we invoke EEPROM write
		non_os_delay();

//	UiDriverSaveEepromValues();		// save EEPROM values

	   sprintf(tx,"                           ");
	   UiLcdHy28_PrintText(80,148,tx,Black,Black,0);

	   sprintf(tx,"       Powering off...     ");
	   UiLcdHy28_PrintText(80,156,tx,Blue2,Black,0);

	   sprintf(tx,"                           ");
	   UiLcdHy28_PrintText(80,168,tx,Blue2,Black,0);


	   sprintf(tx," Saving settings to EEPROM ");
	   UiLcdHy28_PrintText(80,176,tx,Blue,Black,0);

	   sprintf(tx,"            2              ");
	   UiLcdHy28_PrintText(80,188,tx,Blue,Black,0);

	   sprintf(tx,"                           ");
	   UiLcdHy28_PrintText(80,200,tx,Black,Black,0);

	Codec_Mute(1);	// mute audio when powering down

	// Delay before killing power to allow EEPROM write to finish
	//

	for(i = 0; i < 10; i++)
		non_os_delay();
	//

	sprintf(tx,"            1              ");
	UiLcdHy28_PrintText(80,188,tx,Blue,Black,0);

	for(i = 0; i < 10; i++)
		non_os_delay();
	//
	sprintf(tx,"            0              ");
	UiLcdHy28_PrintText(80,188,tx,Blue,Black,0);

	for(i = 0; i < 10; i++)
		non_os_delay();

	ts.powering_down = 1;	// indicate that we should be powering down

	UiDriverSaveEepromValuesPowerDown();		// save EEPROM values again - to make sure...

	//
	// Actual power-down moved to "UiDriverHandlePowerSupply()" with part of delay
	// so that EEPROM write could complete without non_os_delay
	// using the constant "POWERDOWN_DELAY_COUNT" as the last "second" of the delay
	//
	// POWER_DOWN_PIO->BSRRL = POWER_DOWN;
}

//*----------------------------------------------------------------------------
//* Function Name       : mchf_board_init
//* Object              :
//* Object              :
//* Input Parameters    :
//* Output Parameters   :
//* Functions called    :
//*----------------------------------------------------------------------------
void mchf_board_init(void)
{
	// Enable clock on all ports
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA, ENABLE);
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOB, ENABLE);
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOC, ENABLE);
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOD, ENABLE);
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOE, ENABLE);

	// Power up hardware
	mchf_board_power_down_init();

	// Filter control lines
	mchf_board_band_cntr_init();

	// Debugging on
	mchf_board_debug_init();

	// LED init
	mchf_board_led_init();

	// Init keypad hw
	mchf_board_keypad_init();

	// I2C init
	mchf_hw_i2c_init();

	// Get startup frequency of Si570, by DF8OE, 201506
	ui_si570_calc_startupfrequency();

	// Codec control interface
	mchf_hw_i2c2_init();

	// LCD Init
	//UiLcdHy28_Init();

	// Encoders init
	//UiRotaryFreqEncoderInit();
	//UiRotaryEncoderOneInit();
	//UiRotaryEncoderTwoInit();
	//UiRotaryEncoderThreeInit();

	// Init DACs - PA4 used for CS on the API SPI
#ifndef DSP_MODE
	mchf_board_dac0_init();
#endif
	mchf_board_dac1_init();

	// Enable all ADCs
	mchf_board_adc1_init();
	mchf_board_adc2_init();
	mchf_board_adc3_init();

	// Init watchdog - not working
	//mchf_board_watchdog_init();
}

//*----------------------------------------------------------------------------
//* Function Name       : mchf_board_post_init
//* Object              : Extra init, which requires full boot up first
//* Object              :
//* Input Parameters    :
//* Output Parameters   :
//* Functions called    :
//*----------------------------------------------------------------------------
void mchf_board_post_init(void)
{
	// Set system tick interrupt
	// Currently used for UI driver processing only
	mchf_board_set_system_tick_value();

	// Init power button IRQ
	mchf_board_power_button_irq_init();

	// PTT control
	mchf_board_ptt_init();

	// Init keyer interface
	mchf_board_keyer_irq_init();
}



//
// Interface for virtual EEPROM functions and our code
//
uint16_t Read_VirtEEPROM(uint16_t addr, uint16_t *value)	{		// reference to virtual EEPROM read function

	return(EE_ReadVariable(VirtAddVarTab[addr], value));
}

uint16_t Write_VirtEEPROM(uint16_t addr, uint16_t value)	{		// reference to virtual EEPROM write function, writing unsigned 16 bit
	uint16_t	retvar;
//	char	temp[32];

	retvar = (EE_WriteVariable(VirtAddVarTab[addr], value));

//	sprintf(temp, "Wstat=%d ", retvar);		// Debug indication of write status
//	UiLcdHy28_PrintText((POS_PB_IND_X + 32),(POS_PB_IND_Y + 1), temp,White,Black,0);

	return retvar;
}

uint16_t Write_VirtEEPROM_Signed(uint16_t addr, int value)	{		// reference to virtual EEPROM write function, writing signed integer
	uint16_t	*u_var;
	uint16_t	retvar;
//	char	temp[32];

	u_var = (uint16_t *)&value;
	retvar = (EE_WriteVariable(VirtAddVarTab[addr], *u_var));

//	sprintf(temp, "Wstat=%d ", retvar);		// Debug indication of write status
//	UiLcdHy28_PrintText((POS_PB_IND_X + 32),(POS_PB_IND_Y + 1), temp,White,Black,0);

	return retvar;
}

#endif
