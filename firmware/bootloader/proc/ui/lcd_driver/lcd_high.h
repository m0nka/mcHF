/**
  ******************************************************************************
  * @file    basic_gui.h
  * @author  MCD Application Team
  * @brief   This file contains the common defines and functions prototypes for
  *          the basic_gui.c driver.
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2018 STMicroelectronics.
  * All rights reserved.</center></h2>
  *
  * This software component is licensed by ST under BSD 3-Clause license,
  * the "License"; You may not use this file except in compliance with the
  * License. You may obtain a copy of the License at:
  *                        opensource.org/licenses/BSD-3-Clause
  *
  ******************************************************************************
  */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __LCD_HIGH_H
#define __LCD_HIGH_H

#ifdef USE_LCD_BAREMETAL

/* Includes ------------------------------------------------------------------*/
#include "fonts.h"
#include <stddef.h>

#define BASIC_lcd_low_LANDSCAPE_565_MODE

#define lcd_low_COLOR_BLUE          0xFF0000FFUL
#define lcd_low_COLOR_GREEN         0xFF00FF00UL
#define lcd_low_COLOR_RED           0xFFFF0000UL
#define lcd_low_COLOR_CYAN          0xFF00FFFFUL
#define lcd_low_COLOR_MAGENTA       0xFFFF00FFUL
#define lcd_low_COLOR_YELLOW        0xFFFFFF00UL
#define lcd_low_COLOR_LIGHTBLUE     0xFF8080FFUL
#define lcd_low_COLOR_LIGHTGREEN    0xFF80FF80UL
#define lcd_low_COLOR_LIGHTRED      0xFFFF8080UL
#define lcd_low_COLOR_LIGHTCYAN     0xFF80FFFFUL
#define lcd_low_COLOR_LIGHTMAGENTA  0xFFFF80FFUL
#define lcd_low_COLOR_LIGHTYELLOW   0xFFFFFF80UL
#define lcd_low_COLOR_DARKBLUE      0xFF000080UL
#define lcd_low_COLOR_DARKGREEN     0xFF008000UL
#define lcd_low_COLOR_DARKRED       0xFF800000UL
#define lcd_low_COLOR_DARKCYAN      0xFF008080UL
#define lcd_low_COLOR_DARKMAGENTA   0xFF800080UL
#define lcd_low_COLOR_DARKYELLOW    0xFF808000UL
#define lcd_low_COLOR_WHITE         0xFFFFFFFFUL
#define lcd_low_COLOR_LIGHTGRAY     0xFFD3D3D3UL
#define lcd_low_COLOR_GRAY          0xFF808080UL
#define lcd_low_COLOR_DARKGRAY      0xFF404040UL
#define lcd_low_COLOR_BLACK         0xFF000000UL
#define lcd_low_COLOR_BROWN         0xFFA52A2AUL
#define lcd_low_COLOR_ORANGE        0xFFFFA500UL

/* Defintion of Official ST Colors */
#define lcd_low_COLOR_ST_BLUE_DARK   0xFF002052UL
#define lcd_low_COLOR_ST_BLUE        0xFF39A9DCUL
#define lcd_low_COLOR_ST_BLUE_LIGHT  0xFFD1E4F3UL
#define lcd_low_COLOR_ST_GREEN_LIGHT 0xFFBBCC01UL
#define lcd_low_COLOR_ST_GREEN_DARK  0xFF003D14UL
#define lcd_low_COLOR_ST_YELLOW      0xFFFFD300UL
#define lcd_low_COLOR_ST_BROWN       0xFF5C0915UL
#define lcd_low_COLOR_ST_PINK        0xFFD4007AUL
#define lcd_low_COLOR_ST_PURPLE      0xFF590D58UL
#define lcd_low_COLOR_ST_GRAY_DARK   0xFF4F5251UL
#define lcd_low_COLOR_ST_GRAY        0xFF90989EUL
#define lcd_low_COLOR_ST_GRAY_LIGHT  0xFFB9C4CAUL

/**
  * @brief GUI default font
  */
#define lcd_low_DEFAULT_FONT        Font24

/**
  * @}
  */

/** @defgroup BASIC_lcd_low_Exported_Types BASIC GUI Exported Types
  * @{
  */
#ifndef BASIC_lcd_low_LANDSCAPE_565_MODE
#define LINE(x) ((x) * (((sFONT *)lcd_low_GetFont())->Height))
#else
#define LINE(x) ((x) * (((sFONT *)lcd_low_GetFont())->Height))
#endif
/**
  * @}
  */

/** @defgroup BASIC_lcd_low_Exported_Types BASIC GUI Exported Types
  * @{
  */

/**
  * @brief  GUI Drawing main properties
  */
typedef struct
{
  uint32_t  TextColor; /*!< Specifies the color of text */
  uint32_t  BackColor; /*!< Specifies the background color below the text */
  sFONT    *pFont;     /*!< Specifies the font used for the text */
  uint32_t  GuiLayer;
  uint32_t  GuiDevice;
  uint32_t  GuiXsize;
  uint32_t  GuiYsize;
  uint32_t  GuiPixelFormat;
} lcd_low_Ctx_t;

/**
  * @brief  GUI Drawing point (pixel) geometric definition
  */
typedef struct
{
  int16_t X; /*!< geometric X position of drawing */
  int16_t Y; /*!< geometric Y position of drawing */
} Point;

/**
  * @brief  Pointer on GUI Drawing point (pixel) geometric definition
  */
typedef Point * pPoint;

/**
  * @brief  GUI drawing Line alignment mode definitions
  */
typedef enum
{
  CENTER_MODE             = 0x01,    /*!< Center mode */
  RIGHT_MODE              = 0x02,    /*!< Right mode  */
  LEFT_MODE               = 0x03     /*!< Left mode   */
} Text_AlignModeTypdef;

/**
  * @}
  */

/** @addtogroup BASIC_lcd_low_Exported_Functions
  * @{
  */
void     lcd_low_SetFuncDriver(const lcd_low_Drv_t *pDrv);

void     lcd_low_SetLayer(uint32_t Layer);
void     lcd_low_SetDevice(uint32_t Device);

void     lcd_low_SetTextColor(uint32_t Color);
uint32_t lcd_low_GetTextColor(void);
void     lcd_low_SetBackColor(uint32_t Color);
uint32_t lcd_low_GetBackColor(void);
void     lcd_low_SetFont(sFONT *fonts);
sFONT    *lcd_low_GetFont(void);

void     lcd_low_Clear(uint32_t Color);
void     lcd_low_ClearStringLine(uint32_t Line);
void     lcd_low_DisplayStringAtLine(uint32_t Line, uint8_t *ptr);
void     lcd_low_DisplayStringAt(uint32_t Xpos, uint32_t Ypos, uint8_t *Text, Text_AlignModeTypdef Mode);
void     lcd_low_DisplayChar(uint32_t Xpos, uint32_t Ypos, uint8_t Ascii);
void     lcd_low_GetPixel(uint16_t Xpos, uint16_t Ypos, uint32_t *Color);
void     lcd_low_SetPixel(uint16_t Xpos, uint16_t Ypos, uint32_t Color);
void     lcd_low_FillRGBRect(uint32_t Xpos, uint32_t Ypos, uint8_t *pData, uint32_t Width, uint32_t Height);
void     lcd_low_DrawHLine(uint32_t Xpos, uint32_t Ypos, uint32_t Length, uint32_t Color);
void     lcd_low_DrawVLine(uint32_t Xpos, uint32_t Ypos, uint32_t Length, uint32_t Color);
void     lcd_low_DrawBitmap(uint32_t Xpos, uint32_t Ypos, uint8_t *pData);
void     lcd_low_FillRect(uint32_t Xpos, uint32_t Ypos, uint32_t Width, uint32_t Height, uint32_t Color);
void     lcd_low_DrawLine(uint32_t Xpos1, uint32_t Ypos1, uint32_t Xpos2, uint32_t Ypos2, uint32_t Color);
void     lcd_low_DrawRect(uint32_t Xpos, uint32_t Ypos, uint32_t Width, uint32_t Height, uint32_t Color);
void     lcd_low_DrawCircle(uint32_t Xpos, uint32_t Ypos, uint32_t Radius, uint32_t Color);
void     lcd_low_DrawCircleG(uint32_t Xpos, uint32_t Ypos, uint32_t Radius, uint32_t Color);
void     lcd_low_DrawPolygon(pPoint Points, uint32_t PointCount, uint32_t Color);
void     lcd_low_DrawEllipse(int Xpos, int Ypos, int XRadius, int YRadius, uint32_t Color);
void     lcd_low_FillCircle(uint32_t Xpos, uint32_t Ypos, uint32_t Radius, uint32_t Color);
void     lcd_low_FillPolygon(pPoint Points, uint32_t PointCount, uint32_t Color);
void     lcd_low_FillEllipse(int Xpos, int Ypos, int XRadius, int YRadius, uint32_t Color);

#endif

#endif /* BASIC_lcd_low_H */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
