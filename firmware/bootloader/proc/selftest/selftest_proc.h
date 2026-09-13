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

// Extended HW tests - Power/GPIO (return new state: 0=off, 1=on)
int     test_5v_toggle(void);
int     test_fan_toggle(void);
int     test_leds_toggle(void);
int     test_backlight_cycle(void);

// Extended HW tests - I2C bus (return 0=pass, nonzero=fail)
int     test_bq25730_ch224a(uchar *bq_ok, uchar *ch_ok);
int     test_codec_i2c(void);
int     test_si5351_i2c(void);
int     test_gt911_i2c(void);

// Extended HW tests - Peripherals
int     test_gps_check(void);
int     test_lora_check(void);
int     test_encoders(uchar *enc1, uchar *enc2);

#endif
