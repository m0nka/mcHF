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
//
// ToDo: File duplicated in radio firmware - fix!
//
#include "main.h"
#include "mchf_pro_board.h"

//#ifdef CONTEXT_BMS

#include "shared_i2c.h"

#include "bq40z80.h"

ushort bq40z80_regs[0x1C];

void bq40z80_delay(ulong delay)
{
	#if defined (RADIO)
	osDelay(delay);
	#endif

	#if defined (BOOTLOADER)
	HAL_Delay(delay);
	#endif
}

uchar bq40z80_mac_read_block(ushort cmd, uchar *buf, uchar len)
{
	ulong err;
	uchar t_buf[40];
	//uchar t_len;
	//uchar i;

	if(buf ==  NULL)
		return 1;

	t_buf[0] = (uchar)(cmd >> 8);
	t_buf[1] = (uchar)cmd;

	err = shared_i2c_write_reg(0x16, 0x00, t_buf, 2);
	if(err != 0)
	{
		printf("write block %d\r\n", (int)err);
		return 2;
	}

	bq40z80_delay(100);

	err = shared_i2c_read_reg(0x16, 0x00, t_buf, 36);
	if(err != 0)
	{
		printf("read block %d\r\n", (int)err);
		return 3;
	}

	print_hex_array(t_buf, 36);

	//t_len = ret;

	/* ret contains number of data bytes in gauge's response*/
	//fg_print_buf("mac_read_block", t_buf, t_len);

	//for (i = 0; i < t_len - 2; i++)
	//	buf[i] = t_buf[i+2];

	return 0;
}

uchar bq40z80_write_16bit_reg(uchar reg, ushort val)
{
	ulong err;
	uchar data[2];

	data[0] = (uchar)(val >> 8);
	data[1] = (uchar)(val);

	err = shared_i2c_write_reg(0x16, reg, data, 2);
	if(err != 0)
	{
		printf("write reg %02x, err %d\r\n", (int)reg, (int)err);
		return 2;
	}

	return 0;
}

uchar bq40z80_read_16bit_reg(uchar reg, ushort *val)
{
	ulong err;
	uchar data[2];

	if(val ==  NULL)
		return 1;

	err = shared_i2c_read_reg(0x17, reg, data, 2);
	if(err != 0)
	{
		//printf("read reg %02x, err %d\r\n", (int)reg, (int)err);
		return 2;
	}

	*val = (data[1] << 8)|data[0];

	return 0;
}

uchar bq40z80_read_fw_ver(void)
{
	uchar buf[100];

	if(bq40z80_write_16bit_reg(0x44, 0x0006) != 0)
		return 1;

	bq40z80_delay(2);

	if(bq40z80_mac_read_block(0x0044, buf, 11) != 0)
		return 2;

	return 0;
}

void bq40z80_read_all_regs(void)
{
	ushort val = 0;
	uchar i;

	printf("dump bms registers...\r\n");

	for(i = 0; i < 0x1C; i++)
	{
		bq40z80_regs[i] = 0xAAAA;
		if(bq40z80_read_16bit_reg(i, &val) == 0)
		{
			bq40z80_regs[i] = val;
			val = 0;
		}

		bq40z80_delay(50);
	}

	printf("dump bms registers done\r\n");
}

uchar bms_loc_init = 0;

uchar bq40z80_read_soc(void)
{
	ushort val = 0;

	if(!bms_loc_init)
		return 0xFF;

	// SOC (relative 0x0D, absolute 0x0E)
	if(bq40z80_read_16bit_reg(0x0D, &val) == 0)
	{
		//printf("soc: %d%% \r\n", val);
		return val;
	}

	return 0xFF;
}

ushort bq40z80_read_runtime(void)
{
	ushort val = 0;

	if(!bms_loc_init)
		return 0xFFFF;

	// Runtime to empty(0x11, 0x12)
	if(bq40z80_read_16bit_reg(0x11, &val) == 0)
		return val;

	if(val > 65000)
		return 0;

	return 0xFFFF;
}

ushort bq40z80_read_status(void)
{
	ushort val = 0;

	if(!bms_loc_init)
		return 0xFFFF;

	if(bq40z80_read_16bit_reg(0x16, &val) == 0)
		return val;

	return 0xFFFF;
}

short bq40z80_read_current(void)
{
	ushort curr;

	if(!bms_loc_init)
		return 0;

	if(bq40z80_read_16bit_reg(0x0A, &curr) == 0)
	{
		// Add calib factor
		//curr -= 250;

		//printf("curr: %dmA \r\n", curr);
		return curr;
	}

	return 0;
}

ushort bq40z80_read_pack_voltage(void)
{
	ushort volt;

	if(!bms_loc_init)
		return 0;

	if(bq40z80_read_16bit_reg(0x09, &volt) == 0)
	{
		//printf("pack: %dmV \r\n", volt);
		return volt;
	}

	return 0;
}

// -----------------------------------------------------------------------
// MAC block write to ManufacturerBlockAccess(0x44)
// -----------------------------------------------------------------------
uchar bq40z80_mac_write(ushort cmd, uchar *data, uchar len)
{
	uchar t_buf[40];
	uchar i;

	if(len > BQ40Z80_DF_ROW)
		return 1;

	t_buf[0] = len + 2;				// SMBus block byte count
	t_buf[1] = (uchar)(cmd);			// MAC command word, little endian
	t_buf[2] = (uchar)(cmd >> 8);

	for(i = 0; i < len; i++)
		t_buf[3 + i] = data[i];

	if(shared_i2c_write_reg(0x16, 0x44, t_buf, (len + 3)) != 0)
		return 2;

	return 0;
}

// -----------------------------------------------------------------------
// MAC block read from ManufacturerBlockAccess(0x44)
// -----------------------------------------------------------------------
uchar bq40z80_mac_read(ushort cmd, uchar *buf, uchar len)
{
	uchar t_buf[40];

	if((buf == NULL)||(len > BQ40Z80_DF_ROW))
		return 1;

	if(bq40z80_mac_write(cmd, NULL, 0) != 0)
		return 2;

	bq40z80_delay(10);

	if(shared_i2c_read_reg(0x16, 0x44, t_buf, (len + 3)) != 0)
		return 3;

	// Check the command word echo
	if((t_buf[1] != (uchar)(cmd))||(t_buf[2] != (uchar)(cmd >> 8)))
		return 4;

	memcpy(buf, (t_buf + 3), len);

	return 0;
}

// -----------------------------------------------------------------------
// Unseal the gauge (unlock MAC access)
// -----------------------------------------------------------------------
uchar bq40z80_unseal(void)
{
	if(bq40z80_write_16bit_reg(0x00, 0x0414) != 0)
		return 1;

	bq40z80_delay(20);

	if(bq40z80_write_16bit_reg(0x00, 0x3672) != 0)
		return 2;

	return 0;
}

// -----------------------------------------------------------------------
// Seal the gauge (lock bms)
// -----------------------------------------------------------------------
uchar bq40z80_seal(void)
{
	if(bq40z80_write_16bit_reg(0x00, 0x0030) != 0)
		return 1;

	return 0;
}

// -----------------------------------------------------------------------
// Promote from unsealed to full access (needed for DF writes)
// -----------------------------------------------------------------------
uchar bq40z80_full_access(void)
{
	if(bq40z80_write_16bit_reg(0x00, BQ40Z80_FA_KEY_W0) != 0)
		return 1;

	bq40z80_delay(20);

	if(bq40z80_write_16bit_reg(0x00, BQ40Z80_FA_KEY_W1) != 0)
		return 2;

	bq40z80_delay(20);

	return 0;
}

// -----------------------------------------------------------------------
// DeviceReset(0x0041), gauge reboots and re-reads data flash
// -----------------------------------------------------------------------
uchar bq40z80_device_reset(void)
{
	if(bq40z80_write_16bit_reg(0x00, 0x0041) != 0)
		return 1;

	return 0;
}

// -----------------------------------------------------------------------
// Read ManufacturingStatus() via MAC 0x0057
// -----------------------------------------------------------------------
uchar bq40z80_read_mfg_status(ushort *val)
{
	uchar buf[2];

	if(val == NULL)
		return 1;

	if(bq40z80_mac_read(0x0057, buf, 2) != 0)
		return 2;

	*val = (buf[1] << 8)|buf[0];

	return 0;
}

// -----------------------------------------------------------------------
// Toggle GAUGE_EN (MAC 0x0021)
// -----------------------------------------------------------------------
uchar bq40z80_gauging_toggle(void)
{
	if(bq40z80_write_16bit_reg(0x00, 0x0021) != 0)
		return 1;

	return 0;
}

// -----------------------------------------------------------------------
// Toggle FET_EN (MAC 0x0022)
// -----------------------------------------------------------------------
uchar bq40z80_fet_en_toggle(void)
{
	if(bq40z80_write_16bit_reg(0x00, 0x0022) != 0)
		return 1;

	return 0;
}

// -----------------------------------------------------------------------
// Read a data flash row (needs unsealed mode)
// -----------------------------------------------------------------------
uchar bq40z80_df_read_row(ushort addr, uchar *buf, uchar len)
{
	if((addr < BQ40Z80_DF_START)||(addr > BQ40Z80_DF_END))
		return 1;

	return bq40z80_mac_read(addr, buf, len);
}

// -----------------------------------------------------------------------
void bq40z80_init(void)
{
	//ulong err;
	//ushort val = 0;

	// Do we need init ?
	#ifndef CONTEXT_AUDIO
	if(shared_i2c_init() != 0)
		printf("i2c init err 1!\r\n");
	#endif

	if(shared_i2c_is_ready(0x17, 10) != 0)
		return;

	#if 0
	// Reset(0x41, 0x12)
	if(bq40z80_write_16bit_reg(0x41, 0x0000) != 0)
		return;

	HAL_Delay(500);
	power_off_x(0);
	#endif

	bms_loc_init = 1;
	//printf("== bms ready ==\r\n");

	// Read battery status: 0x0040 - no cells
	//
	//if(bq40z80_read_16bit_reg(0x16, &val) == 0)
	//	printf("stat: %04x \r\n", val);

	//if(bq40z80_read_16bit_reg(0x18, &val) == 0)
	//	printf("dsgn capa: %d \r\n", val);

	//bq40z80_read_all_regs();

	//if(bq40z80_read_16bit_reg(0x08, &val) == 0)
	//	printf("temp: %d deg C(0x%04x)\r\n", (val/10 - 273), val);

	//if(bq40z80_read_16bit_reg(0x09, &val) == 0)
	//	printf("batt: %dmV \r\n", val);

	//printf("soc:  %d%% \r\n", bq40z80_read_soc());

	// Runtime to empty(0x11, 0x12)
	//val = bq40z80_read_runtime();
	//if(val != 0xFFFF)
	//	printf("runt: %dh%dm \r\n", val/60, val%60);

	//uchar buf[100];
	//if(bq40z80_mac_read_block(0x0075, buf, 24) == 0)
	//	printf("fw ver \r\n");

	//bq40z80_read_fw_ver();

	/*for(uchar i = 0; i < 0x1C; i++)
	{
		if(bq40z80_regs[i] != 0xAAAA)
			printf("%02x:%04x \r\n", i, bq40z80_regs[i]);

		osDelay(100);
	}*/

	//printf("batt status:%04x \r\n", bq40z80_regs[0x15]);
}


//#endif
