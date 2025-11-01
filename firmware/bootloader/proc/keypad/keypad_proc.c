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
**  Licence:               GNU GPLv3                                               **
************************************************************************************/
#include "main.h"
#include "mchf_pro_board.h"

#include "keypad_proc.h"

// Local keypad state
struct 			KEYPAD_STATE			ks;

extern uchar stay_in_boot;

void keypad_proc_irq(uchar id)
{
#if 0
	BaseType_t xHigherPriorityTaskWoken;

	xHigherPriorityTaskWoken = pdFALSE;
	xTaskNotifyFromISR(hKbdTask, id, eSetBits, &xHigherPriorityTaskWoken);
	portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
#endif

	ks.irq_id = id;
}

void EXTI15_10_IRQHandler(void)
{
	if(LL_EXTI_IsActiveFlag_0_31(LL_EXTI_LINE_11) != RESET)
	{
		LL_EXTI_ClearFlag_0_31(LL_EXTI_LINE_11);
		keypad_proc_irq(4);
	}
	else if(LL_EXTI_IsActiveFlag_0_31(LL_EXTI_LINE_12) != RESET)
	{
		LL_EXTI_ClearFlag_0_31(LL_EXTI_LINE_12);
		keypad_proc_irq(2);
	}
	else if(LL_EXTI_IsActiveFlag_0_31(LL_EXTI_LINE_13) != RESET)
	{
		LL_EXTI_ClearFlag_0_31(LL_EXTI_LINE_13);
		keypad_proc_irq(1);
	}
	else if(LL_EXTI_IsActiveFlag_0_31(LL_EXTI_LINE_14) != RESET)
	{
		LL_EXTI_ClearFlag_0_31(LL_EXTI_LINE_14);
		keypad_proc_irq(3);
	}
}

//*----------------------------------------------------------------------------
//* Function Name       : keypad_task
//* Object              :
//* Notes    			: call from main(), during init, due to a H7 bug
//* Notes   			: not from this driver!!!
//* Notes    			:
//* Context    			: CONTEXT_RESET_VECTOR
//*----------------------------------------------------------------------------
void keypad_proc_init(void)
{
	// All horizontal lines as inputs
	LL_GPIO_SetPinMode(KEYPAD_Y1_PORT, KEYPAD_Y1_LL, LL_GPIO_MODE_INPUT);
	LL_GPIO_SetPinMode(KEYPAD_Y2_PORT, KEYPAD_Y2_LL, LL_GPIO_MODE_INPUT);
	LL_GPIO_SetPinMode(KEYPAD_Y3_PORT, KEYPAD_Y3_LL, LL_GPIO_MODE_INPUT);
	#ifndef PCB_V9_REV_A
	LL_GPIO_SetPinMode(KEYPAD_Y4_PORT, KEYPAD_Y4_LL, LL_GPIO_MODE_INPUT);
	#endif
	// All with pullups
	LL_GPIO_SetPinPull(KEYPAD_Y1_PORT, KEYPAD_Y1_LL, LL_GPIO_PULL_UP);
	LL_GPIO_SetPinPull(KEYPAD_Y2_PORT, KEYPAD_Y2_LL, LL_GPIO_PULL_UP);
	LL_GPIO_SetPinPull(KEYPAD_Y3_PORT, KEYPAD_Y3_LL, LL_GPIO_PULL_UP);
	#ifndef PCB_V9_REV_A
	LL_GPIO_SetPinPull(KEYPAD_Y4_PORT, KEYPAD_Y4_LL, LL_GPIO_PULL_UP);
	#endif

	// Slow speed
	LL_GPIO_SetPinSpeed(KEYPAD_Y1_PORT, KEYPAD_Y1_LL, LL_GPIO_SPEED_FREQ_LOW);
	LL_GPIO_SetPinSpeed(KEYPAD_Y2_PORT, KEYPAD_Y2_LL, LL_GPIO_SPEED_FREQ_LOW);
	LL_GPIO_SetPinSpeed(KEYPAD_Y3_PORT, KEYPAD_Y3_LL, LL_GPIO_SPEED_FREQ_LOW);
	#ifndef PCB_V9_REV_A
	LL_GPIO_SetPinSpeed(KEYPAD_Y4_PORT, KEYPAD_Y4_LL, LL_GPIO_SPEED_FREQ_LOW);
	#endif

	// This clock already set ?
	LL_APB4_GRP1_EnableClock(LL_APB4_GRP1_PERIPH_SYSCFG);

	// Connect External Line to the GPIO
	LL_SYSCFG_SetEXTISource(LL_SYSCFG_EXTI_PORTI, LL_SYSCFG_EXTI_LINE11);
	LL_SYSCFG_SetEXTISource(LL_SYSCFG_EXTI_PORTG, LL_SYSCFG_EXTI_LINE12);
	LL_SYSCFG_SetEXTISource(LL_SYSCFG_EXTI_PORTG, LL_SYSCFG_EXTI_LINE13);
	LL_SYSCFG_SetEXTISource(LL_SYSCFG_EXTI_PORTG, LL_SYSCFG_EXTI_LINE14);

	// Enable interrupt
	LL_EXTI_EnableIT_0_31(LL_EXTI_LINE_11);
	LL_EXTI_EnableIT_0_31(LL_EXTI_LINE_12);
	LL_EXTI_EnableIT_0_31(LL_EXTI_LINE_13);
	LL_EXTI_EnableIT_0_31(LL_EXTI_LINE_14);

	// On falling edge
	LL_EXTI_EnableFallingTrig_0_31(LL_EXTI_LINE_11);
	LL_EXTI_EnableFallingTrig_0_31(LL_EXTI_LINE_12);
	LL_EXTI_EnableFallingTrig_0_31(LL_EXTI_LINE_13);
	LL_EXTI_EnableFallingTrig_0_31(LL_EXTI_LINE_14);

	// All vertical lines as outputs(low)
	LL_GPIO_SetPinMode(KEYPAD_X1_PORT, KEYPAD_X1_LL, LL_GPIO_MODE_OUTPUT);
	LL_GPIO_SetPinMode(KEYPAD_X2_PORT, KEYPAD_X2_LL, LL_GPIO_MODE_OUTPUT);
	LL_GPIO_SetPinMode(KEYPAD_X3_PORT, KEYPAD_X3_LL, LL_GPIO_MODE_OUTPUT);
	#ifndef PCB_V9_REV_A
	LL_GPIO_SetPinMode(KEYPAD_X4_PORT, KEYPAD_X4_LL, LL_GPIO_MODE_OUTPUT);
	LL_GPIO_SetPinMode(KEYPAD_X5_PORT, KEYPAD_X5_LL, LL_GPIO_MODE_OUTPUT);
	LL_GPIO_SetPinMode(KEYPAD_X6_PORT, KEYPAD_X6_LL, LL_GPIO_MODE_OUTPUT);
	#endif
	//
	LL_GPIO_SetPinSpeed(KEYPAD_X1_PORT, KEYPAD_X1_LL, LL_GPIO_SPEED_FREQ_LOW);
	LL_GPIO_SetPinSpeed(KEYPAD_X2_PORT, KEYPAD_X2_LL, LL_GPIO_SPEED_FREQ_LOW);
	LL_GPIO_SetPinSpeed(KEYPAD_X3_PORT, KEYPAD_X3_LL, LL_GPIO_SPEED_FREQ_LOW);
	#ifndef PCB_V9_REV_A
	LL_GPIO_SetPinSpeed(KEYPAD_X4_PORT, KEYPAD_X4_LL, LL_GPIO_SPEED_FREQ_LOW);
	LL_GPIO_SetPinSpeed(KEYPAD_X5_PORT, KEYPAD_X5_LL, LL_GPIO_SPEED_FREQ_LOW);
	LL_GPIO_SetPinSpeed(KEYPAD_X6_PORT, KEYPAD_X6_LL, LL_GPIO_SPEED_FREQ_LOW);
	#endif
	//
	LL_GPIO_ResetOutputPin(KEYPAD_X1_PORT, KEYPAD_X1_LL);
	LL_GPIO_ResetOutputPin(KEYPAD_X2_PORT, KEYPAD_X2_LL);
	LL_GPIO_ResetOutputPin(KEYPAD_X3_PORT, KEYPAD_X3_LL);
	#ifndef PCB_V9_REV_A
	LL_GPIO_ResetOutputPin(KEYPAD_X4_PORT, KEYPAD_X4_LL);
	LL_GPIO_ResetOutputPin(KEYPAD_X5_PORT, KEYPAD_X5_LL);
	LL_GPIO_ResetOutputPin(KEYPAD_X6_PORT, KEYPAD_X6_LL);
	#endif

	// Multitap publics
	ks.tap_cnt 	= 0;
	ks.tap_id	= 0;
	ks.irq_id	= 0;

	// Enable process wake-up
	NVIC_EnableIRQ	(EXTI15_10_IRQn);
	NVIC_SetPriority(EXTI15_10_IRQn, 15);
}

//*----------------------------------------------------------------------------
//* Function Name       : keypad_handle_multitap
//* Object              :
//* Notes    			: Create Nokia style multitap experience
//* Notes   			:
//* Notes    			:
//* Context    			: CONTEXT_DRIVER_KEYPAD
//*----------------------------------------------------------------------------
static void keypad_handle_multitap(uchar max_ids)
{
#if 0
	//printf("tap_cnt=%d\r\n",ks.tap_cnt);

	// Only if clicks are not far apart
	if(ks.tap_cnt > 10)
		return;

	// Erase previous char on screen
	GUI_StoreKeyMsg(GUI_KEY_BACKSPACE,1);
	GUI_StoreKeyMsg(GUI_KEY_BACKSPACE,0);

	// Advance tap counter
	(ks.tap_id)++;
	if(ks.tap_id > max_ids) ks.tap_id = 0;
#endif
}

//*----------------------------------------------------------------------------
//* Function Name       : keypad_cmd_processor
//* Object              :
//* Notes    			:
//* Notes   			:
//* Notes    			:
//* Context    			: CONTEXT_DRIVER_KEYPAD
//*----------------------------------------------------------------------------
static void keypad_cmd_processor(uchar x,uchar y, uchar hold, uchar release)
{
	//printf("x=%d, y=%d, hld=%d, rel=%d\r\n", x, y, hold, release);

	// Button held on start
	if((x == 1)&&(y == 4)&&(hold == 1)&&(release == 1))
	{
		//printf("stay in boot request  \r\n");
		stay_in_boot = 1;
	}

	// Button held on start
	if((x == 5)&&(y == 3)&&(hold == 1)&&(release == 1))
	{
		//printf("leave boot request  \r\n");
		stay_in_boot = 0;
	}

	if((hold == 0)&&(release == 0))
	{
		ks.curr_x = x;
		ks.curr_y = y;
	}
}

//*----------------------------------------------------------------------------
//* Function Name       : keypad_check_input_lines
//* Object              :
//* Notes    			: Return ID of clicked button
//* Notes   			:
//* Notes    			:
//* Context    			: CONTEXT_DRIVER_KEYPAD
//*----------------------------------------------------------------------------
static uchar keypad_check_input_lines_a(void)
{
	if((KEYPAD_Y1_PORT->IDR & KEYPAD_Y1_LL) != KEYPAD_Y1_LL)
		return 1;
	if((KEYPAD_Y2_PORT->IDR & KEYPAD_Y2_LL) != KEYPAD_Y2_LL)
		return 2;
	if((KEYPAD_Y3_PORT->IDR & KEYPAD_Y3_LL) != KEYPAD_Y3_LL)
		return 3;
	#ifndef PCB_V9_REV_A
	if((KEYPAD_Y4_PORT->IDR & KEYPAD_Y4_LL) != KEYPAD_Y4_LL)
		return 4;
	#endif

	return 0;
}

//*----------------------------------------------------------------------------
//* Function Name       : keypad_set_out_lines
//* Object              :
//* Notes    			: Set scanning GPIO lines state for scan
//* Notes   			:
//* Notes    			:
//* Context    			: CONTEXT_DRIVER_KEYPAD
//*----------------------------------------------------------------------------
static void keypad_set_out_lines_a(uchar y)
{
	// Rotate output state
	switch(y)
	{
		case 0:
			scan_x1();
			break;
		case 1:
			scan_x2();
			break;
		case 2:
			scan_x3();
			break;
		case 3:
			scan_x4();
			break;
		case 4:
			scan_x5();
			break;
		case 5:
			scan_x6();
			break;
		default:
			scan_off();
			return;
	}
}

//*----------------------------------------------------------------------------
//* Function Name       : keypad_scan
//* Object              :
//* Notes    			: support for press and hold
//* Notes   			: - non blocking implementation
//* Notes    			: - support for multitap
//* Notes    			:
//* Notes    			: Most multitap scanning keyboard implementations will
//* Notes    			: do very fast scan and store state in publics. But
//* Notes    			: our main directive is as low as possible RF noise,
//* Notes    			: so that is why the slow scanning
//* Notes    			:
//* Context    			: CONTEXT_DRIVER_KEYPAD
//*----------------------------------------------------------------------------
static void keypad_scan_a(void)
{
	uchar i, j, id;

	//--printf("proc %d \r\n", ks.irq_id);

	(ks.tap_cnt)++;											// Increase multi-tap counter
	if(ks.tap_cnt > 20) ks.tap_id = 0;						// Reset multi-tap char id

	// Line scan
	for(i = 0; i < SCAN_MAX; i++)
	{
		keypad_set_out_lines_a(i);							// Rotate output state

		id = keypad_check_input_lines_a();					// Check lines, and get an id
		if(!id) continue;									// Next

		//--printf("id %d \r\n", id);
		for(j = 0; j < 40; j++)								// More than 400mS is press and hold
		{
			HAL_Delay(10);									// High resolution de-bounce
			if(keypad_check_input_lines_a() != id)
			{
				keypad_set_out_lines_a(8);					// Scan off
				keypad_cmd_processor((i + 1), id ,0, 0);	// Process 'click', button down

				if(keypad_check_input_lines_a() != id)		// Finally is key released ?
				{
					ks.tap_cnt = 0;							// Reset multi-tap counter
					keypad_cmd_processor((i + 1), id, 0, 1);// Process 'click', button up
				}
				else
					HAL_Delay(100);							// Need this ?

				return;
			}
		}
		keypad_set_out_lines_a(8);							// Scan off
		keypad_cmd_processor((i + 1), id, 1, 0);			// Process 'hold', button down

		if(keypad_check_input_lines_a() != id)				// Finally is key released ?
			keypad_cmd_processor((i + 1), id, 1, 1);		// Process 'hold', button up

		HAL_Delay(500);										// Static de-bounce, maybe there is a better way ?
		return;
	}
}

uchar keypad_proc_is_held_on_start(void)
{
	uchar res = 0;

	#ifndef REV_0_8_4_PATCH
	keypad_set_out_lines_a(0);

	// F4, held on start
	if(keypad_check_input_lines_a() == 3)
	{
		HAL_Delay(50);
		if(keypad_check_input_lines_a() == 3)
		{
			res = 1;
			stay_in_boot = 1;
		}
	}

	keypad_set_out_lines_a(8);
	#else
	res = 1;
	stay_in_boot = 1;
	#endif

	return res;
}

//*----------------------------------------------------------------------------
//* Function Name       : keypad_task
//* Object              :
//* Notes    			:
//* Notes   			:
//* Notes    			:
//* Context    			:
//*----------------------------------------------------------------------------
void keypad_proc(void)
{
	if(ks.irq_id)
	{
		// Disable wait
		NVIC_DisableIRQ	(EXTI15_10_IRQn);
		scan_off();

		// Quick scan on a single horizontal line
		keypad_scan_a();

		// Back to wait
		scan_on();
		NVIC_EnableIRQ	(EXTI15_10_IRQn);
		ks.irq_id = 0;
	}
}

uchar keypad_proc_get(uchar clear)
{
	if(clear)
	{
		ks.curr_x = 0;
		ks.curr_y = 0;
		return 0;
	}

	//printf("x=%d, y=%d\r\n", ks.curr_x, ks.curr_y);
	return ((ks.curr_x << 4)|ks.curr_y);
}





