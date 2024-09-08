/************************************************************************************
**                                                                                 **
**                             mcHF Pro QRP Transceiver                            **
**                         Krassi Atanassov - M0NKA, 2013-2024                     **
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
#include "main.h"

#ifdef CONTEXT_VIDEO

#include "gui.h"
#include "dialog.h"

#include "ui_controls_menu_button.h"
#include "desktop\ui_controls_layout.h"

extern GUI_CONST_STORAGE GUI_BITMAP bmsettings_menu;

//*----------------------------------------------------------------------------
//* Function Name       :
//* Object              :
//* Notes    			:
//* Notes   			:
//* Notes    			:
//* Context    			: CONTEXT_VIDEO
//*----------------------------------------------------------------------------
void ui_controls_menu_button_init(void)
{
	// Background
	GUI_SetColor(GUI_LIGHTGRAY);
	GUI_FillRoundedRect(MENU_BTN_X, MENU_BTN_Y, (MENU_BTN_X + MENU_BTN_SIZE_X), (MENU_BTN_Y + MENU_BTN_SIZE_Y), 2);

	// Show icon
	GUI_DrawBitmap(&bmsettings_menu, MENU_BTN_X, (MENU_BTN_Y + 1));

	// Draw outline
	GUI_SetColor(GUI_WHITE);
	GUI_DrawRoundedRect(MENU_BTN_X, MENU_BTN_Y, (MENU_BTN_X + MENU_BTN_SIZE_X), (MENU_BTN_Y + MENU_BTN_SIZE_Y), 2);
	GUI_DrawRoundedRect(MENU_BTN_X + 1, MENU_BTN_Y + 1, (MENU_BTN_X + MENU_BTN_SIZE_X - 1), (MENU_BTN_Y + MENU_BTN_SIZE_Y - 1), 2);

	// Bottom text
	GUI_SetColor(GUI_BLACK);
	GUI_SetFont (GUI_FONT_16B_1);
	GUI_DispStringAt("MENU", (MENU_BTN_X + 14), (MENU_BTN_Y + MENU_BTN_SIZE_Y - 20));
}

// Is it touched ?
int ui_controls_menu_button_is_touch(int x, int y)
{
	int bar_x, bar_y;

	//-------------------------------------------
	// MENU position
	bar_x = MENU_BTN_X;
	bar_y = MENU_BTN_Y;

	// Is MENU label touched ?
	if((x > bar_x) && (x < (bar_x + MENU_BTN_SIZE_X)) && (y > (bar_y - 0)) && (y < bar_y + MENU_BTN_SIZE_Y))
		return 1;

	return 0;
}

//*----------------------------------------------------------------------------
//* Function Name       :
//* Object              :
//* Notes    			:
//* Notes   			:
//* Notes    			:
//* Context    			: CONTEXT_VIDEO
//*----------------------------------------------------------------------------
void ui_controls_menu_button_quit(void)
{

}

//*----------------------------------------------------------------------------
//* Function Name       :
//* Object              :
//* Notes    			:
//* Notes   			:
//* Notes    			:
//* Context    			: CONTEXT_VIDEO
//*----------------------------------------------------------------------------
void ui_controls_menu_button_refresh(void)
{

}

#endif
