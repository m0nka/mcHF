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
**  Licence:                                                                       **
************************************************************************************/
#ifndef __TRX_PROC_H
#define __TRX_PROC_H

#define DAC_OFFSET	823

void trx_proc_hw_init(void);
void trx_proc_power_clean_up(void);
void trx_proc_task(void const *arg);

#endif
