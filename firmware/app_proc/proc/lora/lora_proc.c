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
#include "sx126x_commands.h"
#include "lora_spi.h"

#include "lora_proc.h"

sx126x_handle_t radio_drv;
uchar			radio_init_done = 0;

void lora_proc_busy_irq(void)
{
	printf("busy\r\n");
	sx1262_busy_handler((void *)&radio_drv);
}

void lora_proc_dio1_irq(void)
{
	printf("dio1\r\n");
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

uchar lora_proc_find_chip(void)
{
	char version[100];
	int i;

	// Retry loop
	for(i = 0; i < 10; i++)
	{
		// HW reset
		sx1262_reset(NULL);

		// Set standby mode
		if(sx126x_set_op_mode_standby(&radio_drv, 0) != 0)
			continue;

		// Read ID string
		if(sx126x_read_version_string(&radio_drv, version, sizeof(version)) != 0)
			continue;

		// Debug only
		#if 1
		if(version[0] > 0x80)
			print_hex_array((uchar *)version, 6);
		else
			printf("ver: %s \r\n", version);
		#endif

		// Detected
		if(strncmp(version,"SX1261", 6) == 0)
			return 0;

		// Retry delay
		vTaskDelay(10);
	}

	return 1;
}

uchar lora_proc_modem_setup(void)
{
	// Setup TCXO
	if(sx126x_set_dio3_as_txco_ctrl(&radio_drv, 1.6f, 5000.0f) != 0)
		return 1;

	// Set modem type
	if(sx126x_set_packet_type(&radio_drv, RADIOLIB_SX126X_PACKET_TYPE_LORA) != 0)
		return 2;

	// Set initial CAD parameters
	if(sx126x_set_cad_params(&radio_drv,
							RADIOLIB_SX126X_CAD_ON_8_SYMB,
							(11 + 13),							// spreading factor
							RADIOLIB_SX126X_CAD_PARAM_DET_MIN,
							RADIOLIB_SX126X_CAD_GOTO_STDBY,
							0) != 0)
	{
		return 3;
	}

	// Clear IRQ
	if(sx126x_set_dio_irq_params(&radio_drv, RADIOLIB_SX126X_IRQ_NONE, RADIOLIB_SX126X_IRQ_NONE, 0, 0) != 0)
		return 4;

	// Calibrate All
	if(sx126x_calibrate(&radio_drv, true, true, true, true, true, true, true) != 0)
		return 5;

	vTaskDelay(5);

	// Check some status ??
	// ...

	return 0;
}

uchar lora_proc_radio_init(void)
{
	// Lora power on
	lora_spi_power_state(1);

	// Basic init
	if(sx126x_init(&radio_drv, 0, 0, 0, 0, 0))
		return 1;

	// Detect module
	if(lora_proc_find_chip())
		return 2;

	// Setup modem
	if(lora_proc_modem_setup() != 0)
		return 3;

	// Select LDO
	if(sx126x_set_regulator_mode(&radio_drv, 0) != 0)
		return 4;

	// ... next

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
	//
	// SPI HW init
	lora_spi_init();

	// Radio init
	int err = lora_proc_radio_init();
	if(err) printf("radio init err: %d \r\n", err);
	//
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
