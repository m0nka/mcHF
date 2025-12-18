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

// Storage Host logical drive number
char StorageDISK_Drive[4];

#ifdef CONTEXT_SD

#include "sd_card.h"
#include "sd_diskio.h"

#include "storage_proc.h"

// File system object for MSD disk logical drive
#ifdef SD_USE_DMA
// ToDo: when upgrading, move 'win' member at the base pointer,
//       so this alignment actually works!
__attribute__((section(".axi_mem"))) __attribute__ ((aligned (32))) FATFS StorageDISK_FatFs[NUM_DISK_UNITS];
#else
FATFS StorageDISK_FatFs[NUM_DISK_UNITS];
#endif

osSemaphoreId      					StorageSemaphore[NUM_DISK_UNITS];
static Diskio_drvTypeDef  	const 	*Storage_Driver[NUM_DISK_UNITS];

uint8_t           					StorageID[NUM_DISK_UNITS];
STORAGE_Status_t   					StorageStatus[NUM_DISK_UNITS];

extern TaskHandle_t 				hSdcTask;

//*----------------------------------------------------------------------------
//* Function Name       : EXTI0_IRQHandler
//* Object              :
//* Notes    			: exti trap, line0
//* Notes   			:
//* Notes    			:
//* Context    			: CONTEXT_IRQ
//*----------------------------------------------------------------------------
void EXTI0_IRQHandler(void)
{
	if(__HAL_GPIO_EXTI_GET_IT(GPIO_PIN_0) != 0x00U)
	{
		BaseType_t xHigherPriorityTaskWoken;

		xHigherPriorityTaskWoken = pdFALSE;
		xTaskNotifyFromISR(hSdcTask, 0x45, eSetBits, &xHigherPriorityTaskWoken);
		portYIELD_FROM_ISR(xHigherPriorityTaskWoken );

		__HAL_GPIO_EXTI_CLEAR_IT(GPIO_PIN_0);
	}
}

//*----------------------------------------------------------------------------
//* Function Name       : storage_proc_try_mount
//* Object              :
//* Notes    			:
//* Notes   			:
//* Notes    			:
//* Context    			: CONTEXT_SD
//*----------------------------------------------------------------------------
static STORAGE_Status_t storage_proc_try_mount( const uint8_t unit )
{
	//printf("StorageTryMount  \r\n");

	osSemaphoreWait(StorageSemaphore[unit], osWaitForever);

	if(unit == MSD_DISK_UNIT)
	{
		// We need to check for SD Card before mounting the volume
		if(!sd_card_is_detected())
		{
			StorageStatus[unit] = STORAGE_UNMOUNTED;
			goto unlock_exit;
		}
	}

	if(StorageStatus[unit] != STORAGE_MOUNTED)
	{
		// Link the disk I/O driver
		if(FATFS_LinkDriver(Storage_Driver[unit], StorageDISK_Drive) != 0)
			goto unlock_exit;

		if(f_mount(&StorageDISK_FatFs[unit], StorageDISK_Drive, 0))
			goto unlock_exit;

		//printf("drive: %s \r\n", StorageDISK_Drive);

		// Set SD storage status
		// if(unit == MSD_DISK_UNIT)
		StorageStatus[unit] = STORAGE_MOUNTED;
	}

unlock_exit:
  	  osSemaphoreRelease(StorageSemaphore[unit]);
  	  return StorageStatus[unit];
}

//*----------------------------------------------------------------------------
//* Function Name       : storage_proc_try_unmount
//* Object              :
//* Notes    			:
//* Notes   			:
//* Notes    			:
//* Context    			: CONTEXT_SD
//*----------------------------------------------------------------------------
static STORAGE_Status_t storage_proc_try_unmount( const uint8_t unit )
{
	//printf("storage_proc_try_unmount...  \r\n");

	if(StorageID[unit] == 0)
		return StorageStatus[unit];

	osSemaphoreWait(StorageSemaphore[unit], osWaitForever);

	if(StorageStatus[unit] != STORAGE_MOUNTED)
		goto unlock_exit;

	if(f_mount(0, StorageDISK_Drive, 0))
		goto unlock_exit;

	if(FATFS_UnLinkDriver(StorageDISK_Drive))
		goto unlock_exit;

	memset(StorageDISK_Drive, 0, sizeof(StorageDISK_Drive));

	// Reset storage status
	StorageStatus[unit] = STORAGE_UNMOUNTED;

unlock_exit:
	osSemaphoreRelease(StorageSemaphore[unit]);

	//printf("storage_proc_try_unmount...ok  \r\n");
	return StorageStatus[unit];
}

//*----------------------------------------------------------------------------
//* Function Name       : storage_proc_init_msd
//* Object              :
//* Notes    			:
//* Notes   			:
//* Notes    			:
//* Context    			: CONTEXT_SD
//*----------------------------------------------------------------------------
static uint8_t storage_proc_init_msd(void)
{
	uint8_t sd_status = BSP_ERROR_NONE;

	// Init done already ?
	if(StorageID[MSD_DISK_UNIT] != 0)
	{
		printf("skip card init  \r\n");
		return 1;
	}

	// Exit on no card
	if(sd_card_is_detected() == SD_NOT_PRESENT)
	{
		printf("card not avail  \r\n");
		return 2;
	}

	// Card init
	sd_status = sd_card_init();
	if(sd_status != BSP_ERROR_NONE)
	{
		printf("card init failed(%d)  \r\n", sd_status);
		return 3;
	}

	// Mark the storage as initialised
	StorageID[MSD_DISK_UNIT] = 1;
	Storage_Driver[MSD_DISK_UNIT] = &SD_Driver;

	// Try mount the storage
	storage_proc_try_mount(MSD_DISK_UNIT);

	#if 1
	if(StorageStatus[MSD_DISK_UNIT] != STORAGE_MOUNTED)
		printf("card not ready!  \r\n");
	else
		printf("card ready  \r\n");
	#endif

	return 0;
}

static void storage_proc_connection_evt(void)
{
	if(sd_card_is_detected() != SD_PRESENT)
		return;

	printf("card inserted  \r\n");

	// Power on
	sd_card_power(1);

	// Power on delay
	vTaskDelay(100);

	// In case the card was not inserted on reset
	if(storage_proc_init_msd() != 0)
	{
		// FS Clean-up
		//--storage_proc_try_unmount(MSD_DISK_UNIT);

		//
		// ToDo: How do we properly reset drivers(bad init result, read errors, etc)
		//
		sd_card_reset_driver();

		// Power off
		sd_card_power(0);

		//--StorageID[MSD_DISK_UNIT] = STORAGE_NOINIT;
	}
}

static void storage_proc_disconnect_evt(void)
{
	if(sd_card_is_detected() != SD_NOT_PRESENT)
		return;

	printf("card surprise removal  \r\n");

	// FS Clean-up
	storage_proc_try_unmount(MSD_DISK_UNIT);

	// Power off
	sd_card_power(0);

	StorageID[MSD_DISK_UNIT] = STORAGE_NOINIT;
}

//*----------------------------------------------------------------------------
//* Function Name       : storage_proc_task
//* Object              :
//* Notes    			:
//* Notes   			:
//* Notes    			:
//* Context    			: CONTEXT_SD
//*----------------------------------------------------------------------------
void storage_proc_task(void const * argument)
{
	//osEvent event;
	ulong 	ulNotificationValue = 0, ulNotif;

	vTaskDelay(SD_PROC_START_DELAY);
	printf("start  \r\n");

	#ifndef SD_DETECT_BEFORE_OS
	//
	// Initialize the MSD Storage
	storage_proc_init_msd();
	//
	// Configure SD Interrupt mode
	sd_card_set_exti_irq(0);
	//
	#endif

	//
	// ToDo: Get from ini file...
	//
    // Set radio public values
    radio_init_on_reset();

	for(;;)
	{
		#if 0
		event = osMessageGet(StorageEvent, osWaitForever);
		if(event.status == osEventMessage)
		#else
		ulNotif = xTaskNotifyWait(0x00, ULONG_MAX, &ulNotificationValue, SD_PROC_SLEEP_TIME);
		if((ulNotif) && (ulNotificationValue))
		#endif
		{
			//--printf("notif: %d  \r\n", ulNotificationValue);

			// Lock IRQ
			HAL_NVIC_DisableIRQ  (EXTI0_IRQn);

			// Debounce delay
			vTaskDelay(80);

			ulong port_val = HAL_GPIO_ReadPin(SD_DET_PORT, SD_DET);

			//--printf("after debounce(%d)\r\n", (int)port_val);

			// Overwrite with debounce value
			if(!port_val)
				ulNotificationValue = MSDDISK_CONNECTION_EVENT;
			else
				ulNotificationValue = MSDDISK_DISCONNECTION_EVENT;

			switch(ulNotificationValue)
			{
        		case MSDDISK_CONNECTION_EVENT:
        			storage_proc_connection_evt();
        			break;

        		case MSDDISK_DISCONNECTION_EVENT:
        			storage_proc_disconnect_evt();
        			break;

        		// Add Eject button from UI
        		// ...

        		default:
        			break;
			}

			// Unlock IRQ
			HAL_NVIC_EnableIRQ  (EXTI0_IRQn);
		}
	}
}

//*----------------------------------------------------------------------------
//* Function Name       : storage_proc_init
//* Object              :
//* Notes    			: pre-os init
//* Notes   			:
//* Notes    			:
//* Context    			: CONTEXT_RESET
//*----------------------------------------------------------------------------
void storage_proc_init(void)
{
	uint8_t storage_id = 0;

	// Reset All storage status
	//for(storage_id = 0; storage_id < sizeof(StorageStatus); storage_id++)
	//{
	//	StorageStatus[storage_id] = STORAGE_NOINIT;
	//}
	StorageStatus[storage_id] = STORAGE_NOINIT;

	// Create Storage Semaphore
	osSemaphoreDef(STORAGE_MSD_Semaphore);
	StorageSemaphore[MSD_DISK_UNIT] = osSemaphoreCreate (osSemaphore(STORAGE_MSD_Semaphore), 1);

	// GPIO init
	sd_card_low_level_init();

	//
	// Are we detecting before the OS is running ?
	//
	#ifdef SD_DETECT_BEFORE_OS
	//
	// Initialize the MSD Storage
	storage_proc_init_msd();
	//
	// Configure SD Interrupt mode
	sd_card_set_exti_irq();
	//
	#endif

	//--printf("sd low init \r\n");
}

#endif
