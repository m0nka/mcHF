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
#ifndef __LORA_PROC_H
#define __LORA_PROC_H

void lora_proc_busy_irq(void);
void lora_proc_dio1_irq(void);

void lora_proc_init(void);
void lora_proc_power_cleanup(void);
void lora_proc_task(void const *arg);

#endif
