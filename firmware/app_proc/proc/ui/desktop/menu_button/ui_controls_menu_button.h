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
#ifndef __UI_CONTROLS_MENU_BUTTON_H
#define __UI_CONTROLS_MENU_BUTTON_H

// Exports
void ui_controls_menu_button_init(void);
int ui_controls_menu_button_is_touch(int x, int y);
void ui_controls_menu_button_quit(void);
void ui_controls_menu_button_refresh(void);

#endif
