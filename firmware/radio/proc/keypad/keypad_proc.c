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

#include "radio_init.h"

#include "keypad_proc.h"

//#include "gui.h"
//#include "dialog.h"
//#include "ST_GUI_Addons.h"

#ifdef CONTEXT_KEYPAD

// Local keypad state
struct 			KEYPAD_STATE			ks;

// Public UI driver state
extern struct	UI_DRIVER_STATE			ui_s;

// Public radio state
extern struct	TRANSCEIVER_STATE_UI	tsu;

// Process handle
extern 			TaskHandle_t 			hKbdTask;
extern 			TaskHandle_t 			hUiTask;

// API Driver messaging
//extern osMessageQId 					hApiMessage;
//struct APIMessage						api_keypad;

void keypad_proc_irq(uchar id)
{
	BaseType_t xHigherPriorityTaskWoken;

	xHigherPriorityTaskWoken = pdFALSE;
	xTaskNotifyFromISR(hKbdTask, id, eSetBits, &xHigherPriorityTaskWoken);
	portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
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
#if 1
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

//uchar bc_mode_toggle = 0;

//*----------------------------------------------------------------------------
//* Function Name       : keypad_cmd_processor_desktop
//* Object              :
//* Notes    			: keypad commands routed to custom Desktop controls
//* Notes   			:
//* Notes    			:
//* Context    			: CONTEXT_DRIVER_KEYPAD
//*----------------------------------------------------------------------------
#ifndef PCB_V9_REV_A
static void keypad_cmd_processor_desktop(uchar x, uchar y, uchar hold, uchar release)
{
	#ifdef KEYPAD_ALLOW_DEBUG
	printf("x=%d, y=%d, hld=%d, rel=%d\r\n", x, y, hold, release);
	#endif

	if((x == 1) && (y == 1) && (!release))
	{
		if(!hold)
		{
			printf("SSB\r\n");
			GUI_StoreKeyMsg('B', 1);
		}
		else
		{
			printf("SSB hold\r\n");
		}
		return;
	}

	if((x == 2) && (y == 1) && (!release))
	{
		if(!hold)
		{
			printf("160m\r\n");
			GUI_StoreKeyMsg('1', 1);
		}
		else
		{
			printf("160m hold\r\n");
		}
		return;
	}

	if((x == 3) && (y == 1) && (!release))
	{
		if(!hold)
		{
			printf("80m\r\n");
			GUI_StoreKeyMsg('2', 1);
		}
		else
		{
			printf("80m hold\r\n");
		}
		return;
	}

	if((x == 4) && (y == 1) && (!release))
	{
		if(!hold)
		{
			printf("60m\r\n");
			GUI_StoreKeyMsg('3', 1);
		}
		else
		{
			printf("60m hold\r\n");
		}
		return;
	}

	if((x == 5) && (y == 1) && (!release))
	{
		if(!hold)
		{
			printf("40m\r\n");
			GUI_StoreKeyMsg('M', 1);
		}
		else
		{
			printf("40m hold\r\n");
		}
		return;
	}

	if((x == 6) && (y == 1) && (!release))
	{
		if(!hold)
		{
			printf("STEP+\r\n");
			GUI_StoreKeyMsg('+', 1);
		}
		else
		{
			printf("STEP+ hold\r\n");
		}
		return;
	}

	if((x == 1) && (y == 2) && (!release))
	{
		if(!hold)
		{
			printf("CW\r\n");
			GUI_StoreKeyMsg('C', 1);
		}
		else
		{
			printf("CW hold\r\n");
		}
		return;
	}

	if((x == 2) && (y == 2) && (!release))
	{
		if(!hold)
		{
			printf("30m\r\n");
			GUI_StoreKeyMsg('4', 1);
		}
		else
		{
			printf("30m hold\r\n");
		}
		return;
	}

	if((x == 3) && (y == 2) && (!release))
	{
		if(!hold)
		{
			printf("20m\r\n");
			GUI_StoreKeyMsg('5', 1);
		}
		else
		{
			printf("20m hold\r\n");
		}
		return;
	}

	if((x == 4) && (y == 2) && (!release))
	{
		if(!hold)
		{
			printf("17m\r\n");
			GUI_StoreKeyMsg('6', 1);
		}
		else
		{
			printf("17m hold\r\n");
		}
		return;
	}

	if((x == 5) && (y == 2) && (!release))
	{
		if(!hold)
		{
			printf("15m\r\n");
			GUI_StoreKeyMsg('S', 1);
		}
		else
		{
			printf("15m hold\r\n");
		}
		return;
	}

	if((x == 6) && (y == 2) && (!release))
	{
		if(!hold)
		{
			printf("Step-\r\n");
			GUI_StoreKeyMsg('-', 1);
		}
		else
		{
			printf("Step- hold\r\n");
		}
		return;
	}

	if((x == 1) && (y == 3) && (!release))
	{
		if(!hold)
		{
			printf("AM\r\n");
			GUI_StoreKeyMsg('Q', 1);
		}
		else
		{
			printf("AM hold\r\n");
		}
		return;
	}

	if((x == 2) && (y == 3) && (!release))
	{
		if(!hold)
		{
			printf("12m\r\n");
			GUI_StoreKeyMsg('7', 1);
		}
		else
		{
			printf("12m hold\r\n");
		}
		return;
	}

	if((x == 3) && (y == 3) && (!release))
	{
		if(!hold)
		{
			printf("10m\r\n");
			GUI_StoreKeyMsg('8', 1);
		}
		else
		{
			printf("10m hold\r\n");
		}
		return;
	}

	if((x == 4) && (y == 3) && (!release))
	{
		if(!hold)
		{
			printf("VFO\r\n");
			GUI_StoreKeyMsg('V', 1);
		}
		else
		{
			printf("VFO hold\r\n");
		}
		return;
	}

	if((x == 5) && (y == 3) && (!release))
	{
		if(!hold)
		{
			printf("Menu\r\n");
			ui_s.req_state = MODE_MENU;
			xTaskNotify(hUiTask, UI_NEW_MODE_EVENT, eSetValueWithOverwrite);
		}
		else
		{
			printf("Enter hold\r\n");
		}
		return;
	}

	if((x == 6) && (y == 3) && (!release))
	{
		if(!hold)
		{
			printf("Filter\r\n");
			GUI_StoreKeyMsg('F', 1);
		}
		else
		{
			printf("Filter hold\r\n");
		}
		return;
	}

	if((x == 1) && (y == 4) && (!release))
	{
		if(!hold)
		{
			printf("Fix/Centre\r\n");
			GUI_StoreKeyMsg('I', 1);
		}
		else
		{
			printf("Fix/Centre hold\r\n");
		}
		return;
	}

	if((x == 2) && (y == 4) && (!release))
	{
		if(!hold)
		{
			printf("Audio\r\n");
			GUI_StoreKeyMsg('A', 1);
		}
		else
		{
			printf("Audio hold\r\n");
		}
		return;
	}

	if((x == 3) && (y == 4) && (!release))
	{
		if(!hold)
		{
			printf("AGC\r\n");
			GUI_StoreKeyMsg('G', 1);
		}
		else
		{
			printf("AGC hold\r\n");
		}
		return;
	}

	if((x == 4) && (y == 4) && (!release))
	{
		if(!hold)
		{
			#if 0
			printf("KEYB\r\n");
			GUI_StoreKeyMsg('K', 1);
			#else
			if(ui_s.cur_state != MODE_DESKTOP_FT8)
			{
				printf("enter FT8\r\n");
				ui_s.req_state = MODE_DESKTOP_FT8;
			}
			else
			{
				printf("exit FT8\r\n");
				ui_s.req_state = MODE_DESKTOP;
			}
			xTaskNotify(hUiTask, UI_NEW_MODE_EVENT, eSetValueWithOverwrite);
#endif
		}
		else
		{
			printf("KEYB hold\r\n");
		}
		return;
	}

	if((x == 5) && (y == 4) && (!release))
	{
		if(!hold)
		{
			printf("QuickLog\r\n");
			GUI_StoreKeyMsg('L', 1);
		}
		else
			printf("Record/Mute\r\n");

		return;
	}

	if((x == 6) && (y == 4) && (!release))
	{
		if(!hold)
		{
			printf("PWR\r\n");
			GUI_StoreKeyMsg('W', 1);
		}
		else
		{
			printf("PWR hold\r\n");
		}
	}
}
#else
static void keypad_cmd_processor_desktop(uchar x, uchar y, uchar hold, uchar release)
{
	#ifdef KEYPAD_ALLOW_DEBUG
	//--printf("x=%d, y=%d, hld=%d, rel=%d\r\n", x, y, hold, release);
	#endif

	// Band- button
	if((x == 2) && (y == 1) && (!release))
	{
		if(!hold)
		{
			printf("Band-\r\n");
			GUI_StoreKeyMsg('-', 1);
		}
		else
		{
			printf("Band- hold\r\n");
		}
		return;
	}

	// Band+ button
	if((x == 3) && (y == 2) && (!release))
	{
		if(!hold)
		{
			printf("Band+\r\n");
			GUI_StoreKeyMsg('+', 1);
		}
		else
		{
			printf("Band+ hold\r\n");
		}
		return;
	}

	// Mode button
	if((x == 1) && (y == 1) && (!release))
	{
		if(!hold)
		{
			printf("Mode\r\n");

		}
		else
		{
			printf("Mode hold\r\n");
		}
		return;
	}

	// DSP button
	if((x == 3) && (y == 1) && (!release))
	{
		if(!hold)
		{
			printf("DSP\r\n");

		}
		else
		{
			printf("DSP hold\r\n");
		}
		return;
	}

	// F1 button
	if((x == 1) && (y == 2) && (!release))
	{
		if(!hold)
		{
			printf("F1->Menu\r\n");
			ui_s.req_state = MODE_MENU;
			xTaskNotify(hUiTask, UI_NEW_MODE_EVENT, eSetValueWithOverwrite);
		}
		else
		{
			printf("F1 hold\r\n");
		}
		return;
	}

	// F2 button
	if((x == 2) && (y == 2) && (!release))
	{
		if(!hold)
		{
			printf("F2->AGC\r\n");
			GUI_StoreKeyMsg('G', 1);
		}
		else
		{
			printf("F2 hold\r\n");
		}
		return;
	}

	// F3 button
	if((x == 3) && (y == 3) && (!release))
	{
		if(!hold)
		{
			printf("F3->Audio\r\n");
			GUI_StoreKeyMsg('A', 1);
		}
		else
		{
			printf("F3 hold\r\n");
		}
		return;
	}

	// F4 button
	if((x == 1) && (y == 3) && (!release))
	{
		if(!hold)
		{
			printf("F4->Keyb\r\n");
			GUI_StoreKeyMsg('K', 1);
		}
		else
		{
			printf("F4 hold\r\n");
		}
		return;
	}

	// F5 button
	if((x == 2) && (y == 3) && (!release))
	{
		if(!hold)
		{
			printf("F5->QuickLog\r\n");
			GUI_StoreKeyMsg('L', 1);
		}
		else
			printf("F5 hold\r\n");

		return;
	}


}
#endif

//*----------------------------------------------------------------------------
//* Function Name       : keypad_cmd_processor_wm
//* Object              :
//* Notes    			: keypad router for emWin WM control
//* Notes   			:
//* Notes    			:
//* Context    			: CONTEXT_DRIVER_KEYPAD
//*----------------------------------------------------------------------------
static void keypad_cmd_processor_wm(uchar x,uchar y, uchar hold, uchar release)
{
	#ifdef KEYPAD_ALLOW_DEBUG
	printf("x=%d, y=%d, hld=%d, rel=%d\r\n", x, y, hold, release);
	#endif
#if 0
	// SSB - USB/LSB
	if((x == 1) && (y == 1))
	{
		if(!hold)
		{

		}
		else
		{
			// ..
		}

		return;
	}
	// 160m/1
	if((x == 2) && (y == 1))
	{
		if(!hold)
		{
			if(!release)
				GUI_StoreKeyMsg('1',1);
			else
				GUI_StoreKeyMsg('1',0);
		}
		else
		{
			// ..
		}

		return;
	}
	// 80m/2ABC
	if((x == 3) && (y == 1))
	{
		if(!hold)
		{
			if(!release) keypad_handle_multitap(3);

			switch(ks.tap_id)
			{
				case 0:
				{
					if(!release)
						GUI_StoreKeyMsg('2',1);
					else
						GUI_StoreKeyMsg('2',0);
					break;
				}
				case 1:
				{
					if(!release)
						GUI_StoreKeyMsg('A',1);
					else
						GUI_StoreKeyMsg('A',0);
					break;
				}
				case 2:
				{
					if(!release)
						GUI_StoreKeyMsg('B',1);
					else
						GUI_StoreKeyMsg('B',0);
					break;
				}
				case 3:
				{
					if(!release)
						GUI_StoreKeyMsg('C',1);
					else
						GUI_StoreKeyMsg('C',0);
					break;
				}
				default:
					break;
			}
		}
		else
		{
			// ..
		}

		return;
	}
	// 60m/3DEF
	if((x == 4) && (y == 1))
	{
		if(!hold)
		{
			if(!release) keypad_handle_multitap(3);

			switch(ks.tap_id)
			{
				case 0:
				{
					if(!release)
						GUI_StoreKeyMsg('3',1);
					else
						GUI_StoreKeyMsg('3',0);
					break;
				}
				case 1:
				{
					if(!release)
						GUI_StoreKeyMsg('D',1);
					else
						GUI_StoreKeyMsg('D',0);
					break;
				}
				case 2:
				{
					if(!release)
						GUI_StoreKeyMsg('E',1);
					else
						GUI_StoreKeyMsg('E',0);
					break;
				}
				case 3:
				{
					if(!release)
						GUI_StoreKeyMsg('F',1);
					else
						GUI_StoreKeyMsg('F',0);
					break;
				}
				default:
					break;
			}
		}
		else
		{
			// ..
		}

		return;
	}
	// 40m
	if((x == 5) && (y == 1))
	{
		if(!hold)
		{

		}
		else
		{
			// ..
		}

		return;
	}
	// DSP
	if((x == 6) && (y == 1))
	{
		if(!hold)
		{
			if(!release)
				GUI_StoreKeyMsg(GUI_KEY_LEFT,1);
			else
				GUI_StoreKeyMsg(GUI_KEY_LEFT,0);
		}
		else
		{
			// ..
		}

		return;
	}
	// CW
	if((x == 1) && (y == 2))
	{
		if(!hold)
		{

		}
		else
		{
			// ..
		}

		return;
	}
	// 30m/4GHI
	if((x == 2) && (y == 2))
	{
		if(!hold)
		{
			if(!release) keypad_handle_multitap(3);

			switch(ks.tap_id)
			{
				case 0:
				{
					if(!release)
						GUI_StoreKeyMsg('4',1);
					else
						GUI_StoreKeyMsg('4',0);
					break;
				}
				case 1:
				{
					if(!release)
						GUI_StoreKeyMsg('G',1);
					else
						GUI_StoreKeyMsg('G',0);
					break;
				}
				case 2:
				{
					if(!release)
						GUI_StoreKeyMsg('H',1);
					else
						GUI_StoreKeyMsg('H',0);
					break;
				}
				case 3:
				{
					if(!release)
						GUI_StoreKeyMsg('I',1);
					else
						GUI_StoreKeyMsg('I',0);
					break;
				}
				default:
					break;
			}
		}
		else
		{
			// ..
		}

		return;
	}
	// 20m/5JKL
	if((x == 3) && (y == 2))
	{

		if(!hold)
		{
			if(!release) keypad_handle_multitap(3);

			switch(ks.tap_id)
			{
				case 0:
				{
					if(!release)
						GUI_StoreKeyMsg('5',1);
					else
						GUI_StoreKeyMsg('5',0);
					break;
				}
				case 1:
				{
					if(!release)
						GUI_StoreKeyMsg('J',1);
					else
						GUI_StoreKeyMsg('J',0);
					break;
				}
				case 2:
				{
					if(!release)
						GUI_StoreKeyMsg('K',1);
					else
						GUI_StoreKeyMsg('K',0);
					break;
				}
				case 3:
				{
					if(!release)
						GUI_StoreKeyMsg('L',1);
					else
						GUI_StoreKeyMsg('L',0);
					break;
				}
				default:
					break;
			}
		}
		else
		{
			// ..
		}

		return;
	}
	// 17m/6MNO
	if((x == 4) && (y == 2))
	{
		if(!hold)
		{
			if(!release) keypad_handle_multitap(3);

			switch(ks.tap_id)
			{
				case 0:
				{
					if(!release)
						GUI_StoreKeyMsg('6',1);
					else
						GUI_StoreKeyMsg('6',0);
					break;
				}
				case 1:
				{
					if(!release)
						GUI_StoreKeyMsg('M',1);
					else
						GUI_StoreKeyMsg('M',0);
					break;
				}
				case 2:
				{
					if(!release)
						GUI_StoreKeyMsg('N',1);
					else
						GUI_StoreKeyMsg('N',0);
					break;
				}
				case 3:
				{
					if(!release)
						GUI_StoreKeyMsg('O',1);
					else
						GUI_StoreKeyMsg('O',0);
					break;
				}
				default:
					break;
			}
		}
		else
		{
			// ..
		}

		return;
	}
	// 15m
	if((x == 5) && (y == 2))
	{
		if(!hold)
		{

		}
		else
		{
			// ..
		}

		return;
	}
	// Step
	if((x == 6) && (y == 2))
	{
		if(!hold)
		{
			if(!release)
				GUI_StoreKeyMsg(GUI_KEY_RIGHT,1);
			else
				GUI_StoreKeyMsg(GUI_KEY_RIGHT,0);
		}
		else
		{

		}

		return;
	}
	// AM
	if((x == 1) && (y == 3))
	{
		if(!hold)
		{

		}
		else
		{
			if(ui_s.req_state == MODE_DESKTOP_FT8)
				ui_s.req_state = MODE_DESKTOP;
		}

		return;
	}
	// 12m/7PQRS
	if((x == 2) && (y == 3))
	{
		if(!hold)
		{
			if(!release) keypad_handle_multitap(4);

			switch(ks.tap_id)
			{
				case 0:
				{
					if(!release)
						GUI_StoreKeyMsg('7',1);
					else
						GUI_StoreKeyMsg('7',0);
					break;
				}
				case 1:
				{
					if(!release)
						GUI_StoreKeyMsg('P',1);
					else
						GUI_StoreKeyMsg('P',0);
					break;
				}
				case 2:
				{
					if(!release)
						GUI_StoreKeyMsg('Q',1);
					else
						GUI_StoreKeyMsg('Q',0);
					break;
				}
				case 3:
				{
					if(!release)
						GUI_StoreKeyMsg('R',1);
					else
						GUI_StoreKeyMsg('R',0);
					break;
				}
				case 4:
				{
					if(!release)
						GUI_StoreKeyMsg('S',1);
					else
						GUI_StoreKeyMsg('S',0);
					break;
				}
				default:
					break;
			}
		}
		else
		{
			// ..
		}

		return;
	}
	// 10m/8TUV
	if((x == 3) && (y == 3))
	{
		if(!hold)
		{
			if(!release) keypad_handle_multitap(3);

			switch(ks.tap_id)
			{
				case 0:
				{
					if(!release)
						GUI_StoreKeyMsg('8',1);
					else
						GUI_StoreKeyMsg('8',0);
					break;
				}
				case 1:
				{
					if(!release)
						GUI_StoreKeyMsg('T',1);
					else
						GUI_StoreKeyMsg('T',0);
					break;
				}
				case 2:
				{
					if(!release)
						GUI_StoreKeyMsg('U',1);
					else
						GUI_StoreKeyMsg('U',0);
					break;
				}
				case 3:
				{
					if(!release)
						GUI_StoreKeyMsg('V',1);
					else
						GUI_StoreKeyMsg('V',0);
					break;
				}
				default:
					break;
			}
		}
		else
		{
			// ..
		}

		return;
	}
	// 6m/9WXYZ
	if((x == 4) && (y == 3))
	{
		if(!hold)
		{
			if(!release) keypad_handle_multitap(4);

			switch(ks.tap_id)
			{
				case 0:
				{
					if(!release)
						GUI_StoreKeyMsg('9',1);
					else
						GUI_StoreKeyMsg('9',0);
					break;
				}
				case 1:
				{
					if(!release)
						GUI_StoreKeyMsg('W',1);
					else
						GUI_StoreKeyMsg('W',0);
					break;
				}
				case 2:
				{
					if(!release)
						GUI_StoreKeyMsg('X',1);
					else
						GUI_StoreKeyMsg('X',0);
					break;
				}
				case 3:
				{
					if(!release)
						GUI_StoreKeyMsg('Y',1);
					else
						GUI_StoreKeyMsg('Y',0);
					break;
				}
				case 4:
				{
					if(!release)
						GUI_StoreKeyMsg('Z',1);
					else
						GUI_StoreKeyMsg('Z',0);
					break;
				}
				default:
					break;
			}
		}
		else
		{
			// ..
		}

		return;
	}
	// Enter(MENU)
	if((x == 5) && (y == 3))
	{
		if(!hold)
		{
			if(!release)
				GUI_StoreKeyMsg(GUI_KEY_ENTER,1);
			else
				GUI_StoreKeyMsg(GUI_KEY_ENTER,0);
		}
		else
		{
			if(!ui_s.lock_requests)
			{
				// Lock yourself out, then only the UI driver can release the lock
				// after GUI repaint
				ui_s.lock_requests = 1;

				// Pass request to UI driver to change mode
				if(ui_s.req_state == MODE_DESKTOP)
					ui_s.req_state = MODE_MENU;
				else
				{
					if(ui_s.req_state == MODE_MENU)
						ui_s.req_state = MODE_DESKTOP;
				}

				// Large debounce
				vTaskDelay(500);
			}
			else
				printf("locked\r\n");
		}

		return;
	}
	// Filter
	if((x == 6) && (y == 3))
	{
		if(!hold)
		{
			if(!release)
				GUI_StoreKeyMsg(GUI_KEY_HOME,1);
			else
				GUI_StoreKeyMsg(GUI_KEY_HOME,0);
		}
		else
		{
			// ..
		}

		return;
	}
	// Toggle Fix/Centre VFO mode
	if((x == 1) && (y == 4))
	{
		if(!hold)
		{

		}
		else
		{
			// ..
		}

		return;
	}
	// 4m/(special chars)
	if((x == 2) && (y == 4))
	{
		if(!hold)
		{
			if(!release) keypad_handle_multitap(3);

			switch(ks.tap_id)
			{
				case 0:
				{
					if(!release)
						GUI_StoreKeyMsg('.',1);
					else
						GUI_StoreKeyMsg('.',0);
					break;
				}
				case 1:
				{
					if(!release)
						GUI_StoreKeyMsg('/',1);
					else
						GUI_StoreKeyMsg('/',0);
					break;
				}
				case 2:
				{
					if(!release)
						GUI_StoreKeyMsg('_',1);
					else
						GUI_StoreKeyMsg('_',0);
					break;
				}
				case 3:
				{
					if(!release)
						GUI_StoreKeyMsg('-',1);
					else
						GUI_StoreKeyMsg('-',0);
					break;
				}
				default:
					break;
			}
		}
		else
		{
			// ..
		}

		return;
	}
	// LF/0(Space)
	if((x == 3) && (y == 4))
	{
		if(!hold)
		{
			if(!release) keypad_handle_multitap(1);

			switch(ks.tap_id)
			{
				case 0:
				{
					if(!release)
						GUI_StoreKeyMsg('0',1);
					else
						GUI_StoreKeyMsg('0',0);
					break;
				}
				case 1:
				{
					if(!release)
						GUI_StoreKeyMsg(GUI_KEY_SPACE,1);
					else
						GUI_StoreKeyMsg(GUI_KEY_SPACE,0);
					break;
				}
				default:
					break;
			}
		}
		else
		{
			if(ui_s.req_state == MODE_QUICK_LOG)
				ui_s.req_state = MODE_DESKTOP;
		}

		return;
	}
	// MF(Backspace)
	if((x == 4) && (y == 4))
	{
		if(!hold)
		{
			if(!release)
				GUI_StoreKeyMsg(GUI_KEY_BACKSPACE,1);
			else
				GUI_StoreKeyMsg(GUI_KEY_BACKSPACE,0);
		}
		else
		{
			// ..
		}

		return;
	}
	// No button connected
	if((x == 5) && (y == 4))
	{
		// NA
	}
	// VFO A/B, SPLIT(TAB)
	if((x == 6) && (y == 4))
	{
		if(!hold)
		{
			if(!release)
				GUI_StoreKeyMsg(GUI_KEY_TAB,1);
			else
				GUI_StoreKeyMsg(GUI_KEY_TAB,0);
		}
		else
		{
			// ..
		}
	}
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
	// Manage different UI driver modes
	switch(ui_s.cur_state)
	{
		// -------------------------------------------------
		// Main radio desktop
		case MODE_DESKTOP:
		case MODE_DESKTOP_FT8:	// so can exit via button
			keypad_cmd_processor_desktop(x,y,hold,release);
			break;

		// -------------------------------------------------
		// Route Keypad input to emWin Window Manager
		case MODE_MENU:
		case MODE_QUICK_LOG:
		case MODE_SIDE_ENC_MENU:
			keypad_cmd_processor_wm(x,y,hold,release);
			break;

		// Do we need keyboard processing in other modes ?
		// If yes, declare and handle...
		default:
			break;
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

	//printf("proc %d \r\n", ks.irq_id);

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
			vTaskDelay(10);									// High resolution de-bounce
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
					vTaskDelay(100);						// Need this ?

				return;
			}
		}
		keypad_set_out_lines_a(8);							// Scan off
		keypad_cmd_processor((i + 1), id, 1, 0);			// Process 'hold', button down

		if(keypad_check_input_lines_a() != id)				// Finally is key released ?
			keypad_cmd_processor((i + 1), id, 1, 1);		// Process 'hold', button up

		vTaskDelay(500);									// Static de-bounce, maybe there is a better way ?
		return;
	}
}

//*----------------------------------------------------------------------------
//* Function Name       : keypad_task
//* Object              :
//* Notes    			:
//* Notes   			:
//* Notes    			:
//* Context    			: CONTEXT_KEYPAD
//*----------------------------------------------------------------------------
void keypad_proc_task(void const * argument)
{
	ulong 	ulNotificationValue = 0, ulNotif;

	// Delay start, so UI can paint properly
	vTaskDelay(KEYPAD_PROC_START_DELAY);

	//printf("keypad process start\r\n");

	// Enable process wake-up
	NVIC_EnableIRQ	(EXTI15_10_IRQn);
	NVIC_SetPriority(EXTI15_10_IRQn, 15);

keypad_proc_loop:

	// Wait key press
	ulNotif = xTaskNotifyWait(0x00, ULONG_MAX, &ulNotificationValue, KEYPAD_PROC_SLEEP_TIME);
	if((ulNotif) && (ulNotificationValue))
	{
		ks.irq_id = (uchar)ulNotificationValue;
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

	goto keypad_proc_loop;
}

#endif




