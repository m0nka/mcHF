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
#ifndef UI_CONTROLS_BATTERY_H
#define UI_CONTROLS_BATTERY_H

// Control type
//#define BATT_VERTICAL

#define BATT_COLOUR					GUI_ORANGE

// Exports
void ui_controls_battery_init(void);
void ui_controls_battery_quit(void);

void ui_controls_battery_touch(void);
void ui_controls_battery_refresh(void);

#endif
