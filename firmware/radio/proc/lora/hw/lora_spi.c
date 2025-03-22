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

uint32_t timeout = 0;

/* SPI Init Structure */
LL_SPI_InitTypeDef   SPI_InitStruct;

/* GPIO Init Structure */
LL_GPIO_InitTypeDef  GPIO_InitStruct;

uint8_t    SPIx_TxBuffer[] = "**** SPI_OneBoard_IT communication **** SPI_OneBoard_IT communication **** SPI_OneBoard_IT communication ****";
uint32_t   SPIx_NbDataToTransmit = ((sizeof(SPIx_TxBuffer)/ sizeof(*SPIx_TxBuffer)) - 1);

uint8_t    SPI1_RxBuffer[sizeof(SPIx_TxBuffer)];
uint32_t   SPI1_ReceiveIndex = 0;
uint32_t   SPI1_TransmitIndex = 0;

void lora_spi_rx_callback(void)
{
	SPI1_RxBuffer[SPI1_ReceiveIndex++] = LL_SPI_ReceiveData8(SPI1);
}

void lora_spi_tx_callback(void)
{
	LL_SPI_TransmitData8(SPI1, SPIx_TxBuffer[SPI1_TransmitIndex++]);
}

void lora_spi_eot_callback(void)
{
	LL_SPI_Disable(SPI1);
	LL_SPI_DisableIT_TXP(SPI1);
	LL_SPI_DisableIT_RXP(SPI1);
	LL_SPI_DisableIT_CRCERR(SPI1);
	LL_SPI_DisableIT_OVR(SPI1);
	LL_SPI_DisableIT_UDR(SPI1);
	LL_SPI_DisableIT_EOT(SPI1);
}

void lora_spi_err_callback(void)
{
	LL_SPI_DisableIT_TXP(SPI1);
	LL_SPI_DisableIT_RXP(SPI1);
	LL_SPI_DisableIT_CRCERR(SPI1);
	LL_SPI_DisableIT_OVR(SPI1);
	LL_SPI_DisableIT_UDR(SPI1);
	LL_SPI_DisableIT_EOT(SPI1);

	LL_SPI_Disable(SPI1);
}

static void lora_misc_gpio_config(void)
{
	// ToDo: Set initial state
	// ...

	GPIO_InitStruct.Mode      = LL_GPIO_MODE_OUTPUT;
	GPIO_InitStruct.Pull      = LL_GPIO_PULL_NO;
	GPIO_InitStruct.Speed     = LL_GPIO_SPEED_LOW;

	GPIO_InitStruct.Pin       = RFM_RST;
	LL_GPIO_Init(RFM_RST_PORT, &GPIO_InitStruct);

	GPIO_InitStruct.Pin       = RFM_NSS;
	LL_GPIO_Init(RFM_NSS_PORT, &GPIO_InitStruct);

	GPIO_InitStruct.Pin       = RFM_DIO1;
	LL_GPIO_Init(RFM_DIO1_PORT, &GPIO_InitStruct);

	GPIO_InitStruct.Pin       = RFM_DIO0;
	LL_GPIO_Init(RFM_DIO0_PORT, &GPIO_InitStruct);

	GPIO_InitStruct.Pin       = LORA_POWER;
	LL_GPIO_Init(LORA_POWER_PORT, &GPIO_InitStruct);
}

static void lora_spi_gpio_config(void)
{
	GPIO_InitStruct.Mode      = LL_GPIO_MODE_ALTERNATE;
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

	NVIC_SetPriority(SPI1_IRQn, 3);
	NVIC_EnableIRQ(SPI1_IRQn);
}

static void lora_spi_config (void)
{
  /* Configure SPI MASTER ****************************************************/
  /* Enable SPI1 Clock */
  LL_APB2_GRP1_EnableClock(LL_APB2_GRP1_PERIPH_SPI1);

  /* Configure the SPI1 parameters */
  SPI_InitStruct.BaudRate          = LL_SPI_BAUDRATEPRESCALER_DIV256;
  SPI_InitStruct.TransferDirection = LL_SPI_FULL_DUPLEX;
  SPI_InitStruct.ClockPhase        = LL_SPI_PHASE_2EDGE;
  SPI_InitStruct.ClockPolarity     = LL_SPI_POLARITY_HIGH;
  SPI_InitStruct.BitOrder          = LL_SPI_MSB_FIRST;
  SPI_InitStruct.DataWidth         = LL_SPI_DATAWIDTH_8BIT;
  SPI_InitStruct.NSS               = LL_SPI_NSS_SOFT;
  SPI_InitStruct.CRCCalculation    = LL_SPI_CRCCALCULATION_DISABLE;
  SPI_InitStruct.Mode              = LL_SPI_MODE_MASTER;

  LL_SPI_Init(SPI1, &SPI_InitStruct);

  /* Lock GPIO for master to avoid glitches on the clock output */
  LL_SPI_EnableGPIOControl(SPI1);
  LL_SPI_EnableMasterRxAutoSuspend(SPI1);

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
}

void lora_spi_go(void)
{
	timeout = 0xFF;

	lora_misc_gpio_config();
	lora_spi_gpio_config();
	lora_spi_config ();

	/* 1 - Start Master Transfer(SPI1) ******************************************/
	  LL_SPI_StartMasterTransfer(SPI1);

	  /* 2 - Wait end of transfer *************************************************/
	  while ((timeout != 0))
	  {
	    if (SysTick->CTRL & SysTick_CTRL_COUNTFLAG_Msk)
	    {
	      timeout--;
	    }
	  }
}

#endif







