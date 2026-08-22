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

#ifdef MESHCORE
#include "mc_client.h"
#endif

#include "lora_proc.h"

sx126x_handle_t radio_drv;
uchar			radio_init_done = 0;

// FreeRTOS process state
extern struct PROC_STATE 				ps;

// UI driver public state
extern struct	UI_DRIVER_STATE			ui_s;

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

//*----------------------------------------------------------------------------
//* Function Name       : lora_proc_modem_init
//* Object              :
//* Notes    			:
//* Notes   			:
//* Notes    			:
//* Context    			: CONTEXT_LORA
//*----------------------------------------------------------------------------
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
//* Function Name       : file_b_send_msg
//* Object              : Send message to queue
//* Input Parameters    : none
//* Output Parameters   : none
//* Functions called    : none
//*----------------------------------------------------------------------------
static uchar lora_proc_send_msg(xQueueHandle pvQueueHandle, ulong *ulMessageBuffer, uchar ucNumberOfItems)
{
	ulong ulDummy;
	uchar ucCount;

	if(ui_s.cur_state != MODE_DESKTOP)
		return 55;

	/* Clear Rx Queue before posting */
	while( uxQueueMessagesWaiting(pvQueueHandle))
	{
		xQueueReceive(pvQueueHandle, (void *)&ulDummy, (portTickType)0);
	}

	/* Send all items */
	for(ucCount = 0;ucCount < ucNumberOfItems;ucCount++)
	{
    	ulDummy = *ulMessageBuffer++;

	    /* Insert the item */
		if(xQueueSend(pvQueueHandle, (void *)&ulDummy, (portTickType)0) != pdPASS )
			return 1;
	}

	return 0;
}

//*----------------------------------------------------------------------------
//* Function Name       : lora_proc_client_exec
//* Object              :
//* Notes    			:
//* Notes   			:
//* Notes    			:
//* Context    			: CONTEXT_LORA
//*----------------------------------------------------------------------------
static void lora_proc_client_exec(xQueueHandle *RxQueue)
{
	//uchar  msg[256];
	//ushort siz = 0;
	char   notif[300];	// enough size for description text added to message
	ulong  ulData[10];

	struct LORA_PACKET_RX lprx;

	if(!radio_init_done)
		return;

	lprx.avail = 0;

	// Wait RX packet (radio layer)
	lora_radio_rx_check(&lprx);

	// Process message(meshcore stack)
	if(lprx.raw_rx_size != 0)
	{
		// Unpack message
		client_decode(&lprx, lprx.raw_rx_msg, lprx.raw_rx_size, notif);
		//printf("text: %s(%d) \r\n", notif, strlen(notif));

		// Notify UI
		if(strlen(notif))
		{
			ulData[0] = 0x55;
			ulData[1] = (ulong)notif;
			ulData[2] = (ulong)&lprx;

			// Fill queue
			lora_proc_send_msg(*RxQueue, ulData, 3);

			// Notify UI
			if(ps.hUiTask != NULL)
				xTaskNotify(ps.hUiTask, UI_LORA_NOTIFICATION, eSetValueWithOverwrite);

			// Keep stack var valid until dumped by UI
			vTaskDelay(200);

			return;
		}
	}

	if((lprx.raw_rx_size != 0)||(lprx.avail))
	{
		ulData[0] = 0x67;
		ulData[1] = 0x00;
		ulData[2] = (ulong)&lprx;

		// Fill queue
		lora_proc_send_msg(*RxQueue, ulData, 3);

		// Notify UI
		if(ps.hUiTask != NULL)
			xTaskNotify(ps.hUiTask, UI_LORA_NOTIFICATION, eSetValueWithOverwrite);

		// Keep stack var valid until dumped by UI
		vTaskDelay(200);
	}
}

//*----------------------------------------------------------------------------
//* Function Name       : lora_proc_task
//* Object              :
//* Notes    			:
//* Notes   			:
//* Notes    			:
//* Context    			: CONTEXT_LORA
//*----------------------------------------------------------------------------
void lora_proc_task(void const *arg)
{
	xQueueHandle	*RxQueue;

	// Delay start, so UI can paint properly
	vTaskDelay(LORA_PROC_START_DELAY);
	//printf("start\r\n");

	// Get rx queue ptr
	RxQueue = (xQueueHandle *)arg;

	// Radio driver init
	#ifndef SPI_GPIO_TEST
	lora_proc_modem_init();
	#endif

	#ifdef MESHCORE_UNIT_TEST
	client_unit_test();
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
	lora_proc_client_exec(RxQueue);
	#endif

	vTaskDelay(5);
	goto lora_proc_loop;
}

//*----------------------------------------------------------------------------
//* Function Name       : lora_proc_init
//* Object              :
//* Notes    			:
//* Notes   			:
//* Notes    			:
//* Context    			: CONTEXT_RESET
//*----------------------------------------------------------------------------
void lora_proc_init(void)
{
	// Basic GPIO init before OS is run, keep here!
	lora_gpio_init();

	//printf("lora pre-os init\r\n");
}

//*----------------------------------------------------------------------------
//* Function Name       : lora_proc_power_cleanup
//* Object              :
//* Notes    			:
//* Notes   			:
//* Notes    			:
//* Context    			: CONTEXT_RESET
//*----------------------------------------------------------------------------
void lora_proc_power_cleanup(void)
{
	// Lora power off
	lora_spi_power_state(0);

	// ToDo: all pins inputs ?
	//
}

#endif
