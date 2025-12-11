/*********************************************************************
*                    SEGGER Microcontroller GmbH                     *
*        Solutions for real time microcontroller applications        *
**********************************************************************
*                                                                    *
*        (c) 1996 - 2020  SEGGER Microcontroller GmbH                *
*                                                                    *
*        Internet: www.segger.com    Support:  support@segger.com    *
*                                                                    *
**********************************************************************

** emWin V6.10 - Graphical user interface for embedded applications **
emWin is protected by international copyright laws.   Knowledge of the
source code may not be used to write a similar product.  This file may
only  be used  in accordance  with  a license  and should  not be  re-
distributed in any way. We appreciate your understanding and fairness.
----------------------------------------------------------------------
File        : DIALOG_Keypad.c
Purpose     : QWERTY keypad implementation.
Requirements: WindowManager - (x)
              MemoryDevices - ( )
              AntiAliasing  - ( )
              VNC-Server    - ( )
              PNG-Library   - ( )
              TrueTypeFonts - ( )
----------------------------------------------------------------------
*/

#include "c_keypad.h"

#if 0

/*********************************************************************
*
*       Static data
*
**********************************************************************
*/
/*********************************************************************
*
*       bmReturn, bmBackSpace, bmUp: Bitmaps used for keyboard
*/
static GUI_CONST_STORAGE unsigned long acReturn[] = {
#if (GUI_USE_ARGB == 1)
  0x00FFFFFF, 0x00FFFFFF, 0x00FFFFFF, 0x00FFFFFF, 0x00FFFFFF, 0x00FFFFFF, 0x5FA0A0A0, 0xDF202020, 0x00FFFFFF, 0x00FFFFFF, 0x00FFFFFF, 0x00FFFFFF, 0x00FFFFFF, 0x00FFFFFF, 0xAF505050, 0xAF505050,
  0x00FFFFFF, 0x00FFFFFF, 0x00FFFFFF, 0x00FFFFFF, 0x5FA0A0A0, 0xDF202020, 0xFF000000, 0xFF000000, 0x00FFFFFF, 0x00FFFFFF, 0x00FFFFFF, 0x00FFFFFF, 0x00FFFFFF, 0x00FFFFFF, 0xFF000000, 0xFF000000,
  0x00FFFFFF, 0x00FFFFFF, 0x5FA0A0A0, 0xDF202020, 0xFF000000, 0xFF000000, 0xFF000000, 0xFF000000, 0x00FFFFFF, 0x00FFFFFF, 0x00FFFFFF, 0x00FFFFFF, 0x00FFFFFF, 0x00FFFFFF, 0xFF000000, 0xFF000000,
  0x5FA0A0A0, 0xDF202020, 0xFF000000, 0xFF000000, 0xFF000000, 0xFF000000, 0xFF000000, 0xFF000000, 0xFF000000, 0xFF000000, 0xFF000000, 0xFF000000, 0xFF000000, 0xFF000000, 0xFF000000, 0xFF000000,
  0x5FA0A0A0, 0xDF202020, 0xFF000000, 0xFF000000, 0xFF000000, 0xFF000000, 0xFF000000, 0xFF000000, 0xFF000000, 0xFF000000, 0xFF000000, 0xFF000000, 0xFF000000, 0xFF000000, 0xFF000000, 0xAF505050,
  0x00FFFFFF, 0x00FFFFFF, 0x5FA0A0A0, 0xDF202020, 0xFF000000, 0xFF000000, 0xFF000000, 0xFF000000, 0x00FFFFFF, 0x00FFFFFF, 0x00FFFFFF, 0x00FFFFFF, 0x00FFFFFF, 0x00FFFFFF, 0x00FFFFFF, 0x00FFFFFF,
  0x00FFFFFF, 0x00FFFFFF, 0x00FFFFFF, 0x00FFFFFF, 0x5FA0A0A0, 0xDF202020, 0xFF000000, 0xFF000000, 0x00FFFFFF, 0x00FFFFFF, 0x00FFFFFF, 0x00FFFFFF, 0x00FFFFFF, 0x00FFFFFF, 0x00FFFFFF, 0x00FFFFFF,
  0x00FFFFFF, 0x00FFFFFF, 0x00FFFFFF, 0x00FFFFFF, 0x00FFFFFF, 0x00FFFFFF, 0x5FA0A0A0, 0xDF202020, 0x00FFFFFF, 0x00FFFFFF, 0x00FFFFFF, 0x00FFFFFF, 0x00FFFFFF, 0x00FFFFFF, 0x00FFFFFF, 0x00FFFFFF
#else
  0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xA0A0A0A0, 0x20202020, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0x50505050, 0x50505050,
  0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xA0A0A0A0, 0x20202020, 0x00000000, 0x00000000, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0x00000000, 0x00000000,
  0xFFFFFFFF, 0xFFFFFFFF, 0xA0A0A0A0, 0x20202020, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0x00000000, 0x00000000,
  0xA0A0A0A0, 0x20202020, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
  0xA0A0A0A0, 0x20202020, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x50505050,
  0xFFFFFFFF, 0xFFFFFFFF, 0xA0A0A0A0, 0x20202020, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF,
  0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xA0A0A0A0, 0x20202020, 0x00000000, 0x00000000, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF,
  0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xA0A0A0A0, 0x20202020, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF
#endif
};

static GUI_CONST_STORAGE GUI_BITMAP bmReturn = {
  16, // XSize
  8, // YSize
  64, // BytesPerLine
  32, // BitsPerPixel
  (unsigned char *)acReturn,  // Pointer to picture data
  NULL,  // Pointer to palette
#if (GUI_USE_ARGB == 1)
  GUI_DRAW_BMPM8888I
#else
  GUI_DRAW_BMP8888
#endif
};

static GUI_CONST_STORAGE unsigned long acBackSpace[] = {
#if (GUI_USE_ARGB == 1)
  0x00FFFFFF, 0x00FFFFFF, 0x00FFFFFF, 0x00FFFFFF, 0x00FFFFFF, 0x00FFFFFF, 0x5FA0A0A0, 0xDF202020, 0x00FFFFFF, 0x00FFFFFF, 0x00FFFFFF, 0x00FFFFFF, 0x00FFFFFF, 0x00FFFFFF, 0x00FFFFFF, 0x00FFFFFF,
  0x00FFFFFF, 0x00FFFFFF, 0x00FFFFFF, 0x00FFFFFF, 0x5FA0A0A0, 0xDF202020, 0xFF000000, 0xFF000000, 0x00FFFFFF, 0x00FFFFFF, 0x00FFFFFF, 0x00FFFFFF, 0x00FFFFFF, 0x00FFFFFF, 0x00FFFFFF, 0x00FFFFFF,
  0x00FFFFFF, 0x00FFFFFF, 0x5FA0A0A0, 0xDF202020, 0xFF000000, 0xFF000000, 0xFF000000, 0xFF000000, 0x00FFFFFF, 0x00FFFFFF, 0x00FFFFFF, 0x00FFFFFF, 0x00FFFFFF, 0x00FFFFFF, 0x00FFFFFF, 0x00FFFFFF,
  0x5FA0A0A0, 0xDF202020, 0xFF000000, 0xFF000000, 0xFF000000, 0xFF000000, 0xFF000000, 0xFF000000, 0xFF000000, 0xFF000000, 0xFF000000, 0xFF000000, 0xFF000000, 0xFF000000, 0xFF000000, 0xFF000000,
  0x5FA0A0A0, 0xDF202020, 0xFF000000, 0xFF000000, 0xFF000000, 0xFF000000, 0xFF000000, 0xFF000000, 0xFF000000, 0xFF000000, 0xFF000000, 0xFF000000, 0xFF000000, 0xFF000000, 0xFF000000, 0xFF000000,
  0x00FFFFFF, 0x00FFFFFF, 0x5FA0A0A0, 0xDF202020, 0xFF000000, 0xFF000000, 0xFF000000, 0xFF000000, 0x00FFFFFF, 0x00FFFFFF, 0x00FFFFFF, 0x00FFFFFF, 0x00FFFFFF, 0x00FFFFFF, 0x00FFFFFF, 0x00FFFFFF,
  0x00FFFFFF, 0x00FFFFFF, 0x00FFFFFF, 0x00FFFFFF, 0x5FA0A0A0, 0xDF202020, 0xFF000000, 0xFF000000, 0x00FFFFFF, 0x00FFFFFF, 0x00FFFFFF, 0x00FFFFFF, 0x00FFFFFF, 0x00FFFFFF, 0x00FFFFFF, 0x00FFFFFF,
  0x00FFFFFF, 0x00FFFFFF, 0x00FFFFFF, 0x00FFFFFF, 0x00FFFFFF, 0x00FFFFFF, 0x5FA0A0A0, 0xDF202020, 0x00FFFFFF, 0x00FFFFFF, 0x00FFFFFF, 0x00FFFFFF, 0x00FFFFFF, 0x00FFFFFF, 0x00FFFFFF, 0x00FFFFFF
#else
  0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xA0A0A0A0, 0x20202020, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF,
  0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xA0A0A0A0, 0x20202020, 0x00000000, 0x00000000, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF,
  0xFFFFFFFF, 0xFFFFFFFF, 0xA0A0A0A0, 0x20202020, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF,
  0xA0A0A0A0, 0x20202020, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
  0xA0A0A0A0, 0x20202020, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
  0xFFFFFFFF, 0xFFFFFFFF, 0xA0A0A0A0, 0x20202020, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF,
  0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xA0A0A0A0, 0x20202020, 0x00000000, 0x00000000, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF,
  0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xA0A0A0A0, 0x20202020, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF
#endif
};

static GUI_CONST_STORAGE GUI_BITMAP bmBackSpace = {
  16, // XSize
  8, // YSize
  64, // BytesPerLine
  32, // BitsPerPixel
  (unsigned char *)acBackSpace,  // Pointer to picture data
  NULL,  // Pointer to palette
#if (GUI_USE_ARGB == 1)
  GUI_DRAW_BMPM8888I
#else
  GUI_DRAW_BMP8888
#endif
};

static GUI_CONST_STORAGE unsigned long acUp[] = {
#if (GUI_USE_ARGB == 1)
  0x00FFFFFF, 0x00FFFFFF, 0x00FFFFFF, 0x9F606060, 0x9F606060, 0x00FFFFFF, 0x00FFFFFF, 0x00FFFFFF,
  0x00FFFFFF, 0x00FFFFFF, 0x9F606060, 0xCF303030, 0xCF303030, 0x9F606060, 0x00FFFFFF, 0x00FFFFFF,
  0x00FFFFFF, 0x9F606060, 0xCF303030, 0x10EFEFEF, 0x10EFEFEF, 0xCF303030, 0x9F606060, 0x00FFFFFF,
  0x9F606060, 0xEF101010, 0x50AFAFAF, 0x00FFFFFF, 0x00FFFFFF, 0x50AFAFAF, 0xEF101010, 0x9F606060,
  0xBF404040, 0xBF404040, 0xFF000000, 0x00FFFFFF, 0x00FFFFFF, 0xFF000000, 0xBF404040, 0xBF404040,
  0x00FFFFFF, 0x00FFFFFF, 0xFF000000, 0x00FFFFFF, 0x00FFFFFF, 0xFF000000, 0x00FFFFFF, 0x00FFFFFF,
  0x00FFFFFF, 0x00FFFFFF, 0xFF000000, 0x00FFFFFF, 0x00FFFFFF, 0xFF000000, 0x00FFFFFF, 0x00FFFFFF,
  0x00FFFFFF, 0x00FFFFFF, 0xFF000000, 0xFF000000, 0xFF000000, 0xFF000000, 0x00FFFFFF, 0x00FFFFFF
#else
  0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0x60606060, 0x60606060, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF,
  0xFFFFFFFF, 0xFFFFFFFF, 0x60606060, 0x30303030, 0x30303030, 0x60606060, 0xFFFFFFFF, 0xFFFFFFFF,
  0xFFFFFFFF, 0x60606060, 0x30303030, 0xEFEFEFEF, 0xEFEFEFEF, 0x30303030, 0x60606060, 0xFFFFFFFF,
  0x60606060, 0x10101010, 0xAFAFAFAF, 0xFFFFFFFF, 0xFFFFFFFF, 0xAFAFAFAF, 0x10101010, 0x60606060,
  0x40404040, 0x40404040, 0x00000000, 0xFFFFFFFF, 0xFFFFFFFF, 0x00000000, 0x40404040, 0x40404040,
  0xFFFFFFFF, 0xFFFFFFFF, 0x00000000, 0xFFFFFFFF, 0xFFFFFFFF, 0x00000000, 0xFFFFFFFF, 0xFFFFFFFF,
  0xFFFFFFFF, 0xFFFFFFFF, 0x00000000, 0xFFFFFFFF, 0xFFFFFFFF, 0x00000000, 0xFFFFFFFF, 0xFFFFFFFF,
  0xFFFFFFFF, 0xFFFFFFFF, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0xFFFFFFFF, 0xFFFFFFFF
#endif
};

static GUI_CONST_STORAGE GUI_BITMAP bmUp = {
  8, // XSize
  8, // YSize
  32, // BytesPerLine
  32, // BitsPerPixel
  (unsigned char *)acUp,  // Pointer to picture data
  NULL,  // Pointer to palette
#if (GUI_USE_ARGB == 1)
  GUI_DRAW_BMPM8888I
#else
  GUI_DRAW_BMP8888
#endif
};

static GUI_CONST_STORAGE U32 _acBackButton_53x16[] = {
  0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x004E4E4E, 0x00000000, 0x00000000, 0x00000000, 0x60595959, 0xF77D7D7D, 0xFF9F9F9F, 0xFF9D9D9D, 0xFF9C9C9C, 0xFF9C9C9C, 0xFF9C9C9C, 0xFF9C9C9C, 
        0xFF9C9C9C, 0xFF9C9C9C, 0xFF9C9C9C, 0xFF9C9C9C, 0xFF9C9C9C, 0xFF9C9C9C, 0xFF9C9C9C, 0xFF9C9C9C, 0xFF9C9C9C, 0xFF9C9C9C, 0xFF9C9C9C, 0xFF9C9C9C, 0xFF9C9C9C, 0xFF9C9C9C, 0xFF9C9C9C, 0xFF9C9C9C, 0xFF9C9C9C, 0xFF9C9C9C, 0xFF9C9C9C, 0xFF9C9C9C, 
        0xFF9C9C9C, 0xFF9C9C9C, 0xFF9C9C9C, 0xFF9C9C9C, 0xFF9C9C9C, 0xFF9C9C9C, 0xFF9C9C9C, 0xFF9C9C9C, 0xFF9C9C9C, 0xFF9F9F9F, 0xFF818181, 0xC45D5D5D,
  0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x004E4E4E, 0x00000000, 0x00000000, 0x04151515, 0x5A535353, 0xF3898989, 0xFFE1E1E1, 0xFFF2F2F2, 0xFFF2F2F2, 0xFFF2F2F2, 0xFFF2F2F2, 0xFFF2F2F2, 0xFFF2F2F2, 
        0xFFF2F2F2, 0xFFF2F2F2, 0xFFF2F2F2, 0xFFF2F2F2, 0xFFF2F2F2, 0xFFF2F2F2, 0xFFF2F2F2, 0xFFF2F2F2, 0xFFF2F2F2, 0xFFF2F2F2, 0xFFF2F2F2, 0xFFF2F2F2, 0xFFF2F2F2, 0xFFF2F2F2, 0xFFF2F2F2, 0xFFF2F2F2, 0xFFF2F2F2, 0xFFF2F2F2, 0xFFF2F2F2, 0xFFF2F2F2, 
        0xFFF2F2F2, 0xFFF2F2F2, 0xFFF2F2F2, 0xFFF2F2F2, 0xFFF2F2F2, 0xFFF2F2F2, 0xFFF2F2F2, 0xFFF2F2F2, 0xFFF2F2F2, 0xFFF2F2F2, 0xFFFCFCFC, 0xFF808080,
  0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x08333333, 0x5F575757, 0xED8A8A8A, 0xFFDADADA, 0xFFECECEC, 0xFFEAEAEA, 0xFFEAEAEA, 0xFFEAEAEA, 0xFFEAEAEA, 0xFFEAEAEA, 0xFFEAEAEA, 
        0xFFEAEAEA, 0xFFEAEAEA, 0xFFEAEAEA, 0xFFEAEAEA, 0xFFEAEAEA, 0xFFEAEAEA, 0xFFEAEAEA, 0xFFEAEAEA, 0xFFEAEAEA, 0xFFEAEAEA, 0xFFEAEAEA, 0xFFEAEAEA, 0xFFEAEAEA, 0xFFEAEAEA, 0xFFEAEAEA, 0xFFEAEAEA, 0xFFEAEAEA, 0xFFEAEAEA, 0xFFEAEAEA, 0xFFEAEAEA, 
        0xFFEAEAEA, 0xFFEAEAEA, 0xFFEAEAEA, 0xFFEAEAEA, 0xFFEAEAEA, 0xFFEAEAEA, 0xFFEAEAEA, 0xFFEAEAEA, 0xFFEAEAEA, 0xFFEAEAEA, 0xFFF0F0F0, 0xFF9C9C9C,
  0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x004B4B4B, 0x00000000, 0x05191919, 0x60575757, 0xEA8C8C8C, 0xFFD4D4D4, 0xFFEAEAEA, 0xFFE8E8E8, 0xFFE8E8E8, 0xFFE8E8E8, 0xFFE8E8E8, 0xFFE8E8E8, 0xFFE8E8E8, 0xFFE8E8E8, 
        0xFFE8E8E8, 0xFFE8E8E8, 0xFFE8E8E8, 0xFFE8E8E8, 0xFFE8E8E8, 0xFFE8E8E8, 0xFFE8E8E8, 0xFFE8E8E8, 0xFFE8E8E8, 0xFFE8E8E8, 0xFFE8E8E8, 0xFFE8E8E8, 0xFFE8E8E8, 0xFFE8E8E8, 0xFFE8E8E8, 0xFFE8E8E8, 0xFFE8E8E8, 0xFFE8E8E8, 0xFFE8E8E8, 0xFFE8E8E8, 
        0xFFE8E8E8, 0xFFE8E8E8, 0xFFE8E8E8, 0xFFE8E8E8, 0xFFE8E8E8, 0xFFE8E8E8, 0xFFE8E8E8, 0xFFE8E8E8, 0xFFE8E8E8, 0xFFE8E8E8, 0xFFEDEDED, 0xFF999999,
  0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x004C4C4C, 0x00000000, 0x031B1B1B, 0x58515151, 0xE88A8A8A, 0xFFD1D1D1, 0xFFE8E8E8, 0xFFE6E6E6, 0xFFE6E6E6, 0xFFE6E6E6, 0xFFE6E6E6, 0xFFE6E6E6, 0xFFE6E6E6, 0xFFE6E6E6, 0xFFE6E6E6, 
        0xFFE6E6E6, 0xFFE6E6E6, 0xFFE6E6E6, 0xFFEFEFEF, 0xFFEFEFEF, 0xFFEFEFEF, 0xFFEFEFEF, 0xFFEFEFEF, 0xFFEFEFEF, 0xFFEFEFEF, 0xFFEFEFEF, 0xFFEFEFEF, 0xFFEFEFEF, 0xFFEFEFEF, 0xFFEFEFEF, 0xFFEFEFEF, 0xFFEFEFEF, 0xFFEFEFEF, 0xFFEFEFEF, 0xFFEFEFEF, 
        0xFFEFEFEF, 0xFFECECEC, 0xFFE7E7E7, 0xFFE6E6E6, 0xFFE6E6E6, 0xFFE6E6E6, 0xFFE6E6E6, 0xFFE6E6E6, 0xFFE6E6E6, 0xFFE6E6E6, 0xFFEBEBEB, 0xFF989898,
  0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x004A4A4A, 0x00000000, 0x01202020, 0x5A4D4D4D, 0xEC858585, 0xFFD2D2D2, 0xFFE6E6E6, 0xFFE4E4E4, 0xFFE4E4E4, 0xFFE4E4E4, 0xFFE4E4E4, 0xFFE4E4E4, 0xFFE4E4E4, 0xFFE4E4E4, 0xFFE4E4E4, 0xFFE4E4E4, 
        0xFFE4E4E4, 0xFFE4E4E4, 0xFFDDDDDD, 0xFF777777, 0xFF7E7E7E, 0xFF7E7E7E, 0xFF7E7E7E, 0xFF7E7E7E, 0xFF7E7E7E, 0xFF7E7E7E, 0xFF7E7E7E, 0xFF7E7E7E, 0xFF7E7E7E, 0xFF7E7E7E, 0xFF7E7E7E, 0xFF7E7E7E, 0xFF7E7E7E, 0xFF7E7E7E, 0xFF7E7E7E, 0xFF7E7E7E, 
        0xFF7B7B7B, 0xFF9B9B9B, 0xFFDADADA, 0xFFE4E4E4, 0xFFE4E4E4, 0xFFE4E4E4, 0xFFE4E4E4, 0xFFE4E4E4, 0xFFE4E4E4, 0xFFE4E4E4, 0xFFE9E9E9, 0xFF969696,
  0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x004A4A4A, 0x00000000, 0x00000000, 0x57494949, 0xF4818181, 0xFFD6D6D6, 0xFFE3E3E3, 0xFFE1E1E1, 0xFFE1E1E1, 0xFFE1E1E1, 0xFFE1E1E1, 0xFFE1E1E1, 0xFFE1E1E1, 0xFFE1E1E1, 0xFFE1E1E1, 0xFFE1E1E1, 0xFFE1E1E1, 
        0xFFE1E1E1, 0xFFE1E1E1, 0xFFE0E0E0, 0xFFC3C3C3, 0xFFBEBEBE, 0xFFBFBFBF, 0xFFBFBFBF, 0xFFBFBFBF, 0xFFBFBFBF, 0xFFBFBFBF, 0xFFBFBFBF, 0xFFBFBFBF, 0xFFBFBFBF, 0xFFBFBFBF, 0xFFBFBFBF, 0xFFBFBFBF, 0xFFBFBFBF, 0xFFBFBFBF, 0xFFBFBFBF, 0xFFBFBFBF, 
        0xFFBEBEBE, 0xFFC3C3C3, 0xFFDCDCDC, 0xFFE2E2E2, 0xFFE1E1E1, 0xFFE1E1E1, 0xFFE1E1E1, 0xFFE1E1E1, 0xFFE1E1E1, 0xFFE1E1E1, 0xFFE6E6E6, 0xFF959595,
  0x00000000, 0x00000000, 0x004C4C4C, 0x005E5E5E, 0x00000000, 0x00000000, 0x51434343, 0xF97D7D7D, 0xFFD9D9D9, 0xFFE0E0E0, 0xFFDFDFDF, 0xFFDFDFDF, 0xFFDFDFDF, 0xFFDFDFDF, 0xFFDFDFDF, 0xFFDFDFDF, 0xFFDFDFDF, 0xFFDFDFDF, 0xFFDFDFDF, 0xFFDFDFDF, 0xFFDFDFDF, 
        0xFFDFDFDF, 0xFFDFDFDF, 0xFFDFDFDF, 0xFFE1E1E1, 0xFFE1E1E1, 0xFFE1E1E1, 0xFFE1E1E1, 0xFFE1E1E1, 0xFFE1E1E1, 0xFFE1E1E1, 0xFFE1E1E1, 0xFFE1E1E1, 0xFFE1E1E1, 0xFFE1E1E1, 0xFFE1E1E1, 0xFFE1E1E1, 0xFFE1E1E1, 0xFFE1E1E1, 0xFFE1E1E1, 0xFFE1E1E1, 
        0xFFE1E1E1, 0xFFE0E0E0, 0xFFDFDFDF, 0xFFDFDFDF, 0xFFDFDFDF, 0xFFDFDFDF, 0xFFDFDFDF, 0xFFDFDFDF, 0xFFDFDFDF, 0xFFDFDFDF, 0xFFE4E4E4, 0xFF949494,
  0x00525252, 0x00000000, 0x00000000, 0x00404040, 0x0A4B4B4B, 0x55484848, 0xF67D7D7D, 0xFFD8D8D8, 0xFFDDDDDD, 0xFFDDDDDD, 0xFFDDDDDD, 0xFFDDDDDD, 0xFFDDDDDD, 0xFFDDDDDD, 0xFFDDDDDD, 0xFFDDDDDD, 0xFFDDDDDD, 0xFFDDDDDD, 0xFFDDDDDD, 0xFFDDDDDD, 0xFFDDDDDD, 
        0xFFDDDDDD, 0xFFDDDDDD, 0xFFD8D8D8, 0xFF8A8A8A, 0xFF8F8F8F, 0xFF8F8F8F, 0xFF8F8F8F, 0xFF8F8F8F, 0xFF8F8F8F, 0xFF8F8F8F, 0xFF8F8F8F, 0xFF8F8F8F, 0xFF8F8F8F, 0xFF8F8F8F, 0xFF8F8F8F, 0xFF8F8F8F, 0xFF8F8F8F, 0xFF8F8F8F, 0xFF8F8F8F, 0xFF8F8F8F, 
        0xFF8D8D8D, 0xFFABABAB, 0xFFD7D7D7, 0xFFDDDDDD, 0xFFDDDDDD, 0xFFDDDDDD, 0xFFDDDDDD, 0xFFDDDDDD, 0xFFDDDDDD, 0xFFDDDDDD, 0xFFE2E2E2, 0xFF939393,
  0x00000000, 0x00000000, 0x014D4D4D, 0x164C4C4C, 0x70545454, 0xF3818181, 0xFFD2D2D2, 0xFFDBDBDB, 0xFFDBDBDB, 0xFFDBDBDB, 0xFFDBDBDB, 0xFFDBDBDB, 0xFFDBDBDB, 0xFFDBDBDB, 0xFFDBDBDB, 0xFFDBDBDB, 0xFFDBDBDB, 0xFFDBDBDB, 0xFFDBDBDB, 0xFFDBDBDB, 0xFFDBDBDB, 
        0xFFDBDBDB, 0xFFDBDBDB, 0xFFD8D8D8, 0xFFACACAC, 0xFFAAAAAA, 0xFFABABAB, 0xFFABABAB, 0xFFABABAB, 0xFFABABAB, 0xFFABABAB, 0xFFABABAB, 0xFFABABAB, 0xFFABABAB, 0xFFABABAB, 0xFFABABAB, 0xFFABABAB, 0xFFABABAB, 0xFFABABAB, 0xFFABABAB, 0xFFABABAB, 
        0xFFAAAAAA, 0xFFB0B0B0, 0xFFD3D3D3, 0xFFDBDBDB, 0xFFDBDBDB, 0xFFDBDBDB, 0xFFDBDBDB, 0xFFDBDBDB, 0xFFDBDBDB, 0xFFDBDBDB, 0xFFDFDFDF, 0xFF929292,
  0x00000000, 0x014A4A4A, 0x1D505050, 0x7F5B5B5B, 0xF0868686, 0xFFCCCCCC, 0xFFDADADA, 0xFFD8D8D8, 0xFFD8D8D8, 0xFFD8D8D8, 0xFFD8D8D8, 0xFFD8D8D8, 0xFFD8D8D8, 0xFFD8D8D8, 0xFFD8D8D8, 0xFFD8D8D8, 0xFFD8D8D8, 0xFFD8D8D8, 0xFFD8D8D8, 0xFFD8D8D8, 0xFFD8D8D8, 
        0xFFD8D8D8, 0xFFD8D8D8, 0xFFD9D9D9, 0xFFE1E1E1, 0xFFE0E0E0, 0xFFE0E0E0, 0xFFE0E0E0, 0xFFE0E0E0, 0xFFE0E0E0, 0xFFE0E0E0, 0xFFE0E0E0, 0xFFE0E0E0, 0xFFE0E0E0, 0xFFE0E0E0, 0xFFE0E0E0, 0xFFE0E0E0, 0xFFE0E0E0, 0xFFE0E0E0, 0xFFE0E0E0, 0xFFE0E0E0, 
        0xFFE1E1E1, 0xFFDFDFDF, 0xFFD9D9D9, 0xFFD8D8D8, 0xFFD8D8D8, 0xFFD8D8D8, 0xFFD8D8D8, 0xFFD8D8D8, 0xFFD8D8D8, 0xFFD8D8D8, 0xFFDDDDDD, 0xFF919191,
  0x013E3E3E, 0x18494949, 0x7E5A5A5A, 0xEF898989, 0xFFC7C7C7, 0xFFD8D8D8, 0xFFD6D6D6, 0xFFD6D6D6, 0xFFD6D6D6, 0xFFD6D6D6, 0xFFD6D6D6, 0xFFD6D6D6, 0xFFD6D6D6, 0xFFD6D6D6, 0xFFD6D6D6, 0xFFD6D6D6, 0xFFD6D6D6, 0xFFD6D6D6, 0xFFD6D6D6, 0xFFD6D6D6, 0xFFD6D6D6, 
        0xFFD6D6D6, 0xFFD6D6D6, 0xFFD3D3D3, 0xFF9C9C9C, 0xFFA0A0A0, 0xFFA0A0A0, 0xFFA0A0A0, 0xFFA0A0A0, 0xFFA0A0A0, 0xFFA0A0A0, 0xFFA0A0A0, 0xFFA0A0A0, 0xFFA0A0A0, 0xFFA0A0A0, 0xFFA0A0A0, 0xFFA0A0A0, 0xFFA0A0A0, 0xFFA0A0A0, 0xFFA0A0A0, 0xFFA0A0A0, 
        0xFF9F9F9F, 0xFFB2B2B2, 0xFFD2D2D2, 0xFFD6D6D6, 0xFFD6D6D6, 0xFFD6D6D6, 0xFFD6D6D6, 0xFFD6D6D6, 0xFFD6D6D6, 0xFFD6D6D6, 0xFFDBDBDB, 0xFF8F8F8F,
  0x164B4B4B, 0x75535353, 0xED848484, 0xFFC5C5C5, 0xFFD6D6D6, 0xFFD4D4D4, 0xFFD4D4D4, 0xFFD4D4D4, 0xFFD4D4D4, 0xFFD4D4D4, 0xFFD4D4D4, 0xFFD4D4D4, 0xFFD4D4D4, 0xFFD4D4D4, 0xFFD4D4D4, 0xFFD4D4D4, 0xFFD4D4D4, 0xFFD4D4D4, 0xFFD4D4D4, 0xFFD4D4D4, 0xFFD4D4D4, 
        0xFFD4D4D4, 0xFFD4D4D4, 0xFFD0D0D0, 0xFF969696, 0xFF989898, 0xFF989898, 0xFF989898, 0xFF989898, 0xFF989898, 0xFF989898, 0xFF989898, 0xFF989898, 0xFF989898, 0xFF989898, 0xFF989898, 0xFF989898, 0xFF989898, 0xFF989898, 0xFF989898, 0xFF989898, 
        0xFF969696, 0xFFA9A9A9, 0xFFCECECE, 0xFFD4D4D4, 0xFFD4D4D4, 0xFFD4D4D4, 0xFFD4D4D4, 0xFFD4D4D4, 0xFFD4D4D4, 0xFFD4D4D4, 0xFFD8D8D8, 0xFF8E8E8E,
  0x72525252, 0xF0808080, 0xFFC5C5C5, 0xFFD4D4D4, 0xFFD2D2D2, 0xFFD2D2D2, 0xFFD2D2D2, 0xFFD2D2D2, 0xFFD2D2D2, 0xFFD2D2D2, 0xFFD2D2D2, 0xFFD2D2D2, 0xFFD2D2D2, 0xFFD2D2D2, 0xFFD2D2D2, 0xFFD2D2D2, 0xFFD2D2D2, 0xFFD2D2D2, 0xFFD2D2D2, 0xFFD2D2D2, 0xFFD2D2D2, 
        0xFFD2D2D2, 0xFFD2D2D2, 0xFFD2D2D2, 0xFFD4D4D4, 0xFFD3D3D3, 0xFFD3D3D3, 0xFFD3D3D3, 0xFFD3D3D3, 0xFFD3D3D3, 0xFFD3D3D3, 0xFFD3D3D3, 0xFFD3D3D3, 0xFFD3D3D3, 0xFFD3D3D3, 0xFFD3D3D3, 0xFFD3D3D3, 0xFFD3D3D3, 0xFFD3D3D3, 0xFFD3D3D3, 0xFFD3D3D3, 
        0xFFD3D3D3, 0xFFD1D1D1, 0xFFD1D1D1, 0xFFD2D2D2, 0xFFD2D2D2, 0xFFD2D2D2, 0xFFD2D2D2, 0xFFD2D2D2, 0xFFD2D2D2, 0xFFD2D2D2, 0xFFD6D6D6, 0xFF8F8F8F,
  0xF6626262, 0xFFCFCFCF, 0xFFD5D5D5, 0xFFD4D4D4, 0xFFD4D4D4, 0xFFD4D4D4, 0xFFD4D4D4, 0xFFD4D4D4, 0xFFD4D4D4, 0xFFD4D4D4, 0xFFD4D4D4, 0xFFD4D4D4, 0xFFD4D4D4, 0xFFD4D4D4, 0xFFD4D4D4, 0xFFD4D4D4, 0xFFD4D4D4, 0xFFD4D4D4, 0xFFD4D4D4, 0xFFD4D4D4, 0xFFD4D4D4, 
        0xFFD4D4D4, 0xFFD4D4D4, 0xFFD4D4D4, 0xFFD4D4D4, 0xFFD4D4D4, 0xFFD4D4D4, 0xFFD4D4D4, 0xFFD4D4D4, 0xFFD4D4D4, 0xFFD4D4D4, 0xFFD4D4D4, 0xFFD4D4D4, 0xFFD4D4D4, 0xFFD4D4D4, 0xFFD4D4D4, 0xFFD4D4D4, 0xFFD4D4D4, 0xFFD4D4D4, 0xFFD4D4D4, 0xFFD4D4D4, 
        0xFFD4D4D4, 0xFFD4D4D4, 0xFFD4D4D4, 0xFFD4D4D4, 0xFFD4D4D4, 0xFFD4D4D4, 0xFFD4D4D4, 0xFFD4D4D4, 0xFFD4D4D4, 0xFFD3D3D3, 0xFFDCDCDC, 0xFF747474,
  0xC4515151, 0xFF747474, 0xFF8E8E8E, 0xFF8C8C8C, 0xFF8C8C8C, 0xFF8C8C8C, 0xFF8C8C8C, 0xFF8C8C8C, 0xFF8C8C8C, 0xFF8C8C8C, 0xFF8C8C8C, 0xFF8C8C8C, 0xFF8C8C8C, 0xFF8C8C8C, 0xFF8C8C8C, 0xFF8C8C8C, 0xFF8C8C8C, 0xFF8C8C8C, 0xFF8C8C8C, 0xFF8C8C8C, 0xFF8C8C8C, 
        0xFF8C8C8C, 0xFF8C8C8C, 0xFF8C8C8C, 0xFF8C8C8C, 0xFF8C8C8C, 0xFF8C8C8C, 0xFF8C8C8C, 0xFF8C8C8C, 0xFF8C8C8C, 0xFF8C8C8C, 0xFF8C8C8C, 0xFF8C8C8C, 0xFF8C8C8C, 0xFF8C8C8C, 0xFF8C8C8C, 0xFF8C8C8C, 0xFF8C8C8C, 0xFF8C8C8C, 0xFF8C8C8C, 0xFF8C8C8C, 
        0xFF8C8C8C, 0xFF8C8C8C, 0xFF8C8C8C, 0xFF8C8C8C, 0xFF8C8C8C, 0xFF8C8C8C, 0xFF8C8C8C, 0xFF8C8C8C, 0xFF8C8C8C, 0xFF8E8E8E, 0xFF747474, 0xC4525252
};

GUI_CONST_STORAGE GUI_BITMAP bmBackButton_53x16 = {
  53, // xSize
  16, // ySize
  212, // BytesPerLine
  32, // BitsPerPixel
  (unsigned char *)_acBackButton_53x16,  // Pointer to picture data
  NULL,  // Pointer to palette
  GUI_DRAW_BMPM8888I
};

/*********************************************************************
*
*       Static code
*
**********************************************************************
*/

/*********************************************************************
*
*       _GetSpecialChar
*
* Function description
*   Returns upper case special characters
*/
static char _GetSpecialChar(char c) {
  switch(c) {
  case '1':
    return '!';
  case '2':
    return '@';
  case '3':
    return '#';
  case '4':
    return '$';
  case '5':
    return '%';
  case '6':
    return '^';
  case '7':
    return '&';
  case '8':
    return '*';
  case '9':
    return '(';
  case '0':
    return ')';
  case '-':
    return '_';
  case '=':
    return '+';
  case ';':
    return ':';
  case '\'':
    return '"';
  case ',':
    return '<';
  case '.':
    return '>';
  case '/':
    return '?';
  case '`':
    return '~';
  default:
    return c;
  }
}

/*********************************************************************
*
*       _DrawCentered
*
* Function description
*   Draws the button bitmap
*/
static void _DrawCentered(WM_HWIN hWin, const GUI_BITMAP * pBM) {
  int xSizeWin;
  int ySizeWin;
  int xSizeBMP;
  int ySizeBMP;
  int xPos;
  int yPos;

  xSizeWin = WM_GetWindowSizeX(hWin);
  ySizeWin = WM_GetWindowSizeY(hWin);
  xSizeBMP = pBM->XSize;
  ySizeBMP = pBM->YSize;
  xPos = (xSizeWin - xSizeBMP) >> 1;
  yPos = (ySizeWin - ySizeBMP) >> 1;
  GUI_DrawBitmap(pBM, xPos, yPos);
}

/*********************************************************************
*
*       _DispSCentered
*
* Function description
*   Draws the button string
*/
static void _DispSCentered(WM_HWIN hWin, const char * s) {
  GUI_RECT Rect;

  Rect.x0 = 0;
  Rect.y0 = 0;
  Rect.x1 = WM_GetWindowSizeX(hWin) - 1;
  Rect.y1 = WM_GetWindowSizeY(hWin) - 1;
  GUI_SetTextMode(GUI_TM_TRANS);
  GUI_SetColor(GUI_BLACK);
  GUI_SetFont(&GUI_Font13_1);
  GUI_DispStringInRect(s, &Rect, GUI_TA_HCENTER | GUI_TA_VCENTER);
}

/*********************************************************************
*
*       _DrawBkSpc
*       _DrawReturn
*       _DrawShift
*/
static void _DrawBkSpc (WM_HWIN hWin) {  _DrawCentered(hWin, &bmBackSpace); }
static void _DrawReturn(WM_HWIN hWin) {  _DrawCentered(hWin, &bmReturn); }
static void _DrawShift (WM_HWIN hWin) {  _DrawCentered(hWin, &bmUp); }
static void _DrawCtrl  (WM_HWIN hWin) {  _DispSCentered(hWin, "Ctrl"); }
static void _DrawAlt   (WM_HWIN hWin) {  _DispSCentered(hWin, "Alt"); }
static void _DrawAltGr (WM_HWIN hWin) {  _DispSCentered(hWin, "Alt"); }

/*********************************************************************
*
*       Static data using function pointers
*
**********************************************************************
*/
static const BUTTON_DATA _aButtonData[] = {
  // First row
  {  23,   4,  36, 36, "1", NULL, 2 },  // !
  {  61,   4,  36, 36, "2", NULL, 2 },  // @
  {  99,   4,  36, 36, "3", NULL, 2 },  // #
  { 137,   4,  36, 36, "4", NULL, 2 },  // $
  { 175,   4,  36, 36, "5", NULL, 2 },  // %
  { 213,   4,  36, 36, "6", NULL, 2 },  // ^
  { 251,   4,  36, 36, "7", NULL, 2 },  // &
  { 289,   4,  36, 36, "8", NULL, 2 },  // *
  { 327,   4,  36, 36, "9", NULL, 2 },  // (
  { 365,   4,  36, 36, "0", NULL, 2 },  // )
  { 403,   4,  64, 36, "\x08", _DrawBkSpc, 0 },
  // Second row
  {  13,  42,  36, 36, "Q", NULL, 1 },
  {  51,  42,  36, 36, "W", NULL, 1 },
  {  89,  42,  36, 36, "E", NULL, 1 },
  { 127,  42,  36, 36, "R", NULL, 1 },
  { 165,  42,  36, 36, "T", NULL, 1 },
  { 203,  42,  36, 36, "Y", NULL, 1 },
  { 241,  42,  36, 36, "U", NULL, 1 },
  { 279,  42,  36, 36, "I", NULL, 1 },
  { 317,  42,  36, 36, "O", NULL, 1 },
  { 355,  42,  36, 36, "P", NULL, 1 },
  { 393,  42,  36, 36, "-", NULL, 2 },  // _
  { 431,  42,  36, 36, "=", NULL, 2 },  // +
  // Third row
  {  23,  80,  36, 36, "A", NULL, 1 },
  {  61,  80,  36, 36, "S", NULL, 1 },
  {  99,  80,  36, 36, "D", NULL, 1 },
  { 137,  80,  36, 36, "F", NULL, 1 },
  { 175,  80,  36, 36, "G", NULL, 1 },
  { 213,  80,  36, 36, "H", NULL, 1 },
  { 251,  80,  36, 36, "J", NULL, 1 },
  { 289,  80,  36, 36, "K", NULL, 1 },
  { 327,  80,  36, 36, "L", NULL, 1 },
  { 365,  80,  36, 36, ";", NULL, 2 },  // :
  { 403,  80,  36, 36, "'", NULL, 2 },  // "
  // Fourth row
  {  13, 118,  36, 36, "\x19", _DrawShift, 0 },
  {  51, 118,  36, 36, "Z", NULL, 1 },
  {  89, 118,  36, 36, "X", NULL, 1 },
  { 127, 118,  36, 36, "C", NULL, 1 },
  { 165, 118,  36, 36, "V", NULL, 1 },
  { 203, 118,  36, 36, "B", NULL, 1 },
  { 241, 118,  36, 36, "N", NULL, 1 },
  { 279, 118,  36, 36, "M", NULL, 1 },
  { 317, 118,  36, 36, ",", NULL, 2 },  // <
  { 355, 118,  36, 36, ".", NULL, 2 },  // >
  { 393, 118,  36, 36, "/", NULL, 2 },  // ?
  { 431, 118,  36, 36, "`", NULL, 2 },  // ~
  // Fifth row
  {  23, 156,  36, 36, "\x1a", _DrawCtrl,   0 },
  {  61, 156,  36, 36, "",     _DrawAlt,    0 },  // no function
  {  99, 156, 226, 36, " ",    NULL,        0 },
  { 327, 156,  36, 36, "",     _DrawAltGr,  0 },  // no function
  { 365, 156,  36, 36, "\x1a", _DrawCtrl,   0 },
  { 403, 156,  64, 36, "\x0d", _DrawReturn, 0 },
};

static GUI_ANIM_HANDLE hAnim;
static ANIM_DATA       AnimData;

/*********************************************************************
*
*       Static code
*
**********************************************************************
*/
/*********************************************************************
*
*       _AnimateWin
*/
static void _AnimateWin(GUI_ANIM_INFO * pInfo, void * pVoid) {
  int         yPos;
  ANIM_DATA * pData;

  pData = (ANIM_DATA *)pVoid;
  if (pData->Dir) {
    yPos = (LCD_GetYSize() - SIZE_Y_KEYPAD) + ((SIZE_Y_KEYPAD * pInfo->Pos) / GUI_ANIM_RANGE);
  } else {
    yPos = LCD_GetYSize() - ((SIZE_Y_KEYPAD * pInfo->Pos) / GUI_ANIM_RANGE);
  }
  WM_MoveTo(pData->hWin,    0, yPos);
  WM_MoveTo(pData->hBackButton, LCD_GetXSize() - BM_BACKBUTTON.XSize, yPos - BM_BACKBUTTON.YSize);
}

/*********************************************************************
*
*       _AnimateDelete
*/
static void _AnimateDelete(void * pVoid) {
  ANIM_DATA * pData;

  pData = (ANIM_DATA *)pVoid;
  pData->Dir ^= 1;
}

/*********************************************************************
*
*       _AnimateCreate
*/
static void _AnimateCreate(GUI_ANIM_HANDLE * pAnim, ANIM_DATA * pAnimData) {
  *pAnim = GUI_ANIM_Create(ANIMATION_TIME, 10, (void *)pAnimData, 0);
  GUI_ANIM_AddItem(*pAnim, 0, ANIMATION_TIME, ANIM_ACCELDECEL, (void *)pAnimData, _AnimateWin);
  GUI_ANIM_StartEx(*pAnim, 1, _AnimateDelete);
}

/*********************************************************************
*
*       _DrawSkinFlex_BUTTON
*
* Function description
*   Skinning routine for keypad buttons
*/
static int _DrawSkinFlex_BUTTON(const WIDGET_ITEM_DRAW_INFO * pDrawItemInfo) {
  static GUI_MEMDEV_Handle ahMemEdges[4];
         GUI_MEMDEV_Handle hMem;
         WM_HWIN           hWin;
         unsigned          i;
         int               xSize;
         int               ySize;
         int               x;
         int               y;
         int               Id;
         int               IsPressed;
         void (* pfDraw)(WM_HWIN hWin);

  x = 0;
  y = 0;
  switch (pDrawItemInfo->Cmd) {
  case WIDGET_ITEM_CREATE:
    if (ahMemEdges[0] == 0) {
      //
      // Create small helper window which never becomes visible
      //
      hWin = WM_CreateWindowAsChild(0, 0, 32, 32, WM_HBKWIN, WM_CF_SHOW | WM_CF_STAYONTOP, NULL, 0);
      WM_SelectWindow(hWin);
      //
      // Create memory devices for the edges of the buttons
      //
      for (i = 0; i < GUI_COUNTOF(ahMemEdges); i++) {
        switch (i) {
        case 0:
          x = 0;
          y = 0;
          break;
        case 1:
          x = -16;
          y = 0;
          break;
        case 2:
          x = -16;
          y = -16;
          break;
        case 3:
          x = 0;
          y = -16;
          break;
        }
        //
        // Create magnified device
        //
        hMem = GUI_MEMDEV_CreateFixed32(0, 0, 16, 16);
        GUI_MEMDEV_Select(hMem);
        GUI_SetBkColor(GUI_TRANSPARENT);
        GUI_Clear();
        GUI_SetColor(COLOR_BORDER);
        GUI_DrawRoundedFrame(x, y, x + 31, y + 31, 16, 4);
        //
        // Shrink to required size
        //
        ahMemEdges[i] = GUI_MEMDEV_CreateFixed32(0, 0, 4, 4);
        GUI_MEMDEV_Select(ahMemEdges[i]);
        GUI_SetBkColor(GUI_TRANSPARENT);
        GUI_Clear();
        GUI_MEMDEV_RotateHQ(hMem, ahMemEdges[i], -6, -6, 0, 250);
        GUI_MEMDEV_Delete(hMem);
        GUI_MEMDEV_Select(0);
      }
      WM_DeleteWindow(hWin);
    }
    break;
  case WIDGET_ITEM_DRAW_TEXT:
    //
    // Use private or default function for drawing bitmap or text
    //
    hWin   = pDrawItemInfo->hWin;
    Id     = WM_GetId(hWin);
    i      = Id - ID_BUTTON;
    pfDraw = _aButtonData[i].pfDraw;
    if (pfDraw) {
      pfDraw(hWin);
    } else {
      BUTTON_DrawSkinFlex(pDrawItemInfo);
    }
    break;
  case WIDGET_ITEM_DRAW_BACKGROUND:
    //
    // Draw background of button
    //
    IsPressed = BUTTON_IsPressed(pDrawItemInfo->hWin);
    xSize     = WM_GetWindowSizeX(pDrawItemInfo->hWin);
    ySize     = WM_GetWindowSizeY(pDrawItemInfo->hWin);
    if (IsPressed) {
      GUI_SetColor(BUTTON_COLOR3);
    } else {
      GUI_SetColor(BUTTON_COLOR0);
    }
    GUI_AA_FillRoundedRect(0, 0, xSize - 1, ySize - 1, 4);
    break;
  default:
    //
    // Use the default skinning routine for processing all other commands
    //
    return BUTTON_DrawSkinFlex(pDrawItemInfo);
  }
  return 0;
}

/*********************************************************************
*
*       _cbBackButton
*/
static void _cbBackButton(WM_MESSAGE * pMsg) {
  static int      Pressed;
  GUI_PID_STATE * pState;
  GUI_RECT        Rect;
  int             IsPressed;
  int             i;

  switch (pMsg->MsgId) {
  case WM_PAINT:
    WM_GetClientRect(&Rect);
    Rect.y1 += 10;
    IsPressed = BUTTON_IsPressed(pMsg->hWin);
    if (IsPressed) {
      GUI_SetColor(BUTTON_COLOR3);
    } else {
      GUI_SetColor(BUTTON_COLOR0);
    }
    GUI_AA_FillRoundedRectEx(&Rect, 4);
    GUI_SetColor(COLOR_BORDER);
    GUI_AA_DrawRoundedRectEx(&Rect, 4);
    for (i = 3; i <= 7; i += 2) {
      GUI_DrawHLine(5 + i, 15, Rect.x1 - 15);
    }
    break;
  case WM_TOUCH:
    pState = (GUI_PID_STATE *)pMsg->Data.p;
    if (pState) {
      if (pState->Pressed && Pressed == 0) {
        _AnimateCreate(&hAnim, &AnimData);
        Pressed = 1;
      } else {
        Pressed = 0;
      }
    }
    BUTTON_Callback(pMsg);
    break;
  default:
    BUTTON_Callback(pMsg);
  }
}

/*********************************************************************
*
*       _ChangeButtonString
*/
static void _ChangeButtonString(WM_MESSAGE * pMsg, int HigherLower) {
  WM_HWIN hButton;
  U32     i;
  char    acBuffer[2];

  for (i = 0; i < GUI_COUNTOF(_aButtonData); i++) {
    if (_aButtonData[i].HasLowerCase == 0) {
      continue;
    }
    acBuffer[1] = '\0';
    hButton = WM_GetDialogItem(pMsg->hWin, ID_BUTTON + i);
    if(_aButtonData[i].HasLowerCase == 2 && !HigherLower) {
      acBuffer[0] = _GetSpecialChar(_aButtonData[i].acLabel[0]);
      goto SetText;
    }
    acBuffer[0] = _aButtonData[i].acLabel[0];
    if (HigherLower) {
      acBuffer[0] |= 0x20;
    }
SetText:
    BUTTON_SetText(hButton, acBuffer);
  }
}

/*********************************************************************
*
*       _cbKeyPad
*
* Function description
*   Callback routine of keypad window
*/
static void _cbKeyPad(WM_MESSAGE * pMsg) {
  static int      LowerCase;
         WM_HWIN  hWin;
         WM_HWIN  hButton;
         char     c;
         int      Id;
         int      NCode;
         unsigned i;

  hWin = pMsg->hWin;
  switch (pMsg->MsgId) {
  case APP_INIT_LOWERCASE:
    LowerCase = 0;
    break;
  case WM_CREATE:
    //
    // Create the keyboard buttons
    //
    for (i = 0; i < GUI_COUNTOF(_aButtonData); i++) {
      hButton = BUTTON_CreateEx(_aButtonData[i].xPos, _aButtonData[i].yPos, _aButtonData[i].xSize, _aButtonData[i].ySize, 
                                hWin, WM_CF_SHOW | WM_CF_HASTRANS, 0, ID_BUTTON + i);
      BUTTON_SetText(hButton, _aButtonData[i].acLabel);
      BUTTON_SetFocussable(hButton, 0);
      BUTTON_SetSkin(hButton, _DrawSkinFlex_BUTTON);
      BUTTON_SetFont(hButton, &GUI_Font13_1);
    }
    LowerCase = 0;
    _ChangeButtonString(pMsg, LowerCase);
    AnimData.hBackButton = BUTTON_CreateEx(WM_GetWindowSizeX(pMsg->hWin) - BM_BACKBUTTON.XSize,
                              WM_GetWindowOrgY(pMsg->hWin) - BM_BACKBUTTON.YSize,
                              BM_BACKBUTTON.XSize,
                              BM_BACKBUTTON.YSize, 
                              WM_GetParent(hWin),
                              WM_CF_SHOW | WM_CF_HASTRANS,
                              0,
                              ID_BUTTON + i);
    BUTTON_SetFocussable(AnimData.hBackButton, 0);
    WM_SetCallback(AnimData.hBackButton, _cbBackButton);
    AnimData.Dir  = 0;
    AnimData.hWin = pMsg->hWin;
    break;
  case WM_PAINT:
    //
    // Draw background
    //
    GUI_SetBkColor(COLOR_BORDER);
    GUI_Clear();
    break;
  case MSG_ANIMATE:
    if (AnimData.Dir != pMsg->Data.v) {
      _AnimateCreate(&hAnim, &AnimData);
    }
    break;
  case WM_DELETE:
    WM_DeleteWindow(AnimData.hBackButton);
    break;
  case WM_NOTIFY_PARENT:
    //
    // Send key message to currently focused window
    //
    Id    = WM_GetId(pMsg->hWinSrc);
    NCode = pMsg->Data.v;
    switch (NCode) {
    case WM_NOTIFICATION_RELEASED:
      if (_aButtonData[Id - ID_BUTTON].acLabel) {
        c = _aButtonData[Id - ID_BUTTON].acLabel[0];        
        switch (c) {
        case '\0':
          return;
        case GUI_KEY_SHIFT:
          LowerCase ^= 1;
          _ChangeButtonString(pMsg, LowerCase);
          break;
        case GUI_KEY_ENTER:
          _AnimateCreate(&hAnim, &AnimData);
          LowerCase = 0;
          _ChangeButtonString(pMsg, LowerCase);
          WM_SendMessageNoPara(WM_GetParent(pMsg->hWin), MSG_DO_TRACE);
          break;
        case GUI_KEY_CONTROL:
          return;
        case GUI_KEY_BACKSPACE:
          //LowerCase = 0;
          //_ChangeButtonString(pMsg, LowerCase);
          break;
        default:
          if (LowerCase) {
            if ((c < '0') || (c > '9')) {
              c |= 0x20;
            }
          } else {
            if(((c >= '0') && (c <= '9')) || (c == '-') || (c == '=') || (c == ';') || (c == '\'') || (c == ',') || (c == '.') || (c == '/') || (c == '`')) {
              c = _GetSpecialChar(c);
            }
          }
          break;
        }
        GUI_StoreKeyMsg(c, 1);
        GUI_StoreKeyMsg(c, 0);
        break;
      }
    }
  }
}

/*********************************************************************
*
*       Public code
*
**********************************************************************
*/
WM_HWIN GUI_CreateKeyPad(WM_HWIN hParent) {
  return WM_CreateWindowAsChild(0, LCD_GetYSize(), LCD_GetXSize(), SIZE_Y_KEYPAD, hParent, WM_CF_SHOW | WM_CF_STAYONTOP, _cbKeyPad, 0);
}
#endif
/*************************** End of file ****************************/
