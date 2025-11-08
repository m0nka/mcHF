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

	//gpio_init_structure.Mode  = GPIO_MODE_OUTPUT_PP;
	gpio_init_structure.Pull  = GPIO_PULLDOWN;
	gpio_init_structure.Speed = GPIO_SPEED_FREQ_LOW;

	#ifndef PCB_V9_REV_A
	gpio_init_structure.Pin   = BMS_PWM_PIN;
	gpio_init_structure.Pull  = GPIO_NOPULL;
	gpio_init_structure.Mode  = GPIO_MODE_OUTPUT_PP;
	HAL_GPIO_Init(BMS_PWM_PORT, &gpio_init_structure);

	// Full charge allowed
	HAL_GPIO_WritePin(BMS_PWM_PORT, BMS_PWM_PIN, GPIO_PIN_RESET);
	#endif

	// Disable charge FET
	//HAL_GPIO_WritePin(BMS_PWM_PORT, BMS_PWM_PIN, GPIO_PIN_SET);

	// Power button (encoder switch line)
	gpio_init_structure.Pin   = POWER_BUTTON;
	gpio_init_structure.Mode  = GPIO_MODE_INPUT;
	HAL_GPIO_Init(POWER_BUTTON_PORT, &gpio_init_structure);
}

static void bms_proc_power_off(void)
{
	static uchar wait_radio_boot_up = 0;

	// Wait for full bootup, before allowing power off
	if(wait_radio_boot_up < 100)
	{
		wait_radio_boot_up++;
		return;
	}

	// Check for power off
   	if(HAL_GPIO_ReadPin(POWER_BUTTON_PORT, POWER_BUTTON))
   	{
   		// ToDo: Use power button hold as power off
   		//       click as Mute...
   		//
   		HAL_Delay(200);

   		if(HAL_GPIO_ReadPin(POWER_BUTTON_PORT, POWER_BUTTON))
   		{
   			printf("user held button, will power off, bye!\r\n");
   			HAL_Delay(200);

   			// Power off
   			//HAL_GPIO_WritePin(LCD_BL_CTRL_GPIO_PORT, LCD_BL_CTRL_PIN, GPIO_PIN_RESET);

   			LL_GPIO_ResetOutputPin(ON_LED_PORT, ON_LED);
   			LL_GPIO_ResetOutputPin(TX_LED_PORT, TX_LED);
   			LL_GPIO_ResetOutputPin(POWER_HOLD_PORT, POWER_HOLD);
   			while(1);
   		}
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
		#ifndef PCB_V9_REV_A
		HAL_GPIO_WritePin(BMS_PWM_PORT, BMS_PWM_PIN, GPIO_PIN_RESET);
		#endif
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

	bms_proc_power_off();

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
