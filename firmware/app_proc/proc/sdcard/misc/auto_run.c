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
#include "main.h"

#ifdef CONTEXT_SD

#include "storage_api.h"
#include "auto_run.h"

extern APPLOADER_QUEUE_PARAMETERS pxAppLoaderParameters;

//*----------------------------------------------------------------------------
//* Function Name       : file_b_send_msg
//* Object              : Send message to queue
//* Input Parameters    : none
//* Output Parameters   : none
//* Functions called    : none
//*----------------------------------------------------------------------------
static uchar auto_run_send_msg(xQueueHandle pvQueueHandle, ulong *ulMessageBuffer, uchar ucNumberOfItems)
{
	ulong ulDummy;
	uchar ucCount;

	/* Clear Rx Queue before posting */
	while( uxQueueMessagesWaiting(pvQueueHandle))
	{
		xQueueReceive(pvQueueHandle, (void *)&ulDummy, (portTickType)0);
	}

	/* Send all items */
	for(ucCount = 0;ucCount < ucNumberOfItems;ucCount++)
	{
    	ulDummy = *ulMessageBuffer++;

	    /* Insert the item */
		if(xQueueSend(pvQueueHandle, (void *)&ulDummy, (portTickType)0) != pdPASS )
			return 1;
	}

	return 0;
}

//*--------------------------------------------------------------------------------------
//* Function Name       : auto_run_wait_msg
//* Object              : Read pending messages
//* Input Parameters    : Rx Queue ptr and items buffer
//* Output Parameters   : none.
//*--------------------------------------------------------------------------------------
static uchar auto_run_wait_msg(xQueueHandle pRxQueue,ulong *ulQueueBuffer)
{
	uchar ucNext = 0;

	*ulQueueBuffer = 0;
	while(uxQueueMessagesWaiting(pRxQueue))
	{
		if(xQueueReceive( pRxQueue, (ulQueueBuffer + ucNext), (portTickType)0) == pdPASS)
		{
			ucNext++;
		}
	}

	return ucNext;
}

//*----------------------------------------------------------------------------
//* Function Name       : auto_run_load_app
//* Object              :
//* Notes    			:
//* Notes   			:
//* Notes    			:
//* Context    			: CONTEXT_SD
//*----------------------------------------------------------------------------
static uchar auto_run_load_app(char *chAppName, uchar ucIsScript)
{
	ulong 			ulData[10];
	ulong			ulTimeout = 600;
	char			chCertPath[32];

	if(chAppName == NULL)
		return 100;

	//printf("file: %s \r\n", chAppName);

	// Insert the requst as a message
	ulData[0] = 0xC3;
	ulData[2] = (ulong)chCertPath;

	// Insert path
	ulData[1] = (ulong)chAppName;

	// Post load request
	if(auto_run_send_msg(pxAppLoaderParameters.xAppLoaderRxQueue,ulData,3) != 0)
	{
		return 1;
	}

	// Wait to check loading result
	while((auto_run_wait_msg(pxAppLoaderParameters.xAppLoaderTxQueue,ulData) == 0) && ulTimeout)
	{
		vTaskDelay(20);
		ulTimeout--;
	}

	// Check if msg received
	if(ulTimeout == 0)
	{
		//printf("err timeout!\n\r");
		return 2;
	}

	// Check return msg
	if(ulData[0] != 0xD6)
	{
		//printf("err ret msg!\n\r");
		return 3;
	}

	// Check func result
	if((ulData[1] & 0xFF) != 0x00)
	{
		printf("err exec: %d \r\n", (int)(ulData[1] & 0xFF));
		return 4;
	}

	//printf("ok.\n\r");
	return 0;
}

static uchar auto_run_check(char *auto_path, ulong size)
{
	FIL   		file;
	FILINFO 	fno;
	ulong 		read;
	int			res;
	char		a_f[] = "0://system/autorun.txt";

	if((auto_path == NULL)||(size == 0))
		return 100;

	memset(auto_path, 0, size);

	// Get size first
 	res = f_stat(a_f, &fno);
 	if(res != FR_OK)
 	{
 		return 1;
 	}

 	// Check size
 	if(fno.fsize > size)
 		return 2;

 	// Open for read
	res = f_open(&file, a_f, FA_READ);
	if(res != FR_OK)
		return 3;

	// Read file
	res = f_read(&file, auto_path, fno.fsize, (void *)&read);
	if(res != FR_OK)
	{
		f_close(&file);
		return 4;
	}

	f_close(&file);

	// Commented out ?
	if((*(auto_path + 0) != '0')||(*(auto_path + 1) != ':'))
		return 5;

 	return 0;
}

//*----------------------------------------------------------------------------
//* Function Name       : auto_run_handle_all
//* Object              :
//* Notes    			:
//* Notes   			:
//* Notes    			:
//* Context    			: CONTEXT_SD
//*----------------------------------------------------------------------------
void auto_run_handle_all(void)
{
	char file_path[64];

	vTaskDelay(3000);

	if(!Storage_GetStatus(MSD_DISK_UNIT))
	{
		//printf("auto run abort, no disk \r\n");
		return;
	}

	// Get auto run file
	if(auto_run_check(file_path, sizeof(file_path)) != 0)
		return;

	printf("autorun: %s \r\n", file_path);

	// Load
	//
	// "0://apps/meshcore/firmware.bin"
	//
	auto_run_load_app(file_path, 0);
}

#endif
