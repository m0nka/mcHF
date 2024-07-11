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
#ifndef UI_CONTROLS_FILTER_H
#define UI_CONTROLS_FILTER_H

#define	FIL_BKG_COLOR			GUI_LIGHTBLUE

#define FIL_BTN_X				40
#define FIL_BTN_SHFT			66

// Exports
void ui_controls_filter_init(void);
void ui_controls_filter_quit(void);

void ui_controls_filter_touch(void);
void ui_controls_filter_refresh(void);

#endif
