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
#include "main.h"

#ifdef CONTEXT_VIDEO

#include "gui.h"
#include "dialog.h"

#include "ui_controls_wifi.h"
#include "desktop\ui_controls_layout.h"

#include "ui_lora_state.h"

uchar loc_esp32_status = 0;
uchar loc_wifi_on = 0;

// Public radio state
extern struct	TRANSCEIVER_STATE_UI	tsu;
extern struct 	UI_LORA_STATE 			uls;

//*----------------------------------------------------------------------------
//* Function Name       : ui_controls_wifi_init
//* Object              :
//* Notes    			: create control
//* Notes   			:
//* Notes    			:
//* Context    			: CONTEXT_VIDEO
//*----------------------------------------------------------------------------
void ui_controls_wifi_init(void)
{
	// State on start
	loc_esp32_status = 0;
	loc_wifi_on      = 0;

	// Fill the inside
	GUI_SetColor(GUI_LIGHTGRAY);
	GUI_FillRect(WIFI_X, WIFI_Y, WIFI_X + WIFI_SIZE_X, WIFI_Y + WIFI_SIZE_Y);

	// Create frame
	GUI_SetColor(GUI_WHITE);
	GUI_DrawRect(WIFI_X, WIFI_Y, WIFI_X + WIFI_SIZE_X, WIFI_Y + WIFI_SIZE_Y);
	GUI_DrawRect(WIFI_X + 1, WIFI_Y + 1, WIFI_X + WIFI_SIZE_X - 1, WIFI_Y + WIFI_SIZE_Y - 1);

	// Control separators
	GUI_SetColor(GUI_WHITE);
	GUI_DrawHLine(WIFI_Y + 20, WIFI_X, WIFI_X + WIFI_SIZE_X/2);
	GUI_DrawHLine(WIFI_Y + 21, WIFI_X, WIFI_X + WIFI_SIZE_X/2);
	GUI_DrawVLine(WIFI_X + WIFI_SIZE_X/2, WIFI_Y, WIFI_Y + WIFI_SIZE_Y - 15);
	GUI_DrawVLine(WIFI_X + WIFI_SIZE_X/2 + 1, WIFI_Y, WIFI_Y + WIFI_SIZE_Y - 15);
	GUI_DrawHLine(WIFI_Y + WIFI_SIZE_Y - 15, WIFI_X + WIFI_SIZE_X/2, WIFI_X + WIFI_SIZE_X);
	GUI_DrawHLine(WIFI_Y + WIFI_SIZE_Y - 16, WIFI_X + WIFI_SIZE_X/2, WIFI_X + WIFI_SIZE_X);

	// Cut-outs for top line text
	// COOP
	GUI_SetColor(GUI_GRAY);
	GUI_FillRect(WIFI_X + 3, WIFI_Y + 3, WIFI_X + 40, WIFI_Y + 18);
	GUI_SetColor(GUI_WHITE);
	GUI_DrawRect(WIFI_X + 3, WIFI_Y + 3, WIFI_X + 40, WIFI_Y + 18);
	// Blinker
	GUI_SetColor(GUI_GRAY);
	GUI_FillRect(WIFI_X + 43, WIFI_Y + 3, WIFI_X + 50, WIFI_Y + 18);
	GUI_SetColor(GUI_WHITE);
	GUI_DrawRect(WIFI_X + 43, WIFI_Y + 3, WIFI_X + 50, WIFI_Y + 18);
	// Version
	GUI_SetColor(GUI_GRAY);
	GUI_FillRect(WIFI_X + 53, WIFI_Y + 3, WIFI_X + 102, WIFI_Y + 18);
	GUI_SetColor(GUI_WHITE);
	GUI_DrawRect(WIFI_X + 53, WIFI_Y + 3, WIFI_X + 102, WIFI_Y + 18);
	// LORA
	GUI_SetColor(GUI_GRAY);
	GUI_FillRect((WIFI_X + WIFI_SIZE_X/2) + 3, WIFI_Y + 3, (WIFI_X + WIFI_SIZE_X/2) + 40, WIFI_Y + 18);
	GUI_SetColor(GUI_WHITE);
	GUI_DrawRect((WIFI_X + WIFI_SIZE_X/2) + 3, WIFI_Y + 3, (WIFI_X + WIFI_SIZE_X/2) + 40, WIFI_Y + 18);
	// LORA State
	GUI_SetColor(GUI_GRAY);
	GUI_FillRect((WIFI_X + WIFI_SIZE_X/2) + 44, WIFI_Y + 3, (WIFI_X + WIFI_SIZE_X/2) + 90, WIFI_Y + 18);
	GUI_SetColor(GUI_WHITE);
	GUI_DrawRect((WIFI_X + WIFI_SIZE_X/2) + 44, WIFI_Y + 3, (WIFI_X + WIFI_SIZE_X/2) + 90, WIFI_Y + 18);

	// Top line text
	GUI_SetFont(&GUI_Font8x16_1);
	GUI_SetColor(GUI_LIGHTGRAY);
//!	GUI_DispStringAt("COOP", WIFI_X + 6, WIFI_Y + 4);
	GUI_SetColor(GUI_LIGHTGRAY);
//!	GUI_DispStringAt("LORA", (WIFI_X + WIFI_SIZE_X/2) + 6, WIFI_Y + 4);
	GUI_SetColor(GUI_LIGHTGRAY);
//!	GUI_DispStringAt("OFF", (WIFI_X + WIFI_SIZE_X/2) + 48, WIFI_Y + 4);

	// Cut-outs for middle line text
	// WIFI
	GUI_SetColor(GUI_GRAY);
	GUI_FillRect(WIFI_X + 3, WIFI_Y + 24, WIFI_X + 40, WIFI_Y + 40);
	GUI_SetColor(GUI_WHITE);
	GUI_DrawRect(WIFI_X + 3, WIFI_Y + 24, WIFI_X + 40, WIFI_Y + 40);
	// RSSI
	GUI_SetColor(GUI_GRAY);
	GUI_FillRect((WIFI_X + 44), WIFI_Y + 24, WIFI_X + 102, WIFI_Y + 40);
	GUI_SetColor(GUI_WHITE);
	GUI_DrawRect((WIFI_X + 44), WIFI_Y + 24, WIFI_X + 102, WIFI_Y + 40);
	// LORA devices
	GUI_SetColor(GUI_GRAY);
	GUI_FillRect((WIFI_X + 108), WIFI_Y + 24, WIFI_X + 190, WIFI_Y + 40);
	GUI_SetColor(GUI_WHITE);
	GUI_DrawRect((WIFI_X + 108), WIFI_Y + 24, WIFI_X + 190, WIFI_Y + 40);

	// Middle text
	GUI_SetFont(&GUI_Font8x16_1);
	GUI_SetColor(GUI_LIGHTGRAY);
//!	GUI_DispStringAt("WIFI", WIFI_X + 6, WIFI_Y + 26);
	GUI_SetColor(GUI_LIGHTGRAY);
//!	GUI_DispStringAt("-52dBm", WIFI_X + 48, WIFI_Y + 26);

	// Cut-outs for bottom line(SSID)
	GUI_SetColor(GUI_GRAY);
	GUI_FillRect(WIFI_X + 3, WIFI_Y + 42, WIFI_X + 206, WIFI_Y + 54);
	GUI_SetColor(GUI_WHITE);
	GUI_DrawRect(WIFI_X + 3, WIFI_Y + 42, WIFI_X + 206, WIFI_Y + 54);

	// Bottom text
	GUI_SetFont(&GUI_Font8x8_1);
	GUI_SetColor(GUI_LIGHTRED);
//!	GUI_DispStringAt("Varna City", WIFI_X + 6, WIFI_Y + 45);
}

//*----------------------------------------------------------------------------
//* Function Name       : ui_controls_wifi_quit
//* Object              :
//* Notes    			: clean-up
//* Notes   			:
//* Notes    			:
//* Context    			: CONTEXT_VIDEO
//*----------------------------------------------------------------------------
void ui_controls_wifi_quit(void)
{

}

//*----------------------------------------------------------------------------
//* Function Name       : ui_controls_wifi_touch
//* Object              :
//* Notes    			:
//* Notes   			:
//* Notes    			:
//* Context    			: CONTEXT_VIDEO
//*----------------------------------------------------------------------------
void ui_controls_wifi_touch(void)
{

}

//*----------------------------------------------------------------------------
//* Function Name       : ui_controls_wifi_refresh
//* Object              :
//* Notes    			:
//* Notes   			:
//* Notes    			:
//* Context    			: CONTEXT_VIDEO
//*----------------------------------------------------------------------------
void ui_controls_wifi_refresh(void)
{
	// Update co-processor status
	if(loc_esp32_status != tsu.esp32_alive)
	{
		// Cut-outs for top line text (COOP)
		GUI_SetColor(GUI_GRAY);
		GUI_FillRect(WIFI_X + 3, WIFI_Y + 3, WIFI_X + 40, WIFI_Y + 18);
		GUI_SetColor(GUI_WHITE);
		GUI_DrawRect(WIFI_X + 3, WIFI_Y + 3, WIFI_X + 40, WIFI_Y + 18);

		GUI_SetFont(&GUI_Font8x16_1);

		if(tsu.esp32_alive)
		{
			GUI_SetColor(GUI_RED);

			// Show version
			if(strlen(tsu.esp32_version))
				GUI_DispStringAt(tsu.esp32_version + 4, WIFI_X + 57, WIFI_Y + 4);
		}
		else
			GUI_SetColor(GUI_LIGHTGRAY);

		// Top line text
//!		GUI_DispStringAt("COOP", WIFI_X + 6, WIFI_Y + 4);

		loc_esp32_status = tsu.esp32_alive;
	}
	else if(tsu.esp32_alive)
	{
		// Cut-out for top line (blinker)
		GUI_SetColor(GUI_GRAY);
		GUI_FillRect(WIFI_X + 43, WIFI_Y + 3, WIFI_X + 50, WIFI_Y + 18);
		GUI_SetColor(GUI_WHITE);
		GUI_DrawRect(WIFI_X + 43, WIFI_Y + 3, WIFI_X + 50, WIFI_Y + 18);

		// Sort the blinker
		if(tsu.esp32_blink)
		{
			GUI_SetColor(GUI_RED);
			GUI_FillRect(WIFI_X + 45, WIFI_Y + 5, WIFI_X + 48, WIFI_Y + 16);
		}
	}

	// Need to update WiFi state
	if(loc_wifi_on != tsu.wifi_on)
	{
		#if 0
		// Top line text
		GUI_SetFont(&GUI_Font8x16_1);

		// Clear control
		GUI_SetColor(GUI_BLACK);
		GUI_FillRect(WIFI_X + 2, WIFI_Y + 2, WIFI_X + 39, WIFI_Y + 17);
		GUI_SetColor(GUI_WHITE);
		GUI_DrawRect(WIFI_X + 2, WIFI_Y + 2, WIFI_X + 39, WIFI_Y + 17);

		if(tsu.wifi_on)
			GUI_SetColor(GUI_RED);
		else
			GUI_SetColor(GUI_LIGHTGRAY);

		GUI_DispStringAt("WIFI", WIFI_X + 5, WIFI_Y + 3);
		#endif

		#if 0
		if(tsu.wifi_rssi)
		{
				char buf[30];
				hItem = WM_GetDialogItem(pMsg->hWin, ID_TEXT_WIFI);
				sprintf(buf, "%d dBm", tsu.wifi_rssi);
				TEXT_SetText(hItem, buf);

				WM_InvalidateWindow(hWiFiDialog);
		}
		#endif

		// Save last state
		loc_wifi_on = tsu.wifi_on;
	}

	// LORA state changes causes repaint of specific controls
	if(uls.force_ui_repaint)
	{
		uchar ev = uls.last_event;

		//printf("lora repaint: %d\r\n", uls.last_event);

		GUI_SetFont(&GUI_Font8x16_1);

		GUI_SetColor(GUI_GRAY);
		GUI_FillRect((WIFI_X + WIFI_SIZE_X/2) + 3, WIFI_Y + 3, (WIFI_X + WIFI_SIZE_X/2) + 40, WIFI_Y + 18);

		if(uls.power_on)
			GUI_SetColor(GUI_WHITE);
		else
			GUI_SetColor(GUI_LIGHTGRAY);

//!		GUI_DispStringAt("LORA", (WIFI_X + WIFI_SIZE_X/2) + 6, WIFI_Y + 4);

		char buff[50];

		memset(buff, 0, sizeof(buff));
		if(ui_lora_str_state(ev, buff) == 0)
		{
			GUI_SetColor(GUI_GRAY);
			GUI_FillRect((WIFI_X + WIFI_SIZE_X/2) + 44, WIFI_Y + 3, (WIFI_X + WIFI_SIZE_X/2) + 90, WIFI_Y + 18);

			if(ev == EV_TXSTART)
				GUI_SetColor(GUI_RED);
			else
				GUI_SetColor(GUI_WHITE);

			GUI_DispStringAt(buff, (WIFI_X + WIFI_SIZE_X/2) + 48, WIFI_Y + 4);
		}

		uls.force_ui_repaint = 0;
	}
}

#endif
