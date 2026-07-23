//************************************************************************************
//**                                                                                 **
//**                                 mcHF QRP Transceiver                            **
//**                         Krassi Atanassov - M0NKA, 2013-2026                     **
//**                                                                                 **
//**---------------------------------------------------------------------------------**
//**                                                                                 **
//**  File name:		bms_gold.c                                                     **
//**  Description:	BQ40Z80 data flash backup/program from SD card gold file       **
//**  Last Modified:                                                                 **
//**  Licence:			https://github.com/m0nka/mcHF/blob/main/LICENSE            **
//************************************************************************************
//
// Gold file is a TI style flash stream(.fs), one raw SMBus transaction per line:
//
//		; comment
//		W: 16 44 22 00 40 <32 data bytes>		- write(dev addr, reg, data...)
//		C: 16 00 41 00							- read back and compare
//		X: 100									- delay in mS, decimal
//
// All W/C values are hex without 0x prefix, device address is the 8-bit
// write address(0x16 for the gauge). Files exported by bqStudio golden
// image(df.fs) use the same format, as do the backups generated here.
//
#include "main.h"
#include "mchf_pro_board.h"

#if defined(CONTEXT_BMS) && defined(CONTEXT_SD)

#include <stdlib.h>

#include "ff.h"
#include "shared_i2c.h"

#include "hw/bq40z80.h"
#include "bms_proc.h"
#include "bms_gold.h"

extern struct BMSState	bmss;

// SD card io chunk, lives in AXI ram like the other disk buffers
__attribute__((section(".axi_mem"))) __attribute__ ((aligned (32))) \
static char		bms_gold_iobuf[4096];

// File object off the task stack, also DMA safe
__attribute__((section(".axi_mem"))) __attribute__ ((aligned (32))) \
static FIL		bms_gold_fil;

// Flash stream line assembly buffer
static char		bms_gold_line[384];

//*----------------------------------------------------------------------------
//* Function Name       : bms_gold_session_open
//* Object              :
//* Notes    			: unseal the gauge and promote to full access
//* Notes   			:
//* Notes    			:
//* Context    			: CONTEXT_BMS
//*----------------------------------------------------------------------------
static uchar bms_gold_session_open(void)
{
	uchar retry;

	for(retry = 0; retry < 3; retry++)
	{
		if(bq40z80_unseal() == 0)
			break;

		osDelay(50);
	}

	if(retry == 3)
	{
		bmss.gold_err = BMS_GOLD_ERR_UNSEAL;
		return 1;
	}

	osDelay(50);

	for(retry = 0; retry < 3; retry++)
	{
		if(bq40z80_full_access() == 0)
			break;

		osDelay(50);
	}

	if(retry == 3)
	{
		bmss.gold_err = BMS_GOLD_ERR_FULL_ACCESS;
		return 2;
	}

	osDelay(50);

	return 0;
}

//*----------------------------------------------------------------------------
//* Function Name       : bms_gold_session_close
//* Object              :
//* Notes    			: restore the seal state the rest of the firmware
//* Notes   			: expects(BMS menu holds the gauge unsealed)
//* Notes    			:
//* Context    			: CONTEXT_BMS
//*----------------------------------------------------------------------------
static void bms_gold_session_close(void)
{
	if(!bmss.bms_unlock_state)
		bq40z80_seal();
}

//*----------------------------------------------------------------------------
//* Function Name       : bms_gold_dump_df
//* Object              :
//* Notes    			: dump the entire data flash to a replayable flash
//* Notes   			: stream file on the SD card
//* Notes    			: needs unsealed mode for the row reads
//* Context    			: CONTEXT_BMS
//*----------------------------------------------------------------------------
static uchar bms_gold_dump_df(const char *path)
{
	FRESULT		res;
	UINT		bw;
	uchar		row[BQ40Z80_DF_ROW];
	ulong		addr;
	uchar		retry, err;
	int			used, i;

	f_mkdir(BMS_GOLD_DIR);		// ok if it exists

	res = f_open(&bms_gold_fil, path, FA_WRITE|FA_CREATE_ALWAYS);
	if(res != FR_OK)
	{
		printf("bms gold: dump open err(%d) \r\n", res);
		bmss.gold_err = BMS_GOLD_ERR_FILE_OPEN;
		return 1;
	}

	used = sprintf(bms_gold_iobuf, "; bq40z80 data flash dump\r\n; replay with the mcHF BMS gold flash\r\n");

	for(addr = BQ40Z80_DF_START; addr <= BQ40Z80_DF_END; addr += BQ40Z80_DF_ROW)
	{
		err = 1;
		for(retry = 0; retry < 3; retry++)
		{
			err = bq40z80_df_read_row((ushort)addr, row, BQ40Z80_DF_ROW);
			if(err == 0)
				break;

			osDelay(25);
		}

		if(err != 0)
		{
			printf("bms gold: df read err(%d) at 0x%04x \r\n", err, (int)addr);
			f_close(&bms_gold_fil);
			bmss.gold_err  = BMS_GOLD_ERR_DF_READ;
			bmss.gold_line = (ushort)addr;
			return 2;
		}

		// One row as raw SMBus block write to ManufacturerBlockAccess(0x44)
		used += sprintf((bms_gold_iobuf + used), "W: 16 44 %02X %02X %02X",
						(BQ40Z80_DF_ROW + 2), (int)(addr & 0xFF), (int)(addr >> 8));

		for(i = 0; i < BQ40Z80_DF_ROW; i++)
			used += sprintf((bms_gold_iobuf + used), " %02X", row[i]);

		used += sprintf((bms_gold_iobuf + used), "\r\nX: %d\r\n", BQ40Z80_DF_WRITE_MS);

		// Flush the io buffer when getting full
		if(used > (int)(sizeof(bms_gold_iobuf) - 256))
		{
			res = f_write(&bms_gold_fil, bms_gold_iobuf, used, &bw);
			if((res != FR_OK)||(bw != (UINT)used))
			{
				f_close(&bms_gold_fil);
				bmss.gold_err = BMS_GOLD_ERR_FILE_WRITE;
				return 3;
			}

			used = 0;
		}

		bmss.gold_perc = (uchar)(((addr - BQ40Z80_DF_START)*100)/(BQ40Z80_DF_END - BQ40Z80_DF_START));
	}

	// Flush the leftover
	if(used > 0)
	{
		res = f_write(&bms_gold_fil, bms_gold_iobuf, used, &bw);
		if((res != FR_OK)||(bw != (UINT)used))
		{
			f_close(&bms_gold_fil);
			bmss.gold_err = BMS_GOLD_ERR_FILE_WRITE;
			return 4;
		}
	}

	f_close(&bms_gold_fil);
	bmss.gold_perc = 100;

	printf("bms gold: df dumped to %s \r\n", path);

	return 0;
}

//*----------------------------------------------------------------------------
//* Function Name       : bms_gold_exec_line
//* Object              :
//* Notes    			: replay one flash stream line on the i2c bus
//* Notes   			:
//* Notes    			:
//* Context    			: CONTEXT_BMS
//*----------------------------------------------------------------------------
static uchar bms_gold_exec_line(char *line)
{
	char	*p = line;
	char	*end;
	char	type;
	uchar	data[128];
	uchar	t_buf[128];
	ulong	val;
	ushort	n = 0;
	uchar	retry;

	while((*p == ' ')||(*p == '\t'))
		p++;

	// Empty or comment line
	if((*p == 0)||(*p == ';'))
		return 0;

	type = *p++;

	while((*p == ' ')||(*p == '\t'))
		p++;

	if(*p != ':')
		return BMS_GOLD_ERR_LINE_SYNTAX;

	p++;

	// Delay entry, decimal mS
	if((type == 'X')||(type == 'x'))
	{
		val = strtoul(p, NULL, 10);
		if(val > 5000)
			val = 5000;

		osDelay(val);
		return 0;
	}

	// Collect hex bytes
	for(;;)
	{
		while((*p == ' ')||(*p == '\t'))
			p++;

		if(*p == 0)
			break;

		val = strtoul(p, &end, 16);
		if(end == p)
			return BMS_GOLD_ERR_LINE_SYNTAX;

		if(n >= sizeof(data))
			return BMS_GOLD_ERR_LINE_SYNTAX;

		data[n++] = (uchar)val;
		p = end;
	}

	// Need at least device address, register and one data byte
	if(n < 3)
		return BMS_GOLD_ERR_LINE_SYNTAX;

	switch(type)
	{
		case 'W':
		case 'w':
		{
			for(retry = 0; retry < 3; retry++)
			{
				if(shared_i2c_write_reg(data[0], data[1], (data + 2), (n - 2)) == 0)
					break;

				osDelay(20);
			}

			if(retry == 3)
				return BMS_GOLD_ERR_I2C_WRITE;

			// Inter command pacing
			osDelay(2);
			break;
		}

		case 'C':
		case 'c':
		{
			for(retry = 0; retry < 3; retry++)
			{
				if(shared_i2c_read_reg(data[0], data[1], t_buf, (n - 2)) == 0)
				{
					if(memcmp(t_buf, (data + 2), (n - 2)) == 0)
						break;
				}

				osDelay(20);
			}

			if(retry == 3)
				return BMS_GOLD_ERR_VERIFY;

			break;
		}

		default:
			return BMS_GOLD_ERR_LINE_SYNTAX;
	}

	return 0;
}

//*----------------------------------------------------------------------------
//* Function Name       : bms_gold_activate
//* Object              :
//* Notes    			: post flash activation - make sure gauging and FET
//* Notes   			: control are enabled, as bqStudio operators do by
//* Notes    			: hand. GAUGE_EN/FET_EN MAC commands are TOGGLES,
//* Notes    			: so check ManufacturingStatus() first!
//* Context    			: CONTEXT_BMS
//*----------------------------------------------------------------------------
static uchar bms_gold_activate(ushort *mfg_stat)
{
	uchar	retry;
	ushort	stat = 0;

	*mfg_stat = 0;

	// Gauge boots sealed after the reset
	for(retry = 0; retry < 3; retry++)
	{
		if(bq40z80_unseal() == 0)
			break;

		osDelay(100);
	}

	if(retry == 3)
		return 1;

	osDelay(50);

	if(bq40z80_read_mfg_status(&stat) != 0)
		return 2;

	printf("bms gold: mfg status %04x \r\n", stat);

	// Impedance Track gauging off ?
	if(!(stat & BQ40Z80_MFG_GAUGE_EN))
	{
		printf("bms gold: enabling gauging \r\n");
		bq40z80_gauging_toggle();
		osDelay(100);
	}

	// Firmware FET control off ?
	if(!(stat & BQ40Z80_MFG_FET_EN))
	{
		printf("bms gold: enabling fets \r\n");
		bq40z80_fet_en_toggle();
		osDelay(100);
	}

	// Read back and verify
	if(bq40z80_read_mfg_status(&stat) != 0)
		return 3;

	*mfg_stat = stat;

	if((stat & (BQ40Z80_MFG_GAUGE_EN|BQ40Z80_MFG_FET_EN)) != (BQ40Z80_MFG_GAUGE_EN|BQ40Z80_MFG_FET_EN))
		return 4;

	return 0;
}

//*----------------------------------------------------------------------------
//* Function Name       : bms_gold_backup
//* Object              :
//* Notes    			: dump the gauge data flash to 0://bms/backup.fs
//* Notes   			: rename to gold.fs to make it the gold image
//* Notes    			:
//* Context    			: CONTEXT_BMS
//*----------------------------------------------------------------------------
uchar bms_gold_backup(void)
{
	uchar err;

	bmss.gold_state = BMS_GOLD_BUSY_BACKUP;
	bmss.gold_err   = BMS_GOLD_ERR_NONE;
	bmss.gold_line  = 0;
	bmss.gold_perc  = 0;

	printf("bms gold: backup start \r\n");

	err = bms_gold_session_open();
	if(err == 0)
		err = bms_gold_dump_df(BMS_GOLD_BACKUP_FILE);

	bms_gold_session_close();

	bmss.gold_state = (err != 0) ? BMS_GOLD_ERROR : BMS_GOLD_DONE;

	printf("bms gold: backup done(%d) \r\n", err);

	return err;
}

//*----------------------------------------------------------------------------
//* Function Name       : bms_gold_flash
//* Object              :
//* Notes    			: program the gauge from 0://bms/gold.fs, previous
//* Notes   			: config is saved to 0://bms/undo.fs first, gauge is
//* Notes    			: reset at the end to activate the new data flash
//* Context    			: CONTEXT_BMS
//*----------------------------------------------------------------------------
uchar bms_gold_flash(void)
{
	FRESULT		res;
	UINT		br, i;
	ulong		fsize, done = 0;
	ushort		line_no = 1;
	int			len = 0;
	uchar		err = 0;

	bmss.gold_state = BMS_GOLD_BUSY_FLASH;
	bmss.gold_err   = BMS_GOLD_ERR_NONE;
	bmss.gold_line  = 0;
	bmss.gold_perc  = 0;

	// The gold file must be present before touching the gauge
	res = f_open(&bms_gold_fil, BMS_GOLD_FILE, FA_READ);
	if(res != FR_OK)
	{
		printf("bms gold: no gold file(%d) \r\n", res);
		bmss.gold_err   = BMS_GOLD_ERR_NO_GOLD_FILE;
		bmss.gold_state = BMS_GOLD_ERROR;
		return 1;
	}

	fsize = f_size(&bms_gold_fil);
	f_close(&bms_gold_fil);		// reopened after the undo dump

	if(fsize == 0)
	{
		bmss.gold_err   = BMS_GOLD_ERR_NO_GOLD_FILE;
		bmss.gold_state = BMS_GOLD_ERROR;
		return 1;
	}

	printf("bms gold: flash start, %d bytes \r\n", (int)fsize);

	if(bms_gold_session_open() != 0)
	{
		bmss.gold_state = BMS_GOLD_ERROR;
		return 2;
	}

	// Safety net, keep a copy of the current config
	if(bms_gold_dump_df(BMS_GOLD_UNDO_FILE) != 0)
	{
		bms_gold_session_close();
		bmss.gold_state = BMS_GOLD_ERROR;
		return 3;
	}

	bmss.gold_perc = 0;

	res = f_open(&bms_gold_fil, BMS_GOLD_FILE, FA_READ);
	if(res != FR_OK)
	{
		bms_gold_session_close();
		bmss.gold_err   = BMS_GOLD_ERR_FILE_OPEN;
		bmss.gold_state = BMS_GOLD_ERROR;
		return 4;
	}

	// Stream the file and replay it line by line
	for(;;)
	{
		res = f_read(&bms_gold_fil, bms_gold_iobuf, sizeof(bms_gold_iobuf), &br);
		if(res != FR_OK)
		{
			err = BMS_GOLD_ERR_FILE_READ;
			break;
		}

		if(br == 0)
			break;

		for(i = 0; i < br; i++)
		{
			char c = bms_gold_iobuf[i];

			if(c == '\n')
			{
				bms_gold_line[len] = 0;
				len = 0;

				err = bms_gold_exec_line(bms_gold_line);
				if(err != 0)
					break;

				line_no++;
			}
			else if(c != '\r')
			{
				if(len >= (int)(sizeof(bms_gold_line) - 1))
				{
					err = BMS_GOLD_ERR_LINE_TOO_LONG;
					break;
				}

				bms_gold_line[len++] = c;
			}
		}

		done += br;
		bmss.gold_perc = (uchar)((done*100)/fsize);

		if(err != 0)
			break;
	}

	// Last line without a newline termination
	if((err == 0)&&(len > 0))
	{
		bms_gold_line[len] = 0;
		err = bms_gold_exec_line(bms_gold_line);
	}

	f_close(&bms_gold_fil);

	if(err != 0)
	{
		bms_gold_session_close();

		printf("bms gold: flash err(%d) at line %d \r\n", err, line_no);
		bmss.gold_err   = err;
		bmss.gold_line  = line_no;
		bmss.gold_state = BMS_GOLD_ERROR;
		return 5;
	}

	// Activate the new configuration - gauge re-reads
	// data flash on reset and boots up sealed
	bq40z80_device_reset();
	osDelay(2500);

	// Make sure gauging and FET control ended up enabled,
	// leaves the gauge unsealed
	ushort mfg_stat = 0;
	err = bms_gold_activate(&mfg_stat);

	// Restore the seal state the rest of the firmware expects
	if(!bmss.bms_unlock_state)
		bq40z80_seal();

	if(err != 0)
	{
		printf("bms gold: activate err(%d), mfg status %04x \r\n", err, mfg_stat);
		bmss.gold_err   = BMS_GOLD_ERR_ACTIVATE;
		bmss.gold_line  = mfg_stat;
		bmss.gold_state = BMS_GOLD_ERROR;
		return 6;
	}

	bmss.gold_perc  = 100;
	bmss.gold_line  = mfg_stat;
	bmss.gold_state = BMS_GOLD_DONE;

	printf("bms gold: flash done, %d lines, mfg status %04x \r\n", line_no, mfg_stat);

	return 0;
}

#endif
