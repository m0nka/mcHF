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

//#define KEYPAD_ALLOW_DEBUG

#ifdef CONTEXT_KEYPAD

#define scan_x1() 	{	KEYPAD_X1_PORT->BSRR = KEYPAD_X1_LL << 16;\
						KEYPAD_X2_PORT->BSRR = KEYPAD_X2_LL;\
						KEYPAD_X3_PORT->BSRR = KEYPAD_X3_LL;\
						KEYPAD_X4_PORT->BSRR = KEYPAD_X4_LL;\
						KEYPAD_X5_PORT->BSRR = KEYPAD_X5_LL;\
						KEYPAD_X6_PORT->BSRR = KEYPAD_X6_LL;\
					}

#define scan_x2() 	{	KEYPAD_X1_PORT->BSRR = KEYPAD_X1_LL;\
						KEYPAD_X2_PORT->BSRR = KEYPAD_X2_LL << 16;\
						KEYPAD_X3_PORT->BSRR = KEYPAD_X3_LL;\
						KEYPAD_X4_PORT->BSRR = KEYPAD_X4_LL;\
						KEYPAD_X5_PORT->BSRR = KEYPAD_X5_LL;\
						KEYPAD_X6_PORT->BSRR = KEYPAD_X6_LL;\
					}

#define scan_x3() 	{	KEYPAD_X1_PORT->BSRR = KEYPAD_X1_LL;\
						KEYPAD_X2_PORT->BSRR = KEYPAD_X2_LL;\
						KEYPAD_X3_PORT->BSRR = KEYPAD_X3_LL << 16;\
						KEYPAD_X4_PORT->BSRR = KEYPAD_X4_LL;\
						KEYPAD_X5_PORT->BSRR = KEYPAD_X5_LL;\
						KEYPAD_X6_PORT->BSRR = KEYPAD_X6_LL;\
					}

#define scan_x4() 	{	KEYPAD_X1_PORT->BSRR = KEYPAD_X1_LL;\
						KEYPAD_X2_PORT->BSRR = KEYPAD_X2_LL;\
						KEYPAD_X3_PORT->BSRR = KEYPAD_X3_LL;\
						KEYPAD_X4_PORT->BSRR = KEYPAD_X4_LL << 16;\
						KEYPAD_X5_PORT->BSRR = KEYPAD_X5_LL;\
						KEYPAD_X6_PORT->BSRR = KEYPAD_X6_LL;\
					}

#define scan_x5() 	{	KEYPAD_X1_PORT->BSRR = KEYPAD_X1_LL;\
						KEYPAD_X2_PORT->BSRR = KEYPAD_X2_LL;\
						KEYPAD_X3_PORT->BSRR = KEYPAD_X3_LL;\
						KEYPAD_X4_PORT->BSRR = KEYPAD_X4_LL;\
						KEYPAD_X5_PORT->BSRR = KEYPAD_X5_LL << 16;\
						KEYPAD_X6_PORT->BSRR = KEYPAD_X6_LL;\
					}

#define scan_x6() 	{	KEYPAD_X1_PORT->BSRR = KEYPAD_X1_LL;\
						KEYPAD_X2_PORT->BSRR = KEYPAD_X2_LL;\
						KEYPAD_X3_PORT->BSRR = KEYPAD_X3_LL;\
						KEYPAD_X4_PORT->BSRR = KEYPAD_X4_LL;\
						KEYPAD_X5_PORT->BSRR = KEYPAD_X5_LL;\
						KEYPAD_X6_PORT->BSRR = KEYPAD_X6_LL << 16;\
					}

#define scan_off() 	{	KEYPAD_X1_PORT->BSRR = KEYPAD_X1_LL;\
						KEYPAD_X2_PORT->BSRR = KEYPAD_X2_LL;\
						KEYPAD_X3_PORT->BSRR = KEYPAD_X3_LL;\
						KEYPAD_X4_PORT->BSRR = KEYPAD_X4_LL;\
						KEYPAD_X5_PORT->BSRR = KEYPAD_X5_LL;\
						KEYPAD_X6_PORT->BSRR = KEYPAD_X6_LL;\
					}

#define scan_on() 	{	KEYPAD_X1_PORT->BSRR = KEYPAD_X1_LL << 16;\
						KEYPAD_X2_PORT->BSRR = KEYPAD_X2_LL << 16;\
						KEYPAD_X3_PORT->BSRR = KEYPAD_X3_LL << 16;\
						KEYPAD_X4_PORT->BSRR = KEYPAD_X4_LL << 16;\
						KEYPAD_X5_PORT->BSRR = KEYPAD_X5_LL << 16;\
						KEYPAD_X6_PORT->BSRR = KEYPAD_X6_LL << 16;\
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
