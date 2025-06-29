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
#ifndef UI_MENU_LAYOUT_H
#define UI_MENU_LAYOUT_H

#include "GUI.h"

// ------------------------------------------------------
#define BKG_COLOR_1							GUI_WHITE
//
#define BKG_COLOR_2							GUI_RED
//
// ------------------------------------------------------
#define MENU_BAR_X_1						5
#define MENU_BAR_Y_1						3			//443
#define MENU_BAR_X_SIZE_1					260
#define MENU_BAR_Y_SIZE_1					30
#define MENU_BAR_COLOR_1					GUI_ORANGE
#define MENU_TXT_COLOR_1					GUI_WHITE
//
#define MENU_BAR_X_2						5
#define MENU_BAR_Y_2						3
#define MENU_BAR_X_SIZE_2					260
#define MENU_BAR_Y_SIZE_2					30
#define MENU_BAR_COLOR_2					GUI_BLUE
#define MENU_TXT_COLOR_2					GUI_WHITE
//
// ------------------------------------------------------
#define ICONVIEW_X_1						0
#define ICONVIEW_Y_1						35			//0
#define ICONVIEW_BKG_COL_UNSEL_1			GUI_WHITE
#define ICONVIEW_BKG_COL_SEL_1				GUI_WHITE
#define ICONVIEW_TXT_COL_UNSEL_1			GUI_BLACK
#define ICONVIEW_TXT_COL_SEL_1				GUI_RED
//
#define ICONVIEW_X_2						0
#define ICONVIEW_Y_2						35
#define ICONVIEW_BKG_COL_UNSEL_2			GUI_WHITE
#define ICONVIEW_BKG_COL_SEL_2				GUI_WHITE
#define ICONVIEW_TXT_COL_UNSEL_2			GUI_RED
#define ICONVIEW_TXT_COL_SEL_2				GUI_BLACK
//
// ------------------------------------------------------
__attribute__((__common__)) struct UIMenuLayout {

	// Theme name
	char	name[16];

	// Background
	ulong	bkg_color;

	// Footer
	ushort  mbar_x;
	ushort  mbar_y;
	ushort  mbar_sz_x;
	ushort  mbar_sz_y;
	ulong	mbar_bkg_clr;
	ulong	mbar_txt_clr;

	// ICONVIEW
	ushort	iconview_x;
	ushort	iconview_y;
	ulong	iconview_bkg_clr_unsel;
	ulong	iconview_bkg_clr_sel;
	ulong	iconview_txt_clr_unsel;
	ulong	iconview_txt_clr_sel;

} UIMenuLayout;

#endif
