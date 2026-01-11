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

int32_t lora_spi_set_exti_irq(void)
{
	GPIO_InitTypeDef gpio_init_structure;

	gpio_init_structure.Pin 	= LORA_BUSY;
	gpio_init_structure.Pull 	= GPIO_PULLDOWN;
	gpio_init_structure.Speed 	= GPIO_SPEED_FREQ_LOW;
	gpio_init_structure.Mode 	= GPIO_MODE_IT_RISING_FALLING;
	HAL_GPIO_Init(LORA_BUSY_PORT, &gpio_init_structure);

	gpio_init_structure.Pin 	= LORA_DIO1;
	gpio_init_structure.Pull 	= GPIO_PULLUP;
	gpio_init_structure.Speed 	= GPIO_SPEED_FREQ_LOW;
	gpio_init_structure.Mode 	= GPIO_MODE_IT_RISING;
	HAL_GPIO_Init(LORA_DIO1_PORT, &gpio_init_structure);

	//HAL_NVIC_SetPriority(EXTI4_IRQn, 15U, 0x00);
	//HAL_NVIC_EnableIRQ  (EXTI4_IRQn);
	//HAL_NVIC_SetPriority(EXTI9_5_IRQn, 15U, 0x00);
	//HAL_NVIC_EnableIRQ  (EXTI9_5_IRQn);

	return BSP_ERROR_NONE;
}

static void lora_spi_misc_gpio_config(void)
{
	LL_GPIO_InitTypeDef  GPIO_InitStruct;

	// Lora power off
	lora_spi_power_state(0);

	// Initial state
	LL_GPIO_SetOutputPin(LORA_NSS_PORT,   LORA_NSS);	// de-selected
	LL_GPIO_SetOutputPin(LORA_RESET_PORT, LORA_RESET);	// in reset

	// Common
	GPIO_InitStruct.Pull      = LL_GPIO_PULL_NO;
	GPIO_InitStruct.Speed     = LL_GPIO_SPEED_LOW;

	// ----------------------------------------------
	// Outputs
	GPIO_InitStruct.Mode      = LL_GPIO_MODE_OUTPUT;

	// POWER, PA2
	GPIO_InitStruct.Pin       = LORA_POWER;
	LL_GPIO_Init(LORA_POWER_PORT, &GPIO_InitStruct);

	// Chip select, PC1
	GPIO_InitStruct.Pin       = LORA_NSS;
	LL_GPIO_Init(LORA_NSS_PORT, &GPIO_InitStruct);

	// GPIO0, PA3 (NRST)
	GPIO_InitStruct.Pin       = LORA_RESET;
	LL_GPIO_Init(LORA_RESET_PORT, &GPIO_InitStruct);

	#if 0
	// ----------------------------------------------
	// Inputs/EXTI
	GPIO_InitStruct.Mode      = LL_GPIO_MODE_INPUT;

	// DIO1, PC4 (IRQ)
	GPIO_InitStruct.Pin       = LORA_DIO1;
	GPIO_InitStruct.Pull      = LL_GPIO_PULL_UP;
	LL_GPIO_Init(LORA_DIO1_PORT, &GPIO_InitStruct);

	// Busy(PC5)
	GPIO_InitStruct.Pin       = LORA_BUSY;
	GPIO_InitStruct.Pull      = LL_GPIO_PULL_DOWN;
	LL_GPIO_Init(LORA_BUSY_PORT, &GPIO_InitStruct);

	LL_SYSCFG_SetEXTISource(LL_SYSCFG_EXTI_PORTC, LL_SYSCFG_EXTI_LINE4);
	LL_SYSCFG_SetEXTISource(LL_SYSCFG_EXTI_PORTC, LL_SYSCFG_EXTI_LINE5);

	LL_EXTI_EnableIT_0_31(LL_SYSCFG_EXTI_LINE4);
	LL_EXTI_EnableIT_0_31(LL_SYSCFG_EXTI_LINE5);

	LL_EXTI_EnableRisingTrig_0_31(LL_SYSCFG_EXTI_LINE4);
	LL_EXTI_EnableFallingTrig_0_31(LL_SYSCFG_EXTI_LINE5);
	LL_EXTI_EnableRisingTrig_0_31(LL_SYSCFG_EXTI_LINE5);

	//NVIC_SetPriority(EXTI4_IRQn, 15U);
	//NVIC_EnableIRQ  (EXTI4_IRQn);
	//NVIC_SetPriority(EXTI9_5_IRQn, 15U);
	//NVIC_EnableIRQ  (EXTI9_5_IRQn);
	#else
	lora_spi_set_exti_irq();
	#endif

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

	GPIO_InitStruct.Pin       = LORA_MISO_SPI1;
	LL_GPIO_Init(LORA_MISO_SPI1_PORT, &GPIO_InitStruct);

	GPIO_InitStruct.Pin       = LORA_MOSI_SPI1;
	LL_GPIO_Init(LORA_MOSI_SPI1_PORT, &GPIO_InitStruct);

	GPIO_InitStruct.Pin       = LORA_SCK_SPI1;
	LL_GPIO_Init(LORA_SCK_SPI1_PORT, &GPIO_InitStruct);

	//printf("lora_spi_gpio_config\r\n");
}

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

	//printf("total: %d \r\n", size);

	// CS low
	LL_GPIO_ResetOutputPin(LORA_NSS_PORT, LORA_NSS);

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

    	if(rx_buffer)
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

    // CS high
	LL_GPIO_SetOutputPin(LORA_NSS_PORT, LORA_NSS);

	return ret;
}

int spi_device_transmit(int device, spi_transaction_t *t)
{
	uchar tx_buff[10], rx_buff[200], shift = 0;
	ulong out_len = t->length;
	int  ret = 0;

	//--printf("spi transfer \r\n");

	if(t == NULL)
		return 100;

	switch(t->cmd)
	{
		case SX126X_CMD_READ_REGISTER:
		{
			tx_buff[0] = t->cmd;
			out_len += 8;
			shift++;

			if((t->flags & SPI_TRANS_VARIABLE_ADDR) == SPI_TRANS_VARIABLE_ADDR)
			{
				//--printf("add address \r\n");
				tx_buff[1] = t->addr >> 8;
				tx_buff[2] = t->addr & 0xFF;
				out_len += 16;
				shift += 2;
			}

			if((t->flags & SPI_TRANS_VARIABLE_DUMMY) == SPI_TRANS_VARIABLE_DUMMY)
			{
				//--printf("add dummy \r\n");
				tx_buff[3] = 0;
				out_len += 8;
				shift++;
			}

			ret = spi_transfer(tx_buff, rx_buff, out_len);

			if(ret)
				printf("spi res: %d \r\n", ret);
			else
			{
				// To bytes
				out_len /= 8;

				// Copy without TX data
				memcpy((uchar *)(t->rx_buffer), (uchar *)&rx_buff[shift], (out_len - shift));
			}
			break;
		}

		case SX126X_CMD_WRITE_REGISTER:
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

			break;
		}

		case SX126X_CMD_SET_STANDBY:
		case SX126X_CMD_SET_DIO3_AS_TXCO_CTRL:
		case SX126X_CMD_SET_PACKET_TYPE:
		case SX126X_CMD_SET_CAD_PARAMS:
		case SX126X_CMD_SET_DIO_IRQ_PARAMS:
		case SX126X_CMD_CALIBRATE:
		case SX126X_CMD_SET_REGULATOR_MODE:
		case SX126X_CMD_SET_MODULATION_PARAMS:
		case SX126X_CMD_SET_PACKET_PARAMS:
		case SX126X_CMD_SET_RF_FREQUENCY:
		case SX126X_CMD_SET_RX:
		{
			ret = spi_transfer(t->tx_data, t->rx_data, out_len);
			break;
		}

		default:
			printf("not supported cmd: 0x%x \r\n", t->cmd);
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







