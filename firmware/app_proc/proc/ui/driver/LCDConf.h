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

#define DRIVER_32_BIT			1

#define NUM_BUFFERS  			2
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

#if (GUI_NUM_LAYERS > 2)
//
#ifdef LCD_LANDSCAPE
//#ifdef STARTEK_43INCH
//#define DISPLAY_DRIVER_1   		GUIDRV_LIN_OSX_32
//#else
#define DISPLAY_DRIVER_2   		GUIDRV_LIN_OSY_32
//#endif
#else
#define DISPLAY_DRIVER_2   		GUIDRV_LIN_32
#endif
//
#define COLOR_CONVERSION_2 		GUICC_M8888I
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

//#define LCD_LAYER0_FRAME_BUFFER  	((int)SDRAM_DEVICE_ADDR)
//#define LAYER_MEM_REQUIRED			(854 * 480 * 4)
//#define LCD_LAYER1_FRAME_BUFFER  	(LCD_LAYER0_FRAME_BUFFER + LAYER_MEM_REQUIRED)

// ---------------------------------------------------------------
// Video RAM memory map
//
// Total size 4 MB
//
// C0 00 00 00 - C0 17 78 00 - Layer 0 buffer, 3203 kB (1 536 000 bytes, 2kB guard)
// C0 17 78 00 - C0 2E F0 00 - Layer 1 buffer, 3203 kB (1 536 000 bytes, 2kB guard)
// C0 2E F0 00 - C0 46 50 00 - Layer 2 buffer, 3203 kB (1 536 000 bytes, 2kB guard)
// C0 46 68 00 - C0 80 00 00 - emWin heap
//
#define LAYER_MEM_REQUIRED			((800 * 480 * 4) + 0x800)
#define LCD_LAYER0_FRAME_BUFFER  	((int)0xC0000000)
#define LCD_LAYER1_FRAME_BUFFER  	((int)0xC0177800)
#define LCD_LAYER2_FRAME_BUFFER  	((int)0xC02EF000)

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

