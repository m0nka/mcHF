//************************************************************************************
//**                                                                                 **
//**                                 mcHF QRP Transceiver                            **
//**                         Krassi Atanassov - M0NKA, 2013-2026                     **
//**                                                                                 **
//**---------------------------------------------------------------------------------**
//**                                                                                 **
//**  File name:		lcd_high.h                                                   **
//**  Description:  	basic drawing services (text, shapes) on top of lcd_low     **
//**  Licence:			https://github.com/m0nka/mcHF/blob/main/LICENSE            **
//************************************************************************************
#ifndef __LCD_HIGH_H
#define __LCD_HIGH_H

#ifdef USE_LCD_BAREMETAL

#include "fonts.h"
#include <stddef.h>

// All colours are ARGB8888 - quantised to the RGB332 palette by lcd_low
#define LCD_COLOR_BLUE          0xFF0000FFUL
#define LCD_COLOR_GREEN         0xFF00FF00UL
#define LCD_COLOR_RED           0xFFFF0000UL
#define LCD_COLOR_CYAN          0xFF00FFFFUL
#define LCD_COLOR_MAGENTA       0xFFFF00FFUL
#define LCD_COLOR_YELLOW        0xFFFFFF00UL
#define LCD_COLOR_LIGHTBLUE     0xFF8080FFUL
#define LCD_COLOR_LIGHTGREEN    0xFF80FF80UL
#define LCD_COLOR_LIGHTRED      0xFFFF8080UL
#define LCD_COLOR_LIGHTCYAN     0xFF80FFFFUL
#define LCD_COLOR_LIGHTMAGENTA  0xFFFF80FFUL
#define LCD_COLOR_LIGHTYELLOW   0xFFFFFF80UL
#define LCD_COLOR_DARKBLUE      0xFF000080UL
#define LCD_COLOR_DARKGREEN     0xFF008000UL
#define LCD_COLOR_DARKRED       0xFF800000UL
#define LCD_COLOR_DARKCYAN      0xFF008080UL
#define LCD_COLOR_DARKMAGENTA   0xFF800080UL
#define LCD_COLOR_DARKYELLOW    0xFF808000UL
#define LCD_COLOR_WHITE         0xFFFFFFFFUL
#define LCD_COLOR_LIGHTGRAY     0xFFD3D3D3UL
#define LCD_COLOR_GRAY          0xFF808080UL
#define LCD_COLOR_DARKGRAY      0xFF404040UL
#define LCD_COLOR_BLACK         0xFF000000UL
#define LCD_COLOR_BROWN         0xFFA52A2AUL
#define LCD_COLOR_ORANGE        0xFFFFA500UL

// Default font
#define LCD_DEFAULT_FONT        Font24

#define LINE(x) ((x) * (((sFONT *)lcd_high_GetFont())->Height))

// Drawing context
typedef struct
{
  uint32_t  TextColor;			// text colour
  uint32_t  BackColor;			// background colour below the text
  sFONT    *pFont;				// font used for the text
  uint32_t  GuiLayer;
  uint32_t  GuiDevice;
  uint32_t  GuiXsize;
  uint32_t  GuiYsize;
} lcd_high_Ctx_t;

// Drawing point (pixel) geometric definition
typedef struct
{
  int16_t X;
  int16_t Y;
} Point;

typedef Point * pPoint;

// Text alignment
typedef enum
{
  CENTER_MODE             = 0x01,
  RIGHT_MODE              = 0x02,
  LEFT_MODE               = 0x03
} Text_AlignModeTypdef;

void     lcd_high_SetFuncDriver(const lcd_low_Drv_t *pDrv);

void     lcd_high_SetLayer(uint32_t Layer);
void     lcd_high_SetDevice(uint32_t Device);

void     lcd_high_SetTextColor(uint32_t Color);
uint32_t lcd_high_GetTextColor(void);
void     lcd_high_SetBackColor(uint32_t Color);
uint32_t lcd_high_GetBackColor(void);
void     lcd_high_SetFont(sFONT *fonts);
sFONT    *lcd_high_GetFont(void);

void     lcd_high_Clear(uint32_t Color);
void     lcd_high_ClearStringLine(uint32_t Line);
void     lcd_high_DisplayStringAtLine(uint32_t Line, uint8_t *ptr);
void     lcd_high_DisplayStringAt(uint32_t Xpos, uint32_t Ypos, uint8_t *Text, Text_AlignModeTypdef Mode);
void     lcd_high_DisplayChar(uint32_t Xpos, uint32_t Ypos, uint8_t Ascii);
void     lcd_high_GetPixel(uint16_t Xpos, uint16_t Ypos, uint32_t *Color);
void     lcd_high_SetPixel(uint16_t Xpos, uint16_t Ypos, uint32_t Color);
void     lcd_high_FillRGBRect(uint32_t Xpos, uint32_t Ypos, uint8_t *pData, uint32_t Width, uint32_t Height);
void     lcd_high_DrawHLine(uint32_t Xpos, uint32_t Ypos, uint32_t Length, uint32_t Color);
void     lcd_high_DrawVLine(uint32_t Xpos, uint32_t Ypos, uint32_t Length, uint32_t Color);
void     lcd_high_DrawBitmap(uint32_t Xpos, uint32_t Ypos, uint8_t *pData);
void     lcd_high_FillRect(uint32_t Xpos, uint32_t Ypos, uint32_t Width, uint32_t Height, uint32_t Color);
void     lcd_high_DrawLine(uint32_t Xpos1, uint32_t Ypos1, uint32_t Xpos2, uint32_t Ypos2, uint32_t Color);
void     lcd_high_DrawRect(uint32_t Xpos, uint32_t Ypos, uint32_t Width, uint32_t Height, uint32_t Color);
void     lcd_high_DrawCircle(uint32_t Xpos, uint32_t Ypos, uint32_t Radius, uint32_t Color);
void     lcd_high_DrawCircleG(uint32_t Xpos, uint32_t Ypos, uint32_t Radius, uint32_t Color);
void     lcd_high_DrawPolygon(pPoint Points, uint32_t PointCount, uint32_t Color);
void     lcd_high_DrawEllipse(int Xpos, int Ypos, int XRadius, int YRadius, uint32_t Color);
void     lcd_high_FillCircle(uint32_t Xpos, uint32_t Ypos, uint32_t Radius, uint32_t Color);
void     lcd_high_FillPolygon(pPoint Points, uint32_t PointCount, uint32_t Color);
void     lcd_high_FillEllipse(int Xpos, int Ypos, int XRadius, int YRadius, uint32_t Color);

#endif

#endif
