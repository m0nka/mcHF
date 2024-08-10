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
//#define REV_8_2
#define REV_8_4

#ifdef REV_8_2
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
#define LCD_BL_CTRL_PIN                  GPIO_PIN_9
#define LCD_BL_CTRL_GPIO_PORT            GPIOA
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

#ifdef REV_8_4

// ----------------------------------------------------
// ----------------------------------------------------
// PortA

// PA0			RFM_RST

// PA1			ENC1_I
#define ENC1_I							GPIO_PIN_1
#define ENC1_I_PORT						GPIOA

// PA2			LORA_POWER
// PA3			RFM_DIO2

// PA4			DAC1_OUT1
#define DAC1_OUT1						GPIO_PIN_4
#define DAC1_OUTX_PORT					GPIOA

// PA5			DAC1_OUT2
#define DAC1_OUT2						GPIO_PIN_5

// PA6			RFM_MISO_SPI1
// PA7			RFM_MOSI_SPI1
// PA8			CLK_42M
// PA9			BMS_IRQ

// PA10			FAN_ON
#define FAN_CNTR						GPIO_PIN_10
#define FAN_CNTR_PORT					GPIOA

// PA11			USB_DFU_N
// PA12			USB_DFU_P
// PA13			SWDIO
// PA14			SWCLK

// PA15			ENC1_Q
#define ENC1_Q							GPIO_PIN_15
#define ENC1_Q_PORT						GPIOA

// ----------------------------------------------------
// ----------------------------------------------------
// PortB

// PB0			CODEC_RESET
#define CODEC_RESET						GPIO_PIN_0
#define CODEC_RESET_PORT           		GPIOB

// PB1			LCD_BL_CTRL
#define LCD_BL_CTRL_PIN               	GPIO_PIN_1
#define LCD_BL_CTRL_GPIO_PORT         	GPIOB

// PB2			ATT_CLK
// PB3			RFM_SCK_SPI1

// PB4			ENC2_I
#define ENC2_I_PIN               		GPIO_PIN_4
#define ENC2_I_PORT               		GPIOB

// PB5			KEYPAD_X5
// PB6			TOUCH_SCK_I2C1
// PB7			TOUCH_SDA_I2C1

// PB8			CODEC_SCL_I2C4
#define CODEC_SCL_I2C4_PIN             	GPIO_PIN_8
#define CODEC_SCL_I2C4_PORT           	GPIOB
#define CODEC_SCL_I2C4_AF            	GPIO_AF6_I2C4

// PB9			CODEC_SDA_I2C4
#define CODEC_SDA_I2C4_PIN           	GPIO_PIN_9
#define CODEC_SDA_I2C4_PORT          	GPIOB
#define CODEC_SDA_I2C4_AF            	GPIO_AF6_I2C4

// PB10			ATT_LE
// PB11			DSI_TE
// PB12			PTT
// PB13			VCC_5V_ON
#define VCC_5V_ON						GPIO_PIN_13
#define VCC_5V_ON_PORT            		GPIOB

// PB14			OTG_HS_DM
// PB15			OTG_HS_DP

// ----------------------------------------------------
// ----------------------------------------------------
// PortC

// PC0			FMC_SDNWE
// PC1			RFM_NSS

// PC2			MUTE
#define CODEC_MUTE						GPIO_PIN_2
#define CODEC_MUTE_PORT           		GPIOC

// PC3			ADC3_INP1
// PC4			RFM_DIO1
// PC5			RFM_DIO0

// PC6			BAND0
#define BAND0_PIN						GPIO_PIN_6
#define BAND0_PORT						GPIOC

// PC7			ENC2_Q
#define ENC2_Q_PIN               		GPIO_PIN_7
#define ENC2_Q_PORT               		GPIOC

// PC8			SDMMC1_D0
#define SDMMC1_D0                   	GPIO_PIN_8
#define SDMMC1_SDIO_PORTC              	GPIOC

// PC9			SDMMC1_D1
#define SDMMC1_D1                   	GPIO_PIN_9

// PC10			SDMMC1_D2
#define SDMMC1_D2                   	GPIO_PIN_10

// PC11			SDMMC1_D3
#define SDMMC1_D3                   	GPIO_PIN_11

// PC12			SDMMC1_CK
#define SDMMC1_CLK                   	GPIO_PIN_12

// PC13			POWER_HOLD
#define POWER_HOLD						GPIO_PIN_13
#define POWER_HOLD_PORT            		GPIOC

// ----------------------------------------------------
// ----------------------------------------------------
// PortD

// PD0			FMC_D2
// PD1			FMC_D3
// PD2			SDMMC1_CMD
#define SDMMC1_CMD                   	GPIO_PIN_2
#define SDMMC1_SDIO_PORTD              	GPIOD

// PD3			KEYPAD_X1
// PD4			SD_DET
#define SD_DET                   		GPIO_PIN_4
#define SD_DET_PORT              		GPIOD

// PD5			USART2_TX
// PD6			KEYPAD_Y1
// PD7			KEYPAD_X2
// PD8			FMC_D13
// PD9			FMC_D14
// PD10			FMC_D15
// PD11			ATT_DATA
// PD12			FREE3
// PD13			FREE5
// PD14			FMC_D0
// PD15			FMC_D1

// ----------------------------------------------------
// ----------------------------------------------------
// PortE

// PE0			FMC_NBL0
// PE1			FMC_NBL1
// PE2			FREE4
// PE3			SAI1_SD_B
// PE4			SAI1_FS_A
// PE5			SAI1_SCK_A
// PE6			SAI1_SD_B
// PE7			FMC_D4
// PE8			FMC_D5
// PE9			FMC_D6
// PE10			FMC_D7
// PE11			FMC_D8
// PE12			FMC_D9
// PE13			FMC_D10
// PE14			FMC_D11
// PE15			FMC_D12

// ----------------------------------------------------
// ----------------------------------------------------
// PortF

// PF0			FMC_A0
// PF1			FMC_A1
// PF2			FMC_A2
// PF3			FMC_A3
// PF4			FMC_A4
// PF5			FMC_A5
// PF6			ADC3_INP8
#define POWER_LED						GPIO_PIN_6
#define POWER_LED_PORT            		GPIOF

// PF7			ADC3_INP3
// PF8			SAI1_SCK_B
// PF9			SAI1_FS_B
// PF10			ADC3_INP6
// PF11			FMC_SDNRAS
// PF12			FMC_A6
// PF13			FMC_A7
// PF14			FMC_A8
// PF15			FMC_A9

// ----------------------------------------------------
// ----------------------------------------------------
// PortG

// PG0			FMC_A10
// PG1			FMC_A11
// PG2			DIT_IRQ
#define PADDLE_DIT_PIO					GPIOG
#define PADDLE_DIT						LL_GPIO_PIN_2

// PG3			DAH_IRQ
#define PADDLE_DAH_PIO 					GPIOG
#define PADDLE_DAH						LL_GPIO_PIN_3

// PG4			FMC_BA0
// PG5			FMC_BA1

// PG6			BAND1
#define BAND1_PIN						GPIO_PIN_6
#define BAND1_PORT						GPIOG

// PG7			SAI1_MCLK_A
// PG8			FMC_SDCLK
// PG9			KEYPAD_Y2

// PG10		SD_PWR_CNTR
#define SD_PWR_CNTR                   	GPIO_PIN_10
#define SD_PWR_CNTR_PORT              	GPIOG

// PG11		POWER_BUTTON
#define POWER_BUTTON					GPIO_PIN_11
#define POWER_BUTTON_PORT            	GPIOG

// PG12		KEYPAD_X3
// PG13		KEYPAD_X4
// PG14		KEYPAD_Y3
// PG15		FMC_SDNCAS

// ----------------------------------------------------
// ----------------------------------------------------
// PortH

// PH1			BAND3
#define BAND3_PIN						GPIO_PIN_1
#define BAND3_PORT						GPIOH

// PH2			FMC_SDCKE0
// PH3			FMC_SDNE0
// PH4			LO_SCL_I2C2
// PH5			LO_SDA_I2C2

// PH6			TOUCH_INT
#define TS_INT_PIN                 		GPIO_PIN_6
#define TS_INT_GPIO_PORT            	GPIOH

// PH7			DSI_RESET
#define LCD_RESET_PIN             		GPIO_PIN_7
#define LCD_RESET_PULL              	GPIO_NOPULL
#define LCD_RESET_GPIO_PORT         	GPIOH

// PH8			FMC_D16
// PH9			FMC_D17
// PH10			FMC_D18
// PH11			FMC_D19
// PH12			FMC_D20
// PH13			FMC_D21
// PH14			FMC_D22
// PH15			FMC_D23

// ----------------------------------------------------
// ----------------------------------------------------
// PortI

// PI0			FMC_D24
// PI1			FMC_D25
// PI2			FMC_D26
// PI3			FMC_D27
// PI4			FMC_NBL2
// PI5			FMC_NBL3
// PI6			FMC_D28
// PI7			FMC_D29
// PI8			KEYPAD_X6
// PI9			FMC_D30
// PI10			FMC_D31
// PI11			KEYPAD_Y4

// PI15			BAND2
#define BAND2_PIN						GPIO_PIN_15
#define BAND2_PORT						GPIOI

#endif

#endif
