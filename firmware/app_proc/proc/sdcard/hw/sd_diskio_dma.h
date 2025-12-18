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
#ifndef __SD_DISKIO_DMA_H
#define __SD_DISKIO_DMA_H

#define QUEUE_SIZE         	(uint32_t) 10
#define READ_CPLT_MSG      	(uint32_t) 1
#define WRITE_CPLT_MSG     	(uint32_t) 2
#define RW_ERROR_MSG       	(uint32_t) 3
#define RW_ABORT_MSG       	(uint32_t) 4

//#define SD_TIMEOUT 		30 * 1000
#define SD_TIMEOUT 			5000

#define DISABLE_SD_INIT

void BSP_SD_AbortCallback(void);
void BSP_SD_ErrorCallback(void);
void BSP_SD_WriteCpltCallback(void);
void BSP_SD_ReadCpltCallback(void);
void BSP_SD_DetectCallback(uint32_t Status);

#endif
