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
#ifndef __KEYPAD_PROC_H
#define __KEYPAD_PROC_H

#define KEYPAD_ALLOW_DEBUG

#ifdef CONTEXT_KEYPAD

#define scan_y1() 	{	KEYPAD_Y1_PORT->BSRR = KEYPAD_Y1 << 16;\
						KEYPAD_Y2_PORT->BSRR = KEYPAD_Y2;\
						KEYPAD_Y3_PORT->BSRR = KEYPAD_Y3;\
						KEYPAD_Y4_PORT->BSRR = KEYPAD_Y4;\
					}

#define scan_y2() 	{	KEYPAD_Y1_PORT->BSRR = KEYPAD_Y1;\
						KEYPAD_Y2_PORT->BSRR = KEYPAD_Y2 << 16;\
						KEYPAD_Y3_PORT->BSRR = KEYPAD_Y3;\
						KEYPAD_Y4_PORT->BSRR = KEYPAD_Y4;\
					}

#define scan_y3() 	{	KEYPAD_Y1_PORT->BSRR = KEYPAD_Y1;\
						KEYPAD_Y2_PORT->BSRR = KEYPAD_Y2;\
						KEYPAD_Y3_PORT->BSRR = KEYPAD_Y3 << 16;\
						KEYPAD_Y4_PORT->BSRR = KEYPAD_Y4;\
					}

#define scan_y4() 	{	KEYPAD_Y1_PORT->BSRR = KEYPAD_Y1;\
						KEYPAD_Y2_PORT->BSRR = KEYPAD_Y2;\
						KEYPAD_Y3_PORT->BSRR = KEYPAD_Y3;\
						KEYPAD_Y4_PORT->BSRR = KEYPAD_Y4 << 16;\
					}

#define scan_off() 	{	KEYPAD_Y1_PORT->BSRR = KEYPAD_Y1;\
						KEYPAD_Y2_PORT->BSRR = KEYPAD_Y2;\
						KEYPAD_Y3_PORT->BSRR = KEYPAD_Y3;\
						KEYPAD_Y4_PORT->BSRR = KEYPAD_Y4;\
					}

/*
#define KEYLED_XLAT_PIN		GPIO_PIN_11
#define KEYLED_XLAT_PORT	GPIOI
#define KEYLED_BLANK_PIN	GPIO_PIN_8
#define KEYLED_BLANK_PORT	GPIOI

#define KEYLED_SCK_PIN		GPIO_PIN_5
#define KEYLED_SCK_PORT		GPIOA
#define KEYLED_MOSI_PIN		GPIO_PIN_5
#define KEYLED_MOSI_PORT	GPIOB

#define KEY_LED_SSB			0
#define KEY_LED_ONE			1
#define KEY_LED_TWO			2
#define KEY_LED_THREE		3
#define KEY_LED_M			4
#define KEY_LED_DSP			5
#define KEY_LED_CW			6
#define KEY_LED_FOUR		7
#define KEY_LED_FIVE		8
#define KEY_LED_SIX			9
#define KEY_LED_S			10
#define KEY_LED_STEP		11
#define KEY_LED_AM			12
#define KEY_LED_SEVEN		13
#define KEY_LED_EIGHT		14
#define KEY_LED_NINE		15
#define KEY_LED_ENTER		16
#define KEY_LED_FILTER		17
#define KEY_LED_FIX			18
#define KEY_LED_DOT			19
#define KEY_LED_ZERO		20
#define KEY_LED_C			21
#define KEY_LED_SPLIT		22

#define KEY_LED_OFF_LIGHT	0
#define KEY_LED_LOW_LIGHT	64
#define KEY_LED_MID_LIGHT	96
#define KEY_LED_HIGH_LIGHT	128
*/

__attribute__((__common__)) struct KEYPAD_STATE {

	// Keypad
	ulong	tap_cnt;
	uchar	tap_id;

	// LEDs
	//uchar 	btn_id;
	//ushort 	pwmbuffer[2*24*1];
	//uchar 	start_counter;

} KEYPAD_STATE;

void keypad_proc_init(void);
void keypad_proc_task(void const * argument);

#endif
#endif
