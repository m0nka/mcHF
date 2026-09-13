/************************************************************************************
**                                                                                 **
**                                 mcHF QRP Transceiver                            **
**                         Krassi Atanassov - M0NKA, 2013-2026                     **
**                                                                                 **
**---------------------------------------------------------------------------------**
**                                                                                 **
**  File name:       menu_proc.h                                                   **
**  Description:     console-like menu system for bootloader                       **
**  Last Modified:                                                                 **
**  Licence:         https://github.com/m0nka/mcHF/blob/main/LICENSE              **
************************************************************************************/
#ifndef __MENU_PROC_H
#define __MENU_PROC_H

// -----------------------------------------------------------------------
// Menu IDs
// -----------------------------------------------------------------------
#define MENU_MAIN               0
#define MENU_HW_TESTS           1
#define MENU_FW_UPDATE          2
#define MENU_BMS_TOOLS          3
#define MENU_HW_CORE            4   // sub: SDRAM, SD Card, BMS
#define MENU_HW_POWER           5   // sub: 5V toggle, Fan, LEDs, Backlight
#define MENU_HW_I2C             6   // sub: BQ25730+CH224A, Codec, SI5351, GT911
#define MENU_HW_PERIPH          7   // sub: GPS, LoRa, Encoders

// -----------------------------------------------------------------------
// Key button IDs mapped to menu actions
//
// These are the scan matrix column numbers (x=1..6) returned by the
// keypad scanner.  Adjust the defines below to match the physical
// button labels on the front panel.
// -----------------------------------------------------------------------
#define MENU_BTN_1              1       // F1
#define MENU_BTN_2              2       // F2
#define MENU_BTN_3              3       // F3
#define MENU_BTN_4              4       // F4
#define MENU_BTN_5              5       // F5

// -----------------------------------------------------------------------
// Max lines in the log/results area
// -----------------------------------------------------------------------
#define MENU_LOG_MAX_LINES      9

// -----------------------------------------------------------------------
// Publics
// -----------------------------------------------------------------------
void menu_proc_init(void);
void menu_proc(void);

// Called by keypad scanner when a button press is detected
void menu_proc_key_event(uchar x, uchar y, uchar hold);

#endif
