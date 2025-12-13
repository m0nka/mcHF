/************************************************************************************
**                                                                                 **
**                             mcHF Pro QRP Transceiver                            **
**                         Krassi Atanassov - M0NKA 2012-2020                      **
**                            mail: djchrismarc@gmail.com                          **
**                                 twitter: @bph_co                                **
**---------------------------------------------------------------------------------**
**                                                                                 **
**  File name:                                                                     **
**  Description:                                                                   **
**  Last Modified:                                                                 **
**  Licence:                                                                       **
**          The mcHF project is released for radio amateurs experimentation,       **
**          non-commercial use only. All source files under GPL-3.0, unless        **
**          third party drivers specifies otherwise. Thank you!                    **
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
