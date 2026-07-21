/************************************************************************************
**                                                                                 **
**                                 mcHF QRP Transceiver                            **
**                         Krassi Atanassov - M0NKA, 2013-2026                     **
**                                                                                 **
**---------------------------------------------------------------------------------**
**                                                                                 **
**  File name:		marschat_ui.h                                                  **
**  Description:	MarsChat chat dialog - widget ids and screen geometry          **
**  Licence:		https://github.com/m0nka/mcHF/blob/main/LICENSE                **
************************************************************************************/
#ifndef __MARSCHAT_UI_H
#define __MARSCHAT_UI_H

#define ID_WINDOW_0				(GUI_ID_USER + 0x00)

// 16 character buttons, A-P (row major), then the action row
#define ID_BUTTON_CHAR_0		(GUI_ID_USER + 0x10)
#define ID_BUTTON_SPACE			(GUI_ID_USER + 0x20)
#define ID_BUTTON_BACKSPACE		(GUI_ID_USER + 0x21)
#define ID_BUTTON_CLEAR			(GUI_ID_USER + 0x22)
#define ID_BUTTON_SEND			(GUI_ID_USER + 0x23)
#define ID_BUTTON_CALLER		(GUI_ID_USER + 0x24)
#define ID_BUTTON_PEER			(GUI_ID_USER + 0x25)

// ---------------------------------------------------------------------
// Screen geometry (atlas theme). Unlike the other menu items this screen
// takes the whole display: the menu's own window paints the background
// behind its items (ui_menu.c WM_PAINT clears to the theme colour), and
// a half covered screen shows that colour around our ground. Leaving the
// menu deletes the dialog, which resets the menu's repaint latch, so it
// draws itself back

#define MC_UI_W					800
#define MC_UI_H					480

#define MC_TITLE_H				34					// title, dial frequency, clock

#define MC_SLOT_X				10					// role / slot state bar
#define MC_SLOT_Y				38
#define MC_SLOT_W				780
#define MC_SLOT_H				38
#define MC_SLOT_WIN_H			44					// bar + burst hairline below it
#define MC_BURST_Y				41					// inside the slot window
#define MC_BURST_H				3

#define MC_HIST_X				10					// message list
#define MC_HIST_Y				86
#define MC_HIST_W				470
#define MC_HIST_H				200
#define MC_HIST_ROW_H			24
#define MC_HIST_ROWS			8

#define MC_TEL_X				490					// telemetry column
#define MC_TEL_Y				86
#define MC_TEL_W				300
#define MC_TEL_H				200

#define MC_COMP_X				10					// draft being composed
#define MC_COMP_Y				292
#define MC_COMP_W				780
#define MC_COMP_H				30

#define MC_KEY_X				10					// two rows of eight keys
#define MC_KEY_Y1				336
#define MC_KEY_Y2				378
#define MC_KEY_W				94
#define MC_KEY_H				38
#define MC_KEY_GAP				4

#define MC_ACT_Y				422					// action row
#define MC_ACT_H				38

#endif
