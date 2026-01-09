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

#include "lora_proc.h"

sx126x_handle_t radio_drv;
uchar			radio_init_done = 0;

#ifdef CONTEXT_LORA__
//*----------------------------------------------------------------------------
//* Function Name       : SPI1_IRQHandler
//* Object              :
//* Notes    			: LORA SPI irq handler
//* Notes   			:
//* Notes    			:
//* Context    			: CONTEXT_IRQ
//*----------------------------------------------------------------------------
/*void SPI1_IRQHandler(void)
{
    if(LL_SPI_IsActiveFlag_OVR(SPI1) || LL_SPI_IsActiveFlag_UDR(SPI1))
    {
    	lora_spi_err_callback();
    }

    if(LL_SPI_IsActiveFlag_RXP(SPI1) && LL_SPI_IsEnabledIT_RXP(SPI1))
    {
    	lora_spi_rx_callback();
    	return;
    }

    if((LL_SPI_IsActiveFlag_TXP(SPI1) && LL_SPI_IsEnabledIT_TXP(SPI1)))
    {
    	lora_spi_tx_callback();
    	return;
    }

    if(LL_SPI_IsActiveFlag_EOT(SPI1) && LL_SPI_IsEnabledIT_EOT(SPI1))
    {
    	lora_spi_eot_callback();
    	return;
    }
}*/
extern SPI_HandleTypeDef SpiHandle1;
extern DMA_HandleTypeDef hdma_tx;
extern DMA_HandleTypeDef hdma_rx;
void SPI1_IRQHandler(void)
{
  HAL_SPI_IRQHandler(&SpiHandle1);
}

void SPI1_DMA_RX_IRQHandler(void)
{
  HAL_DMA_IRQHandler(SpiHandle1.hdmarx);
}

void SPI1_DMA_TX_IRQHandler(void)
{
  HAL_DMA_IRQHandler(SpiHandle1.hdmatx);
}
#endif

#ifdef SPI_GPIO_TEST
void lora_proc_gpio_test(void)
{
	// Toggle misc pins
	LL_GPIO_TogglePin(RFM_NSS_PORT, RFM_NSS);
	LL_GPIO_TogglePin(RFM_DIO0_PORT, RFM_DIO0);
	//
	// Toggle spi pins
	LL_GPIO_TogglePin(RFM_MISO_SPI1_PORT, RFM_MISO_SPI1);
	LL_GPIO_TogglePin(RFM_MOSI_SPI1_PORT, RFM_MOSI_SPI1);
	LL_GPIO_TogglePin(RFM_SCK_SPI1_PORT, RFM_SCK_SPI1);
}
#endif

uchar lora_proc_radio_init(void)
{
	char version[100];

	// Basic init
	if(sx126x_init(&radio_drv,0,0,0,0,0))
		return 1;

	// Read ID string
	if(sx126x_read_version_string(&radio_drv, version, sizeof(version)) != 0)
		return 2;

	printf("ver: %s \r\n", version);

	// Enable driver
	radio_init_done = 1;

	return 0;
}

//*----------------------------------------------------------------------------
//* Function Name       : lora_proc_task
//* Object              :
//* Notes    			:
//* Notes   			:
//* Notes    			:
//* Context    			: CONTEXT_LORA
//*----------------------------------------------------------------------------
void lora_proc_task(void const * argument)
{
	// Delay start, so UI can paint properly
	vTaskDelay(LORA_PROC_START_DELAY);
	printf("start\r\n");

	// Radio driver init
	#ifndef SPI_GPIO_TEST
	lora_proc_radio_init();
	#endif

lora_proc_loop:

	#ifdef SPI_GPIO_TEST
	lora_proc_gpio_test();
	#endif

	vTaskDelay(20);

	goto lora_proc_loop;
}

void lora_proc_init(void)
{
	// Basic GPIO init before OS is run, keep here!
	lora_gpio_init();

	//--printf("lora pre-os init\r\n");
}

void lora_proc_power_cleanup(void)
{
	// Lora power off
	lora_spi_power_state(0);

	// ToDo: all pins inputs ?
	//
}

#endif
