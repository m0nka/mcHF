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
#include "mchf_pro_board.h"

#include "ui_menu_layout.h"

// UI controls definitions based on Theme (in Flash)
//
const struct UIMenuLayout menu_layout[MAX_THEME] =
{
	// ------------------------------------------------------
	// THEME_KENWOOD - mcHF 0.8 look
	{
			"Kenwood",
			//
			BKG_COLOR_1,
			//
			MENU_BAR_X_1, MENU_BAR_Y_1, MENU_BAR_X_SIZE_1, MENU_BAR_Y_SIZE_1, MENU_BAR_COLOR_1, MENU_TXT_COLOR_1,
			//
			ICONVIEW_X_1, ICONVIEW_Y_1, ICONVIEW_BKG_COL_UNSEL_1, ICONVIEW_BKG_COL_SEL_1, ICONVIEW_TXT_COL_UNSEL_1, ICONVIEW_TXT_COL_SEL_1
	},

	// ------------------------------------------------------
	// THEME_SENBONO - the S10 has an amazing look, it is what the Hilberling PT-8000 could have been if the coders had the vision
	{
			"Red Dawn",
			//
			BKG_COLOR_2,
			//
			MENU_BAR_X_2, MENU_BAR_Y_2, MENU_BAR_X_SIZE_2, MENU_BAR_Y_SIZE_2, MENU_BAR_COLOR_2, MENU_TXT_COLOR_2,
			//
			ICONVIEW_X_2, ICONVIEW_Y_2, ICONVIEW_BKG_COL_UNSEL_2, ICONVIEW_BKG_COL_SEL_2, ICONVIEW_TXT_COL_UNSEL_2, ICONVIEW_TXT_COL_SEL_2
	},

	// ------------------------------------------------------
	// THEME_ICOM - the classic mcHF 0.6 look
	{
			"Icom",
			//
			BKG_COLOR_1,
			//
			MENU_BAR_X_1, MENU_BAR_Y_1, MENU_BAR_X_SIZE_1, MENU_BAR_Y_SIZE_1, MENU_BAR_COLOR_1, MENU_TXT_COLOR_1,
			//
			ICONVIEW_X_1, ICONVIEW_Y_1, ICONVIEW_BKG_COL_UNSEL_1, ICONVIEW_BKG_COL_SEL_1, ICONVIEW_TXT_COL_UNSEL_1, ICONVIEW_TXT_COL_SEL_1
	}
};
