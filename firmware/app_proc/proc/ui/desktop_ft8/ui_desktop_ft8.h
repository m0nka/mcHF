/************************************************************************************
**                                                                                 **
**                                 mcHF QRP Transceiver                            **
**                         Krassi Atanassov - M0NKA, 2013-2026                     **
**                                                                                 **
**---------------------------------------------------------------------------------**
**                                                                                 **
**  File name:		ui_desktop_ft8.h                                               **
**  Description:	FT8 desktop - widget ids and screen geometry                   **
**  Licence:		https://github.com/m0nka/mcHF/blob/main/LICENSE                **
************************************************************************************/
#ifndef __UI_DESKTOP_FT8_H
#define __UI_DESKTOP_FT8_H

// Action row buttons. There are no character keys on this screen - FT8
// messages are built by the QSO sequencer from the decode you tap, not
// typed, so the keyboard MarsChat carries is not needed here
#define ID_FT8_WINDOW			(GUI_ID_USER + 0x00)
#define ID_FT8_BTN_ENABLE		(GUI_ID_USER + 0x01)	// arm / disarm the transmitter
#define ID_FT8_BTN_CQ			(GUI_ID_USER + 0x02)	// call CQ in the next slot
#define ID_FT8_BTN_ANSWER		(GUI_ID_USER + 0x03)	// answer the selected decode
#define ID_FT8_BTN_HALT			(GUI_ID_USER + 0x04)	// abort the current transmission
#define ID_FT8_BTN_LOG			(GUI_ID_USER + 0x05)	// log the current QSO
#define ID_FT8_BTN_BAND			(GUI_ID_USER + 0x06)	// step the FT8 band

// ---------------------------------------------------------------------
// Screen geometry (atlas theme). Top level screen, owns the whole
// display for as long as MODE_DESKTOP_FT8 is the current UI state.
//
// The panel is 800x480. The previous version of this file laid the
// dialog out at 854 px wide - a leftover from the larger LCD on the
// earlier board revision - so the right hand 54 px, which included the
// end of the RX FREQUENCY list, were simply off the screen

#define FT8_UI_W				800
#define FT8_UI_H				480

#define FT8_TITLE_H				34					// title, dial frequency, clock

// Slot bar - phase, countdown, sync source, decode count, and the slot
// progress hairline underneath it
#define FT8_SLOT_X				10
#define FT8_SLOT_Y				38
#define FT8_SLOT_W				780
#define FT8_SLOT_H				38
#define FT8_SLOT_WIN_H			44					// bar + progress hairline below it
#define FT8_PROG_Y				41					// inside the slot window
#define FT8_PROG_H				3

// The two decode lists. Left is everything heard in the slot, right is
// traffic on our own TX frequency - the WSJT-X split, and the reason a
// crowded band stays readable
#define FT8_LIST_Y				90
#define FT8_LIST_H				290
#define FT8_LIST_W				385
#define FT8_LIST_L_X			10					// band activity
#define FT8_LIST_R_X			405					// rx frequency

#define FT8_LIST_HDR_H			24					// column header inside the panel
#define FT8_LIST_ROW_H			22
#define FT8_LIST_ROWS			11

// Column offsets within a list panel, relative to its left edge
#define FT8_COL_TIME			8
#define FT8_COL_SNR				58
#define FT8_COL_DT				98
#define FT8_COL_FREQ			140
#define FT8_COL_MSG				184

// Next transmission - what goes out in the next slot we own
#define FT8_TX_X				10
#define FT8_TX_Y				386
#define FT8_TX_W				780
#define FT8_TX_H				34

// Action row
#define FT8_ACT_X				10
#define FT8_ACT_Y				426
#define FT8_ACT_H				44
#define FT8_ACT_W				125					// one button
#define FT8_ACT_GAP				6
#define FT8_ACT_COL_X(c)		(FT8_ACT_X + (c) * (FT8_ACT_W + FT8_ACT_GAP))

// FT8 protocol timing, needed by the UI for the slot bar
#define FT8_SLOT_SECS			15					// slot period
#define FT8_TX_SECS				13					// signal ends ~12.64 s in

// Screen create / destroy, called from the UI mode switch
void	ui_desktop_ft8_create(void);
void	ui_desktop_ft8_destroy(void);

#endif
