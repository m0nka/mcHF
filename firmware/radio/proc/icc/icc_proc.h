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
#ifndef __ICC_PROC_H
#define __ICC_PROC_H

#define API_UI_ALLOW_DEBUG

// -----------------------------------------------------------------------

void icc_proc_hw_init(void);
void icc_proc_power_cleanup(void);
void icc_proc_task(void const * argument);

#endif
