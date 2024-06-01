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
#ifndef __MCHF_PRO_PINMAP_H
#define __MCHF_PRO_PINMAP_H

// -----------------------------------------
// What board are we running on ?
//
// Define in Project preprocessor!
//
// DISCO EVAL board with huge BGA chip:		BOARD_EVAL_747
// Release board v 0.9.0:					BOARD_MCHF_PRO
// Codec Proto board:						BOARD_TEST_CODEC
// BMS Proto board:							BOARD_TEST_BMS

// PCB revisions, here or project file ?
#define REV_8_2

// ----------------------------------------------------------------------
// Pin map for EVAL board
#ifdef  BOARD_EVAL_747
#define LCD_RESET_PIN                    GPIO_PIN_3
#define LCD_RESET_PULL                   GPIO_NOPULL
#define LCD_RESET_GPIO_PORT              GPIOG
//#define LCD_RESET_GPIO_CLK_ENABLE()      __HAL_RCC_GPIOG_CLK_ENABLE()
//#define LCD_RESET_GPIO_CLK_DISABLE()     __HAL_RCC_GPIOG_CLK_DISABLE()

/* LCD tearing effect pin */
#define LCD_TE_PIN                       GPIO_PIN_2
#define LCD_TE_GPIO_PORT                 GPIOJ
//#define LCD_TE_GPIO_CLK_ENABLE()         __HAL_RCC_GPIOJ_CLK_ENABLE()
//#define LCD_TE_GPIO_CLK_DISABLE()        __HAL_RCC_GPIOJ_CLK_DISABLE()

/* Back-light control pin */
#define LCD_BL_CTRL_PIN                  GPIO_PIN_12
#define LCD_BL_CTRL_GPIO_PORT            GPIOJ
//#define LCD_BL_CTRL_GPIO_CLK_ENABLE()    __HAL_RCC_GPIOJ_CLK_ENABLE()
//#define LCD_BL_CTRL_GPIO_CLK_DISABLE()   __HAL_RCC_GPIOJ_CLK_DISABLE()
//
#endif
// ----------------------------------------------------------------------
// Pin map of final board
#ifdef  BOARD_MCHF_PRO
#define LCD_RESET_PIN                    GPIO_PIN_7
#define LCD_RESET_PULL                   GPIO_NOPULL
#define LCD_RESET_GPIO_PORT              GPIOH
//#define LCD_RESET_GPIO_CLK_ENABLE()      __HAL_RCC_GPIOG_CLK_ENABLE()
//#define LCD_RESET_GPIO_CLK_DISABLE()     __HAL_RCC_GPIOG_CLK_DISABLE()

/* LCD tearing effect pin */
//#define LCD_TE_PIN                       GPIO_PIN_15
//#define LCD_TE_GPIO_PORT                 GPIOI
//#define LCD_TE_GPIO_CLK_ENABLE()         __HAL_RCC_GPIOJ_CLK_ENABLE()
//#define LCD_TE_GPIO_CLK_DISABLE()        __HAL_RCC_GPIOJ_CLK_DISABLE()

/* Back-light control pin */
#define LCD_BL_CTRL_PIN                  GPIO_PIN_1
#define LCD_BL_CTRL_GPIO_PORT            GPIOB
//#define LCD_BL_CTRL_GPIO_CLK_ENABLE()    __HAL_RCC_GPIOJ_CLK_ENABLE()
//#define LCD_BL_CTRL_GPIO_CLK_DISABLE()   __HAL_RCC_GPIOJ_CLK_DISABLE()
//
#endif
#ifdef  BOARD_MCHF_PRO
#define PIN_001_PE2						GPIO_PIN_2
#define PIN_001_PORT					GPIOE
//
#define PIN_002_PE3						GPIO_PIN_3
#define PIN_002_PORT					GPIOE
//
#endif

// -----------------------------------------------------------------------
//			Differences between PCB rev 0.8.1 and 0.8.2
// -----------------------------------------------------------------------
//	PCB Net Name			8.1			8.2			Notes
//
// 	POWER_BUTTON			NA			PC13		new circuit
//
// 	ESP_GPIO0				PC15		PD7
//
// 	CODEC_RESET				PC14		PD6
//
// 	MUTE					NA			PD4			new circuit
//
// 	BAL4_ON					PD7			PG9
//
// 	BAND0					PC13		PD3
//
// 	BAND2					PB11		PI15
//
// 	DISCH_ON				PD6			NA			moved to ESP32
//
// 	CHGR_ON					PD4			NA			moved to ESP32
//
// 	CC_CV					PD3			NA			moved to ESP32
//
//	VCC_8V_ON				PG9			NA			HW control(5V INHIBIT)
//
//	DSI_TE					PI15		PB11		use MUX signal
//
// ------------------------------------------------------------------------
#ifdef REV_8_2

// Balancer
#define BAL1_ON							GPIO_PIN_6
#define BAL2_ON							GPIO_PIN_3
#define BAL3_ON							GPIO_PIN_2
#define BAL13_PORT            			GPIOG

#define BAL4_ON							GPIO_PIN_9
#define BAL4_PORT            			GPIOG

// Charger control
// - Net name is I2S2_MCK
#define CHGR_ON							GPIO_PIN_6
#define CHGR_ON_PORT           			GPIOC
// - Net name is I2S2_SDO
#define CC_CV							GPIO_PIN_15
#define CC_CV_PORT           			GPIOB

// Codec
#define CODEC_RESET						GPIO_PIN_6
#define CODEC_RESET_PORT           		GPIOD

#define CODEC_MUTE						GPIO_PIN_4
#define CODEC_MUTE_PORT           		GPIOD

#define BAND0_PIN						GPIO_PIN_3
#define BAND0_PORT						GPIOD
#define BAND1_PIN						GPIO_PIN_5
#define BAND1_PORT						GPIOB
#define BAND2_PIN						GPIO_PIN_15
#define BAND2_PORT						GPIOI
#define BAND3_PIN						GPIO_PIN_1
#define BAND3_PORT						GPIOH

// ESP32 Reset line is PG14
#define ESP_RESET						GPIO_PIN_14
#define ESP_RESET_PORT					GPIOG

// ESP32 Power is PI11
#define ESP_POWER						GPIO_PIN_11
#define ESP_POWER_PORT					GPIOI

// ESP32 IO0 is PD7
#define ESP_GPIO0						GPIO_PIN_7
#define ESP_GPIO0_PORT					GPIOD

// SD Card detect
#define SD_DET                   		GPIO_PIN_2
#define SD_DET_PORT              		GPIOC

// SD Card power
#define SD_PWR_CNTR                   	GPIO_PIN_13
#define SD_PWR_CNTR_PORT              	GPIOB

// SD Card SDIO interface
#define SDMMC1_D0                   	GPIO_PIN_8
#define SDMMC1_D1                   	GPIO_PIN_9
#define SDMMC1_D2                   	GPIO_PIN_10
#define SDMMC1_D3                   	GPIO_PIN_11
#define SDMMC1_CLK                   	GPIO_PIN_12
#define SDMMC1_SDIO_PORTC              	GPIOC
#define SDMMC1_CMD                   	GPIO_PIN_2
#define SDMMC1_SDIO_PORTD              	GPIOD

#define TS_INT_PIN                        ((uint32_t)GPIO_PIN_6)
#define TS_INT_GPIO_PORT                  ((GPIO_TypeDef*)GPIOH)

#else

// Balancer
#define BAL1_ON							GPIO_PIN_6
#define BAL2_ON							GPIO_PIN_3
#define BAL3_ON							GPIO_PIN_2
#define BAL13_PORT            			GPIOG

#define BAL4_ON							GPIO_PIN_7
#define BAL4_PORT            			GPIOD

// Codec
#define CODEC_RESET						GPIO_PIN_14
#define CODEC_RESET_PORT           		GPIOC

//
// This mod swaps M16 and M36, so DSI TE signal
// from the MIPI PHY goes to LCD (future use of Command mode)
//
//#define M16_MOD

#define BAND0_PIN			GPIO_PIN_13
#define BAND0_PORT			GPIOC

#define BAND1_PIN			GPIO_PIN_5
#define BAND1_PORT			GPIOB

#ifndef M16_MOD
#define BAND2_PIN			GPIO_PIN_11
#define BAND2_PORT			GPIOB
#else
#define BAND2_PIN			GPIO_PIN_15
#define BAND2_PORT			GPIOI
#endif

#define BAND3_PIN			GPIO_PIN_1
#define BAND3_PORT			GPIOH

// Reset line is PG14
#define ESP_RESET				GPIO_PIN_14
#define ESP_RESET_PORT			GPIOG

// Power is PI11
#define ESP_POWER				GPIO_PIN_11
#define ESP_POWER_PORT			GPIOI

// IO0 is PC15
#define ESP_GPIO0				GPIO_PIN_15
#define ESP_GPIO0_PORT			GPIOC

#endif

// Regulator control (5V and 8V)
#define VCC_5V_ON						GPIO_PIN_10
#define VCC_5V_ON_PORT            		GPIOG

// Power hold (high level keeps 3V regulator on)
#define POWER_HOLD						GPIO_PIN_13
#define POWER_HOLD_PORT            		GPIOC

// Power button
#define POWER_BUTTON					GPIO_PIN_11
#define POWER_BUTTON_PORT            	GPIOG
//
//#define BUTTON_WAKEUP_PIN				GPIO_PIN_13
//#define BUTTON_WAKEUP_GPIO_PORT			GPIOC

// Power LED (also ambient light sensor, ADC3 input)
#define POWER_LED						GPIO_PIN_6
#define POWER_LED_PORT            		GPIOF

#endif
