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
#include "mchf_pro_board.h"
#include "main.h"

#ifdef CONTEXT_VIDEO

#include "gui.h"
#include "dialog.h"

#include "ui_controls_sd_icon.h"
#include "desktop\ui_controls_layout.h"

extern GUI_CONST_STORAGE GUI_BITMAP bmsd_small;

void ui_controls_sd_icon_update(uchar is_init)
{
	static uchar loc_sd_status = 0;
	uchar isInitialized;

	// From SD driver
	isInitialized = Storage_GetStatus(MSD_DISK_UNIT);

	// Prevent over update
	if((loc_sd_status == isInitialized) && (!is_init))
		return;

	if(isInitialized)
	{
		// Icon
		GUI_DrawBitmap(&bmsd_small, (SD_CARD_X + 1), (SD_CARD_Y + 1));

		// Frame
		GUI_SetColor(GUI_RED);
		GUI_DrawRoundedRect((SD_CARD_X + 0),(SD_CARD_Y + 0),(SD_CARD_X + 40),(SD_CARD_Y + 49),2);
		GUI_DrawRect((SD_CARD_X + 1),(SD_CARD_Y + 1),(SD_CARD_X + 39),(SD_CARD_Y + 48));
	}
	else
	{
		// Delete
		GUI_SetColor(GUI_BLACK);
		GUI_FillRoundedRect((SD_CARD_X + 0),(SD_CARD_Y + 0),(SD_CARD_X + 40),(SD_CARD_Y + 49),2);
	}

	// Save to local
	loc_sd_status = isInitialized;
}

//*----------------------------------------------------------------------------
//* Function Name       :
//* Object              :
//* Notes    			:
//* Notes   			:
//* Notes    			:
//* Context    			: CONTEXT_VIDEO
//*----------------------------------------------------------------------------
void ui_controls_sd_icon_init(void)
{
	// Initial paint
	ui_controls_sd_icon_update(1);
}

//*----------------------------------------------------------------------------
//* Function Name       :
//* Object              :
//* Notes    			:
//* Notes   			:
//* Notes    			:
//* Context    			: CONTEXT_VIDEO
//*----------------------------------------------------------------------------
void ui_controls_sd_icon_quit(void)
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
void ui_controls_sd_icon_touch(void)
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
void ui_controls_sd_icon_refresh(void)
{
	// Initial paint
	ui_controls_sd_icon_update(0);
}

#endif
