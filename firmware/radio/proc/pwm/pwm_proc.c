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
#include "mchf_pro_board.h"

#ifdef CONTEXT_PWM

#include "pwm_proc.h"

//*----------------------------------------------------------------------------
//* Function Name       : pwm_proc_hw_init
//* Object              :
//* Notes    			:
//* Notes   			:
//* Notes    			:
//* Context    			: CONTEXT_RESET
//*----------------------------------------------------------------------------
void pwm_proc_hw_init(void)
{
	//printf("pwm_proc_hw_init\r\n");

	// ...

	//printf("pwm_proc_hw_init ok\r\n");
}

//*----------------------------------------------------------------------------
//* Function Name       : pwm_proc_worker
//* Object              :
//* Notes    			:
//* Notes   			:
//* Notes    			:
//* Context    			: CONTEXT_PWM
//*----------------------------------------------------------------------------
static void pwm_proc_worker(void)
{

}

//*----------------------------------------------------------------------------
//* Function Name       : pwm_proc_task
//* Object              :
//* Notes    			:
//* Notes   			:
//* Notes    			:
//* Context    			: CONTEXT_PWM
//*----------------------------------------------------------------------------
void pwm_proc_task(void const * argument)
{
	ulong 	ulNotificationValue = 0, ulNotif;

	vTaskDelay(PWM_PROC_START_DELAY);
	printf("pwm proc start\r\n");

pwm_proc_loop:

	ulNotif = xTaskNotifyWait(0x00, ULONG_MAX, &ulNotificationValue, BAND_PROC_SLEEP_TIME);
	if((ulNotif)&&(ulNotificationValue))
	{
		pwm_proc_worker();
	}

	goto pwm_proc_loop;
}

#endif
