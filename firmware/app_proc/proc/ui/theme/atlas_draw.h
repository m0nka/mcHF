/************************************************************************************
**                                                                                 **
**                                 mcHF QRP Transceiver                            **
**                         Krassi Atanassov - M0NKA, 2013-2026                     **
**                                                                                 **
**---------------------------------------------------------------------------------**
**                                                                                 **
**  File name:		atlas_draw.h                                                   **
**  Description:	Atlas theme - palette and drawing primitives for the sci-fi    **
**					instrument look (dark navy ground, cyan gradient bars, amber   **
**					corner brackets, hairline tick rows).                          **
**					Nothing here knows what it is drawing for - screens call it    **
**  Licence:		https://github.com/m0nka/mcHF/blob/main/LICENSE                **
************************************************************************************/
#ifndef __ATLAS_DRAW_H
#define __ATLAS_DRAW_H

#include "GUI.h"

// emWin colours are 0x00BBGGRR, so the channels are given in the order
// they are read off a design (r, g, b) and swapped here once
#define ATLAS_RGB(r, g, b)	((GUI_COLOR)(((uint32_t)(b) << 16) | ((uint32_t)(g) << 8) | (uint32_t)(r)))

#define ATLAS_GROUND		ATLAS_RGB(0x04, 0x10, 0x1F)		// screen background
#define ATLAS_BAND			ATLAS_RGB(0x06, 0x17, 0x28)		// ground hairlines (pre-blended)
#define ATLAS_PANEL			ATLAS_RGB(0x08, 0x1B, 0x2C)		// panel fill
#define ATLAS_ROW			ATLAS_RGB(0x0B, 0x25, 0x3C)		// alternating row band
#define ATLAS_LINE			ATLAS_RGB(0x1E, 0x5C, 0x86)		// borders, outlines
#define ATLAS_LINE_OFF		ATLAS_RGB(0x14, 0x36, 0x4F)		// disabled outline
#define ATLAS_CYAN			ATLAS_RGB(0x2F, 0xC6, 0xEE)
#define ATLAS_CYAN_HI		ATLAS_RGB(0x9F, 0xE8, 0xFF)
#define ATLAS_CYAN_DEEP		ATLAS_RGB(0x0F, 0x65, 0x91)
#define ATLAS_AMBER			ATLAS_RGB(0xFF, 0xB0, 0x20)
#define ATLAS_AMBER_DEEP	ATLAS_RGB(0x7A, 0x52, 0x0E)
#define ATLAS_LIME			ATLAS_RGB(0xB9, 0xE2, 0x4B)
#define ATLAS_TEXT			ATLAS_RGB(0xDC, 0xEE, 0xF8)
#define ATLAS_DIM			ATLAS_RGB(0x6D, 0x93, 0xAE)
#define ATLAS_OFF			ATLAS_RGB(0x2C, 0x53, 0x72)		// disabled glyph
#define ATLAS_INK			ATLAS_RGB(0x06, 0x20, 0x33)		// text on a cyan fill

// Corner bracket geometry (the amber "L" marks framing a panel)
#define ATLAS_BRACKET_LEN	16
#define ATLAS_BRACKET_TH	3

// Ground: flat fill plus the faint 6 px banding of the reference
void	atlas_background(int x, int y, int w, int h);

// Panel: fill + hairline border, with amber corner brackets when
// brackets != 0
void	atlas_panel(int x, int y, int w, int h, uint8_t brackets);

// Horizontal gradient block (the channel/slot bar body)
void	atlas_bar(int x, int y, int w, int h, GUI_COLOR left, GUI_COLOR right);

// Row of hairline ticks - decorative telemetry texture. step is the
// pitch, tw the width of one tick
void	atlas_ticks(int x, int y, int w, int h, int step, int tw, GUI_COLOR col);

// Text with extra spacing between characters - emWin has no letter
// spacing, and the look depends on it. Uses the current font and colour,
// returns the x just past the last character
int		atlas_text(int x, int y, const char *s, int tracking);
int		atlas_text_width(const char *s, int tracking);
void	atlas_text_right(int x_right, int y, const char *s, int tracking);

// Small outlined label (RX / TX / --) used in the message list
void	atlas_chip(int x, int y, int w, int h, const char *s, GUI_COLOR col, uint8_t filled);

// Progress/level track with a filled portion, e.g. the burst hairline
void	atlas_track(int x, int y, int w, int h, int filled_w, GUI_COLOR back, GUI_COLOR fill);

#endif
