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
**  Licence:                                                                       **
************************************************************************************/
#ifndef __BMS_PROC_H
#define __BMS_PROC_H

#include "stm32h7xx_hal.h"
#include "stm32h7xx_hal_gpio.h"
#include "stm32h7xx_hal_rcc.h"
#include "stm32h7xx_hal_rcc_ex.h"
#include "stm32h7xx_hal_adc.h"

// Somewhat this value should allow to detect if the radio
// is running on dc or batteries
#define PACK_CURR_THRSH						-50

//#define USE_BALANCER
#define BALANCER_FREQ						30

#define USE_OVERSAMPLER

#define NORMAL_PROC_REFRESH					80
#define CHARGE_PROC_REFRESH					10

#define CELL_MIN_VALUE						2600
#define CELL_MAX_VALUE						4100

#define ADC_MIN_VALUE						1900

#define CHARGER_MIN_VOLTAGE					15000

#define ADCx_CLK_ENABLE()               	__HAL_RCC_ADC12_CLK_ENABLE()

#define ADCx_FORCE_RESET()              	__HAL_RCC_ADC12_FORCE_RESET()
#define ADCx_RELEASE_RESET()            	__HAL_RCC_ADC12_RELEASE_RESET()

// PA6, ADC1_INP3 - CELL1
#define ADCx_CELL1_PIN          	      	GPIO_PIN_6
#define ADCx_CELL1_GPIO_PORT          		GPIOA
#define ADCx_CH_CELL1                    	ADC_CHANNEL_3

// PC4, ADC1_INP4 - CELL2 BRANCH
#define ADCx_CELL2_PIN      	          	GPIO_PIN_4
#define ADCx_CELL2_GPIO_PORT          		GPIOC
#define ADCx_CH_CELL2                    	ADC_CHANNEL_4

// PB1, ADC1_INP5 - CELL3 BRANCH
#define ADCx_CELL3_PIN    	            	GPIO_PIN_1
#define ADCx_CELL3_GPIO_PORT          		GPIOB
#define ADCx_CH_CELL3                    	ADC_CHANNEL_5

// PA7, ADC1_INP7 - CELL4 BRANCH
#define ADCx_CELL4_PIN	                	GPIO_PIN_7
#define ADCx_CELL4_GPIO_PORT	          	GPIOA
#define ADCx_CH_CELL4                    	ADC_CHANNEL_7

// PB0, ADC1_INP9 - CHARGER
#define ADCx_CHARGE_PIN	                	GPIO_PIN_0
#define ADCx_CHARGE_GPIO_PORT	          	GPIOB
#define ADCx_CH_CHARGE                    	ADC_CHANNEL_9

// PC5, ADC1_INP8 - LOAD
#define ADCx_LOAD_PIN	                	GPIO_PIN_5
#define ADCx_LOAD_GPIO_PORT		          	GPIOC
#define ADCx_CH_LOAD                    	ADC_CHANNEL_8

// PC1, ADC1_INP11 - NTC CELL1
#define ADCx_TEMP1_PIN	                	GPIO_PIN_5
#define ADCx_TEMP1_GPIO_PORT		        GPIOC
#define ADCx_CH_TEMP1                    	ADC_CHANNEL_11

// PA2, ADC1_INP14 - NTC CELL2
#define ADCx_TEMP2_PIN	                	GPIO_PIN_2
#define ADCx_TEMP2_GPIO_PORT		        GPIOA
#define ADCx_CH_TEMP2                    	ADC_CHANNEL_14

// PA3, ADC1_INP15 - NTC CELL3
#define ADCx_TEMP3_PIN	                	GPIO_PIN_3
#define ADCx_TEMP3_GPIO_PORT		        GPIOA
#define ADCx_CH_TEMP3                    	ADC_CHANNEL_15

// PA0, ADC1_INP16 - NTC CELL4
#define ADCx_TEMP4_PIN	                	GPIO_PIN_0
#define ADCx_TEMP4_GPIO_PORT		        GPIOA
#define ADCx_CH_TEMP4                    	ADC_CHANNEL_16

#define ADC_CHANNELS_CNT				 	10

#ifdef USE_OVERSAMPLER
#define ADC_SAMPLINGTIME                    ADC_SAMPLETIME_64CYCLES_5
#define ADC_RESOLUTION						0xFFFF
//
// https://community.st.com/s/question/0D50X0000BTdvHp/how-to-do-adc-averaging-in-stm32h7-mcu-using-hardware-over-sampling
//
#define OVERSAMPLING_RATIO              	1024
#define RIGHTBITSHIFT                   	ADC_RIGHTBITSHIFT_10
#define TRIGGEREDMODE                   	ADC_TRIGGEREDMODE_SINGLE_TRIGGER
#define OVERSAMPLINGSTOPRESET           	ADC_REGOVERSAMPLING_CONTINUED_MODE

#else
#define ADC_SAMPLINGTIME                    ADC_SAMPLETIME_810CYCLES_5
#define ADC_RESOLUTION						0xFFFF
#endif

#define ADC_EXT_VREF_mV						3390

#define ADC_POLLING_WAIT					50

#define ROUND_DIVIDE(numer, denom) (((numer) + (denom) / 2) / (denom))

__attribute__((__common__)) struct BMSState {

	ulong a[10];		// adc channel voltage
	ulong s[4];		// branch voltage
	ulong c[4];		// calculated cell voltage
	ulong e[4];		// channel errors, accumulated
	ulong t[4];		// cell temperature

	ulong ba[4];	// balancer accumulator

	//ulong k[4];
	//ulong f[4];	// filtered values (long averaging)

	ulong chgr;
	ulong load;
	ulong curr;

	ulong t_err;	// total error, last cycle

	ulong ulCH1;
	ulong ulCH2;
	ulong ulCH3;
	ulong ulCH4;
	ulong ulCH5;
	ulong ulCH6;
	ulong ulCH8;
	ulong ulCH9;
	ulong ulCH10;
	ulong ulCH11;

	ulong usBalID[4];
	ulong usCV[4];
	ulong usChargeValue;
	ulong usLoadValue;

	short cal_adc[10];	// calibration value for ADC mV trimming
	short cal_res[10];	// calibration value for resistor divider trimming

	short vref;

	// Reading ready
	uchar rr;

	//uchar lac;		// filter lenght

	uchar perc;			// % value of SOC left
	ushort mins;

	uchar charger_on;
	uchar h_prot_on;
	uchar run_on_dc;

} BMSState;

void bms_proc_hw_init(void);
void bms_proc_power_cleanup(void);
void bms_proc_task(void const *arg);

#endif


