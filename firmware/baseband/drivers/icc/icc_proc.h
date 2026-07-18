/************************************************************************************
**                                                                                 **
**                                 mcHF QRP Transceiver                            **
**                         Krassi Atanassov - M0NKA, 2013-2026                     **
**                                                                                 **
**---------------------------------------------------------------------------------**
**                                                                                 **
**  File name:		icc_proc.h                                                     **
**  Description:	M4 core side of the inter core comms driver (OpenAMP/HSEM),   **
**					protocol identical to the CLINT baseband project               **
**  Licence:		https://github.com/m0nka/mcHF/blob/main/LICENSE                **
************************************************************************************/
#ifndef __ICC_PROC_H
#define __ICC_PROC_H

void icc_proc_notify_fft_ready(void);

void icc_proc_hw_init(void);
void icc_proc_task(void const *arg);

#endif
