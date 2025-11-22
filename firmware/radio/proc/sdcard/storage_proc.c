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
	if(!StorageEvent)
		return;

	// Notify storage thread
	if(!state)
		osMessagePut(StorageEvent, MSDDISK_CONNECTION_EVENT, 	0);
	else
		osMessagePut(StorageEvent, MSDDISK_DISCONNECTION_EVENT, 0);
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
	//printf("StorageTryUnMount...  \r\n");

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

	//printf("StorageTryUnMount...ok  \r\n");
	return StorageStatus[unit];
}

static void StorageThread(void const * argument)
{
	osEvent event;

	printf("storage proc  \r\n");

	for(;;)
	{
		event = osMessageGet(StorageEvent, osWaitForever);
		if(event.status == osEventMessage)
		{
			switch(event.value.v)
			{
        		case MSDDISK_CONNECTION_EVENT:
        		{
        			// Lock IRQ
        			HAL_NVIC_DisableIRQ  (EXTI0_IRQn);

        			if(BSP_SD_IsDetected() == SD_PRESENT)
        			{
        				printf("card inserted  \r\n");

						#if 0
        				printf("--------  \r\n");
        				printf("power on  \r\n");
        				printf("--------  \r\n");
						#endif

        				// Power on
						#ifndef SD_PWR_SWAP_POLARITY
        				HAL_GPIO_WritePin(SD_PWR_CNTR_PORT, SD_PWR_CNTR, GPIO_PIN_RESET);
						#else
        				HAL_GPIO_WritePin(SD_PWR_CNTR_PORT, SD_PWR_CNTR, GPIO_PIN_SET);
						#endif

        				// Enable SD Interrupt mode
        				if(sd_card_init(0) == BSP_ERROR_NONE)
        				{
        					if(sd_card_set_exti_irq(0) == BSP_ERROR_NONE)
        					{
        						StorageTryMount(MSD_DISK_UNIT);
        					}
        				}
        			}

        			// Unlock IRQ
        			HAL_NVIC_EnableIRQ  (EXTI0_IRQn);

        			break;
        		}

        		case MSDDISK_DISCONNECTION_EVENT:
        		{
        			// Lock IRQ
        			HAL_NVIC_DisableIRQ  (EXTI0_IRQn);

        			if(BSP_SD_IsDetected() == SD_NOT_PRESENT)
        			{
        				printf("card removed  \r\n");

        				StorageTryUnMount(MSD_DISK_UNIT);

						#if 0
        				printf("--------  \r\n");
        				printf("power off  \r\n");
        				printf("--------  \r\n");
						#endif

        				// Power off
						#ifdef SD_PWR_SWAP_POLARITY
        				HAL_GPIO_WritePin(SD_PWR_CNTR_PORT, SD_PWR_CNTR, GPIO_PIN_RESET);
						#else
        				HAL_GPIO_WritePin(SD_PWR_CNTR_PORT, SD_PWR_CNTR, GPIO_PIN_SET);
						#endif

        				// Do we need release of HW ? Maybe on task cleanup ?
        				//BSP_SD_DeInit(0);
        			}

        			// Unlock IRQ
        			HAL_NVIC_EnableIRQ  (EXTI0_IRQn);

        			break;
        		}

        		default:
        			break;
			}
		}
	}
}

static uint8_t storage_proc_init_msd(void)
{
	uint8_t sd_status = BSP_ERROR_NONE;

	if(StorageID[MSD_DISK_UNIT] != 0)
		return StorageID[MSD_DISK_UNIT];

	//printf("storage_proc_init_msd..  \r\n");

	// Configure SD Interrupt mode
	sd_card_set_exti_irq(0);

	sd_status = sd_card_init(0);
	if(sd_status != BSP_ERROR_NONE)
	{
		printf("card init failed(%d)  \r\n", sd_status);
		return 0;
	}

	// Create Storage Semaphore
	osSemaphoreDef(STORAGE_MSD_Semaphore);
	StorageSemaphore[MSD_DISK_UNIT] = osSemaphoreCreate (osSemaphore(STORAGE_MSD_Semaphore), 1);

	// Mark the storage as initialised
	StorageID[MSD_DISK_UNIT] = 1;
	Storage_Driver[MSD_DISK_UNIT] = &SD_Driver;

	// Try mount the storage
	StorageTryMount(MSD_DISK_UNIT);

	//printf("storage_proc_init_msd..ok  \r\n");

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
	storage_proc_init_msd();

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

#endif
