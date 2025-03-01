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

#include "gui.h"
#include "dialog.h"

#include "ui_cool_progress.h"

static void ui_cool_progress_create(int x, int y, ushort val)
{
	uchar i;

	// Background
	GUI_SetColor(GUI_DARKGRAY);
	for(i = 0; i < COOL_PROG_WIDTH*2; i++)
	{
		GUI_SetAlpha(20 * (i + 1));
		GUI_DrawArc(x, y, (COOL_PROG_SIZE - 2 + i), 0, 0, 360);
	}

	GUI_SetAlpha(128);

	// Foreground
	GUI_SetColor(GUI_WHITE);
	for(i = 0; i < COOL_PROG_WIDTH; i++)
		GUI_DrawArc(x, y, (COOL_PROG_SIZE + i), 0, val, 450);

	GUI_SetAlpha(255);
}

void ui_cool_progress_tx_pwr(int x, int y, ushort val, char *txt)
{
	ulong 	st;

	if(txt == NULL)
		return;

	if(val > 100)
		val = 100;

	// To start angle
	st = (4500 - (0 + val*36))/10;

	// Draw common component
	ui_cool_progress_create(x, y, st);

	// Progress name
	GUI_SetColor(GUI_WHITE);
	GUI_SetFont(&GUI_Font8x16_1);
	GUI_SetAlpha(88);
	GUI_DispStringAt("power", x - 20, y + 32);
	GUI_SetAlpha(255);

	// Clear progress text
	GUI_SetColor(GUI_BLACK);
	GUI_FillRect(x - 15, y - 15, x + 16, y + 16);

	//if(val < 70)
	//	GUI_SetColor(GUI_GREEN);
	//else
	//	GUI_SetColor(GUI_RED);
	GUI_SetColor(GUI_WHITE);

	// Progress text
	GUI_SetFont(&GUI_Font32B_ASCII);
	GUI_DispStringAt(txt, x - 15, y - 15);
}
