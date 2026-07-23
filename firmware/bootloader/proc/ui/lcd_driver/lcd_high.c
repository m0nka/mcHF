//************************************************************************************
//**                                                                                 **
//**                                 mcHF QRP Transceiver                            **
//**                         Krassi Atanassov - M0NKA, 2013-2026                     **
//**                                                                                 **
//**---------------------------------------------------------------------------------**
//**                                                                                 **
//**  File name:		lcd_high.c                                                   **
//**  Description:  	basic drawing services (text, shapes) on top of lcd_low     **
//**  Licence:			https://github.com/m0nka/mcHF/blob/main/LICENSE            **
//************************************************************************************
//
// The panel scans in portrait (480 wide, 800 tall) but the boot screen is
// used in landscape: text drawing rotates the glyphs in software, so a
// "line" of text stacks along the short X axis (from the right edge) and
// runs along the long Y axis. Shapes (rect, line, circle) draw in raw
// portrait framebuffer coordinates.
//
#include "mchf_pro_board.h"
#include "main.h"

#include "lcd_low.h"
#include "lcd_high.h"

#ifndef LCD_HIGH_MAX_LAYERS_NBR
#define LCD_HIGH_MAX_LAYERS_NBR    2U
#endif

#define ABS(X)                 ((X) > 0 ? (X) : -(X))
#define POLY_X(Z)              ((int32_t)((Points + (Z))->X))
#define POLY_Y(Z)              ((int32_t)((Points + (Z))->Y))

typedef struct
{
  uint32_t x1;
  uint32_t y1;
  uint32_t x2;
  uint32_t y2;
  uint32_t x3;
  uint32_t y3;
} Triangle_Positions_t;

// Current drawing layer properties
static lcd_high_Ctx_t DrawProp[LCD_HIGH_MAX_LAYERS_NBR];
static lcd_low_Drv_t FuncDriver;

static void DrawChar(uint32_t Xpos, uint32_t Ypos, const uint8_t *pData);
static void FillTriangle(Triangle_Positions_t *Positions, uint32_t Color);

//*----------------------------------------------------------------------------
//* Function Name       : lcd_high_SetFuncDriver
//* Object              : link the lcd_low hardware driver
//*----------------------------------------------------------------------------
void lcd_high_SetFuncDriver(const lcd_low_Drv_t *pDrv)
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
}

void lcd_high_SetLayer(uint32_t Layer)
{
  if(FuncDriver.SetLayer != NULL)
  {
    if(FuncDriver.SetLayer(DrawProp->GuiDevice, Layer) == 0)
    {
      DrawProp->GuiLayer = Layer;
    }
  }
}

void lcd_high_SetDevice(uint32_t Device)
{
  DrawProp->GuiDevice = Device;
  FuncDriver.GetXSize(Device, &DrawProp->GuiXsize);
  FuncDriver.GetYSize(Device, &DrawProp->GuiYsize);
}

void lcd_high_SetTextColor(uint32_t Color)
{
  DrawProp[DrawProp->GuiLayer].TextColor = Color;
}

uint32_t lcd_high_GetTextColor(void)
{
  return DrawProp[DrawProp->GuiLayer].TextColor;
}

void lcd_high_SetBackColor(uint32_t Color)
{
  DrawProp[DrawProp->GuiLayer].BackColor = Color;
}

uint32_t lcd_high_GetBackColor(void)
{
  return DrawProp[DrawProp->GuiLayer].BackColor;
}

void lcd_high_SetFont(sFONT *fonts)
{
  DrawProp[DrawProp->GuiLayer].pFont = fonts;
}

sFONT *lcd_high_GetFont(void)
{
  return DrawProp[DrawProp->GuiLayer].pFont;
}

// Draw a rectangle of ARGB8888 pixel data
void lcd_high_FillRGBRect(uint32_t Xpos, uint32_t Ypos, uint8_t *pData, uint32_t Width, uint32_t Height)
{
  FuncDriver.FillRGBRect(DrawProp->GuiDevice, Xpos, Ypos, pData, Width, Height);
}

void lcd_high_DrawHLine(uint32_t Xpos, uint32_t Ypos, uint32_t Length, uint32_t Color)
{
  FuncDriver.DrawHLine(DrawProp->GuiDevice, Xpos, Ypos, Length, Color);
}

void lcd_high_DrawVLine(uint32_t Xpos, uint32_t Ypos, uint32_t Length, uint32_t Color)
{
  FuncDriver.DrawVLine(DrawProp->GuiDevice, Xpos, Ypos, Length, Color);
}

void lcd_high_GetPixel(uint16_t Xpos, uint16_t Ypos, uint32_t *Color)
{
  FuncDriver.GetPixel(DrawProp->GuiDevice, Xpos, Ypos, Color);
}

void lcd_high_SetPixel(uint16_t Xpos, uint16_t Ypos, uint32_t Color)
{
  FuncDriver.SetPixel(DrawProp->GuiDevice, Xpos, Ypos, Color);
}

// Clear the whole currently active layer
void lcd_high_Clear(uint32_t Color)
{
  lcd_high_FillRect(0, 0, DrawProp->GuiXsize, DrawProp->GuiYsize, Color);
}

// Clear the selected text line
void lcd_high_ClearStringLine(uint32_t Line)
{
  lcd_high_FillRect(0, (Line * DrawProp[DrawProp->GuiLayer].pFont->Height), DrawProp->GuiXsize, DrawProp[DrawProp->GuiLayer].pFont->Height, DrawProp[DrawProp->GuiLayer].BackColor);
}

// Display one character (ascii 0x20 to 0x7E)
void lcd_high_DisplayChar(uint32_t Xpos, uint32_t Ypos, uint8_t Ascii)
{
  DrawChar(Xpos, Ypos, &DrawProp[DrawProp->GuiLayer].pFont->table[(Ascii-' ') *\
  DrawProp[DrawProp->GuiLayer].pFont->Height * ((DrawProp[DrawProp->GuiLayer].pFont->Width + 7) / 8)]);
}

// Display a string - Xpos selects the text line (stacked along the short
// axis), Ypos is the start position along the long axis
void lcd_high_DisplayStringAt(uint32_t Xpos, uint32_t Ypos, uint8_t *Text, Text_AlignModeTypdef Mode)
{
  uint32_t refcolumn = Ypos;

  // Only left alignment is supported on the rotated boot screen
  (void)Mode;

  // Check that the start column is located in the screen
  if ((refcolumn < 1) || (refcolumn >= 0x8000))
  {
    refcolumn = 1;
  }

  // Send the string character by character, stop at the edge of the
  // long (Y) axis the text runs along
  while ((*Text != 0) && ((refcolumn + DrawProp[DrawProp->GuiLayer].pFont->Width) <= DrawProp->GuiYsize))
  {
    // Display one character
    lcd_high_DisplayChar(Xpos, refcolumn, *Text);

    // Advance along the text run axis
    refcolumn += DrawProp[DrawProp->GuiLayer].pFont->Width;

    // Point on the next character
    Text++;
  }
}

void lcd_high_DisplayStringAtLine(uint32_t Line, uint8_t *ptr)
{
  lcd_high_DisplayStringAt(0, LINE(Line), ptr, LEFT_MODE);
}

// Draw an uni-line between two points
void lcd_high_DrawLine(uint32_t Xpos1, uint32_t Ypos1, uint32_t Xpos2, uint32_t Ypos2, uint32_t Color)
{
  int16_t deltax = 0, deltay = 0, x = 0, y = 0, xinc1 = 0, xinc2 = 0,
  yinc1 = 0, yinc2 = 0, den = 0, num = 0, numadd = 0, numpixels = 0,
  curpixel = 0;
  int32_t x_diff, y_diff;

  x_diff = Xpos2 - Xpos1;
  y_diff = Ypos2 - Ypos1;

  deltax = ABS(x_diff);
  deltay = ABS(y_diff);
  x = Xpos1;
  y = Ypos1;

  if (Xpos2 >= Xpos1)
  {
    xinc1 = 1;
    xinc2 = 1;
  }
  else
  {
    xinc1 = -1;
    xinc2 = -1;
  }

  if (Ypos2 >= Ypos1)
  {
    yinc1 = 1;
    yinc2 = 1;
  }
  else
  {
    yinc1 = -1;
    yinc2 = -1;
  }

  if (deltax >= deltay)
  {
    xinc1 = 0;
    yinc2 = 0;
    den = deltax;
    num = deltax / 2;
    numadd = deltay;
    numpixels = deltax;
  }
  else
  {
    xinc2 = 0;
    yinc1 = 0;
    den = deltay;
    num = deltay / 2;
    numadd = deltax;
    numpixels = deltay;
  }

  for (curpixel = 0; curpixel <= numpixels; curpixel++)
  {
    lcd_high_SetPixel(x, y, Color);
    num += numadd;
    if (num >= den)
    {
      num -= den;
      x += xinc1;
      y += yinc1;
    }
    x += xinc2;
    y += yinc2;
  }
}

void lcd_high_DrawRect(uint32_t Xpos, uint32_t Ypos, uint32_t Width, uint32_t Height, uint32_t Color)
{
  // Draw horizontal lines
  lcd_high_DrawHLine(Xpos, Ypos, Width, Color);
  lcd_high_DrawHLine(Xpos, (Ypos+ Height - 1U), Width, Color);

  // Draw vertical lines
  lcd_high_DrawVLine(Xpos, Ypos, Height, Color);
  lcd_high_DrawVLine((Xpos + Width - 1U), Ypos, Height, Color);
}

void lcd_high_DrawCircle(uint32_t Xpos, uint32_t Ypos, uint32_t Radius, uint32_t Color)
{
  int32_t   decision;
  uint32_t  current_x;
  uint32_t  current_y;

  decision = 3 - (Radius << 1);
  current_x = 0;
  current_y = Radius;

  while (current_x <= current_y)
  {
    if((Ypos - current_y) < DrawProp->GuiYsize)
    {
      if((Xpos + current_x) < DrawProp->GuiXsize)
      {
        lcd_high_SetPixel((Xpos + current_x), (Ypos - current_y), Color);
      }
      if((Xpos - current_x) < DrawProp->GuiXsize)
      {
        lcd_high_SetPixel((Xpos - current_x), (Ypos - current_y), Color);
      }
    }

    if((Ypos - current_x) < DrawProp->GuiYsize)
    {
      if((Xpos + current_y) < DrawProp->GuiXsize)
      {
        lcd_high_SetPixel((Xpos + current_y), (Ypos - current_x), Color);
      }
      if((Xpos - current_y) < DrawProp->GuiXsize)
      {
        lcd_high_SetPixel((Xpos - current_y), (Ypos - current_x), Color);
      }
    }

    if((Ypos + current_y) < DrawProp->GuiYsize)
    {
      if((Xpos + current_x) < DrawProp->GuiXsize)
      {
        lcd_high_SetPixel((Xpos + current_x), (Ypos + current_y), Color);
      }
      if((Xpos - current_x) < DrawProp->GuiXsize)
      {
        lcd_high_SetPixel((Xpos - current_x), (Ypos + current_y), Color);
      }
    }

    if((Ypos + current_x) < DrawProp->GuiYsize)
    {
      if((Xpos + current_y) < DrawProp->GuiXsize)
      {
        lcd_high_SetPixel((Xpos + current_y), (Ypos + current_x), Color);
      }
      if((Xpos - current_y) < DrawProp->GuiXsize)
      {
        lcd_high_SetPixel((Xpos - current_y), (Ypos + current_x), Color);
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

// Circle with a simple radial colour gradient
void lcd_high_DrawCircleG(uint32_t Xpos, uint32_t Ypos, uint32_t Radius, uint32_t c)
{
  int32_t   decision;
  uint32_t  current_x;
  uint32_t  current_y;
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
        lcd_high_SetPixel((Xpos + current_x), (Ypos - current_y), Color);
      }
      if((Xpos - current_x) < DrawProp->GuiXsize)
      {
        lcd_high_SetPixel((Xpos - current_x), (Ypos - current_y), Color);
      }
    }

    if((Ypos - current_x) < DrawProp->GuiYsize)
    {
      if((Xpos + current_y) < DrawProp->GuiXsize)
      {
        lcd_high_SetPixel((Xpos + current_y), (Ypos - current_x), Color);
      }
      if((Xpos - current_y) < DrawProp->GuiXsize)
      {
        lcd_high_SetPixel((Xpos - current_y), (Ypos - current_x), Color);
      }
    }

    if((Ypos + current_y) < DrawProp->GuiYsize)
    {
      if((Xpos + current_x) < DrawProp->GuiXsize)
      {
        lcd_high_SetPixel((Xpos + current_x), (Ypos + current_y), Color);
      }
      if((Xpos - current_x) < DrawProp->GuiXsize)
      {
        lcd_high_SetPixel((Xpos - current_x), (Ypos + current_y), Color);
      }
    }

    if((Ypos + current_x) < DrawProp->GuiYsize)
    {
      if((Xpos + current_y) < DrawProp->GuiXsize)
      {
        lcd_high_SetPixel((Xpos + current_y), (Ypos + current_x), Color);
      }
      if((Xpos - current_y) < DrawProp->GuiXsize)
      {
        lcd_high_SetPixel((Xpos - current_y), (Ypos + current_x), Color);
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

void lcd_high_DrawPolygon(pPoint Points, uint32_t PointCount, uint32_t Color)
{
  int16_t x_pos = 0, y_pos = 0;

  if(PointCount < 2)
  {
    return;
  }

  lcd_high_DrawLine(Points->X, Points->Y, (Points+PointCount-1)->X, (Points+PointCount-1)->Y, Color);

  while(--PointCount)
  {
    x_pos = Points->X;
    y_pos = Points->Y;
    Points++;
    lcd_high_DrawLine(x_pos, y_pos, Points->X, Points->Y, Color);
  }
}

void lcd_high_DrawEllipse(int Xpos, int Ypos, int XRadius, int YRadius, uint32_t Color)
{
  int x_pos = 0, y_pos = -YRadius, err = 2-2*XRadius, e2;
  float k = 0, rad1 = 0, rad2 = 0;

  rad1 = XRadius;
  rad2 = YRadius;

  k = (float)(rad2/rad1);

  do
  {
    lcd_high_SetPixel((Xpos-(uint32_t)(x_pos/k)), (Ypos + y_pos), Color);
    lcd_high_SetPixel((Xpos+(uint32_t)(x_pos/k)), (Ypos + y_pos), Color);
    lcd_high_SetPixel((Xpos+(uint32_t)(x_pos/k)), (Ypos - y_pos), Color);
    lcd_high_SetPixel((Xpos-(uint32_t)(x_pos/k)), (Ypos - y_pos), Color);

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

// Draw an uncompressed 24/32bpp BMP from flash
void lcd_high_DrawBitmap(uint32_t Xpos, uint32_t Ypos, uint8_t *pData)
{
  FuncDriver.DrawBitmap(DrawProp->GuiDevice, Xpos, Ypos, pData);
}

void lcd_high_FillRect(uint32_t Xpos, uint32_t Ypos, uint32_t Width, uint32_t Height, uint32_t Color)
{
  FuncDriver.FillRect(DrawProp->GuiDevice, Xpos, Ypos, Width, Height, Color);
}

void lcd_high_FillCircle(uint32_t Xpos, uint32_t Ypos, uint32_t Radius, uint32_t Color)
{
  int32_t   decision;
  uint32_t  current_x;
  uint32_t  current_y;

  decision = 3 - (Radius << 1);

  current_x = 0;
  current_y = Radius;

  while (current_x <= current_y)
  {
    if(current_y > 0)
    {
      if(current_y >= Xpos)
      {
        lcd_high_DrawHLine(0, Ypos + current_x, 2*current_y - (current_y - Xpos), Color);
        lcd_high_DrawHLine(0, Ypos - current_x, 2*current_y - (current_y - Xpos), Color);
      }
      else
      {
        lcd_high_DrawHLine(Xpos - current_y, Ypos + current_x, 2*current_y, Color);
        lcd_high_DrawHLine(Xpos - current_y, Ypos - current_x, 2*current_y, Color);
      }
    }

    if(current_x > 0)
    {
      if(current_x >= Xpos)
      {
        lcd_high_DrawHLine(0, Ypos - current_y, 2*current_x - (current_x - Xpos), Color);
        lcd_high_DrawHLine(0, Ypos + current_y, 2*current_x - (current_x - Xpos), Color);
      }
      else
      {
        lcd_high_DrawHLine(Xpos - current_x, Ypos - current_y, 2*current_x, Color);
        lcd_high_DrawHLine(Xpos - current_x, Ypos + current_y, 2*current_x, Color);
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

  lcd_high_DrawCircle(Xpos, Ypos, Radius, Color);
}

void lcd_high_FillPolygon(pPoint Points, uint32_t PointCount, uint32_t Color)
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

void lcd_high_FillEllipse(int Xpos, int Ypos, int XRadius, int YRadius, uint32_t Color)
{
  int x_pos = 0, y_pos = -YRadius, err = 2-2*XRadius, e2;
  float k = 0, rad1 = 0, rad2 = 0;

  rad1 = XRadius;
  rad2 = YRadius;

  k = (float)(rad2/rad1);

  do
  {
    lcd_high_DrawHLine((Xpos-(uint32_t)(x_pos/k)), (Ypos + y_pos), (2*(uint32_t)(x_pos/k) + 1), Color);
    lcd_high_DrawHLine((Xpos-(uint32_t)(x_pos/k)), (Ypos - y_pos), (2*(uint32_t)(x_pos/k) + 1), Color);

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

//*----------------------------------------------------------------------------
//* Function Name       : DrawChar
//* Object              : draw one glyph rotated 90 degrees, so text reads
//*                       in landscape on the portrait scanned panel - each
//*                       glyph row becomes a 1 pixel wide vertical strip
//*----------------------------------------------------------------------------
static void DrawChar(uint32_t Xpos, uint32_t Ypos, const uint8_t *pData)
{
	uint32_t i = 0, j = 0, offset;
	uint32_t height, width;
	uint8_t  *pchar;
	uint32_t line;
	uint32_t argb8888[24];

	height = DrawProp[DrawProp->GuiLayer].pFont->Height;
	width  = DrawProp[DrawProp->GuiLayer].pFont->Width;
	offset = 8*((width + 7)/8) - width;
	Xpos   = (DrawProp->GuiXsize - 1) - Xpos;
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
				argb8888[j] = DrawProp[DrawProp->GuiLayer].TextColor;
			else
				argb8888[j] = DrawProp[DrawProp->GuiLayer].BackColor;
		}

		lcd_high_FillRGBRect(Xpos--, Ypos, (uint8_t*)&argb8888[0], 1, width);
    }
}

//*----------------------------------------------------------------------------
//* Function Name       : FillTriangle
//* Object              : fill a triangle between 3 points
//*----------------------------------------------------------------------------
static void FillTriangle(Triangle_Positions_t *Positions, uint32_t Color)
{
  int16_t deltax = 0, deltay = 0, x = 0, y = 0, xinc1 = 0, xinc2 = 0,
  yinc1 = 0, yinc2 = 0, den = 0, num = 0, numadd = 0, numpixels = 0,
  curpixel = 0;
  int32_t x_diff, y_diff;

  x_diff = Positions->x2 - Positions->x1;
  y_diff = Positions->y2 - Positions->y1;

  deltax = ABS(x_diff);
  deltay = ABS(y_diff);
  x = Positions->x1;
  y = Positions->y1;

  if (Positions->x2 >= Positions->x1)
  {
    xinc1 = 1;
    xinc2 = 1;
  }
  else
  {
    xinc1 = -1;
    xinc2 = -1;
  }

  if (Positions->y2 >= Positions->y1)
  {
    yinc1 = 1;
    yinc2 = 1;
  }
  else
  {
    yinc1 = -1;
    yinc2 = -1;
  }

  if (deltax >= deltay)
  {
    xinc1 = 0;
    yinc2 = 0;
    den = deltax;
    num = deltax / 2;
    numadd = deltay;
    numpixels = deltax;
  }
  else
  {
    xinc2 = 0;
    yinc1 = 0;
    den = deltay;
    num = deltay / 2;
    numadd = deltax;
    numpixels = deltay;
  }

  for (curpixel = 0; curpixel <= numpixels; curpixel++)
  {
    lcd_high_DrawLine(x, y, Positions->x3, Positions->y3, Color);

    num += numadd;
    if (num >= den)
    {
      num -= den;
      x += xinc1;
      y += yinc1;
    }
    x += xinc2;
    y += yinc2;
  }
}
