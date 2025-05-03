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

ushort bq40z80_regs[0x1C];

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

	osDelay(100);

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

	osDelay(2);

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

		osDelay(50);
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

void bq40z80_init(void)
{
	//ulong err;
	ushort val = 0;

	// Do we need init ?
	#ifndef CONTEXT_AUDIO
	if(shared_i2c_init() != 0)
		printf("i2c init err 1!\r\n");
	#endif

	if(shared_i2c_is_ready(0x17, 10) != 0)
		return;

	bms_loc_init = 1;
	//printf("== bms ready ==\r\n");

	// Read battery status: 0x0040 - no cells
	//
	//if(bq40z80_read_16bit_reg(0x16, &val) == 0)
	//	printf("stat: %04x \r\n", val);

	//if(bq40z80_read_16bit_reg(0x18, &val) == 0)
	//	printf("dsgn capa: %04x \r\n", val);

	//bq40z80_read_all_regs();

	//if(bq40z80_read_16bit_reg(0x08, &val) == 0)
	//	printf("temp: %d deg C(0x%04x)\r\n", (val/10 - 273), val);

	//if(bq40z80_read_16bit_reg(0x09, &val) == 0)
	//	printf("volt: %dmV \r\n", val);

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


#endif
