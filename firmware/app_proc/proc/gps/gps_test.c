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
#include "mchf_pro_board.h"
#include "main.h"

#include "gps_proc.h"
#include "gps_test.h"

#if defined (CONTEXT_GPS) && defined (GPS_TEST_GPIO)

void gps_test_init(void)
{
	LL_GPIO_SetPinMode(GPS_RX_PORT, 	GPS_RX_PIN, 	LL_GPIO_MODE_OUTPUT);
	LL_GPIO_SetPinMode(GPS_TX_PORT, 	GPS_TX_PIN, 	LL_GPIO_MODE_OUTPUT);
	LL_GPIO_SetPinMode(GPS_PPS_PORT, 	GPS_PPS_PIN,	LL_GPIO_MODE_OUTPUT);
	LL_GPIO_SetPinMode(GPS_EN_PORT, 	GPS_EN, 		LL_GPIO_MODE_OUTPUT);

	// Power on
	LL_GPIO_SetOutputPin(GPS_EN_PORT, GPS_EN);

	vTaskDelay(5000);

	// Power off
	LL_GPIO_ResetOutputPin(GPS_EN_PORT, GPS_EN);
}

void gps_test_run(void)
{
	LL_GPIO_TogglePin(GPS_RX_PORT, GPS_RX_PIN);
	vTaskDelay(50);
	LL_GPIO_TogglePin(GPS_TX_PORT, GPS_TX_PIN);
	vTaskDelay(50);
	LL_GPIO_TogglePin(GPS_PPS_PORT, GPS_PPS_PIN);
	vTaskDelay(50);
}


#endif
