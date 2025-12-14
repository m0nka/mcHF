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


#endif
