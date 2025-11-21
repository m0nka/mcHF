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

#ifdef CONTEXT_SD

#include "sd_card.h"
#include "sd_diskio.h"

#include "storage_proc.h"

__attribute__((section(".sdio_heap"))) __attribute__ ((aligned (32)))

static FATFS              StorageDISK_FatFs[NUM_DISK_UNITS];	// File system object for MSD disk logical drive
static char               StorageDISK_Drive[NUM_DISK_UNITS][4];	// Storage Host logical drive number
static osSemaphoreId      StorageSemaphore[NUM_DISK_UNITS];
static Diskio_drvTypeDef  const * Storage_Driver[NUM_DISK_UNITS];

static  uint8_t           StorageID[NUM_DISK_UNITS];
static STORAGE_Status_t   StorageStatus[NUM_DISK_UNITS];

osMessageQId              StorageEvent    = {0};
osThreadId                StorageThreadId = {0};

static STORAGE_Status_t StorageTryMount( const uint8_t unit )
{
	printf("StorageTryMount  \r\n");

	osSemaphoreWait(StorageSemaphore[unit], osWaitForever);

	if(unit == MSD_DISK_UNIT)
	{
		// We need to check for SD Card before mounting the volume
		if(!BSP_SD_IsDetected(0))
		{
			StorageStatus[unit] = STORAGE_UNMOUNTED;
			goto unlock_exit;
		}
	}

	if(StorageStatus[unit] != STORAGE_MOUNTED)
	{
		// Link the disk I/O driver
		if(FATFS_LinkDriver(Storage_Driver[unit], StorageDISK_Drive[unit]) != 0)
			goto unlock_exit;

		if(f_mount(&StorageDISK_FatFs[unit], StorageDISK_Drive[unit], 0))
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
	printf("StorageTryUnMount  \r\n");

	if(StorageID[unit] == 0)
		return StorageStatus[unit];

	osSemaphoreWait(StorageSemaphore[unit], osWaitForever);

	if(StorageStatus[unit] != STORAGE_MOUNTED)
		goto unlock_exit;

	if(f_mount(0, StorageDISK_Drive[unit], 0))
		goto unlock_exit;

	if(FATFS_UnLinkDriver(StorageDISK_Drive[unit]))
		goto unlock_exit;

	memset(StorageDISK_Drive[unit], 0, sizeof(StorageDISK_Drive[unit]));

	// Reset storage status
	StorageStatus[unit] = STORAGE_UNMOUNTED;

unlock_exit:
	osSemaphoreRelease(StorageSemaphore[unit]);

	return StorageStatus[unit];
}

static void StorageThread(void const * argument)
{
	osEvent event;

	printf("storage proc  \r\n");

	// Initialize the MSD Storage
	#if 0
	StorageInitMSD();
	#endif

	for(;;)
	{
		event = osMessageGet(StorageEvent, osWaitForever);
		if(event.status == osEventMessage)
		{
			switch(event.value.v)
			{
        		case MSDDISK_CONNECTION_EVENT:
        		{
        			printf("card inserted  \r\n");

					#ifdef STORAGE_BSP_INIT
        			// Enable SD Interrupt mode
        			if(BSP_SD_Init(0) == BSP_ERROR_NONE)
        			{
        				if(BSP_SD_DetectITConfig(0) == BSP_ERROR_NONE)
        					StorageTryMount(MSD_DISK_UNIT);
        			}
					#else
        			StorageTryMount(MSD_DISK_UNIT);
					#endif // STORAGE_BSP_INIT

        			break;
        		}

        		case MSDDISK_DISCONNECTION_EVENT:
        		{
        			printf("card removed  \r\n");

        			StorageTryUnMount(MSD_DISK_UNIT);

					#if 0
        			printf("power off  \r\n");

        			// Power off
        			#ifdef SD_PWR_SWAP_POLARITY
        			HAL_GPIO_WritePin(SD_PWR_CNTR_PORT, SD_PWR_CNTR, GPIO_PIN_RESET);
        			#else
        			HAL_GPIO_WritePin(SD_PWR_CNTR_PORT, SD_PWR_CNTR, GPIO_PIN_SET);
        			#endif
					#endif

					#ifdef STORAGE_BSP_INIT
//!        			BSP_SD_DeInit(0);
					#endif // STORAGE_BSP_INIT

        			break;
        		}

        		default:
        			break;
			}
		}
	}
}

static uint8_t StorageInitMSD(void)
{
	uint8_t sd_status = BSP_ERROR_NONE;

	if(StorageID[MSD_DISK_UNIT] != 0)
		return StorageID[MSD_DISK_UNIT];

	#ifdef STORAGE_BSP_INIT
	sd_status = BSP_SD_Init(0);
	if(sd_status != BSP_ERROR_NONE)
	{
		printf("card init failed(%d)  \r\n", sd_status);

		// Undo the SD CArd init
		BSP_SD_DeInit(0);
	}
	else
	{
		// Configure SD Interrupt mode
		sd_status = BSP_SD_DetectITConfig(0);
	}
	#endif // STORAGE_BSP_INIT

	if(sd_status == BSP_ERROR_NONE)
	{
		// Create Storage Semaphore
		osSemaphoreDef(STORAGE_MSD_Semaphore);
		StorageSemaphore[MSD_DISK_UNIT] = osSemaphoreCreate (osSemaphore(STORAGE_MSD_Semaphore), 1);

		// Mark the storage as initialised
		StorageID[MSD_DISK_UNIT] = 1;
		Storage_Driver[MSD_DISK_UNIT] = &SD_Driver;

		// Try mount the storage
		StorageTryMount(MSD_DISK_UNIT);

		printf("card init ok  \r\n");
	}

	return StorageID[MSD_DISK_UNIT];
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

	// Create Storage Message Queue
	osMessageQDef(osqueue, 10, uint16_t);
	StorageEvent = osMessageCreate (osMessageQ(osqueue), NULL);

	// Initialize the MSD Storage
	StorageInitMSD();

    // It's Okay then Create Storage background task and exit from here
    osThreadDef(STORAGE_Thread, StorageThread, STORAGE_THREAD_PRIORITY, 0, STORAGE_THREAD_STACK_SIZE);
    StorageThreadId = osThreadCreate (osThread(STORAGE_Thread), NULL);
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
	if(StorageEvent)
	{
		osMessageDelete (StorageEvent);
		StorageEvent = 0;
	}

	#if defined(USE_SDCARD)
	// DeInit MSD Storage
	StorageDeInitMSD();
	#endif
}

uint8_t Storage_GetStatus(uint8_t unit)
{
	uint8_t status = STORAGE_NOINIT;

	if(StorageID[unit])
	{
		osSemaphoreWait(StorageSemaphore[unit], osWaitForever);
		status = (StorageStatus[unit] == STORAGE_MOUNTED);
		osSemaphoreRelease(StorageSemaphore[unit]);
	}

	return status;
}

uint32_t Storage_GetCapacity (uint8_t unit)
{
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
}

uint32_t Storage_GetFree (uint8_t unit)
{
  uint32_t	fre_clust = 0, ret = 0;
  FATFS 	*fs;
  FRESULT 	res = FR_INT_ERR;

  if(StorageID[unit])
  {
	  osSemaphoreWait(StorageSemaphore[unit], osWaitForever);
    fs = &StorageDISK_FatFs[unit];
    res = f_getfree(StorageDISK_Drive[unit], (DWORD *)&fre_clust, &fs);

    //printf("f_getfree res: %d\r\n",res);

    if(res == FR_OK)
      ret = (fre_clust * fs->csize);
    else
      ret = 0;
    osSemaphoreRelease(StorageSemaphore[unit]);
  }

  return ret;
}

const char *Storage_GetDrive(uint8_t unit)
{
	if(StorageStatus[unit] == STORAGE_MOUNTED)
		return StorageDISK_Drive[unit];
	else
		return '\0';
}

void Storage_DetectSDCard(void)
{
	if(!StorageEvent)
		return;

	if((BSP_SD_IsDetected(0)))
	{
		// After sd disconnection, a SD Init is required
		if(Storage_GetStatus(MSD_DISK_UNIT) == STORAGE_NOINIT)
		{
			if(BSP_SD_Init(0) == BSP_ERROR_NONE)
			{
				if(BSP_SD_DetectITConfig(0) == BSP_ERROR_NONE)
					osMessagePut(StorageEvent, MSDDISK_CONNECTION_EVENT, 0);
			}
		}
		else if(Storage_GetStatus(MSD_DISK_UNIT) == STORAGE_UNMOUNTED)
		{
			osMessagePut(StorageEvent, MSDDISK_CONNECTION_EVENT, 0);
		}
	}
	else
	{
		osMessagePut(StorageEvent, MSDDISK_DISCONNECTION_EVENT, 0);
	}
}

#endif
