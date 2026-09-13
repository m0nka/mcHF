/************************************************************************************
**                                                                                 **
**                                 mcHF QRP Transceiver                            **
**                         Krassi Atanassov - M0NKA, 2013-2026                     **
**                                                                                 **
**---------------------------------------------------------------------------------**
**                                                                                 **
**  File name:       bms_gold.c                                                    **
**  Description:     BQ40Z80 data flash backup/program from SD card gold file      **
**                   Bootloader variant - uses HAL_Delay, no RTOS                  **
**  Last Modified:                                                                 **
**  Licence:         https://github.com/m0nka/mcHF/blob/main/LICENSE              **
************************************************************************************/
//
// Gold file is a TI style flash stream(.fs), one raw SMBus transaction per line:
//
//      ; comment
//      W: 16 44 22 00 40 <32 data bytes>       - write(dev addr, reg, data...)
//      C: 16 00 41 00                          - read back and compare
//      X: 100                                  - delay in mS, decimal
//
// All W/C values are hex without 0x prefix, device address is the 8-bit
// write address(0x16 for the gauge).
//
#include "main.h"
#include "mchf_pro_board.h"

#include <stdlib.h>

#include "shared_i2c.h"
#include "bq40z80.h"
#include "bms_gold.h"

#include "hw_sd.h"
#include "ff_gen_drv.h"
#include "sd_diskio.h"

#include "selftest_proc.h"

// FatFS objects defined in selftest_proc.c
extern FATFS SDFatFs;
extern FIL  MyFile;
extern char SDPath[4];

// IO and line buffers, placed at file scope to avoid stack pressure
static char     gold_iobuf[1024];
static char     gold_line[384];

// -----------------------------------------------------------------------
// SD card + FatFS init helper
// -----------------------------------------------------------------------
static uchar gold_sd_init(void)
{
    if(test_sd_card() != 0)
        return BMS_GOLD_ERR_SD_INIT;

    if(FATFS_LinkDriver(&SD_Driver, SDPath) != 0)
        return BMS_GOLD_ERR_FS_INIT;

    if(f_mount(&SDFatFs, (TCHAR const*)SDPath, 0) != FR_OK)
        return BMS_GOLD_ERR_FS_INIT;

    return 0;
}

// -----------------------------------------------------------------------
// Session open: unseal + full access
// -----------------------------------------------------------------------
static uchar gold_session_open(void)
{
    uchar retry;

    for(retry = 0; retry < 3; retry++)
    {
        if(bq40z80_unseal() == 0)
            break;

        HAL_Delay(50);
    }

    if(retry == 3)
        return BMS_GOLD_ERR_UNSEAL;

    HAL_Delay(50);

    for(retry = 0; retry < 3; retry++)
    {
        if(bq40z80_full_access() == 0)
            break;

        HAL_Delay(50);
    }

    if(retry == 3)
        return BMS_GOLD_ERR_FULL_ACCESS;

    HAL_Delay(50);

    return 0;
}

// -----------------------------------------------------------------------
// Session close: seal the gauge
// -----------------------------------------------------------------------
static void gold_session_close(void)
{
    bq40z80_seal();
}

// -----------------------------------------------------------------------
// Dump the entire data flash to a flash stream file on SD
// -----------------------------------------------------------------------
static uchar gold_dump_df(const char *path)
{
    FRESULT     res;
    UINT        bw;
    uchar       row[BQ40Z80_DF_ROW];
    ulong       addr;
    uchar       retry, err;
    int         used, i;

    f_mkdir(BMS_GOLD_DIR);

    res = f_open(&MyFile, path, FA_WRITE|FA_CREATE_ALWAYS);
    if(res != FR_OK)
    {
        printf("bms gold: dump open err(%d) \r\n", res);
        return BMS_GOLD_ERR_FILE_OPEN;
    }

    used = sprintf(gold_iobuf, "; bq40z80 data flash dump\r\n");

    for(addr = BQ40Z80_DF_START; addr <= BQ40Z80_DF_END; addr += BQ40Z80_DF_ROW)
    {
        err = 1;
        for(retry = 0; retry < 3; retry++)
        {
            err = bq40z80_df_read_row((ushort)addr, row, BQ40Z80_DF_ROW);
            if(err == 0)
                break;

            HAL_Delay(25);
        }

        if(err != 0)
        {
            printf("bms gold: df read err at 0x%04x \r\n", (int)addr);
            f_close(&MyFile);
            return BMS_GOLD_ERR_DF_READ;
        }

        // One row as raw SMBus block write to ManufacturerBlockAccess(0x44)
        used += sprintf((gold_iobuf + used), "W: 16 44 %02X %02X %02X",
                        (BQ40Z80_DF_ROW + 2), (int)(addr & 0xFF), (int)(addr >> 8));

        for(i = 0; i < BQ40Z80_DF_ROW; i++)
            used += sprintf((gold_iobuf + used), " %02X", row[i]);

        used += sprintf((gold_iobuf + used), "\r\nX: %d\r\n", BQ40Z80_DF_WRITE_MS);

        // Flush when getting full
        if(used > (int)(sizeof(gold_iobuf) - 256))
        {
            res = f_write(&MyFile, gold_iobuf, used, &bw);
            if((res != FR_OK)||(bw != (UINT)used))
            {
                f_close(&MyFile);
                return BMS_GOLD_ERR_FILE_WRITE;
            }

            used = 0;
        }
    }

    // Flush leftover
    if(used > 0)
    {
        res = f_write(&MyFile, gold_iobuf, used, &bw);
        if((res != FR_OK)||(bw != (UINT)used))
        {
            f_close(&MyFile);
            return BMS_GOLD_ERR_FILE_WRITE;
        }
    }

    f_close(&MyFile);
    printf("bms gold: df dumped to %s \r\n", path);

    return 0;
}

// -----------------------------------------------------------------------
// Execute one flash stream line on the I2C bus
// -----------------------------------------------------------------------
static uchar gold_exec_line(char *line)
{
    char    *p = line;
    char    *end;
    char    type;
    uchar   data[128];
    uchar   t_buf[128];
    ulong   val;
    ushort  n = 0;
    uchar   retry;

    while((*p == ' ')||(*p == '\t'))
        p++;

    // Empty or comment
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

        HAL_Delay(val);
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

                HAL_Delay(20);
            }

            if(retry == 3)
                return BMS_GOLD_ERR_I2C_WRITE;

            HAL_Delay(2);
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

                HAL_Delay(20);
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

// -----------------------------------------------------------------------
// Post-flash activation: make sure gauging and FET control are on
// -----------------------------------------------------------------------
static uchar gold_activate(void)
{
    uchar   retry;
    ushort  stat = 0;

    for(retry = 0; retry < 3; retry++)
    {
        if(bq40z80_unseal() == 0)
            break;

        HAL_Delay(100);
    }

    if(retry == 3)
        return 1;

    HAL_Delay(50);

    if(bq40z80_read_mfg_status(&stat) != 0)
        return 2;

    printf("bms gold: mfg status %04x \r\n", stat);

    if(!(stat & BQ40Z80_MFG_GAUGE_EN))
    {
        printf("bms gold: enabling gauging \r\n");
        bq40z80_gauging_toggle();
        HAL_Delay(100);
    }

    if(!(stat & BQ40Z80_MFG_FET_EN))
    {
        printf("bms gold: enabling fets \r\n");
        bq40z80_fet_en_toggle();
        HAL_Delay(100);
    }

    // Verify
    if(bq40z80_read_mfg_status(&stat) != 0)
        return 3;

    if((stat & (BQ40Z80_MFG_GAUGE_EN|BQ40Z80_MFG_FET_EN)) !=
              (BQ40Z80_MFG_GAUGE_EN|BQ40Z80_MFG_FET_EN))
        return 4;

    return 0;
}

// -----------------------------------------------------------------------
// Backup: dump gauge DF to backup.fs
// -----------------------------------------------------------------------
uchar bms_gold_backup_boot(void)
{
    uchar err;

    printf("bms gold: backup start \r\n");

    err = gold_sd_init();
    if(err != 0)
        return err;

    err = gold_session_open();
    if(err != 0)
        goto backup_done;

    err = gold_dump_df(BMS_GOLD_BACKUP_FILE);

backup_done:
    gold_session_close();
    fs_cleanup();

    printf("bms gold: backup done(%d) \r\n", err);
    return err;
}

// -----------------------------------------------------------------------
// Flash: program gauge from gold.fs, undo dump first
// -----------------------------------------------------------------------
uchar bms_gold_flash_boot(void)
{
    FRESULT     res;
    UINT        br, i;
    ulong       fsize, done = 0;
    ushort      line_no = 1;
    int         len = 0;
    uchar       err = 0;

    printf("bms gold: flash start \r\n");

    err = gold_sd_init();
    if(err != 0)
        return err;

    // Verify gold file exists
    res = f_open(&MyFile, BMS_GOLD_FILE, FA_READ);
    if(res != FR_OK)
    {
        printf("bms gold: no gold file(%d) \r\n", res);
        fs_cleanup();
        return BMS_GOLD_ERR_NO_GOLD_FILE;
    }

    fsize = f_size(&MyFile);
    f_close(&MyFile);

    if(fsize == 0)
    {
        fs_cleanup();
        return BMS_GOLD_ERR_NO_GOLD_FILE;
    }

    printf("bms gold: gold.fs %d bytes \r\n", (int)fsize);

    if(gold_session_open() != 0)
    {
        fs_cleanup();
        return BMS_GOLD_ERR_UNSEAL;
    }

    // Safety net: backup current config
    if(gold_dump_df(BMS_GOLD_UNDO_FILE) != 0)
    {
        gold_session_close();
        fs_cleanup();
        return BMS_GOLD_ERR_FILE_WRITE;
    }

    res = f_open(&MyFile, BMS_GOLD_FILE, FA_READ);
    if(res != FR_OK)
    {
        gold_session_close();
        fs_cleanup();
        return BMS_GOLD_ERR_FILE_OPEN;
    }

    // Stream the file and replay it line by line
    for(;;)
    {
        res = f_read(&MyFile, gold_iobuf, sizeof(gold_iobuf), &br);
        if(res != FR_OK)
        {
            err = BMS_GOLD_ERR_FILE_READ;
            break;
        }

        if(br == 0)
            break;

        for(i = 0; i < br; i++)
        {
            char c = gold_iobuf[i];

            if(c == '\n')
            {
                gold_line[len] = 0;
                len = 0;

                err = gold_exec_line(gold_line);
                if(err != 0)
                    break;

                line_no++;
            }
            else if(c != '\r')
            {
                if(len >= (int)(sizeof(gold_line) - 1))
                {
                    err = BMS_GOLD_ERR_LINE_TOO_LONG;
                    break;
                }

                gold_line[len++] = c;
            }
        }

        done += br;

        if(err != 0)
            break;
    }

    // Last line without newline
    if((err == 0)&&(len > 0))
    {
        gold_line[len] = 0;
        err = gold_exec_line(gold_line);
    }

    f_close(&MyFile);

    if(err != 0)
    {
        gold_session_close();
        fs_cleanup();
        printf("bms gold: flash err(%d) at line %d \r\n", err, line_no);
        return err;
    }

    // Device reset to activate new DF
    bq40z80_device_reset();
    HAL_Delay(2500);

    // Make sure gauging and FET control ended up enabled
    err = gold_activate();

    bq40z80_seal();
    fs_cleanup();

    if(err != 0)
    {
        printf("bms gold: activate err(%d) \r\n", err);
        return BMS_GOLD_ERR_ACTIVATE;
    }

    printf("bms gold: flash done, %d lines \r\n", line_no);

    return 0;
}
