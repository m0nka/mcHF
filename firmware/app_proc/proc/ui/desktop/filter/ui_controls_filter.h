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
#ifndef UI_CONTROLS_FILTER_H
#define UI_CONTROLS_FILTER_H

#define	FIL_BKG_COLOR			GUI_LIGHTBLUE

#define FIL_BTN_X				40
#define FIL_BTN_SHFT			70

// Exports
void ui_controls_filter_init(void);
void ui_controls_filter_quit(void);

void ui_controls_filter_touch(void);
void ui_controls_filter_refresh(void);

#endif
