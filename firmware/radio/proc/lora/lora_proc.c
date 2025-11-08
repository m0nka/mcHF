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

#ifdef MESHCORE_REPEATER
#include "repeater_main.h"
#endif

#include "lora_proc.h"

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
	//ulong 	ulNotificationValue = 0, ulNotif;

	// Delay start, so UI can paint properly
	vTaskDelay(LORA_PROC_START_DELAY);

	//printf("lora process start\r\n");
	//lora_spi_init();

	#ifdef MESHCORE_REPEATER
	setup();
	#endif

lora_proc_loop:

	#ifdef MESHCORE_REPEATER
	loop();
	#endif

	// Wait key press
	//ulNotif = xTaskNotifyWait(0x00, ULONG_MAX, &ulNotificationValue, LORA_PROC_SLEEP_TIME);
	//if((ulNotif) && (ulNotificationValue))
	//{
		// ..
	//}

	#ifdef SPI_GPIO_TEST
	// Toggle misc pins
	LL_GPIO_TogglePin(RFM_NSS_PORT, RFM_NSS);
	LL_GPIO_TogglePin(RFM_DIO1_PORT, RFM_DIO1);
	LL_GPIO_TogglePin(RFM_DIO0_PORT, RFM_DIO0);
	//
	// Toggle spi pins
	LL_GPIO_TogglePin(RFM_MISO_SPI1_PORT, RFM_MISO_SPI1);
	LL_GPIO_TogglePin(RFM_MOSI_SPI1_PORT, RFM_MOSI_SPI1);
	LL_GPIO_TogglePin(RFM_SCK_SPI1_PORT, RFM_SCK_SPI1);
	#endif

	vTaskDelay(20);

	goto lora_proc_loop;
}

void lora_proc_init(void)
{
	lora_gpio_init();

	//--printf("lora pre-os init\r\n");
}

void lora_proc_power_cleanup(void)
{

}

#endif
