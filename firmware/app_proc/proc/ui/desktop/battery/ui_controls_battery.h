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
**  Licence:               GNU GPLv3                                               **
************************************************************************************/
#ifndef UI_CONTROLS_BATTERY_H
#define UI_CONTROLS_BATTERY_H

// Control type
//#define BATT_VERTICAL

#define BATT_COLOUR					GUI_DARKYELLOW

// Exports
void ui_controls_battery_init(void);
void ui_controls_battery_quit(void);

void ui_controls_battery_touch(void);
void ui_controls_battery_refresh(void);

#endif
