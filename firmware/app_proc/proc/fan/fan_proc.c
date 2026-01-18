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
#include "mchf_pro_board.h"
#include "main.h"

#ifdef CONTEXT_FAN

#include "fan_proc.h"

// Public radio state
extern struct	TRANSCEIVER_STATE_UI	tsu;

//*----------------------------------------------------------------------------
//* Function Name       : fan_proc_hw_init
//* Object              :
//* Notes    			:
//* Notes   			:
//* Notes    			:
//* Context    			: CONTEXT_RESET
//*----------------------------------------------------------------------------
void fan_proc_hw_init(void)
{
	LL_GPIO_InitTypeDef  GPIO_InitStruct;

	// Fan On
	LL_GPIO_SetOutputPin(FAN_CNTR_PORT, FAN_CNTR);

	GPIO_InitStruct.Pull      = LL_GPIO_PULL_DOWN;
	GPIO_InitStruct.Speed     = LL_GPIO_SPEED_LOW;
	GPIO_InitStruct.Mode      = LL_GPIO_MODE_OUTPUT;

	GPIO_InitStruct.Pin       = FAN_CNTR;
	LL_GPIO_Init(FAN_CNTR_PORT, &GPIO_InitStruct);

	//--printf("fan_proc_hw_init ok\r\n");
}

//*----------------------------------------------------------------------------
//* Function Name       : fan_proc_power_clean_up
//* Object              :
//* Notes    			:
//* Notes   			:
//* Notes    			:
//* Context    			: CONTEXT_RESET
//*----------------------------------------------------------------------------
void fan_proc_power_clean_up(void)
{
	// Fan Off
	HAL_GPIO_WritePin(FAN_CNTR_PORT, FAN_CNTR, GPIO_PIN_RESET);
}

//*----------------------------------------------------------------------------
//* Function Name       : fan_proc_task
//* Object              :
//* Notes    			:
//* Notes   			:
//* Notes    			:
//* Context    			: CONTEXT_FAN
//*----------------------------------------------------------------------------
void fan_proc_task(void const *arg)
{
	ulong 	ulNotificationValue = 0, ulNotif;

	vTaskDelay(FAN_PROC_START_DELAY);
	printf("start\r\n");

	// Fan Off
	LL_GPIO_ResetOutputPin(FAN_CNTR_PORT, FAN_CNTR);

fan_proc_loop:

	ulNotif = xTaskNotifyWait(0x00, ULONG_MAX, &ulNotificationValue, TRX_PROC_SLEEP_TIME);
	if((ulNotif)&&(ulNotificationValue))
	{
		if(ulNotificationValue)
			LL_GPIO_SetOutputPin(FAN_CNTR_PORT, FAN_CNTR);
		else
			LL_GPIO_ResetOutputPin(FAN_CNTR_PORT, FAN_CNTR);
	}

	goto fan_proc_loop;
}

#endif
