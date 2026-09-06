/************************************************************************************
**                                                                                 **
**                                 mcHF QRP Transceiver                            **
**                         Krassi Atanassov - M0NKA, 2013-2026                     **
**                                                                                 **
**---------------------------------------------------------------------------------**
**                                                                                 **
**  File name:		atlas_draw.c                                                   **
**  Description:	Atlas theme drawing primitives                                 **
**  Licence:		https://github.com/m0nka/mcHF/blob/main/LICENSE                **
************************************************************************************/
//
// The look comes from the Atlas comms panel reference (claude/MarsChat,
// design stills) and from the sketch in the bootloader's ui_proc.c: the
// same vocabulary of hairline ticks, nested outlines and corner marks,
// re-drawn through emWin's GUI_* calls so it can live under widgets.
//
// All colours are opaque - this emWin build has no alpha, so the
// translucent fills of the reference are pre-blended into the palette
//
#include "mchf_pro_board.h"
#include "main.h"

#include "GUI.h"

#include "atlas_draw.h"

//*----------------------------------------------------------------------------
//* Function Name       : atlas_background
//* Object              : ground fill plus the faint horizontal banding
//* Context    			: CONTEXT_VIDEO (gui task, inside WM_PAINT)
//*----------------------------------------------------------------------------
void atlas_background(int x, int y, int w, int h)
{
	int	i;

	GUI_SetColor(ATLAS_GROUND);
	GUI_FillRect(x, y, x + w - 1, y + h - 1);

	GUI_SetColor(ATLAS_BAND);

	for(i = y; i < (y + h); i += 6)
		GUI_DrawHLine(i, x, x + w - 1);
}

//*----------------------------------------------------------------------------
//* Function Name       : atlas_grid
//* Object              : faint overlay grid with edge labels - the sci-fi
//*						: instrumentation texture from the Atlas reference
//* Context    			: CONTEXT_VIDEO (gui task, inside WM_PAINT)
//*----------------------------------------------------------------------------
void atlas_grid(int x, int y, int w, int h, int pitch)
{
	int		i, n;
	char	tag[8];

	if(pitch < 8)
		pitch = 8;

	// Grid lines - ATLAS_LINE_OFF is dim enough to read as low-alpha
	// but bright enough to actually show on the IPS panel (ATLAS_BAND
	// was invisible)
	GUI_SetColor(ATLAS_LINE_OFF);

	for(i = x + pitch; i < (x + w); i += pitch)
		GUI_DrawVLine(i, y, y + h - 1);

	for(i = y + pitch; i < (y + h); i += pitch)
		GUI_DrawHLine(i, x, x + w - 1);

	// Small edge labels - decorative "instrument readout" texture
	GUI_SetFont(&GUI_Font8_1);
	GUI_SetColor(ATLAS_OFF);
	GUI_SetTextMode(GUI_TM_TRANS);

	// Left edge: "X:nn" at each horizontal grid line
	n = 0;
	for(i = y + pitch; i < (y + h - 8); i += pitch)
	{
		n++;
		tag[0] = 'X';
		tag[1] = ':';
		tag[2] = '0' + (n / 10) % 10;
		tag[3] = '0' + n % 10;
		tag[4] = 0;
		GUI_DispStringAt(tag, x + 2, i + 2);
	}

	// Bottom edge: "Y:nn" at each vertical grid line
	n = 0;
	for(i = x + pitch; i < (x + w - 20); i += pitch)
	{
		n++;
		tag[0] = 'Y';
		tag[1] = ':';
		tag[2] = '0' + (n / 10) % 10;
		tag[3] = '0' + n % 10;
		tag[4] = 0;
		GUI_DispStringAt(tag, i + 2, y + h - 10);
	}
}

//*----------------------------------------------------------------------------
//* Function Name       : atlas_panel
//* Object              : panel fill, hairline border and the amber corner
//*						: brackets that frame content in the reference
//* Context    			: CONTEXT_VIDEO (gui task, inside WM_PAINT)
//*----------------------------------------------------------------------------
void atlas_panel(int x, int y, int w, int h, uint8_t brackets)
{
	int	x1 = x + w - 1;
	int	y1 = y + h - 1;
	int	l  = ATLAS_BRACKET_LEN;
	int	t  = ATLAS_BRACKET_TH;

	GUI_SetColor(ATLAS_PANEL);
	GUI_FillRect(x, y, x1, y1);

	GUI_SetColor(ATLAS_LINE);
	GUI_DrawRect(x, y, x1, y1);

	if(!brackets)
		return;

	GUI_SetColor(ATLAS_AMBER);

	// Top left / top right
	GUI_FillRect(x, y, x + l, y + t - 1);
	GUI_FillRect(x, y, x + t - 1, y + l);
	GUI_FillRect(x1 - l, y, x1, y + t - 1);
	GUI_FillRect(x1 - t + 1, y, x1, y + l);

	// Bottom left / bottom right
	GUI_FillRect(x, y1 - t + 1, x + l, y1);
	GUI_FillRect(x, y1 - l, x + t - 1, y1);
	GUI_FillRect(x1 - l, y1 - t + 1, x1, y1);
	GUI_FillRect(x1 - t + 1, y1 - l, x1, y1);
}

//*----------------------------------------------------------------------------
//* Function Name       : atlas_bar
//* Object              : horizontal gradient block
//* Context    			: CONTEXT_VIDEO (gui task, inside WM_PAINT)
//*----------------------------------------------------------------------------
void atlas_bar(int x, int y, int w, int h, GUI_COLOR left, GUI_COLOR right)
{
	GUI_DrawGradientH(x, y, x + w - 1, y + h - 1, left, right);
}

//*----------------------------------------------------------------------------
//* Function Name       : atlas_ticks
//* Object              : row of hairline ticks
//* Context    			: CONTEXT_VIDEO (gui task, inside WM_PAINT)
//*----------------------------------------------------------------------------
void atlas_ticks(int x, int y, int w, int h, int step, int tw, GUI_COLOR col)
{
	int	i;

	if(step < 1)
		step = 1;

	GUI_SetColor(col);

	for(i = 0; i < w; i += step)
		GUI_FillRect(x + i, y, x + i + tw - 1, y + h - 1);
}

//*----------------------------------------------------------------------------
//* Function Name       : atlas_text
//* Object              : draw a string with extra spacing between the
//*						: characters - emWin has no letter spacing and the
//*						: wide tracking is half of this look
//* Notes    			: uses the current font and colour, returns the x
//*						: just past the last character drawn
//* Context    			: CONTEXT_VIDEO (gui task, inside WM_PAINT)
//*----------------------------------------------------------------------------
int atlas_text(int x, int y, const char *s, int tracking)
{
	if(s == NULL)
		return x;

	while(*s)
	{
		GUI_DispCharAt((U16)*s, x, y);
		x += GUI_GetCharDistX((U16)*s) + tracking;
		s++;
	}

	return x;
}

//*----------------------------------------------------------------------------
//* Function Name       : atlas_text_width
//* Object              : width atlas_text would occupy, tracking included
//* Context    			: CONTEXT_VIDEO (gui task)
//*----------------------------------------------------------------------------
int atlas_text_width(const char *s, int tracking)
{
	int	w = 0;

	if(s == NULL)
		return 0;

	while(*s)
	{
		w += GUI_GetCharDistX((U16)*s) + tracking;
		s++;
	}

	// The trailing gap is not part of the glyph run
	if(w > 0)
		w -= tracking;

	return w;
}

//*----------------------------------------------------------------------------
//* Function Name       : atlas_text_right
//* Object              : tracked text ending at x_right
//* Context    			: CONTEXT_VIDEO (gui task, inside WM_PAINT)
//*----------------------------------------------------------------------------
void atlas_text_right(int x_right, int y, const char *s, int tracking)
{
	atlas_text(x_right - atlas_text_width(s, tracking), y, s, tracking);
}

//*----------------------------------------------------------------------------
//* Function Name       : atlas_chip
//* Object              : small outlined (or filled) label
//* Context    			: CONTEXT_VIDEO (gui task, inside WM_PAINT)
//*----------------------------------------------------------------------------
void atlas_chip(int x, int y, int w, int h, const char *s, GUI_COLOR col, uint8_t filled)
{
	int	tw;

	if(filled)
	{
		GUI_SetColor(col);
		GUI_FillRect(x, y, x + w - 1, y + h - 1);
		GUI_SetColor(ATLAS_INK);
	}
	else
	{
		GUI_SetColor(col);
		GUI_DrawRect(x, y, x + w - 1, y + h - 1);
	}

	GUI_SetBkColor(filled ? col : ATLAS_PANEL);

	tw = atlas_text_width(s, 1);
	atlas_text(x + ((w - tw) / 2), y + ((h - GUI_GetFontSizeY()) / 2), s, 1);
}

//*----------------------------------------------------------------------------
//* Function Name       : atlas_track
//* Object              : progress / level track
//* Context    			: CONTEXT_VIDEO (gui task, inside WM_PAINT)
//*----------------------------------------------------------------------------
void atlas_track(int x, int y, int w, int h, int filled_w, GUI_COLOR back, GUI_COLOR fill)
{
	if(filled_w < 0)
		filled_w = 0;

	if(filled_w > w)
		filled_w = w;

	GUI_SetColor(back);
	GUI_FillRect(x, y, x + w - 1, y + h - 1);

	if(filled_w == 0)
		return;

	GUI_SetColor(fill);
	GUI_FillRect(x, y, x + filled_w - 1, y + h - 1);
}
