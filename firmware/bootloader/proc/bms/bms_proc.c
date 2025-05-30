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
short	pack_curr	= 0;
ushort  pack_volt	= 0;

extern ulong  sys_timer;

//*----------------------------------------------------------------------------
//* Function Name       :
//* Object              :
//* Notes    			:
//* Notes   			:
//* Notes    			:
//* Context    			: CONTEXT_RESET
//*----------------------------------------------------------------------------
void bms_proc_periodic(void)
{
	// 50% duty, 1kHz
	if(charge_mode)
	{
		//HAL_GPIO_TogglePin(BMS_PWM_PORT, BMS_PWM_PIN);
	}

	// Reduce brightness
	//HAL_GPIO_TogglePin(LCD_BL_CTRL_GPIO_PORT, LCD_BL_CTRL_PIN);
}

//*----------------------------------------------------------------------------
//* Function Name       :
//* Object              :
//* Notes    			:
//* Notes   			:
//* Notes    			:
//* Context    			: CONTEXT_RESET
//*----------------------------------------------------------------------------
void bms_proc_hw_init(void)
{
	GPIO_InitTypeDef  gpio_init_structure;

	gpio_init_structure.Pin   = BMS_PWM_PIN;
	gpio_init_structure.Pull  = GPIO_NOPULL;
	gpio_init_structure.Mode  = GPIO_MODE_OUTPUT_PP;
	HAL_GPIO_Init(BMS_PWM_PORT, &gpio_init_structure);

	// Full charge allowed
	HAL_GPIO_WritePin(BMS_PWM_PORT, BMS_PWM_PIN, GPIO_PIN_RESET);

	// Disable charge FET
	//HAL_GPIO_WritePin(BMS_PWM_PORT, BMS_PWM_PIN, GPIO_PIN_SET);
}

//*----------------------------------------------------------------------------
//* Function Name       :
//* Object              :
//* Notes    			:
//* Notes   			:
//* Notes    			:
//* Context    			: CONTEXT_RESET
//*----------------------------------------------------------------------------
void bms_proc_is_charging(void)
{
	// Need at least the init bit on
	if((batt_status == 0xFFFF)||(batt_status == 0))
		return;

	short curr = bq40z80_read_current();

	if(((batt_status & 0x40) != 0x40)&&(curr > 0))
		charge_mode = 1;
	else
		charge_mode = 0;

	// Disable PWM
	if(charge_mode == 0)
	{
		// Full charge allowed
		HAL_GPIO_WritePin(BMS_PWM_PORT, BMS_PWM_PIN, GPIO_PIN_RESET);
	}
}

//*----------------------------------------------------------------------------
//* Function Name       :
//* Object              :
//* Notes    			:
//* Notes   			:
//* Notes    			:
//* Context    			: CONTEXT_RESET
//*----------------------------------------------------------------------------
void bms_proc(void)
{
	static uchar soc_skip = 0;
	static ulong bms_timer = 0;

	// Run timer
	if(bms_timer == 0)
		bms_timer = sys_timer;
	else if((bms_timer + 500) < sys_timer)
		bms_timer = sys_timer;
	else
		return;

	//--printf("bms proc..  \r\n");

	// Update local status
	batt_status = bq40z80_read_status();

	// Update charge status
	bms_proc_is_charging();

	// Get charge data
	pack_curr = bq40z80_read_current();
	pack_volt = bq40z80_read_pack_voltage();

	// Update SOC(every 10s)
	if(soc_skip++ > 20)
	{
		soc = bq40z80_read_soc();
		//if(soc != 0xFF) printf("soc: %d%% \r\n", soc);

		soc_skip = 0;
	}
}

//*----------------------------------------------------------------------------
//* Function Name       :
//* Object              :
//* Notes    			:
//* Notes   			:
//* Notes    			:
//* Context    			: CONTEXT_RESET
//*----------------------------------------------------------------------------
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
