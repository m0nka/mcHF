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
#ifndef __ROTARY_PROC_H
#define __ROTARY_PROC_H

void rotary_proc_hw_init(void);
void rotary_proc_power_cleanup(void);
void rotary_proc_task(void const *arg);

#endif
