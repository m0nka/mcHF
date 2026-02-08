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
#ifndef __MAIN_H
#define __MAIN_H

#include "mchf_types.h"

//
// Define timeout to prevent HAL driver task stall
//
#define SDMMC_SWDATATIMEOUT		5000

#ifndef WIN32
#include "stm32h7xx_hal.h"

#include "stm32h747i_discovery_errno.h"
#include "otm8009a.h"
#include "cmsis_os.h"

#include "stm32h7xx_ll_bus.h"
#include "stm32h7xx_ll_rcc.h"
#include "stm32h7xx_ll_system.h"
#include "stm32h7xx_ll_utils.h"
#include "stm32h7xx_ll_gpio.h"
#include "stm32h7xx_ll_exti.h"
#include "stm32h7xx_ll_spi.h"
#include "stm32h7xx_ll_pwr.h"
#include "stm32h7xx_ll_adc.h"
#include "stm32h7xx_ll_dma.h"
#include "stm32h7xx_ll_bdma.h"
#include "stm32h7xx_ll_spi.h"
#include "stm32h7xx_ll_sdmmc.h"

#include "board.h"

/* FatFs includes component */
#include "ff_gen_drv.h"

#endif /* !WIN32 */

/* GUI includes components */
#include "GUI.h"
#include "DIALOG.h"
#include "LCDConf.h"
#ifndef WIN32
//#include "ST_GUI_Addons.h"
#endif /* !WIN32 */

#ifndef WIN32
/* Kernel includes components */
//#include "storage.h"
//#include "calibration.h"
//#include "gui_task.h"
#endif /* !WIN32 */

/* standard includes components */
//#include <stdio.h>
//#include <stdint.h>
//#include <stddef.h>
//#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "version.h"
#include "radio_init.h"
#include "rtc.h"

//#include "bsp.h"
#include "adc.h"
#include "att.h"
#include "WM.h"

//#include "ipc_proc.h"
#include "ui_proc.h"
#include "icc_proc.h"
#include "audio_proc.h"
#include "touch_proc.h"
#include "rotary_proc.h"
#include "bms_proc.h"
#include "vfo_proc.h"
#include "band_proc.h"
#include "trx_proc.h"
#include "fan_proc.h"
#include "keypad_proc.h"
#include "lora_proc.h"
#include "storage_proc.h"
#include "os_apploader.h"

#if 0
#define	TASK_PROC_IDLE				0
#define	TASK_PROC_WORK				1
#define	TASK_PROC_DONE				2

struct ESPMessage {

	uchar 	ucMessageID;
	uchar	ucProcStatus;
	uchar	ucDataReady;
	uchar	ucExecResult;

	uchar 	ucData[128];

} ESPMessage;
#endif

// ----------------------------------------------------------------------
#define MESH_ID_MC			0x01
#define MESH_ID_MT			0x02

typedef struct LORA_PACKET_RX
{
	uchar	avail;
	uchar	mesh_id;

	uchar	raw_rx_msg[256];
	ushort	raw_rx_size;

	char	sig_pwr[16];
	char	sig_snr[16];
	char	sig_rssi[16];

	char	msg_type[8];

	char	decoded_text[300];

} LORA_PACKET_RX;

// ----------------------------------------------------------------------

__attribute__((__common__)) struct PROC_STATE {

	// Process handles
	TaskHandle_t	hIccTask;
	TaskHandle_t 	hTouchTask;
	TaskHandle_t 	hUiTask;
	TaskHandle_t 	hVfoTask;
	TaskHandle_t 	hAudioTask;
	TaskHandle_t 	hBandTask;
	TaskHandle_t 	hTrxTask;
	TaskHandle_t 	hFanTask;
	TaskHandle_t 	hKbdTask;
	TaskHandle_t 	hLraTask;
	TaskHandle_t 	hSdcTask;
	TaskHandle_t 	hAppTask;

	// Task messaging
	xQueueHandle 	xBmsRxQueue;

	// UI Notification queue
	xQueueHandle 	xUiNotifRxQueue;

	// System timer
	ulong 			epoch;

} PROC_STATE;

/* Exported macros -----------------------------------------------------------*/
/* Exported functions ------------------------------------------------------- */
void 	NMI_Handler(void);
void 	HardFault_Handler(void);
void 	MemManage_Handler(void);
void 	BusFault_Handler(void);
void 	UsageFault_Handler(void);
void 	SVC_Handler(void);
void 	PendSV_Handler(void);
void 	SysTick_Handler(void);

void 	Error_Handler(int err);
//void BSP_ErrorHandler(void);

void printf_init(uchar is_shared);

// math_util.c
void ftoa(float f, char *buf, size_t bufsiz);

#endif
