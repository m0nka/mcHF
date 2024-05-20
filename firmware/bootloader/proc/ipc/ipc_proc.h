/************************************************************************************
**                                                                                 **
**                             mcHF Pro QRP Transceiver                            **
**                         Krassi Atanassov - M0NKA, 2013-2021                     **
**                                                                                 **
**---------------------------------------------------------------------------------**
**                                                                                 **
**  File name:                                                                     **
**  Description:                                                                   **
**  Last Modified:                                                                 **
**  Licence:       The mcHF project is released for radio amateurs experimentation **
**               and non-commercial use only.Check 3rd party drivers for licensing **
************************************************************************************/
#ifndef __IPC_PROC_H
#define __IPC_PROC_H

#ifdef CONTEXT_IPC_PROC

void ipc_proc_init(void);
int ipc_proc_establish_link(char *fw_ver);
//void ipc_proc_task(void const *arg);

#endif

#endif
