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
#include "menu_proc.h"

static uint32_t LCD_X_Size = 0;
static uint32_t LCD_Y_Size = 0;

extern ulong  	sys_timer;

extern ulong 	reset_reason;
extern uchar	gen_boot_reason_err;
extern ushort 	batt_status;
extern uchar  	charge_mode;
extern uchar 	soc;
extern short	pack_curr;
extern ushort  	pack_volt;

#if 0
static void draw_atlas_circle(ushort c2x,ushort c2y, uchar dir)
{
	short factorX = -12;
	short factorY = -12;

	if(dir)
	{
		//factorX = 14;
		factorY = 12;
	}

	lcd_high_DrawCircle(c2x + factorX, 	c2y + factorY, 		17, 0x403079B0);
	lcd_high_DrawCircle(c2x + factorX, 	c2y + factorY, 		18, 0x403079B0);
	lcd_high_FillCircle(c2x + factorX/2, c2y + factorY/2, 	16, LCD_COLOR_BLACK);
	lcd_high_DrawCircle(c2x + factorX/2, c2y + factorY/2, 	17, 0x803b0bc0);
	lcd_high_DrawCircle(c2x + factorX/2, c2y + factorY/2, 	18, 0x803b0bc0);
	lcd_high_FillCircle(c2x, 			c2y, 				16, LCD_COLOR_BLACK);
	lcd_high_DrawCircle(c2x, 			c2y, 				17, 0xff3c8bc7);
	lcd_high_DrawCircle(c2x, 			c2y, 				18, 0xff3c8bc7);
	lcd_high_FillCircle(c2x, 			c2y, 			 	 9, 0xffffce23);
}
#endif

#if 0
static void draw_atlas_ui(void)
{
	uchar i, j, k;
	ulong col;

	// Draw a gradient line
	for(j = 0, k = 0; j < 6; j++)
	{
		col = LCD_COLOR_DARKGRAY;
		for(i = 0; i < 6; i++)
		{
			if(i)
				lcd_high_FillRect(460, k + 45 + j*72 + + i*12, 4, 2, col);
			else
				lcd_high_FillRect(460, k + 45 + j*72 + + i*12, 6, 4, col);
			col += 0x101010;
		}

		if(j == 2) k = 20;
	}

	// Mid point vertical blue line
	lcd_high_DrawHLine(450, 265, 20, 0xff3b6a97);
	lcd_high_DrawHLine(450, 266, 20, 0xff3b6a97);
	lcd_high_DrawHLine(450, 267, 20, 0xff3b6a97);

	// Side circles
	draw_atlas_circle(430,450, 0);
	draw_atlas_circle(430, 60, 1);

	// Top text
	lcd_high_SetFont(&Font24);
	lcd_high_SetTextColor(LCD_COLOR_WHITE);
	lcd_high_DisplayStringAt(50, 160, (uint8_t *)"BOOTLOADER", LEFT_MODE);
}
#endif

uchar bare_lcd_init(void)
{
	if(BSP_LCD_Init(0) != BSP_ERROR_NONE)
	{
		printf("== lcd init error ==\r\n");
		return 1;
	}

	lcd_high_SetFuncDriver(&LCD_Driver);
	lcd_high_SetLayer(0);

	BSP_LCD_GetXSize(0, &LCD_X_Size);
	BSP_LCD_GetYSize(0, &LCD_Y_Size);

	HAL_GPIO_WritePin(LCD_BL_CTRL_GPIO_PORT, LCD_BL_CTRL_PIN, GPIO_PIN_SET); 			// backlight on

	lcd_high_Clear(LCD_COLOR_BLACK);

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
	lcd_high_SetBackColor(LCD_COLOR_BLACK);
	lcd_high_SetFont(&Font16);

	for(i = 0, stat = batt_status; i < 12; i++)
	{
		if((stat & 0x8000) == 0x8000)
			lcd_high_SetTextColor(LCD_COLOR_LIGHTGREEN);
		else
			lcd_high_SetTextColor(LCD_COLOR_DARKGRAY);

		switch(i)
		{
			case 0:
				lcd_high_DisplayStringAt(LINE(line), BMS_FLAGS_X1,  (uchar *)"OCA", LEFT_MODE);
				break;
			case 1:
				lcd_high_DisplayStringAt(LINE(line), BMS_FLAGS_X2,  (uchar *)"TCA", LEFT_MODE);
				break;
			case 2:
				lcd_high_DisplayStringAt(LINE(line), BMS_FLAGS_X1,  (uchar *)"---", LEFT_MODE);
				break;
			case 3:
				lcd_high_DisplayStringAt(LINE(line), BMS_FLAGS_X2,  (uchar *)"OTA", LEFT_MODE);
				break;
			case 4:
				lcd_high_DisplayStringAt(LINE(line), BMS_FLAGS_X1,  (uchar *)"TDA", LEFT_MODE);
				break;
			case 5:
				lcd_high_DisplayStringAt(LINE(line), BMS_FLAGS_X2,  (uchar *)"---", LEFT_MODE);
				break;
			case 6:
				lcd_high_DisplayStringAt(LINE(line), BMS_FLAGS_X1,  (uchar *)"RCA", LEFT_MODE);
				break;
			case 7:
				lcd_high_DisplayStringAt(LINE(line), BMS_FLAGS_X2,  (uchar *)"RTA", LEFT_MODE);
				break;
			case 8:
				lcd_high_DisplayStringAt(LINE(line), BMS_FLAGS_X1,  (uchar *)"INIT", LEFT_MODE);
				break;
			case 9:
				lcd_high_DisplayStringAt(LINE(line), BMS_FLAGS_X2,  (uchar *)"DSG", LEFT_MODE);
				break;
			case 10:
				lcd_high_DisplayStringAt(LINE(line), BMS_FLAGS_X1,  (uchar *)"FC", LEFT_MODE);
				break;
			case 11:
				lcd_high_DisplayStringAt(LINE(line), BMS_FLAGS_X2,  (uchar *)"FD", LEFT_MODE);
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
		lcd_high_SetTextColor(LCD_COLOR_LIGHTGREEN);
	}
	else
		lcd_high_SetTextColor(LCD_COLOR_DARKGRAY);

	lcd_high_DisplayStringAt(LINE(line), BMS_FLAGS_X1,  (uchar *)"EC = ", LEFT_MODE);
	sprintf(buff, "%d", stat);
	lcd_high_DisplayStringAt(LINE(line), BMS_FLAGS_X2,  (uchar *)buff, LEFT_MODE);
}

void ui_proc_show_charge_msg(void)
{
    // Text attributes
	lcd_high_SetBackColor(LCD_COLOR_BLACK);
	lcd_high_SetFont(&Font16);

	if(charge_mode)
		lcd_high_SetTextColor(LCD_COLOR_RED);
	else
		lcd_high_SetTextColor(LCD_COLOR_BLACK);

	lcd_high_DisplayStringAt(LINE(28), 40,  (uchar *)"[Charging]", LEFT_MODE);
}

void ui_proc_show_soc(void)
{
	char 	buff[50];

	if(soc == 0xFF)
		return;

    // Text attributes
	lcd_high_SetBackColor(LCD_COLOR_BLACK);
	lcd_high_SetFont(&Font16);

	if(charge_mode)
		lcd_high_SetTextColor(LCD_COLOR_RED);
	else
		lcd_high_SetTextColor(LCD_COLOR_WHITE);

	lcd_high_DisplayStringAt(LINE(14), BMS_FLAGS_X1,  (uchar *)"SOC:", LEFT_MODE);
	sprintf(buff, "%d%%", soc);
	lcd_high_DisplayStringAt(LINE(14), BMS_FLAGS_X2,  (uchar *)buff, LEFT_MODE);
}

void ui_proc_show_charge_data(void)
{
	char 	buff[50];

    // Text attributes
	lcd_high_SetBackColor(LCD_COLOR_BLACK);
	lcd_high_SetTextColor(LCD_COLOR_WHITE);
	lcd_high_SetFont(&Font16);

	lcd_high_DisplayStringAt(LINE(26), 15,  (uchar *)"Pack:", LEFT_MODE);
	lcd_high_DisplayStringAt(LINE(27), 15,  (uchar *)"Curr:", LEFT_MODE);

	sprintf(buff, "%dmV", pack_volt);
	lcd_high_DisplayStringAt(LINE(26), 100,  (uchar *)buff, LEFT_MODE);

	sprintf(buff, "%dmA     ", pack_curr);
	lcd_high_DisplayStringAt(LINE(27), 100,  (uchar *)buff, LEFT_MODE);
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
	lcd_high_SetBackColor(LCD_COLOR_BLACK);
	lcd_high_SetTextColor(LCD_COLOR_GREEN);
	lcd_high_SetFont(&Font16);

	// Run timer
	if(keyb_timer == 0)
		keyb_timer = sys_timer;
	else if((keyb_timer + 4000) < sys_timer)
	{
		keyb_timer = sys_timer;
		keypad_proc_get(1);

		lcd_high_SetTextColor(LCD_COLOR_BLACK);
		lcd_high_DisplayStringAt(LINE(24), 15,  (uchar *)"                         ", LEFT_MODE);
		return;
	}

	lcd_high_DisplayStringAt(LINE(24), 15,  (uchar *)"KEY:", LEFT_MODE);

	sprintf(buff, "x=%d, y=%d", key >> 4, key & 0x0F);
	lcd_high_DisplayStringAt(LINE(24), 80,  (uchar *)buff, LEFT_MODE);
}

void ui_proc_bootup(void)
{
	// Init LCD
	if(bare_lcd_init() != 0)
		return;

	// Start the menu system
	menu_proc_init();
}

void ui_proc(void)
{
	// Menu system handles all UI painting
	menu_proc();
}

void ui_proc_init(void)
{
	ui_proc_bootup();
}
