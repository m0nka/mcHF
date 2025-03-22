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

#ifdef CONTEXT_LORA

#include "lora_spi.h"
#include "lora_proc.h"

//*----------------------------------------------------------------------------
//* Function Name       : lora_proc_task
//* Object              :
//* Notes    			:
//* Notes   			:
//* Notes    			:
//* Context    			: CONTEXT_LORA
//*----------------------------------------------------------------------------
void lora_proc_task(void const * argument)
{
	ulong 	ulNotificationValue = 0, ulNotif;

	// Delay start, so UI can paint properly
	vTaskDelay(LORA_PROC_START_DELAY);

	printf("lora process start\r\n");

lora_proc_loop:

	// Wait key press
	ulNotif = xTaskNotifyWait(0x00, ULONG_MAX, &ulNotificationValue, LORA_PROC_SLEEP_TIME);
	if((ulNotif) && (ulNotificationValue))
	{
		// ..
	}

	goto lora_proc_loop;
}

#endif
