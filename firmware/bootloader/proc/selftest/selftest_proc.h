#ifndef __SELFTEST_PROC_H
#define __SELFTEST_PROC_H

int 	sdram_test(void);

int 	test_sd_card(void);
void 	fs_cleanup(void);

ulong 	is_firmware_valid(void);

#endif
