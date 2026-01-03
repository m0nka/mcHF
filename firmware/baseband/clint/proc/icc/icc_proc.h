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
#ifndef __ICC_PROC_H
#define __ICC_PROC_H

void icc_proc_notify_fft_ready(void);

void icc_proc_notify_of_tx(void);
void icc_proc_notify_of_rx(void);

void icc_proc_async_broadcast(uchar *buff);
void icc_proc_hw_init(void);
void icc_proc_task(void const *arg);

#endif
