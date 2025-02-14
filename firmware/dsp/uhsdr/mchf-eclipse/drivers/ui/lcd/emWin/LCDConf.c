/*********************************************************************
*          Portions COPYRIGHT 2016 STMicroelectronics                *
*          Portions SEGGER Microcontroller GmbH & Co. KG             *
*        Solutions for real time microcontroller applications        *
**********************************************************************
*                                                                    *
*        (c) 1996 - 2015  SEGGER Microcontroller GmbH & Co. KG       *
*                                                                    *
*        Internet: www.segger.com    Support:  support@segger.com    *
*                                                                    *
**********************************************************************

** emWin V5.32 - Graphical user interface for embedded applications **
All  Intellectual Property rights  in the Software belongs to  SEGGER.
emWin is protected by  international copyright laws.  Knowledge of the
source code may not be used to write a similar product.  This file may
only be used in accordance with the following terms:

The  software has  been licensed  to STMicroelectronics International
N.V. a Dutch company with a Swiss branch and its headquarters in Plan-
les-Ouates, Geneva, 39 Chemin du Champ des Filles, Switzerland for the
purposes of creating libraries for ARM Cortex-M-based 32-bit microcon_
troller products commercialized by Licensee only, sublicensed and dis_
tributed under the terms and conditions of the End User License Agree_
ment supplied by STMicroelectronics International N.V.
Full source code is available at: www.segger.com

We appreciate your understanding and fairness.
----------------------------------------------------------------------
File        : LCDConf_FlexColor_Template.c
Purpose     : Display controller configuration (single layer)
---------------------------END-OF-HEADER------------------------------
*/

/**
  ******************************************************************************
  * @attention
  *
  * Licensed under MCD-ST Liberty SW License Agreement V2, (the "License");
  * You may not use this file except in compliance with the License.
  * You may obtain a copy of the License at:
  *
  *        http://www.st.com/software_license_agreement_liberty_v2
  *
  * Unless required by applicable law or agreed to in writing, software
  * distributed under the License is distributed on an "AS IS" BASIS,
  * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  * See the License for the specific language governing permissions and
  * limitations under the License.
  *
  ******************************************************************************
  */

#include "uhsdr_board.h"
#include "uhsdr_board_config.h"

#include "ui_lcd_hy28.h"

#ifdef USE_EMWIN

#include "GUI.h"
#include "GUIDRV_FlexColor.h"

#include "GUIConf.h"
#include "LCDConf.h"

#define XSIZE_PHYS  240 // To be adapted to x-screen size
#define YSIZE_PHYS  320 // To be adapted to y-screen size

#ifndef   VXSIZE_PHYS
  #define VXSIZE_PHYS XSIZE_PHYS
#endif
#ifndef   VYSIZE_PHYS
  #define VYSIZE_PHYS YSIZE_PHYS
#endif
#ifndef   XSIZE_PHYS
  #error Physical X size of display is not defined!
#endif
#ifndef   YSIZE_PHYS
  #error Physical Y size of display is not defined!
#endif
#ifndef   GUICC_565
  #error Color conversion not defined!
#endif
#ifndef   GUIDRV_FLEXCOLOR
  #error No display driver defined!
#endif

#define LCD_REG      (*((volatile unsigned short *) 0x60000000))
#define LCD_RAM      (*((volatile unsigned short *) 0x60020000))

// --------------------------------------------------------------------------
// Parallel only test
static void LcdWriteReg(U16 Data)
{
	LCD_REG = Data;
	__DMB();
}

static inline void LcdWriteData(U16 Data)
{
	LCD_REG = Data;
	__DMB();
}

static inline void LcdWriteDataMultiple(U16 * pData, int NumItems)
{
    while (NumItems--)
    {
    	LCD_RAM = *pData++;;
    	__DMB();
    }
}
// -------------------------------------------------------------------------

void LCD_X_Config(void)
{
	GUI_DEVICE 			*pDevice;
	CONFIG_FLEXCOLOR 	Config = {0};
	GUI_PORT_API 		PortAPI = {0};

	// Set display driver and color conversion
	pDevice = GUI_DEVICE_CreateAndLink(GUIDRV_FLEXCOLOR, GUICC_565, 0, 0);

	// Display driver configuration, required for Lin-driver
	LCD_SetSizeEx    (0, XSIZE_PHYS,   YSIZE_PHYS);
	LCD_SetVSizeEx   (0, VXSIZE_PHYS,  VYSIZE_PHYS);

	// Orientation
	Config.Orientation = GUI_SWAP_XY;
	GUIDRV_FlexColor_Config(pDevice, &Config);

	PortAPI.pfWrite16_A0  = LcdWriteReg;
	PortAPI.pfWrite16_A1  = LcdWriteData;
	PortAPI.pfWriteM16_A1 = LcdWriteDataMultiple;

	// Set controller and operation mode
	GUIDRV_FlexColor_SetFunc(pDevice, &PortAPI, GUIDRV_FLEXCOLOR_F66708, GUIDRV_FLEXCOLOR_M16C0B16);
}

int LCD_X_DisplayDriver(unsigned LayerIndex, unsigned Cmd, void * pData)
{
	int r = 0;

	(void) LayerIndex;
	(void) pData;

	switch (Cmd)
	{
		case LCD_X_INITCONTROLLER:
		{
			// already done
			return 0;
		}

		default:
			r = -1;
	}

	return r;
}
#endif
/*************************** End of file ****************************/
