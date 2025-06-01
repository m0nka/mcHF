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
#ifndef __UI_CONTROLS_CLOCK_PANEL
#define __UI_CONTROLS_CLOCK_PANEL

#define CLOCK_PANEL_COL		APPLE_MAC_GREY
#define CLOCK_COLOR			GUI_BLACK
#define CLOCK_FONT			GUI_Font32B_ASCII

#define CLOCK_UNITS_SHIFT 	10
#define CLOCK_HOURS_SHIFT 	0
#define CLOCK_SECND_SHIFT 	62
#define CLOCK_DATES_SHIFT 	160
#define CLOCK_LOCKS_SHIFT 	280

// Exports
void ui_controls_clock_panel_init(void);
void ui_controls_clock_panel_quit(void);
void ui_controls_clock_panel_refresh(void);
void ui_controls_clock_panel_restore(void);

#endif
