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

ushort bq40z80_regs[0x1C];
uchar  bms_loc_init = 0;

extern struct BMSState	bmss;

//*----------------------------------------------------------------------------
//* Function Name       : bq40z80_sbs_read_block
//* Object              :
//* Notes    			: read block in normal mode
//* Notes   			:
//* Notes    			:
//* Context    			: CONTEXT_BMS
//*----------------------------------------------------------------------------
uchar bq40z80_sbs_read_block(ushort cmd, uchar *buf, uchar len)
{
	ulong err;
	uchar t_buf[40];

	if(buf ==  NULL)
		return 1;

	if(len > (sizeof(t_buf) - 1))
		return 2;

	t_buf[0] = 0x00;
	t_buf[1] = cmd;

	err = shared_i2c_write_reg(0x16, 0x00, t_buf, 1);
	if(err != 0)
	{
		printf("write block %d\r\n", (int)err);
		return 2;
	}

	osDelay(100);

	err = shared_i2c_read_reg(0x16, cmd, t_buf, (len + 1));
	if(err != 0)
	{
		printf("read block %d\r\n", (int)err);
		return 3;
	}

	//printf("read block size 0x%02x\r\n", (int)t_buf[0]);
	//print_hex_array(t_buf + 1, 32);

	memcpy(buf, t_buf + 1, len);

	return 0;
}

//*----------------------------------------------------------------------------
//* Function Name       : bq40z80_mac_read_block
//* Object              :
//* Notes    			: read block in MAC mode
//* Notes   			:
//* Notes    			:
//* Context    			: CONTEXT_BMS
//*----------------------------------------------------------------------------
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

//*----------------------------------------------------------------------------
//* Function Name       : bq40z80_write_16bit_reg
//* Object              :
//* Notes    			:
//* Notes   			:
//* Notes    			:
//* Context    			: CONTEXT_BMS
//*----------------------------------------------------------------------------
uchar bq40z80_write_16bit_reg(uchar reg, ushort val)
{
	ulong err;
	uchar data[2];

	data[1] = (uchar)(val >> 8);
	data[0] = (uchar)(val);

	err = shared_i2c_write_reg(0x16, reg, data, 2);
	if(err != 0)
	{
		printf("write reg %02x, err %d\r\n", (int)reg, (int)err);
		return 2;
	}

	return 0;
}

//*----------------------------------------------------------------------------
//* Function Name       : bq40z80_read_16bit_reg
//* Object              :
//* Notes    			:
//* Notes   			:
//* Notes    			:
//* Context    			: CONTEXT_BMS
//*----------------------------------------------------------------------------
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

//*----------------------------------------------------------------------------
//* Function Name       : bq40z80_shutdown
//* Object              :
//* Notes    			:
//* Notes   			:
//* Notes    			:
//* Context    			: CONTEXT_BMS
//*----------------------------------------------------------------------------
uchar bq40z80_shutdown(void)
{
	if(bq40z80_write_16bit_reg(0x00, 0x0010) != 0)
		return 1;

	osDelay(20);

	if(bq40z80_write_16bit_reg(0x00, 0x0010) != 0)
		return 2;

	printf("== shutdown cmd ok == \r\n");
	return 0;
}

//*----------------------------------------------------------------------------
//* Function Name       : bq40z80_unseal
//* Object              :
//* Notes    			: unlock MAC access
//* Notes   			:
//* Notes    			:
//* Context    			: CONTEXT_BMS
//*----------------------------------------------------------------------------
uchar bq40z80_unseal(void)
{
	if(bq40z80_write_16bit_reg(0x00, 0x0414) != 0)
		return 1;

	osDelay(20);

	if(bq40z80_write_16bit_reg(0x00, 0x3672) != 0)
		return 2;

	//printf("== unseal cmd ok == \r\n");
	return 0;
}

//*----------------------------------------------------------------------------
//* Function Name       : bq40z80_seal
//* Object              :
//* Notes    			: lock bms
//* Notes   			:
//* Notes    			:
//* Context    			: CONTEXT_BMS
//*----------------------------------------------------------------------------
uchar bq40z80_seal(void)
{
	if(bq40z80_write_16bit_reg(0x00, 0x0030) != 0)
		return 1;

	//printf("== seal cmd ok == \r\n");
	return 0;
}

//*----------------------------------------------------------------------------
//* Function Name       : bq40z80_read_fw_ver
//* Object              :
//* Notes    			:
//* Notes   			:
//* Notes    			:
//* Context    			: CONTEXT_BMS
//*----------------------------------------------------------------------------
uchar bq40z80_read_fw_ver(void)
{
	uchar buf[100];

	if(bq40z80_write_16bit_reg(0x44, 0x0002) != 0)
		return 1;

	osDelay(2);

	if(bq40z80_mac_read_block(0x0044, buf, 11) != 0)
		return 2;

	//print_hex_array(buf, 11);

	return 0;
}

//*----------------------------------------------------------------------------
//* Function Name       : bq40z80_read_all_regs
//* Object              :
//* Notes    			:
//* Notes   			:
//* Notes    			:
//* Context    			: CONTEXT_BMS
//*----------------------------------------------------------------------------
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

//*----------------------------------------------------------------------------
//* Function Name       : bq40z80_read_soc
//* Object              :
//* Notes    			:
//* Notes   			:
//* Notes    			:
//* Context    			: CONTEXT_BMS
//*----------------------------------------------------------------------------
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

//*----------------------------------------------------------------------------
//* Function Name       : bq40z80_read_runtime
//* Object              :
//* Notes    			:
//* Notes   			:
//* Notes    			:
//* Context    			: CONTEXT_BMS
//*----------------------------------------------------------------------------
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

//*----------------------------------------------------------------------------
//* Function Name       : bq40z80_read_status
//* Object              :
//* Notes    			:
//* Notes   			:
//* Notes    			:
//* Context    			: CONTEXT_BMS
//*----------------------------------------------------------------------------
ushort bq40z80_read_status(void)
{
	ushort val = 0;

	if(!bms_loc_init)
		return 0xFFFF;

	if(bq40z80_read_16bit_reg(0x16, &val) == 0)
		return val;

	return 0xFFFF;
}

//*----------------------------------------------------------------------------
//* Function Name       : bq40z80_read_current
//* Object              :
//* Notes    			:
//* Notes   			:
//* Notes    			:
//* Context    			: CONTEXT_BMS
//*----------------------------------------------------------------------------
short bq40z80_read_current(void)
{
	ushort curr;

	if(!bms_loc_init)
		return 0;

	if(bq40z80_read_16bit_reg(0x0A, &curr) == 0)
	{
		//printf("curr: %dmA \r\n", (short)curr);
		return (short)curr;
	}

	return 0;
}

//*----------------------------------------------------------------------------
//* Function Name       : bq40z80_read_da_status
//* Object              :
//* Notes    			:
//* Notes   			:
//* Notes    			:
//* Context    			: CONTEXT_BMS
//*----------------------------------------------------------------------------
short bq40z80_read_da_status(void)
{
	uchar da_stat[50];
	//ushort intt, ts4t;

	// Read DA status1
	if(bq40z80_sbs_read_block(0x71, da_stat, 32))
		return 1;

	//print_hex_array(da_stat, 32);

	bmss.c[0] = (da_stat[1] << 8)|da_stat[0];
	bmss.c[1] = (da_stat[3] << 8)|da_stat[2];
	bmss.c[2] = (da_stat[5] << 8)|da_stat[4];
	bmss.c[3] = (da_stat[7] << 8)|da_stat[6];

	// Read DA status2
	if(bq40z80_sbs_read_block(0x72, da_stat, 16))
		return 2;

	//print_hex_array(da_stat, 16);

	// Internal temp
	//intt = (da_stat[1] << 8)|da_stat[0];

	// TS1 - next to cell1
	bmss.t[0] = (da_stat[3] << 8)|da_stat[2];
	bmss.t[0] = (bmss.t[0]*10 - 27315);

	// TS2 - next to cell 3
	bmss.t[2] = (da_stat[5] << 8)|da_stat[4];
	bmss.t[2] = (bmss.t[2]*10 - 27315);

	// TS3 - next to cell 5
	bmss.t[4] = (da_stat[7] << 8)|da_stat[6];
	bmss.t[4] = (bmss.t[4]*10 - 27315);

	// Other two cells as average of neighbour cells
	bmss.t[1] = (bmss.t[0] + bmss.t[2])/2;
	bmss.t[3] = (bmss.t[2] + bmss.t[4])/2;

	// TS4
	//ts4t = (da_stat[9] << 8)|da_stat[8];

	//printf("int t %dC \r\n", (int)((intt*10 - 27315)/100));
	//printf("fet t %dC \r\n", (int)((ts4t*10 - 27315)/100));

	// Read DA status3
	if(bq40z80_sbs_read_block(0x7B, da_stat, 18))
		return 3;

	//print_hex_array(da_stat, 18);

	bmss.c[4] = (da_stat[1] << 8)|da_stat[0];

	// Notify UI
	bmss.rr = 1;

	return 0;
}

//*----------------------------------------------------------------------------
//* Function Name       : bq40z80_init
//* Object              :
//* Notes    			:
//* Notes   			:
//* Notes    			:
//* Context    			: CONTEXT_BMS
//*----------------------------------------------------------------------------
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

	//bq40z80_read_da_status();
}


#endif
