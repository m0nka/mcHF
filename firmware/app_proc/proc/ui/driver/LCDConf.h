/************************************************************************************
**                                                                                 **
**                                 mcHF QRP Transceiver                            **
**                         Krassi Atanassov - M0NKA, 2013-2026                     **
**                                                                                 **
**---------------------------------------------------------------------------------**
**                                                                                 **
**  File name:                                                                     **
**  Description:                                                                   **
**  Last Modified:                                                                 **
**  Licence:			https://github.com/m0nka/mcHF/blob/main/LICENSE            **
************************************************************************************/
#include "mchf_pro_board.h"
#include "main.h"
#include "GUIDRV_Lin.h"

#ifndef __LCDCONF_H
#define __LCDCONF_H

// LCD clocks in kHz

//#define 	LCD_LANE_CLK			62500
//#define 	LCD_LANE_CLK			58750				// 58750
#define 	LCD_LANE_CLK			55000
#define	 	ST7701_PIXEL_CLK  		(LCD_LANE_CLK/2)

#define DRIVER_32_BIT			1

#define NUM_BUFFERS  			3
#define NUM_VSCREENS 			1

#undef  GUI_NUM_LAYERS
#define GUI_NUM_LAYERS 			2

#if (DRIVER_32_BIT == 1)
// -----------------------------------------------------------
// GUIDRV_LIN_OSX_32 - 32bpp, X axis mirrored, X and Y swapped
#ifdef LCD_LANDSCAPE
//
//#ifdef STARTEK_43INCH
//#define DISPLAY_DRIVER_0   		GUIDRV_LIN_OSX_32
//#else
#define DISPLAY_DRIVER_0   		GUIDRV_LIN_OSY_32
//#endif
#else
#define DISPLAY_DRIVER_0   		GUIDRV_LIN_32
#endif
//
#define DSI_COLOR				DSI_RGB888
#define COLOR_CONVERSION_0 		GUICC_M8888I
//
#if (GUI_NUM_LAYERS > 1)
//
#ifdef LCD_LANDSCAPE
//#ifdef STARTEK_43INCH
//#define DISPLAY_DRIVER_1   		GUIDRV_LIN_OSX_32
//#else
#define DISPLAY_DRIVER_1   		GUIDRV_LIN_OSY_32
//#endif
#else
#define DISPLAY_DRIVER_1   		GUIDRV_LIN_32
#endif
//
#define COLOR_CONVERSION_1 		GUICC_M8888I
//
#endif

// -----------------------------------------------------------
#else
// -----------------------------------------------------------
// GUIDRV_LIN_OSX_16 - 16bpp, X axis mirrored, X and Y swapped
#ifdef LCD_LANDSCAPE
//
#define DISPLAY_DRIVER_0   		GUIDRV_LIN_OSX_16
#else
#define DISPLAY_DRIVER_0   		GUIDRV_LIN_16
#endif
//
#define DSI_COLOR				DSI_RGB565
#define COLOR_CONVERSION_0 		GUICC_M565
//
#if (GUI_NUM_LAYERS > 1)
//
#ifdef LCD_LANDSCAPE
#define DISPLAY_DRIVER_1   		GUIDRV_LIN_OSX_16
#else
#define DISPLAY_DRIVER_1   		GUIDRV_LIN_16
#endif
//
#define COLOR_CONVERSION_1 		GUICC_M565
//
#endif
// ------------------------------------------------------------
#endif

#ifndef   NUM_VSCREENS
  #define NUM_VSCREENS 1
#else
  #if (NUM_VSCREENS <= 0)
    #error At least one screeen needs to be defined!
  #endif
#endif
#if (NUM_VSCREENS > 1) && (NUM_BUFFERS > 1)
  #error Virtual screens and multiple buffers are not allowed!
#endif

#define LAYER_MEM_REQUIRED			(800 * 480 * 4)

typedef struct
{
  int32_t      address;
  __IO int32_t pending_buffer;
  int32_t      buffer_index;
  int32_t      xSize;
  int32_t      ySize;
  int32_t      BytesPerPixel;
  LCD_API_COLOR_CONV   *pColorConvAPI;
} LCD_LayerPropTypedef;

#endif

