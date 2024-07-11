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
#include "mchf_pro_board.h"
#include "GUI_Private.h"

#include "st7701.h"
#include "LCDConf.h"

static const LCD_API_COLOR_CONV	*apColorConvAPI[] =
{
	COLOR_CONVERSION_0,
	#if GUI_NUM_LAYERS > 1
	COLOR_CONVERSION_1,
	#endif
};

static LCD_LayerPropTypedef		layer_prop[GUI_NUM_LAYERS];

LTDC_HandleTypeDef  			hltdc;
DSI_HandleTypeDef   			hdsi;
DSI_VidCfgTypeDef   			hdsivideo_handle;

uint32_t 						lcd_x_size = 0;
uint32_t 						lcd_y_size = 0;

//*----------------------------------------------------------------------------
//* Function Name       : LTDC_IRQHandler
//* Object              :
//* Input Parameters    :
//* Output Parameters   :
//* Functions called    :
//* Context    			: CONTEXT_RESET_VECTOR
//*----------------------------------------------------------------------------
void LTDC_IRQHandler(void)
{
	HAL_LTDC_IRQHandler(&hltdc);
}

//*----------------------------------------------------------------------------
//* Function Name       : DSI_IRQHandler
//* Object              :
//* Input Parameters    :
//* Output Parameters   :
//* Functions called    :
//* Context    			: CONTEXT_RESET_VECTOR
//*----------------------------------------------------------------------------
void DSI_IRQHandler(void)
{
	HAL_DSI_IRQHandler(&hdsi);
}

//*----------------------------------------------------------------------------
//* Function Name       : DMA2D_IRQHandler
//* Object              :
//* Input Parameters    :
//* Output Parameters   :
//* Functions called    :
//* Context    			: CONTEXT_RESET_VECTOR
//*----------------------------------------------------------------------------
void DMA2D_IRQHandler(void)
{
	if(DMA2D->ISR & DMA2D_ISR_TEIF)
	{
		//Error_Handler(16); /* Should never happen */
	}

	/* Clear the Transfer complete interrupt */
	DMA2D->IFCR = (U32)(DMA2D_IFCR_CTCIF | DMA2D_IFCR_CCTCIF | DMA2D_IFCR_CTEIF);

	/* Signal semaphore */
	//osSemaphoreRelease(osDma2dSemph);
}

void DSI_IO_WriteCmd(uint32_t NbrParams, uint8_t *pParams)
{
	if(NbrParams <= 1)
		HAL_DSI_ShortWrite(&hdsi, 0, DSI_DCS_SHORT_PKT_WRITE_P1, pParams[0], pParams[1]);
	else
		HAL_DSI_LongWrite(&hdsi,  0, DSI_DCS_LONG_PKT_WRITE, NbrParams, pParams[NbrParams], pParams);
}

static int32_t DSI_IO_Read(uint16_t Reg, uint8_t *pData, uint16_t Size)
{
	return HAL_DSI_Read(&hdsi, 0, pData, Size, DSI_DCS_SHORT_PKT_READ, Reg, pData);
}

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

static U32 GetPixelformat(U32 LayerIndex)
{
	const LCD_API_COLOR_CONV * pColorConvAPI;

	pColorConvAPI = layer_prop[LayerIndex].pColorConvAPI;

	if (pColorConvAPI == GUICC_M8888I)
	{
		return LTDC_PIXEL_FORMAT_ARGB8888;
	}
	else if (pColorConvAPI == GUICC_M888)
	{
		return LTDC_PIXEL_FORMAT_RGB888;
	}
	else if (pColorConvAPI == GUICC_M565)
	{
		return LTDC_PIXEL_FORMAT_RGB565;
	}

	return 0;
}

static U32 GetBufferSize(U32 LayerIndex)
{
	return (layer_prop[LayerIndex].xSize * layer_prop[LayerIndex].ySize * layer_prop[LayerIndex].BytesPerPixel);
}

static void ClearCacheHook(U32 LayerMask)
{
	int i;
	for (i = 0; i < GUI_NUM_LAYERS; i++)
	{
		if (LayerMask & (1 << i))
		{
			SCB_CleanDCache_by_Addr ((uint32_t *)layer_prop[i].address, LAYER_MEM_REQUIRED);
		}
	}
}

void HAL_LTDC_LineEvenCallback(LTDC_HandleTypeDef *hltdc) {

  U32 Addr;
  U32 layer;
#if 1
  for (layer = 0; layer < GUI_NUM_LAYERS; layer++)
  {
    if (layer_prop[layer].pending_buffer >= 0)
    {
      /* Calculate address of buffer to be used  as visible frame buffer */
      Addr = layer_prop[layer].address + \
             layer_prop[layer].xSize * layer_prop[layer].ySize * layer_prop[layer].pending_buffer * layer_prop[layer].BytesPerPixel;

      __HAL_LTDC_LAYER(hltdc, layer)->CFBAR = Addr;

      __HAL_LTDC_RELOAD_CONFIG(hltdc);

      /* Notify STemWin that buffer is used */
      GUI_MULTIBUF_ConfirmEx(layer, layer_prop[layer].pending_buffer);

      /* Clear pending buffer flag of layer */
      layer_prop[layer].pending_buffer = -1;
    }
  }

  HAL_LTDC_ProgramLineEvent(hltdc, 0);
#endif
}

static void LCD_LL_LayerInit(U32 LayerIndex, U32 address)
{
	LTDC_LayerCfgTypeDef  Layercfg;

	//printf("lcd_x_size: %d\r\n",lcd_x_size);
	//printf("lcd_y_size: %d\r\n",lcd_y_size);

	Layercfg.WindowX0 			= 0;
	Layercfg.WindowX1 			= lcd_x_size;
	Layercfg.WindowY0 			= 0;
	Layercfg.WindowY1 			= lcd_y_size;
	Layercfg.PixelFormat 		= GetPixelformat(LayerIndex);
	Layercfg.FBStartAdress		= address;
	Layercfg.Alpha 				= 255;
	Layercfg.Alpha0 			= 0;
	Layercfg.Backcolor.Blue 	= 0;
	Layercfg.Backcolor.Green 	= 0;
	Layercfg.Backcolor.Red 		= 0;
	Layercfg.BlendingFactor1 	= LTDC_BLENDING_FACTOR1_PAxCA;
	Layercfg.BlendingFactor2 	= LTDC_BLENDING_FACTOR2_PAxCA;
	Layercfg.ImageWidth 		= lcd_x_size;
	Layercfg.ImageHeight 		= lcd_y_size;

	HAL_LTDC_ConfigLayer(&hltdc, &Layercfg, LayerIndex);
}

static void DMA2D_CopyBuffer(U32 LayerIndex, void * pSrc, void * pDst, U32 xSize, U32 ySize, U32 OffLineSrc, U32 OffLineDst)
{
	U32 PixelFormat;

	PixelFormat = GetPixelformat(LayerIndex);
	DMA2D->CR      = 0x00000000UL | (1 << 9);

	// Set up pointers
	DMA2D->FGMAR   = (U32)pSrc;
	DMA2D->OMAR    = (U32)pDst;
	DMA2D->FGOR    = OffLineSrc;
	DMA2D->OOR     = OffLineDst;

	// Set up pixel format
	DMA2D->FGPFCCR = PixelFormat;

	//  Set up size
	DMA2D->NLR     = (U32)(xSize << 16) | (U16)ySize;

	DMA2D->CR     |= DMA2D_CR_START;

	// Wait until transfer is done
	while (DMA2D->CR & DMA2D_CR_START)
	{
		__asm("nop");
	}
}

static void DMA2D_CopyBufferWithAlpha(U32 LayerIndex, void * pSrc, void * pDst, U32 xSize, U32 ySize, U32 OffLineSrc, U32 OffLineDst)
{
	uint32_t PixelFormat;

	PixelFormat = GetPixelformat(LayerIndex);
	DMA2D->CR      = 0x00000000UL | (1 << 9) | (0x2 << 16);

	// Set up pointers
	DMA2D->FGMAR   = (U32)pSrc;
	DMA2D->OMAR    = (U32)pDst;
	DMA2D->BGMAR   = (U32)pDst;
	DMA2D->FGOR    = OffLineSrc;
	DMA2D->OOR     = OffLineDst;
	DMA2D->BGOR     = OffLineDst;

	// Set up pixel format
	DMA2D->FGPFCCR = LTDC_PIXEL_FORMAT_ARGB8888;
	DMA2D->BGPFCCR = PixelFormat;
	DMA2D->OPFCCR = PixelFormat;

	//  Set up size
	DMA2D->NLR     = (U32)(xSize << 16) | (U16)ySize;

	DMA2D->CR     |= DMA2D_CR_START;

	// Wait until transfer is done
	while (DMA2D->CR & DMA2D_CR_START)
	{
		__asm("nop");
	}
}

static void DMA2D_FillBuffer(U32 LayerIndex, void * pDst, U32 xSize, U32 ySize, U32 OffLine, U32 ColorIndex)
{
	U32 PixelFormat;

	PixelFormat = GetPixelformat(LayerIndex);

	// Set up mode
	DMA2D->CR      = 0x00030000UL | (1 << 9);
	DMA2D->OCOLR   = ColorIndex;

	// Set up pointers
	DMA2D->OMAR    = (U32)pDst;

	// Set up offsets
	DMA2D->OOR     = OffLine;

	// Set up pixel format
	DMA2D->OPFCCR  = PixelFormat;

	//  Set up size
	DMA2D->NLR     = (U32)(xSize << 16) | (U16)ySize;

	DMA2D->CR     |= DMA2D_CR_START;

	// Wait until transfer is done
	while (DMA2D->CR & DMA2D_CR_START)
	{
		__asm("nop");
	}
}

static void DMA2D_CopyRGB565(const void * pSrc, void * pDst, int xSize, int ySize, int OffLineSrc, int OffLineDst)
{
	//osMutexWait(osDeviceMutex, SEM_WAIT);

	// Setup DMA2D Configuration
	DMA2D->CR      = 0x00000000UL | (1 << 9) | (1 << 8);
	DMA2D->FGMAR   = (U32)pSrc;
	DMA2D->OMAR    = (U32)pDst;
	DMA2D->FGOR    = OffLineSrc;
	DMA2D->OOR     = OffLineDst;
	DMA2D->FGPFCCR = LTDC_PIXEL_FORMAT_RGB565;
	DMA2D->NLR     = (U32)(xSize << 16) | (U16)ySize;

	// Start the transfer, and enable the transfer complete IT
	DMA2D->CR |= DMA2D_CR_START;

	// Wait until transfer is done
	while (DMA2D->CR & DMA2D_CR_START)
	{
		__asm("nop");
	}

	/* Wait for the end of the transfer */
	//_DMA2D_ExecOperation();
	//osMutexRelease(osDeviceMutex);
}

static void DMA2D_DrawAlphaBitmap(void * pDst, const void * pSrc, int xSize, int ySize, int OffLineSrc, int OffLineDst, int PixelFormat)
{
	//osMutexWait(osDeviceMutex, SEM_WAIT);

	/* Setup DMA2D Configuration */
	DMA2D->CR      = 0x00020000UL | (1 << 9) | (1 << 8);
	DMA2D->FGMAR   = (U32)pSrc;
	DMA2D->BGMAR   = (U32)pDst;
	DMA2D->OMAR    = (U32)pDst;
	DMA2D->FGOR    = OffLineSrc;
	DMA2D->BGOR    = OffLineDst;
	DMA2D->OOR     = OffLineDst;
	DMA2D->FGPFCCR = LTDC_PIXEL_FORMAT_ARGB8888;
	DMA2D->BGPFCCR = PixelFormat;
	DMA2D->OPFCCR  = PixelFormat;
	DMA2D->NLR     = (U32)(xSize << 16) | (U16)ySize;

	// Start the transfer, and enable the transfer complete IT
	DMA2D->CR |= DMA2D_CR_START;

	// Wait until transfer is done
	while (DMA2D->CR & DMA2D_CR_START)
	{
		__asm("nop");
	}

	/* Wait for the end of the transfer */
	//_DMA2D_ExecOperation();
	//osMutexRelease(osDeviceMutex);
}

static void DMA2D_DrawBitmapL8(void * pSrc, void * pDst,  U32 OffSrc, U32 OffDst, U32 PixelFormatDst, U32 xSize, U32 ySize)
{
	// Set up mode
	DMA2D->CR      = 0x00010000UL | (1 << 9);

	// Set up pointers
	DMA2D->FGMAR   = (U32)pSrc;
	DMA2D->OMAR    = (U32)pDst;

	// Set up offsets
	DMA2D->FGOR    = OffSrc;
	DMA2D->OOR     = OffDst;

	// Set up pixel format
	DMA2D->FGPFCCR = LTDC_PIXEL_FORMAT_L8;
	DMA2D->OPFCCR  = PixelFormatDst;

	// Set up size
	DMA2D->NLR     = (U32)(xSize << 16) | ySize;

	// Execute operation
	DMA2D->CR     |= DMA2D_CR_START;

	// Wait until transfer is done
	while (DMA2D->CR & DMA2D_CR_START)
	{
		__asm("nop");
	}
}

static void LCD_DrawMemdev16bpp(void * pDst, const void * pSrc, int xSize, int ySize, int BytesPerLineDst, int BytesPerLineSrc)
{
	int OffLineSrc, OffLineDst;

	OffLineSrc = (BytesPerLineSrc / 2) - xSize;
	OffLineDst = (BytesPerLineDst / 2) - xSize;

	DMA2D_CopyRGB565(pSrc, pDst, xSize, ySize, OffLineSrc, OffLineDst);
}

static void LCD_DrawBitmapAlpha(int LayerIndex, int x, int y, const void * p, int xSize, int ySize, int BytesPerLine)
{
	#if 0
	U32 AddrDst;
	int OffLineSrc, OffLineDst;
	U32 PixelFormat;

	PixelFormat = _GetPixelformat(LayerIndex);
	AddrDst  = LCD_GetBufferAddress(LayerIndex, _aBufferIndex[LayerIndex]);
	AddrDst += ((y * _axSize[LayerIndex] + x) * _aBytesPerPixels[LayerIndex]);
	OffLineSrc = (BytesPerLine / 4) - xSize;
	OffLineDst = _axSize[LayerIndex] - xSize;

	DMA2D_DrawAlphaBitmap((void *)AddrDst, p, xSize, ySize, OffLineSrc, OffLineDst, PixelFormat);
	#endif

	U32 BufferSize, AddrDst;
	int OffLineSrc, OffLineDst;
	U32 PixelFormat;

	PixelFormat = GetPixelformat(LayerIndex);
	BufferSize =  GetBufferSize(LayerIndex);

	//AddrDst = (U32 )_aAddr[LayerIndex] +       BufferSize * _aBufferIndex[LayerIndex]           + (y * _axSize[LayerIndex]          + x) * _aBytesPerPixels[LayerIndex];
	AddrDst = layer_prop[LayerIndex].address + BufferSize * layer_prop[LayerIndex].buffer_index + (y * layer_prop[LayerIndex].xSize + x) * layer_prop[LayerIndex].BytesPerPixel;

	OffLineSrc = (BytesPerLine / 4) - xSize;
	OffLineDst = layer_prop[LayerIndex].xSize /*_axSize[LayerIndex]*/ - xSize;

	DMA2D_DrawAlphaBitmap((void *)AddrDst, p, xSize, ySize, OffLineSrc, OffLineDst, PixelFormat);
}

static void LCD_DrawMemdevAlpha(void * pDst, const void * pSrc, int xSize, int ySize, int BytesPerLineDst, int BytesPerLineSrc)
{
	int OffLineSrc, OffLineDst;

	OffLineSrc = (BytesPerLineSrc / 4) - xSize;
	OffLineDst = (BytesPerLineDst / 4) - xSize;

	DMA2D_DrawAlphaBitmap(pDst, pSrc, xSize, ySize, OffLineSrc, OffLineDst, LTDC_PIXEL_FORMAT_ARGB8888);
}

static void LCD_LL_CopyBuffer(int LayerIndex, int IndexSrc, int IndexDst)
{
	U32 BufferSize, AddrSrc, AddrDst;

	BufferSize = GetBufferSize(LayerIndex);
	AddrSrc    = layer_prop[LayerIndex].address + BufferSize * IndexSrc;
	AddrDst    = layer_prop[LayerIndex].address + BufferSize * IndexDst;

	DMA2D_CopyBuffer(LayerIndex, (void *)AddrSrc, (void *)AddrDst, layer_prop[LayerIndex].xSize, layer_prop[LayerIndex].ySize, 0, 0);

	layer_prop[LayerIndex].buffer_index = IndexDst;
}

static void LCD_LL_DrawBitmap8bpp(int LayerIndex, int x, int y, U8 const * p, int xSize, int ySize, int BytesPerLine)
{
	U32 BufferSize, AddrDst;
	int OffLineSrc, OffLineDst;
	U32 PixelFormat;

	BufferSize = GetBufferSize(LayerIndex);
	AddrDst = layer_prop[LayerIndex].address + BufferSize * layer_prop[LayerIndex].buffer_index + (y * layer_prop[LayerIndex].xSize + x) * layer_prop[LayerIndex].BytesPerPixel;
	OffLineSrc = BytesPerLine - xSize;
	OffLineDst = layer_prop[LayerIndex].xSize - xSize;
	PixelFormat = GetPixelformat(LayerIndex);

	DMA2D_DrawBitmapL8((void *)p, (void *)AddrDst, OffLineSrc, OffLineDst, PixelFormat, xSize, ySize);
}

static void LCD_LL_DrawBitmap16bpp(int LayerIndex, int x, int y, U16 const * p, int xSize, int ySize, int BytesPerLine)
{
	U32 BufferSize, AddrDst;
	int OffLineSrc, OffLineDst;

	BufferSize = GetBufferSize(LayerIndex);
	AddrDst = layer_prop[LayerIndex].address + BufferSize * layer_prop[LayerIndex].buffer_index + (y * layer_prop[LayerIndex].xSize + x) * layer_prop[LayerIndex].BytesPerPixel;
	OffLineSrc = (BytesPerLine / 2) - xSize;
	OffLineDst = layer_prop[LayerIndex].xSize - xSize;

	DMA2D_CopyBuffer(LayerIndex, (void *)p, (void *)AddrDst, xSize, ySize, OffLineSrc, OffLineDst);
}

static void LCD_LL_DrawBitmap32bpp(int LayerIndex, int x, int y, U8 const * p, int xSize, int ySize, int BytesPerLine)
{
	U32 BufferSize, AddrDst;
	int OffLineSrc, OffLineDst;

	BufferSize = GetBufferSize(LayerIndex);
	AddrDst = layer_prop[LayerIndex].address + BufferSize * layer_prop[LayerIndex].buffer_index + (y * layer_prop[LayerIndex].xSize + x) * layer_prop[LayerIndex].BytesPerPixel;
	OffLineSrc = (BytesPerLine / 4) - xSize;
	OffLineDst = layer_prop[LayerIndex].xSize - xSize;

	DMA2D_CopyBufferWithAlpha(LayerIndex, (void *)p, (void *)AddrDst, xSize, ySize, OffLineSrc, OffLineDst);
}

static void LCD_LL_CopyRect(int LayerIndex, int x0, int y0, int x1, int y1, int xSize, int ySize)
{
	#if 1
	U32 BufferSize, AddrSrc, AddrDst;

	BufferSize = GetBufferSize(LayerIndex);
	AddrSrc = layer_prop[LayerIndex].address + BufferSize * layer_prop[LayerIndex].pending_buffer + (y0 * layer_prop[LayerIndex].xSize + x0) * layer_prop[LayerIndex].BytesPerPixel;
	AddrDst = layer_prop[LayerIndex].address + BufferSize * layer_prop[LayerIndex].pending_buffer + (y1 * layer_prop[LayerIndex].xSize + x1) * layer_prop[LayerIndex].BytesPerPixel;
	DMA2D_CopyBuffer(LayerIndex, (void *)AddrSrc, (void *)AddrDst, xSize, ySize, layer_prop[LayerIndex].xSize - xSize, 0);
	#endif

	#if 0
	U32 AddrSrc, AddrDst;
	int l_y0, l_y1;

	// Swapped LCD
	l_y0 = 480 - y0;
	l_y1 = 480 - y1;

	// Calculate source
	AddrSrc = layer_prop[LayerIndex].address + (l_y0 * layer_prop[LayerIndex].xSize + x0) * layer_prop[LayerIndex].BytesPerPixel;

	// Calculate destination
	AddrDst = layer_prop[LayerIndex].address + (l_y1 * layer_prop[LayerIndex].xSize + x1) * layer_prop[LayerIndex].BytesPerPixel;

	// Copy via DMA
	DMA2D_CopyBuffer(	LayerIndex,
						(void *)AddrSrc,
						(void *)AddrDst,
						xSize,
						ySize,
						(layer_prop[LayerIndex].xSize - xSize),
						(layer_prop[LayerIndex].xSize - xSize)
					);
	#endif
}

#if 0
//*----------------------------------------------------------------------------
//* Function Name       : LCD_LL_FillRect
//* Object              :
//* Notes    			: GUI_Clear, GUI_FillRect, GUI_FillRoundedRect
//* Notes   			: GUI_DrawHLine, GUI_DrawVLine
//* Notes   			:
//* Notes   			:
//* Functions called    :
//*----------------------------------------------------------------------------
static void LCD_LL_FillRect(int LayerIndex, int x0_, int y0_, int x1_, int y1_, U32 PixelIndex)
{
	U32 BufferSize, AddrDst;
	int xSize, ySize;
	int mode,k;
	int x0, x1,y0, y1;

	mode = GUI_GetDrawMode();

	printf("-----------------------------------\r\n");
	printf("%d: x0=%d, y0=%d, x1=%d, y1=%d\r\n",mode, x0, y0, x1, y1);

	if(mode == GUI_DM_XOR)
	{
		LCD_SetDevFunc(LayerIndex, LCD_DEVFUNC_FILLRECT, NULL);
		LCD_FillRect(x0, y0, x1, y1);
		LCD_SetDevFunc(LayerIndex, LCD_DEVFUNC_FILLRECT, (void(*)(void))LCD_LL_FillRect);
		return;
	}
	else if(mode == GUI_DM_REV)
	{
		x0 = x0_;
		x1 = x1_;
		y0 = y0_;
		y1 = y1_;

		ySize = x1 - x0 + 1;
		xSize = y1 - y0 + 1;

		//k  = x0;
		//x0 = 479 - x1;
		//x1 = 479 - k;

		printf("%d: x: %d - %d, y: %d - %d (%d,%d)\r\n",mode, x0, x1, y0, y1, xSize, ySize);

		BufferSize = GetBufferSize(LayerIndex);
		AddrDst = layer_prop[LayerIndex].address + BufferSize * layer_prop[LayerIndex].buffer_index + (y0 * layer_prop[LayerIndex].xSize + x0) * layer_prop[LayerIndex].BytesPerPixel;
		DMA2D_FillBuffer(LayerIndex, (void *)AddrDst, xSize, ySize, layer_prop[LayerIndex].xSize - xSize, PixelIndex);
		return;
	}
	else
	{
		x0 = y0_;
		x1 = y1_;
		y0 = x0_;
		y1 = x1_;

		ySize = x1 - x0 + 1;
		xSize = y1 - y0 + 1;

		//k  = x0;
		//x0 = 853 - x0;
		//x1 = 853 - x1;

		printf("%d: x: %d - %d, y: %d - %d (%d,%d)\r\n",mode, x0, x1, y0, y1, xSize, ySize);

		#if 1
		BufferSize = GetBufferSize(LayerIndex);
		AddrDst = layer_prop[LayerIndex].address + BufferSize * layer_prop[LayerIndex].buffer_index + (y0 * layer_prop[LayerIndex].xSize + x0) * layer_prop[LayerIndex].BytesPerPixel;
		DMA2D_FillBuffer(LayerIndex, (void *)AddrDst, xSize, ySize, layer_prop[LayerIndex].xSize - xSize, PixelIndex);
		#endif

		#if 0
		BufferSize = GetBufferSize(LayerIndex);
		AddrDst = layer_prop[LayerIndex].address + BufferSize * layer_prop[LayerIndex].buffer_index + (x0 * layer_prop[LayerIndex].ySize + y0) * layer_prop[LayerIndex].BytesPerPixel;
		DMA2D_FillBuffer(LayerIndex, (void *)AddrDst, xSize, ySize, layer_prop[LayerIndex].ySize - ySize, PixelIndex);
		#endif
	}
}
#endif

#if 0
static void LCD_LL_FillRect(int LayerIndex, int x0, int y0, int x1, int y1, U32 PixelIndex)
{
	U32 BufferSize, AddrDst;
	int xSize, ySize;

	if(GUI_GetDrawMode() == GUI_DM_XOR)
	{
		LCD_SetDevFunc(LayerIndex, LCD_DEVFUNC_FILLRECT, NULL);
		LCD_FillRect(x0, y0, x1, y1);
		LCD_SetDevFunc(LayerIndex, LCD_DEVFUNC_FILLRECT, (void(*)(void))LCD_LL_FillRect);
	}
	else
	{
		#ifdef LCD_LANDSCAPE
		ySize = x1 - x0 + 1;
		xSize = y1 - y0 + 1;
		#else
		xSize = x1 - x0 + 1;
		ySize = y1 - y0 + 1;
		#endif
		BufferSize = GetBufferSize(LayerIndex);
		AddrDst = layer_prop[LayerIndex].address + BufferSize * layer_prop[LayerIndex].buffer_index + (y0 * layer_prop[LayerIndex].xSize + x0) * layer_prop[LayerIndex].BytesPerPixel;
		DMA2D_FillBuffer(LayerIndex, (void *)AddrDst, xSize, ySize, layer_prop[LayerIndex].xSize - xSize, PixelIndex);
	}
}
#endif

#ifndef CONTEXT_TOUCH
void LCD_LL_Reset(void)
{
	GPIO_InitTypeDef  gpio_init_structure;

	gpio_init_structure.Mode  = GPIO_MODE_OUTPUT_PP;
	gpio_init_structure.Pull  = GPIO_PULLUP;
	gpio_init_structure.Speed = GPIO_SPEED_FREQ_LOW;

	// Reset as output
	gpio_init_structure.Pin   = GPIO_PIN_7;
	HAL_GPIO_Init(GPIOH, &gpio_init_structure);

	HAL_GPIO_WritePin(GPIOH, GPIO_PIN_7, GPIO_PIN_RESET);
	vTaskDelay(20);
	HAL_GPIO_WritePin(GPIOH, GPIO_PIN_7, GPIO_PIN_SET);
	vTaskDelay(300);
}
#endif

//*----------------------------------------------------------------------------
//* Function Name       : LCD_LL_Init
//* Object              :
//* Input Parameters    : MIPI/LTDC HW init
//* Output Parameters   :
//* Functions called    : CONTEXT_VIDEO
//*----------------------------------------------------------------------------
static void LCD_LL_Init(void)
{
	DSI_PLLInitTypeDef 				dsiPllInit;
	static RCC_PeriphCLKInitTypeDef	PeriphClkInitStruct;

	uint32_t 						Clockratio = 0;
	uint32_t 						VSYNC;
	uint32_t 						VBP;
	uint32_t 						VFP;
	uint32_t 						HSYNC;
	uint32_t 						HBP;
	uint32_t 						HFP;
	uchar 							id[4];

	// LCD controller needs to be initialised
	// but before Touch process init, as INT
	// pin on the GT911 is output during reset!
	#ifndef CONTEXT_TOUCH
	LCD_LL_Reset();
	#endif

	// Enable video interrupts
	HAL_NVIC_SetPriority(LTDC_IRQn, 0x0F, 0);				// ToDo: Priority a bit high ?
	HAL_NVIC_EnableIRQ(LTDC_IRQn);
	//
	//HAL_NVIC_SetPriority(DMA2D_IRQn, 0x0F, 0);
	//HAL_NVIC_EnableIRQ(DMA2D_IRQn);
	//
	HAL_NVIC_SetPriority(DSI_IRQn, 0x0F, 0);
	HAL_NVIC_EnableIRQ(DSI_IRQn);

	// Enable video clocks
	__HAL_RCC_LTDC_CLK_ENABLE();
	__HAL_RCC_LTDC_FORCE_RESET();
	__HAL_RCC_LTDC_RELEASE_RESET();
	//
	__HAL_RCC_DMA2D_CLK_ENABLE();
	__HAL_RCC_DMA2D_FORCE_RESET();
	__HAL_RCC_DMA2D_RELEASE_RESET();
	//
	__HAL_RCC_DSI_CLK_ENABLE();
	__HAL_RCC_DSI_FORCE_RESET();
	__HAL_RCC_DSI_RELEASE_RESET();

	// Read ID
	if(LCDConf_ReadID(id) != 0)
	{
		printf("== unable to read LCD ID! ==\r\n");
		Error_Handler(222);
	}

	// Check if supported
	if((id[0] != 0x40)&&(id[0] != 0xFF))
	{
		printf("== not supported lcd! ==\r\n");
		Error_Handler(222);
	}

	hdsi.Instance = DSI;
	HAL_DSI_DeInit(&(hdsi));

	#if (LCD_LANE_CLK == 62500)
	// 62.5/32.25 Mhz lane clock (PLL out = 500 Mhz, 15.625 MHz TX)
    hdsi.Init.AutomaticClockLaneControl	= DSI_AUTO_CLK_LANE_CTRL_DISABLE;
    hdsi.Init.NumberOfLanes 			= DSI_TWO_DATA_LANES;
    dsiPllInit.PLLIDF   				= DSI_PLL_IN_DIV5;
    dsiPllInit.PLLNDIV  				= 100;
	dsiPllInit.PLLODF 					= DSI_PLL_OUT_DIV1;
	hdsi.Init.TXEscapeCkdiv 			= LCD_LANE_CLK/15620;					/* TXEscapeCkdiv = f(LaneByteClk)/15.62 = 4 */
	#else
	// 58.75/29.375 Mhz lane clock	(PLL out = 470 Mhz, 14.6875 MHz TX)
    hdsi.Init.AutomaticClockLaneControl	= DSI_AUTO_CLK_LANE_CTRL_DISABLE;
    hdsi.Init.NumberOfLanes 			= DSI_TWO_DATA_LANES;
    dsiPllInit.PLLIDF   				= DSI_PLL_IN_DIV5;		// IDF 			= 5
    dsiPllInit.PLLNDIV  				= 94;					// NDIF 		= 94
	dsiPllInit.PLLODF 					= DSI_PLL_OUT_DIV1;		// ODF 			= 1
	hdsi.Init.TXEscapeCkdiv 			= 4;					// TX Prescaler = 4
	#endif

	HAL_DSI_Init(&(hdsi), &(dsiPllInit));

    // Timing parameters for all Video modes
    if(id[0] == 0x40)
    {
    	VSYNC  		= OTM8009A_800X480_VSYNC;
    	VBP  		= OTM8009A_800X480_VBP;
    	VFP  		= OTM8009A_800X480_VFP;
    	HSYNC  		= OTM8009A_800X480_HSYNC;
    	HBP  		= OTM8009A_800X480_HBP;
    	HFP  		= OTM8009A_800X480_HFP;
    	//
    	// Portrait mode (we rotate in emWin driver)
    	lcd_x_size	= OTM8009A_480X800_WIDTH;
    	lcd_y_size 	= OTM8009A_480X800_HEIGHT;
    	//
    	Clockratio 	= LCD_LANE_CLK/OTM8009A_PIXEL_CLK;
    }
    else
    {
    	VSYNC  		= ST7701_VSYNC;
    	VBP  		= ST7701_VBP;
    	VFP  		= ST7701_VFP;
    	lcd_y_size 	= ST7701_HEIGHT;

    	HSYNC  		= ST7701_HSYNC;
    	HBP  		= ST7701_HBP;
    	HFP  		= ST7701_HFP;
    	lcd_x_size 	= ST7701_WIDTH;

    	Clockratio 	= LCD_LANE_CLK/ST7701_PIXEL_CLK;
    }

    // The reference value given by the manufacturer is 58.2MHz,  then fps is :
    // fps = 58200000 / (480 + 160 + 160 +24) * (1280 + 12 + 10 + 2) = 54Hz
    int refresh_rate   = (ST7701_PIXEL_CLK * 1000)/((lcd_x_size + HSYNC + HBP + HFP)*(VSYNC + lcd_y_size + VBP + VFP));
    int refresh_rate_m = (ST7701_PIXEL_CLK * 1000)%((lcd_x_size + HSYNC + HBP + HFP)*(VSYNC + lcd_y_size + VBP + VFP));

    // That is, the transmission rate of each MIPI data lane.
    // dsi_hs_clk = ((h_active + hfp + hbp + h_sync) * (v_active + vfp + vbp + v_sync) * fps * bpp) / lane_number
    // dsi_hs_clk = ((480 + 160 + 160 +24) * (1280 + 12 + 10 + 2) * 54 * 24) / 4 = 348136704 bps = 348 Mbps
    int dsi_bandwidth = (((lcd_x_size + HSYNC + HBP + HFP)*(VSYNC + lcd_y_size + VBP + VFP)) * refresh_rate * 24)/2;

    printf("== LCD CLK: %dkHz, FPS: %d.%dHz, bandwidth 2x%dMbps ==\r\n", ST7701_PIXEL_CLK, refresh_rate, refresh_rate_m/10000, dsi_bandwidth/(1000*1000));

    hdsivideo_handle.VirtualChannelID 					= 0;
    hdsivideo_handle.ColorCoding 						= DSI_COLOR;
    hdsivideo_handle.LooselyPacked    					= DSI_LOOSELY_PACKED_DISABLE;
    hdsivideo_handle.Mode 								= DSI_VID_MODE_BURST;
    hdsivideo_handle.PacketSize                			= lcd_x_size;
    hdsivideo_handle.NumberOfChunks 					= 0;
    hdsivideo_handle.NullPacketSize 					= 0xFFF;
    hdsivideo_handle.VSPolarity 						= DSI_VSYNC_ACTIVE_HIGH;
    hdsivideo_handle.HSPolarity 						= DSI_HSYNC_ACTIVE_HIGH;
    hdsivideo_handle.DEPolarity 						= DSI_DATA_ENABLE_ACTIVE_HIGH;
    hdsivideo_handle.HorizontalSyncActive      			= (HSYNC*Clockratio);
    hdsivideo_handle.HorizontalBackPorch       			= (HBP	*Clockratio);
    hdsivideo_handle.HorizontalLine            			= (lcd_x_size + HSYNC + HBP + HFP)*Clockratio;
    hdsivideo_handle.VerticalSyncActive        			= VSYNC;
    hdsivideo_handle.VerticalBackPorch         			= VBP;
    hdsivideo_handle.VerticalFrontPorch        			= VFP;
    hdsivideo_handle.VerticalActive            			= lcd_y_size;

    // Enable or disable sending LP command while streaming is active in video mode
    hdsivideo_handle.LPCommandEnable 					= DSI_LP_COMMAND_ENABLE;

    //if(id[0] == 0x40)
    //{
    	hdsivideo_handle.LPLargestPacketSize 				= 64;
    	hdsivideo_handle.LPVACTLargestPacketSize 			= 64;
    //}
    //else
    //{
    //	hdsivideo_handle.LPLargestPacketSize 				= 4;
    //	hdsivideo_handle.LPVACTLargestPacketSize 			= 4;
    //}

    /* Specify for each region of the video frame, if the transmission of command in LP mode is allowed in this region */
    /* while streaming is active in video mode                                                                         */
    hdsivideo_handle.LPHorizontalFrontPorchEnable 		= DSI_LP_HFP_ENABLE;
    hdsivideo_handle.LPHorizontalBackPorchEnable  		= DSI_LP_HBP_ENABLE;
    hdsivideo_handle.LPVerticalActiveEnable 			= DSI_LP_VACT_ENABLE;
    hdsivideo_handle.LPVerticalFrontPorchEnable 		= DSI_LP_VFP_ENABLE;
    hdsivideo_handle.LPVerticalBackPorchEnable 			= DSI_LP_VBP_ENABLE;
    hdsivideo_handle.LPVerticalSyncActiveEnable 		= DSI_LP_VSYNC_ENABLE;

    /* Configure DSI Video mode timings with settings set above */
    HAL_DSI_ConfigVideoMode(&(hdsi), &(hdsivideo_handle));

    // LCD clock configuration
	#if (LCD_LANE_CLK == 62500)
    // 27.5 Mhz
    PeriphClkInitStruct.PLL3.PLL3M      				= 5U;
    PeriphClkInitStruct.PLL3.PLL3N      				= 132U;
    PeriphClkInitStruct.PLL3.PLL3P      				= 2U;
    PeriphClkInitStruct.PLL3.PLL3Q      				= 2U;
    PeriphClkInitStruct.PLL3.PLL3R      				= 24U;
	#else
    // 29.375 Mhz (same as lane clock ??)
    PeriphClkInitStruct.PLL3.PLL3M      				= 5U;	// DIVM3 = 5
    PeriphClkInitStruct.PLL3.PLL3N      				= 132U;	// DIVN3 = 141
    PeriphClkInitStruct.PLL3.PLL3R      				= 24U;	// DIVR3 = 24
    PeriphClkInitStruct.PLL3.PLL3P      				= 2U;	// NOT USED ?
    PeriphClkInitStruct.PLL3.PLL3Q      				= 2U;	// NOT USED ?
	#endif
    PeriphClkInitStruct.PeriphClockSelection   			= RCC_PERIPHCLK_LTDC;
    HAL_RCCEx_PeriphCLKConfig(&PeriphClkInitStruct);

   	hltdc.Instance 					= LTDC;
   	hltdc.Init.HSPolarity 			= LTDC_HSPOLARITY_AL;
   	hltdc.Init.VSPolarity 			= LTDC_VSPOLARITY_AL;
   	hltdc.Init.DEPolarity 			= LTDC_DEPOLARITY_AL;
   	hltdc.Init.PCPolarity 			= LTDC_PCPOLARITY_IPC;

   	hltdc.Init.HorizontalSync     	= (HSYNC - 0);
   	hltdc.Init.AccumulatedHBP     	= (HSYNC + HBP - 0);
   	hltdc.Init.AccumulatedActiveW	= (HSYNC + lcd_x_size + HBP - 0);
   	hltdc.Init.TotalWidth         	= (HSYNC + lcd_x_size + HBP + HFP - 0);
   	hltdc.Init.VerticalSync       	= (VSYNC - 0);
   	hltdc.Init.AccumulatedVBP     	= (VSYNC + VBP - 0);
   	hltdc.Init.AccumulatedActiveH 	= (VSYNC + lcd_y_size + VBP - 0);
   	hltdc.Init.TotalHeigh         	= (VSYNC + lcd_y_size + VBP + VFP - 0);

   	hltdc.Init.Backcolor.Blue  		= 0x00;
   	hltdc.Init.Backcolor.Green 		= 0x00;
   	hltdc.Init.Backcolor.Red   		= 0x00;

  	// Initialise the LTDC
  	HAL_LTDC_Init(&hltdc);

    // Enable the DSI host and wrapper : but LTDC is not started yet at this stage
    HAL_DSI_Start(&(hdsi));

  	// Init LCD registers
	if(id[0] == 0x40)
		OTM8009A_Init(OTM8009A_FORMAT_RGB888, OTM8009A_ORIENTATION_PORTRAIT);
	else
	{
		//HAL_DSI_ConfigFlowControl(&hdsi, DSI_FLOW_CONTROL_BTA);
		ST7701S_Init(hdsivideo_handle.ColorCoding);
	}

  	// Start buffer refresh
  	//HAL_LTDC_ProgramLineEvent(&hltdc, 0);

//#ifdef BOARD_EVAL_747
	// Backlight on
	HAL_GPIO_WritePin(LCD_BL_CTRL_GPIO_PORT, LCD_BL_CTRL_PIN, GPIO_PIN_SET);
//#endif
}


//*----------------------------------------------------------------------------
//* Function Name       : LCD_X_DisplayDriver
//* Object              :
//* Input Parameters    : STemWin callback
//* Output Parameters   :
//* Functions called    : CONTEXT_VIDEO
//*----------------------------------------------------------------------------
int LCD_X_DisplayDriver(unsigned LayerIndex, unsigned Cmd, void * pData)
{
	int 	r = 0;
	U32 	addr;
	int	 	xPos, yPos;
	U32 	Color;

	//printf("%s:LCD_X_DisplayDriver cmd %d\r\n",  pcTaskGetName(NULL), Cmd);

	switch (Cmd)
	{
    		case LCD_X_INITCONTROLLER:
    			LCD_LL_LayerInit(LayerIndex, layer_prop[LayerIndex].address);
    			break;

    		case LCD_X_SETORG:
    			addr = layer_prop[LayerIndex].address + ((LCD_X_SETORG_INFO *)pData)->yPos * layer_prop[LayerIndex].xSize * layer_prop[LayerIndex].BytesPerPixel;
    			HAL_LTDC_SetAddress(&hltdc, addr, LayerIndex);
    			break;

    		case LCD_X_SHOWBUFFER:
    			layer_prop[LayerIndex].pending_buffer = ((LCD_X_SHOWBUFFER_INFO *)pData)->Index;
    			break;

    		case LCD_X_ON:
    			__HAL_LTDC_ENABLE(&hltdc);
    			break;

    		case LCD_X_OFF:
    			__HAL_LTDC_DISABLE(&hltdc);
    			break;

    		case LCD_X_SETVIS:
    		{
    			if(((LCD_X_SETVIS_INFO *)pData)->OnOff  == ENABLE )
    			{
    				__HAL_LTDC_LAYER_ENABLE(&hltdc, LayerIndex);
    			}
    			else
    			{
    				__HAL_LTDC_LAYER_DISABLE(&hltdc, LayerIndex);
    			}
    			__HAL_LTDC_RELOAD_CONFIG(&hltdc);

    			break;
    		}

    		case LCD_X_SETPOS:
    		{
    			HAL_LTDC_SetWindowPosition(	&hltdc,
    										((LCD_X_SETPOS_INFO *)pData)->xPos,
    										((LCD_X_SETPOS_INFO *)pData)->yPos,
    										LayerIndex);
    			break;
    		}

    		case LCD_X_SETSIZE:
    		{
    			GUI_GetLayerPosEx(LayerIndex, &xPos, &yPos);
    			layer_prop[LayerIndex].xSize = ((LCD_X_SETSIZE_INFO *)pData)->xSize;
    			layer_prop[LayerIndex].ySize = ((LCD_X_SETSIZE_INFO *)pData)->ySize;
    			HAL_LTDC_SetWindowPosition(&hltdc, xPos, yPos, LayerIndex);
    			break;
    		}

    		case LCD_X_SETALPHA:
    			HAL_LTDC_SetAlpha(&hltdc, ((LCD_X_SETALPHA_INFO *)pData)->Alpha, LayerIndex);
    			break;

    		case LCD_X_SETCHROMAMODE:
    		{
    			if(((LCD_X_SETCHROMAMODE_INFO *)pData)->ChromaMode != 0)
    			{
    				HAL_LTDC_EnableColorKeying(&hltdc, LayerIndex);
    			}
    			else
    			{
    				HAL_LTDC_DisableColorKeying(&hltdc, LayerIndex);
    			}
    			break;
    		}

    		case LCD_X_SETCHROMA:
    		{
    			Color = ((((LCD_X_SETCHROMA_INFO *)pData)->ChromaMin & 0xFF0000) >> 16) |\
    					(((LCD_X_SETCHROMA_INFO *)pData)->ChromaMin & 0x00FF00) |\
    					((((LCD_X_SETCHROMA_INFO *)pData)->ChromaMin & 0x0000FF) << 16);

    			HAL_LTDC_ConfigColorKeying(&hltdc, Color, LayerIndex);
    			break;
    		}

    		default:
    			r = -1;
    			break;
	}

	//printf("LCD_X_DisplayDriver->ok\r\n");
	return r;
}

//*----------------------------------------------------------------------------
//* Function Name       : LCD_X_Config
//* Object              :
//* Input Parameters    : STemWin callback
//* Output Parameters   :
//* Functions called    : CONTEXT_VIDEO
//*----------------------------------------------------------------------------
void LCD_X_Config(void)
{
	U32 i;

	//printf("%s:LCD_X_Config\r\n",  pcTaskGetName(NULL));

	// Initialize GUI Layer structure
	layer_prop[0].address = LCD_LAYER0_FRAME_BUFFER;
	#if (GUI_NUM_LAYERS > 1)
	layer_prop[1].address = LCD_LAYER1_FRAME_BUFFER;
	#endif

	// After buffer addresses are known
	GUI_DCACHE_SetClearCacheHook(ClearCacheHook);

	// Basic hw init
	LCD_LL_Init();

	#if 0
	__HAL_DSI_WRAPPER_DISABLE(&hdsi);
	// Moved from LCD_X_DisplayDriver
	LCD_LL_LayerInit(0, layer_prop[0].address);
	//HAL_LTDC_SetPitch(&hltdc, lcd_x_size, 0);
	#if (GUI_NUM_LAYERS > 1)
	LCD_LL_LayerInit(1, layer_prop[1].address);
	//HAL_LTDC_SetPitch(&hltdc, lcd_x_size, 1);
	#endif
	__HAL_DSI_WRAPPER_ENABLE(&hdsi);
	#endif

	// At first initialize use of multiple buffers on demand
	#if (NUM_BUFFERS > 1)
	for (i = 0; i < GUI_NUM_LAYERS; i++)
		GUI_MULTIBUF_ConfigEx(i, NUM_BUFFERS);
	#endif

	// Set display driver and color conversion for 1st layer
	GUI_DEVICE_CreateAndLink(DISPLAY_DRIVER_0, COLOR_CONVERSION_0, 0, 0);

	// Set size of 1st layer
	LCD_SetSizeEx (0, lcd_y_size, 					lcd_x_size);
	LCD_SetVSizeEx(0, lcd_y_size * NUM_VSCREENS, 	lcd_x_size);

	#if (GUI_NUM_LAYERS > 1)
	// Set display driver and color conversion for 2nd layer
	GUI_DEVICE_CreateAndLink(DISPLAY_DRIVER_1, COLOR_CONVERSION_1, 0, 1);

	// Set size of 2nd layer
	LCD_SetSizeEx (1, lcd_y_size, 					lcd_x_size);
	LCD_SetVSizeEx(1, lcd_y_size * NUM_VSCREENS, 	lcd_x_size);
	#endif

	// Setting up VRam address and custom functions for CopyBuffer-, CopyRect- and FillRect operations
	for (i = 0; i < GUI_NUM_LAYERS; i++)
	{
		layer_prop[i].pColorConvAPI = (LCD_API_COLOR_CONV *)apColorConvAPI[i];
		layer_prop[i].pending_buffer = -1;

		// Set VRAM address
		LCD_SetVRAMAddrEx(i, (void *)(layer_prop[i].address));

		// Remember color depth for further operations
		layer_prop[i].BytesPerPixel = LCD_GetBitsPerPixelEx(i) >> 3;

		// Set custom functions for several operations
		LCD_SetDevFunc(i, LCD_DEVFUNC_COPYBUFFER, 	(void(*)(void))LCD_LL_CopyBuffer);
		//LCD_SetDevFunc(i, LCD_DEVFUNC_COPYRECT,   	(void(*)(void))LCD_LL_CopyRect);	- not working!

		// Filling via DMA2D does only work with 16bpp or more
		//LCD_SetDevFunc(i, LCD_DEVFUNC_FILLRECT, 	(void(*)(void))LCD_LL_FillRect); - DMA2D implementation doesn't work ;(
		LCD_SetDevFunc(i, LCD_DEVFUNC_DRAWBMP_8BPP, (void(*)(void))LCD_LL_DrawBitmap8bpp);
		LCD_SetDevFunc(i, LCD_DEVFUNC_DRAWBMP_16BPP,(void(*)(void))LCD_LL_DrawBitmap16bpp);
		LCD_SetDevFunc(i, LCD_DEVFUNC_DRAWBMP_32BPP,(void(*)(void))LCD_LL_DrawBitmap32bpp);
	}

	#if GUI_SUPPORT_MEMDEV
	//GUI_MEMDEV_SetDrawMemdev16bppFunc(LCD_DrawMemdev16bpp);
	GUI_SetFuncDrawAlpha			 (LCD_DrawMemdevAlpha, LCD_DrawBitmapAlpha);
	#endif

	// Basic hw init
	//LCD_LL_Init();

	// Start buffer refresh
  	HAL_LTDC_ProgramLineEvent(&hltdc, 0);

	//printf("LCD_X_Config->ok\r\n");
}
