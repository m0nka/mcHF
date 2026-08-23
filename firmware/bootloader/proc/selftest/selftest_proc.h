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

int     sdram_test(void);

int     test_sd_card(void);
void    fs_cleanup(void);

ulong   is_firmware_valid(void);

void selftest_proc(void);
void selftest_proc_init(void);

// FW update/boot functions (implemented in mchf_pro_board.c)
uchar   update_radio(void);
uchar   update_baseband(void);
void    jump_to_fw(uint32_t addr);

#endif
