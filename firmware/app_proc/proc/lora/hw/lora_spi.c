/************************************************************************************
**                                                                                 **
**                                 mcHF QRP Transceiver                            **
**                         Krassi Atanassov - M0NKA, 2013-2026                     **
**                                                                                 **
**---------------------------------------------------------------------------------**
**                                                                                 **
**  File name:                                                                     **
**  Description:                                                                   **
**  Last Modified:                                                                 **
**  Licence:			https://github.com/m0nka/mcHF/blob/main/LICENSE            **
************************************************************************************/
#include "main.h"
#include "mchf_pro_board.h"

#ifdef CONTEXT_LORA

#include "sx126x.h"
#include "lora_spi.h"

// FreeRTOS process state
extern struct PROC_STATE 			ps;

#if 0
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
#endif

static void lora_spi_misc_gpio_config(void)
{
	LL_GPIO_InitTypeDef  GPIO_InitStruct;

	GPIO_InitStruct.Mode      = LL_GPIO_MODE_OUTPUT;
	GPIO_InitStruct.Pull      = LL_GPIO_PULL_NO;
	GPIO_InitStruct.Speed     = LL_GPIO_SPEED_LOW;

	// Lora power off
	lora_spi_power_state(0);

	// Initial state
	LL_GPIO_SetOutputPin(RFM_NSS_PORT,  RFM_NSS);	// de-selected
	LL_GPIO_SetOutputPin(RFM_DIO0_PORT, RFM_DIO0);	// in reset

	// Chip select, PC1
	GPIO_InitStruct.Pin       = RFM_NSS;
	LL_GPIO_Init(RFM_NSS_PORT, &GPIO_InitStruct);

	// GPIO0, PC5 (NRST)
	GPIO_InitStruct.Pin       = RFM_DIO0;
	LL_GPIO_Init(RFM_DIO0_PORT, &GPIO_InitStruct);

	// POWER, PA2
	GPIO_InitStruct.Pin       = LORA_POWER;
	LL_GPIO_Init(LORA_POWER_PORT, &GPIO_InitStruct);

	// Busy(PA0) is input
	GPIO_InitStruct.Mode      = LL_GPIO_MODE_INPUT;
	GPIO_InitStruct.Pin       = RFM_BUSY;
	LL_GPIO_Init(RFM_BUSY_PORT, &GPIO_InitStruct);

	// GPIO1, PC4 (IRQ) input
	GPIO_InitStruct.Pin       = RFM_DIO1;
	LL_GPIO_Init(RFM_DIO1_PORT, &GPIO_InitStruct);

	// GPIO2, PC4, NC, so input
	GPIO_InitStruct.Pin       = RFM_DIO2;
	LL_GPIO_Init(RFM_DIO2_PORT, &GPIO_InitStruct);

	//printf("lora_spi_misc_gpio_config\r\n");
}

static void lora_spi_gpio_config(void)
{
	LL_GPIO_InitTypeDef  GPIO_InitStruct;

	#ifndef SPI_GPIO_TEST
	GPIO_InitStruct.Mode      = LL_GPIO_MODE_ALTERNATE;
	GPIO_InitStruct.Pull      = LL_GPIO_PULL_DOWN;
	GPIO_InitStruct.Speed     = LL_GPIO_SPEED_HIGH;
	GPIO_InitStruct.Alternate = LL_GPIO_AF_5;
	#else
	GPIO_InitStruct.Mode      = LL_GPIO_MODE_OUTPUT;
	#endif

	GPIO_InitStruct.Pin       = RFM_MISO_SPI1;
	LL_GPIO_Init(RFM_MISO_SPI1_PORT, &GPIO_InitStruct);

	GPIO_InitStruct.Pin       = RFM_MOSI_SPI1;
	LL_GPIO_Init(RFM_MOSI_SPI1_PORT, &GPIO_InitStruct);

	GPIO_InitStruct.Pin       = RFM_SCK_SPI1;
	LL_GPIO_Init(RFM_SCK_SPI1_PORT, &GPIO_InitStruct);

	//printf("lora_spi_gpio_config\r\n");
}

#if 0
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
#endif

void lora_spi_init(void)
{
	LL_SPI_InitTypeDef	SPI_InitStruct;

  /* Configure SPI MASTER ****************************************************/
  /* Enable SPI1 Clock */
  LL_APB2_GRP1_EnableClock(LL_APB2_GRP1_PERIPH_SPI1);

  /* Configure the SPI1 parameters */
  SPI_InitStruct.BaudRate          = LL_SPI_BAUDRATEPRESCALER_DIV32;	// ~ 7.5Mhz
  SPI_InitStruct.TransferDirection = LL_SPI_FULL_DUPLEX;
  SPI_InitStruct.ClockPhase        = LL_SPI_PHASE_1EDGE;
  SPI_InitStruct.ClockPolarity     = LL_SPI_POLARITY_LOW;
  SPI_InitStruct.BitOrder          = LL_SPI_MSB_FIRST;
  SPI_InitStruct.DataWidth         = LL_SPI_DATAWIDTH_8BIT;
  SPI_InitStruct.NSS               = LL_SPI_NSS_SOFT;
  SPI_InitStruct.CRCCalculation    = LL_SPI_CRCCALCULATION_DISABLE;
  SPI_InitStruct.Mode              = LL_SPI_MODE_MASTER;

  LL_SPI_Init(SPI1, &SPI_InitStruct);

  /* Lock GPIO for master to avoid glitches on the clock output */
  LL_SPI_EnableGPIOControl(SPI1);
  LL_SPI_EnableMasterRxAutoSuspend(SPI1);

#if 0
  /* Set number of date to transmit */
  LL_SPI_SetTransferSize(SPI1, SPIx_NbDataToTransmit);

  /* Enable SPI1 */
  LL_SPI_Enable(SPI1);

  /* Enable TXP Interrupt */
  LL_SPI_EnableIT_TXP(SPI1);

  /* Enable RXP Interrupt */
  LL_SPI_EnableIT_RXP(SPI1);

  /* Enable SPI Errors Interrupt */
  LL_SPI_EnableIT_CRCERR(SPI1);
  LL_SPI_EnableIT_UDR(SPI1);
  LL_SPI_EnableIT_OVR(SPI1);
  LL_SPI_EnableIT_EOT(SPI1);
#endif
}

static uchar spi_transfer(const uchar *tx_buffer, uchar *rx_buffer, uchar len)
{
	uchar 		ret = 0;
	uint32_t 	tickstart, size = len/8;
	uchar 		*tx_buf = (uchar *)tx_buffer;

	if(len == 0)
		return 1;

	//--printf("total: %d \r\n", size);

	LL_GPIO_ResetOutputPin(RFM_NSS_PORT,  RFM_NSS);

    // Start transfer
    LL_SPI_SetTransferSize(SPI1, size);
    LL_SPI_Enable(SPI1);
    LL_SPI_StartMasterTransfer(SPI1);

    while(size--)
    {
    	tickstart = ps.epoch;
    	while(!LL_SPI_IsActiveFlag_TXP(SPI1))
    	{
    		if((ps.epoch - tickstart) >= SPI_TRANSFER_TIMEOUT)
    		{
    			//--printf("spi tx err at: %d \r\n", (int)size);
    			ret = 2;
    			goto spi_abort;
    		}
    	}

    	LL_SPI_TransmitData8(SPI1, tx_buf ? *tx_buf++ : 0XFF);

    	tickstart = ps.epoch;
    	while(!LL_SPI_IsActiveFlag_RXP(SPI1))
    	{
    		if((ps.epoch - tickstart) >= SPI_TRANSFER_TIMEOUT)
    		{
    			//--printf("spi rx err at: %d \r\n", (int)size);
    			ret = 3;
    			goto spi_abort;
    		}
    	}

    	if (rx_buffer)
    	{
    		*rx_buffer++ = LL_SPI_ReceiveData8(SPI1);
    	}
    	else
    	{
    		LL_SPI_ReceiveData8(SPI1);
    	}

    	//if ((SPI_TRANSFER_TIMEOUT != HAL_MAX_DELAY) && (ps.epoch - tickstart >= SPI_TRANSFER_TIMEOUT))
    	//{
    	//	ret = 1;
    	//	break;
    	//}
    }

spi_abort:
    // Add a delay before disabling SPI otherwise last-bit/last-clock may be truncated
    // See https://github.com/stm32duino/Arduino_Core_STM32/issues/1294
    // Computed delay is half SPI clock
    //delayMicroseconds(1000);

    /* Close transfer */
    /* Clear flags */
    LL_SPI_ClearFlag_EOT(SPI1);
    LL_SPI_ClearFlag_TXTF(SPI1);

    /* Disable SPI peripheral */
    LL_SPI_Disable(SPI1);

	LL_GPIO_SetOutputPin(RFM_NSS_PORT,  RFM_NSS);

	return ret;
}

int spi_device_transmit(int device, spi_transaction_t *t)
{
	uchar tx_buff[10], rx_buff[200], shift = 0;
	ulong out_len = t->length;
	int  ret = 0;

	//printf("spi transfer \r\n");

	if(t->cmd == SX126X_CMD_READ_REGISTER)
	{
		tx_buff[0] = t->cmd;
		out_len += 8;
		shift++;

		if((t->flags & SPI_TRANS_VARIABLE_ADDR) == SPI_TRANS_VARIABLE_ADDR)
		{
			//printf("add address \r\n");

			tx_buff[1] = t->addr >> 8;
			tx_buff[2] = t->addr & 0xFF;
			out_len += 16;
			shift += 2;
		}

		if((t->flags & SPI_TRANS_VARIABLE_DUMMY) == SPI_TRANS_VARIABLE_DUMMY)
		{
			//printf("add dummy \r\n");

			tx_buff[3] = 0;
			out_len += 8;
			shift++;
		}

		ret = spi_transfer(tx_buff, rx_buff, out_len);

		// Copy without TX data
		memcpy(t->rx_buffer, (rx_buff + shift), (out_len - shift));

		if(ret)
			printf("spi res: %d \r\n", ret);
	}
	else
	{
		printf("not supported \r\n");
		return 10;
	}

	return ret;
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







