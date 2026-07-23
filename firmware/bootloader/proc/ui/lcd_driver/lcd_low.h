//************************************************************************************
//**                                                                                 **
//**                                 mcHF QRP Transceiver                            **
//**                         Krassi Atanassov - M0NKA, 2013-2026                     **
//**                                                                                 **
//**---------------------------------------------------------------------------------**
//**                                                                                 **
//**  File name:		lcd_low.h                                                    **
//**  Description:  	LTDC/DSI hardware driver, L8 framebuffer in AXI SRAM        **
//**  Licence:			https://github.com/m0nka/mcHF/blob/main/LICENSE            **
//************************************************************************************
#ifndef __LCD_LOW_H
#define __LCD_LOW_H

#ifdef USE_LCD_BAREMETAL

#include "ili9806e.h"

// Framebuffer lives in the 512K AXI SRAM - the external SDRAM is not
// initialised yet when the boot screen comes up. The full native panel
// resolution fits because the buffer is 8bit indexed colour:
// 480 x 800 x 1 = 384000 bytes
#define LAYER0_ADDRESS        			0x24000000

// LTDC layer pixel format of the framebuffer (expanded to 24bit RGB
// through the layer CLUT by the LTDC pixel format converter)
#define LCD_PIXEL_FORMAT_L8				0x00000005U

typedef struct
{
  int32_t ( *DrawBitmap      ) (uint32_t, uint32_t, uint32_t, uint8_t *);
  int32_t ( *FillRGBRect     ) (uint32_t, uint32_t, uint32_t, uint8_t*, uint32_t, uint32_t);
  int32_t ( *DrawHLine       ) (uint32_t, uint32_t, uint32_t, uint32_t, uint32_t);
  int32_t ( *DrawVLine       ) (uint32_t, uint32_t, uint32_t, uint32_t, uint32_t);
  int32_t ( *FillRect        ) (uint32_t, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t);
  int32_t ( *GetPixel        ) (uint32_t, uint32_t, uint32_t, uint32_t*);
  int32_t ( *SetPixel        ) (uint32_t, uint32_t, uint32_t, uint32_t);
  int32_t ( *GetXSize        ) (uint32_t, uint32_t *);
  int32_t ( *GetYSize        ) (uint32_t, uint32_t *);
  int32_t ( *SetLayer        ) (uint32_t, uint32_t);
  int32_t ( *GetFormat       ) (uint32_t, uint32_t *);
} lcd_low_Drv_t;

extern const lcd_low_Drv_t LCD_Driver;

// Init
int32_t BSP_LCD_Init(uint32_t Instance);

// Draw operations, all colours are ARGB8888 and get quantised to the
// RGB332 palette of the L8 framebuffer internally
int32_t BSP_LCD_DrawBitmap(uint32_t Instance, uint32_t Xpos, uint32_t Ypos, uint8_t *pBmp);
int32_t BSP_LCD_FillRGBRect(uint32_t Instance, uint32_t Xpos, uint32_t Ypos, uint8_t *pData, uint32_t Width, uint32_t Height);
int32_t BSP_LCD_DrawHLine(uint32_t Instance, uint32_t Xpos, uint32_t Ypos, uint32_t Length, uint32_t Color);
int32_t BSP_LCD_DrawVLine(uint32_t Instance, uint32_t Xpos, uint32_t Ypos, uint32_t Length, uint32_t Color);
int32_t BSP_LCD_FillRect(uint32_t Instance, uint32_t Xpos, uint32_t Ypos, uint32_t Width, uint32_t Height, uint32_t Color);
int32_t BSP_LCD_ReadPixel(uint32_t Instance, uint32_t Xpos, uint32_t Ypos, uint32_t *Color);
int32_t BSP_LCD_WritePixel(uint32_t Instance, uint32_t Xpos, uint32_t Ypos, uint32_t Color);
int32_t BSP_LCD_GetXSize(uint32_t Instance, uint32_t *XSize);
int32_t BSP_LCD_GetYSize(uint32_t Instance, uint32_t *YSize);
int32_t BSP_LCD_SetActiveLayer(uint32_t Instance, uint32_t LayerIndex);
int32_t BSP_LCD_GetPixelFormat(uint32_t Instance, uint32_t *PixelFormat);

#endif

#endif
