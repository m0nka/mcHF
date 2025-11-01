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
#include "version.h"


#include "lcd_low.h"
#include "lcd_high.h"

#include "hw_lcd.h"
#include "hw_sdram.h"

#include "hw_sd.h"
#include "ff_gen_drv.h"
#include "sd_diskio.h"

#include "hw_flash.h"

#include "selftest_proc.h"
#include "keypad_proc.h"

#include "ui_proc.h"

static uint32_t LCD_X_Size = 0;
static uint32_t LCD_Y_Size = 0;

extern LTDC_HandleTypeDef hltdc;

extern ulong  	sys_timer;

extern ulong 	reset_reason;
extern uchar	gen_boot_reason_err;
extern ushort 	batt_status;
extern uchar  	charge_mode;
extern uchar 	soc;
extern short	pack_curr;
extern ushort  	pack_volt;

static void draw_atlas_circle(ushort c2x,ushort c2y, uchar dir)
{
	short factorX = -12;
	short factorY = -12;

	if(dir)
	{
		//factorX = 14;
		factorY = 12;
	}

	lcd_low_DrawCircle(c2x + factorX, 	c2y + factorY, 		17, 0x403079B0);
	lcd_low_DrawCircle(c2x + factorX, 	c2y + factorY, 		18, 0x403079B0);
	lcd_low_FillCircle(c2x + factorX/2, c2y + factorY/2, 	16, lcd_low_COLOR_BLACK);
	lcd_low_DrawCircle(c2x + factorX/2, c2y + factorY/2, 	17, 0x803b0bc0);
	lcd_low_DrawCircle(c2x + factorX/2, c2y + factorY/2, 	18, 0x803b0bc0);
	lcd_low_FillCircle(c2x, 			c2y, 				16, lcd_low_COLOR_BLACK);
	lcd_low_DrawCircle(c2x, 			c2y, 				17, 0xff3c8bc7);
	lcd_low_DrawCircle(c2x, 			c2y, 				18, 0xff3c8bc7);
	lcd_low_FillCircle(c2x, 			c2y, 			 	 9, 0xffffce23);
}

static void draw_atlas_ui(void)
{
	uchar i, j, k;
	ulong col;

	// Draw a gradient line
	for(j = 0, k = 0; j < 6; j++)
	{
		col = lcd_low_COLOR_DARKGRAY;
		for(i = 0; i < 6; i++)
		{
			if(i)
				lcd_low_FillRect(460, k + 45 + j*72 + + i*12, 4, 2, col);
			else
				lcd_low_FillRect(460, k + 45 + j*72 + + i*12, 6, 4, col);
			col += 0x101010;
		}

		if(j == 2) k = 20;
	}

	// Mid point vertical blue line
	lcd_low_DrawHLine(450, 265, 20, 0xff3b6a97);
	lcd_low_DrawHLine(450, 266, 20, 0xff3b6a97);
	lcd_low_DrawHLine(450, 267, 20, 0xff3b6a97);

	// Side circles
	draw_atlas_circle(430,450, 0);
	draw_atlas_circle(430, 60, 1);

	// Top text
	lcd_low_SetFont(&Font24);
	lcd_low_SetTextColor(lcd_low_COLOR_WHITE);
	lcd_low_DisplayStringAt(50, 160, (uint8_t *)"BOOTLOADER", LEFT_MODE);
}

uchar bare_lcd_init(void)
{
	if(BSP_LCD_Init(0, LCD_ORIENTATION_PORTRAIT) != BSP_ERROR_NONE)
	{
		printf("== lcd init error ==\r\n");
		return 1;
	}

	lcd_low_SetFuncDriver(&LCD_Driver);
	lcd_low_SetLayer(0);

	BSP_LCD_GetXSize(0, &LCD_X_Size);
	BSP_LCD_GetYSize(0, &LCD_Y_Size);

	HAL_LTDC_ProgramLineEvent(&hltdc, 0);
	HAL_GPIO_WritePin(LCD_BL_CTRL_GPIO_PORT, LCD_BL_CTRL_PIN, GPIO_PIN_SET); 			// backlight on

	lcd_low_Clear(lcd_low_COLOR_BLACK);

	//draw_atlas_ui();

	// Right side info bar
	//lcd_low_DrawRect(1, 345, 479, 134, lcd_low_COLOR_WHITE);							// rect outline

	lcd_low_DrawRect(290, 345, 189, 134, lcd_low_COLOR_LIGHTGRAY);
	lcd_low_DrawRect(  1, 345, 279, 134, lcd_low_COLOR_LIGHTBLUE);
	lcd_low_DrawRect(  1,  19,  70, 320, lcd_low_COLOR_LIGHTCYAN);
	//
	//lcd_low_SetBackColor(lcd_low_COLOR_WHITE);
	lcd_low_SetTextColor(lcd_low_COLOR_BLACK);
	lcd_low_DisplayStringAt(LINE(0) + 1, 335, (uint8_t *)"boot version", LEFT_MODE);	// label
	lcd_low_DisplayStringAt(LINE(2) + 1, 335, (uint8_t *)"coop version", LEFT_MODE);	// label

	return 0;
}

void ui_proc_show_bms_flags(void)
{
	ushort i, stat, line = 16;
	char 	buff[50];

	// Skip paint on error
	if((batt_status == 0xFFFF)||(batt_status == 0))
		return;

	//printf("sta: %04x \r\n", batt_status);

    // Text attributes
	lcd_low_SetBackColor(lcd_low_COLOR_BLACK);
	lcd_low_SetFont(&Font16);

	for(i = 0, stat = batt_status; i < 12; i++)
	{
		if((stat & 0x8000) == 0x8000)
			lcd_low_SetTextColor(lcd_low_COLOR_LIGHTGREEN);
		else
			lcd_low_SetTextColor(lcd_low_COLOR_DARKGRAY);

		switch(i)
		{
			case 0:
				lcd_low_DisplayStringAt(LINE(line), BMS_FLAGS_X1,  (uchar *)"OCA", LEFT_MODE);
				break;
			case 1:
				lcd_low_DisplayStringAt(LINE(line), BMS_FLAGS_X2,  (uchar *)"TCA", LEFT_MODE);
				break;
			case 2:
				lcd_low_DisplayStringAt(LINE(line), BMS_FLAGS_X1,  (uchar *)"---", LEFT_MODE);
				break;
			case 3:
				lcd_low_DisplayStringAt(LINE(line), BMS_FLAGS_X2,  (uchar *)"OTA", LEFT_MODE);
				break;
			case 4:
				lcd_low_DisplayStringAt(LINE(line), BMS_FLAGS_X1,  (uchar *)"TDA", LEFT_MODE);
				break;
			case 5:
				lcd_low_DisplayStringAt(LINE(line), BMS_FLAGS_X2,  (uchar *)"---", LEFT_MODE);
				break;
			case 6:
				lcd_low_DisplayStringAt(LINE(line), BMS_FLAGS_X1,  (uchar *)"RCA", LEFT_MODE);
				break;
			case 7:
				lcd_low_DisplayStringAt(LINE(line), BMS_FLAGS_X2,  (uchar *)"RTA", LEFT_MODE);
				break;
			case 8:
				lcd_low_DisplayStringAt(LINE(line), BMS_FLAGS_X1,  (uchar *)"INIT", LEFT_MODE);
				break;
			case 9:
				lcd_low_DisplayStringAt(LINE(line), BMS_FLAGS_X2,  (uchar *)"DSG", LEFT_MODE);
				break;
			case 10:
				lcd_low_DisplayStringAt(LINE(line), BMS_FLAGS_X1,  (uchar *)"FC", LEFT_MODE);
				break;
			case 11:
				lcd_low_DisplayStringAt(LINE(line), BMS_FLAGS_X2,  (uchar *)"FD", LEFT_MODE);
				break;
		}

		// Next row
		if((i)&&(i%2 != 0)) line += 2;

		// Next bit
		stat = stat << 1;
	}

	stat = batt_status & 4;
	if(stat)
	{
		lcd_low_SetTextColor(lcd_low_COLOR_LIGHTGREEN);
	}
	else
		lcd_low_SetTextColor(lcd_low_COLOR_DARKGRAY);

	lcd_low_DisplayStringAt(LINE(line), BMS_FLAGS_X1,  (uchar *)"EC = ", LEFT_MODE);
	sprintf(buff, "%d", stat);
	lcd_low_DisplayStringAt(LINE(line), BMS_FLAGS_X2,  (uchar *)buff, LEFT_MODE);
}

void ui_proc_show_charge_msg(void)
{
    // Text attributes
	lcd_low_SetBackColor(lcd_low_COLOR_BLACK);
	lcd_low_SetFont(&Font16);

	if(charge_mode)
		lcd_low_SetTextColor(lcd_low_COLOR_RED);
	else
		lcd_low_SetTextColor(lcd_low_COLOR_BLACK);

	lcd_low_DisplayStringAt(LINE(28), 40,  (uchar *)"[Charging]", LEFT_MODE);
}

void ui_proc_show_soc(void)
{
	char 	buff[50];

	if(soc == 0xFF)
		return;

    // Text attributes
	lcd_low_SetBackColor(lcd_low_COLOR_BLACK);
	lcd_low_SetFont(&Font16);

	if(charge_mode)
		lcd_low_SetTextColor(lcd_low_COLOR_RED);
	else
		lcd_low_SetTextColor(lcd_low_COLOR_WHITE);

	lcd_low_DisplayStringAt(LINE(14), BMS_FLAGS_X1,  (uchar *)"SOC:", LEFT_MODE);
	sprintf(buff, "%d%%", soc);
	lcd_low_DisplayStringAt(LINE(14), BMS_FLAGS_X2,  (uchar *)buff, LEFT_MODE);
}

void ui_proc_show_charge_data(void)
{
	char 	buff[50];

    // Text attributes
	lcd_low_SetBackColor(lcd_low_COLOR_BLACK);
	lcd_low_SetTextColor(lcd_low_COLOR_WHITE);
	lcd_low_SetFont(&Font16);

	lcd_low_DisplayStringAt(LINE(26), 15,  (uchar *)"Pack:", LEFT_MODE);
	lcd_low_DisplayStringAt(LINE(27), 15,  (uchar *)"Curr:", LEFT_MODE);

	sprintf(buff, "%dmV", pack_volt);
	lcd_low_DisplayStringAt(LINE(26), 100,  (uchar *)buff, LEFT_MODE);

	sprintf(buff, "%dmA     ", pack_curr);
	lcd_low_DisplayStringAt(LINE(27), 100,  (uchar *)buff, LEFT_MODE);
}

void ui_proc_show_keyboard(void)
{
	char 	buff[50];
	static ulong keyb_timer = 0;

	// Get last key
	uchar key = keypad_proc_get(0);

	if(key == 0)
		return;

    // Text attributes
	lcd_low_SetBackColor(lcd_low_COLOR_BLACK);
	lcd_low_SetTextColor(lcd_low_COLOR_GREEN);
	lcd_low_SetFont(&Font16);

	// Run timer
	if(keyb_timer == 0)
		keyb_timer = sys_timer;
	else if((keyb_timer + 4000) < sys_timer)
	{
		keyb_timer = sys_timer;
		keypad_proc_get(1);

		lcd_low_SetTextColor(lcd_low_COLOR_BLACK);
		lcd_low_DisplayStringAt(LINE(24), 15,  (uchar *)"                         ", LEFT_MODE);
		return;
	}

	lcd_low_DisplayStringAt(LINE(24), 15,  (uchar *)"KEY:", LEFT_MODE);

	sprintf(buff, "x=%d, y=%d", key >> 4, key & 0x0F);
	lcd_low_DisplayStringAt(LINE(24), 80,  (uchar *)buff, LEFT_MODE);
}

void ui_proc_bootup(void)
{
	char 	buff[200];
	int 	line = 1;

	// Init LCD
    if(bare_lcd_init() != 0)
    {
    	goto run_radio;
    }

    // Text attributes
	lcd_low_SetBackColor(lcd_low_COLOR_BLACK);
	lcd_low_SetTextColor(lcd_low_COLOR_WHITE);
	lcd_low_SetFont(&Font16);

	// -----------------------------------------------------------------------------------------------
	// -----------------------------------------------------------------------------------------------
	sprintf(buff, "%s(via F4)", DEVICE_STRING);
	lcd_low_DisplayStringAt(LINE(line), LEFT_POS, (uchar *)buff, LEFT_MODE);
	line += 2;
	sprintf(buff, "%d.%d.%d.%d", MCHF_L_VER_MAJOR, MCHF_L_VER_MINOR, MCHF_L_VER_RELEASE, MCHF_L_VER_BUILD);
	lcd_low_DisplayStringAt(LINE(1) + 1, 350, (uint8_t *)buff, LEFT_MODE);	// label

	// Show bms flags in the right panel
	ui_proc_show_bms_flags();

    // Text attributes
	lcd_low_SetBackColor(lcd_low_COLOR_BLACK);
	lcd_low_SetTextColor(lcd_low_COLOR_WHITE);
	lcd_low_SetFont(&Font16);

	// -----------------------------------------------------------------------------------------------
	// -----------------------------------------------------------------------------------------------
	// Test for general boot error (clocks, lcd, etc)
	lcd_low_DisplayStringAt(LINE(line), LEFT_POS, (uchar *)"Testing BOOT UP....", LEFT_MODE);

	if(gen_boot_reason_err == 0)
	{
		//HAL_Delay(500);
		lcd_low_DisplayStringAt(LINE(line), LEFT_POS, (uchar *)"Testing BOOT UP....PASS", LEFT_MODE);
	}
	else
	{
		//HAL_Delay(500);
		sprintf(buff, "Update Firmware....FAIL(%d)", gen_boot_reason_err);
		lcd_low_DisplayStringAt(LINE(line), LEFT_POS, (uchar *)buff, LEFT_MODE);
	}
	line++;

	// -----------------------------------------------------------------------------------------------
	// -----------------------------------------------------------------------------------------------
	// Test BMS
	lcd_low_DisplayStringAt(LINE(line), LEFT_POS, (uchar *)"Testing BMS........", LEFT_MODE);

	if((batt_status != 0xFFFF)&&(batt_status != 0))
	{
		//HAL_Delay(500);
		lcd_low_DisplayStringAt(LINE(line), LEFT_POS, (uchar *)"Testing BMS........PASS", LEFT_MODE);
	}
	else
	{
		//HAL_Delay(500);
		sprintf(buff, "Testing BMS........FAIL(%04x)", batt_status);
		lcd_low_DisplayStringAt(LINE(line), LEFT_POS, (uchar *)buff, LEFT_MODE);
	}
	line++;

	// -----------------------------------------------------------------------------------------------
	// -----------------------------------------------------------------------------------------------
	// SDRAM test
	lcd_low_DisplayStringAt(LINE(line), LEFT_POS, (uchar *)"Testing SDRAM......", LEFT_MODE);

	if(sdram_test() != 0)
	{
		//HAL_Delay(500);
		lcd_low_DisplayStringAt(LINE(line), LEFT_POS, (uchar *)"Testing SDRAM......FAIL.", LEFT_MODE);
		goto stall;
	}
	else
	{
		//HAL_Delay(500);
		lcd_low_DisplayStringAt(LINE(line), LEFT_POS, (uchar *)"Testing SDRAM......PASS", LEFT_MODE);
	}
	line++;

	// -----------------------------------------------------------------------------------------------
	// -----------------------------------------------------------------------------------------------
	// SD Card test
	lcd_low_DisplayStringAt(LINE(line), LEFT_POS, (uchar *)"Testing SD Card....", LEFT_MODE);

	if(test_sd_card() == 0)
	{
		//HAL_Delay(500);
		lcd_low_DisplayStringAt(LINE(line), LEFT_POS, (uchar *)"Testing SD Card....PASS", LEFT_MODE);
		line++;

		if(reset_reason == RESET_UPDATE_FW)
		{
			//HAL_Delay(500);
			lcd_low_DisplayStringAt(LINE(line), LEFT_POS, (uchar *)"Update Firmware....", LEFT_MODE);

			uchar res = 0;//update_radio();
			if(res == 0)
			{
				//HAL_Delay(500);
				lcd_low_DisplayStringAt(LINE(line), LEFT_POS, (uchar *)"Update Firmware....PASS", LEFT_MODE);
			}
			else
			{
				//HAL_Delay(500);
				sprintf(buff, "Update Firmware....FAIL(%d)", res);
				lcd_low_DisplayStringAt(LINE(line), LEFT_POS, (uchar *)buff, LEFT_MODE);
			}
			line++;
		}

		#if 0
		lcd_low_DisplayStringAt(LINE(line), LEFT_POS, (uchar *)"Upload DSP Core....", LEFT_MODE);
		uchar d_res = load_default_dsp_core(1);
		if(d_res == 0)
			lcd_low_DisplayStringAt(LINE(line), LEFT_POS, (uchar *)"Upload DSP Core....PASS", LEFT_MODE);
		else
		{
			sprintf(buff, "Upload DSP Core....FAIL(%d)", d_res);
			lcd_low_DisplayStringAt(LINE(line), LEFT_POS, (uchar *)buff, LEFT_MODE);
		}
		line++;
		#endif

		// Clean up anyway, do we need on fail sd test as well ?
		fs_cleanup();
	}
	else
	{
		//HAL_Delay(500);
		lcd_low_DisplayStringAt(LINE(line), LEFT_POS, (uchar *)"Testing SD Card....FAIL", LEFT_MODE);
		line++;
	}

	// -----------------------------------------------------------------------------------------------
	// -----------------------------------------------------------------------------------------------
	// Firmware test
//!	lcd_low_DisplayStringAt(LINE(line), LEFT_POS, (uchar *)"Testing Firmware...", LEFT_MODE);

run_radio:
	return;

stall:
	return;


#if 0
	if(is_firmware_valid() == 0)
	{
		//HAL_Delay(500);
//!		lcd_low_DisplayStringAt(LINE(line), LEFT_POS, (uchar *)"Testing Firmware...PASS", LEFT_MODE);
		line++;

//!		lcd_low_DisplayStringAt(LINE(line), LEFT_POS, (uchar *)"Booting to radio...", LEFT_MODE);
		//HAL_Delay(2000);

		// Jump
		jump_to_fw(RADIO_FIRM_ADDR);
	}

	HAL_Delay(500);
	lcd_low_DisplayStringAt(LINE(line), LEFT_POS, (uchar *)"Testing Firmware...FAIL", LEFT_MODE);
	line++;

	// Test - chip blank programming
	#if 0
	HAL_PWR_EnableBkUpAccess();
	WRITE_REG(BKP_REG_RESET_REASON, RESET_UPDATE_FW);
	HAL_PWR_DisableBkUpAccess();
	NVIC_SystemReset();
	#endif

	// -----------------------------------------------------------------------------------------------
	// -----------------------------------------------------------------------------------------------
	// Stall, and power off eventually
stall:

	ulong pc = 0;
	while(1)
	{
		HAL_Delay(1000);
		pc++;

		// On timeout go to sleep, to prevent draining of batteries
		if(pc > 25)
		{
			lcd_low_DisplayStringAt(LINE(line), 10, (uchar *)"Testing Power Off...", LEFT_MODE);
			line++;

			printf("== will power off ==\r\n");
			power_off_x(0);
		}
	}
#endif
}

void ui_proc(void)
{
	static ulong ui_timer = 0;

	// Run timer
	if(ui_timer == 0)
		ui_timer = sys_timer;
	else if((ui_timer + 500) < sys_timer)
		ui_timer = sys_timer;
	else
		return;

	// Toggle backlight, to track fading problem on 4.3inch LCD
	#if 0
	static uchar xl = 0;
	if(xl++ > 10)
	{
		HAL_GPIO_TogglePin(LCD_BL_CTRL_GPIO_PORT, LCD_BL_CTRL_PIN);
		xl = 0;
	}
	#endif

	ui_proc_show_bms_flags();
	ui_proc_show_charge_msg();
	ui_proc_show_soc();
	ui_proc_show_charge_data();
	ui_proc_show_keyboard();
}

void ui_proc_init(void)
{
	ui_proc_bootup();
}
