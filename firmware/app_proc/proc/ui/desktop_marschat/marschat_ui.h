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

// Character keys (row major, ids CHAR_0 .. CHAR_0 + MC_KEY_CHARS-1), the
// shift key that pages them, then the action row. The char id block is
// 0x10..0x29 for 26 keys, so shift and the action ids start clear of it
#define ID_BUTTON_CHAR_0		(GUI_ID_USER + 0x10)
#define ID_BUTTON_SHIFT			(GUI_ID_USER + 0x3F)
#define ID_BUTTON_SPACE			(GUI_ID_USER + 0x40)
#define ID_BUTTON_BACKSPACE		(GUI_ID_USER + 0x41)
#define ID_BUTTON_CLEAR			(GUI_ID_USER + 0x42)
#define ID_BUTTON_SEND			(GUI_ID_USER + 0x43)
#define ID_BUTTON_CALLER		(GUI_ID_USER + 0x44)
#define ID_BUTTON_PEER			(GUI_ID_USER + 0x45)

// ---------------------------------------------------------------------
// Screen geometry (atlas theme). This is a top level screen, not a menu
// item: it owns the whole display for as long as MODE_DESKTOP_MARSCHAT
// is the current UI state, with nothing of the menu or the desktop left
// underneath it to repaint over our pixels

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

// On-screen keyboard - big keys for a 4" touch panel, so only half the
// charset shows at once and a shift key pages between them: page 0 is the
// 26 letters, page 1 is 0-9 plus the everyday punctuation. Nine columns by
// three rows = 27 slots; the last slot (bottom right) is the shift key, the
// other 26 are the character keys of the current page
#define MC_KEY_COLS				9
#define MC_KEY_ROWS				3
#define MC_KEY_SLOTS			(MC_KEY_COLS * MC_KEY_ROWS)		// 27
#define MC_KEY_CHARS			(MC_KEY_SLOTS - 1)				// 26, shift takes the last

#define MC_KEY_X				10
#define MC_KEY_Y1				322
#define MC_KEY_W				83
#define MC_KEY_H				36
#define MC_KEY_GAP				4					// between columns
#define MC_KEY_VGAP				3					// between rows

#define MC_KEY_COL_X(c)			(MC_KEY_X  + (c) * (MC_KEY_W + MC_KEY_GAP))
#define MC_KEY_ROW_Y(r)			(MC_KEY_Y1 + (r) * (MC_KEY_H + MC_KEY_VGAP))

#define MC_ACT_Y				MC_KEY_ROW_Y(MC_KEY_ROWS)	// action row below the keys
#define MC_ACT_H				36

// Screen create / destroy, called from the UI mode switch
void	marschat_ui_create(void);
void	marschat_ui_destroy(void);

#endif
