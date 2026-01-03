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
#ifndef __SELFTEST_PROC_H
#define __SELFTEST_PROC_H

int 	sdram_test(void);

int 	test_sd_card(void);
void 	fs_cleanup(void);

ulong 	is_firmware_valid(void);

void selftest_proc();
void selftest_proc_init();

#endif
