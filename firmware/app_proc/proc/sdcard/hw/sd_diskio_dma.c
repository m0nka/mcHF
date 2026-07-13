/************************************************************************************
**                                                                                 **
**                                 mcHF QRP Transceiver                            **
**                         Krassi Atanassov - M0NKA, 2013-2026                     **
**                                                                                 **
**---------------------------------------------------------------------------------**
**                                                                                 **
**  File name:                                                                     **
**  Description:                                                                   **
**  Last Modified:                                                                 **
**  Licence:			https://github.com/m0nka/mcHF/blob/main/LICENSE            **
************************************************************************************/
#include "mchf_pro_board.h"

#ifdef CONTEXT_SD

#include "ff_gen_drv.h"
#include "sd_diskio.h"

#include "sd_card.h"

#include <cmsis_os.h>

#include <stdio.h>
#include <string.h>

#include "sd_diskio_dma.h"

static volatile DSTATUS Stat = STA_NOINIT;
static osMessageQId SDQueueID;

#ifdef SD_USE_DMA
__attribute__((section(".axi_mem"))) __attribute__ ((aligned (32))) static uint8_t buffer[BLOCKSIZE];
#else
static uint8_t buffer[BLOCKSIZE];
#endif

static DSTATUS SD_CheckStatus(BYTE lun)
{
	uint32_t timer = osKernelSysTick() + SD_TIMEOUT;

	Stat = STA_NOINIT;

	// After any write the card sits in its internal programming state for
	// a short while - that is 'busy', not 'not initialized'. FatFS polls
	// disk_status() inside validate() on every file operation, so treating
	// a momentary busy as NOINIT invalidates open files mid-stream: the
	// first sustained streaming writer (wspr capture) saw f_write fail
	// with FR_INVALID_OBJECT. Wait the busy out, like SD_write does
	while(osKernelSysTick() < timer)
	{
		if(sd_card_get_card_state() == SD_TRANSFER_OK)
		{
			Stat &= ~STA_NOINIT;
			break;
		}
	}

	return Stat;
}

DSTATUS SD_initialize(BYTE lun)
{
	Stat = STA_NOINIT;

	/*
	 * check that the kernel has been started before continuing
	 * as the osMessage API will fail otherwise
	 */
	if(osKernelRunning())
	{
		#if !defined(DISABLE_SD_INIT)
		if(BSP_SD_Init(0) == BSP_ERROR_NONE)
		{
			Stat = SD_CheckStatus(lun);
		}
		#else
		Stat = SD_CheckStatus(lun);
		#endif

		/*
		 * if the SD is correctly initialized, create the operation queue
		 */
		if(Stat != STA_NOINIT)
		{
			osMessageQDef(SD_Queue, QUEUE_SIZE, uint16_t);
			SDQueueID = osMessageCreate (osMessageQ(SD_Queue), NULL);
		}
	}

	return Stat;
}

DSTATUS SD_status(BYTE lun)
{
	return SD_CheckStatus(lun);
}

/**
  * @brief  Reads Sector(s)
  * @param  lun : not used
  * @param  *buff: Data buffer to store read data
  * @param  sector: Sector address (LBA)
  * @param  count: Number of sectors to read (1..128)
  * @retval DRESULT: Operation result
  */
DRESULT SD_read(BYTE lun, BYTE *buff, DWORD sector, UINT count)
{
    DRESULT res = RES_ERROR;

	#ifdef SD_USE_DMA
    osEvent event;
	#endif

	#if (ENABLE_SD_DMA_CACHE_MAINTENANCE == 1)
    uint32_t alignedAddr;
	#endif

    uint32_t timer = osKernelSysTick() + SD_TIMEOUT;

    // first ensure the SDCard is ready for a new operation
    while((sd_card_get_card_state() == SD_TRANSFER_BUSY))
    {
    	if(timer < osKernelSysTick())
    	{
    		printf("read busy err  \r\n");
    		return RES_NOTRDY;
    	}
    }

    // Is address aligned/correct RAM
    if((!((uint32_t)buff & 0x3))&&(((ulong)buff >> 24) == (D1_AXISRAM_BASE >> 24)))
    {
    	//printf("SD_readA %d,%d(%08x) \r\n", (int)sector, (int)count, (int)buff);

		#ifndef SD_USE_DMA
    	uint8_t ret = sd_card_read_blocks((uint32_t*)buff, (uint32_t)(sector), count);
    	if(ret != BSP_ERROR_NONE)
        {
        	printf("read errA  \r\n");
        	res = RES_ERROR;
        }
    	else
    		res = RES_OK;
		#else
    	// Fast path: the provided destination buffer is correctly aligned.
    	// One retry - a single transient error would otherwise abort a
    	// multi-megabyte streaming read (wspr capture decode)
    	int attempt;

    	for(attempt = 0; (attempt < 2) && (res != RES_OK); attempt++)
    	{
    		if(attempt)
    		{
    			// Let the card settle and become ready again
    			osDelay(10);
    			timer = osKernelSysTick() + SD_TIMEOUT;
    			while(sd_card_get_card_state() == SD_TRANSFER_BUSY)
    			{
    				if(timer < osKernelSysTick())
    					break;
    			}
    			printf("readA retry  \r\n");
    		}

    		uint8_t ret = sd_card_read_blocks_dma((uint32_t*)buff, (uint32_t)(sector), count);
    		if (ret == BSP_ERROR_NONE)
    		{
    			// wait for a message from the queue or a timeout
    			event = osMessageGet(SDQueueID, SD_TIMEOUT);
    			if (event.status == osEventMessage)
    			{
    				if (event.value.v == READ_CPLT_MSG)
    				{
    					res = RES_OK;
						#if (ENABLE_SD_DMA_CACHE_MAINTENANCE == 1)
    					// Invalidate the cache after the DMA read, to see actual data
    					alignedAddr = (uint32_t)buff & ~0x1F;
    					SCB_InvalidateDCache_by_Addr((uint32_t*)alignedAddr, count*BLOCKSIZE + ((uint32_t)buff - alignedAddr));
						#endif
    				}
    				else if (event.value.v == RW_ERROR_MSG)
    				{
    					printf("readA dma err  \r\n");
    					res = RES_ERROR;
    				}
    			}
    			else
    			{
    				printf("readA dma timeout  \r\n");
    				res = RES_ERROR;
    			}
    		}
    		else
    			printf("readA start err  \r\n");
    	}
		#endif
    }
    else
    {
        // Slow path: fetch each sector a part and memcpy to destination buffer
        uint8_t ret = BSP_ERROR_NONE;
        int i;

        //printf("SD_readB %d,%d(%08x) \r\n", (int)sector, (int)count, (int)buff);

        for (i = 0; i < count; i++)
        {
			#ifndef SD_USE_DMA
        	uint8_t ret = sd_card_read_blocks((uint32_t*)buffer, (uint32_t)(sector++), 1);
        	if(ret != BSP_ERROR_NONE)
        	{
        		printf("read errB  \r\n");
        		break;
        	}
        	else
        	{
                memcpy(buff, buffer, BLOCKSIZE);
                buff += BLOCKSIZE;
        	}
			#else
            ret = sd_card_read_blocks_dma((uint32_t*)buffer, (uint32_t)sector++, 1);
            if (ret == BSP_ERROR_NONE)
            {
                /* wait for a message from the queue or a timeout */
                event = osMessageGet(SDQueueID, SD_TIMEOUT);
                if (event.status == osEventMessage)
                {
                    if (event.value.v == READ_CPLT_MSG)
                    {
						#if (ENABLE_SD_DMA_CACHE_MAINTENANCE == 1)
                        SCB_InvalidateDCache_by_Addr((uint32_t*)buffer, BLOCKSIZE);
						#endif

                        memcpy(buff, buffer, BLOCKSIZE);
                        buff += BLOCKSIZE;
                    }
                    else if (event.value.v == RW_ERROR_MSG)
                    {
                    	printf("readB dma err(%d)  \r\n", i);
                    	res = RES_ERROR;
                    }
                }
                else
                {
                	printf("readB dma timeout  \r\n");
                	res = RES_ERROR;
                }
            }
            else
                break;
			#endif
        }

        if ((i == count) && (ret == BSP_ERROR_NONE))
            res = RES_OK;
    }

    return res;
}

/**
  * @brief  Writes Sector(s)
  * @param  lun : not used
  * @param  *buff: Data to be written
  * @param  sector: Sector address (LBA)
  * @param  count: Number of sectors to write (1..128)
  * @retval DRESULT: Operation result
  */
#if _USE_WRITE == 1
DRESULT SD_write(BYTE lun, const BYTE *buff, DWORD sector, UINT count)
{
	osEvent event;
	DRESULT res = RES_ERROR;

	//printf("SD_write  \r\n");

	uint32_t timer = osKernelSysTick() + SD_TIMEOUT;

	// first ensure the SDCard is ready for a new operation
	while((sd_card_get_card_state() == SD_TRANSFER_BUSY))
	{
		if(timer < osKernelSysTick())
		{
			printf("write busy err  \r\n");
			return RES_NOTRDY;
		}
	}

	#if (ENABLE_SD_DMA_CACHE_MAINTENANCE == 1)
	uint32_t alignedAddr;
	#endif

	if(!((uint32_t)buff & 0x3))
	{
		#if (ENABLE_SD_DMA_CACHE_MAINTENANCE == 1)
		/*
		 * Invalidate the chache before writting into the buffer.
		 * This is not needed if the memory region is configured as W/T.
		 */
		alignedAddr = (uint32_t)buff & ~0x1F;
		SCB_InvalidateDCache_by_Addr((uint32_t*)alignedAddr, count*BLOCKSIZE + ((uint32_t)buff - alignedAddr));
		#endif

		if(sd_card_write_blocks_dma	((uint32_t*)buff,
									(uint32_t) (sector),
									count) == BSP_ERROR_NONE)
		{
			// Get the message from the queue
			event = osMessageGet(SDQueueID, SD_TIMEOUT);

			if(event.status == osEventMessage)
			{
				if(event.value.v == WRITE_CPLT_MSG)
				{
					res = RES_OK;
				}
				else
					printf("writeA dma err  \r\n");
			}
			else
				printf("writeA dma timeout  \r\n");
		}
		else
			printf("writeA start err  \r\n");
	}
	else
	{
		// Slow path, fetch each sector a part and memcpy to destination buffer
		int i;

		#if(ENABLE_SD_DMA_CACHE_MAINTENANCE == 1)
		// invalidate the scratch buffer before the next write to get the actual data instead of the cached one
		SCB_InvalidateDCache_by_Addr((uint32_t*)buffer, BLOCKSIZE);
		#endif

		for (i = 0; i < count; i++)
		{
			uint8_t ret = sd_card_write_blocks_dma((uint32_t*)buffer, (uint32_t)sector++, 1);
			if (ret == BSP_ERROR_NONE) {
				// wait for a message from the queue or a timeout
				event = osMessageGet(SDQueueID, SD_TIMEOUT);

				if (event.status == osEventMessage) {
					if (event.value.v == WRITE_CPLT_MSG) {
						res = RES_OK;
						memcpy((void *)buff, (void *)buffer, BLOCKSIZE);
						buff += BLOCKSIZE;
					}
				}
			} else
				break;
		}
	}

	return res;
}
#endif /* _USE_WRITE == 1 */

/**
  * @brief  I/O control operation
  * @param  lun : not used
  * @param  cmd: Control code
  * @param  *buff: Buffer to send/receive control data
  * @retval DRESULT: Operation result
  */
#if _USE_IOCTL == 1
DRESULT SD_ioctl(BYTE lun, BYTE cmd, void *buff)
{
	DRESULT res = RES_ERROR;

	BSP_SD_CardInfo CardInfo;

	//printf("SD_ioctl  \r\n");

	if(Stat & STA_NOINIT) return RES_NOTRDY;

	switch (cmd)
	{
		// Make sure that no pending write process
		case CTRL_SYNC :
			res = RES_OK;
			break;

		// Get number of sectors on the disk (DWORD)
		case GET_SECTOR_COUNT :
			ad_card_get_card_info(&CardInfo);
			*(DWORD*)buff = CardInfo.LogBlockNbr;
			res = RES_OK;
			break;

		// Get R/W sector size (WORD)
		case GET_SECTOR_SIZE :
			ad_card_get_card_info(&CardInfo);
			*(WORD*)buff = CardInfo.LogBlockSize;
			res = RES_OK;
			break;

		// Get erase block size in unit of sector (DWORD)
		case GET_BLOCK_SIZE :
			ad_card_get_card_info(&CardInfo);
			*(DWORD*)buff = CardInfo.LogBlockSize / BLOCKSIZE;
			res = RES_OK;
			break;
		default:
			res = RES_PARERR;
	}

	return res;
}
#endif /* _USE_IOCTL == 1 */

#ifdef SD_USE_DMA
void BSP_SD_WriteCpltCallback(void)
{
  /*
   * No need to add an "osKernelRunning()" check here, as the SD_initialize()
   * is always called before any SD_Read()/SD_Write() call
   */
   osMessagePut(SDQueueID, WRITE_CPLT_MSG, osWaitForever);
}

void BSP_SD_ReadCpltCallback(void)
{
  /*
   * No need to add an "osKernelRunning()" check here, as the SD_initialize()
   * is always called before any SD_Read()/SD_Write() call
   */
	 //printf("read complete  \r\n");
   osMessagePut(SDQueueID, READ_CPLT_MSG, osWaitForever);
}

void BSP_SD_ErrorCallback(void)
{
  /* Non Blocking Call to BSP_ErrorHandler() */
  //BSP_ErrorHandler();
	//printf("SD_err  \r\n");

  /*
   * No need to add an "osKernelRunning()" check here, as the SD_initialize()
   * is always called before any SD_Read()/SD_Write() call
   */
   osMessagePut(SDQueueID, RW_ERROR_MSG, osWaitForever);
}

void BSP_SD_AbortCallback(void)
{
  /* Non Blocking Call to BSP_ErrorHandler() */
  //BSP_ErrorHandler();
	//printf("SD_abort  \r\n");

  /*
   * No need to add an "osKernelRunning()" check here, as the SD_initialize()
   * is always called before any SD_Read()/SD_Write() call
   */
   osMessagePut(SDQueueID, RW_ABORT_MSG, osWaitForever);
}
#endif

const Diskio_drvTypeDef  SD_Driver =
{
	SD_initialize,
	SD_status,
	SD_read,
  	#if _USE_WRITE == 1
	SD_write,
  	#endif

	#if _USE_IOCTL == 1
	SD_ioctl,
	#endif
};

#endif

