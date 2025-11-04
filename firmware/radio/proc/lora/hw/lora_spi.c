/************************************************************************************
**                                                                                 **
**                             mcHF Pro QRP Transceiver                            **
**                         Krassi Atanassov - M0NKA, 2013-2025                     **
**                                                                                 **
**---------------------------------------------------------------------------------**
**                                                                                 **
**  File name:                                                                     **
**  Description:                                                                   **
**  Last Modified:                                                                 **
**  Licence:               GNU GPLv3                                               **
************************************************************************************/
#include "main.h"
#include "mchf_pro_board.h"

#ifdef CONTEXT_LORA

#include "lora_spi.h"

SPI_HandleTypeDef SpiHandle1;
DMA_HandleTypeDef hdma_tx;
DMA_HandleTypeDef hdma_rx;

const uint8_t LoraTxBuffer[] = "aaaaaaaaaaaaaaaa";

#define BUFFER_ALIGNED_SIZE (((BUFFERSIZE+31)/32)*32)
ALIGN_32BYTES(uint8_t LoraRxBuffer[BUFFER_ALIGNED_SIZE]);

__IO uint32_t wTransferState = TRANSFER_WAIT;

void HAL_SPI_TxRxCpltCallback(SPI_HandleTypeDef *hspi)
{
  wTransferState = TRANSFER_COMPLETE;
}

void HAL_SPI_ErrorCallback(SPI_HandleTypeDef *hspi)
{
  wTransferState = TRANSFER_ERROR;
}

static uint16_t Buffercmp(uint8_t* pBuffer1, uint8_t* pBuffer2, uint16_t BufferLength)
{
  while (BufferLength--)
  {
    if((*pBuffer1) != *pBuffer2)
    {
      return BufferLength;
    }
    pBuffer1++;
    pBuffer2++;
  }

  return 0;
}

static void lora_spi_misc_gpio_config(void)
{
	LL_GPIO_InitTypeDef  GPIO_InitStruct;

	GPIO_InitStruct.Mode      = LL_GPIO_MODE_OUTPUT;
	GPIO_InitStruct.Pull      = LL_GPIO_PULL_NO;
	GPIO_InitStruct.Speed     = LL_GPIO_SPEED_LOW;

	// Lora power off
	lora_spi_power_state(0);

	// All low
	//LL_GPIO_ResetOutputPin(RFM_RST_PORT,  RFM_RST); - this GND on the module!!!
	LL_GPIO_ResetOutputPin(RFM_NSS_PORT,  RFM_NSS);
	LL_GPIO_ResetOutputPin(RFM_DIO1_PORT, RFM_DIO1);
	LL_GPIO_ResetOutputPin(RFM_DIO0_PORT, RFM_DIO0);

	// Reset line, PA0
	//GPIO_InitStruct.Pin       = RFM_RST;
	//LL_GPIO_Init(RFM_RST_PORT, &GPIO_InitStruct);

	// Chip select, PC1
	GPIO_InitStruct.Pin       = RFM_NSS;
	LL_GPIO_Init(RFM_NSS_PORT, &GPIO_InitStruct);

	// GPIO1, PC4
	GPIO_InitStruct.Pin       = RFM_DIO1;
	LL_GPIO_Init(RFM_DIO1_PORT, &GPIO_InitStruct);

	// GPIO0, PC5
	GPIO_InitStruct.Pin       = RFM_DIO0;
	LL_GPIO_Init(RFM_DIO0_PORT, &GPIO_InitStruct);

	// POWER, PA2
	GPIO_InitStruct.Pin       = LORA_POWER;
	LL_GPIO_Init(LORA_POWER_PORT, &GPIO_InitStruct);
}

static void lora_spi_gpio_config(void)
{
	LL_GPIO_InitTypeDef  GPIO_InitStruct;

	#ifndef SPI_GPIO_TEST
	GPIO_InitStruct.Mode      = LL_GPIO_MODE_ALTERNATE;
	#else
	GPIO_InitStruct.Mode      = LL_GPIO_MODE_OUTPUT;
	#endif

	GPIO_InitStruct.Pull      = LL_GPIO_PULL_DOWN;
	GPIO_InitStruct.Speed     = LL_GPIO_SPEED_HIGH;

	GPIO_InitStruct.Pin       = RFM_MISO_SPI1;
	GPIO_InitStruct.Alternate = LL_GPIO_AF_5;
	LL_GPIO_Init(RFM_MISO_SPI1_PORT, &GPIO_InitStruct);

	GPIO_InitStruct.Pin       = RFM_MOSI_SPI1;
	GPIO_InitStruct.Alternate = LL_GPIO_AF_5;
	LL_GPIO_Init(RFM_MOSI_SPI1_PORT, &GPIO_InitStruct);

	GPIO_InitStruct.Pin       = RFM_SCK_SPI1;
	GPIO_InitStruct.Alternate = LL_GPIO_AF_5;
	LL_GPIO_Init(RFM_SCK_SPI1_PORT, &GPIO_InitStruct);
}

void HAL_SPI_MspInit(SPI_HandleTypeDef *hspi)
{
  if (hspi->Instance == SPI1)
  {
    SPI1_SCK_GPIO_CLK_ENABLE();
    SPI1_MISO_GPIO_CLK_ENABLE();
    SPI1_MOSI_GPIO_CLK_ENABLE();

    SPI1_CLK_ENABLE();
    DMA1_CLK_ENABLE();

    hdma_tx.Instance                 = SPI1_TX_DMA_STREAM;
    hdma_tx.Init.FIFOMode            = DMA_FIFOMODE_DISABLE;
    hdma_tx.Init.FIFOThreshold       = DMA_FIFO_THRESHOLD_FULL;
    hdma_tx.Init.MemBurst            = DMA_MBURST_INC4;
    hdma_tx.Init.PeriphBurst         = DMA_PBURST_INC4;
    hdma_tx.Init.Request             = SPI1_TX_DMA_REQUEST;
    hdma_tx.Init.Direction           = DMA_MEMORY_TO_PERIPH;
    hdma_tx.Init.PeriphInc           = DMA_PINC_DISABLE;
    hdma_tx.Init.MemInc              = DMA_MINC_ENABLE;
    hdma_tx.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
    hdma_tx.Init.MemDataAlignment    = DMA_MDATAALIGN_BYTE;
    hdma_tx.Init.Mode                = DMA_NORMAL;
    hdma_tx.Init.Priority            = DMA_PRIORITY_LOW;

    HAL_DMA_Init(&hdma_tx);

    __HAL_LINKDMA(hspi, hdmatx, hdma_tx);
/*
    hdma_rx.Instance                 = SPI1_RX_DMA_STREAM;
    hdma_rx.Init.FIFOMode            = DMA_FIFOMODE_DISABLE;
    hdma_rx.Init.FIFOThreshold       = DMA_FIFO_THRESHOLD_FULL;
    hdma_rx.Init.MemBurst            = DMA_MBURST_INC4;
    hdma_rx.Init.PeriphBurst         = DMA_PBURST_INC4;
    hdma_rx.Init.Request             = SPI1_RX_DMA_REQUEST;
    hdma_rx.Init.Direction           = DMA_PERIPH_TO_MEMORY;
    hdma_rx.Init.PeriphInc           = DMA_PINC_DISABLE;
    hdma_rx.Init.MemInc              = DMA_MINC_ENABLE;
    hdma_rx.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
    hdma_rx.Init.MemDataAlignment    = DMA_MDATAALIGN_BYTE;
    hdma_rx.Init.Mode                = DMA_NORMAL;
    hdma_rx.Init.Priority            = DMA_PRIORITY_HIGH;

    HAL_DMA_Init(&hdma_rx);

    __HAL_LINKDMA(hspi, hdmarx, hdma_rx);*/

    HAL_NVIC_SetPriority(SPI1_DMA_TX_IRQn, 1, 1);
    HAL_NVIC_EnableIRQ(SPI1_DMA_TX_IRQn);

    //HAL_NVIC_SetPriority(SPI1_DMA_RX_IRQn, 15, 0);
    //HAL_NVIC_EnableIRQ(SPI1_DMA_RX_IRQn);

    HAL_NVIC_SetPriority(SPI1_IRQn, 1, 0);
    HAL_NVIC_EnableIRQ(SPI1_IRQn);

  }
}

uchar lora_spi_init(void)
{
	printf("spi start\r\n");

	SpiHandle1.Instance               = SPI1;

	SpiHandle1.Init.Mode              = SPI_MODE_MASTER;
	SpiHandle1.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_32;
	SpiHandle1.Init.Direction         = SPI_DIRECTION_2LINES;
	SpiHandle1.Init.CLKPhase          = SPI_PHASE_1EDGE;

	SpiHandle1.Init.CLKPolarity       = SPI_POLARITY_LOW;

	SpiHandle1.Init.DataSize          = SPI_DATASIZE_8BIT;

	SpiHandle1.Init.FirstBit          = SPI_FIRSTBIT_MSB;

	SpiHandle1.Init.TIMode            = SPI_TIMODE_DISABLE;

	SpiHandle1.Init.CRCCalculation    = SPI_CRCCALCULATION_DISABLE;

	SpiHandle1.Init.CRCPolynomial     = 7;

	SpiHandle1.Init.CRCLength         = SPI_CRC_LENGTH_8BIT;

	SpiHandle1.Init.NSS               = SPI_NSS_SOFT;

	SpiHandle1.Init.NSSPMode          = SPI_NSS_PULSE_DISABLE;

	SpiHandle1.Init.MasterKeepIOState = SPI_MASTER_KEEP_IO_STATE_ENABLE;  // Recommended setting to avoid glitches


	if(HAL_SPI_Init(&SpiHandle1) != HAL_OK)

	{
		printf("spi err1\r\n");
		return 1;

	}



	//if(HAL_SPI_TransmitReceive_DMA(&SpiHandle1, (uint8_t*)LoraTxBuffer, (uint8_t *)LoraRxBuffer, BUFFERSIZE) != HAL_OK)
	if(HAL_SPI_Transmit_DMA(&SpiHandle1, (uint8_t*)LoraTxBuffer, BUFFERSIZE) != HAL_OK)
	{
		printf("spi err2\r\n");
		return 2;

	}
	printf("spi tx ok\r\n");

	while (wTransferState == TRANSFER_WAIT)

	{

	}

	printf("spi wait ok\r\n");

	// Invalidate cache prior to access by CPU

	SCB_InvalidateDCache_by_Addr ((uint32_t *)LoraRxBuffer, BUFFERSIZE);


	switch(wTransferState)

	{

		case TRANSFER_COMPLETE :
	      if(Buffercmp((uint8_t*)LoraTxBuffer, (uint8_t*)LoraRxBuffer, BUFFERSIZE))
	      {
	        return 3;
	      }
	      break;
	    default :
	      return 4;
	      break;
	  }

	  return 0;

}

void lora_gpio_init(void)
{
	lora_spi_misc_gpio_config();
	lora_spi_gpio_config();
}

void lora_spi_power_state(uchar on)
{
	if(on)
	{
		#ifndef LORA_POWER_INV
		LL_GPIO_SetOutputPin(LORA_POWER_PORT, LORA_POWER);
		#else
		LL_GPIO_ResetOutputPin(LORA_POWER_PORT, LORA_POWER);
		#endif
	}
	else
	{
		#ifdef LORA_POWER_INV
		LL_GPIO_SetOutputPin(LORA_POWER_PORT, LORA_POWER);
		#else
		LL_GPIO_ResetOutputPin(LORA_POWER_PORT, LORA_POWER);
		#endif
	}
}


#endif







