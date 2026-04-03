/************************************************************************************
**                                                                                 **
**                                 mcHF QRP Transceiver                            **
**                         Krassi Atanassov - M0NKA, 2013-2026                     **
**                                                                                 **
**---------------------------------------------------------------------------------**
**                                                                                 **
**  File name:                                                                     **
**  Description:                                                                   **
**  Last Modified:                                                                 **
**  Licence:			https://github.com/m0nka/mcHF/blob/main/LICENSE            **
************************************************************************************/
#ifndef __KEYPAD_PROC_H
#define __KEYPAD_PROC_H

#ifdef CONTEXT_KEYPAD

// ...
#define KEYPAD_ALLOW_DEBUG

//
#define SCAN_MAX			3
//
#define scan_x1() 	{	KEYPAD_X1_PORT->BSRR = KEYPAD_X1_LL << 16;\
						KEYPAD_X2_PORT->BSRR = KEYPAD_X2_LL;\
						KEYPAD_X3_PORT->BSRR = KEYPAD_X3_LL;\
					}

#define scan_x2() 	{	KEYPAD_X1_PORT->BSRR = KEYPAD_X1_LL;\
						KEYPAD_X2_PORT->BSRR = KEYPAD_X2_LL << 16;\
						KEYPAD_X3_PORT->BSRR = KEYPAD_X3_LL;\
					}

#define scan_x3() 	{	KEYPAD_X1_PORT->BSRR = KEYPAD_X1_LL;\
						KEYPAD_X2_PORT->BSRR = KEYPAD_X2_LL;\
						KEYPAD_X3_PORT->BSRR = KEYPAD_X3_LL << 16;\
					}

#define scan_off() 	{	KEYPAD_X1_PORT->BSRR = KEYPAD_X1_LL;\
						KEYPAD_X2_PORT->BSRR = KEYPAD_X2_LL;\
						KEYPAD_X3_PORT->BSRR = KEYPAD_X3_LL;\
					}

#define scan_on() 	{	KEYPAD_X1_PORT->BSRR = KEYPAD_X1_LL << 16;\
						KEYPAD_X2_PORT->BSRR = KEYPAD_X2_LL << 16;\
						KEYPAD_X3_PORT->BSRR = KEYPAD_X3_LL << 16;\
					}

__attribute__((__common__)) struct KEYPAD_STATE {

	// Keypad
	ulong	tap_cnt;
	uchar	tap_id;
	uchar	irq_id;

} KEYPAD_STATE;

void keypad_proc_irq(uchar id);
void keypad_proc_init(void);
void keypad_proc_task(void const * argument);

#endif
#endif
