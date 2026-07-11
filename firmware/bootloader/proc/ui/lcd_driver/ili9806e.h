//************************************************************************************
//**                                                                                 **
//**                                 mcHF QRP Transceiver                            **
//**                         Krassi Atanassov - M0NKA, 2013-2026                     **
//**                                                                                 **
//**---------------------------------------------------------------------------------**
//**                                                                                 **
//**  File name:		ili9806e.h                                                   **
//**  Description:  	ILI9806E panel init (KD043WVFIA083, 480x800, MIPI DSI)       **
//**  Licence:			https://github.com/m0nka/mcHF/blob/main/LICENSE            **
//************************************************************************************
#ifndef __ILI9806E_H
#define __ILI9806E_H

// ----------------------------------------------------------------------------------
// Panel timing, same values as the radio firmware (app_proc)
#define  ILI9806E_VSYNC             			2
#define  ILI9806E_VBP               			20
#define  ILI9806E_VFP            		   		10
#define  ILI9806E_HEIGHT            			800
//
#define  ILI9806E_HSYNC			            	4
#define  ILI9806E_HBP               			33
#define  ILI9806E_HFP               			60
#define  ILI9806E_WIDTH             			480

// Clocks in kHz - 54.167MHz byte clock, exact 2:1 against the
// 27.083MHz PLL3 pixel clock (same as the radio firmware)
#define  LCD_LANE_CLK							54000
#define  ILI9806E_PIXEL_CLK  					(LCD_LANE_CLK/2)

int ili9806e_init(void);

#endif
