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
__attribute__((section(".axi_ram"))) __attribute__ ((aligned (32))) FATFS StorageDISK_FatFs[NUM_DISK_UNITS];

static osSemaphoreId      	StorageSemaphore[NUM_DISK_UNITS];
static Diskio_drvTypeDef  	const * Storage_Driver[NUM_DISK_UNITS];

static  uint8_t           	StorageID[NUM_DISK_UNITS];
static STORAGE_Status_t   	StorageStatus[NUM_DISK_UNITS];

osThreadId                	StorageThreadId = {0};

//*----------------------------------------------------------------------------
//* Function Name       : storage_proc_detect_sd_card
//* Object              :
//* Notes    			:
//* Notes   			:
//* Notes    			:
//* Context    			: CONTEXT_IRQ
//*----------------------------------------------------------------------------
void storage_proc_detect_sd_card(ulong state)
{
	//if(!StorageEvent)
	//	return;

	if(StorageThreadId == NULL)
		return;

	// Notify storage thread
	#if 0
	if(!state)
	{
		printf("conn  \r\n");
		osMessagePut(StorageEvent, MSDDISK_CONNECTION_EVENT, 	0);
	}
	else
		osMessagePut(StorageEvent, MSDDISK_DISCONNECTION_EVENT, 0);
	#else
	BaseType_t xHigherPriorityTaskWoken;
	ulong id;

	if(!state)
		id = MSDDISK_CONNECTION_EVENT;
	else
		id = MSDDISK_DISCONNECTION_EVENT;

	xHigherPriorityTaskWoken = pdFALSE;
	xTaskNotifyFromISR(StorageThreadId, id, eSetBits, &xHigherPriorityTaskWoken );
	portYIELD_FROM_ISR(xHigherPriorityTaskWoken );
	#endif
}

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
		ulong port_val = HAL_GPIO_ReadPin(SD_DET_PORT, SD_DET);

		//printf("sd irq(%d)\r\n", (int)port_val);
		storage_proc_detect_sd_card(port_val);

		__HAL_GPIO_EXTI_CLEAR_IT(GPIO_PIN_0);
	}
}

static STORAGE_Status_t StorageTryMount( const uint8_t unit )
{
	//printf("StorageTryMount  \r\n");

	osSemaphoreWait(StorageSemaphore[unit], osWaitForever);

	if(unit == MSD_DISK_UNIT)
	{
		// We need to check for SD Card before mounting the volume
		if(!BSP_SD_IsDetected())
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

		// Set SD storage status
		// if(unit == MSD_DISK_UNIT)
		StorageStatus[unit] = STORAGE_MOUNTED;
	}

unlock_exit:
  	  osSemaphoreRelease(StorageSemaphore[unit]);

  	  return StorageStatus[unit];
}

static STORAGE_Status_t StorageTryUnMount( const uint8_t unit )
{
	//printf("StorageTryUnMount...  \r\n");

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

	//printf("StorageTryUnMount...ok  \r\n");
	return StorageStatus[unit];
}

static uint8_t storage_proc_init_msd(void)
{
	uint8_t sd_status = BSP_ERROR_NONE;

	// Init done already ?
	if(StorageID[MSD_DISK_UNIT] != 0)
	{
		printf("skip card init  \r\n");
		return StorageID[MSD_DISK_UNIT];
	}

	// Exit on no card
	if(BSP_SD_IsDetected() == SD_NOT_PRESENT)
		return BSP_ERROR_NONE;

	// Card init
	sd_status = sd_card_init(0);
	if(sd_status != BSP_ERROR_NONE)
	{
		printf("card init failed(%d)  \r\n", sd_status);
		return BSP_ERROR_NONE;
	}

	// Create Storage Semaphore
	osSemaphoreDef(STORAGE_MSD_Semaphore);
	StorageSemaphore[MSD_DISK_UNIT] = osSemaphoreCreate (osSemaphore(STORAGE_MSD_Semaphore), 1);

	// Mark the storage as initialised
	StorageID[MSD_DISK_UNIT] = 1;
	Storage_Driver[MSD_DISK_UNIT] = &SD_Driver;

	// Try mount the storage
	StorageTryMount(MSD_DISK_UNIT);

	printf("card ok  \r\n");
	return StorageID[MSD_DISK_UNIT];
}

void StorageThread(void const * argument)
{
	//osEvent event;
	ulong 	ulNotificationValue = 0, ulNotif;

	vTaskDelay(SD_PROC_START_DELAY);
	printf("start  \r\n");

	// Initialize the MSD Storage
	#if 1
	storage_proc_init_msd();

	// Configure SD Interrupt mode
	sd_card_set_exti_irq(0);
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
			vTaskDelay(100);

			//switch(event.value.v)
			switch(ulNotificationValue)
			{
        		case MSDDISK_CONNECTION_EVENT:
        		{
        			// Debounce
        			if(BSP_SD_IsDetected() != SD_PRESENT)
        				break;

        			printf("card inserted  \r\n");

        			// Power on
        			sd_card_power(1);

        			// Power on delay
        			vTaskDelay(100);

        			// In case the card was not inserted on reset
        			storage_proc_init_msd();

        			// Card init
					#if 0
        			if(sd_card_init(0) != BSP_ERROR_NONE)
        				break;
					#endif

        			// Mount FS
        			StorageTryMount(MSD_DISK_UNIT);

        			// Test only
        			//if(StorageStatus[0] == STORAGE_MOUNTED)
        			//	printf("card size: 0x%x  \r\n", (int)Storage_GetCapacity(0));

        			break;
        		}

        		case MSDDISK_DISCONNECTION_EVENT:
        		{
        			// Debounce
        			if(BSP_SD_IsDetected() != SD_NOT_PRESENT)
        				break;

        			printf("card surprise removal  \r\n");

        			// FS Clean-up
        			StorageTryUnMount(MSD_DISK_UNIT);

        			// Power off
        			sd_card_power(0);

        			StorageID[MSD_DISK_UNIT] = STORAGE_NOINIT;

        			break;
        		}

        		default:
        			break;
			}

			// Unlock IRQ
			HAL_NVIC_EnableIRQ  (EXTI0_IRQn);
		}
	}
}

static void StorageDeInitMSD(void)
{
	#ifdef STORAGE_BSP_INIT
	BSP_SD_DeInit(0);
	#endif // STORAGE_BSP_INIT
  
	if(StorageSemaphore[MSD_DISK_UNIT])
	{
		osSemaphoreDelete(StorageSemaphore[MSD_DISK_UNIT]);
		StorageSemaphore[MSD_DISK_UNIT] = 0;
	}
}

void Storage_Init(void)
{
	uint8_t storage_id = 0;

	// Reset All storage status
	for(storage_id = 0; storage_id < sizeof(StorageStatus); storage_id++)
	{
		StorageStatus[storage_id] = STORAGE_NOINIT;
	}

	// GPIO init
	sd_card_low_level_init(0);

	// Initialize the MSD Storage
	#if 0
	storage_proc_init_msd();

	// Configure SD Interrupt mode
	sd_card_set_exti_irq(0);
	#endif
}

void Storage_DeInit(void)
{
	uint8_t storage_id = 0;

	// Try Unmount All storage
	for(storage_id = 0; storage_id < sizeof(StorageStatus); storage_id++)
	{
		StorageTryUnMount(storage_id);
	}

	// Terminate Storage background task
	if(StorageThreadId)
	{
		osThreadTerminate (StorageThreadId);
		StorageThreadId = 0;
	}

	// Delete Storage Message Queue
	//if(StorageEvent)
	//{
	//	osMessageDelete (StorageEvent);
	//	StorageEvent = 0;
	//}

	#if defined(USE_SDCARD)
	// DeInit MSD Storage
	StorageDeInitMSD();
	#endif
}

#endif

// -------------------------------------------------------------------------
// -------------  				Access calls			--------------------
// -------------------------------------------------------------------------

uint8_t Storage_GetStatus(uint8_t unit)
{
	#ifdef CONTEXT_SD
	uint8_t status = STORAGE_NOINIT;

	if(StorageID[unit])
	{
		osSemaphoreWait(StorageSemaphore[unit], osWaitForever);
		status = (StorageStatus[unit] == STORAGE_MOUNTED);
		osSemaphoreRelease(StorageSemaphore[unit]);
	}

	return status;
	#else
	return 0;
	#endif
}

uint32_t Storage_GetCapacity (uint8_t unit)
{
	#ifdef CONTEXT_SD
	uint32_t   tot_sect = 0;
	FATFS 	*fs;

	if(StorageID[unit])
	{
		osSemaphoreWait(StorageSemaphore[unit], osWaitForever);
		fs = &StorageDISK_FatFs[unit];
		tot_sect = (fs->n_fatent - 2) * fs->csize;
		osSemaphoreRelease(StorageSemaphore[unit]);
	}

	return (tot_sect);
	#else
	return 0;
	#endif
}

uint32_t Storage_GetLabel(char *label)
{
	#ifdef CONTEXT_SD
	FRESULT res;
	FATFS 	*fs;

	if(label == NULL)
		return FR_INT_ERR;

	//if(StorageID[unit])
	//{
		osSemaphoreWait(StorageSemaphore[0], osWaitForever);
		fs = &StorageDISK_FatFs[0];
		res = f_getlabel(StorageDISK_Drive, label, NULL);
		osSemaphoreRelease(StorageSemaphore[0]);
	//}

	return FR_OK;
	#else
	return 0;
	#endif
}

uint32_t Storage_GetFree (uint8_t unit)
{
	#ifdef CONTEXT_SD
	uint32_t	fre_clust = 0, ret = 0;
	FATFS 		*fs;
	FRESULT 	res = FR_INT_ERR;

	if(StorageID[unit])
	{
		osSemaphoreWait(StorageSemaphore[unit], osWaitForever);
		fs = &StorageDISK_FatFs[unit];
		res = f_getfree(StorageDISK_Drive, (DWORD *)&fre_clust, &fs);
		if(res == FR_OK)
			ret = (fre_clust * fs->csize);
		else
			ret = 0;
		osSemaphoreRelease(StorageSemaphore[unit]);
	}

	return ret;
	#else
	return 0;
	#endif
}

const char *Storage_GetDrive(uint8_t unit)
{
	#ifdef CONTEXT_SD
	if(StorageStatus[unit] == STORAGE_MOUNTED)
		return StorageDISK_Drive;
	else
		return '\0';
	#else
	return 0;
	#endif
}


