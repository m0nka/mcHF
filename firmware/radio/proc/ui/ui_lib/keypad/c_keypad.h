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
File        : DIALOG_Keypad.h
Purpose     : Header for keypad dialog.
Requirements: WindowManager - (x)
              MemoryDevices - ( )
              AntiAliasing  - ( )
              VNC-Server    - ( )
              PNG-Library   - ( )
              TrueTypeFonts - ( )
----------------------------------------------------------------------
*/

#include <stddef.h>
#include "DIALOG.h"

/*********************************************************************
*
*       Defines
*
**********************************************************************
*/
#define COLOR_BACK0            GUI_MAKE_COLOR(0xFF3333)
#define COLOR_BACK1            GUI_MAKE_COLOR(0x550000)
#define COLOR_BORDER           GUI_MAKE_COLOR(0x444444)
#define COLOR_KEYPAD0          GUI_MAKE_COLOR(0xAAAAAA)
#define COLOR_KEYPAD1          GUI_MAKE_COLOR(0x555555)
#define BUTTON_COLOR0          GUI_MAKE_COLOR(0xEEEEEE)
#define BUTTON_COLOR1          GUI_MAKE_COLOR(0xCCCCCC)
#define BUTTON_COLOR2          GUI_MAKE_COLOR(0xCCCCCC)
#define BUTTON_COLOR3          GUI_MAKE_COLOR(0xAAAAAA)
#define COLOR_BLUE             GUI_MAKE_COLOR(0x00865B00)
#define BUTTON_SKINFLEX_RADIUS 4
#define ID_BUTTON              (GUI_ID_USER + 0)
#define APP_INIT_LOWERCASE     (WM_USER + 0)
#define MSG_DO_TRACE           (WM_USER + 0x01)
#define MSG_ANIMATE            (WM_USER + 0x02)
#define ANIMATION_TIME         300
#define BM_BACKBUTTON          bmBackButton_53x16
#define SIZE_Y_KEYPAD          (194)

/*********************************************************************
*
*       Types
*
**********************************************************************
*/
typedef struct {
  int          xPos;
  int          yPos;
  int          xSize;
  int          ySize;
  const char * acLabel;
  void      (* pfDraw)(WM_HWIN hWin);
  int          HasLowerCase;
} BUTTON_DATA;

typedef struct {
  BUTTON_SKINFLEX_PROPS * pProp;
  int                     Index;
  BUTTON_SKINFLEX_PROPS   PropOld;
} BUTTON_PROP;

typedef struct {
  WM_HWIN hWin;
  WM_HWIN hBackButton;
  int     Dir;
} ANIM_DATA;

/*********************************************************************
*
*       Prototypes
*
**********************************************************************
*/
WM_HWIN GUI_CreateKeyPad(WM_HWIN hParent);

/*************************** End of file ****************************/
