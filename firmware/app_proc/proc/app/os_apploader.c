/************************************************************************************
**                                                                                 **
**                          GENIE RTOS SAM7 IMPLEMENTATION                         **
**                                  B-phreaks, 2006                                **
**                                                                                 **
**---------------------------------------------------------------------------------**
**                                                                                 **
**  File name:                                                                     **
**  Description:                                                                   **
**  Last Modified:                                                                 **
**                                                                                 **
************************************************************************************/
#include "main.h"
#include "mchf_pro_board.h"

#ifdef CONTEXT_APP

#include "os_sednadecl.h"
#include "os_sedna_exports.h"
#include "os_sednaelf.h"
#include "sdram.h"

#include "os_apploader.h"

/* Keep the count of running applications
   needed for the variable sleep rate of 
   this driver */
uchar	ucRunningApplicationsCount = 0;

/* Blank declaration of the application task we plan to run */
static void (* pvAppLoaderDinamiclyLoadedFunc) ( void *pvParameters );

APPLOADER_APP_PARAMETERS  pxAppLoaderAppParametersArr[SEDNA_MAXIMUM_APPLICATIONS];
APPLOADER_APP_PARAMETERS  *pxAppLoaderAppParameters = pxAppLoaderAppParametersArr;

APP_PARAMETERS *pxAppParameters;

const unsigned portCHAR 	ucQueueSize = ( unsigned portCHAR ) 10;

APPLOADER_APP_PARAMETERS 	*pxAppLdrParametersPub = NULL;


static void os_apploader_get_ext(char * pFile, char * pExt)
{
  int Len;
  int i;
  int j;

  /* Search beginning of extension */
  Len = strlen(pFile);
  for (i = Len; i > 0; i--) {
    if (*(pFile + i) == '.') {
      break;
    }
  }

  /* Copy extension */
  j = 0;
  while (*(pFile + ++i) != '\0') {
    *(pExt + j) = *(pFile + i);
    j++;
  }
  *(pExt + j) = '\0';          /* Set end of string */
}

static portSHORT os_apploader_create_proc(NewTaskData *nt)
{
	// ToDo: check args

	return xTaskCreate(	(TaskFunction_t)nt->pvTaskCode,\
						nt->pcName,\
						nt->usStackDepth,\
						nt->pvParameters,\
						nt->ucPriority,\
						nt->pxCreatedTask);
}

//*----------------------------------------------------------------------------
//* Function Name       : os_apploader_memcpy
//* Object              : Copy buffers
//* Input Parameters    : 
//* Output Parameters   : none
//* Functions called    : none
//*----------------------------------------------------------------------------
static void os_apploader_memcpy(uchar *pDestBuffer, uchar *pSourceBuffer,uint nCpySize)
{
ulong    i;

    for(i=0;i<nCpySize;i++)
	{
		*pDestBuffer++ = *pSourceBuffer++;
	}

}

//*----------------------------------------------------------------------------
//* Function Name       : os_apploader_memset
//* Object              : Set buffer
//* Input Parameters    : 
//* Output Parameters   : none
//* Functions called    : none
//*----------------------------------------------------------------------------
static void os_apploader_memset(uchar *pDestBuffer, uchar ucValue,uint nCpySize)
{
ulong    i;

    for(i=0;i<nCpySize;i++)
	{
		*pDestBuffer++ = ucValue;
	}

}

//*----------------------------------------------------------------------------
//* Function Name       : os_apploader_send_msg
//* Object              : Send message to queue
//* Input Parameters    : none
//* Output Parameters   : none
//* Functions called    : none
//*----------------------------------------------------------------------------
static uchar os_apploader_send_msg(xQueueHandle pvQueueHandle, ulong *ulMessageBuffer, uchar ucNumberOfItems)
{
	ulong ulDummy;
	uchar ucCount;

	if(pvQueueHandle == NULL)
		return 0;

	/* Clear Rx Queue before posting */
	while(uxQueueMessagesWaiting(pvQueueHandle))
	{
		xQueueReceive(pvQueueHandle,(void *)&ulDummy, (portTickType)0);
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
//* Function Name       : os_apploader_wait_msg
//* Object              : Read pending messages
//* Input Parameters    : Rx Queue ptr and items buffer
//* Output Parameters   : none.
//*--------------------------------------------------------------------------------------
static uchar os_apploader_wait_msg(xQueueHandle pRxQueue, ulong *ulQueueBuffer)
{
	uchar ucNext = 0;

	if(pRxQueue == NULL)
		return 0;

	*ulQueueBuffer = 0;	
	while(uxQueueMessagesWaiting(pRxQueue))
	{			
		if(xQueueReceive(pRxQueue, (ulQueueBuffer + ucNext), (portTickType)0) == pdPASS)
		{
			ucNext++;									
		}
	}

	return ucNext;
}

//*----------------------------------------------------------------------------
//* Function Name       : os_apploader_cleanup_msg
//* Object              : Send clean up message to application
//* Input Parameters    : none
//* Output Parameters   : none
//* Functions called    : none
//*----------------------------------------------------------------------------
static void os_apploader_cleanup_msg(xTaskHandle xCAppHandle)
{
	ulong 						ulRxData[5],ulTxData[5],i;

	/* Clean up call buffer, menu is 0xFF */
	uchar						ucTempBuffer[2] = {0xFF,0x00};

	/* Insert the buffer address */
	ulTxData[0] = (ulong)&ucTempBuffer;

	/* Insert the buffer size */
	ulTxData[1] = 2;

	/* Enumerate all app struct entries and find our application */
  	for(i = 0, pxAppLoaderAppParameters = pxAppLoaderAppParametersArr; i < SEDNA_MAXIMUM_APPLICATIONS; i++)
	{
		/* Exit if all entries are taken */
		if(i == (SEDNA_MAXIMUM_APPLICATIONS - 1))
			break;

		/* Found our task in the appl struct */
		if((ulong)(pxAppLoaderAppParameters->xAppHandle) == (ulong)xCAppHandle)
		{
			/* This message is sent to the application */
			os_apploader_send_msg(pxAppParameters->xAppMsgRxQueue,ulTxData,2);

			break;
		}

		/* Next entry */
		pxAppLoaderAppParameters++;
	}

	/* Wait finish status of the Clean Up routine, if not, simpy terminate */
	while(os_apploader_wait_msg(pxAppParameters->xAppMsgTxQueue,ulRxData) != 3)
	{
		/* App loader will stall here, if the application fail to send ask msg !!! */
		vTaskDelay(100);
	}

	//DebugPrint("os_apploader_cleanup_msg->success\n\r");
}

//*----------------------------------------------------------------------------
//* Function Name       : os_apploader_unload
//* Object              : Unload App - Delete from task lists and free memory
//* Input Parameters    : app handle and name
//* Output Parameters   : load result
//* Functions called    : none
//*----------------------------------------------------------------------------
static uchar os_apploader_unload(xTaskHandle xRunningTask,char *cAppName)
{
	ulong ulTask;
	uchar i;

	// ToDo: Test if user is trying to close kernel task ...
	// ...

	/* Check app name against running tasks list */
//!	ulTask = ucSearchTaskList(cAppName);
	if(ulTask == 0)
	  return SEDNA_APP_NOT_RUNNING;

	/* Check if returned handle for a found task is
	   matching the requested one */
	if(ulTask != (ulong)xRunningTask)
	  return SEDNA_APP_HANDLE_MISMATCH;

	/* Enumerate all app struct entries and find the one we want to delete */
  	for(i = 0, pxAppLoaderAppParameters = pxAppLoaderAppParametersArr; i < SEDNA_MAXIMUM_APPLICATIONS; i++)
	{
		/* Exit if all entries are taken */
		if(i == (SEDNA_MAXIMUM_APPLICATIONS - 1))
			return SEDNA_APP_NOT_RUNNING;

		/* Found our task in the appl struct */
		if((ulong)(pxAppLoaderAppParameters->xAppHandle) == (ulong)xRunningTask)
			break;

		/* Next entry */
		pxAppLoaderAppParameters++;
	}

	/* Call the cleanup routine before deleting task */
	os_apploader_cleanup_msg(xRunningTask);

	/* Delete any running app threads */
	for(i = 0;i < MAX_APPLICATION_THREADS; i++)
	{
		/* Quite risky, maybe we need some additional checks !!! */
		if(pxAppLoaderAppParameters->xAppThreads[i] != NULL)
			vTaskDelete(pxAppLoaderAppParameters->xAppThreads[i]);
	}

	/* Delete Application */
    if(xRunningTask != NULL)
      vTaskDelete( xRunningTask );
    else
      return SEDNA_APP_INVALID_HANDLE;

	/* Free the function buffer ocupied by deleted task */
	vPortFree( pxAppLoaderAppParameters->ucFunctionBuffer);

	/* Mark the entry as empty */
	pxAppLoaderAppParameters->xAppHandle = NULL;

	/* Decrease running applications counter */
	if(ucRunningApplicationsCount) ucRunningApplicationsCount--;

	return SEDNA_APP_UNLOAD_SUCCESS;
}

//*----------------------------------------------------------------------------
//* Function Name       : os_apploader_load
//* Object              : Loads and starts Sedna Application
//* Input Parameters    : usb tranfer buffer ptr
//* Output Parameters   : load result
//* Functions called    : none
//*----------------------------------------------------------------------------
static uchar os_apploader_load(char *chSomeAppName,char *chSomeCertPath, xTaskHandle *xCreatedTask,char *cAppName,APPLOADER_QUEUE_PARAMETERS *pxAppLdrParam)
{
	uchar 		ucFuncResult,i;
	uchar 		*ucCurrentFunctionBuffer;

	uchar 		ucAppName[16];
	char 		ext[10];

	uchar 		ucAppPriority = tskIDLE_PRIORITY;

	FRESULT 	res;
	FIL   		file;
	FILINFO 	fno;

	uint  		uiAppSize 	= 0;			// file size
	ulong 		read;
	NewTaskData ntd;

	// Enumerate all app struct entries and find a empty one
  	for(i = 0, pxAppLoaderAppParameters = pxAppLoaderAppParametersArr; i < SEDNA_MAXIMUM_APPLICATIONS; i++)
	{
		// Exit if all entries are taken
		if(i == (SEDNA_MAXIMUM_APPLICATIONS - 1))
			return SEDNA_APP_TOO_MANY_LOADED;

		// Found empty, use it for this application
		if(pxAppLoaderAppParameters->xAppHandle == NULL)
			break;

		// Next entry
		pxAppLoaderAppParameters++;
	}

	// Test for valid string ptr
 	if(chSomeAppName  == NULL)
 		return 10;

 	// Test for valid string ptr
 	//if(chSomeCertPath == NULL)
 	//	return 11;

 	uiAppSize = strlen(chSomeAppName);

 	// Test for valid string
 	if(uiAppSize == 0)
 		return 11;

 	//printf("file: %s \r\n", chSomeAppName);

 	// Get extension
 	os_apploader_get_ext(chSomeAppName, ext);
 	//printf("ext: %s \r\n", ext);

 	res = f_stat(chSomeAppName, &fno);
 	if(res != FR_OK)
 	{
 		printf("fs err: %d \r\n", res);
 		return 12;
 	}

 	uiAppSize = fno.fsize;
 	//printf("size %d bytes\r\n", uiAppSize);

 	if(strcmp(ext, "elf") == 0)
 	{
 		goto process_elf;
 	}
 	else if(strcmp(ext, "bin") == 0)
 	{
 		uchar *p_f = (uchar *)SDRAM_APP_ADDR;
 		uchar app_header[32];
 		ulong alloc_addr = 0;

 		// Open for read
 	 	res = f_open(&file, chSomeAppName, FA_READ);
 		if(res != FR_OK)
 			return 13;

 		// Read header
 		res = f_read(&file, app_header, 32, (void *)&read);
 		if(res != FR_OK)
		{
 			printf("fs header err: %d \r\n", res);
 			f_close(&file);
 			return 14;
 		}
 		//--print_hex_array(app_header, 8);

 		// Check for magic
 		if( (app_header[0] != 0x69) ||
 	   		(app_header[1] != 0x6D) ||
 	   		(app_header[2] != 0x67) ||
 		 	(app_header[3] != 0x00) )
 	   	{
 			f_close(&file);
 			return 15;
 	   	}

 		// Extract allocation address(stack_ptr - app_area size)
 		alloc_addr  = app_header[4];
 		alloc_addr |= app_header[5] <<  8;
 		alloc_addr |= app_header[6] << 16;
 		alloc_addr |= app_header[7] << 24;
 		alloc_addr -= MAX_CHUNK_IN_RAM;

 		//printf("alloc: %08x, file: %08x \r\n", pxAppLoaderAppParameters->ext_ram_addr, alloc_addr);

 		// Is linker allocation matching section we have selected ?
 		if(pxAppLoaderAppParameters->ext_ram_addr != alloc_addr)
 		{
 			f_close(&file);
 			return 16;
 		}

 		// Header into area
 		memcpy(p_f, app_header, 32);

 		// Read whole app
 		res = f_read(&file,(p_f + 32), (uiAppSize - 32), (void *)&read);
 		if(res != FR_OK)
 		{
 			printf("fs err: %d \r\n", res);
 			f_close(&file);
 			return 17;
 		}

 		// Close file
 		f_close(&file);

 		ulong chk = 0;
 		for(int i = 0; i < uiAppSize; i++)
 			chk += *p_f++;

 		//printf("checksum: 0x%08x \r\n", (int)chk);

 		// ToDo: do something with the checksum...
 		//

 		// Reload ptr
 		p_f = (uchar *)(pxAppLoaderAppParameters->ext_ram_addr);

 		// Get ptr from descriptor
 		ulong app_addr = *(ulong *)(p_f + SEDNA_APP_ENTRY_FUNC_SHIFT);
 		//printf("address:  0x%08x \r\n", (int)app_addr);

 		// Set function pointer to allocated space
 		pvAppLoaderDinamiclyLoadedFunc = (pdTASK_CODE)app_addr;

 		// Copy string descriptor, maybe test for valid name ???
 		os_apploader_memcpy(ucAppName, (p_f + SEDNA_APP_DESC_NAME_SHIFT), configMAX_TASK_NAME_LEN);

 		// Load priority from descriptor and test if not too high
 		//ucAppPriority = *(p_f + SEDNA_APP_DESC_PRIORITY_SHIFT);
 		//if(ucAppPriority > (tskIDLE_PRIORITY + 3) )
 		  ucAppPriority = osPriorityLow;//tskIDLE_PRIORITY + 3;

 		printf("app load ok \r\n");
 		goto processed;
 	}
 	else
 		return 18;

process_elf:

 	// Check size
	if((uiAppSize == 0) || (uiAppSize > 0x8000))
		return 19;

 	// Open application by name, the Current Dir is already selected by UI task
 	res = f_open(&file, chSomeAppName, FA_READ);
	if(res != FR_OK)
		return 20;

	// Allocate function buffer, at the moment allocate ELF size,
	// not the real process size - to be fixed eventually !
	ucCurrentFunctionBuffer = pvPortMalloc(uiAppSize);
	if(ucCurrentFunctionBuffer == NULL)
	{
		f_close(&file);
		return SEDNA_APP_ALLOC_ERROR;
	}

	// Read data
	if(f_read(&file, ucCurrentFunctionBuffer, uiAppSize, (void *)&read) != FR_OK)
	{
		// Release app memory
		vPortFree(ucCurrentFunctionBuffer);
		f_close(&file);

		return 21;
	}

	// Close file
	f_close(&file);

	// Temp debug possibility to run plain ELF files - REMOVE ON RELEASE BUILD !!!
	if( (*(ucCurrentFunctionBuffer + 0) != 0x7F) ||
   		(*(ucCurrentFunctionBuffer + 1) != 0x45) ||
   		(*(ucCurrentFunctionBuffer + 2) != 0x4C) ||
	 	(*(ucCurrentFunctionBuffer + 3) != 0x46) )
   	{
		return 44;
   	}

	// Extract the process image from the ELF file
	ucFuncResult = ucSednaElfProcessElf(ucCurrentFunctionBuffer);
	if(ucFuncResult != SEDNA_APP_LOAD_SUCCESS)
	{
	  	vPortFree(ucCurrentFunctionBuffer);
	  	return 22;
	}

	// Set function pointer to allocated space
	pvAppLoaderDinamiclyLoadedFunc = (pdTASK_CODE) ucCurrentFunctionBuffer;

	//DebugPrintValue("app addr",(ulong)ucCurrentFunctionBuffer);

	// ----------------------------------------------------------------------
	//   Extract application descriptor block
	// ----------------------------------------------------------------------

	// Copy string descriptor, maybe test for valid name ???
	os_apploader_memcpy(ucAppName,(ucCurrentFunctionBuffer + SEDNA_APP_DESC_NAME_SHIFT),configMAX_TASK_NAME_LEN);

	// Load priority from descriptor and test if not too high
	ucAppPriority = *(ucCurrentFunctionBuffer + SEDNA_APP_DESC_PRIORITY_SHIFT);
	if( ucAppPriority > (tskIDLE_PRIORITY + 3) )
	  ucAppPriority = tskIDLE_PRIORITY + 3;

processed:

	// ----------------------------------------------------------------------

	// Create the structure used to pass
	// parameters to the running application
	pxAppParameters = (APP_PARAMETERS *)pvPortMalloc(sizeof(APP_PARAMETERS));

	// Create the RX and TX queue
	pxAppParameters->xAppMsgRxQueue = xQueueCreate( ucQueueSize, (unsigned portCHAR) sizeof(ulong));
	pxAppParameters->xAppMsgTxQueue = xQueueCreate( ucQueueSize, (unsigned portCHAR) sizeof(ulong));
	pxAppParameters->ulApiCallBase  = ulSednaExportsBaseAddress();

	ntd.pvTaskCode 		= pvAppLoaderDinamiclyLoadedFunc;
	ntd.pcName			= (char *)ucAppName;
	ntd.usStackDepth	= (configMINIMAL_STACK_SIZE*64);	//C_APPL_STACK_SIZE;
	ntd.pvParameters	= (void *)pxAppParameters;
	ntd.ucPriority		= ucAppPriority;
	ntd.pxCreatedTask	= xCreatedTask;

	if(os_apploader_create_proc(&ntd) != pdPASS)
	{
		// Could not create the task
		ucFuncResult = SEDNA_APP_EXECUTE_ERROR;

		// Release app memory
		vPortFree(ucCurrentFunctionBuffer);
	}
	else
	{
		// Task start success
		ucFuncResult = SEDNA_APP_LOAD_SUCCESS;

		// Save application name and handle
		os_apploader_memcpy((uchar *)pxAppLoaderAppParameters->pcAppName,ucAppName,configMAX_TASK_NAME_LEN);
	    pxAppLoaderAppParameters->xAppHandle = *xCreatedTask;

	    // Save allocated application ptr
		pxAppLoaderAppParameters->ucFunctionBuffer = ucCurrentFunctionBuffer;

	    // Save allocated task space size
	    pxAppLoaderAppParameters->ulFuncSpaceSize = uiAppSize;

	    // Clear app threads ptrs array
	    for(i = 0;i < MAX_APPLICATION_THREADS; i++)
	       pxAppLoaderAppParameters->xAppThreads[i] = NULL;

	    // Increase running applications counter
	    ucRunningApplicationsCount++;
	}

	// Copy application name, so we can return it to creator
	os_apploader_memcpy((uchar *)cAppName,ucAppName,configMAX_TASK_NAME_LEN);

	return ucFuncResult;
}

//*----------------------------------------------------------------------------
//* Function Name       : os_apploader_task
//* Object              : App loader service task
//* Input Parameters    :
//* Output Parameters   :
//* Functions called    :
//*----------------------------------------------------------------------------
void os_apploader_task(void *pvParameters)
{
	APPLOADER_QUEUE_PARAMETERS 	*pxAppLdrParameters;
	ulong 						ulRxData[10],ulTxData[10],ulRxDataA[10];
	uchar						ucCallResult,i;
	char						cCreatedAppName[16];
	xTaskHandle 				xAppHandle;

	/* This var will allow receive and transmit of application
	   messages. When dumped from the application queue, they
	   will be passed on to the USBFTDI driver Tx queue. The
	   var is true only after SEDNA_SEND_APP_MESSAGE command,
	   then when queue is cleared, the var is set to false */
	uchar						ucApplicationMessagesExpected = 0;

	/* The queues being used are passed in as the parameters struct. */
	pxAppLdrParameters = (APPLOADER_APP_PARAMETERS *)pvParameters;

    /* Copy to public, needed by some application API's instead of the Service Task loop */
    pxAppLdrParametersPub = pxAppLdrParameters;

	vTaskDelay(APP_PROC_START_DELAY);
	printf("start\r\n");

	for(;;)
	{
		// Acknoledge that the task is still running
		pxAppLdrParameters->ulTasksStatus |= APP_LOADER_TASK;

		// Wait messages
		if(os_apploader_wait_msg(pxAppLdrParameters->xAppLoaderRxQueue,ulRxData) > 0)
		{
			// Process message request
			switch(ulRxData[0])
			{
				// Execute Load request
				case SEDNA_LOAD_APPLICATION:
				{
					// Load Application
				    ucCallResult = os_apploader_load((char *)ulRxData[1],(char *)ulRxData[2],&xAppHandle,cCreatedAppName,pxAppLdrParameters);

					//printf("load app res %d \r\n",ucCallResult);

		            // Insert the request ID
				    ulTxData[0] = SEDNA_RETURN_LOAD_STATUS;
				    ulTxData[1] = ucCallResult;

				    // Insert the handle
				    ulTxData[2] = (ulong)xAppHandle;

		            // Insert the app name
					ulTxData[3] = (cCreatedAppName[0] << 24)|(cCreatedAppName[1] << 16)|(cCreatedAppName[2] << 8)|(cCreatedAppName[3]);
					ulTxData[4] = (cCreatedAppName[4] << 24)|(cCreatedAppName[5] << 16)|(cCreatedAppName[6] << 8)|(cCreatedAppName[7]);
					ulTxData[5] = (cCreatedAppName[8] << 24)|(cCreatedAppName[9] << 16)|(cCreatedAppName[10]<< 8)|(cCreatedAppName[11]);
					ulTxData[6] = (cCreatedAppName[12]<< 24)|(cCreatedAppName[13]<< 16)|(cCreatedAppName[14]<< 8)|(cCreatedAppName[15]);

					// Post the result, if success include handle and name
		            if(ucCallResult == SEDNA_APP_LOAD_SUCCESS)
  		              os_apploader_send_msg(pxAppLdrParameters->xAppLoaderTxQueue,ulTxData,7);
		            else
		              os_apploader_send_msg(pxAppLdrParameters->xAppLoaderTxQueue,ulTxData,2);

					break;
				}

				// Execute Unload request
				case SEDNA_UNLOAD_APPLICATION:
				{
					// Extract Handle from message
					xAppHandle = (void *)ulRxData[1];

					// Extract App name from message
					cCreatedAppName[0]  = (char)(ulRxData[2] >> 24);
					cCreatedAppName[1]  = (char)(ulRxData[2] >> 16);
					cCreatedAppName[2]  = (char)(ulRxData[2] >>  8);
					cCreatedAppName[3]  = (char)(ulRxData[2] & 0xFF);
					cCreatedAppName[4]  = (char)(ulRxData[3] >> 24);
					cCreatedAppName[5]  = (char)(ulRxData[3] >> 16);
					cCreatedAppName[6]  = (char)(ulRxData[3] >>  8);
					cCreatedAppName[7]  = (char)(ulRxData[3] & 0xFF);
					cCreatedAppName[8]  = (char)(ulRxData[4] >> 24);
					cCreatedAppName[9]  = (char)(ulRxData[4] >> 16);
					cCreatedAppName[10] = (char)(ulRxData[4] >>  8);
					cCreatedAppName[11] = (char)(ulRxData[4] & 0xFF);
					cCreatedAppName[12] = (char)(ulRxData[5] >> 24);
					cCreatedAppName[13] = (char)(ulRxData[5] >> 16);
					cCreatedAppName[14] = (char)(ulRxData[5] >>  8);
					cCreatedAppName[15] = (char)(ulRxData[5] & 0xFF);

					// Unload Application
					ucCallResult = os_apploader_unload(xAppHandle,cCreatedAppName);

		            // Insert the request ID
				    ulTxData[0] = SEDNA_RETURN_UNLOAD_STATUS;
				    ulTxData[1] = ucCallResult;

		            // Post result
		            os_apploader_send_msg(pxAppLdrParameters->xAppLoaderTxQueue,ulTxData,2);

					break;
				}

				// Post message to running application
				case SEDNA_SEND_APP_MESSAGE:
				{
					uchar *ucHandlePtr = (uchar *)ulRxData[1];
					ulong ulHandle;

					// Extract Handle from message
					ulHandle  = *(ucHandlePtr + 0) << 24;
					ulHandle |= *(ucHandlePtr + 1) << 16;
					ulHandle |= *(ucHandlePtr + 2) <<  8;
					ulHandle |= *(ucHandlePtr + 3);
					xAppHandle = (void *)ulHandle;

		            // Insert the buffer address, remove the handle by shifting ptr
				    ulTxData[0] = ulRxData[1] + 4;

		            // Insert the buffer size
				    ulTxData[1] = ulRxData[2] - 4;

				   	// Enumerate all app struct entries and find our application
  					for(i = 0, pxAppLoaderAppParameters = pxAppLoaderAppParametersArr; i < SEDNA_MAXIMUM_APPLICATIONS; i++)
					{
						// Exit if all entries are taken
						if(i == (SEDNA_MAXIMUM_APPLICATIONS - 1))
							break;

						// Found our task in the appl struct
						if((ulong)(pxAppLoaderAppParameters->xAppHandle) == (ulong)xAppHandle)
						{
							// This message is sent to the application, not USB driver (!)
				            os_apploader_send_msg(pxAppParameters->xAppMsgRxQueue,ulTxData,2);

							// Allow dumping of application messages
							ucApplicationMessagesExpected = 1;

							break;
						}

						// Next entry
						pxAppLoaderAppParameters++;
					}

					break;
				}

				default:
			    	break;
			}
		}

		// Re-translate all application messages to USBFTDI driver
		if(ucApplicationMessagesExpected == 1)
		{
			// Wait any message from Application Queue and pass it to the USBFTDI driver queue
			if(os_apploader_wait_msg(pxAppParameters->xAppMsgTxQueue,ulRxDataA) == 3)
			{
				//DebugPrint("vAppLoaderTask->SEDNA_RETURN_APP_MSG_STATUS\n\r");

		        // Insert the request ID
		    	ulTxData[0] = SEDNA_RETURN_APP_MSG_STATUS;

		       	// Insert the function result
		    	ulTxData[1] = ulRxDataA[0];

				// Insert the buffer ptr
		    	ulTxData[2] = ulRxDataA[1];
		    	//DebugPrintBlock("",(uchar *)ulTxData[2],16);

		       	// Insert the buffer size
		    	ulTxData[3] = ulRxDataA[2];

		    	// Post to the return queue - 4 bytes
		        os_apploader_send_msg(pxAppLdrParameters->xAppLoaderTxQueue,ulTxData,4);

				// We don't expect any messages any more, set to false
				ucApplicationMessagesExpected = 0;
			}
			// case else - > application hasnt posted yet, will try again on next run
		}

		// Go to sleep if not used, otherwise very short release of execution to speed up
		//   application messages processing
		if(ucRunningApplicationsCount)
			vTaskDelay(APP_LDR_SHORT_SLEEP);
		else
			vTaskDelay(APP_LDR_LONG_SLEEP);

		//xxTestEvent ^= 1;
	}
}

//*----------------------------------------------------------------------------
//* Function Name       : os_apploader_init
//* Object              : start loader service
//* Input Parameters    : none
//* Output Parameters   : none
//* Functions called    : none
//*----------------------------------------------------------------------------
void os_apploader_init(void)
{    
	uchar 		i;

	// Clear applications struct - we clear app handle, as we use it
	// for enumeration later 
  	for(i = 0,pxAppLoaderAppParameters = pxAppLoaderAppParametersArr;i < SEDNA_MAXIMUM_APPLICATIONS; i++)  	
	{
		// Add free ext ram locations
		pxAppLoaderAppParameters->ext_ram_addr = SDRAM_APP_ADDR + (MAX_CHUNK_IN_RAM*i);

		pxAppLoaderAppParameters->xAppHandle = NULL;
		pxAppLoaderAppParameters++;
	}
}
#endif
