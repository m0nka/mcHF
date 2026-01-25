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
#include "lora_radio.h"

#include "lora_proc.h"

sx126x_handle_t radio_drv;
uchar			radio_init_done = 0;

void lora_proc_busy_irq(void)
{
	//printf("busy\r\n");
	sx1262_busy_handler((void *)&radio_drv);
}

void lora_proc_dio1_irq(void)
{
	//printf("dio1\r\n");
	sx1262_dio1_handler((void *)&radio_drv);
}

#ifdef SPI_GPIO_TEST
void lora_proc_gpio_test(void)
{
	// Toggle misc pins
	LL_GPIO_TogglePin(LORA_NSS_PORT, LORA_NSS);
	LL_GPIO_TogglePin(LORA_RESET_PORT, LORA_RESET);
	//
	// Toggle spi pins
	LL_GPIO_TogglePin(LORA_MISO_SPI1_PORT, LORA_MISO_SPI1);
	LL_GPIO_TogglePin(LORA_MOSI_SPI1_PORT, LORA_MOSI_SPI1);
	LL_GPIO_TogglePin(LORA_SCK_SPI1_PORT, LORA_SCK_SPI1);
}
#endif

void lora_proc_modem_init(void)
{
	// EXTI IRQs on
	lora_spi_activate_exti_irq();

	// SPI HW init
	lora_spi_init();

	// Radio init
	int err = lora_radio_init();
	if(err)
	{
		printf("radio init err: %d \r\n", err);

		// Lora power off
		lora_spi_power_state(0);

		// Unload
		vTaskSuspend(NULL);
	}

	#ifdef MESHCORE
	printf("modem on(MC)\r\n");
	#else
	printf("modem on(MT)\r\n");
	#endif

	// Enable driver
	radio_init_done = 1;
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
	lora_proc_modem_init();
	#endif

	// Tx on start
	#if 0
	if(radio_init_done)
		lora_radio_schedule_tx();
	#endif

lora_proc_loop:

	#ifdef SPI_GPIO_TEST
	lora_proc_gpio_test();
	#else
	if(radio_init_done)
		lora_radio_rx_check();
	#endif

	vTaskDelay(5);
	goto lora_proc_loop;
}

void lora_proc_init(void)
{
	// Basic GPIO init before OS is run, keep here!
	lora_gpio_init();

	//printf("lora pre-os init\r\n");
}

void lora_proc_power_cleanup(void)
{
	// Lora power off
	lora_spi_power_state(0);

	// ToDo: all pins inputs ?
	//
}

#endif
