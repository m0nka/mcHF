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



#include "ui_proc.h"

static uint32_t LCD_X_Size = 0;
static uint32_t LCD_Y_Size = 0;

extern LTDC_HandleTypeDef hltdc;

extern ulong reset_reason;
extern uchar gen_boot_reason_err;

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
	lcd_low_DrawRect(1, 345, 479, 134, lcd_low_COLOR_WHITE);							// rect outline
	//
	//lcd_low_SetBackColor(lcd_low_COLOR_WHITE);
	lcd_low_SetTextColor(lcd_low_COLOR_BLACK);
	lcd_low_DisplayStringAt(LINE(0) + 1, 335, (uint8_t *)"boot version", LEFT_MODE);	// label
	lcd_low_DisplayStringAt(LINE(2) + 1, 335, (uint8_t *)"coop version", LEFT_MODE);	// label

	return 0;
}

void boot_process(void)
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
	sprintf(buff, "%s", DEVICE_STRING);
	lcd_low_DisplayStringAt(LINE(line), 40, (uchar *)buff, LEFT_MODE);
	line += 2;
	sprintf(buff, "%d.%d.%d.%d", MCHF_L_VER_MAJOR, MCHF_L_VER_MINOR, MCHF_L_VER_RELEASE, MCHF_L_VER_BUILD);
	lcd_low_DisplayStringAt(LINE(1) + 1, 350, (uint8_t *)buff, LEFT_MODE);	// label

	// -----------------------------------------------------------------------------------------------
	// -----------------------------------------------------------------------------------------------
	// Test for general boot error (clocks, lcd, etc)
	lcd_low_DisplayStringAt(LINE(line), 40, (uchar *)"Testing BOOT err...", LEFT_MODE);

	if(gen_boot_reason_err == 0)
	{
		//HAL_Delay(500);
		lcd_low_DisplayStringAt(LINE(line), 40, (uchar *)"Testing BOOT err...PASS", LEFT_MODE);
	}
	else
	{
		HAL_Delay(500);
		sprintf(buff, "Update Firmware....FAIL(%d)", gen_boot_reason_err);
		lcd_low_DisplayStringAt(LINE(line), 40, (uchar *)buff, LEFT_MODE);
	}
	line++;

	// -----------------------------------------------------------------------------------------------
	// -----------------------------------------------------------------------------------------------
	// Test if ESP32 alive
	#ifdef CONTEXT_IPC_PROC
	lcd_low_DisplayStringAt(LINE(line), 10, (uchar *)"Testing Baseband...", LEFT_MODE);

	if(ipc_proc_establish_link(buff) == 0)
	{
		lcd_low_DisplayStringAt(LINE(line), 10, (uchar *)"Testing Baseband...PASS", LEFT_MODE);
		line++;

		lcd_low_DisplayStringAt(LINE(3) + 1, 350, (uint8_t *)buff, LEFT_MODE);
	}
	else
	{
		lcd_low_DisplayStringAt(LINE(line), 10, (uchar *)"Testing Baseband...FAIL", LEFT_MODE);
		line++;
	}
	#endif

#if 0
	// -----------------------------------------------------------------------------------------------
	// -----------------------------------------------------------------------------------------------
	// DSP test(already done on reset, so just showing result)
	lcd_low_DisplayStringAt(LINE(line), 10, (uchar *)"Testing DSP core...", LEFT_MODE);

	if(dsp_core_stat != 0)
		lcd_low_DisplayStringAt(LINE(line), 10, (uchar *)"Testing DSP core...FAIL", LEFT_MODE);
	else
		lcd_low_DisplayStringAt(LINE(line), 10, (uchar *)"Testing DSP core...PASS", LEFT_MODE);
	line++;
#endif

	// -----------------------------------------------------------------------------------------------
	// -----------------------------------------------------------------------------------------------
	// SDRAM test
	lcd_low_DisplayStringAt(LINE(line), 40, (uchar *)"Testing SDRAM......", LEFT_MODE);

	if(sdram_test() != 0)
	{
		HAL_Delay(500);
		lcd_low_DisplayStringAt(LINE(line), 40, (uchar *)"Testing SDRAM......FAIL.", LEFT_MODE);
		goto stall;
	}
	else
	{
		//HAL_Delay(500);
		lcd_low_DisplayStringAt(LINE(line), 40, (uchar *)"Testing SDRAM......PASS", LEFT_MODE);
	}
	line++;

	// -----------------------------------------------------------------------------------------------
	// -----------------------------------------------------------------------------------------------
	// SD Card test
	lcd_low_DisplayStringAt(LINE(line), 40, (uchar *)"Testing SD Card....", LEFT_MODE);

	if(test_sd_card() == 0)
	{
		//HAL_Delay(500);
		lcd_low_DisplayStringAt(LINE(line), 40, (uchar *)"Testing SD Card....PASS", LEFT_MODE);
		line++;

		if(reset_reason == RESET_UPDATE_FW)
		{
			//HAL_Delay(500);
			lcd_low_DisplayStringAt(LINE(line), 40, (uchar *)"Update Firmware....", LEFT_MODE);

			uchar res = 0;//update_radio();
			if(res == 0)
			{
				//HAL_Delay(500);
				lcd_low_DisplayStringAt(LINE(line), 40, (uchar *)"Update Firmware....PASS", LEFT_MODE);
			}
			else
			{
				//HAL_Delay(500);
				sprintf(buff, "Update Firmware....FAIL(%d)", res);
				lcd_low_DisplayStringAt(LINE(line), 40, (uchar *)buff, LEFT_MODE);
			}
			line++;
		}

		#if 0
		lcd_low_DisplayStringAt(LINE(line), 40, (uchar *)"Upload DSP Core....", LEFT_MODE);
		uchar d_res = load_default_dsp_core(1);
		if(d_res == 0)
			lcd_low_DisplayStringAt(LINE(line), 40, (uchar *)"Upload DSP Core....PASS", LEFT_MODE);
		else
		{
			sprintf(buff, "Upload DSP Core....FAIL(%d)", d_res);
			lcd_low_DisplayStringAt(LINE(line), 40, (uchar *)buff, LEFT_MODE);
		}
		line++;
		#endif

		// Clean up anyway, do we need on fail sd test as well ?
		fs_cleanup();
	}
	else
	{
		//HAL_Delay(500);
		lcd_low_DisplayStringAt(LINE(line), 40, (uchar *)"Testing SD Card....FAIL", LEFT_MODE);
		line++;
	}

	// -----------------------------------------------------------------------------------------------
	// -----------------------------------------------------------------------------------------------
	// Firmware test
//!	lcd_low_DisplayStringAt(LINE(line), 40, (uchar *)"Testing Firmware...", LEFT_MODE);

run_radio:

	if(is_firmware_valid() == 0)
	{
		//HAL_Delay(500);
//!		lcd_low_DisplayStringAt(LINE(line), 40, (uchar *)"Testing Firmware...PASS", LEFT_MODE);
		line++;

//!		lcd_low_DisplayStringAt(LINE(line), 40, (uchar *)"Booting to radio...", LEFT_MODE);
		//HAL_Delay(2000);

		// Jump
//!		jump_to_fw(RADIO_FIRM_ADDR);
	}

	HAL_Delay(500);
	lcd_low_DisplayStringAt(LINE(line), 40, (uchar *)"Testing Firmware...FAIL", LEFT_MODE);
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
}
