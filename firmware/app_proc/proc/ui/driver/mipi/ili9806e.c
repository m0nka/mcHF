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
#include "ili9806e.h"

#include <stddef.h>

extern DSI_HandleTypeDef   hdsi;

// Retries for each DCS packet before giving up
#define MIPI_WRITE_RETRY		3

// Panel orientation, page 0 register 0x36 (0x00, 0x01, 0x02, 0x03)
// 0x00 = manufacturer default - the old 0x03 here only ever reached the
// panel on the rare boots where the write survived the LP corruption bug,
// which is what made the image randomly come up 180 deg rotated
#define ILI9806E_MADCTL			0x00

// Dropped/corrupted packet counter, shown on the debug terminal
static int mipi_err_cnt = 0;

static void mipi_change_page(unsigned long controller_id, unsigned char page)
{
	uint8_t lcd_reg_data[10];
	int i;

	lcd_reg_data[0] = (controller_id >> 24) & 0xFF;
	lcd_reg_data[1] = (controller_id >> 16) & 0xFF;
	lcd_reg_data[2] = (controller_id >>  8) & 0xFF;
	lcd_reg_data[3] = (controller_id >>  0) & 0xFF;

	lcd_reg_data[4] = page;

	for(i = 0; i < MIPI_WRITE_RETRY; i++)
	{
		if(HAL_DSI_LongWrite(&hdsi, 0, DSI_DCS_LONG_PKT_WRITE, 6, 0xFF, lcd_reg_data) == HAL_OK)
			return;

		mipi_err_cnt++;
		HAL_Delay(1);
	}

	printf("ili9806e: page %d select failed!\r\n", page);
}

// Write cmd + single byte data
static void mipi_write_short(uint8_t reg, uint8_t data)
{
	int i;

	for(i = 0; i < MIPI_WRITE_RETRY; i++)
	{
		if(HAL_DSI_ShortWrite(&hdsi, 0, DSI_DCS_SHORT_PKT_WRITE_P1, reg, data) == HAL_OK)
			return;

		mipi_err_cnt++;
		HAL_Delay(1);
	}

	printf("ili9806e: write reg %02x failed!\r\n", reg);
}

// Write parameterless cmd (sleep out, display on etc)
static void mipi_write_cmd(uint8_t cmd)
{
	int i;

	for(i = 0; i < MIPI_WRITE_RETRY; i++)
	{
		if(HAL_DSI_ShortWrite(&hdsi, 0, DSI_DCS_SHORT_PKT_WRITE_P0, cmd, 0) == HAL_OK)
			return;

		mipi_err_cnt++;
		HAL_Delay(1);
	}

	printf("ili9806e: write cmd %02x failed!\r\n", cmd);
}

// NOTE: do NOT attempt DCS reads (HAL_DSI_Read/BTA) on this panel while the
// video stream is running - the read always times out AND leaves the link in
// a state where the next transmitted packet corrupts the panel setup, which
// shows as a washed out image on every boot (bench-confirmed 2026-07-06)

int ILI9806ES_Init(unsigned long ColorCoding)
{
	//printf("ILI9806E_Init...\r\n");

	// Change to Page 1 CMD
	mipi_change_page(0xFF980604, 0x01);

	// Output    SDA
	mipi_write_short(0x08, 0x10);

	// set DE/VSYNC mode
	mipi_write_short(0x20, 0x00);

	// DE = 1 Active
	// 01 t800-03 gx09-01
	mipi_write_short(0x21, 0x01);

	// Porch control
	//mipi_write_short(0x25, 0x02);
	//mipi_write_short(0x26, 0x7F);
	//mipi_write_short(0x27, 0x02);
	//mipi_write_short(0x80, 0xC0);

	// Resolution setting 480 X 800
	mipi_write_short(0x30, 0x02);

	// Inversion setting - 0x00 column inversion per manufacturer init, the
	// VCOM/flicker values (0x53/0x55) are tuned for it; 0x02 (2-dot) causes
	// a ~28Hz whole-image flicker
	mipi_write_short(0x31, 0x00);

	// BT DDVDH DDVDL
	// 10,14,18 00	2XVCI
	mipi_write_short(0x40, 0x14);

	// avdd +5.2v,avee-5.2v 33
	mipi_write_short(0x41, 0x33);

	// VGL=DDVDH+VCIP -DDVDL,VGH=2DDVDL-VCIP
	mipi_write_short(0x42, 0x01);

	// SET VGH clamp level +15v
	mipi_write_short(0x43, 0x09);

	// SET VGL clamp level -12.8v
	mipi_write_short(0x44, 0x06);

	// VGLO_REG :-12.3V
	mipi_write_short(0x45, 0x0A);

	// VREG1 for positive Gamma
	// +4.5V
	mipi_write_short(0x50, 0x78);

	// VREG2 for negative Gamma
	// -4.5V
	mipi_write_short(0x51, 0x78);

	// VCOM
	mipi_write_short(0x52, 0x00);

	// Forward Flicker
	mipi_write_short(0x53, 0x3A);
	mipi_write_short(0x54, 0x00);

	// Backward Flicker
	mipi_write_short(0x55, 0x3A);

	mipi_write_short(0x57, 0x50);

	// SDTI
	mipi_write_short(0x60, 0x07);

	// CRTI
	mipi_write_short(0x61, 0x06);

	// EQTI
	mipi_write_short(0x62, 0x06);

	// PCTI
	mipi_write_short(0x63, 0x04);

	// Positive Gamma - FIXXXXXX
	mipi_write_short(0xA0, 0x00);
	mipi_write_short(0xA1, 0x13);
	mipi_write_short(0xA2, 0x19);
	mipi_write_short(0xA3, 0x0C);
	mipi_write_short(0xA4, 0x06);
	mipi_write_short(0xA5, 0x0A);
	mipi_write_short(0xA6, 0x06);
	mipi_write_short(0xA7, 0x04);
	mipi_write_short(0xA8, 0x09);
	mipi_write_short(0xA9, 0x08);
	mipi_write_short(0xAA, 0x12);
	mipi_write_short(0xAB, 0x06);
	mipi_write_short(0xAC, 0x0E);
	mipi_write_short(0xAD, 0x0E);
	mipi_write_short(0xAE, 0x09);
	mipi_write_short(0xAF, 0x00);

	// Negative Gamma
	mipi_write_short(0xC0, 0x00);
	mipi_write_short(0xC1, 0x0D);
	mipi_write_short(0xC2, 0x18);
	mipi_write_short(0xC3, 0x0D);
	mipi_write_short(0xC4, 0x06);
	mipi_write_short(0xC5, 0x09);
	mipi_write_short(0xC6, 0x07);
	mipi_write_short(0xC7, 0x05);
	mipi_write_short(0xC8, 0x08);
	mipi_write_short(0xC9, 0x0E);
	mipi_write_short(0xCA, 0x12);
	mipi_write_short(0xCB, 0x09);
	mipi_write_short(0xCC, 0x0E);
	mipi_write_short(0xCD, 0x0E);
	mipi_write_short(0xCE, 0x08);
	mipi_write_short(0xCF, 0x00);

	// Washout insurance - a packet arriving at the panel with a bad CRC is
	// silently discarded and the host never knows (readback is not possible
	// on this link). A lost write in the power/VREG/VCOM group shows as a
	// washed out image, so send that group a second time - a packet
	// corrupted twice in a row is very unlikely
	mipi_write_short(0x40, 0x14);
	mipi_write_short(0x41, 0x33);
	mipi_write_short(0x42, 0x01);
	mipi_write_short(0x43, 0x09);
	mipi_write_short(0x44, 0x06);
	mipi_write_short(0x45, 0x0A);
	mipi_write_short(0x50, 0x78);
	mipi_write_short(0x51, 0x78);
	mipi_write_short(0x52, 0x00);
	mipi_write_short(0x53, 0x3A);
	mipi_write_short(0x54, 0x00);
	mipi_write_short(0x55, 0x3A);
	mipi_write_short(0x57, 0x50);

	// Change to Page 6 CMD for GIP timing
	mipi_change_page(0xFF980604, 0x06);

	mipi_write_short(0x00, 0x20);
	mipi_write_short(0x01, 0x04);
	mipi_write_short(0x02, 0x00);
	mipi_write_short(0x03, 0x00);
	mipi_write_short(0x04, 0x01);
	mipi_write_short(0x05, 0x01);
	mipi_write_short(0x06, 0x88);
	mipi_write_short(0x07, 0x04);
	mipi_write_short(0x08, 0x01);
	mipi_write_short(0x09, 0x90);
	mipi_write_short(0x0A, 0x03);
	mipi_write_short(0x0B, 0x01);
	mipi_write_short(0x0C, 0x01);
	mipi_write_short(0x0D, 0x01);
	mipi_write_short(0x0E, 0x00);
	mipi_write_short(0x0F, 0x00);
	mipi_write_short(0x10, 0x55);
	mipi_write_short(0x11, 0x53);
	mipi_write_short(0x12, 0x01);
	mipi_write_short(0x13, 0x0D);
	mipi_write_short(0x14, 0x0D);
	mipi_write_short(0x15, 0x43);
	mipi_write_short(0x16, 0x0B);
	mipi_write_short(0x17, 0x00);
	mipi_write_short(0x18, 0x00);
	mipi_write_short(0x19, 0x00);
	mipi_write_short(0x1A, 0x00);
	mipi_write_short(0x1B, 0x00);
	mipi_write_short(0x1C, 0x00);
	mipi_write_short(0x1D, 0x00);
	mipi_write_short(0x20, 0x01);
	mipi_write_short(0x21, 0x23);
	mipi_write_short(0x22, 0x45);
	mipi_write_short(0x23, 0x67);
	mipi_write_short(0x24, 0x01);
	mipi_write_short(0x25, 0x23);
	mipi_write_short(0x26, 0x45);
	mipi_write_short(0x27, 0x67);
	mipi_write_short(0x30, 0x02);
	mipi_write_short(0x31, 0x22);
	mipi_write_short(0x32, 0x11);
	mipi_write_short(0x33, 0xAA);
	mipi_write_short(0x34, 0xBB);
	mipi_write_short(0x35, 0x66);
	mipi_write_short(0x36, 0x00);
	mipi_write_short(0x37, 0x22);
	mipi_write_short(0x38, 0x22);
	mipi_write_short(0x39, 0x22);
	mipi_write_short(0x3A, 0x22);
	mipi_write_short(0x3B, 0x22);
	mipi_write_short(0x3C, 0x22);
	mipi_write_short(0x3D, 0x22);
	mipi_write_short(0x3E, 0x22);
	mipi_write_short(0x3F, 0x22);
	mipi_write_short(0x40, 0x22);
	mipi_write_short(0x52, 0x10);
	mipi_write_short(0x53, 0x12);
	mipi_write_short(0x54, 0x13);

	// Change to Page 7 CMD for GIP timing
	mipi_change_page(0xFF980604, 0x07);

	mipi_write_short(0x17, 0x32);
	mipi_write_short(0x18, 0x1D);
	mipi_write_short(0x26, 0xB2);
	mipi_write_short(0x02, 0x77);
	mipi_write_short(0xE1, 0x79);
	mipi_write_short(0xB3, 0x10);

	// Change to Page 0 CMD for Normal command
	mipi_change_page(0xFF980604, 0x00);

	// Display rotation - written twice as insurance, readback verification
	// is not possible on this link (see note above) and a packet corrupted
	// twice in a row is very unlikely
	mipi_write_short(0x36, ILI9806E_MADCTL);
	mipi_write_short(0x36, ILI9806E_MADCTL);

	// 24bit colour
	mipi_write_short(0x3A, 0x70);

	// Backlight control off
	//mipi_write_short(0x53, 0x00);

	// Sleep out
	mipi_write_cmd(0x11);
	HAL_Delay(120);

	// Display on
	mipi_write_cmd(0x29);
	HAL_Delay(25);

	if(mipi_err_cnt)
		printf("ili9806e: %d packet retries during init\r\n", mipi_err_cnt);

	//printf("ILI9806E_Init done.\r\n");
	return 0;
}
