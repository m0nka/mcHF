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
#ifndef UI_CONTROLS_KEYER_H
#define UI_CONTROLS_KEYER_H

// Exports
uchar ui_controls_keyer_init(WM_HWIN hParent);
void ui_controls_keyer_quit(void);

void ui_controls_keyer_touch(void);
void ui_controls_keyer_refresh(void);

#endif
