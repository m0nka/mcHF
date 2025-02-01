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

#if 0
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
#endif

#define scan_x1() 	{	KEYPAD_X1_PORT->BSRR = KEYPAD_X1 << 16;\
						KEYPAD_X2_PORT->BSRR = KEYPAD_X2;\
						KEYPAD_X3_PORT->BSRR = KEYPAD_X3;\
						KEYPAD_X4_PORT->BSRR = KEYPAD_X4;\
						KEYPAD_X5_PORT->BSRR = KEYPAD_X5;\
						KEYPAD_X6_PORT->BSRR = KEYPAD_X6;\
					}

#define scan_x2() 	{	KEYPAD_X1_PORT->BSRR = KEYPAD_X1;\
						KEYPAD_X2_PORT->BSRR = KEYPAD_X2 << 16;\
						KEYPAD_X3_PORT->BSRR = KEYPAD_X3;\
						KEYPAD_X4_PORT->BSRR = KEYPAD_X4;\
						KEYPAD_X5_PORT->BSRR = KEYPAD_X5;\
						KEYPAD_X6_PORT->BSRR = KEYPAD_X6;\
					}

#define scan_x3() 	{	KEYPAD_X1_PORT->BSRR = KEYPAD_X1;\
						KEYPAD_X2_PORT->BSRR = KEYPAD_X2;\
						KEYPAD_X3_PORT->BSRR = KEYPAD_X3 << 16;\
						KEYPAD_X4_PORT->BSRR = KEYPAD_X4;\
						KEYPAD_X5_PORT->BSRR = KEYPAD_X5;\
						KEYPAD_X6_PORT->BSRR = KEYPAD_X6;\
					}

#define scan_x4() 	{	KEYPAD_X1_PORT->BSRR = KEYPAD_X1;\
						KEYPAD_X2_PORT->BSRR = KEYPAD_X2;\
						KEYPAD_X3_PORT->BSRR = KEYPAD_X3;\
						KEYPAD_X4_PORT->BSRR = KEYPAD_X4 << 16;\
						KEYPAD_X5_PORT->BSRR = KEYPAD_X5;\
						KEYPAD_X6_PORT->BSRR = KEYPAD_X6;\
					}

#define scan_x5() 	{	KEYPAD_X1_PORT->BSRR = KEYPAD_X1;\
						KEYPAD_X2_PORT->BSRR = KEYPAD_X2;\
						KEYPAD_X3_PORT->BSRR = KEYPAD_X3;\
						KEYPAD_X4_PORT->BSRR = KEYPAD_X4;\
						KEYPAD_X5_PORT->BSRR = KEYPAD_X5 << 16;\
						KEYPAD_X6_PORT->BSRR = KEYPAD_X6;\
					}

#define scan_x6() 	{	KEYPAD_X1_PORT->BSRR = KEYPAD_X1;\
						KEYPAD_X2_PORT->BSRR = KEYPAD_X2;\
						KEYPAD_X3_PORT->BSRR = KEYPAD_X3;\
						KEYPAD_X4_PORT->BSRR = KEYPAD_X4;\
						KEYPAD_X5_PORT->BSRR = KEYPAD_X5;\
						KEYPAD_X6_PORT->BSRR = KEYPAD_X6 << 16;\
					}

#define scan_off() 	{	KEYPAD_X1_PORT->BSRR = KEYPAD_X1;\
						KEYPAD_X2_PORT->BSRR = KEYPAD_X2;\
						KEYPAD_X3_PORT->BSRR = KEYPAD_X3;\
						KEYPAD_X4_PORT->BSRR = KEYPAD_X4;\
						KEYPAD_X5_PORT->BSRR = KEYPAD_X5;\
						KEYPAD_X6_PORT->BSRR = KEYPAD_X6;\
					}

#define scan_on() 	{	KEYPAD_X1_PORT->BSRR = KEYPAD_X1 << 16;\
						KEYPAD_X2_PORT->BSRR = KEYPAD_X2 << 16;\
						KEYPAD_X3_PORT->BSRR = KEYPAD_X3 << 16;\
						KEYPAD_X4_PORT->BSRR = KEYPAD_X4 << 16;\
						KEYPAD_X5_PORT->BSRR = KEYPAD_X5 << 16;\
						KEYPAD_X6_PORT->BSRR = KEYPAD_X6 << 16;\
					}

__attribute__((__common__)) struct KEYPAD_STATE {

	// Keypad
	ulong	tap_cnt;
	uchar	tap_id;
	uchar	irq_id;

	// LEDs
	//uchar 	btn_id;
	//ushort 	pwmbuffer[2*24*1];
	//uchar 	start_counter;

} KEYPAD_STATE;

void keypad_proc_exti(uchar id);
void keypad_proc_init(void);
void keypad_proc_task(void const * argument);

#endif
#endif
