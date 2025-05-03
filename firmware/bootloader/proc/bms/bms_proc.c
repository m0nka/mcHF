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
#include "mchf_pro_board.h"
#include "main.h"

#include "shared_i2c.h"
#include "bq40z80.h"

#include "bms_proc.h"

ushort 	batt_status = 0;
uchar 	soc 		= 0;
uchar	charge_mode = 0;

void bms_proc_periodic(void)
{
	//HAL_GPIO_TogglePin(BMS_PWM_PORT, BMS_PWM_PIN);
}

void bms_proc_hw_init(void)
{
	GPIO_InitTypeDef  gpio_init_structure;

	gpio_init_structure.Pull  = GPIO_NOPULL;

	// BT Power Control
	gpio_init_structure.Pin   = BMS_PWM_PIN;
	gpio_init_structure.Mode  = GPIO_MODE_OUTPUT_PP;
	HAL_GPIO_Init(BMS_PWM_PORT, &gpio_init_structure);

	// Power off
	HAL_GPIO_WritePin(BMS_PWM_PORT, BMS_PWM_PIN, GPIO_PIN_RESET);
}

void bms_proc_is_charging(void)
{
	// Need at least the init bit on
	if((batt_status == 0xFFFF)||(batt_status == 0))
		return;

	if((batt_status & 0x40) != 0x40)
		charge_mode = 1;
	else
		charge_mode = 0;
}

void bms_proc(void)
{
	static uchar soc_skip = 0;

	// Update local status
	batt_status = bq40z80_read_status();

	// Update charge status
	bms_proc_is_charging();

	//if(charge_mode)
	//bq40z80_read_current();

	// Update SOC
	if(soc_skip++ > 20)
	{
		soc = bq40z80_read_soc();
		//if(soc != 0xFF)
		//	printf("soc: %d%% \r\n", soc);

		soc_skip = 0;
	}
}

void bms_proc_init(void)
{
	// Charge pins
	bms_proc_hw_init();

	// I2C init
	bq40z80_init();

	// Status on start
	batt_status = bq40z80_read_status();

	// SOC on start
	soc = bq40z80_read_soc();

	// Update charge status
	bms_proc_is_charging();

	//if(batt_status != 0xFFFF)
	//	printf("sta: %04x \r\n", batt_status);
}
