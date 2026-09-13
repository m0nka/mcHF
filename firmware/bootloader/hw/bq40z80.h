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
#ifndef __BQ40Z80_H
#define __BQ40Z80_H

// BQ40Z80 constants
#define BQ40Z80_FA_KEY_W0           0xFFFF
#define BQ40Z80_FA_KEY_W1           0xFFFF
#define BQ40Z80_DF_START            0x4000
#define BQ40Z80_DF_END              0x5FFF
#define BQ40Z80_DF_ROW              32
#define BQ40Z80_DF_WRITE_MS         100
#define BQ40Z80_MFG_GAUGE_EN       0x0008
#define BQ40Z80_MFG_FET_EN         0x0010

// Basic register access
uchar bq40z80_mac_read_block(ushort cmd, uchar *buf, uchar len);
uchar bq40z80_write_16bit_reg(uchar reg, ushort val);
uchar bq40z80_read_16bit_reg(uchar reg, ushort *val);

// MAC block access (needed for gold file operations)
uchar bq40z80_mac_write(ushort cmd, uchar *data, uchar len);
uchar bq40z80_mac_read(ushort cmd, uchar *buf, uchar len);

// Security state
uchar bq40z80_unseal(void);
uchar bq40z80_seal(void);
uchar bq40z80_full_access(void);
uchar bq40z80_device_reset(void);

// Manufacturing status
uchar bq40z80_read_mfg_status(ushort *val);
uchar bq40z80_gauging_toggle(void);
uchar bq40z80_fet_en_toggle(void);

// Data flash row access
uchar bq40z80_df_read_row(ushort addr, uchar *buf, uchar len);

// Standard reads
uchar bq40z80_read_fw_ver(void);
uchar bq40z80_read_soc(void);
ushort bq40z80_read_runtime(void);
ushort bq40z80_read_status(void);
short bq40z80_read_current(void);
ushort bq40z80_read_pack_voltage(void);

void  bq40z80_init(void);

#endif


