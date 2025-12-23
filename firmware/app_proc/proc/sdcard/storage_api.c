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

#include "sd_card.h"
#include "sd_diskio.h"
#include "storage_proc.h"

#include "storage_api.h"

// sd process local vars
extern FATFS 			StorageDISK_FatFs[];
extern uint8_t			StorageID[];
extern osSemaphoreId	StorageSemaphore[];
extern STORAGE_Status_t	StorageStatus[];

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

uint32_t Storage_GetLabel(uint8_t unit, char *label)
{
	#ifdef CONTEXT_SD
	FRESULT res;

	if(label == NULL)
		return FR_INT_ERR;

	if(StorageID[unit])
	{
		osSemaphoreWait(StorageSemaphore[0], osWaitForever);

		res = f_getlabel(StorageDISK_Drive, label, NULL);

		osSemaphoreRelease(StorageSemaphore[0]);
	}

	return res;
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

uint32_t Storage_GetDrive(uint8_t unit, char *disk)
{
	FRESULT 	res = FR_INT_ERR;

	#ifdef CONTEXT_SD

	osSemaphoreWait(StorageSemaphore[unit], osWaitForever);

	if((StorageStatus[unit] == STORAGE_MOUNTED)&&(disk != NULL))
	{
		strcpy(disk, StorageDISK_Drive);
		res = 0;
	}
	else
		res = 1;

	osSemaphoreRelease(StorageSemaphore[unit]);

	return res;
	#else
	return 1;
	#endif
}


