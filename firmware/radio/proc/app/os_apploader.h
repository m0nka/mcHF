/************************************************************************************
**                                                                                 **
**                          GENIE RTOS AT91 IMPLEMENTATION                         **
**                                  B-phreaks, 2005                                **
**                                                                                 **
**---------------------------------------------------------------------------------**
**                                                                                 **
**  File name:                                                                     **
**  Description:                                                                   **
**  Last Modified:                                                                 **
**                                                                                 **
************************************************************************************/

#ifndef __OS_APPLOADER_H
#define __OS_APPLOADER_H

#define SEDNA_MAXIMUM_APPLICATIONS			   2
#define MAX_APPLICATION_THREADS				   2

// --------------------------------------------------------
// This task stack size
//
// New application task size
#define C_APPL_STACK_SIZE			(MIN_STACK_SIZE * 32)
//
// Script stack size
#define SCRIPT_STACK_SIZE			(MIN_STACK_SIZE * 32)
// --------------------------------------------------------

/* Sleep const's */
#define APP_LDR_LONG_SLEEP			100
#define APP_LDR_SHORT_SLEEP			8

#define APPLICATION_HEADER_SIZE		256

#define APP_LOADER_TASK          	(1 <<  4)

#define MIN_STACK_SIZE				32

#define APP_LOADER_QUEUE_SIZE		10

/* ----------- Struct to hold loaded applications parameters ------------ 	*/
typedef struct APPLOADER_APP_PARAMETERS
{
	/* Allocated app ptr. We keep the ptr to Application allocated space here.
	   We can free the space on unload using this entry from the struct 	*/
	uchar 		*ucFunctionBuffer;

	/* Allocated size. We need this to map the Application Task space when
	   processing StartThread requests.										*/
	ulong		ulFuncSpaceSize;

	/* App name. Used together with the handle for Unload and SendMsg		*/
	portCHAR	pcAppName[configMAX_TASK_NAME_LEN];

	/* App handle. Spare copy of the Task handle							*/
	xTaskHandle xAppHandle;

	/* App threads ptr arr. Here all ptr to threads started by the Appl	    */
	xTaskHandle xAppThreads[MAX_APPLICATION_THREADS];

} APPLOADER_APP_PARAMETERS;

/* ---- Structure used to pass parameters to the running application. --- 					*/
typedef struct APP_PARAMETERS
{
	xQueueHandle 	xAppMsgRxQueue;	/* The TX queue 'OS    		-> Application' 	   		*/
	xQueueHandle	xAppMsgTxQueue;	/* The RX queue 'Application -> OS	      '  	  		*/
	ulong			ulApiCallBase;	/* Base ptr for all Sedna API calls						*/

} APP_PARAMETERS;

/* Structure used to pass parameters to the app loader task. */
typedef struct APPLOADER_QUEUE_PARAMETERS
{
	xQueueHandle 		xAppLoaderRxQueue;		/* The TX queue UsbDrv    -> AppLoader          */
	xQueueHandle 		xAppLoaderTxQueue;		/* The RX queue AppLoader -> UsbDriver          */
	portTickType 		xBlockTime;				/* The block time to use on queue reads/writes. */

	xQueueHandle 		xI2CDriverRxQueue;		/* The TX queue I2CDriver -> AppLoader          */
	xQueueHandle 		xI2CDriverTxQueue;		/* The RX queue AppLoader -> I2CDriver          */

	xQueueHandle 		xCryptoServiceRxQueue;	/* The TX queue Crypto Service -> AppLoader     */
	xQueueHandle 		xCryptoServiceTxQueue;	/* The RX queue AppLoader -> Crypto Service     */

	ulong				ulTasksStatus;			// Each bit represent a status of a task

} APPLOADER_QUEUE_PARAMETERS;

typedef struct NewTaskData
{
	pdTASK_CODE 		pvTaskCode;
	void 				*pvParameters;
	xTaskHandle 		*pxCreatedTask;

	unsigned portSHORT 	usStackDepth;

	portCHAR 			*pcName;
	unsigned portCHAR 	ucPriority;

	uchar 				ucType;
	uchar 				ulOwnerId;
	uchar 				ucThumbFlag;

} NewTaskData;

//void  	ucAppLoaderStartTask(NewTaskData *ntd,void *xAppLoaderParams, unsigned portCHAR ucPriority,ushort usStackSize);
void os_apploader_task(void *pvParameters);
void os_apploader_init(void);

void 	*pvSednaCreateThread(pdTASK_CODE pvTaskCode, const portCHAR *pcName, void *pvParameters);

#endif
