/************************************************************************************
**                                                                                 **
**                             mcHF Pro QRP Transceiver                            **
**                         Krassi Atanassov - M0NKA, 2013-2021                     **
**                                                                                 **
**---------------------------------------------------------------------------------**
**                                                                                 **
**  File name:                                                                     **
**  Description:                                                                   **
**  Last Modified:                                                                 **
**  Licence:       The mcHF project is released for radio amateurs experimentation **
**               and non-commercial use only.Check 3rd party drivers for licensing **
************************************************************************************/
#include "main.h"
#include "mchf_pro_board.h"

#ifdef CONTEXT_BMS

#include "shared_i2c.h"

#include "bq40z80.h"


uchar bq40z80_mac_read_block(ushort cmd, uchar *buf, uchar len)
{
	ulong err;
	uchar t_buf[40];
	uchar t_len;
	uchar i;

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

	err = shared_i2c_read_reg(0x16, 0x44, t_buf, 36);
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
	data[1] = (uchar)val;

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

	err = shared_i2c_read_reg(0x16, reg, data, 2);
	if(err != 0)
	{
		printf("read reg %02x, err %d\r\n", (int)reg, (int)err);
		return 2;
	}

	*val = (data[1] << 8)|data[0];

	return 0;
}

uchar bq40z80_read_fw_ver(void)
{
	uchar buf[100];

	//bq40z80_write_16bit_reg(0x00, 0x0002);

	//osDelay(2);

	bq40z80_mac_read_block(0x0002, buf, 11);
}


#endif
