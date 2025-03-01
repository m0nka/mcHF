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

#include "stm32h7xx_hal.h"
//#include "otm8009a.h"
#include "st7701.h"
#include <stddef.h>

int32_t ST7701_Init(OTM8009A_Object_t *pObj, uint32_t ColorCoding, uint32_t Orientation)
{
	//printf("ST7701_Init\r\n");
	return 0;
}

int32_t ST7701_DeInit(OTM8009A_Object_t *pObj)
{
	//printf("ST7701_DeInit\r\n");
  return 0;
}

int32_t ST7701_ReadID(OTM8009A_Object_t *pObj, uint32_t *Id)
{
	//printf("ST7701_ReadID\r\n");
	return 0;
}

int32_t ST7701_SetBrightness(OTM8009A_Object_t *pObj, uint32_t Brightness)
{
	//printf("ST7701_SetBrightness\r\n");
  return 0;
}

int32_t ST7701_GetBrightness(OTM8009A_Object_t *pObj, uint32_t *Brightness)
{
	//printf("ST7701_GetBrightness\r\n");
  return 0;
}

int32_t ST7701_DisplayOn(OTM8009A_Object_t *pObj)
{
	//printf("ST7701_DisplayOn\r\n");
  return 0;
}

int32_t ST7701_DisplayOff(OTM8009A_Object_t *pObj)
{
	//printf("ST7701_DisplayOff\r\n");
  return 0;
}

int32_t ST7701_SetOrientation(OTM8009A_Object_t *pObj, uint32_t Orientation)
{
	//printf("ST7701_SetOrientation\r\n");
  return 0;
}

int32_t ST7701_GetOrientation(OTM8009A_Object_t *pObj, uint32_t *Orientation)
{
	//printf("ST7701_GetOrientation\r\n");
  *Orientation = 0;
  return 0;
}

int32_t ST7701_GetXSize(OTM8009A_Object_t *pObj, uint32_t *Xsize)
{
	//printf("ST7701_GetXSize\r\n");
  *Xsize = 480;
  return 0;
}

int32_t ST7701_GetYSize(OTM8009A_Object_t *pObj, uint32_t *Ysize)
{
	//printf("ST7701_GetYSize\r\n");
  *Ysize = 480;
  return 0;
}

int32_t ST7701_SetCursor(OTM8009A_Object_t *pObj, uint32_t Xpos, uint32_t Ypos)
{
	//printf("ST7701_SetCursor\r\n");
  return 0;
}

int32_t ST7701_DrawBitmap(OTM8009A_Object_t *pObj, uint32_t Xpos, uint32_t Ypos, uint8_t *pBmp)
{
	//printf("ST7701_DrawBitmap\r\n");
  return 0;
}

int32_t ST7701_FillRGBRect(OTM8009A_Object_t *pObj, uint32_t Xpos, uint32_t Ypos, uint8_t *pData, uint32_t Width, uint32_t Height)
{
	//printf("ST7701_FillRGBRect\r\n");
  return 0;
}

int32_t ST7701_DrawHLine(OTM8009A_Object_t *pObj, uint32_t Xpos, uint32_t Ypos, uint32_t Length, uint32_t Color)
{
	//printf("ST7701_DrawHLine\r\n");
  return 0;
}

int32_t ST7701_DrawVLine(OTM8009A_Object_t *pObj, uint32_t Xpos, uint32_t Ypos, uint32_t Length, uint32_t Color)
{
	//printf("ST7701_DrawVLine\r\n");
  return 0;
}

int32_t ST7701_FillRect(OTM8009A_Object_t *pObj, uint32_t Xpos, uint32_t Ypos, uint32_t Width, uint32_t Height, uint32_t Color)
{
	//printf("ST7701_FillRect\r\n");
  return 0;
}

int32_t ST7701_GetPixel(OTM8009A_Object_t *pObj, uint32_t Xpos, uint32_t Ypos, uint32_t *Color)
{
	//printf("ST7701_GetPixel\r\n");
  return 0;
}

int32_t ST7701_SetPixel(OTM8009A_Object_t *pObj, uint32_t Xpos, uint32_t Ypos, uint32_t Color)
{
	//printf("ST7701_SetPixel\r\n");
  return 0;
}

static int32_t ST7701_ReadRegWrap(void *Handle, uint16_t Reg, uint8_t* pData, uint16_t Length)
{
	//printf("ST7701_ReadRegWrap\r\n");
  return 0;
}

static int32_t ST7701_WriteRegWrap(void *Handle, uint16_t Reg, uint8_t *pData, uint16_t Length)
{
	//printf("ST7701_WriteRegWrap\r\n");
  return 0;
}

static int32_t ST7701_IO_Delay(OTM8009A_Object_t *pObj, uint32_t Delay)
{
	//printf("ST7701_IO_Delay\r\n");
  return 0;
}

OTM8009A_LCD_Drv_t   ST7701_LCD_Driver =
{
  ST7701_Init,
  ST7701_DeInit,
  ST7701_ReadID,
  ST7701_DisplayOn,
  ST7701_DisplayOff,
  ST7701_SetBrightness,
  ST7701_GetBrightness,
  ST7701_SetOrientation,
  ST7701_GetOrientation,
  ST7701_SetCursor,
  ST7701_DrawBitmap,
  ST7701_FillRGBRect,
  ST7701_DrawHLine,
  ST7701_DrawVLine,
  ST7701_FillRect,
  ST7701_GetPixel,
  ST7701_SetPixel,
  ST7701_GetXSize,
  ST7701_GetYSize,
};

// Vendor specific/not documented
static const uint8_t cmd2_bk1_e0[] = {0x00, 0x00, 0x02};
static const uint8_t cmd2_bk1_e1[] = {0x08, 0x00, 0x0A, 0x00, 0x07, 0x00, 0x09, 0x00, 0x00, 0x33, 0x33};
static const uint8_t cmd2_bk1_e2[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
static const uint8_t cmd2_bk1_e3[] = {0x00, 0x00, 0x33, 0x33};
static const uint8_t cmd2_bk1_e4[] = {0x44, 0x44};
static const uint8_t cmd2_bk1_e5[] = {0x0E, 0x60, 0xA0, 0xA0, 0x10, 0x60, 0xA0, 0xA0, 0x0A, 0x60, 0xA0, 0xA0, 0x0C, 0x60, 0xA0, 0xA0};
static const uint8_t cmd2_bk1_e6[] = {0x00, 0x00, 0x33, 0x33};
static const uint8_t cmd2_bk1_e7[] = {0x44, 0x44};
static const uint8_t cmd2_bk1_e8[] = {0x0D, 0x60, 0xA0, 0xA0, 0x0F, 0x60, 0xA0, 0xA0, 0x09, 0x60, 0xA0, 0xA0, 0x0B, 0x60, 0xA0, 0xA0};
static const uint8_t cmd2_bk1_eb[] = {0x02, 0x01, 0xE4, 0xE4, 0x44, 0x00, 0x40};
static const uint8_t cmd2_bk1_ec[] = {0x02, 0x01};
static const uint8_t cmd2_bk1_ed[] = {0xAB, 0x89, 0x76, 0x54, 0x01, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x10, 0x45, 0x67, 0x98, 0xBA};

const uint8_t sleep_out[] = {OTM8009A_CMD_SLPOUT, 0x00};

extern DSI_HandleTypeDef   hdsi;

//#define LOC_DEBUG

void mipi_change_page(unsigned char page)
{
	static const uint8_t lcd_reg_data00[] =   {0x77, 0x01, 0x00, 0x00, 0x00};
	uint8_t lcd_reg_data[10];

	memcpy(lcd_reg_data, lcd_reg_data00, 4);
	lcd_reg_data[4] = page;

	HAL_DSI_LongWrite(&hdsi, 0, DSI_DCS_LONG_PKT_WRITE, 6, 0xFF, lcd_reg_data);

	#ifdef LOC_DEBUG
	lcd_reg_data[0] = 0xFF;
	memcpy(lcd_reg_data + 1, lcd_reg_data00, 4);
	lcd_reg_data[5] = page;
	print_hex_array(lcd_reg_data,6);
	#endif
}

// Write cmd + single byte data
void mipi_write_short(uint8_t reg, uint8_t data)
{
	HAL_DSI_ShortWrite(&hdsi, 0, DSI_DCS_SHORT_PKT_WRITE_P1, reg, data);

	#ifdef LOC_DEBUG
	uint8_t lcd_reg_data[2];
	lcd_reg_data[0] = reg;
	lcd_reg_data[1] = data;
	print_hex_array(lcd_reg_data,2);
	#endif
}

void mipi_write_long(uchar cmd, const uchar * data, ushort size)
{
	HAL_DSI_LongWrite(&hdsi, 0, DSI_DCS_LONG_PKT_WRITE, size, cmd, (unsigned char *)data);

	#ifdef LOC_DEBUG
	uint8_t lcd_reg_data[100];
	lcd_reg_data[0] = cmd;
	memcpy(lcd_reg_data + 1, data, size);
	print_hex_array(lcd_reg_data,size + 1);
	#endif
}

int ST7701S_Init(unsigned long ColorCoding)
{
	static const uint8_t lcd_reg_data03[] = {0x02, 0x00, 0x00};
	static const uint8_t lcd_reg_data04[] = {0x00, 0x0D, 0x14, 0x0D, 0x10, 0x05, 0x02, 0x08, 0x08, 0x1E, 0x05, 0x13, 0x11, 0xA3, 0x29, 0x18};
	static const uint8_t lcd_reg_data05[] = {0x00, 0x0C, 0x14, 0x0C, 0x10, 0x05, 0x03, 0x08, 0x07, 0x20, 0x05, 0x13, 0x11, 0xA4, 0x29, 0x18};

	unsigned char buff[40];

	//printf("ST7701_Init...\r\n");

	// After reset delay	-	 ToDo: check if kernel running, if yes, use OS delay!
	HAL_Delay(200);

	//mipi_write_long(  MIPI_DCS_EXIT_SLEEP_MODE, buff, 0);
	//DSI_IO_WriteCmd(0, (uint8_t *)sleep_out);
	mipi_write_short(MIPI_DCS_EXIT_SLEEP_MODE, 0);

	// After sleep delay	-	 ToDo: check if kernel running, if yes, use OS delay!
	HAL_Delay(120);

	// -------------------------------------------------------------------------------
	// -------------------------------------------------------------------------------
	// -------------------------------------------------------------------------------
	// Change to Page Command2_BK0 (0x10)
	mipi_change_page(DSI_CMD2BK0_SEL);

	// -------------------------------------------------------------------------------
	// Display Line setting (0xC0)
	//
	// EX:(C0:0x6b,0x00) ((0x6b+1) x 8) = 864;
	// EX:(C0:0xe9,0x03) ((0x69+1) x 8) + ( 3x2 ) = 854
	//
	buff[0] = DSI_CMD2_BK0_LNESET_B0;
	buff[1] = DSI_CMD2_BK0_LNESET_B1;
	mipi_write_long(DSI_CMD2_BK0_LNESET, buff, 2);

	// -------------------------------------------------------------------------------
	// Porch control (0xC1)
	//
	buff[0] = DSI_CMD2_BK0_PORCTRL_B0;
	buff[1] = DSI_CMD2_BK0_PORCTRL_B1;
	mipi_write_long(DSI_CMD2_BK0_PORCTRL, buff, 2);

	// -------------------------------------------------------------------------------
	// Inversion selection and frame rate control
	//
	// Densitron: 	0x37 0x08
	// Linux:		0x37 0x06
	//
	buff[0] = DSI_CMD2_BK0_INVSEL_B0;
	buff[1] = DSI_CMD2_BK0_INVSEL_B1;
	mipi_write_long(0xC2, buff, 2);

	// Rotate LCD 180deg for v 0.8.4
	mipi_write_short(0xC7, 0x04);

	mipi_write_short(0xCC, 0x10);
	mipi_write_long(0xC3, &lcd_reg_data03[0], sizeof(lcd_reg_data03));
	mipi_write_long(0xB0, &lcd_reg_data04[0], sizeof(lcd_reg_data04));
	mipi_write_long(0xB1, &lcd_reg_data05[0], sizeof(lcd_reg_data05));
	//printf("== 88 ==\r\n");
	// -------------------------------------------------------------------------------
	// -------------------------------------------------------------------------------
	// -------------------------------------------------------------------------------
	// Change to Command2_BK1 (0x11)
	mipi_change_page(DSI_CMD2BK1_SEL);

	mipi_write_short(0xB0, 0x6C);
	mipi_write_short(0xB1, 0x3F);
	mipi_write_short(0xB2, 0x07);
	mipi_write_short(0xCC, 0x10);
	mipi_write_short(0xB3, 0x80);
	mipi_write_short(0xB5, 0x47);
//	mipi_write_short(0xB7, DSI_CMD2_BK1_PWCTLR1_SET);	// 0x85
	mipi_write_short(0xB8, 0x20);
	mipi_write_short(0xB9, 0x10);
	mipi_write_short(0xC1, 0x78);
	mipi_write_short(0xC2, 0x78);
	mipi_write_short(0xD0, 0x88);
	//printf("== 99 ==\r\n");
	// -------------------------------------------------------------------------------
	// Vendor specific/not documented
	mipi_write_long(0xE0, &cmd2_bk1_e0[0], sizeof(cmd2_bk1_e0));
	mipi_write_long(0xE1, &cmd2_bk1_e1[0], sizeof(cmd2_bk1_e1));
	mipi_write_long(0xE2, &cmd2_bk1_e2[0], sizeof(cmd2_bk1_e2));
	mipi_write_long(0xE3, &cmd2_bk1_e3[0], sizeof(cmd2_bk1_e3));
	mipi_write_long(0xE4, &cmd2_bk1_e4[0], sizeof(cmd2_bk1_e4));
	mipi_write_long(0xE5, &cmd2_bk1_e5[0], sizeof(cmd2_bk1_e5));
	mipi_write_long(0xE6, &cmd2_bk1_e6[0], sizeof(cmd2_bk1_e6));
	mipi_write_long(0xE7, &cmd2_bk1_e7[0], sizeof(cmd2_bk1_e7));
	mipi_write_long(0xE8, &cmd2_bk1_e8[0], sizeof(cmd2_bk1_e8));
	mipi_write_long(0xEB, &cmd2_bk1_eb[0], sizeof(cmd2_bk1_eb));
	mipi_write_long(0xEC, &cmd2_bk1_ec[0], sizeof(cmd2_bk1_ec));
	mipi_write_long(0xED, &cmd2_bk1_ed[0], sizeof(cmd2_bk1_ed));

	// -------------------------------------------------------------------------------
	// -------------------------------------------------------------------------------
	// -------------------------------------------------------------------------------
	// Change to Normal command (0x00)
	mipi_change_page(DSI_CMD2BKX_SEL_NONE);

	// 24BIT
	if(ColorCoding == DSI_RGB565)
		mipi_write_short(0x3A, 0x50);
	else
		mipi_write_short(0x3A, 0x70);

	// Rotate LCD 180deg for v 0.8.4
	mipi_write_short(0x36, 0x10);

    mipi_write_short(OTM8009A_CMD_DISPON, 0);

	//printf("ST7701_Init done.\r\n");
	return 0;
}
