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
#ifndef __MCHF_PRO_PINMAP_H
#define __MCHF_PRO_PINMAP_H


#define REV_8_5

// Changes between 0.8.4 and 0.8.5
//
//	NET					0.8.4		0.8.5
//	--------------------------------------
//	LCD_BL_CTRL			PB1			PA9
//	BST_EN				---			PB1
//	BMS_IRQ				PA9			---
//
//	CLK_42M				PA8			---
//	BMS_PWM				---			PA8
//
//	FREE3				PD12		---
//	FREE5				PD13		---
//	BMS_SCL				PB8			PD12
//	BMS_SDA				PB9			PD13
//
#ifdef REV_8_5

// ----------------------------------------------------
// ----------------------------------------------------
// PortA

// PA0			RFM_RST, Lora driver
#define RFM_RST							LL_GPIO_PIN_0
#define RFM_RST_PORT					GPIOA

// PA1			ENC1_I
#define ENC1_I							GPIO_PIN_1
#define ENC1_I_PORT						GPIOA

// PA2			LORA_POWER, Lora driver
#define LORA_POWER						LL_GPIO_PIN_2
#define LORA_POWER_PORT					GPIOA

// PA3			RFM_DIO2, was Lora driver, now BT module power control
#define RFM_DIO2						GPIO_PIN_3
#define RFM_DIO2_PORT					GPIOA

// PA4			DAC1_OUT1
#define DAC1_OUT1						GPIO_PIN_4
#define DAC1_OUTX_PORT					GPIOA

// PA5			DAC1_OUT2
#define DAC1_OUT2						GPIO_PIN_5

// PA6			RFM_MISO_SPI1, Lora driver
#define RFM_MISO_SPI1					LL_GPIO_PIN_6
#define RFM_MISO_SPI1_PORT				GPIOA

// PA7			RFM_MOSI_SPI1, Lora driver
#define RFM_MOSI_SPI1					LL_GPIO_PIN_7
#define RFM_MOSI_SPI1_PORT				GPIOA

// PA8			BMS_PWM -> ToDo: BMS CC control
#define BMS_PWM_PIN               		GPIO_PIN_8
#define BMS_PWM_PORT         			GPIOA

// PA9			LCD_BL_CTRL
#define LCD_BL_CTRL_PIN               	GPIO_PIN_9
#define LCD_BL_CTRL_GPIO_PORT         	GPIOA

// PA10			FAN_ON
#define FAN_CNTR						GPIO_PIN_10
#define FAN_CNTR_PORT					GPIOA

// ToDo: Allocate to DSP core for rig control and real time audio streaming
// PA11			USB_DFU_N
// PA12			USB_DFU_P

// Programming port only
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

// PB1			BST_EN -> ToDo: BMS step-up converter enable
#define BST_EN		               		GPIO_PIN_1
#define BST_EN_PORT         			GPIOB

// PB2			ATT_CLK, Attenuator
#define ATT_CLK               			LL_GPIO_PIN_2
#define ATT_CLK_PORT         			GPIOB

// PB3			RFM_SCK_SPI1, Lora driver
#define RFM_SCK_SPI1               		LL_GPIO_PIN_3
#define RFM_SCK_SPI1_PORT         		GPIOB

// PB4			ENC2_I
#define ENC2_I_PIN               		GPIO_PIN_4
#define ENC2_I_PORT               		GPIOB

// PB5			KEYPAD_X5
#define KEYPAD_X5_LL                   	LL_GPIO_PIN_5
#define KEYPAD_X5_PORT               	GPIOB

// PB6			TOUCH_SCK_I2C1
#define TOUCH_SCK_SCL_PIN              	GPIO_PIN_6
#define TOUCH_SCK_SCL_GPIO_PORT      	GPIOB
#define TOUCH_SCK_SCL_AF             	GPIO_AF4_I2C4

// PB7			TOUCH_SDA_I2C1
#define TOUCH_SDA_SDA_PIN             	GPIO_PIN_7
#define TOUCH_SDA_SDA_AF                GPIO_AF4_I2C4
#define TOUCH_SDA_SDA_GPIO_PORT         GPIOB

// PB8			CODEC_SCL_I2C4
#define CODEC_SCL_I2C4_PIN             	GPIO_PIN_8
#define CODEC_SCL_I2C4_PORT           	GPIOB
#define CODEC_SCL_I2C4_AF            	GPIO_AF6_I2C4

// PB9			CODEC_SDA_I2C4
#define CODEC_SDA_I2C4_PIN           	GPIO_PIN_9
#define CODEC_SDA_I2C4_PORT          	GPIOB
#define CODEC_SDA_I2C4_AF            	GPIO_AF6_I2C4

// PB10			ATT_LE, latch enable, Attenuator
#define ATT_LE							LL_GPIO_PIN_10
#define ATT_LE_PORT            			GPIOB

// PB11			DSI_TE - reserved, future use
#define DSI_TE							GPIO_PIN_11
#define DSI_TE_PORT            			GPIOB

// PB12			PTT
#define PTT_PIN							GPIO_PIN_12
#define PTT_PIN_PORT            		GPIOB

// PB13			VCC_5V_ON
#define VCC_5V_ON						GPIO_PIN_13
#define VCC_5V_ON_PORT            		GPIOB

// Physical keyboard ? Allocate to DPS core ? Bootrom USB stick support ?
// PB14			OTG_HS_DM
// PB15			OTG_HS_DP

// ----------------------------------------------------
// ----------------------------------------------------
// PortC

// PC0			FMC_SDNWE
//
//

// PC1			RFM_NSS, Lora driver
#define RFM_NSS							LL_GPIO_PIN_1
#define RFM_NSS_PORT           			GPIOC

// PC2			MUTE
#define CODEC_MUTE						GPIO_PIN_2
#define CODEC_MUTE_PORT           		GPIOC

// PC3			ADC3_INP1, Reflected Power, from bridge
#define ADC3_INP1						LL_GPIO_PIN_3
#define ADC3_INP1_PORT           		GPIOC

// PC4			RFM_DIO1, Lora driver
#define RFM_DIO1						LL_GPIO_PIN_4
#define RFM_DIO1_PORT           		GPIOC

// PC5			RFM_DIO0, Lora driver
#define RFM_DIO0						LL_GPIO_PIN_5
#define RFM_DIO0_PORT           		GPIOC

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
#define POWER_HOLD						LL_GPIO_PIN_13
#define POWER_HOLD_PORT            		GPIOC

// ----------------------------------------------------
// ----------------------------------------------------
// PortD

// PD0			FMC_D2
//
//

// PD1			FMC_D3
//
//

// PD2			SDMMC1_CMD
#define SDMMC1_CMD                   	GPIO_PIN_2
#define SDMMC1_SDIO_PORTD              	GPIOD

// PD3			KEYPAD_X1
#define KEYPAD_X1_LL                   	LL_GPIO_PIN_3
#define KEYPAD_X1_PORT              	GPIOD

// PD4			SD_DET
#define SD_DET                   		GPIO_PIN_4
#define SD_DET_PORT              		GPIOD

// PD5			USART2_TX - printf, on USB-C connector
#define USART2_TX                   	GPIO_PIN_5
#define USART2_TX_PORT              	GPIOD
#define USART2_TX_AF                    GPIO_AF7_USART2

// PD6		KEYPAD_X4
#define KEYPAD_X4_LL                   	LL_GPIO_PIN_6
#define KEYPAD_X4_PORT              	GPIOD

// PD7			KEYPAD_X2
#define KEYPAD_X2_LL                   	LL_GPIO_PIN_7
#define KEYPAD_X2_PORT              	GPIOD

// PD8			FMC_D13
//
//

// PD9			FMC_D14
//
//

// PD10			FMC_D15
//
//

// PD11			ATT_DATA, Attenuator
#define ATT_DATA                   		LL_GPIO_PIN_11
#define ATT_DATA_PORT              		GPIOD

// PD12			BMS_SCL
#define BMS_SCL_PIN                   	GPIO_PIN_12
#define BMS_SCL_PORT              		GPIOD

// PD13			BMS_SDA
#define BMS_SDA_PIN                   	GPIO_PIN_13
#define BMS_SDA_PORT              		GPIOD

// PD14			FMC_D0
//
//

// PD15			FMC_D1
//
//

// ----------------------------------------------------
// ----------------------------------------------------
// PortE

// PE0			FMC_NBL0
//
//

// PE1			FMC_NBL1
//
//

// PE2			BT_CONNECT_STATUS
#define BT_CONNECT_STATUS				GPIO_PIN_2
#define BT_CONNECT_STATUS_PORT          GPIOE

// PE3			SAI1_SD_B - in DSP core!!!
//
//

// PE4			SAI1_FS_A
//
//

// PE5			SAI1_SCK_A
//
//

// PE6			SAI1_SD_B
//
//

// PE7			FMC_D4
//
//

// PE8			FMC_D5
//
//

// PE9			FMC_D6
//
//

// PE10			FMC_D7
//
//

// PE11			FMC_D8
//
//

// PE12			FMC_D9
//
//

// PE13			FMC_D10
//
//

// PE14			FMC_D11
//
//

// PE15			FMC_D12
//
//

// ----------------------------------------------------
// ----------------------------------------------------
// PortF

// PF0			FMC_A0
//
//

// PF1			FMC_A1
//
//

// PF2			FMC_A2
//
//

// PF3			FMC_A3
//
//

// PF4			FMC_A4
//
//

// PF5			FMC_A5
//
//

// PF6			ADC3_INP8, Ambient light sensor
#define POWER_LED						LL_GPIO_PIN_6
#define POWER_LED_PORT            		GPIOF

// PF7			ADC3_INP3, PA Temperature
#define ADC3_INP3						LL_GPIO_PIN_7
#define ADC3_INP3_PORT            		GPIOF

// PF8			SAI1_SCK_B
//
//

// PF9			SAI1_FS_B
//
//

// PF10			ADC3_INP6, Forward Power
#define ADC3_INP6						LL_GPIO_PIN_10
#define ADC3_INP6_PORT            		GPIOF

// PF11			FMC_SDNRAS
//
//

// PF12			FMC_A6
//
//

// PF13			FMC_A7
//
//

// PF14			FMC_A8
//
//

// PF15			FMC_A9
//
//

// ----------------------------------------------------
// ----------------------------------------------------
// PortG

// PG0			FMC_A10
//
//

// PG1			FMC_A11
//
//

// PG2			DIT_IRQ
#define PADDLE_DIT_PIO					GPIOG
#define PADDLE_DIT						GPIO_PIN_2
#define PADDLE_DIT_LL					LL_GPIO_PIN_2

// PG3			DAH_IRQ
#define PADDLE_DAH_PIO 					GPIOG
#define PADDLE_DAH						GPIO_PIN_3
#define PADDLE_DAH_LL					LL_GPIO_PIN_3

// PG4			FMC_BA0
//
//

// PG5			FMC_BA1
//
//

// PG6			BAND1
#define BAND1_PIN						GPIO_PIN_6
#define BAND1_PORT						GPIOG

// PG7			SAI1_MCLK_A
//
//

// PG8			FMC_SDCLK
//
//

// PG9		KEYPAD_X3
#define KEYPAD_X3_LL                   	LL_GPIO_PIN_9
#define KEYPAD_X3_PORT              	GPIOG

// PG10		SD_PWR_CNTR
#define SD_PWR_CNTR                   	GPIO_PIN_10
#define SD_PWR_CNTR_PORT              	GPIOG

// PG11		POWER_BUTTON
#define POWER_BUTTON					GPIO_PIN_11
#define POWER_BUTTON_PORT            	GPIOG

// PG12		KEYPAD_Y2
#define KEYPAD_Y2_LL                   	LL_GPIO_PIN_12
#define KEYPAD_Y2_PORT              	GPIOG

// PG13		KEYPAD_Y1
#define KEYPAD_Y1_LL                   	LL_GPIO_PIN_13
#define KEYPAD_Y1_PORT              	GPIOG

// PG14		KEYPAD_Y3
#define KEYPAD_Y3_LL                   	LL_GPIO_PIN_14
#define KEYPAD_Y3_PORT              	GPIOG

// PG15		FMC_SDNCAS
//
//

// ----------------------------------------------------
// ----------------------------------------------------
// PortH

// PH1			BAND3
#define BAND3_PIN						GPIO_PIN_1
#define BAND3_PORT						GPIOH

// PH2			FMC_SDCKE0
//
//

// PH3			FMC_SDNE0
//
//

// PH4			LO_SCL_I2C2
//
//

// PH5			LO_SDA_I2C2
//
//

// PH6			TOUCH_INT
#define TS_INT_PIN                 		GPIO_PIN_6
#define TS_INT_GPIO_PORT            	GPIOH

// PH7			DSI_RESET
#define LCD_RESET_PIN             		GPIO_PIN_7
#define LCD_RESET_PULL              	GPIO_NOPULL
#define LCD_RESET_GPIO_PORT         	GPIOH

// PH8			FMC_D16
//
//

// PH9			FMC_D17
//
//

// PH10			FMC_D18
//
//

// PH11			FMC_D19
//
//

// PH12			FMC_D20
//
//

// PH13			FMC_D21
//
//

// PH14			FMC_D22
//
//

// PH15			FMC_D23
//
//

// ----------------------------------------------------
// ----------------------------------------------------
// PortI

// PI0			FMC_D24
//
//

// PI1			FMC_D25
//
//

// PI2			FMC_D26
//
//

// PI3			FMC_D27
//
//

// PI4			FMC_NBL2
//
//

// PI5			FMC_NBL3
//
//

// PI6			FMC_D28
//
//

// PI7			FMC_D29
//
//

// PI8			KEYPAD_X6
#define KEYPAD_X6_LL                   	LL_GPIO_PIN_8
#define KEYPAD_X6_PORT					GPIOI

// PI9			FMC_D30
//
//

// PI10			FMC_D31
//
//

// PI11			KEYPAD_Y4
#define KEYPAD_Y4_LL                   	LL_GPIO_PIN_11
#define KEYPAD_Y4_PORT					GPIOI

// PI15			BAND2
#define BAND2_PIN						GPIO_PIN_15
#define BAND2_PORT						GPIOI

// EXTI Line usage:
//
// EXTI_LINE2 - paddle
// EXTI_LINE3 - paddle
//
// EXTI_LINE_6 - touch
//
// EXTI_LINE_8 - SD Card detect


#endif

#endif
