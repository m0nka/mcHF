/**
  ******************************************************************************
  * @file    basic_gui.c
  * @author  MCD Application Team
  * @brief   This file includes the basic functionalities to drive LCD
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

/* File Info: ------------------------------------------------------------------
                                   User NOTES
1. How To use this driver:
--------------------------
   - This driver is a generic driver that provides basic display services. It should
     be used by any platform once LCD is initialized and following draw methods are
     registered:
         BSP_LCD_DrawBitmap
         BSP_LCD_DrawHLine
         BSP_LCD_DrawVLine
         BSP_LCD_FillRect
         BSP_LCD_ReadPixel
         BSP_LCD_WritePixel
         BSP_LCD_GetXSize
         BSP_LCD_GetYSize
         BSP_LCD_SetActiveLayer

   - At application level, once the LCD is initialized, user should call lcd_low_SetFuncDriver()
     API to link board LCD drivers to BASIC GUI LCD drivers.
     User can then call the BASIC GUI services:
         lcd_low_SetFuncDriver()
         lcd_low_SetLayer()
         lcd_low_SetDevice()
         lcd_low_SetTextColor()
         lcd_low_GetTextColor()
         lcd_low_SetBackColor()
         lcd_low_GetBackColor()
         lcd_low_SetFont()
         lcd_low_GetFont()
         lcd_low_Clear)
         lcd_low_ClearStringLine()
         lcd_low_DisplayStringAtLine()
         lcd_low_DisplayStringAt()
         lcd_low_DisplayChar()
         lcd_low_GetPixel()
         lcd_low_SetPixel()
         lcd_low_FillRGBRect()
         lcd_low_DrawHLine()
         lcd_low_DrawVLine()
         lcd_low_DrawBitmap()
         lcd_low_FillRect()
         lcd_low_DrawLine()
         lcd_low_DrawRect()
         lcd_low_DrawCircle()
         lcd_low_DrawPolygon()
         lcd_low_DrawEllipse()
         lcd_low_FillCircle()
         lcd_low_FillPolygon()
         lcd_low_FillEllipse()
------------------------------------------------------------------------------*/
#include "mchf_pro_board.h"
#include "main.h"

/* Includes ------------------------------------------------------------------*/
#include "lcd_low.h"
#include "lcd_high.h"
//#include "font24.c"
//#include "font20.c"
//#include "font16.c"
//#include "font12.c"
//#include "font8.c"

#ifndef lcd_low_MAX_LAYERS_NBR
  #define lcd_low_MAX_LAYERS_NBR    2U
#endif

/** @defgroup BASIC_lcd_low_Private_Macros BASIC GUI Private Macros
  * @{
  */
#define ABS(X)                 ((X) > 0 ? (X) : -(X))
#define POLY_X(Z)              ((int32_t)((Points + (Z))->X))
#define POLY_Y(Z)              ((int32_t)((Points + (Z))->Y))

#define CONVERTARGB88882RGB565(Color)((((Color & 0xFFU) >> 3) & 0x1FU) |\
                                     (((((Color & 0xFF00U) >> 8) >>2) & 0x3FU) << 5) |\
                                     (((((Color & 0xFF0000U) >> 16) >>3) & 0x1FU) << 11))

#define CONVERTRGB5652ARGB8888(Color)(((((((Color >> 11) & 0x1FU) * 527) + 23) >> 6) << 16) |\
                                     ((((((Color >> 5) & 0x3FU) * 259) + 33) >> 6) << 8) |\
                                     ((((Color & 0x1FU) * 527) + 23) >> 6) | 0xFF000000)

/**
  * @}
  */

/** @defgroup BASIC_lcd_low_Private_Types BASIC GUI Private Types
  * @{
  */
typedef struct
{
  uint32_t x1;
  uint32_t y1;
  uint32_t x2;
  uint32_t y2;
  uint32_t x3;
  uint32_t y3;
}Triangle_Positions_t;

/**
  * @}
  */

/** @defgroup BASIC_lcd_low_Private_Variables BASIC GUI Private Variables
  * @{
  */

/**
  * @brief  Current Drawing Layer properties variable
  */
static lcd_low_Ctx_t DrawProp[lcd_low_MAX_LAYERS_NBR];
static lcd_low_Drv_t FuncDriver;

/**
  * @}
  */

/** @defgroup BASIC_lcd_low_Private_FunctionPrototypes BASIC GUI Private FunctionPrototypes
  * @{
  */
static void DrawChar(uint32_t Xpos, uint32_t Ypos, const uint8_t *pData);
static void FillTriangle(Triangle_Positions_t *Positions, uint32_t Color);
/**
  * @}
  */

/** @defgroup BASIC_lcd_low_Exported_Functions BASIC GUI Exported Functions
  * @{
  */

/**
  * @brief  Link board LCD drivers to BASIC GUI LCD drivers
  * @param  pDrv Structure of LCD functions
  */
void lcd_low_SetFuncDriver(const lcd_low_Drv_t *pDrv)
{
  FuncDriver.DrawBitmap     = pDrv->DrawBitmap;
  FuncDriver.FillRGBRect    = pDrv->FillRGBRect;
  FuncDriver.DrawHLine      = pDrv->DrawHLine;
  FuncDriver.DrawVLine      = pDrv->DrawVLine;
  FuncDriver.FillRect       = pDrv->FillRect;
  FuncDriver.GetPixel       = pDrv->GetPixel;
  FuncDriver.SetPixel       = pDrv->SetPixel;
  FuncDriver.GetXSize       = pDrv->GetXSize;
  FuncDriver.GetYSize       = pDrv->GetYSize;
  FuncDriver.SetLayer       = pDrv->SetLayer;
  FuncDriver.GetFormat      = pDrv->GetFormat;

  DrawProp->GuiLayer 		= 0;
  DrawProp->GuiDevice 		= 0;

  FuncDriver.GetXSize(0, &DrawProp->GuiXsize);
  FuncDriver.GetYSize(0, &DrawProp->GuiYsize);
  FuncDriver.GetFormat(0, &DrawProp->GuiPixelFormat);
}

/**
  * @brief  Set the LCD layer.
  * @param  Layer  LCD layer
  */
void lcd_low_SetLayer(uint32_t Layer)
{
  if(FuncDriver.SetLayer != NULL)
  {
    if(FuncDriver.SetLayer(DrawProp->GuiDevice, Layer) == 0)
    {
      DrawProp->GuiLayer = Layer;
    }
  }
}

/**
  * @brief  Set the LCD instance to be used.
  * @param  Device  LCD instance
  */
void lcd_low_SetDevice(uint32_t Device)
{
  DrawProp->GuiDevice = Device;
  FuncDriver.GetXSize(Device, &DrawProp->GuiXsize);
  FuncDriver.GetYSize(Device, &DrawProp->GuiYsize);
}

/**
  * @brief  Sets the LCD text color.
  * @param  Color  Text color code
  */
void lcd_low_SetTextColor(uint32_t Color)
{
  DrawProp[DrawProp->GuiLayer].TextColor = Color;
}

/**
  * @brief  Gets the LCD text color.
  * @retval Used text color.
  */
uint32_t lcd_low_GetTextColor(void)
{
  return DrawProp[DrawProp->GuiLayer].TextColor;
}

/**
  * @brief  Sets the LCD background color.
  * @param  Color  Layer background color code
  */
void lcd_low_SetBackColor(uint32_t Color)
{
  DrawProp[DrawProp->GuiLayer].BackColor = Color;
}

/**
  * @brief  Gets the LCD background color.
  * @retval Used background color
  */
uint32_t lcd_low_GetBackColor(void)
{
  return DrawProp[DrawProp->GuiLayer].BackColor;
}

/**
  * @brief  Sets the LCD text font.
  * @param  fonts  Layer font to be used
  */
void lcd_low_SetFont(sFONT *fonts)
{
  DrawProp[DrawProp->GuiLayer].pFont = fonts;
}

/**
  * @brief  Gets the LCD text font.
  * @retval Used layer font
  */
sFONT *lcd_low_GetFont(void)
{
  return DrawProp[DrawProp->GuiLayer].pFont;
}

/**
  * @brief  Draws a RGB rectangle in currently active layer.
  * @param  pData   Pointer to RGB rectangle data  
  * @param  Xpos    X position
  * @param  Ypos    Y position
  * @param  Length  Line length
  */
void lcd_low_FillRGBRect(uint32_t Xpos, uint32_t Ypos, uint8_t *pData, uint32_t Width, uint32_t Height)
{
  /* Write RGB rectangle data */  
  FuncDriver.FillRGBRect(DrawProp->GuiDevice, Xpos, Ypos, pData, Width, Height);
}

/**
  * @brief  Draws an horizontal line in currently active layer.
  * @param  Xpos    X position
  * @param  Ypos    Y position
  * @param  Length  Line length
  * @param  Color   Draw color
  */
void lcd_low_DrawHLine(uint32_t Xpos, uint32_t Ypos, uint32_t Length, uint32_t Color)
{
  /* Write line */
  if(DrawProp->GuiPixelFormat == LCD_PIXEL_FORMAT_RGB565)
  {
    FuncDriver.DrawHLine(DrawProp->GuiDevice, Xpos, Ypos, Length, CONVERTARGB88882RGB565(Color));
  }
  else
  {
    FuncDriver.DrawHLine(DrawProp->GuiDevice, Xpos, Ypos, Length, Color);
  }
}

/**
  * @brief  Draws a vertical line in currently active layer.
  * @param  Xpos    X position
  * @param  Ypos    Y position
  * @param  Length  Line length
  * @param  Color   Draw color
  */
void lcd_low_DrawVLine(uint32_t Xpos, uint32_t Ypos, uint32_t Length, uint32_t Color)
{
  /* Write line */
  if(DrawProp->GuiPixelFormat == LCD_PIXEL_FORMAT_RGB565)
  {
    FuncDriver.DrawVLine(DrawProp->GuiDevice, Xpos, Ypos, Length, CONVERTARGB88882RGB565(Color));
  }
  else
  {
    FuncDriver.DrawVLine(DrawProp->GuiDevice, Xpos, Ypos, Length, Color);
  }
}

/**
  * @brief  Read an LCD pixel.
  * @param  Xpos     X position
  * @param  Ypos     Y position
  * @retval Color    pixel color
  */
void lcd_low_GetPixel(uint16_t Xpos, uint16_t Ypos, uint32_t *Color)
{
  /* Get Pixel */
  FuncDriver.GetPixel(DrawProp->GuiDevice, Xpos, Ypos, Color);
  if(DrawProp->GuiPixelFormat == LCD_PIXEL_FORMAT_RGB565)
  {
    *Color = CONVERTRGB5652ARGB8888(*Color);
  }
}

/**
  * @brief  Draws a pixel on LCD.
  * @param  Xpos     X position
  * @param  Ypos     Y position
  * @param  Color    Pixel color
  */
void lcd_low_SetPixel(uint16_t Xpos, uint16_t Ypos, uint32_t Color)
{
  /* Set Pixel */
  if(DrawProp->GuiPixelFormat == LCD_PIXEL_FORMAT_RGB565)
  {
    FuncDriver.SetPixel(DrawProp->GuiDevice, Xpos, Ypos, CONVERTARGB88882RGB565(Color));
  }
  else
  {
    FuncDriver.SetPixel(DrawProp->GuiDevice, Xpos, Ypos, Color);
  }
}

/**
  * @brief  Clears the whole currently active layer of LTDC.
  * @param  Color  Color of the background
  */
void lcd_low_Clear(uint32_t Color)
{
  /* Clear the LCD */
  lcd_low_FillRect(0, 0, DrawProp->GuiXsize, DrawProp->GuiYsize, Color);
}

/**
  * @brief  Clears the selected line in currently active layer.
  * @param  Line  Line to be cleared
  */
void lcd_low_ClearStringLine(uint32_t Line)
{
  /* Draw rectangle with background color */
  lcd_low_FillRect(0, (Line * DrawProp[DrawProp->GuiLayer].pFont->Height), DrawProp->GuiXsize, DrawProp[DrawProp->GuiLayer].pFont->Height, DrawProp[DrawProp->GuiLayer].BackColor);
}

/**
  * @brief  Displays one character in currently active layer.
  * @param  Xpos Start column address
  * @param  Ypos Line where to display the character shape.
  * @param  Ascii Character ascii code
  *           This parameter must be a number between Min_Data = 0x20 and Max_Data = 0x7E
  */
void lcd_low_DisplayChar(uint32_t Xpos, uint32_t Ypos, uint8_t Ascii)
{
  DrawChar(Xpos, Ypos, &DrawProp[DrawProp->GuiLayer].pFont->table[(Ascii-' ') *\
  DrawProp[DrawProp->GuiLayer].pFont->Height * ((DrawProp[DrawProp->GuiLayer].pFont->Width + 7) / 8)]);
}

/**
  * @brief  Displays characters in currently active layer.
  * @param  Xpos X position (in pixel)
  * @param  Ypos Y position (in pixel)
  * @param  Text Pointer to string to display on LCD
  * @param  Mode Display mode
  *          This parameter can be one of the following values:
  *            @arg  CENTER_MODE
  *            @arg  RIGHT_MODE
  *            @arg  LEFT_MODE
  */
#ifndef BASIC_lcd_low_LANDSCAPE_565_MODE
void lcd_low_DisplayStringAt(uint32_t Xpos, uint32_t Ypos, uint8_t *Text, Text_AlignModeTypdef Mode)
{
  uint32_t refcolumn = 1, i = 0;
  uint32_t size = 0, xsize = 0;
  uint8_t  *ptr = Text;

  /* Get the text size */
  while (*ptr++) size ++ ;

  /* Characters number per line */
  xsize = (DrawProp->GuiXsize/DrawProp[DrawProp->GuiLayer].pFont->Width);

  switch (Mode)
  {
  case CENTER_MODE:
    {
      refcolumn = Xpos + ((xsize - size)* DrawProp[DrawProp->GuiLayer].pFont->Width) / 2;
      break;
    }
  case LEFT_MODE:
    {
      refcolumn = Xpos;
      break;
    }
  case RIGHT_MODE:
    {
      refcolumn = - Xpos + ((xsize - size)*DrawProp[DrawProp->GuiLayer].pFont->Width);
      break;
    }
  default:
    {
      refcolumn = Xpos;
      break;
    }
  }

  /* Check that the Start column is located in the screen */
  if ((refcolumn < 1) || (refcolumn >= 0x8000))
  {
    refcolumn = 1;
  }

  /* Send the string character by character on LCD */
  while ((*Text != 0) & (((DrawProp->GuiXsize - (i*DrawProp[DrawProp->GuiLayer].pFont->Width)) & 0xFFFF) >= DrawProp[DrawProp->GuiLayer].pFont->Width))
  {
    /* Display one character on LCD */
    lcd_low_DisplayChar(refcolumn, Ypos, *Text);
    /* Decrement the column position by 16 */
    refcolumn += DrawProp[DrawProp->GuiLayer].pFont->Width;

    /* Point on the next character */
    Text++;
    i++;
  }
}
#else
void lcd_low_DisplayStringAt(uint32_t Xpos, uint32_t Ypos, uint8_t *Text, Text_AlignModeTypdef Mode)
{
  uint32_t refcolumn = 1, i = 0;
  uint32_t size = 0, xsize = 0;
  uint8_t  *ptr = Text;

  /* Get the text size */
  while (*ptr++) size ++ ;

  /* Characters number per line */
  xsize = (DrawProp->GuiYsize/DrawProp[DrawProp->GuiLayer].pFont->Width);

  switch (Mode)
  {
  /*
  case CENTER_MODE:
    {
      refcolumn = Ypos + ((xsize - size)* DrawProp[DrawProp->GuiLayer].pFont->Width) / 2;
      break;
    }*/
  case LEFT_MODE:
    {
      refcolumn = Ypos;
      break;
    }
    /*
  case RIGHT_MODE:
    {
      refcolumn = (Ypos - 480) + ((xsize - size)*DrawProp[DrawProp->GuiLayer].pFont->Width);
      break;
    }*/
  default:
    {
      refcolumn = Ypos;
      break;
    }
  }

  /* Check that the Start column is located in the screen */
  if ((refcolumn < 1) || (refcolumn >= 0x8000))
  {
    refcolumn = 1;
  }

  /* Send the string character by character on LCD */
  while ((*Text != 0) & (((DrawProp->GuiXsize - (i*DrawProp[DrawProp->GuiLayer].pFont->Width)) & 0xFFFF) >= DrawProp[DrawProp->GuiLayer].pFont->Width))
  {
    /* Display one character on LCD */
    lcd_low_DisplayChar(Xpos, refcolumn, *Text);

    /* Decrement the column position by 16 */
    refcolumn += DrawProp[DrawProp->GuiLayer].pFont->Width;

    /* Point on the next character */
    Text++;
    i++;
  }
}
#endif

/**
  * @brief  Displays a maximum of 60 characters on the LCD.
  * @param  Line: Line where to display the character shape
  * @param  ptr: Pointer to string to display on LCD
  */
void lcd_low_DisplayStringAtLine(uint32_t Line, uint8_t *ptr)
{
  lcd_low_DisplayStringAt(0, LINE(Line), ptr, LEFT_MODE);
}

/**
  * @brief  Draws an uni-line (between two points) in currently active layer.
  * @param  Xpos1 Point 1 X position
  * @param  Ypos1 Point 1 Y position
  * @param  Xpos2 Point 2 X position
  * @param  Ypos2 Point 2 Y position
  * @param  Color Draw color
  */
void lcd_low_DrawLine(uint32_t Xpos1, uint32_t Ypos1, uint32_t Xpos2, uint32_t Ypos2, uint32_t Color)
{
  int16_t deltax = 0, deltay = 0, x = 0, y = 0, xinc1 = 0, xinc2 = 0,
  yinc1 = 0, yinc2 = 0, den = 0, num = 0, numadd = 0, numpixels = 0,
  curpixel = 0;
  int32_t x_diff, y_diff;

  x_diff = Xpos2 - Xpos1;
  y_diff = Ypos2 - Ypos1;

  deltax = ABS(x_diff);         /* The absolute difference between the x's */
  deltay = ABS(y_diff);         /* The absolute difference between the y's */
  x = Xpos1;                       /* Start x off at the first pixel */
  y = Ypos1;                       /* Start y off at the first pixel */

  if (Xpos2 >= Xpos1)                 /* The x-values are increasing */
  {
    xinc1 = 1;
    xinc2 = 1;
  }
  else                          /* The x-values are decreasing */
  {
    xinc1 = -1;
    xinc2 = -1;
  }

  if (Ypos2 >= Ypos1)                 /* The y-values are increasing */
  {
    yinc1 = 1;
    yinc2 = 1;
  }
  else                          /* The y-values are decreasing */
  {
    yinc1 = -1;
    yinc2 = -1;
  }

  if (deltax >= deltay)         /* There is at least one x-value for every y-value */
  {
    xinc1 = 0;                  /* Don't change the x when numerator >= denominator */
    yinc2 = 0;                  /* Don't change the y for every iteration */
    den = deltax;
    num = deltax / 2;
    numadd = deltay;
    numpixels = deltax;         /* There are more x-values than y-values */
  }
  else                          /* There is at least one y-value for every x-value */
  {
    xinc2 = 0;                  /* Don't change the x for every iteration */
    yinc1 = 0;                  /* Don't change the y when numerator >= denominator */
    den = deltay;
    num = deltay / 2;
    numadd = deltax;
    numpixels = deltay;         /* There are more y-values than x-values */
  }

  for (curpixel = 0; curpixel <= numpixels; curpixel++)
  {
    lcd_low_SetPixel(x, y, Color);   /* Draw the current pixel */
    num += numadd;                            /* Increase the numerator by the top of the fraction */
    if (num >= den)                           /* Check if numerator >= denominator */
    {
      num -= den;                             /* Calculate the new numerator value */
      x += xinc1;                             /* Change the x as appropriate */
      y += yinc1;                             /* Change the y as appropriate */
    }
    x += xinc2;                               /* Change the x as appropriate */
    y += yinc2;                               /* Change the y as appropriate */
  }
}

/**
  * @brief  Draws a rectangle in currently active layer.
  * @param  Xpos X position
  * @param  Ypos Y position
  * @param  Width  Rectangle width
  * @param  Height Rectangle height
  * @param  Color  Draw color
  */
void lcd_low_DrawRect(uint32_t Xpos, uint32_t Ypos, uint32_t Width, uint32_t Height, uint32_t Color)
{
  /* Draw horizontal lines */
  lcd_low_DrawHLine(Xpos, Ypos, Width, Color);
  lcd_low_DrawHLine(Xpos, (Ypos+ Height - 1U), Width, Color);

  /* Draw vertical lines */
  lcd_low_DrawVLine(Xpos, Ypos, Height, Color);
  lcd_low_DrawVLine((Xpos + Width - 1U), Ypos, Height, Color);
}

/**
  * @brief  Draws a circle in currently active layer.
  * @param  Xpos    X position
  * @param  Ypos    Y position
  * @param  Radius  Circle radius
  * @param  Color   Draw color
  */
void lcd_low_DrawCircle(uint32_t Xpos, uint32_t Ypos, uint32_t Radius, uint32_t Color)
{
  int32_t   decision;  /* Decision Variable */
  uint32_t  current_x; /* Current X Value */
  uint32_t  current_y; /* Current Y Value */

  decision = 3 - (Radius << 1);
  current_x = 0;
  current_y = Radius;

  while (current_x <= current_y)
  {
    if((Ypos - current_y) < DrawProp->GuiYsize)
    {
      if((Xpos + current_x) < DrawProp->GuiXsize)
      {
        lcd_low_SetPixel((Xpos + current_x), (Ypos - current_y), Color);
      }
      if((Xpos - current_x) < DrawProp->GuiXsize)
      {
        lcd_low_SetPixel((Xpos - current_x), (Ypos - current_y), Color);
      }
    }

    if((Ypos - current_x) < DrawProp->GuiYsize)
    {
      if((Xpos + current_y) < DrawProp->GuiXsize)
      {
        lcd_low_SetPixel((Xpos + current_y), (Ypos - current_x), Color);
      }
      if((Xpos - current_y) < DrawProp->GuiXsize)
      {
        lcd_low_SetPixel((Xpos - current_y), (Ypos - current_x), Color);
      }
    }

    if((Ypos + current_y) < DrawProp->GuiYsize)
    {
      if((Xpos + current_x) < DrawProp->GuiXsize)
      {
        lcd_low_SetPixel((Xpos + current_x), (Ypos + current_y), Color);
      }
      if((Xpos - current_x) < DrawProp->GuiXsize)
      {
        lcd_low_SetPixel((Xpos - current_x), (Ypos + current_y), Color);
      }
    }

    if((Ypos + current_x) < DrawProp->GuiYsize)
    {
      if((Xpos + current_y) < DrawProp->GuiXsize)
      {
        lcd_low_SetPixel((Xpos + current_y), (Ypos + current_x), Color);
      }
      if((Xpos - current_y) < DrawProp->GuiXsize)
      {
        lcd_low_SetPixel((Xpos - current_y), (Ypos + current_x), Color);
      }
    }

    if (decision < 0)
    {
      decision += (current_x << 2) + 6;
    }
    else
    {
      decision += ((current_x - current_y) << 2) + 10;
      current_y--;
    }
    current_x++;
  }
}

void lcd_low_DrawCircleG(uint32_t Xpos, uint32_t Ypos, uint32_t Radius, uint32_t c)
{
  int32_t   decision;  /* Decision Variable */
  uint32_t  current_x; /* Current X Value */
  uint32_t  current_y; /* Current Y Value */
  uint32_t Color = c;

  decision = 3 - (Radius << 1);
  current_x = 0;
  current_y = Radius;

  while (current_x <= current_y)
  {
	  Color -= 0x050505;

    if((Ypos - current_y) < DrawProp->GuiYsize)
    {
      if((Xpos + current_x) < DrawProp->GuiXsize)
      {
        lcd_low_SetPixel((Xpos + current_x), (Ypos - current_y), Color);
      }
      if((Xpos - current_x) < DrawProp->GuiXsize)
      {
        lcd_low_SetPixel((Xpos - current_x), (Ypos - current_y), Color);
      }
    }

    if((Ypos - current_x) < DrawProp->GuiYsize)
    {
      if((Xpos + current_y) < DrawProp->GuiXsize)
      {
        lcd_low_SetPixel((Xpos + current_y), (Ypos - current_x), Color);
      }
      if((Xpos - current_y) < DrawProp->GuiXsize)
      {
        lcd_low_SetPixel((Xpos - current_y), (Ypos - current_x), Color);
      }
    }

    if((Ypos + current_y) < DrawProp->GuiYsize)
    {
      if((Xpos + current_x) < DrawProp->GuiXsize)
      {
        lcd_low_SetPixel((Xpos + current_x), (Ypos + current_y), Color);
      }
      if((Xpos - current_x) < DrawProp->GuiXsize)
      {
        lcd_low_SetPixel((Xpos - current_x), (Ypos + current_y), Color);
      }
    }

    if((Ypos + current_x) < DrawProp->GuiYsize)
    {
      if((Xpos + current_y) < DrawProp->GuiXsize)
      {
        lcd_low_SetPixel((Xpos + current_y), (Ypos + current_x), Color);
      }
      if((Xpos - current_y) < DrawProp->GuiXsize)
      {
        lcd_low_SetPixel((Xpos - current_y), (Ypos + current_x), Color);
      }
    }

    if (decision < 0)
    {
      decision += (current_x << 2) + 6;
    }
    else
    {
      decision += ((current_x - current_y) << 2) + 10;
      current_y--;
    }
    current_x++;
  }
}

/**
  * @brief  Draws an poly-line (between many points) in currently active layer.
  * @param  Points      Pointer to the points array
  * @param  PointCount  Number of points
  * @param  Color       Draw color
  */
void lcd_low_DrawPolygon(pPoint Points, uint32_t PointCount, uint32_t Color)
{
  int16_t x_pos = 0, y_pos = 0;

  if(PointCount < 2)
  {
    return;
  }

  lcd_low_DrawLine(Points->X, Points->Y, (Points+PointCount-1)->X, (Points+PointCount-1)->Y, Color);

  while(--PointCount)
  {
    x_pos = Points->X;
    y_pos = Points->Y;
    Points++;
    lcd_low_DrawLine(x_pos, y_pos, Points->X, Points->Y, Color);
  }
}

/**
  * @brief  Draws an ellipse on LCD in currently active layer.
  * @param  Xpos    X position
  * @param  Ypos    Y position
  * @param  XRadius Ellipse X radius
  * @param  YRadius Ellipse Y radius
  * @param  Color   Draw color
  */
void lcd_low_DrawEllipse(int Xpos, int Ypos, int XRadius, int YRadius, uint32_t Color)
{
  int x_pos = 0, y_pos = -YRadius, err = 2-2*XRadius, e2;
  float k = 0, rad1 = 0, rad2 = 0;

  rad1 = XRadius;
  rad2 = YRadius;

  k = (float)(rad2/rad1);

  do
  {
    lcd_low_SetPixel((Xpos-(uint32_t)(x_pos/k)), (Ypos + y_pos), Color);
    lcd_low_SetPixel((Xpos+(uint32_t)(x_pos/k)), (Ypos + y_pos), Color);
    lcd_low_SetPixel((Xpos+(uint32_t)(x_pos/k)), (Ypos - y_pos), Color);
    lcd_low_SetPixel((Xpos-(uint32_t)(x_pos/k)), (Ypos - y_pos), Color);

    e2 = err;
    if (e2 <= x_pos)
    {
      err += ++x_pos*2+1;
      if (-y_pos == x_pos && e2 <= y_pos) e2 = 0;
    }
    if (e2 > y_pos)
    {
      err += ++y_pos*2+1;
    }
  }while (y_pos <= 0);
}

/**
  * @brief  Draws a bitmap picture loaded in the internal Flash (32 bpp) in currently active layer.
  * @param  Xpos  Bmp X position in the LCD
  * @param  Ypos  Bmp Y position in the LCD
  * @param  pData Pointer to Bmp picture address in the internal Flash
  */
void lcd_low_DrawBitmap(uint32_t Xpos, uint32_t Ypos, uint8_t *pData)
{
  FuncDriver.DrawBitmap(DrawProp->GuiDevice, Xpos, Ypos, pData);
}

/**
  * @brief  Draws a full rectangle in currently active layer.
  * @param  Xpos   X position
  * @param  Ypos   Y position
  * @param  Width  Rectangle width
  * @param  Height Rectangle height
  * @param  Color  Draw color
  */
void lcd_low_FillRect(uint32_t Xpos, uint32_t Ypos, uint32_t Width, uint32_t Height, uint32_t Color)
{
  /* Fill the rectangle */
  if(DrawProp->GuiPixelFormat == LCD_PIXEL_FORMAT_RGB565)
  {
    FuncDriver.FillRect(DrawProp->GuiDevice, Xpos, Ypos, Width, Height, CONVERTARGB88882RGB565(Color));
  }
  else
  {
    FuncDriver.FillRect(DrawProp->GuiDevice, Xpos, Ypos, Width, Height, Color);
  }
}

/**
  * @brief  Draws a full circle in currently active layer.
  * @param  Xpos   X position
  * @param  Ypos   Y position
  * @param  Radius Circle radius
  * @param  Color  Draw color
  */
void lcd_low_FillCircle(uint32_t Xpos, uint32_t Ypos, uint32_t Radius, uint32_t Color)
{
  int32_t   decision;  /* Decision Variable */
  uint32_t  current_x; /* Current X Value */
  uint32_t  current_y; /* Current Y Value */

  decision = 3 - (Radius << 1);

  current_x = 0;
  current_y = Radius;

  while (current_x <= current_y)
  {
    if(current_y > 0)
    {
      if(current_y >= Xpos)
      {
        lcd_low_DrawHLine(0, Ypos + current_x, 2*current_y - (current_y - Xpos), Color);
        lcd_low_DrawHLine(0, Ypos - current_x, 2*current_y - (current_y - Xpos), Color);
      }
      else
      {
        lcd_low_DrawHLine(Xpos - current_y, Ypos + current_x, 2*current_y, Color);
        lcd_low_DrawHLine(Xpos - current_y, Ypos - current_x, 2*current_y, Color);
      }
    }

    if(current_x > 0)
    {
      if(current_x >= Xpos)
      {
        lcd_low_DrawHLine(0, Ypos - current_y, 2*current_x - (current_x - Xpos), Color);
        lcd_low_DrawHLine(0, Ypos + current_y, 2*current_x - (current_x - Xpos), Color);
      }
      else
      {
        lcd_low_DrawHLine(Xpos - current_x, Ypos - current_y, 2*current_x, Color);
        lcd_low_DrawHLine(Xpos - current_x, Ypos + current_y, 2*current_x, Color);
      }
    }
    if (decision < 0)
    {
      decision += (current_x << 2) + 6;
    }
    else
    {
      decision += ((current_x - current_y) << 2) + 10;
      current_y--;
    }
    current_x++;
  }

  lcd_low_DrawCircle(Xpos, Ypos, Radius, Color);
}

/**
  * @brief  Draws a full poly-line (between many points) in currently active layer.
  * @param  Points     Pointer to the points array
  * @param  PointCount Number of points
  * @param  Color      Draw color
  */
void lcd_low_FillPolygon(pPoint Points, uint32_t PointCount, uint32_t Color)
{
  int16_t X = 0, Y = 0, X2 = 0, Y2 = 0, x_center = 0, y_center = 0, x_first = 0, y_first = 0, pixel_x = 0, pixel_y = 0, counter = 0;
  uint32_t  image_left = 0, image_right = 0, image_top = 0, image_bottom = 0;
  Triangle_Positions_t positions;

  image_left = image_right = Points->X;
  image_top= image_bottom = Points->Y;

  for(counter = 1; counter < PointCount; counter++)
  {
    pixel_x = POLY_X(counter);
    if(pixel_x < image_left)
    {
      image_left = pixel_x;
    }
    if(pixel_x > image_right)
    {
      image_right = pixel_x;
    }

    pixel_y = POLY_Y(counter);
    if(pixel_y < image_top)
    {
      image_top = pixel_y;
    }
    if(pixel_y > image_bottom)
    {
      image_bottom = pixel_y;
    }
  }

  if(PointCount < 2)
  {
    return;
  }

  x_center = (image_left + image_right)/2;
  y_center = (image_bottom + image_top)/2;

  x_first = Points->X;
  y_first = Points->Y;

  while(--PointCount)
  {
    X = Points->X;
    Y = Points->Y;
    Points++;
    X2 = Points->X;
    Y2 = Points->Y;
    positions.x1 = X;
    positions.y1 = Y;
    positions.x2 = X2;
    positions.y2 = Y2;
    positions.x3 = x_center;
    positions.y3 = y_center;
    FillTriangle(&positions, Color);

    positions.x2 = x_center;
    positions.y2 = y_center;
    positions.x3 = X2;
    positions.y3 = Y2;
    FillTriangle(&positions, Color);

    positions.x1 = x_center;
    positions.y1 = y_center;
    positions.x2 = X2;
    positions.y2 = Y2;
    positions.x3 = X;
    positions.y3 = Y;
    FillTriangle(&positions, Color);
  }

    positions.x1 = x_first;
    positions.y1 = y_first;
    positions.x2 = X2;
    positions.y2 = Y2;
    positions.x3 = x_center;
    positions.y3 = y_center;
    FillTriangle(&positions, Color);

    positions.x2 = x_center;
    positions.y2 = y_center;
    positions.x3 = X2;
    positions.y3 = Y2;
    FillTriangle(&positions, Color);

    positions.x1 = x_center;
    positions.y1 = y_center;
    positions.x2 = X2;
    positions.y2 = Y2;
    positions.x3 = x_first;
    positions.y3 = y_first;
    FillTriangle(&positions, Color);
}

/**
  * @brief  Draws a full ellipse in currently active layer.
  * @param  Xpos    X position
  * @param  Ypos    Y position
  * @param  XRadius Ellipse X radius
  * @param  YRadius Ellipse Y radius
  * @param  Color   Draw color
  */
void lcd_low_FillEllipse(int Xpos, int Ypos, int XRadius, int YRadius, uint32_t Color)
{
  int x_pos = 0, y_pos = -YRadius, err = 2-2*XRadius, e2;
  float k = 0, rad1 = 0, rad2 = 0;

  rad1 = XRadius;
  rad2 = YRadius;

  k = (float)(rad2/rad1);

  do
  {
    lcd_low_DrawHLine((Xpos-(uint32_t)(x_pos/k)), (Ypos + y_pos), (2*(uint32_t)(x_pos/k) + 1), Color);
    lcd_low_DrawHLine((Xpos-(uint32_t)(x_pos/k)), (Ypos - y_pos), (2*(uint32_t)(x_pos/k) + 1), Color);

    e2 = err;
    if (e2 <= x_pos)
    {
      err += ++x_pos*2+1;
      if (-y_pos == x_pos && e2 <= y_pos) e2 = 0;
    }
    if (e2 > y_pos) err += ++y_pos*2+1;
  }
  while (y_pos <= 0);
}

/**
  * @brief  Draws a character on LCD.
  * @param  Xpos  Line where to display the character shape
  * @param  Ypos  Start column address
  * @param  pData Pointer to the character data
  */
#ifndef BASIC_lcd_low_LANDSCAPE_565_MODE
static void DrawChar(uint32_t Xpos, uint32_t Ypos, const uint8_t *pData)
{
  uint32_t i = 0, j = 0, offset;
  uint32_t height, width;
  uint8_t  *pchar;
  uint32_t line;

  height = DrawProp[DrawProp->GuiLayer].pFont->Height;
  width  = DrawProp[DrawProp->GuiLayer].pFont->Width;
  uint16_t rgb565[24];
  uint32_t argb8888[24];

  offset =  8 *((width + 7)/8) -  width ;

  for(i = 0; i < height; i++)
  {
    pchar = ((uint8_t *)pData + (width + 7)/8 * i);

    switch(((width + 7)/8))
    {

    case 1:
      line =  pchar[0];
      break;

    case 2:
      line =  (pchar[0]<< 8) | pchar[1];
      break;

    case 3:
    default:
      line =  (pchar[0]<< 16) | (pchar[1]<< 8) | pchar[2];
      break;
    }

    if(DrawProp[DrawProp->GuiLayer].GuiPixelFormat == LCD_PIXEL_FORMAT_RGB565)
    {
      for (j = 0; j < width; j++)
      {
        if(line & (1 << (width- j + offset- 1)))
        {
          rgb565[j] = CONVERTARGB88882RGB565(DrawProp[DrawProp->GuiLayer].TextColor);
        }
        else
        {
          rgb565[j] = CONVERTARGB88882RGB565(DrawProp[DrawProp->GuiLayer].BackColor);
        }
      }
      lcd_low_FillRGBRect(Xpos,  Ypos++, (uint8_t*)&rgb565[0], width, 1);
    }
    else
    {
      for (j = 0; j < width; j++)
      {
        if(line & (1 << (width- j + offset- 1)))
        {
          argb8888[j] = DrawProp[DrawProp->GuiLayer].TextColor;
        }
        else
        {
          argb8888[j] = DrawProp[DrawProp->GuiLayer].BackColor;
        }
      }
      lcd_low_FillRGBRect(Xpos,  Ypos++, (uint8_t*)&argb8888[0], width, 1);
    }
  }
}
#else
static void DrawChar(uint32_t Xpos, uint32_t Ypos, const uint8_t *pData)
{
	uint32_t i = 0, j = 0, offset;
	uint32_t height, width;
	uint8_t  *pchar;
	uint32_t line;
	uint16_t rgb565[24];

	height = DrawProp[DrawProp->GuiLayer].pFont->Height;
	width  = DrawProp[DrawProp->GuiLayer].pFont->Width;
	offset = 8*((width + 7)/8) - width;
	Xpos   = 479 - Xpos;
	Ypos  += width;

	for(i = 0; i < height; i++)
	{
		pchar = ((uint8_t *)pData + (width + 7)/8 * i);

		switch(((width + 7)/8))
		{
    		case 1:
    			line =  pchar[0];
    			break;
    		case 2:
    			line =  (pchar[0]<< 8) | pchar[1];
    			break;
    		default:
    			line =  (pchar[0]<< 16) | (pchar[1]<< 8) | pchar[2];
    			break;
		}

		for(j = 0; j < width; j++)
		{
			if(line & (1 << (width- j + offset- 1)))
				rgb565[j] = CONVERTARGB88882RGB565(DrawProp[DrawProp->GuiLayer].TextColor);
			else
				rgb565[j] = CONVERTARGB88882RGB565(DrawProp[DrawProp->GuiLayer].BackColor);

		}
		//printf("x=%d, y=%d\r\n", Xpos, Ypos);
		lcd_low_FillRGBRect(Xpos--, Ypos, (uint8_t*)&rgb565[0], 1, width);
    }
}
#endif

/**
  * @brief  Fills a triangle (between 3 points).
  * @param  Positions  pointer to riangle coordinates
  * @param  Color      Draw color
  */
static void FillTriangle(Triangle_Positions_t *Positions, uint32_t Color)
{
  int16_t deltax = 0, deltay = 0, x = 0, y = 0, xinc1 = 0, xinc2 = 0,
  yinc1 = 0, yinc2 = 0, den = 0, num = 0, numadd = 0, numpixels = 0,
  curpixel = 0;
  int32_t x_diff, y_diff;

  x_diff = Positions->x2 - Positions->x1;
  y_diff = Positions->y2 - Positions->y1;

  deltax = ABS(x_diff);         /* The absolute difference between the x's */
  deltay = ABS(y_diff);         /* The absolute difference between the y's */
  x = Positions->x1;                       /* Start x off at the first pixel */
  y = Positions->y1;                       /* Start y off at the first pixel */

  if (Positions->x2 >= Positions->x1)                 /* The x-values are increasing */
  {
    xinc1 = 1;
    xinc2 = 1;
  }
  else                          /* The x-values are decreasing */
  {
    xinc1 = -1;
    xinc2 = -1;
  }

  if (Positions->y2 >= Positions->y1)                 /* The y-values are increasing */
  {
    yinc1 = 1;
    yinc2 = 1;
  }
  else                          /* The y-values are decreasing */
  {
    yinc1 = -1;
    yinc2 = -1;
  }

  if (deltax >= deltay)         /* There is at least one x-value for every y-value */
  {
    xinc1 = 0;                  /* Don't change the x when numerator >= denominator */
    yinc2 = 0;                  /* Don't change the y for every iteration */
    den = deltax;
    num = deltax / 2;
    numadd = deltay;
    numpixels = deltax;         /* There are more x-values than y-values */
  }
  else                          /* There is at least one y-value for every x-value */
  {
    xinc2 = 0;                  /* Don't change the x for every iteration */
    yinc1 = 0;                  /* Don't change the y when numerator >= denominator */
    den = deltay;
    num = deltay / 2;
    numadd = deltax;
    numpixels = deltay;         /* There are more y-values than x-values */
  }

  for (curpixel = 0; curpixel <= numpixels; curpixel++)
  {
    lcd_low_DrawLine(x, y, Positions->x3, Positions->y3, Color);

    num += numadd;              /* Increase the numerator by the top of the fraction */
    if (num >= den)             /* Check if numerator >= denominator */
    {
      num -= den;               /* Calculate the new numerator value */
      x += xinc1;               /* Change the x as appropriate */
      y += yinc1;               /* Change the y as appropriate */
    }
    x += xinc2;                 /* Change the x as appropriate */
    y += yinc2;                 /* Change the y as appropriate */
  }
}
