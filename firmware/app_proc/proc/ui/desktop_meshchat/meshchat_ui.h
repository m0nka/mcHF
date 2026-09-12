/************************************************************************************
**                                                                                 **
**                                 mcHF QRP Transceiver                            **
**                         Krassi Atanassov - M0NKA, 2013-2026                     **
**                                                                                 **
**---------------------------------------------------------------------------------**
**                                                                                 **
**  File name:		meshchat_ui.h                                                  **
**  Description:	MeshCore chat dialog - widget ids and screen geometry          **
**  Licence:		https://github.com/m0nka/mcHF/blob/main/LICENSE                **
************************************************************************************/
//
// Stock emWin widgets and the default skin throughout - no hand drawing
// beyond the title strip and the compose bar. Where MarsChat paints its
// own Atlas theme, this screen is deliberately plain
//
#ifndef __MESHCHAT_UI_H
#define __MESHCHAT_UI_H

// Widget ids. Prefixed MX rather than reusing the plain ID_BUTTON_xxx
// names - ui_proc.c includes this header alongside the MarsChat one and
// the two screens do not agree on the numbering
#define ID_MX_WINDOW			(GUI_ID_USER + 0x00)

// Character keys occupy a contiguous block, so a press decodes straight
// to an index into the active keyboard page
#define ID_MX_CHAR_0			(GUI_ID_USER + 0x10)
#define ID_MX_SHIFT				(GUI_ID_USER + 0x3F)

#define ID_MX_LIST_LEFT			(GUI_ID_USER + 0x50)	// conversations, or heard nodes
#define ID_MX_LIST_RIGHT		(GUI_ID_USER + 0x51)	// messages, or contact detail

#define ID_MX_TYPE				(GUI_ID_USER + 0x60)
#define ID_MX_SEND				(GUI_ID_USER + 0x61)
#define ID_MX_SPACE				(GUI_ID_USER + 0x62)
#define ID_MX_BACKSPACE			(GUI_ID_USER + 0x63)
#define ID_MX_CLEAR				(GUI_ID_USER + 0x64)
#define ID_MX_CONTACTS			(GUI_ID_USER + 0x65)
#define ID_MX_ADVERT			(GUI_ID_USER + 0x66)
#define ID_MX_ADD				(GUI_ID_USER + 0x68)
#define ID_MX_FORGET			(GUI_ID_USER + 0x69)
#define ID_MX_ADDCHAN			(GUI_ID_USER + 0x6A)
// No EXIT button - F3 closes the screen, the same key that opens it

// ---------------------------------------------------------------------
// Screen geometry. Same overall shape as the MarsChat screen so the two
// chat apps feel related: title, two panes, a compose bar, then the
// action row with the keyboard sliding up underneath it

#define MX_UI_W					800
#define MX_UI_H					480

#define MX_TITLE_H				34

// Title strip runs: MESHCORE | node [hash] | radio settings | clock.
// The node name is clipped so a long one cannot push the rest off
#define MX_TITLE_NAME_X			150
#define MX_TITLE_NAME_MAX		12					// characters, not pixels
#define MX_TITLE_LORA_X			340

// The panes give up some height to the bigger keyboard and the larger
// text - this is a 4" panel at arm's length, legibility wins over how
// many lines fit
#define MX_LIST_Y				40
#define MX_LIST_H				206

#define MX_CONV_X				5					// left pane
#define MX_CONV_W				210

#define MX_MSG_X				222					// right pane
#define MX_MSG_W				573

#define MX_COMP_X				5					// draft being composed
#define MX_COMP_Y				250
#define MX_COMP_W				790
#define MX_COMP_H				32

// Roughly how many characters of the message font fit across the right
// pane - used to fold long messages over several listbox rows. Retune
// this whenever the message font changes, or long lines get clipped
// instead of wrapping (Font24_1 across 573 px is about 46)
#define MX_MSG_WRAP				46

// On screen keyboard - ten columns by three rows, laid out QWERTY. Ten
// columns is what the top row needs (q..p is ten keys); an alphabetical
// grid needs only nine but is far harder to scan, because the eye has
// nowhere learned to look
//
// That gives 30 slots for 26 characters. The four left over carry the
// page key, backspace and a double width space, so the action row does
// not have to:
//
//    q w e r t y u i o p
//    a s d f g h j k l DEL
//  abc z x c v b n m [ SPACE ]
//
#define MX_KEY_COLS				10
#define MX_KEY_ROWS				3
#define MX_KEY_SLOTS			(MX_KEY_COLS * MX_KEY_ROWS)		// 30
#define MX_KEY_CHARS			26

// Slots that are not characters
#define MX_SLOT_DEL				19
#define MX_SLOT_SHIFT			20
#define MX_SLOT_SPACE			28					// spans 28 and 29

#define MX_KEY_X				6
#define MX_KEY_Y1				286					// absolute y when the keyboard is open
#define MX_KEY_W				76
#define MX_KEY_H				46
#define MX_KEY_GAP				3
#define MX_KEY_VGAP				4

#define MX_KEY_COL_X(c)			(MX_KEY_X + (c) * (MX_KEY_W + MX_KEY_GAP))
#define MX_KEY_ROW_Y(r)			((r) * (MX_KEY_H + MX_KEY_VGAP))

#define MX_SLOT_X(s)			MX_KEY_COL_X((s) % MX_KEY_COLS)
#define MX_SLOT_Y(s)			MX_KEY_ROW_Y((s) / MX_KEY_COLS)

// Keyboard container - holds the key grid and slides as one window
#define MX_KB_Y					MX_KEY_Y1
#define MX_KB_W					MX_UI_W
#define MX_KB_H					(MX_KEY_ROWS * (MX_KEY_H + MX_KEY_VGAP))	// 150

// Action row sits below the keyboard and never moves
#define MX_ACT_Y				(MX_KEY_Y1 + MX_KB_H)			// 436
#define MX_ACT_H				40

// Status readout, drawn in the space the collapsed keyboard leaves
#define MX_STAT_X				6
#define MX_STAT_Y				MX_KB_Y
#define MX_STAT_W				788
#define MX_STAT_H				(MX_KB_H - 4)

#define MX_ANIM_TIME			300					// ms

// Compose buffer
#define MX_COMPOSE_MAX			110

// Log emWin free memory and the listbox row counts to the debug UART
// every few seconds while this screen is up. On until the fault inside
// WM__Paint is understood - the trail before a crash says whether the
// allocator was being drained by the rebuilds
#define MX_DEBUG_GUI_MEM

// Screen create / destroy, called from the UI mode switch
void	meshchat_ui_create(void);
void	meshchat_ui_destroy(void);

#endif
