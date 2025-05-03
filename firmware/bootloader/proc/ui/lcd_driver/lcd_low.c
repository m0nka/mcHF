/**
  ******************************************************************************
  * @file    stm32h747i_discovery_lcd.c
  * @author  MCD Application Team
  * @brief   This file includes the driver for Liquid Crystal Display (LCD) module
  *          mounted on STM32H747I_DISCO board.
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2017 STMicroelectronics.
  * All rights reserved.</center></h2>
  *
  * This software component is licensed by ST under BSD 3-Clause license,
  * the "License"; You may not use this file except in compliance with the
  * License. You may obtain a copy of the License at:
  *                        opensource.org/licenses/BSD-3-Clause
  *
  ******************************************************************************
  */
/*  How To use this driver:
  --------------------------
   - This driver is used to drive directly a LCD TFT using the DSI interface.
     The following IPs are implied : DSI Host IP block working in conjunction to the
	 LTDC controller.
   - This driver is linked by construction to LCD KoD mounted on MB1166 daughter board.
   - This driver is also used to drive monitors using HDMI interface.

  Driver description:
  ---------------------
   + Initialization steps:
     o Initialize the LCD in default mode using the BSP_LCD_Init() function with the
       following settings:
        - DSI is configured in video mode
        - Pixelformat : LCD_PIXEL_FORMAT_RBG888
        - Orientation : LCD_ORIENTATION_LANDSCAPE.
        - Width       : LCD_DEFAULT_WIDTH (800)
        - Height      : LCD_DEFAULT_HEIGHT(480)
       The default LTDC layer configured is layer 0.
       BSP_LCD_Init() includes DSI, LTDC, LTDC Layer and clock configurations by calling:
        - MX_LTDC_ClockConfig()
        - MX_LTDC_Init()
        - MX_DSIHOST_DSI_Init()
        - MX_LTDC_ConfigLayer()

     o Initialize the LCD with required parameters using the BSP_LCD_InitEx() function.
       To initialize DSI in command mode, user have to override MX_DSIHOST_DSI_Init(), weak function,
       content at application level.

     o Initialize the display with HDMI using BSP_LCD_InitHDMI(). Two display formats
       are supported: HDMI_FORMAT_720_480 or HDMI_FORMAT_720_576

     o Select the LCD layer to be used using the BSP_LCD_SelectLayer() function.
     o Enable the LCD display using the BSP_LCD_DisplayOn() function.
     o Disable the LCD display using the BSP_LCD_DisplayOff() function.
     o Set the display brightness using the BSP_LCD_SetBrightness() function.
     o Get the display brightness using the BSP_LCD_GetBrightness() function.
     o Write a pixel to the LCD memory using the BSP_LCD_WritePixel() function.
     o Read a pixel from the LCD memory using the BSP_LCD_ReadPixel() function.
     o Draw an horizontal line using the BSP_LCD_DrawHLine() function.
     o Draw a vertical line using the BSP_LCD_DrawVLine() function.
     o Draw a bitmap image using the BSP_LCD_DrawBitmap() function.

   + Options
     o Configure the LTDC reload mode by calling BSP_LCD_Relaod(). By default, the
       reload mode is set to BSP_LCD_RELOAD_IMMEDIATE then LTDC is reloaded immediately.
       To control the reload mode:
         - Call BSP_LCD_Relaod() with ReloadType parameter set to BSP_LCD_RELOAD_NONE
         - Configure LTDC (color keying, transparency ..)
         - Call BSP_LCD_Relaod() with ReloadType parameter set to BSP_LCD_RELOAD_IMMEDIATE
           for immediate reload or BSP_LCD_RELOAD_VERTICAL_BLANKING for LTDC reload
           in the next vertical blanking
     o Configure LTDC layers using BSP_LCD_ConfigLayer()
     o Control layer visibility using BSP_LCD_SetLayerVisible()
     o Configure and enable the color keying functionality using the
       BSP_LCD_SetColorKeying() function.
     o Disable the color keying functionality using the BSP_LCD_ResetColorKeying() function.
     o Modify on the fly the transparency and/or the frame buffer address
       using the following functions:
       - BSP_LCD_SetTransparency()
       - BSP_LCD_SetLayerAddress()

   + Display on LCD
     o To draw and fill a basic shapes (dot, line, rectangle, circle, ellipse, .. bitmap)
       on LCD and display text, utility basic_gui.c/.h must be called. Once the LCD is initialized,
       user should call lcd_low_SetFuncDriver() API to link board LCD drivers to BASIC GUI LCD drivers.
       The basic gui services, defined in basic_gui utility, are ready for use.

  Note:
  --------
    Regarding the "Instance" parameter, needed for all functions, it is used to select
    an LCD instance. On the STM32H747I_DISCO board, there's one instance. Then, this
    parameter should be 0.
  */
#include "mchf_pro_board.h"
#include "main.h"

#include "lcd_low.h"

static LCD_Drv_t *Lcd_Drv = NULL;

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

void                *Lcd_CompObj = NULL;
DSI_HandleTypeDef   hdsi;
DMA2D_HandleTypeDef hlcd_dma2d;
LTDC_HandleTypeDef  hltdc;
BSP_LCD_Ctx_t       Lcd_Ctx[LCD_INSTANCES_NBR];

static int32_t DSI_IO_Write(uint16_t ChannelNbr, uint16_t Reg, uint8_t *pData, uint16_t Size);

static void LTDC_MspInit(LTDC_HandleTypeDef *hltdc);
static void DMA2D_MspInit(DMA2D_HandleTypeDef *hdma2d);
static void DMA2D_MspDeInit(DMA2D_HandleTypeDef *hdma2d);
static void LL_FillBuffer(uint32_t Instance, uint32_t *pDst, uint32_t xSize, uint32_t ySize, uint32_t OffLine, uint32_t Color);
static void LL_ConvertLineToRGB(uint32_t Instance, uint32_t *pSrc, uint32_t *pDst, uint32_t xSize, uint32_t ColorMode);

#if 0
static __IO int32_t  front_buffer = 0;
static __IO int32_t  pend_buffer  = -1;

static const uint32_t Buffers[2] =
{
  LAYER0_ADDRESS,
  LAYER0_ADDRESS + (480*480*4)
};
#endif

/**
  * @brief  Line Event callback.
  * @param  hltdc: pointer to a LTDC_HandleTypeDef structure that contains
  *                the configuration information for the LTDC.
  * @retval None
  */
void HAL_LTDC_LineEventCallback(LTDC_HandleTypeDef *hltdc)
{
#if 0
  if(pend_buffer >= 0)
  {
    LTDC_LAYER(hltdc, 0)->CFBAR = ((uint32_t)Buffers[pend_buffer]);
    __HAL_LTDC_RELOAD_CONFIG(hltdc);

    front_buffer = pend_buffer;
    pend_buffer = -1;
  }

  HAL_LTDC_ProgramLineEvent(hltdc, 0);
#endif
}

void LTDC_IRQHandler(void)
{
  HAL_LTDC_IRQHandler(&hltdc);
}

void DSI_IRQHandler(void)
{
  HAL_DSI_IRQHandler(&hdsi);
}

#if 0
void DMA2D_IRQHandler(void)
{
	if(DMA2D->ISR & DMA2D_ISR_TEIF)
	{
		//Error_Handler(16); /* Should never happen */
	}

	/* Clear the Transfer complete interrupt */
	DMA2D->IFCR = (unsigned long)(DMA2D_IFCR_CTCIF | DMA2D_IFCR_CCTCIF | DMA2D_IFCR_CTEIF);

	/* Signal semaphore */
	//osSemaphoreRelease(osDma2dSemph);
}
#endif

static int32_t DSI_IO_Read(uint16_t Reg, uint8_t *pData, uint16_t Size)
{
	return HAL_DSI_Read(&hdsi, 0, pData, Size, DSI_DCS_SHORT_PKT_READ, Reg, pData);
}

#if 1
// Init DSI just to read ID
//
static int LCDConf_ReadID(uchar *id)
{
	DSI_PLLInitTypeDef 	dsiPllInit;

	hdsi.Instance = DSI;
	HAL_DSI_DeInit(&(hdsi));

	dsiPllInit.PLLNDIV  				= 100;
	dsiPllInit.PLLIDF   				= DSI_PLL_IN_DIV5;
	dsiPllInit.PLLODF 					= DSI_PLL_OUT_DIV1;
	hdsi.Init.NumberOfLanes 			= DSI_TWO_DATA_LANES;
	hdsi.Init.TXEscapeCkdiv 			= LCD_LANE_CLK/15620;
    hdsi.Init.AutomaticClockLaneControl	= DSI_AUTO_CLK_LANE_CTRL_DISABLE;

    if(HAL_DSI_Init(&(hdsi), &(dsiPllInit)) != 0)
    	return 1;

    HAL_DSI_Start(&(hdsi));

  	// Set reading mode
  	HAL_DSI_ConfigFlowControl(&hdsi, DSI_FLOW_CONTROL_BTA);

  	// Read Controller ID
  	if(DSI_IO_Read(0xDA, id, 1) != 0)
  		return 2;

  	//printf("LCD ID: %02x\r\n",id[0]);

  	HAL_DSI_Stop(&hdsi);

	return 0;
}
#endif

int32_t BSP_LCD_Init(uint32_t Instance, uint32_t Orientation)
{
	if(Orientation == LCD_ORIENTATION_LANDSCAPE)
		return BSP_LCD_InitEx(Instance, Orientation, LCD_PIXEL_FORMAT_RGB888, 854, 480);
	else
		return BSP_LCD_InitEx(Instance, Orientation, LCD_PIXEL_FORMAT_RGB565, 480, 480);
}

/**
  * @brief  Initializes the LCD.
  * @param  Instance    LCD Instance
  * @param  Orientation LCD_ORIENTATION_PORTRAIT or LCD_ORIENTATION_LANDSCAPE
  * @param  PixelFormat LCD_PIXEL_FORMAT_RBG565 or LCD_PIXEL_FORMAT_RBG888
  * @param  Width       Display width
  * @param  Height      Display height
  * @retval BSP status
  */
int32_t BSP_LCD_InitEx(uint32_t Instance, uint32_t Orientation, uint32_t PixelFormat, uint32_t Width, uint32_t Height)
{
  int32_t ret = BSP_ERROR_NONE;
  uint32_t ctrl_pixel_format, ltdc_pixel_format, dsi_pixel_format;
  MX_LTDC_LayerConfig_t config;

  Lcd_Drv = (LCD_Drv_t *)(void *) &ST7701_LCD_Driver;

  if((Orientation > LCD_ORIENTATION_LANDSCAPE) || (Instance >= LCD_INSTANCES_NBR) || \
     ((PixelFormat != LCD_PIXEL_FORMAT_RGB565) && (PixelFormat != LTDC_PIXEL_FORMAT_RGB888)))
  {
	  printf("== error 1 ==\r\n");
    ret = BSP_ERROR_WRONG_PARAM;
  }
  else
  {
    if(PixelFormat == LCD_PIXEL_FORMAT_RGB565)
    {
      ltdc_pixel_format = LTDC_PIXEL_FORMAT_RGB565;
      dsi_pixel_format = DSI_RGB565;
      ctrl_pixel_format = OTM8009A_FORMAT_RBG565;
      Lcd_Ctx[Instance].BppFactor = 2U;
    }
    else /* LCD_PIXEL_FORMAT_RGB888 */
    {
      ltdc_pixel_format = LTDC_PIXEL_FORMAT_ARGB8888;
      dsi_pixel_format = DSI_RGB888;
      ctrl_pixel_format = OTM8009A_FORMAT_RGB888;
      Lcd_Ctx[Instance].BppFactor = 4U;
    }

    /* Store pixel format, xsize and ysize information */
    Lcd_Ctx[Instance].PixelFormat = PixelFormat;
    Lcd_Ctx[Instance].XSize  = Width;
    Lcd_Ctx[Instance].YSize  = Height;

    // Enable video interrupts
    HAL_NVIC_SetPriority(LTDC_IRQn, 0x0F, 0);
    HAL_NVIC_EnableIRQ(LTDC_IRQn);

    //HAL_NVIC_SetPriority(DMA2D_IRQn, 0x0F, 0);
    //HAL_NVIC_EnableIRQ(DMA2D_IRQn);

   	HAL_NVIC_SetPriority(DSI_IRQn, 0x0F, 0);
   	HAL_NVIC_EnableIRQ(DSI_IRQn);

   	// Enable video clocks
   	__HAL_RCC_LTDC_CLK_ENABLE();
   	__HAL_RCC_LTDC_FORCE_RESET();
   	__HAL_RCC_LTDC_RELEASE_RESET();

   	__HAL_RCC_DMA2D_CLK_ENABLE();
   	__HAL_RCC_DMA2D_FORCE_RESET();
   	__HAL_RCC_DMA2D_RELEASE_RESET();

   	__HAL_RCC_DSI_CLK_ENABLE();
   	__HAL_RCC_DSI_FORCE_RESET();
   	__HAL_RCC_DSI_RELEASE_RESET();

    uchar id[4];

    // Read ID
    if(LCDConf_ReadID(id) != 0)
    {
    	printf("== unable to read LCD ID! ==\r\n");
    	return 1;
    }

    /* Initializes peripherals instance value */
    hltdc.Instance = LTDC;
    hlcd_dma2d.Instance = DMA2D;
    hdsi.Instance = DSI;

    ret = MX_DSIHOST_DSI_Init(&hdsi, Width, Height, dsi_pixel_format);
    if(ret != HAL_OK)
    {
    	printf("== error 2(%d) ==\r\n", ret);
      ret = BSP_ERROR_PERIPH_FAILURE;
    }
    else if(MX_LTDC_ClockConfig(&hltdc) != HAL_OK)
    {
    	printf("== error 3 ==\r\n");
      ret = BSP_ERROR_PERIPH_FAILURE;
    }
    else
    {
     if(MX_LTDC_Init(&hltdc, Width, Height) != HAL_OK)
     {
    	 printf("== error 4 ==\r\n");
       ret = BSP_ERROR_PERIPH_FAILURE;
     }
    }

    if(ret == BSP_ERROR_NONE)
    {
      /* Before configuring LTDC layer, ensure SDRAM is initialized */
#if !defined(DATA_IN_ExtSDRAM)
#if 0
      /* Initialize the SDRAM */
      if(BSP_SDRAM_Init(0) != BSP_ERROR_NONE)
      {
        return BSP_ERROR_PERIPH_FAILURE;
      }
#endif
#endif /* DATA_IN_ExtSDRAM */

      /* Configure default LTDC Layer 0. This configuration can be override by calling
      BSP_LCD_ConfigLayer() at application level */
      config.X0          = 0;
      config.X1          = Width;
      config.Y0          = 0;
      config.Y1          = Height;
      config.PixelFormat = ltdc_pixel_format;
      config.Address     = LAYER0_ADDRESS;
      if(MX_LTDC_ConfigLayer(&hltdc, 0, &config) != HAL_OK)
      {
    	  printf("== error 5 ==\r\n");
        ret = BSP_ERROR_PERIPH_FAILURE;
      }
      else
      {
        /* Enable the DSI host and wrapper after the LTDC initialisation
        To avoid any synchronization issue, the DSI shall be started after enabling the LTDC */
        (void)HAL_DSI_Start(&hdsi);

        /* Enable the DSI BTW for read operations */
        (void)HAL_DSI_ConfigFlowControl(&hdsi, DSI_FLOW_CONTROL_BTA);

        ST7701S_Init(DSI_RGB565);
      }
    /* By default the reload is activated and executed immediately */
    Lcd_Ctx[Instance].ReloadEnable = 1U;
   }
  }

  return ret;
}

// -------------------------------------------------------------------------------------
// -------------------------------------------------------------------------------------
//
//
// HAL V1.8.0 / 14-February-2020
//
#define DSI_TIMEOUT_VALUE ((uint32_t)1000U)
HAL_StatusTypeDef HAL_DSI_InitA(DSI_HandleTypeDef *hdsi, DSI_PLLInitTypeDef *PLLInit)
{
  uint32_t tickstart;
  uint32_t unitIntervalx4;
  uint32_t tempIDF;

  /* Check the DSI handle allocation */
  if (hdsi == NULL)
  {
    return HAL_ERROR;
  }

  /* Check function parameters */
  assert_param(IS_DSI_PLL_NDIV(PLLInit->PLLNDIV));
  assert_param(IS_DSI_PLL_IDF(PLLInit->PLLIDF));
  assert_param(IS_DSI_PLL_ODF(PLLInit->PLLODF));
  assert_param(IS_DSI_AUTO_CLKLANE_CONTROL(hdsi->Init.AutomaticClockLaneControl));
  assert_param(IS_DSI_NUMBER_OF_LANES(hdsi->Init.NumberOfLanes));

#if (USE_HAL_DSI_REGISTER_CALLBACKS == 1)
  if (hdsi->State == HAL_DSI_STATE_RESET)
  {
    /* Reset the DSI callback to the legacy weak callbacks */
    hdsi->TearingEffectCallback = HAL_DSI_TearingEffectCallback; /* Legacy weak TearingEffectCallback */
    hdsi->EndOfRefreshCallback  = HAL_DSI_EndOfRefreshCallback;  /* Legacy weak EndOfRefreshCallback  */
    hdsi->ErrorCallback         = HAL_DSI_ErrorCallback;         /* Legacy weak ErrorCallback         */

    if (hdsi->MspInitCallback == NULL)
    {
      hdsi->MspInitCallback = HAL_DSI_MspInit;
    }
    /* Initialize the low level hardware */
    hdsi->MspInitCallback(hdsi);
  }
#else
  if (hdsi->State == HAL_DSI_STATE_RESET)
  {
    /* Initialize the low level hardware */
    HAL_DSI_MspInit(hdsi);
  }
#endif /* USE_HAL_DSI_REGISTER_CALLBACKS */

  /* Change DSI peripheral state */
  hdsi->State = HAL_DSI_STATE_BUSY;

  /**************** Turn on the regulator and enable the DSI PLL ****************/

  /* Enable the regulator */
  __HAL_DSI_REG_ENABLE(hdsi);

  /* Get tick */
  tickstart = HAL_GetTick();

  /* Wait until the regulator is ready */
  while (__HAL_DSI_GET_FLAG(hdsi, DSI_FLAG_RRS) == 0U)
  {
    /* Check for the Timeout */
    if ((HAL_GetTick() - tickstart) > DSI_TIMEOUT_VALUE)
    {
      return HAL_TIMEOUT;
    }
  }

  /* Set the PLL division factors */
  hdsi->Instance->WRPCR &= ~(DSI_WRPCR_PLL_NDIV | DSI_WRPCR_PLL_IDF | DSI_WRPCR_PLL_ODF);
  hdsi->Instance->WRPCR |= (((PLLInit->PLLNDIV) << 2U) | ((PLLInit->PLLIDF) << 11U) | ((PLLInit->PLLODF) << 16U));

  /* Enable the DSI PLL */
  __HAL_DSI_PLL_ENABLE(hdsi);

  /* Get tick */
  tickstart = HAL_GetTick();

  /* Wait for the lock of the PLL */
  while (__HAL_DSI_GET_FLAG(hdsi, DSI_FLAG_PLLLS) == 0U)
  {
    /* Check for the Timeout */
    if ((HAL_GetTick() - tickstart) > DSI_TIMEOUT_VALUE)
    {
      return HAL_TIMEOUT;
    }
  }

  /*************************** Set the PHY parameters ***************************/

  /* D-PHY clock and digital enable*/
  hdsi->Instance->PCTLR |= (DSI_PCTLR_CKE | DSI_PCTLR_DEN);

  /* Clock lane configuration */
  hdsi->Instance->CLCR &= ~(DSI_CLCR_DPCC | DSI_CLCR_ACR);
  hdsi->Instance->CLCR |= (DSI_CLCR_DPCC | hdsi->Init.AutomaticClockLaneControl);

  /* Configure the number of active data lanes */
  hdsi->Instance->PCONFR &= ~DSI_PCONFR_NL;
  hdsi->Instance->PCONFR |= hdsi->Init.NumberOfLanes;

  /************************ Set the DSI clock parameters ************************/

  /* Set the TX escape clock division factor */
  hdsi->Instance->CCR &= ~DSI_CCR_TXECKDIV;
  hdsi->Instance->CCR |= hdsi->Init.TXEscapeCkdiv;

  /* Calculate the bit period in high-speed mode in unit of 0.25 ns (UIX4) */
  /* The equation is : UIX4 = IntegerPart( (1000/F_PHY_Mhz) * 4 )          */
  /* Where : F_PHY_Mhz = (NDIV * HSE_Mhz) / (IDF * ODF)                    */
  tempIDF = (PLLInit->PLLIDF > 0U) ? PLLInit->PLLIDF : 1U;
  unitIntervalx4 = (4000000U * tempIDF * ((1UL << (0x3U & PLLInit->PLLODF)))) / ((HSE_VALUE / 1000U) * PLLInit->PLLNDIV);

  /* Set the bit period in high-speed mode */
  hdsi->Instance->WPCR[0U] &= ~DSI_WPCR0_UIX4;
  hdsi->Instance->WPCR[0U] |= unitIntervalx4;

  /****************************** Error management *****************************/

  /* Disable all error interrupts and reset the Error Mask */
  hdsi->Instance->IER[0U] = 0U;
  hdsi->Instance->IER[1U] = 0U;
  hdsi->ErrorMsk = 0U;

  /* Initialise the error code */
  hdsi->ErrorCode = HAL_DSI_ERROR_NONE;

  /* Initialize the DSI state*/
  hdsi->State = HAL_DSI_STATE_READY;

  return HAL_OK;
}
//
// -------------------------------------------------------------------------------------
// -------------------------------------------------------------------------------------
__weak HAL_StatusTypeDef MX_DSIHOST_DSI_Init(DSI_HandleTypeDef *hdsi, uint32_t Width, uint32_t Height, uint32_t PixelFormat)
{
  DSI_PLLInitTypeDef PLLInit;
  DSI_VidCfgTypeDef VidCfg;

  hdsi->Instance 						= DSI;
  hdsi->Init.AutomaticClockLaneControl	= DSI_AUTO_CLK_LANE_CTRL_DISABLE;
  hdsi->Init.TXEscapeCkdiv 				= 4;
  hdsi->Init.NumberOfLanes 				= DSI_TWO_DATA_LANES;
  PLLInit.PLLNDIV 						= 100;
  PLLInit.PLLIDF 						= DSI_PLL_IN_DIV5;
  PLLInit.PLLODF 						= DSI_PLL_OUT_DIV1;

  //
  // Note: use local implementation from older hal release, as the latest rewrite of
  //       of this function fails ;(
  //
  if (HAL_DSI_InitA(hdsi, &PLLInit) != HAL_OK)
  {
    return 1;//HAL_ERROR;
  }

  /* Timing parameters for all Video modes */
  /*
  The lane byte clock is set 62500 Khz
  The pixel clock is set to 27429 Khz
  */
  VidCfg.VirtualChannelID 				= 0;
  VidCfg.ColorCoding 					= PixelFormat;
  VidCfg.LooselyPacked 					= DSI_LOOSELY_PACKED_DISABLE;
  VidCfg.Mode 							= DSI_VID_MODE_BURST;

  VidCfg.PacketSize 					= Width;

  VidCfg.NumberOfChunks 				= 0;
  VidCfg.NullPacketSize 				= 0xFFFU;
  VidCfg.HSPolarity 					= DSI_HSYNC_ACTIVE_HIGH;
  VidCfg.VSPolarity 					= DSI_VSYNC_ACTIVE_HIGH;
  VidCfg.DEPolarity 					= DSI_DATA_ENABLE_ACTIVE_HIGH;
  VidCfg.HorizontalSyncActive 			= (ST7701_HSYNC * 62500U)/PIXEL_CLK;
  VidCfg.HorizontalBackPorch 			= (ST7701_HBP * 62500U)/PIXEL_CLK;
  VidCfg.HorizontalLine 				= ((Width + ST7701_HSYNC + ST7701_HBP + ST7701_HFP) * 62500U)/PIXEL_CLK;
  VidCfg.VerticalSyncActive 			= ST7701_VSYNC;
  VidCfg.VerticalBackPorch 				= ST7701_VBP;
  VidCfg.VerticalFrontPorch 			= ST7701_VFP;

  VidCfg.VerticalActive					= Height;

  VidCfg.LPCommandEnable 				= DSI_LP_COMMAND_ENABLE;
  VidCfg.LPLargestPacketSize			= 4;
  VidCfg.LPVACTLargestPacketSize		= 4;

  VidCfg.LPHorizontalFrontPorchEnable	= DSI_LP_HFP_ENABLE;
  VidCfg.LPHorizontalBackPorchEnable	= DSI_LP_HBP_ENABLE;
  VidCfg.LPVerticalActiveEnable			= DSI_LP_VACT_ENABLE;
  VidCfg.LPVerticalFrontPorchEnable		= DSI_LP_VFP_ENABLE;
  VidCfg.LPVerticalBackPorchEnable		= DSI_LP_VBP_ENABLE;
  VidCfg.LPVerticalSyncActiveEnable		= DSI_LP_VSYNC_ENABLE;

  if (HAL_DSI_ConfigVideoMode(hdsi, &VidCfg) != HAL_OK)
  {
    return 2;//HAL_ERROR;
  }

  return HAL_OK;
}

__weak HAL_StatusTypeDef MX_LTDC_Init(LTDC_HandleTypeDef *hltdc, uint32_t Width, uint32_t Height)
{

	//printf("Width=%d\r\n",Width);
	//printf("Height=%d\r\n",Height);

  hltdc->Instance 					= LTDC;
  hltdc->Init.HSPolarity 			= LTDC_HSPOLARITY_AL;
  hltdc->Init.VSPolarity 			= LTDC_VSPOLARITY_AL;
  hltdc->Init.DEPolarity 			= LTDC_DEPOLARITY_AL;
  hltdc->Init.PCPolarity 			= LTDC_PCPOLARITY_IPC;

  hltdc->Init.HorizontalSync     	= ST7701_HSYNC - 1;
  hltdc->Init.AccumulatedHBP     	= ST7701_HSYNC + ST7701_HBP - 1;
  hltdc->Init.AccumulatedActiveW	= ST7701_HSYNC + Width + ST7701_HBP - 1;
  hltdc->Init.TotalWidth         	= ST7701_HSYNC + Width + ST7701_HBP + ST7701_HFP - 1;
  hltdc->Init.VerticalSync       	= ST7701_VSYNC - 1;
  hltdc->Init.AccumulatedVBP     	= ST7701_VSYNC + ST7701_VBP - 1;
  hltdc->Init.AccumulatedActiveH 	= ST7701_VSYNC + Height + ST7701_VBP - 1;
  hltdc->Init.TotalHeigh         	= ST7701_VSYNC + Height + ST7701_VBP + ST7701_VFP - 1;

  hltdc->Init.Backcolor.Blue  = 0x00;
  hltdc->Init.Backcolor.Green = 0x00;
  hltdc->Init.Backcolor.Red   = 0x00;

  return HAL_LTDC_Init(hltdc);
}

/**
  * @brief  MX LTDC layer configuration.
  * @param  hltdc      LTDC handle
  * @param  LayerIndex Layer 0 or 1
  * @param  Config     Layer configuration
  * @retval HAL status
  */
__weak HAL_StatusTypeDef MX_LTDC_ConfigLayer(LTDC_HandleTypeDef *hltdc, uint32_t LayerIndex, MX_LTDC_LayerConfig_t *Config)
{
  LTDC_LayerCfgTypeDef pLayerCfg;

  pLayerCfg.WindowX0 = Config->X0;
  pLayerCfg.WindowX1 = Config->X1;
  pLayerCfg.WindowY0 = Config->Y0;
  pLayerCfg.WindowY1 = Config->Y1;
  pLayerCfg.PixelFormat = Config->PixelFormat;
  pLayerCfg.Alpha = 255;
  pLayerCfg.Alpha0 = 0;
  pLayerCfg.BlendingFactor1 = LTDC_BLENDING_FACTOR1_PAxCA;
  pLayerCfg.BlendingFactor2 = LTDC_BLENDING_FACTOR2_PAxCA;
  pLayerCfg.FBStartAdress = Config->Address;
  pLayerCfg.ImageWidth = (Config->X1 - Config->X0);
  pLayerCfg.ImageHeight = (Config->Y1 - Config->Y0);
  pLayerCfg.Backcolor.Blue = 0;
  pLayerCfg.Backcolor.Green = 0;
  pLayerCfg.Backcolor.Red = 0;
  return HAL_LTDC_ConfigLayer(hltdc, &pLayerCfg, LayerIndex);
}

/**
  * @brief  LTDC Clock Config for LCD DSI display.
  * @param  hltdc  LTDC Handle
  *         Being __weak it can be overwritten by the application
  * @retval HAL_status
  */
__weak HAL_StatusTypeDef MX_LTDC_ClockConfig(LTDC_HandleTypeDef *hltdc)
{
  RCC_PeriphCLKInitTypeDef  PeriphClkInitStruct;

  PeriphClkInitStruct.PeriphClockSelection   = RCC_PERIPHCLK_LTDC;
  PeriphClkInitStruct.PLL3.PLL3M      = 5U;
  PeriphClkInitStruct.PLL3.PLL3N      = 132U;
  PeriphClkInitStruct.PLL3.PLL3P      = 2U;
  PeriphClkInitStruct.PLL3.PLL3Q      = 2U;
  PeriphClkInitStruct.PLL3.PLL3R      = 24U;
  return HAL_RCCEx_PeriphCLKConfig(&PeriphClkInitStruct);
}

/**
  * @brief  LTDC Clock Config for LCD 2 DPI display.
  * @param  hltdc  LTDC Handle
  *         Being __weak it can be overwritten by the application
  * @retval HAL_status
  */
__weak HAL_StatusTypeDef MX_LTDC_ClockConfig2(LTDC_HandleTypeDef *hltdc)
{
  RCC_PeriphCLKInitTypeDef  PeriphClkInitStruct;

  PeriphClkInitStruct.PeriphClockSelection    = RCC_PERIPHCLK_LTDC;
  PeriphClkInitStruct.PLL3.PLL3M      = 1U;
  PeriphClkInitStruct.PLL3.PLL3N      = 720U;
  PeriphClkInitStruct.PLL3.PLL3P      = 2U;
  PeriphClkInitStruct.PLL3.PLL3Q      = 2U;
  PeriphClkInitStruct.PLL3.PLL3R      = 64U;
  return HAL_RCCEx_PeriphCLKConfig(&PeriphClkInitStruct);
}

/**
  * @brief  LTDC layer configuration.
  * @param  Instance   LCD instance
  * @param  LayerIndex Layer 0 or 1
  * @param  Config     Layer configuration
  * @retval HAL status
  */
int32_t BSP_LCD_ConfigLayer(uint32_t Instance, uint32_t LayerIndex, BSP_LCD_LayerConfig_t *Config)
{
  int32_t ret = BSP_ERROR_NONE;
  if(Instance >= LCD_INSTANCES_NBR)
  {
    ret = BSP_ERROR_WRONG_PARAM;
  }
  else
  {
    if (MX_LTDC_ConfigLayer(&hltdc, LayerIndex, Config) != HAL_OK)
    {
      ret = BSP_ERROR_PERIPH_FAILURE;
    }
  }
  return ret;
}

/**
  * @brief  Set the LCD Active Layer.
  * @param  Instance    LCD Instance
  * @param  LayerIndex  LCD layer index
  * @retval BSP status
  */
int32_t BSP_LCD_SetActiveLayer(uint32_t Instance, uint32_t LayerIndex)
{
  int32_t ret = BSP_ERROR_NONE;

  if(Instance >= LCD_INSTANCES_NBR)
  {
    ret = BSP_ERROR_WRONG_PARAM;
  }
  else
  {
    Lcd_Ctx[Instance].ActiveLayer = LayerIndex;
  }

  return ret;
}
/**
  * @brief  Gets the LCD Active LCD Pixel Format.
  * @param  Instance    LCD Instance
  * @param  PixelFormat Active LCD Pixel Format
  * @retval BSP status
  */
int32_t BSP_LCD_GetPixelFormat(uint32_t Instance, uint32_t *PixelFormat)
{
  int32_t ret = BSP_ERROR_NONE;

  if(Instance >= LCD_INSTANCES_NBR)
  {
    ret = BSP_ERROR_WRONG_PARAM;
  }
  else
  {
    /* Only RGB565 format is supported */
    *PixelFormat = Lcd_Ctx[Instance].PixelFormat;
  }

  return ret;
}
/**
  * @brief  Control the LTDC reload
  * @param  Instance    LCD Instance
  * @param  ReloadType can be one of the following values
  *         - BSP_LCD_RELOAD_NONE
  *         - BSP_LCD_RELOAD_IMMEDIATE
  *         - BSP_LCD_RELOAD_VERTICAL_BLANKING
  * @retval BSP status
  */
int32_t BSP_LCD_Relaod(uint32_t Instance, uint32_t ReloadType)
{
  int32_t ret = BSP_ERROR_NONE;

  if(Instance >= LCD_INSTANCES_NBR)
  {
    ret = BSP_ERROR_WRONG_PARAM;
  }
  else if(ReloadType == BSP_LCD_RELOAD_NONE)
  {
    Lcd_Ctx[Instance].ReloadEnable = 0U;
  }
  else if(HAL_LTDC_Reload (&hltdc, ReloadType) != HAL_OK)
  {
    ret = BSP_ERROR_PERIPH_FAILURE;
  }
  else
  {
    Lcd_Ctx[Instance].ReloadEnable = 1U;
  }

  return ret;
}

/**
  * @brief  Sets an LCD Layer visible
  * @param  Instance    LCD Instance
  * @param  LayerIndex  Visible Layer
  * @param  State  New state of the specified layer
  *          This parameter can be one of the following values:
  *            @arg  ENABLE
  *            @arg  DISABLE
  * @retval BSP status
  */
int32_t BSP_LCD_SetColorKeying(uint32_t Instance, uint32_t LayerIndex, uint32_t Color)
{
  int32_t ret = BSP_ERROR_NONE;

  if(Instance >= LCD_INSTANCES_NBR)
  {
    ret = BSP_ERROR_WRONG_PARAM;
  }
  else
  {
    if(Lcd_Ctx[Instance].ReloadEnable == 1U)
    {
      /* Configure and Enable the color Keying for LCD Layer */
      (void)HAL_LTDC_ConfigColorKeying(&hltdc, Color, LayerIndex);
      (void)HAL_LTDC_EnableColorKeying(&hltdc, LayerIndex);
    }
    else
    {
      /* Configure and Enable the color Keying for LCD Layer */
      (void)HAL_LTDC_ConfigColorKeying_NoReload(&hltdc, Color, LayerIndex);
      (void)HAL_LTDC_EnableColorKeying_NoReload(&hltdc, LayerIndex);
    }
  }
  return ret;
}

int32_t BSP_LCD_SetLayerVisible(uint32_t Instance, uint32_t LayerIndex, FunctionalState State)
{
  int32_t ret = BSP_ERROR_NONE;

  if(Instance >= LCD_INSTANCES_NBR)
  {
    ret = BSP_ERROR_WRONG_PARAM;
  }
  else
  {
    if(State == ENABLE)
    {
      __HAL_LTDC_LAYER_ENABLE(&hltdc, LayerIndex);
    }
    else
    {
      __HAL_LTDC_LAYER_DISABLE(&hltdc, LayerIndex);
    }

    if(Lcd_Ctx[Instance].ReloadEnable == 1U)
    {
      __HAL_LTDC_RELOAD_IMMEDIATE_CONFIG(&hltdc);
    }
  }

  return ret;
}

/**
  * @brief  Configures the transparency.
  * @param  Instance      LCD Instance
  * @param  LayerIndex    Layer foreground or background.
  * @param  Transparency  Transparency
  *           This parameter must be a number between Min_Data = 0x00 and Max_Data = 0xFF
  * @retval BSP status
  */
int32_t BSP_LCD_SetTransparency(uint32_t Instance, uint32_t LayerIndex, uint8_t Transparency)
{
  int32_t ret = BSP_ERROR_NONE;

  if(Instance >= LCD_INSTANCES_NBR)
  {
    ret = BSP_ERROR_WRONG_PARAM;
  }
  else
  {
    if(Lcd_Ctx[Instance].ReloadEnable == 1U)
    {
      (void)HAL_LTDC_SetAlpha(&hltdc, Transparency, LayerIndex);
    }
    else
    {
      (void)HAL_LTDC_SetAlpha_NoReload(&hltdc, Transparency, LayerIndex);
    }
  }

  return ret;
}

/**
  * @brief  Sets an LCD layer frame buffer address.
  * @param  Instance    LCD Instance
  * @param  LayerIndex  Layer foreground or background
  * @param  Address     New LCD frame buffer value
  * @retval BSP status
  */
int32_t BSP_LCD_SetLayerAddress(uint32_t Instance, uint32_t LayerIndex, uint32_t Address)
{
  int32_t ret = BSP_ERROR_NONE;

  if(Instance >= LCD_INSTANCES_NBR)
  {
    ret = BSP_ERROR_WRONG_PARAM;
  }
  else
  {
    if(Lcd_Ctx[Instance].ReloadEnable == 1U)
    {
      (void)HAL_LTDC_SetAddress(&hltdc, Address, LayerIndex);
    }
    else
    {
      (void)HAL_LTDC_SetAddress_NoReload(&hltdc, Address, LayerIndex);
    }
  }

  return ret;
}

/**
  * @brief  Sets display window.
  * @param  Instance    LCD Instance
  * @param  LayerIndex  Layer index
  * @param  Xpos   LCD X position
  * @param  Ypos   LCD Y position
  * @param  Width  LCD window width
  * @param  Height LCD window height
  * @retval BSP status
  */
int32_t BSP_LCD_SetLayerWindow(uint32_t Instance, uint16_t LayerIndex, uint16_t Xpos, uint16_t Ypos, uint16_t Width, uint16_t Height)
{
  int32_t ret = BSP_ERROR_NONE;

  if(Instance >= LCD_INSTANCES_NBR)
  {
    ret = BSP_ERROR_WRONG_PARAM;
  }
  else
  {
    if(Lcd_Ctx[Instance].ReloadEnable == 1U)
    {
      /* Reconfigure the layer size  and position */
      (void)HAL_LTDC_SetWindowSize(&hltdc, Width, Height, LayerIndex);
      (void)HAL_LTDC_SetWindowPosition(&hltdc, Xpos, Ypos, LayerIndex);
    }
    else
    {
      /* Reconfigure the layer size and position */
      (void)HAL_LTDC_SetWindowSize_NoReload(&hltdc, Width, Height, LayerIndex);
      (void)HAL_LTDC_SetWindowPosition_NoReload(&hltdc, Xpos, Ypos, LayerIndex);
    }

    Lcd_Ctx[Instance].XSize = Width;
    Lcd_Ctx[Instance].YSize = Height;
  }

  return ret;
}

/**
  * @brief  Configures and sets the color keying.
  * @param  Instance    LCD Instance
  * @param  LayerIndex  Layer foreground or background
  * @param  Color       Color reference
  * @retval BSP status
  */
/**
  * @brief  Disables the color keying.
  * @param  Instance    LCD Instance
  * @param  LayerIndex Layer foreground or background
  * @retval BSP status
  */
int32_t BSP_LCD_ResetColorKeying(uint32_t Instance, uint32_t LayerIndex)
{
  int32_t ret = BSP_ERROR_NONE;

  if(Instance >= LCD_INSTANCES_NBR)
  {
    ret = BSP_ERROR_WRONG_PARAM;
  }
  else
  {
    if(Lcd_Ctx[Instance].ReloadEnable == 1U)
    {
      /* Disable the color Keying for LCD Layer */
      (void)HAL_LTDC_DisableColorKeying(&hltdc, LayerIndex);
    }
    else
    {
      /* Disable the color Keying for LCD Layer */
      (void)HAL_LTDC_DisableColorKeying_NoReload(&hltdc, LayerIndex);
    }
  }

  return ret;
}

/**
  * @brief  Gets the LCD X size.
  * @param  Instance  LCD Instance
  * @param  XSize     LCD width
  * @retval BSP status
  */
int32_t BSP_LCD_GetXSize(uint32_t Instance, uint32_t *XSize)
{
  int32_t ret = BSP_ERROR_NONE;

  if(Instance >= LCD_INSTANCES_NBR)
  {
    ret = BSP_ERROR_WRONG_PARAM;
  }
  else if(Lcd_Drv->GetXSize != NULL)
  {
    if(Lcd_Drv->GetXSize(Lcd_CompObj, XSize) != BSP_ERROR_NONE)
    {
      ret = BSP_ERROR_COMPONENT_FAILURE;
    }
  }

  return ret;
}

/**
  * @brief  Gets the LCD Y size.
  * @param  Instance  LCD Instance
  * @param  YSize     LCD Height
  * @retval BSP status
  */
int32_t BSP_LCD_GetYSize(uint32_t Instance, uint32_t *YSize)
{
  int32_t ret = BSP_ERROR_NONE;

  if(Instance >= LCD_INSTANCES_NBR)
  {
    ret = BSP_ERROR_WRONG_PARAM;
  }
  else if(Lcd_Drv->GetYSize != NULL)
  {
    if(Lcd_Drv->GetYSize(Lcd_CompObj, YSize) != BSP_ERROR_NONE)
    {
      ret = BSP_ERROR_COMPONENT_FAILURE;
    }
  }

  return ret;
}

/**
  * @brief  Switch On the display.
  * @param  Instance    LCD Instance
  * @retval BSP status
  */
int32_t BSP_LCD_DisplayOn(uint32_t Instance)
{
  int32_t ret;

  if(Instance >= LCD_INSTANCES_NBR)
  {
    ret = BSP_ERROR_WRONG_PARAM;
  }
  else
  {
    if(Lcd_Drv->DisplayOn(Lcd_CompObj) != BSP_ERROR_NONE)
    {
      ret = BSP_ERROR_PERIPH_FAILURE;
    }
    else
    {
      ret = BSP_ERROR_NONE;
    }
  }

  return ret;
}

/**
  * @brief  Switch Off the display.
  * @param  Instance    LCD Instance
  * @retval BSP status
  */
int32_t BSP_LCD_DisplayOff(uint32_t Instance)
{
  int32_t ret;

  if(Instance >= LCD_INSTANCES_NBR)
  {
    ret = BSP_ERROR_WRONG_PARAM;
  }
  else
  {
    if(Lcd_Drv->DisplayOff(Lcd_CompObj) != BSP_ERROR_NONE)
    {
      ret = BSP_ERROR_PERIPH_FAILURE;
    }
    else
    {
      ret = BSP_ERROR_NONE;
    }
  }

  return ret;
}

/**
  * @brief  Set the brightness value
  * @param  Instance    LCD Instance
  * @param  Brightness [00: Min (black), 100 Max]
  * @retval BSP status
  */
int32_t BSP_LCD_SetBrightness(uint32_t Instance, uint32_t Brightness)
{
  int32_t ret = BSP_ERROR_NONE;

  if(Instance >= LCD_INSTANCES_NBR)
  {
    ret = BSP_ERROR_WRONG_PARAM;
  }
  else
  {
    if(Lcd_Drv->SetBrightness(Lcd_CompObj, Brightness) != BSP_ERROR_NONE)
    {
      ret = BSP_ERROR_PERIPH_FAILURE;
    }
  }

  return ret;
}

/**
  * @brief  Set the brightness value
  * @param  Instance    LCD Instance
  * @param  Brightness [00: Min (black), 100 Max]
  * @retval BSP status
  */
int32_t BSP_LCD_GetBrightness(uint32_t Instance, uint32_t *Brightness)
{
  int32_t ret = BSP_ERROR_NONE;

  if(Instance >= LCD_INSTANCES_NBR)
  {
    ret = BSP_ERROR_WRONG_PARAM;
  }
  else
  {
    if(Lcd_Drv->GetBrightness(Lcd_CompObj, Brightness) != BSP_ERROR_NONE)
    {
      ret = BSP_ERROR_PERIPH_FAILURE;
    }
  }

  return ret;
}

/**
  * @brief  Draws a bitmap picture loaded in the internal Flash in currently active layer.
  * @param  Instance LCD Instance
  * @param  Xpos Bmp X position in the LCD
  * @param  Ypos Bmp Y position in the LCD
  * @param  pBmp Pointer to Bmp picture address in the internal Flash.
  * @retval BSP status
  */
int32_t BSP_LCD_DrawBitmap(uint32_t Instance, uint32_t Xpos, uint32_t Ypos, uint8_t *pBmp)
{
  int32_t ret = BSP_ERROR_NONE;
  uint32_t index, width, height, bit_pixel;
  uint32_t Address;
  uint32_t input_color_mode;
  uint8_t *pbmp;

  /* Get bitmap data address offset */
  index = (uint32_t)pBmp[10] + ((uint32_t)pBmp[11] << 8) + ((uint32_t)pBmp[12] << 16)  + ((uint32_t)pBmp[13] << 24);

  /* Read bitmap width */
  width = (uint32_t)pBmp[18] + ((uint32_t)pBmp[19] << 8) + ((uint32_t)pBmp[20] << 16)  + ((uint32_t)pBmp[21] << 24);

  /* Read bitmap height */
  height = (uint32_t)pBmp[22] + ((uint32_t)pBmp[23] << 8) + ((uint32_t)pBmp[24] << 16)  + ((uint32_t)pBmp[25] << 24);

  /* Read bit/pixel */
  bit_pixel = (uint32_t)pBmp[28] + ((uint32_t)pBmp[29] << 8);

  /* Set the address */
  Address = hltdc.LayerCfg[Lcd_Ctx[Instance].ActiveLayer].FBStartAdress + (((Lcd_Ctx[Instance].XSize*Ypos) + Xpos)*Lcd_Ctx[Instance].BppFactor);

  /* Get the layer pixel format */
  if ((bit_pixel/8U) == 4U)
  {
    input_color_mode = DMA2D_INPUT_ARGB8888;
  }
  else if ((bit_pixel/8U) == 2U)
  {
    input_color_mode = DMA2D_INPUT_RGB565;
  }
  else
  {
    input_color_mode = DMA2D_INPUT_RGB888;
  }

  /* Bypass the bitmap header */
  pbmp = pBmp + (index + (width * (height - 1U) * (bit_pixel/8U)));

  /* Convert picture to ARGB8888 pixel format */
  for(index=0; index < height; index++)
  {
    /* Pixel format conversion */
    LL_ConvertLineToRGB(Instance, (uint32_t *)pbmp, (uint32_t *)Address, width, input_color_mode);

    /* Increment the source and destination buffers */
    Address+=  (Lcd_Ctx[Instance].XSize * Lcd_Ctx[Instance].BppFactor);
    pbmp -= width*(bit_pixel/8U);
  }

  return ret;
}
/**
  * @brief  Draw a horizontal line on LCD.
  * @param  Instance LCD Instance.
  * @param  Xpos X position.
  * @param  Ypos Y position.
  * @param  pData Pointer to RGB line data
  * @param  Width Rectangle width.
  * @param  Height Rectangle Height.
  * @retval BSP status.
  */
int32_t BSP_LCD_FillRGBRect(uint32_t Instance, uint32_t Xpos, uint32_t Ypos, uint8_t *pData, uint32_t Width, uint32_t Height)
{
    uint32_t i;

#if (USE_DMA2D_TO_FILL_RGB_RECT == 1)
  uint32_t  Xaddress;
  for(i = 0; i < Height; i++)
  {
    /* Get the line address */
    Xaddress = hltdc.LayerCfg[Lcd_Ctx[Instance].ActiveLayer].FBStartAdress + (Lcd_Ctx[Instance].BppFactor*(((Lcd_Ctx[Instance].XSize + i)*Ypos) + Xpos));

    /* Write line */
    if(Lcd_Ctx[Instance].PixelFormat == LCD_PIXEL_FORMAT_RGB565)
    {
      LL_ConvertLineToRGB(Instance, (uint32_t *)pData, (uint32_t *)Xaddress, Width, DMA2D_INPUT_RGB565);
    }
    else
    {
      LL_ConvertLineToRGB(Instance, (uint32_t *)pData, (uint32_t *)Xaddress, Width, DMA2D_INPUT_ARGB8888);
    }
    pData += Lcd_Ctx[Instance].BppFactor*Width;
  }
#else
  uint32_t color, j;
  for(i = 0; i < Height; i++)
  {
    for(j = 0; j < Width; j++)
    {
      color = *pData | (*(pData + 1) << 8) | (*(pData + 2) << 16) | (*(pData + 3) << 24);
      BSP_LCD_WritePixel(Instance, Xpos + j, Ypos + i, color);
      pData += Lcd_Ctx[Instance].BppFactor;
    }
  }
#endif
  return BSP_ERROR_NONE;
}

/**
  * @brief  Draws an horizontal line in currently active layer.
  * @param  Instance   LCD Instance
  * @param  Xpos  X position
  * @param  Ypos  Y position
  * @param  Length  Line length
  * @param  Color Pixel color
  * @retval BSP status.
  */
int32_t BSP_LCD_DrawHLine(uint32_t Instance, uint32_t Xpos, uint32_t Ypos, uint32_t Length, uint32_t Color)
{
  uint32_t  Xaddress;

  /* Get the line address */
  Xaddress = hltdc.LayerCfg[Lcd_Ctx[Instance].ActiveLayer].FBStartAdress + (Lcd_Ctx[Instance].BppFactor*((Lcd_Ctx[Instance].XSize*Ypos) + Xpos));

  /* Write line */
  if((Xpos + Length) > Lcd_Ctx[Instance].XSize)
  {
    Length = Lcd_Ctx[Instance].XSize - Xpos;
  }
  LL_FillBuffer(Instance, (uint32_t *)Xaddress, Length, 1, 0, Color);

  return BSP_ERROR_NONE;
}

/**
  * @brief  Draws a vertical line in currently active layer.
  * @param  Instance   LCD Instance
  * @param  Xpos  X position
  * @param  Ypos  Y position
  * @param  Length  Line length
  * @param  Color Pixel color
  * @retval BSP status.
  */
int32_t BSP_LCD_DrawVLine(uint32_t Instance, uint32_t Xpos, uint32_t Ypos, uint32_t Length, uint32_t Color)
{
  uint32_t  Xaddress;

  /* Get the line address */
  Xaddress = (hltdc.LayerCfg[Lcd_Ctx[Instance].ActiveLayer].FBStartAdress) + (Lcd_Ctx[Instance].BppFactor*(Lcd_Ctx[Instance].XSize*Ypos + Xpos));

  /* Write line */
  if((Ypos + Length) > Lcd_Ctx[Instance].YSize)
  {
    Length = Lcd_Ctx[Instance].YSize - Ypos;
  }
 LL_FillBuffer(Instance, (uint32_t *)Xaddress, 1, Length, (Lcd_Ctx[Instance].XSize - 1U), Color);

  return BSP_ERROR_NONE;
}

/**
  * @brief  Draws a full rectangle in currently active layer.
  * @param  Instance   LCD Instance
  * @param  Xpos X position
  * @param  Ypos Y position
  * @param  Width Rectangle width
  * @param  Height Rectangle height
  * @param  Color Pixel color
  * @retval BSP status.
  */
int32_t BSP_LCD_FillRect(uint32_t Instance, uint32_t Xpos, uint32_t Ypos, uint32_t Width, uint32_t Height, uint32_t Color)
{
  uint32_t  Xaddress;

  /* Get the rectangle start address */
  Xaddress = (hltdc.LayerCfg[Lcd_Ctx[Instance].ActiveLayer].FBStartAdress) + (Lcd_Ctx[Instance].BppFactor*(Lcd_Ctx[Instance].XSize*Ypos + Xpos));

  /* Fill the rectangle */
 LL_FillBuffer(Instance, (uint32_t *)Xaddress, Width, Height, (Lcd_Ctx[Instance].XSize - Width), Color);

  return BSP_ERROR_NONE;
}

/**
  * @brief  Reads an LCD pixel.
  * @param  Instance    LCD Instance
  * @param  Xpos X position
  * @param  Ypos Y position
  * @param  Color RGB pixel color
  * @retval BSP status
  */
int32_t BSP_LCD_ReadPixel(uint32_t Instance, uint32_t Xpos, uint32_t Ypos, uint32_t *Color)
{
  if(hltdc.LayerCfg[Lcd_Ctx[Instance].ActiveLayer].PixelFormat == LTDC_PIXEL_FORMAT_ARGB8888)
  {
    /* Read data value from SDRAM memory */
    *Color = *(__IO uint32_t*) (hltdc.LayerCfg[Lcd_Ctx[Instance].ActiveLayer].FBStartAdress + (4U*(Ypos*Lcd_Ctx[Instance].XSize + Xpos)));
  }
  else /* if((hltdc.LayerCfg[layer].PixelFormat == LTDC_PIXEL_FORMAT_RGB565) */
  {
    /* Read data value from SDRAM memory */
    *Color = *(__IO uint16_t*) (hltdc.LayerCfg[Lcd_Ctx[Instance].ActiveLayer].FBStartAdress + (2U*(Ypos*Lcd_Ctx[Instance].XSize + Xpos)));
  }

  return BSP_ERROR_NONE;
}

/**
  * @brief  Draws a pixel on LCD.
  * @param  Instance    LCD Instance
  * @param  Xpos X position
  * @param  Ypos Y position
  * @param  Color Pixel color
  * @retval BSP status
  */
int32_t BSP_LCD_WritePixel(uint32_t Instance, uint32_t Xpos, uint32_t Ypos, uint32_t Color)
{
  if(hltdc.LayerCfg[Lcd_Ctx[Instance].ActiveLayer].PixelFormat == LTDC_PIXEL_FORMAT_ARGB8888)
  {
    /* Write data value to SDRAM memory */
    *(__IO uint32_t*) (hltdc.LayerCfg[Lcd_Ctx[Instance].ActiveLayer].FBStartAdress + (4U*(Ypos*Lcd_Ctx[Instance].XSize + Xpos))) = Color;
  }
  else
  {
    /* Write data value to SDRAM memory */
    *(__IO uint16_t*) (hltdc.LayerCfg[Lcd_Ctx[Instance].ActiveLayer].FBStartAdress + (2U*(Ypos*Lcd_Ctx[Instance].XSize + Xpos))) = Color;
  }

  return BSP_ERROR_NONE;
}

/**
  * @}
  */

/** @defgroup STM32H747I_DISCO_LCD_Private_Functions Private Functions
  * @{
  */
/**
  * @brief  Fills a buffer.
  * @param  Instance LCD Instance
  * @param  pDst Pointer to destination buffer
  * @param  xSize Buffer width
  * @param  ySize Buffer height
  * @param  OffLine Offset
  * @param  Color Color index
  */
static void LL_FillBuffer(uint32_t Instance, uint32_t *pDst, uint32_t xSize, uint32_t ySize, uint32_t OffLine, uint32_t Color)
{
  uint32_t output_color_mode, input_color = Color;

  switch(Lcd_Ctx[Instance].PixelFormat)
  {
  case LCD_PIXEL_FORMAT_RGB565:
    output_color_mode = DMA2D_OUTPUT_RGB565; /* RGB565 */
    input_color = CONVERTRGB5652ARGB8888(Color);
    break;
  case LCD_PIXEL_FORMAT_RGB888:
  default:
    output_color_mode = DMA2D_OUTPUT_ARGB8888; /* ARGB8888 */
    break;
  }

  /* Register to memory mode with ARGB8888 as color Mode */
  hlcd_dma2d.Init.Mode         = DMA2D_R2M;
  hlcd_dma2d.Init.ColorMode    = output_color_mode;
  hlcd_dma2d.Init.OutputOffset = OffLine;

  hlcd_dma2d.Instance = DMA2D;

  /* DMA2D Initialization */
  if(HAL_DMA2D_Init(&hlcd_dma2d) == HAL_OK)
  {
    if(HAL_DMA2D_ConfigLayer(&hlcd_dma2d, 1) == HAL_OK)
    {
      if (HAL_DMA2D_Start(&hlcd_dma2d, input_color, (uint32_t)pDst, xSize, ySize) == HAL_OK)
      {
        /* Polling For DMA transfer */
        (void)HAL_DMA2D_PollForTransfer(&hlcd_dma2d, 25);
      }
    }
  }
}

/**
  * @brief  Converts a line to an RGB pixel format.
  * @param  Instance LCD Instance
  * @param  pSrc Pointer to source buffer
  * @param  pDst Output color
  * @param  xSize Buffer width
  * @param  ColorMode Input color mode
  */
static void LL_ConvertLineToRGB(uint32_t Instance, uint32_t *pSrc, uint32_t *pDst, uint32_t xSize, uint32_t ColorMode)
{
  uint32_t output_color_mode;

  switch(Lcd_Ctx[Instance].PixelFormat)
  {
  case LCD_PIXEL_FORMAT_RGB565:
    output_color_mode = DMA2D_OUTPUT_RGB565; /* RGB565 */
    break;
  case LCD_PIXEL_FORMAT_RGB888:
  default:
    output_color_mode = DMA2D_OUTPUT_ARGB8888; /* ARGB8888 */
    break;
  }

  /* Configure the DMA2D Mode, Color Mode and output offset */
  hlcd_dma2d.Init.Mode         = DMA2D_M2M_PFC;
  hlcd_dma2d.Init.ColorMode    = output_color_mode;
  hlcd_dma2d.Init.OutputOffset = 0;

  /* Foreground Configuration */
  hlcd_dma2d.LayerCfg[1].AlphaMode = DMA2D_NO_MODIF_ALPHA;
  hlcd_dma2d.LayerCfg[1].InputAlpha = 0xFF;
  hlcd_dma2d.LayerCfg[1].InputColorMode = ColorMode;
  hlcd_dma2d.LayerCfg[1].InputOffset = 0;

  hlcd_dma2d.Instance = DMA2D;

  /* DMA2D Initialization */
  if(HAL_DMA2D_Init(&hlcd_dma2d) == HAL_OK)
  {
    if(HAL_DMA2D_ConfigLayer(&hlcd_dma2d, 1) == HAL_OK)
    {
      if (HAL_DMA2D_Start(&hlcd_dma2d, (uint32_t)pSrc, (uint32_t)pDst, xSize, 1) == HAL_OK)
      {
        /* Polling For DMA transfer */
        (void)HAL_DMA2D_PollForTransfer(&hlcd_dma2d, 50);
      }
    }
  }
}

/*******************************************************************************
                       BSP Routines:
                                       LTDC
                                       DMA2D
                                       DSI
*******************************************************************************/
/**
  * @brief  Initialize the BSP LTDC Msp.
  * @param  hltdc  LTDC handle
  * @retval None
  */
static void LTDC_MspInit(LTDC_HandleTypeDef *hltdc)
{
  if(hltdc->Instance == LTDC)
  {
	  //printf("== LTDC clock on ==\r\n");

    /** Enable the LTDC clock */
    __HAL_RCC_LTDC_CLK_ENABLE();


    /** Toggle Sw reset of LTDC IP */
    __HAL_RCC_LTDC_FORCE_RESET();
    __HAL_RCC_LTDC_RELEASE_RESET();
  }
}

/**
  * @brief  Initialize the BSP DMA2D Msp.
  * @param  hdma2d  DMA2D handle
  * @retval None
  */
static void DMA2D_MspInit(DMA2D_HandleTypeDef *hdma2d)
{
  if(hdma2d->Instance == DMA2D)
  {
	  //printf("== DMA2D clock on ==\r\n");

    /** Enable the DMA2D clock */
    __HAL_RCC_DMA2D_CLK_ENABLE();

    /** Toggle Sw reset of DMA2D IP */
    __HAL_RCC_DMA2D_FORCE_RESET();
    __HAL_RCC_DMA2D_RELEASE_RESET();
  }
}

/**
  * @brief  De-Initializes the BSP DMA2D Msp
  * @param  hdma2d  DMA2D handle
  * @retval None
  */
static void DMA2D_MspDeInit(DMA2D_HandleTypeDef *hdma2d)
{
  if(hdma2d->Instance == DMA2D)
  {
	  printf("== DMA2D clock off ==\r\n");

    /** Disable IRQ of DMA2D IP */
    HAL_NVIC_DisableIRQ(DMA2D_IRQn);

    /** Force and let in reset state DMA2D */
    __HAL_RCC_DMA2D_FORCE_RESET();

    /** Disable the DMA2D */
    __HAL_RCC_DMA2D_CLK_DISABLE();
  }
}

/**
  * @brief  DCS or Generic short/long write command
  * @param  ChannelNbr Virtual channel ID
  * @param  Reg Register to be written
  * @param  pData pointer to a buffer of data to be write
  * @param  Size To precise command to be used (short or long)
  * @retval BSP status
  */
static int32_t DSI_IO_Write(uint16_t ChannelNbr, uint16_t Reg, uint8_t *pData, uint16_t Size)
{
  int32_t ret = BSP_ERROR_NONE;

  if(Size <= 1U)
  {
    if(HAL_DSI_ShortWrite(&hdsi, ChannelNbr, DSI_DCS_SHORT_PKT_WRITE_P1, Reg, (uint32_t)pData[Size]) != HAL_OK)
    {
      ret = BSP_ERROR_BUS_FAILURE;
    }
  }
  else
  {
    if(HAL_DSI_LongWrite(&hdsi, ChannelNbr, DSI_DCS_LONG_PKT_WRITE, Size, (uint32_t)Reg, pData) != HAL_OK)
    {
      ret = BSP_ERROR_BUS_FAILURE;
    }
  }

  return ret;
}
