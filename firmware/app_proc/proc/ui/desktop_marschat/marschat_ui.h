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
#define ID_BUTTON_TYPE			(GUI_ID_USER + 0x46)	// opens the keyboard

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
// other 26 are the character keys of the current page.
//
// The keyboard lives in its own child window that slides up from below the
// screen when the user taps the compose bar and slides back down after
// SEND (or an explicit dismiss). The animation uses the same GUI_ANIM_*
// API as the Segger keypad sample but the visual is the Atlas-themed
// keyboard already on this screen, not the old skin. The action row
// (SPACE, DEL, CLEAR, SEND, CALLER, PEER) stays on the main dialog at a
// fixed y so the session buttons are always reachable; SPACE/DEL/CLEAR
// are hidden when the keyboard is off-screen
#define MC_KEY_COLS				9
#define MC_KEY_ROWS				3
#define MC_KEY_SLOTS			(MC_KEY_COLS * MC_KEY_ROWS)		// 27
#define MC_KEY_CHARS			(MC_KEY_SLOTS - 1)				// 26, shift takes the last

#define MC_KEY_X				10
#define MC_KEY_Y1				322					// absolute y when keyboard is open
#define MC_KEY_W				83
#define MC_KEY_H				36
#define MC_KEY_GAP				4					// between columns
#define MC_KEY_VGAP				3					// between rows

#define MC_KEY_COL_X(c)			(MC_KEY_X  + (c) * (MC_KEY_W + MC_KEY_GAP))
#define MC_KEY_ROW_Y(r)			(MC_KEY_Y1 + (r) * (MC_KEY_H + MC_KEY_VGAP))

#define MC_ACT_Y				MC_KEY_ROW_Y(MC_KEY_ROWS)	// action row below the keys
#define MC_ACT_H				36

// Keyboard container window - holds the 3 rows of character keys and the
// shift key. The action row (SPACE .. PEER) stays on the main dialog
#define MC_KB_Y					MC_KEY_Y1			// top when visible
#define MC_KB_W					MC_UI_W
#define MC_KB_H					(MC_KEY_ROWS * (MC_KEY_H + MC_KEY_VGAP))	// 117

// Slide animation (same accel/decel easing as c_keypad.c)
#define MC_ANIM_TIME			300					// ms

// Action row button positions: TYPE is always visible and toggles
// between "TYPE" (opens keyboard) and "HIDE" (closes it). When the
// keyboard is hidden CALLER/PEER are shown and SPACE/DEL/CLEAR hide;
// when the keyboard is showing SPACE/DEL/CLEAR appear and CALLER/PEER
// hide. The keyboard-shown positions are hardcoded in layout_action_row
#define MC_FULL_TYPE_X			10					// keyboard-hidden layout
#define MC_FULL_TYPE_W			170
#define MC_FULL_SEND_X			190
#define MC_FULL_SEND_W			190
#define MC_FULL_CALLER_X		390
#define MC_FULL_CALLER_W		190
#define MC_FULL_PEER_X			590
#define MC_FULL_PEER_W			200

// Status panel - drawn in the keyboard area when it is collapsed, gives
// a richer Atlas-style session readout than the compact telemetry panel.
// Same rectangle as the keyboard container (MC_KB_Y / MC_KB_H)
#define MC_STAT_X				10
#define MC_STAT_Y				MC_KB_Y
#define MC_STAT_W				780
#define MC_STAT_H				(MC_KB_H - 3)		// leave a 3 px gap to the action row

// Screen create / destroy, called from the UI mode switch
void	marschat_ui_create(void);
void	marschat_ui_destroy(void);

#endif
