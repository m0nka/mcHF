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

// Mismatch between LCD pixel clock and porch values
// will cause colors on this controller to be swapped
//
//#define	 ST7701_PIXEL_CLK  					27429		// 29000

#define  ST7701_WIDTH             			480

#ifndef USE_LCD_BAREMETAL
#define  ST7701_HEIGHT            			854
#else
#define  ST7701_HEIGHT            			480
#endif

// Startek 5 inch
#if defined(USE_LCD_BAREMETAL) && defined (STARTEK_5INCH)
#define  ST7701_VSYNC             			((uint16_t)1)		// Vertical synchronization   - DO NOT TOUCH!!
#define  ST7701_VBP               			((uint16_t)20)     	// Vertical back porch
#define  ST7701_VFP            		   		((uint16_t)10)     	// Vertical front porch
//
#define  ST7701_HSYNC			            ((uint16_t)2)     	// Horizontal synchronization
#define  ST7701_HBP               			((uint16_t)60)     	// Horizontal back porch
#define  ST7701_HFP               			((uint16_t)10)     	// Horizontal front porch
#endif

// Startek 4.3 inch(ILI9806, Note: orginal timing crashes the driver!)
#if defined(USE_LCD_BAREMETAL) && defined (STARTEK_43INCH)
#if 1
// Not fading, but mirror image(bad porch control ?)
#define  ST7701_VSYNC             			((uint16_t)1)		// 4
#define  ST7701_VBP               			((uint16_t)20)		// 20
#define  ST7701_VFP            		   		((uint16_t)10)		// 10
//
#define  ST7701_HSYNC			            ((uint16_t)4)		// 4
#define  ST7701_HBP               			((uint16_t)10)		// 10
#define  ST7701_HFP               			((uint16_t)45)		// 45
#else
// Image fits, but fades after a while
#define  ST7701_VSYNC             			((uint16_t)1)
#define  ST7701_VBP               			((uint16_t)120)
#define  ST7701_VFP            		   		((uint16_t)240)
//
#define  ST7701_HSYNC			            ((uint16_t)1)
#define  ST7701_HBP               			((uint16_t)1)
#define  ST7701_HFP               			((uint16_t)1)
#endif
#endif

// Startek 3.5 inch
#if defined(USE_LCD_BAREMETAL) && defined (STARTEK_35INCH)
#define  ST7701_VSYNC             			((uint16_t)1)
#define  ST7701_VBP               			((uint16_t)20)
#define  ST7701_VFP            		   		((uint16_t)10)
//
#define  ST7701_HSYNC			            ((uint16_t)2)
#define  ST7701_HBP               			((uint16_t)60)
#define  ST7701_HFP               			((uint16_t)10)
#endif

#ifndef USE_LCD_BAREMETAL
#define  ST7701_VSYNC             			18
#define  ST7701_VBP               			86
#define  ST7701_VFP            		   		46
//
#define  ST7701_HSYNC			            2
#define  ST7701_HBP               		    10
#define  ST7701_HFP               			60
#endif
// ----------------------------------------------------------------------------------

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

#ifndef USE_LCD_BAREMETAL
#define DSI_LINESET_LINE				0x69
#else
#define DSI_LINESET_LINE				0x3B
#endif

#define DSI_LINESET_LDE_EN				BIT(7)
#define DSI_LINESET_LINEDELTA			GENMASK(1, 0)
#define DSI_CMD2_BK0_LNESET_B1			DSI_LINESET_LINEDELTA
#define DSI_CMD2_BK0_LNESET_B0			(DSI_LINESET_LDE_EN | DSI_LINESET_LINE)
#define DSI_INVSEL_DEFAULT				GENMASK(5, 4)
#define DSI_INVSEL_NLINV				GENMASK(2, 0)
#define DSI_INVSEL_RTNI					GENMASK(2, 1)
#define DSI_CMD2_BK0_INVSEL_B1			DSI_INVSEL_RTNI
#define DSI_CMD2_BK0_INVSEL_B0			(DSI_INVSEL_DEFAULT | DSI_INVSEL_NLINV)

#define DSI_CMD2_BK0_PORCTRL_B0			((ST7701_HEIGHT + ST7701_VFP + ST7701_VBP + ST7701_VSYNC - 1) - (ST7701_HEIGHT + ST7701_VFP + ST7701_VBP - 1))
#define DSI_CMD2_BK0_PORCTRL_B1			((ST7701_HEIGHT + ST7701_VFP - 1) - ST7701_HEIGHT - 1)

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


#define OTM8009A_ORIENTATION_PORTRAIT    ((uint32_t)0x00) /* Portrait orientation choice of LCD screen  */
#define OTM8009A_ORIENTATION_LANDSCAPE   ((uint32_t)0x01) /* Landscape orientation choice of LCD screen */

#define OTM8009A_FORMAT_RGB888    ((uint32_t)0x00) /* Pixel format chosen is RGB888 : 24 bpp */
#define OTM8009A_FORMAT_RBG565    ((uint32_t)0x02) /* Pixel format chosen is RGB565 : 16 bpp */

/* List of OTM8009A used commands                                  */
/* Detailed in OTM8009A Data Sheet 'DATA_SHEET_OTM8009A_V0 92.pdf' */
/* Version of 14 June 2012                                         */
#define  OTM8009A_CMD_NOP                   0x00  /* NOP command      */
#define  OTM8009A_CMD_SWRESET               0x01  /* Sw reset command */
#define  OTM8009A_CMD_RDDMADCTL             0x0B  /* Read Display MADCTR command : read memory display access ctrl */
#define  OTM8009A_CMD_RDDCOLMOD             0x0C  /* Read Display pixel format */
#define  OTM8009A_CMD_SLPIN                 0x10  /* Sleep In command */
#define  OTM8009A_CMD_SLPOUT                0x11  /* Sleep Out command */
#define  OTM8009A_CMD_PTLON                 0x12  /* Partial mode On command */

#define  OTM8009A_CMD_DISPOFF               0x28  /* Display Off command */
#define  OTM8009A_CMD_DISPON                0x29  /* Display On command */

#define  OTM8009A_CMD_CASET                 0x2A  /* Column address set command */
#define  OTM8009A_CMD_PASET                 0x2B  /* Page address set command */

#define  OTM8009A_CMD_RAMWR                 0x2C  /* Memory (GRAM) write command */
#define  OTM8009A_CMD_RAMRD                 0x2E  /* Memory (GRAM) read command  */

#define  OTM8009A_CMD_PLTAR                 0x30  /* Partial area command (4 parameters) */

#define  OTM8009A_CMD_TEOFF                 0x34  /* Tearing Effect Line Off command : command with no parameter */

#define  OTM8009A_CMD_TEEON                 0x35  /* Tearing Effect Line On command : command with 1 parameter 'TELOM' */

/* Parameter TELOM : Tearing Effect Line Output Mode : possible values */
#define OTM8009A_TEEON_TELOM_VBLANKING_INFO_ONLY            0x00
#define OTM8009A_TEEON_TELOM_VBLANKING_AND_HBLANKING_INFO   0x01

#define  OTM8009A_CMD_MADCTR                0x36  /* Memory Access write control command  */

/* Possible used values of MADCTR */
#define OTM8009A_MADCTR_MODE_PORTRAIT       0x00
#define OTM8009A_MADCTR_MODE_LANDSCAPE      0x60  /* MY = 0, MX = 1, MV = 1, ML = 0, RGB = 0 */

#define  OTM8009A_CMD_IDMOFF                0x38  /* Idle mode Off command */
#define  OTM8009A_CMD_IDMON                 0x39  /* Idle mode On command  */

#define  OTM8009A_CMD_COLMOD                0x3A  /* Interface Pixel format command */

/* Possible values of COLMOD parameter corresponding to used pixel formats */
#define  OTM8009A_COLMOD_RGB565             0x55
#define  OTM8009A_COLMOD_RGB888             0x77

#define  OTM8009A_CMD_RAMWRC                0x3C  /* Memory write continue command */
#define  OTM8009A_CMD_RAMRDC                0x3E  /* Memory read continue command  */

#define  OTM8009A_CMD_WRTESCN               0x44  /* Write Tearing Effect Scan line command */
#define  OTM8009A_CMD_RDSCNL                0x45  /* Read  Tearing Effect Scan line command */

/* CABC Management : ie : Content Adaptive Back light Control in IC OTM8009a */
#define  OTM8009A_CMD_WRDISBV               0x51  /* Write Display Brightness command          */
#define  OTM8009A_CMD_WRCTRLD               0x53  /* Write CTRL Display command                */
#define  OTM8009A_CMD_WRCABC                0x55  /* Write Content Adaptive Brightness command */
#define  OTM8009A_CMD_WRCABCMB              0x5E  /* Write CABC Minimum Brightness command     */


#ifdef USE_LCD_BAREMETAL

typedef struct
{
  uint32_t  Orientation;
  uint32_t  ColorCode;
  uint32_t  Brightness;
} OTM8009A_LCD_Ctx_t;

typedef struct
{
  uint16_t                  Address;
  //OTM8009A_WriteReg_Func    WriteReg;
  //OTM8009A_ReadReg_Func     ReadReg;
  //OTM8009A_GetTick_Func     GetTick;
} OTM8009A_IO_t;

typedef struct
{
  OTM8009A_IO_t         IO;
  //otm8009a_ctx_t        Ctx;
  uint8_t               IsInitialized;
} OTM8009A_Object_t;

typedef struct
{
  /* Control functions */
  int32_t (*Init             )(OTM8009A_Object_t*, uint32_t, uint32_t);
  int32_t (*DeInit           )(OTM8009A_Object_t*);
  int32_t (*ReadID           )(OTM8009A_Object_t*, uint32_t*);
  int32_t (*DisplayOn        )(OTM8009A_Object_t*);
  int32_t (*DisplayOff       )(OTM8009A_Object_t*);
  int32_t (*SetBrightness    )(OTM8009A_Object_t*, uint32_t);
  int32_t (*GetBrightness    )(OTM8009A_Object_t*, uint32_t*);
  int32_t (*SetOrientation   )(OTM8009A_Object_t*, uint32_t);
  int32_t (*GetOrientation   )(OTM8009A_Object_t*, uint32_t*);

  /* Drawing functions*/
  int32_t ( *SetCursor       ) (OTM8009A_Object_t*, uint32_t, uint32_t);
  int32_t ( *DrawBitmap      ) (OTM8009A_Object_t*, uint32_t, uint32_t, uint8_t *);
  int32_t ( *FillRGBRect     ) (OTM8009A_Object_t *pObj, uint32_t Xpos, uint32_t Ypos, uint8_t *pData, uint32_t Width, uint32_t Height);
  int32_t ( *DrawHLine       ) (OTM8009A_Object_t*, uint32_t, uint32_t, uint32_t, uint32_t);
  int32_t ( *DrawVLine       ) (OTM8009A_Object_t*, uint32_t, uint32_t, uint32_t, uint32_t);
  int32_t ( *FillRect        ) (OTM8009A_Object_t*, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t);
  int32_t ( *GetPixel        ) (OTM8009A_Object_t*, uint32_t, uint32_t, uint32_t*);
  int32_t ( *SetPixel        ) (OTM8009A_Object_t*, uint32_t, uint32_t, uint32_t);
  int32_t ( *GetXSize        ) (OTM8009A_Object_t*, uint32_t *);
  int32_t ( *GetYSize        ) (OTM8009A_Object_t*, uint32_t *);

} OTM8009A_LCD_Drv_t;

extern OTM8009A_LCD_Drv_t   ST7701_LCD_Driver;
#endif

int ST7701S_Init(unsigned long ColorCoding);
ulong mipi_get_type(void);

#endif
