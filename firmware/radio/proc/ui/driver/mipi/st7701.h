/************************************************************************************
**                                                                                 **
**                             mcHF Pro QRP Transceiver                            **
**                         Krassi Atanassov - M0NKA, 2013-2021                     **
**                                                                                 **
**---------------------------------------------------------------------------------**
**                                                                                 **
**  File name:                                                                     **
**  Description:                                                                   **
**  Last Modified:                                                                 **
**  Licence:       The mcHF project is released for radio amateurs experimentation **
**               and non-commercial use only.Check 3rd party drivers for licensing **
************************************************************************************/
#ifndef __ST7701_H
#define __ST7701_H

#include "mchf_pro_board.h"

#if (LCD_LANE_CLK == 62500)
// 31.250Mhz, 53Hz
#define  ST7701_VSYNC             			18
#define  ST7701_VBP               			80
#define  ST7701_VFP            		   		40
#define  ST7701_HEIGHT            			854
//
#define  ST7701_HSYNC			            8
#define  ST7701_HBP               		    10
#define  ST7701_HFP               			56
#define  ST7701_WIDTH             			480
#else
#ifdef STARTEK_5INCH
// 29.375MHz, 60Hz achievable with commented values, but left screen corner cut off ;(
#define  ST7701_VSYNC             			18		//   - 4 -  8	2
#define  ST7701_VBP               			80		// 30/20/12 10  38
#define  ST7701_VFP            		   		40		// 20/30/40 40  12
#define  ST7701_HEIGHT            			854
//
#define  ST7701_HSYNC			            8		// 1-2
#define  ST7701_HBP               		    10		// 10
#define  ST7701_HFP               			56		// 30
#define  ST7701_WIDTH             			480
#endif
#endif
// ----------------------------------------------------------------------------------
#ifdef STARTEK_43INCH
#define  ST7701_VSYNC             			((uint16_t)4)		// 4
#define  ST7701_VBP               			((uint16_t)20)		// 20
#define  ST7701_VFP            		   		((uint16_t)10)		// 10
#define  ST7701_HEIGHT            			800
//
#define  ST7701_HSYNC			            ((uint16_t)4)		// 4
#define  ST7701_HBP               			((uint16_t)10)		// 10
#define  ST7701_HFP               			((uint16_t)45)		// 45
#define  ST7701_WIDTH             			480
#endif

#define BITS_PER_LONG 						32
#define BIT(nr)								(1UL << (nr))

#define GENMASK(h, l) (((~0UL) << (l)) & (~0UL >> (BITS_PER_LONG - 1 - (h))))

#define DSI_CMD2BKX_SEL					0xFF

#define DSI_CMD2_BK0_PVGAMCTRL			0xB0 /* Positive Voltage Gamma Control */
#define DSI_CMD2_BK0_NVGAMCTRL			0xB1 /* Negative Voltage Gamma Control */
#define DSI_CMD2_BK0_LNESET				0xC0 /* Display Line setting */
#define DSI_CMD2_BK0_PORCTRL			0xC1 /* Porch control */
#define DSI_CMD2_BK0_INVSEL				0xC2 /* Inversion selection, Frame Rate Control */

#define DSI_CMD2_BK1_VRHS				0xB0 /* Vop amplitude setting */
#define DSI_CMD2_BK1_VCOM				0xB1 /* VCOM amplitude setting */
#define DSI_CMD2_BK1_VGHSS				0xB2 /* VGH Voltage setting */
#define DSI_CMD2_BK1_TESTCMD			0xB3 /* TEST Command Setting */
#define DSI_CMD2_BK1_VGLS				0xB5 /* VGL Voltage setting */
#define DSI_CMD2_BK1_PWCTLR1			0xB7 /* Power Control 1 */
#define DSI_CMD2_BK1_PWCTLR2			0xB8 /* Power Control 2 */
#define DSI_CMD2_BK1_SPD1				0xC1 /* Source pre_drive timing set1 */
#define DSI_CMD2_BK1_SPD2				0xC2 /* Source EQ2 Setting */
#define DSI_CMD2_BK1_MIPISET1			0xD0 /* MIPI Setting 1 */

#define DSI_CMD2BK1_SEL					0x11
#define DSI_CMD2BK0_SEL					0x10
#define DSI_CMD2BKX_SEL_NONE			0x00

#define DSI_LINESET_LINE				0x69
#define DSI_LINESET_LDE_EN				BIT(7)
#define DSI_LINESET_LINEDELTA			GENMASK(1, 0)
#define DSI_CMD2_BK0_LNESET_B1			DSI_LINESET_LINEDELTA
#define DSI_CMD2_BK0_LNESET_B0			(DSI_LINESET_LDE_EN | DSI_LINESET_LINE)
#define DSI_INVSEL_DEFAULT				GENMASK(5, 4)
#define DSI_INVSEL_NLINV				GENMASK(2, 0)
#define DSI_INVSEL_RTNI					GENMASK(2, 1)
#define DSI_CMD2_BK0_INVSEL_B1			DSI_INVSEL_RTNI
#define DSI_CMD2_BK0_INVSEL_B0			(DSI_INVSEL_DEFAULT | DSI_INVSEL_NLINV)

#define DSI_CMD2_BK0_PORCTRL_B0			((ST7701_HEIGHT + ST7701_VFP + ST7701_VBP + ST7701_VSYNC) - (ST7701_HEIGHT + ST7701_VFP + ST7701_VBP))
#define DSI_CMD2_BK0_PORCTRL_B1			((ST7701_HEIGHT + ST7701_VFP) - ST7701_HEIGHT)

#define DSI_CMD2_BK1_VRHA_SET			0x45
#define DSI_CMD2_BK1_VCOM_SET			0x13
#define DSI_CMD2_BK1_VGHSS_SET			GENMASK(2, 0)
#define DSI_CMD2_BK1_TESTCMD_VAL		BIT(7)
#define DSI_VGLS_DEFAULT				BIT(6)
#define DSI_VGLS_SEL					GENMASK(2, 0)
#define DSI_CMD2_BK1_VGLS_SET			(DSI_VGLS_DEFAULT | DSI_VGLS_SEL)
#define DSI_PWCTLR1_AP					BIT(7) /* Gamma OP bias, max */
#define DSI_PWCTLR1_APIS				BIT(2) /* Source OP input bias, min */
#define DSI_PWCTLR1_APOS				BIT(0) /* Source OP output bias, min */
#define DSI_CMD2_BK1_PWCTLR1_SET		(DSI_PWCTLR1_AP | DSI_PWCTLR1_APIS | DSI_PWCTLR1_APOS)
#define DSI_PWCTLR2_AVDD				BIT(5) /* AVDD 6.6v */
#define DSI_PWCTLR2_AVCL				0x0    /* AVCL -4.4v */
#define DSI_CMD2_BK1_PWCTLR2_SET		(DSI_PWCTLR2_AVDD | DSI_PWCTLR2_AVCL)
#define DSI_SPD1_T2D					BIT(3)
#define DSI_CMD2_BK1_SPD1_SET			(GENMASK(6, 4) | DSI_SPD1_T2D)
#define DSI_CMD2_BK1_SPD2_SET			DSI_CMD2_BK1_SPD1_SET
#define DSI_MIPISET1_EOT_EN				BIT(3)
#define DSI_CMD2_BK1_MIPISET1_SET		(BIT(7) | DSI_MIPISET1_EOT_EN)

#define MIPI_DCS_SOFT_RESET				0x01
#define MIPI_DCS_EXIT_SLEEP_MODE 		0x11

int ST7701S_Init(unsigned long ColorCoding);

#endif
