//************************************************************************************
//**                                                                                 **
//**                                 mcHF QRP Transceiver                            **
//**                         Krassi Atanassov - M0NKA, 2013-2026                     **
//**                                                                                 **
//**---------------------------------------------------------------------------------**
//**                                                                                 **
//**  File name:		lcd_low.c                                                    **
//**  Description:  	LTDC/DSI hardware driver for the ILI9806E panel             **
//**  Licence:			https://github.com/m0nka/mcHF/blob/main/LICENSE            **
//************************************************************************************
//
// The panel is driven at its native 480x800 resolution - anything else
// confuses the ILI9806E scan logic and shows as heavy artifacts. To make
// the full resolution frame fit in the 512K AXI SRAM (external SDRAM is
// not up yet) the LTDC layer runs in L8 indexed colour with a fixed
// RGB332 palette in the layer CLUT: the pixel format converter expands
// each byte to 24bit RGB in hardware, the DSI link itself stays RGB888.
//
// 480 x 800 x 1 byte = 384000 bytes, fits with ~140K to spare.
//
// All drawing is done by the CPU (no DMA2D) - the AXI SRAM MPU region is
// write-through, so no cache maintenance is needed either.
//
#include "mchf_pro_board.h"
#include "main.h"

#include <string.h>

#include "stm32h747i_discovery_errno.h"

#include "lcd_low.h"
#include "ili9806e.h"

const lcd_low_Drv_t LCD_Driver =
{
  BSP_LCD_DrawBitmap,
  BSP_LCD_FillRGBRect,
  BSP_LCD_DrawHLine,
  BSP_LCD_DrawVLine,
  BSP_LCD_FillRect,
  BSP_LCD_ReadPixel,
  BSP_LCD_WritePixel,
  BSP_LCD_GetXSize,
  BSP_LCD_GetYSize,
  BSP_LCD_SetActiveLayer,
  BSP_LCD_GetPixelFormat
};

DSI_HandleTypeDef   hdsi;
LTDC_HandleTypeDef  hltdc;

// RGB332 palette, loaded into the LTDC layer CLUT and used for
// expanding indexes back to ARGB8888 on pixel reads
static uint32_t lcd_clut[256];

void LTDC_IRQHandler(void)
{
  HAL_LTDC_IRQHandler(&hltdc);
}

void DSI_IRQHandler(void)
{
  HAL_DSI_IRQHandler(&hdsi);
}

// ARGB8888 -> RGB332 palette index (top 3/3/2 bits of each channel)
static uint8_t argb_to_l8(uint32_t color)
{
	return (uint8_t)(((color >> 16) & 0xE0U) | ((color >> 11) & 0x1CU) | ((color >> 6) & 0x03U));
}

// -------------------------------------------------------------------------------------
// -------------------------------------------------------------------------------------
//
// HAL V1.8.0 / 14-February-2020
//
// Note: use local implementation from older hal release, as the latest rewrite of
//       of this function fails ;(
//
#define DSI_TIMEOUT_VALUE ((uint32_t)1000U)
static HAL_StatusTypeDef HAL_DSI_InitA(DSI_HandleTypeDef *hdsi, DSI_PLLInitTypeDef *PLLInit)
{
  uint32_t tickstart;
  uint32_t unitIntervalx4;
  uint32_t tempIDF;

  // Check the DSI handle allocation
  if (hdsi == NULL)
  {
    return HAL_ERROR;
  }

  // Check function parameters
  assert_param(IS_DSI_PLL_NDIV(PLLInit->PLLNDIV));
  assert_param(IS_DSI_PLL_IDF(PLLInit->PLLIDF));
  assert_param(IS_DSI_PLL_ODF(PLLInit->PLLODF));
  assert_param(IS_DSI_AUTO_CLKLANE_CONTROL(hdsi->Init.AutomaticClockLaneControl));
  assert_param(IS_DSI_NUMBER_OF_LANES(hdsi->Init.NumberOfLanes));

  if (hdsi->State == HAL_DSI_STATE_RESET)
  {
    // Initialize the low level hardware
    HAL_DSI_MspInit(hdsi);
  }

  // Change DSI peripheral state
  hdsi->State = HAL_DSI_STATE_BUSY;

  // **************** Turn on the regulator and enable the DSI PLL ****************

  // Enable the regulator
  __HAL_DSI_REG_ENABLE(hdsi);

  // Get tick
  tickstart = HAL_GetTick();

  // Wait until the regulator is ready
  while (__HAL_DSI_GET_FLAG(hdsi, DSI_FLAG_RRS) == 0U)
  {
    // Check for the Timeout
    if ((HAL_GetTick() - tickstart) > DSI_TIMEOUT_VALUE)
    {
      return HAL_TIMEOUT;
    }
  }

  // Set the PLL division factors
  hdsi->Instance->WRPCR &= ~(DSI_WRPCR_PLL_NDIV | DSI_WRPCR_PLL_IDF | DSI_WRPCR_PLL_ODF);
  hdsi->Instance->WRPCR |= (((PLLInit->PLLNDIV) << 2U) | ((PLLInit->PLLIDF) << 11U) | ((PLLInit->PLLODF) << 16U));

  // Enable the DSI PLL
  __HAL_DSI_PLL_ENABLE(hdsi);

  // Get tick
  tickstart = HAL_GetTick();

  // Wait for the lock of the PLL
  while (__HAL_DSI_GET_FLAG(hdsi, DSI_FLAG_PLLLS) == 0U)
  {
    // Check for the Timeout
    if ((HAL_GetTick() - tickstart) > DSI_TIMEOUT_VALUE)
    {
      return HAL_TIMEOUT;
    }
  }

  // *************************** Set the PHY parameters ***************************

  // D-PHY clock and digital enable
  hdsi->Instance->PCTLR |= (DSI_PCTLR_CKE | DSI_PCTLR_DEN);

  // Clock lane configuration
  hdsi->Instance->CLCR &= ~(DSI_CLCR_DPCC | DSI_CLCR_ACR);
  hdsi->Instance->CLCR |= (DSI_CLCR_DPCC | hdsi->Init.AutomaticClockLaneControl);

  // Configure the number of active data lanes
  hdsi->Instance->PCONFR &= ~DSI_PCONFR_NL;
  hdsi->Instance->PCONFR |= hdsi->Init.NumberOfLanes;

  // ************************ Set the DSI clock parameters ************************

  // Set the TX escape clock division factor
  hdsi->Instance->CCR &= ~DSI_CCR_TXECKDIV;
  hdsi->Instance->CCR |= hdsi->Init.TXEscapeCkdiv;

  // Calculate the bit period in high-speed mode in unit of 0.25 ns (UIX4)
  // The equation is : UIX4 = IntegerPart( (1000/F_PHY_Mhz) * 4 )
  // Where : F_PHY_Mhz = (NDIV * HSE_Mhz) / (IDF * ODF)
  tempIDF = (PLLInit->PLLIDF > 0U) ? PLLInit->PLLIDF : 1U;
  unitIntervalx4 = (4000000U * tempIDF * ((1UL << (0x3U & PLLInit->PLLODF)))) / ((HSE_VALUE / 1000U) * PLLInit->PLLNDIV);

  // Set the bit period in high-speed mode
  hdsi->Instance->WPCR[0U] &= ~DSI_WPCR0_UIX4;
  hdsi->Instance->WPCR[0U] |= unitIntervalx4;

  // ****************************** Error management *****************************

  // Disable all error interrupts and reset the Error Mask
  hdsi->Instance->IER[0U] = 0U;
  hdsi->Instance->IER[1U] = 0U;
  hdsi->ErrorMsk = 0U;

  // Initialise the error code
  hdsi->ErrorCode = HAL_DSI_ERROR_NONE;

  // Initialize the DSI state
  hdsi->State = HAL_DSI_STATE_READY;

  return HAL_OK;
}

// -------------------------------------------------------------------------------------
// DSI host init - 54.167MHz lane byte clock, burst video mode, RGB888 link.
// LP command sizes 16/0: LP packets only in the vertical blanking windows,
// none during active lines (the old 64/64 setting allowed LP packets where
// the blanking time is far too short and commands got randomly corrupted)
static HAL_StatusTypeDef dsi_host_init(void)
{
	DSI_PLLInitTypeDef PLLInit;
	DSI_VidCfgTypeDef VidCfg;

	hdsi.Instance 							= DSI;
	hdsi.Init.AutomaticClockLaneControl		= DSI_AUTO_CLK_LANE_CTRL_DISABLE;
	hdsi.Init.NumberOfLanes 				= DSI_TWO_DATA_LANES;
	hdsi.Init.TXEscapeCkdiv 				= 4;					// TX Prescaler = 4

	PLLInit.PLLIDF   						= DSI_PLL_IN_DIV3;		// IDF 			= 3
	PLLInit.PLLNDIV  						= 104;					// NDIV 		= 104
	PLLInit.PLLODF 							= DSI_PLL_OUT_DIV2;		// ODF 			= 2

	if(HAL_DSI_InitA(&hdsi, &PLLInit) != HAL_OK)
	{
		return HAL_ERROR;
	}

	uint32_t clock_ratio = LCD_LANE_CLK/ILI9806E_PIXEL_CLK;

	VidCfg.VirtualChannelID 			= 0;
	VidCfg.ColorCoding 					= DSI_RGB888;
	VidCfg.LooselyPacked 				= DSI_LOOSELY_PACKED_DISABLE;
	VidCfg.Mode 						= DSI_VID_MODE_BURST;
	VidCfg.PacketSize 					= ILI9806E_WIDTH;
	VidCfg.NumberOfChunks 				= 0;
	VidCfg.NullPacketSize 				= 0xFFFU;
	VidCfg.VSPolarity 					= DSI_VSYNC_ACTIVE_HIGH;
	VidCfg.HSPolarity 					= DSI_HSYNC_ACTIVE_HIGH;
	VidCfg.DEPolarity 					= DSI_DATA_ENABLE_ACTIVE_HIGH;
	VidCfg.HorizontalSyncActive 		= ILI9806E_HSYNC*clock_ratio;
	VidCfg.HorizontalBackPorch 			= ILI9806E_HBP*clock_ratio;
	VidCfg.HorizontalLine 				= (ILI9806E_WIDTH + ILI9806E_HSYNC + ILI9806E_HBP + ILI9806E_HFP)*clock_ratio;
	VidCfg.VerticalSyncActive 			= ILI9806E_VSYNC;
	VidCfg.VerticalBackPorch 			= ILI9806E_VBP;
	VidCfg.VerticalFrontPorch 			= ILI9806E_VFP;
	VidCfg.VerticalActive				= ILI9806E_HEIGHT;

	// Enable sending LP commands while streaming is active in video mode
	VidCfg.LPCommandEnable 				= DSI_LP_COMMAND_ENABLE;

	// 16 bytes during vertical blanking, none during active video
	VidCfg.LPLargestPacketSize			= 16;
	VidCfg.LPVACTLargestPacketSize		= 0;

	VidCfg.LPHorizontalFrontPorchEnable	= DSI_LP_HFP_ENABLE;
	VidCfg.LPHorizontalBackPorchEnable	= DSI_LP_HBP_ENABLE;
	VidCfg.LPVerticalActiveEnable		= DSI_LP_VACT_ENABLE;
	VidCfg.LPVerticalFrontPorchEnable	= DSI_LP_VFP_ENABLE;
	VidCfg.LPVerticalBackPorchEnable	= DSI_LP_VBP_ENABLE;
	VidCfg.LPVerticalSyncActiveEnable	= DSI_LP_VSYNC_ENABLE;

	return HAL_DSI_ConfigVideoMode(&hdsi, &VidCfg);
}

// LTDC pixel clock from PLL3: 25MHz/6*130/20 = 27.083MHz,
// exact half of the DSI lane byte clock
static HAL_StatusTypeDef ltdc_clock_config(void)
{
	RCC_PeriphCLKInitTypeDef PeriphClkInitStruct;

	PeriphClkInitStruct.PeriphClockSelection	= RCC_PERIPHCLK_LTDC;
	PeriphClkInitStruct.PLL3.PLL3M				= 6U;
	PeriphClkInitStruct.PLL3.PLL3N				= 130U;
	PeriphClkInitStruct.PLL3.PLL3R				= 20U;
	PeriphClkInitStruct.PLL3.PLL3P				= 2U;
	PeriphClkInitStruct.PLL3.PLL3Q				= 2U;

	return HAL_RCCEx_PeriphCLKConfig(&PeriphClkInitStruct);
}

// Same timing register values as the radio firmware
static HAL_StatusTypeDef ltdc_init(void)
{
	hltdc.Instance 					= LTDC;
	hltdc.Init.HSPolarity 			= LTDC_HSPOLARITY_AL;
	hltdc.Init.VSPolarity 			= LTDC_VSPOLARITY_AL;
	hltdc.Init.DEPolarity 			= LTDC_DEPOLARITY_AL;
	hltdc.Init.PCPolarity 			= LTDC_PCPOLARITY_IPC;

	hltdc.Init.HorizontalSync     	= ILI9806E_HSYNC;
	hltdc.Init.AccumulatedHBP     	= ILI9806E_HSYNC + ILI9806E_HBP;
	hltdc.Init.AccumulatedActiveW	= ILI9806E_HSYNC + ILI9806E_WIDTH + ILI9806E_HBP;
	hltdc.Init.TotalWidth         	= ILI9806E_HSYNC + ILI9806E_WIDTH + ILI9806E_HBP + ILI9806E_HFP;
	hltdc.Init.VerticalSync       	= ILI9806E_VSYNC;
	hltdc.Init.AccumulatedVBP     	= ILI9806E_VSYNC + ILI9806E_VBP;
	hltdc.Init.AccumulatedActiveH 	= ILI9806E_VSYNC + ILI9806E_HEIGHT + ILI9806E_VBP;
	hltdc.Init.TotalHeigh         	= ILI9806E_VSYNC + ILI9806E_HEIGHT + ILI9806E_VBP + ILI9806E_VFP;

	hltdc.Init.Backcolor.Blue  		= 0x00;
	hltdc.Init.Backcolor.Green 		= 0x00;
	hltdc.Init.Backcolor.Red   		= 0x00;

	return HAL_LTDC_Init(&hltdc);
}

// L8 layer over the full screen, RGB332 palette in the CLUT
static HAL_StatusTypeDef ltdc_layer_init(void)
{
	LTDC_LayerCfgTypeDef cfg;
	uint32_t i, r, g, b;

	cfg.WindowX0 		= 0;
	cfg.WindowX1 		= ILI9806E_WIDTH;
	cfg.WindowY0 		= 0;
	cfg.WindowY1 		= ILI9806E_HEIGHT;
	cfg.PixelFormat 	= LTDC_PIXEL_FORMAT_L8;
	cfg.Alpha 			= 255;
	cfg.Alpha0 			= 0;
	cfg.BlendingFactor1 = LTDC_BLENDING_FACTOR1_PAxCA;
	cfg.BlendingFactor2 = LTDC_BLENDING_FACTOR2_PAxCA;
	cfg.FBStartAdress 	= LAYER0_ADDRESS;
	cfg.ImageWidth 		= ILI9806E_WIDTH;
	cfg.ImageHeight 	= ILI9806E_HEIGHT;
	cfg.Backcolor.Blue 	= 0;
	cfg.Backcolor.Green = 0;
	cfg.Backcolor.Red 	= 0;

	if(HAL_LTDC_ConfigLayer(&hltdc, &cfg, 0) != HAL_OK)
		return HAL_ERROR;

	// RGB332 palette - index bits RRRGGGBB, each channel expanded
	// to the full 0..255 range
	for(i = 0; i < 256; i++)
	{
		r = (((i >> 5) & 0x07U) * 255U)/7U;
		g = (((i >> 2) & 0x07U) * 255U)/7U;
		b = ((i & 0x03U) * 255U)/3U;

		lcd_clut[i] = (r << 16) | (g << 8) | b;
	}

	if(HAL_LTDC_ConfigCLUT(&hltdc, lcd_clut, 256, 0) != HAL_OK)
		return HAL_ERROR;

	return HAL_LTDC_EnableCLUT(&hltdc, 0);
}

//*----------------------------------------------------------------------------
//* Function Name       : BSP_LCD_Init
//* Object              : full LCD bring up - DSI PLL, video mode, LTDC,
//*                       L8 layer + CLUT, panel init
//*----------------------------------------------------------------------------
int32_t BSP_LCD_Init(uint32_t Instance)
{
	if(Instance > 0)
		return BSP_ERROR_WRONG_PARAM;

	// Enable video interrupts
	HAL_NVIC_SetPriority(LTDC_IRQn, 0x0F, 0);
	HAL_NVIC_EnableIRQ(LTDC_IRQn);
	//
	HAL_NVIC_SetPriority(DSI_IRQn, 0x0F, 0);
	HAL_NVIC_EnableIRQ(DSI_IRQn);

	// Enable video clocks
	__HAL_RCC_LTDC_CLK_ENABLE();
	__HAL_RCC_LTDC_FORCE_RESET();
	__HAL_RCC_LTDC_RELEASE_RESET();
	//
	__HAL_RCC_DSI_CLK_ENABLE();
	__HAL_RCC_DSI_FORCE_RESET();
	__HAL_RCC_DSI_RELEASE_RESET();

	hdsi.Instance = DSI;
	HAL_DSI_DeInit(&hdsi);

	if(dsi_host_init() != HAL_OK)
	{
		printf("== lcd: dsi init error ==\r\n");
		return BSP_ERROR_PERIPH_FAILURE;
	}

	if(ltdc_clock_config() != HAL_OK)
	{
		printf("== lcd: pll3 error ==\r\n");
		return BSP_ERROR_PERIPH_FAILURE;
	}

	// The LTDC comes out of HAL_LTDC_Init() running, and it must stay
	// running: with the wrapper enabled the DSI host transmits LP commands
	// only inside the blanking windows of the video stream, so with the
	// LTDC stopped the command FIFO never drains
	if(ltdc_init() != HAL_OK)
	{
		printf("== lcd: ltdc init error ==\r\n");
		return BSP_ERROR_PERIPH_FAILURE;
	}

	if(ltdc_layer_init() != HAL_OK)
	{
		printf("== lcd: layer init error ==\r\n");
		return BSP_ERROR_PERIPH_FAILURE;
	}

	// Enable the DSI host and wrapper after the LTDC initialisation
	HAL_DSI_Start(&hdsi);

	// No BTA flow control and no DCS reads here: a read in video mode always
	// times out and poisons the link - the next packet after it corrupts the
	// panel setup (washed out image on every boot, bench-confirmed)

	// Init LCD registers
	ili9806e_init();

	return BSP_ERROR_NONE;
}

int32_t BSP_LCD_GetXSize(uint32_t Instance, uint32_t *XSize)
{
	*XSize = ILI9806E_WIDTH;
	return BSP_ERROR_NONE;
}

int32_t BSP_LCD_GetYSize(uint32_t Instance, uint32_t *YSize)
{
	*YSize = ILI9806E_HEIGHT;
	return BSP_ERROR_NONE;
}

int32_t BSP_LCD_SetActiveLayer(uint32_t Instance, uint32_t LayerIndex)
{
	// Single layer only
	return BSP_ERROR_NONE;
}

int32_t BSP_LCD_GetPixelFormat(uint32_t Instance, uint32_t *PixelFormat)
{
	*PixelFormat = LCD_PIXEL_FORMAT_L8;
	return BSP_ERROR_NONE;
}

//*----------------------------------------------------------------------------
//* Function Name       : BSP_LCD_FillRect
//* Object              : clipped rectangle fill, one memset() per row
//*----------------------------------------------------------------------------
int32_t BSP_LCD_FillRect(uint32_t Instance, uint32_t Xpos, uint32_t Ypos, uint32_t Width, uint32_t Height, uint32_t Color)
{
	uint8_t *dst;
	uint8_t idx = argb_to_l8(Color);

	if((Xpos >= ILI9806E_WIDTH)||(Ypos >= ILI9806E_HEIGHT))
		return BSP_ERROR_WRONG_PARAM;

	if((Xpos + Width) > ILI9806E_WIDTH)
		Width = ILI9806E_WIDTH - Xpos;

	if((Ypos + Height) > ILI9806E_HEIGHT)
		Height = ILI9806E_HEIGHT - Ypos;

	dst = (uint8_t *)LAYER0_ADDRESS + Ypos*ILI9806E_WIDTH + Xpos;

	while(Height--)
	{
		memset(dst, idx, Width);
		dst += ILI9806E_WIDTH;
	}

	return BSP_ERROR_NONE;
}

int32_t BSP_LCD_DrawHLine(uint32_t Instance, uint32_t Xpos, uint32_t Ypos, uint32_t Length, uint32_t Color)
{
	return BSP_LCD_FillRect(Instance, Xpos, Ypos, Length, 1, Color);
}

int32_t BSP_LCD_DrawVLine(uint32_t Instance, uint32_t Xpos, uint32_t Ypos, uint32_t Length, uint32_t Color)
{
	return BSP_LCD_FillRect(Instance, Xpos, Ypos, 1, Length, Color);
}

int32_t BSP_LCD_WritePixel(uint32_t Instance, uint32_t Xpos, uint32_t Ypos, uint32_t Color)
{
	if((Xpos >= ILI9806E_WIDTH)||(Ypos >= ILI9806E_HEIGHT))
		return BSP_ERROR_WRONG_PARAM;

	*((uint8_t *)LAYER0_ADDRESS + Ypos*ILI9806E_WIDTH + Xpos) = argb_to_l8(Color);

	return BSP_ERROR_NONE;
}

int32_t BSP_LCD_ReadPixel(uint32_t Instance, uint32_t Xpos, uint32_t Ypos, uint32_t *Color)
{
	if((Xpos >= ILI9806E_WIDTH)||(Ypos >= ILI9806E_HEIGHT))
		return BSP_ERROR_WRONG_PARAM;

	*Color = 0xFF000000U | lcd_clut[*((uint8_t *)LAYER0_ADDRESS + Ypos*ILI9806E_WIDTH + Xpos)];

	return BSP_ERROR_NONE;
}

//*----------------------------------------------------------------------------
//* Function Name       : BSP_LCD_FillRGBRect
//* Object              : write a rectangle of ARGB8888 pixel data
//*----------------------------------------------------------------------------
int32_t BSP_LCD_FillRGBRect(uint32_t Instance, uint32_t Xpos, uint32_t Ypos, uint8_t *pData, uint32_t Width, uint32_t Height)
{
	uint32_t *src = (uint32_t *)pData;
	uint8_t *dst;
	uint32_t i, j;

	if(((Xpos + Width) > ILI9806E_WIDTH)||((Ypos + Height) > ILI9806E_HEIGHT))
		return BSP_ERROR_WRONG_PARAM;

	for(i = 0; i < Height; i++)
	{
		dst = (uint8_t *)LAYER0_ADDRESS + (Ypos + i)*ILI9806E_WIDTH + Xpos;

		for(j = 0; j < Width; j++)
			*dst++ = argb_to_l8(*src++);
	}

	return BSP_ERROR_NONE;
}

//*----------------------------------------------------------------------------
//* Function Name       : BSP_LCD_DrawBitmap
//* Object              : draw an uncompressed 24/32bpp BMP from flash
//*----------------------------------------------------------------------------
int32_t BSP_LCD_DrawBitmap(uint32_t Instance, uint32_t Xpos, uint32_t Ypos, uint8_t *pBmp)
{
	uint32_t index, width, height, bit_pixel, bytes_pp;
	uint32_t i, j, color;
	uint8_t *src;
	uint8_t *dst;

	// Get bitmap data address offset
	index = (uint32_t)pBmp[10] + ((uint32_t)pBmp[11] << 8) + ((uint32_t)pBmp[12] << 16) + ((uint32_t)pBmp[13] << 24);

	// Read bitmap width
	width = (uint32_t)pBmp[18] + ((uint32_t)pBmp[19] << 8) + ((uint32_t)pBmp[20] << 16) + ((uint32_t)pBmp[21] << 24);

	// Read bitmap height
	height = (uint32_t)pBmp[22] + ((uint32_t)pBmp[23] << 8) + ((uint32_t)pBmp[24] << 16) + ((uint32_t)pBmp[25] << 24);

	// Read bit/pixel
	bit_pixel = (uint32_t)pBmp[28] + ((uint32_t)pBmp[29] << 8);
	bytes_pp  = bit_pixel/8U;

	if((bytes_pp != 3U)&&(bytes_pp != 4U))
		return BSP_ERROR_WRONG_PARAM;

	if(((Xpos + width) > ILI9806E_WIDTH)||((Ypos + height) > ILI9806E_HEIGHT))
		return BSP_ERROR_WRONG_PARAM;

	// BMP rows are stored bottom-up
	for(i = 0; i < height; i++)
	{
		src = pBmp + index + (height - 1U - i)*width*bytes_pp;
		dst = (uint8_t *)LAYER0_ADDRESS + (Ypos + i)*ILI9806E_WIDTH + Xpos;

		for(j = 0; j < width; j++)
		{
			color = (uint32_t)src[0] | ((uint32_t)src[1] << 8) | ((uint32_t)src[2] << 16);
			*dst++ = argb_to_l8(color);
			src += bytes_pp;
		}
	}

	return BSP_ERROR_NONE;
}
