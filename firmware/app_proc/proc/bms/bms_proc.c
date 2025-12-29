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

#ifdef CONTEXT_BMS

#include "shared_i2c.h"
#include "bq40z80.h"

#include "bms_proc.h"

struct BMSState	bmss;
//uchar bms_early_init_done = 0;

// ToDo: reuse for PA temperature protection
#if 0
// 10k NTC, https://www.skyeinstruments.com/wp-content/uploads/Steinhart-Hart-Eqn-for-10k-Thermistors.pdf
// R = 10K*ADC / (1023 - ADC), where 10K is the NTC voltage divider value, 1023(8-bit) is max ADC count
// https://learn.adafruit.com/thermistor/using-a-thermistor
static float bms_proc_steinhart_hart(float adc_cnt)
{
	const float SH_A = 0.001125308852122f;
	const float SH_B = 0.000234711863267f;
	const float SH_C = 0.000000085663516f;
	float res, logr;

	// ToDo: Vcc and Vref do not match, probably. Add compensation!
	//

	// ADC count to resistance (assumes NTC VCC = ADC VREF!)
	res = (10.0f * adc_cnt / ((float)ADC_RESOLUTION - adc_cnt)) * 1000.0f;

	// Steinhart-Hart
	logr = log(res);
	return (1.0f / (SH_A + SH_B * logr + SH_C * (pow(logr, 3.0))) - 273.15f);
}
#endif

//*----------------------------------------------------------------------------
//* Function Name       : bms_proc_power_off
//* Object              :
//* Notes    			: current BMS resolution is 500 mS!!
//* Notes   			:
//* Notes    			:
//* Context    			: CONTEXT_BMS
//*----------------------------------------------------------------------------
static void bms_proc_power_off(void)
{
	static uchar wait_radio_boot_up = 0;

	// Wait for full bootup, before allowing power off
	if(wait_radio_boot_up < 10)
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
   		vTaskDelay(200);

   		if(HAL_GPIO_ReadPin(POWER_BUTTON_PORT, POWER_BUTTON))
   		{
   			printf("user held button, will power off, bye!\r\n");
   			vTaskDelay(200);

   			// Need more cleanup ??
   			// ...

   			// Power off process
   			bsp_power_off();
   		}
   	}
}

//*----------------------------------------------------------------------------
//* Function Name       : bms_proc_pins_init
//* Object              :
//* Notes    			:
//* Notes   			:
//* Notes    			:
//* Context    			: CONTEXT_RESET
//*----------------------------------------------------------------------------
static void bms_proc_pins_init(void)
{
	GPIO_InitTypeDef  gpio_init_structure;

	//gpio_init_structure.Mode  = GPIO_MODE_OUTPUT_PP;
	gpio_init_structure.Pull  = GPIO_PULLDOWN;
	gpio_init_structure.Speed = GPIO_SPEED_FREQ_LOW;

	// Power button (encoder switch line)
	gpio_init_structure.Pin   = POWER_BUTTON;
	gpio_init_structure.Mode  = GPIO_MODE_INPUT;
	HAL_GPIO_Init(POWER_BUTTON_PORT, &gpio_init_structure);
}

//*----------------------------------------------------------------------------
//* Function Name       : bms_proc_hw_init
//* Object              :
//* Notes    			: call from main() on startup!
//* Notes   			:
//* Notes    			:
//* Context    			: CONTEXT_RESET
//*----------------------------------------------------------------------------
void bms_proc_hw_init(void)
{
	bms_proc_pins_init();

	// Init success
	//bms_early_init_done = 1;

	//printf("bms_proc_hw_init ok\r\n");
}

//*----------------------------------------------------------------------------
//* Function Name       : bms_proc_power_cleanup
//* Object              :
//* Notes    			:
//* Notes   			:
//* Notes    			:
//* Context    			: CONTEXT_RESET
//*----------------------------------------------------------------------------
void bms_proc_power_cleanup(void)
{
	// ToDo: what cleanup is needed ?
}

//*----------------------------------------------------------------------------
//* Function Name       : bms_proc_is_charging
//* Object              :
//* Notes    			: We have trickle charging during normal operation, so
//* Notes   			: detecting if BMS is toping up the batteries is bit
//* Notes    			: tricky
//* Context    			: CONTEXT_RESET
//*----------------------------------------------------------------------------
uchar bms_proc_is_charging(ushort status)
{
	// Need at least the init bit on
	if((status == 0xFFFF)||(status == 0))
		return 0;

	short curr = bq40z80_read_current();

	// Positive current is charging
	if(((status & 0x40) != 0x40)&&(curr > 0))
		return 1;
	else
		return 0;
}

//*----------------------------------------------------------------------------
//* Function Name       : bms_proc_worker
//* Object              :
//* Notes    			:
//* Notes   			:
//* Notes    			:
//* Context    			: CONTEXT_BMS
//*----------------------------------------------------------------------------
static void bms_proc_worker(void)
{
	static uchar bms_read_skip = 0;
	ushort status = 0;

	bms_proc_power_off();

	// We need to power off the BMS before cell removal
	if(bmss.shutdown_req)
	{
		#if 1
		//
		// Will nuke all permanent data in backup RAM/GPS as
		// this will disconnect discharge MOSFET !!!
		//
		bq40z80_unseal();		// unlock MAC access
		bq40z80_shutdown();		// shutdown safely
		//
		#endif

		bmss.shutdown_req = 0;
	}

	// Read status bits
	status = bq40z80_read_status();

	// Decide if batteries are charging
	bmss.charger_on = bms_proc_is_charging(status);

	// Slow local update of BMS params
	if(bms_read_skip == 0)
	{
		bmss.perc = bq40z80_read_soc();
		bmss.mins = bq40z80_read_runtime();

		// Decide if we run on USB voltage based on BMS
		// current draw from the battery pack
		if(bq40z80_read_current() > PACK_CURR_THRSH)
			bmss.run_on_dc = 1;
		else
			bmss.run_on_dc = 0;

		bq40z80_read_da_status();
	}

	bms_read_skip++;
	if(bms_read_skip > 50)
		bms_read_skip = 0;
}

//*----------------------------------------------------------------------------
//* Function Name       : bms_proc_task
//* Object              :
//* Notes    			: main process
//* Notes   			:
//* Notes    			:
//* Context    			: CONTEXT_BMS
//*----------------------------------------------------------------------------
void bms_proc_task(void const *arg)
{
	vTaskDelay(BMS_PROC_START_DELAY);
	printf("start\r\n");

	bmss.charger_on 	= 0;
	bmss.run_on_dc		= 0;
	bmss.shutdown_req 	= 0;

	// Detect BMS chip
	bq40z80_init();

bms_proc_loop:

	// Process
	bms_proc_worker();

	vTaskDelay(BMS_PROC_SLEEP_TIME);
	goto bms_proc_loop;
}

#endif




