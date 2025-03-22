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
#ifndef __VFO_PROC_H
#define __VFO_PROC_H

void vfo_proc_hw_init(void);
void vfo_proc_power_cleanup(void);
void vfo_proc_task(void const *arg);

#endif
