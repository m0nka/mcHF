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

#ifdef CONTEXT_BMS

#include "shared_i2c.h"
#include "bq40z80.h"
#include "bq25730.h"
#include "ch224a.h"

#include "bms_proc.h"
#include "bms_gold.h"

// Local state
struct BMSState				bmss;

// Charger chip state
bq25730_config_t 			chip_cfg;

// FreeRTOS process state
extern struct PROC_STATE 	ps;

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
   	if(!HAL_GPIO_ReadPin(POWER_BUTTON_PORT, POWER_BUTTON))
   	{
   		// ToDo: Use power button hold as power off
   		//       click as Mute...
   		//
   		vTaskDelay(200);

   		if(!HAL_GPIO_ReadPin(POWER_BUTTON_PORT, POWER_BUTTON))
   		{
   			printf("user held button, will power off, bye!\r\n");
   			vTaskDelay(200);

   			// Need more cleanup ??
   			// ...

   			// Power off process
   			board_power_off();
   		}
   	}
}

static void bms_proc_shutdown(void)
{
	if(!bmss.shutdown_req)
		return;

	#if 1
	//
	// Will nuke all permanent data in backup RAM/GPS as
	// this will disconnect discharge MOSFET !!!
	//
	if(!(bmss.bms_unlock_state))
		bq40z80_unseal();		// unlock MAC access

	bq40z80_shutdown();			// shutdown safely
	//
	#endif

	bmss.shutdown_req = 0;
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
	gpio_init_structure.Pull  = GPIO_PULLUP;
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
	GPIO_InitTypeDef  gpio_init_structure;

	// -----------------------------------------------------------------------
	// Power button cleanup - remove pullup, so the BMS PRES line is released!
	// -----------------------------------------------------------------------
	gpio_init_structure.Pull  = GPIO_NOPULL;
	gpio_init_structure.Speed = GPIO_SPEED_FREQ_LOW;
	gpio_init_structure.Pin   = POWER_BUTTON;
	gpio_init_structure.Mode  = GPIO_MODE_INPUT;
	HAL_GPIO_Init(POWER_BUTTON_PORT, &gpio_init_structure);

	// Lock the BMS
	if(bmss.bms_unlock_state)
		bq40z80_seal();
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

//*--------------------------------------------------------------------------------------
//* Function Name       : bms_proc_wait_msg
//* Object              : Read pending messages
//* Input Parameters    : Rx Queue ptr and items buffer
//* Output Parameters   : none.
//*--------------------------------------------------------------------------------------
static uchar bms_proc_wait_msg(xQueueHandle pRxQueue, ulong *ulQueueBuffer)
{
	uchar ucNext = 0;

	if(pRxQueue == NULL)
		return 0;

	*ulQueueBuffer = 0;
	while(uxQueueMessagesWaiting(pRxQueue))
	{
		if(xQueueReceive(pRxQueue, (ulQueueBuffer + ucNext), (portTickType)0) == pdPASS)
		{
			ucNext++;
		}
	}

	return ucNext;
}

//*----------------------------------------------------------------------------
//* Function Name       : bms_proc_handle_fan
//* Object              :
//* Notes    			:
//* Notes   			:
//* Notes    			:
//* Context    			: CONTEXT_BMS
//*----------------------------------------------------------------------------
void bms_proc_handle_fan(void)
{
	static uchar fan_state = 0xFF;

	if(bmss.charger_on)
	{
		//--printf("curr %d \r\n", bmss.curr);
		if((bmss.curr > PACK_CURR_CHARGE_ON)&&(fan_state == 0))
		{
			//printf("notify fan on \r\n");
			fan_state = 1;

			#ifdef CONTEXT_FAN
			if(ps.hFanTask != NULL)
				xTaskNotify(ps.hFanTask, 0x02, eSetValueWithOverwrite);
			#endif
		}
		else if((bmss.curr < PACK_CURR_CHARGE_OFF)&&(fan_state == 1))
		{
			//printf("notify fan off \r\n");
			fan_state = 0;

			#ifdef CONTEXT_FAN
			if(ps.hFanTask != NULL)
				xTaskNotify(ps.hFanTask, 0x01, eSetValueWithOverwrite);
			#endif
		}
		else if(fan_state == 0xFF)
		{
			//printf("notify reset \r\n");
			fan_state = 0;
		}
	}
	else if(fan_state == 1)
	{
		//printf("notify fan off \r\n");
		fan_state = 0;

		#ifdef CONTEXT_FAN
		if(ps.hFanTask != NULL)
			xTaskNotify(ps.hFanTask, 0x01, eSetValueWithOverwrite);
		#endif
	}
}

void bms_proc_init_charger(void)
{
	// BQ25730 chip configuration
	chip_cfg.adc_mode 		= ADC_CONV_CONT;
	chip_cfg.watchdog_adj	= WDTMR_ADJ_DISABLE;
	chip_cfg.rsr 			= RSNS_5MOHM;
	chip_cfg.rac 			= RSNS_5MOHM;

	// Init
	uchar res = bq25730_init(&chip_cfg);
	if(res)
		printf("charger init err: %d \r\n", res);
}

void bms_proc_charger_handler(void)
{
	bmss.ch_stat = bq25730_read_chg_stat(&chip_cfg);
	bmss.ch_curr = bq25730_read_iin(&chip_cfg);
	bmss.ch_vsys = bq25730_read_vsys(&chip_cfg);
	bmss.ch_vbat = bq25730_read_vbat(&chip_cfg);
	bmss.ch_vbus = bq25730_read_vbus(&chip_cfg);

	bq25730_read_ibat(&chip_cfg, &bmss.ch_chv, &bmss.ch_dcv);

	#if 0
	printf("[%04x] vsys:%d vbat:%d vbus:%d ch:%d dc:%d cr:%d \r\n",
			bmss.ch_stat,
			bmss.ch_vsys,
			bmss.ch_vbat,
			bmss.ch_vbus,
			bmss.ch_chv,
			bmss.ch_dcv,
			bmss.ch_curr);
	#endif
}

//*----------------------------------------------------------------------------
//* Function Name       : bms_proc_worker
//* Object              :
//* Notes    			:
//* Notes   			:
//* Notes    			:
//* Context    			: CONTEXT_BMS
//*----------------------------------------------------------------------------
static void bms_proc_worker(void const *param)
{
	xQueueHandle	*RxQueue;
	ulong 			ulRxData[10];
	ushort 			status = 0;

	//static uchar bms_unlock_stat 	= 0;
	static uchar bms_read_skip 		= 0;

	// Get rx queue ptr
	RxQueue = (xQueueHandle *)param;

	// Any requests, by anyone ?
	if(bms_proc_wait_msg(*RxQueue, ulRxData) > 0)
	{
		// Process message request
		switch(ulRxData[0])
		{
			case 0x27:
			{
				//printf("unlock\r\n");
				if(!bq40z80_unseal())
				{
					bmss.bms_unlock_state = 1;
				}

				break;
			}

			case 0x2A:
			{
				//printf("lock\r\n");
				if(!bq40z80_seal())
				{
					bmss.bms_unlock_state = 0;
				}

				break;
			}

			#ifdef CONTEXT_SD
			// Dump gauge data flash to SD card
			case 0x30:
			{
				bms_gold_backup();
				break;
			}

			// Program gauge data flash from SD card gold file
			case 0x31:
			{
				bms_gold_flash();
				break;
			}
			#endif

			// Read calibration data for diagnostics
			case 0x32:
			{
				uchar cal_res = bq40z80_read_cal_data();

				// Print live current from both chips
				printf("bq40z80 curr: %dmA\r\n", bmss.curr);
				printf("bq25730 ichg: %dmA\r\n", bmss.ch_chv);
				if(bmss.ch_chv > 0)
					printf("ratio: %d.%02dx\r\n",
						bmss.curr / bmss.ch_chv,
						(int)(((long)bmss.curr * 100) / bmss.ch_chv) % 100);

				if(cal_res)
					printf("cal read err %d\r\n", cal_res);

				break;
			}

			// Read CC Gain for calibration UI
			case 0x33:
			{
				float cc;
				short curr_abs;
				int wh, fr;

				if(bq40z80_read_cc_gain(&cc) != 0)
				{
					bmss.cal_state = 3;
					printf("cc gain read err\r\n");
					break;
				}

				bmss.cal_cc_raw = cc;

				wh = (int)cc;
				fr = (int)(cc * 1000) - (wh * 1000);
				if(fr < 0) fr = -fr;
				printf("cc gain: %d.%03d\r\n", wh, fr);

				// Auto-compute proposed value from charger reference
				curr_abs = bmss.curr;
				if(curr_abs < 0) curr_abs = -curr_abs;

				if(bmss.ch_chv > 100 && curr_abs > 100)
				{
					bmss.cal_cc_new = cc * (float)curr_abs / (float)bmss.ch_chv;
					printf("ratio: %d.%02dx\r\n",
						curr_abs / bmss.ch_chv,
						(int)(((long)curr_abs * 100) / bmss.ch_chv) % 100);
				}
				else
				{
					bmss.cal_cc_new = cc;
					printf("no charger ref\r\n");
				}

				wh = (int)bmss.cal_cc_new;
				fr = (int)(bmss.cal_cc_new * 1000) - (wh * 1000);
				if(fr < 0) fr = -fr;
				printf("proposed: %d.%03d\r\n", wh, fr);

				bmss.cal_state = 1;
				break;
			}

			// Write CC Gain from cal_cc_new to data flash
			case 0x34:
			{
				uchar buf[32];
				uchar vbuf[32];
				int wh, fr;

				wh = (int)bmss.cal_cc_new;
				fr = (int)(bmss.cal_cc_new * 1000) - (wh * 1000);
				if(fr < 0) fr = -fr;
				printf("writing cc: %d.%03d\r\n", wh, fr);

				// Full access needed for DF writes
				if(bq40z80_full_access() != 0)
				{
					bmss.cal_state = 4;
					printf("full access err\r\n");
					break;
				}

				// Read current row (read-modify-write)
				if(bq40z80_df_read_row(0x4000, buf, 32) != 0)
				{
					bmss.cal_state = 4;
					printf("df read err\r\n");
					break;
				}

				// Patch CC Gain at offset 6 (4 bytes IEEE 754 float)
				memcpy(&buf[6], &bmss.cal_cc_new, 4);

				// Write row back
				if(bq40z80_df_write_row(0x4000, buf, 32) != 0)
				{
					bmss.cal_state = 4;
					printf("df write err\r\n");
					break;
				}

				// Verify
				if(bq40z80_df_read_row(0x4000, vbuf, 32) != 0)
				{
					bmss.cal_state = 4;
					printf("verify read err\r\n");
					break;
				}

				if(memcmp(&vbuf[6], &bmss.cal_cc_new, 4) != 0)
				{
					bmss.cal_state = 4;
					printf("verify mismatch!\r\n");
					break;
				}

				bmss.cal_state = 2;
				printf("cc gain write ok\r\n");
				printf("restart radio for new cal\r\n");
				break;
			}

			default:
				break;
		}
	}

	// How often do we need to handle it ?
	if(ch224a_detect() == 0)
	{
		// Reset charger chip
		bms_proc_init_charger();

		// Read charging status
		bms_proc_charger_handler();
	}
	else
	{
		// Clear publics
		bmss.max_curr 		= 0;
		bmss.usbpd_status 	= 0;
		bmss.ch_stat		= 0;
		bmss.ch_chv			= 0;
		bmss.ch_dcv			= 0;
		bmss.ch_curr		= 0;
		bmss.ch_vsys		= 0;
		bmss.ch_vbat		= 0;
		bmss.ch_vbus		= 0;
	}

	// Handle power off
	bms_proc_power_off();

	// We need to power off the BMS before cell removal
	bms_proc_shutdown();

	// Read status bits
	status = bq40z80_read_status();

	// Decide if batteries are charging
	bmss.charger_on = bms_proc_is_charging(status);

	// Read cell status
	if(bmss.bms_unlock_state)
	{
		// Read cell status
		bq40z80_read_da_status();

		// Get pack voltage
		bmss.pack_v = bq40z80_read_pack_voltage();

		// Enforce faster read interval(when in BMS menu)
		bms_read_skip = 0;
	}

	// Slow local update of BMS params
	if(bms_read_skip == 0)
	{
		bmss.perc = bq40z80_read_soc();
		bmss.mins = bq40z80_read_runtime();

		// Decide if we run on USB voltage based on BMS
		// current draw from the battery pack
		bmss.curr = bq40z80_read_current();
		if(bmss.curr > PACK_CURR_THRSH)
			bmss.run_on_dc = 1;
		else
			bmss.run_on_dc = 0;
	}

	// Do we need a fan ?
	bms_proc_handle_fan();

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
	//printf("start\r\n");

	// Init publics
	bmss.charger_on 		= 0;
	bmss.run_on_dc			= 0;
	bmss.shutdown_req 		= 0;
	bmss.bms_unlock_state	= 0;
	bmss.gold_state			= 0;
	bmss.gold_perc			= 0;
	bmss.gold_err			= 0;
	bmss.gold_line			= 0;
	bmss.max_curr			= 0;
	bmss.usbpd_status		= 0;

	bmss.ch_stat			= 0;
	bmss.ch_chv				= 0;
	bmss.ch_dcv				= 0;
	bmss.ch_curr			= 0;
	bmss.ch_vsys			= 0;
	bmss.ch_vbat			= 0;
	bmss.ch_vbus			= 0;

	bmss.cal_cc_raw			= 0.0f;
	bmss.cal_cc_new			= 0.0f;
	bmss.cal_state			= 0;

	// Detect BMS chip
	bq40z80_init();

	// Charger chip init, moved to polling call!
	bms_proc_init_charger();

bms_proc_loop:

	// Process
	bms_proc_worker(arg);

	vTaskDelay(BMS_PROC_SLEEP_TIME);
	goto bms_proc_loop;
}

#endif




