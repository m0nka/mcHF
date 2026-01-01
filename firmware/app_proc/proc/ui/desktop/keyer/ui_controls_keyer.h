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
#ifndef UI_CONTROLS_KEYER_H
#define UI_CONTROLS_KEYER_H

// Exports
uchar ui_controls_keyer_init(WM_HWIN hParent);
void ui_controls_keyer_quit(void);

void ui_controls_keyer_touch(void);
void ui_controls_keyer_refresh(void);

#endif
