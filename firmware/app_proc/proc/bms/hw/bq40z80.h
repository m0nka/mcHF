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
#ifndef __BQ40Z80_H
#define __BQ40Z80_H

// Full access keys, sent via ManufacturerAccess() after unseal
// (bqStudio default is FFFF FFFF, change here if pack was re-keyed)
#define BQ40Z80_FA_KEY_W0				0xFFFF
#define BQ40Z80_FA_KEY_W1				0xFFFF

// Data flash address window and MAC row size
// (check against TRM SLUUBT5, chapter Data Flash)
#define BQ40Z80_DF_START				0x4000
#define BQ40Z80_DF_END					0x5FFF
#define BQ40Z80_DF_ROW					32

// Data flash programming time per row write, in mS
#define BQ40Z80_DF_WRITE_MS				100

uchar bq40z80_mac_read_block(ushort cmd, uchar *buf, uchar len);

uchar bq40z80_mac_write(ushort cmd, uchar *data, uchar len);
uchar bq40z80_mac_read(ushort cmd, uchar *buf, uchar len);

uchar bq40z80_df_read_row(ushort addr, uchar *buf, uchar len);
uchar bq40z80_df_write_row(ushort addr, uchar *data, uchar len);

// ManufacturingStatus() bits
#define BQ40Z80_MFG_GAUGE_EN			0x0008
#define BQ40Z80_MFG_FET_EN				0x0010

uchar bq40z80_full_access(void);
uchar bq40z80_device_reset(void);

uchar bq40z80_read_sn(ushort *val);
uchar bq40z80_write_sn(ushort sn);

uchar bq40z80_read_mfg_status(ushort *val);
uchar bq40z80_gauging_toggle(void);
uchar bq40z80_fet_en_toggle(void);

uchar bq40z80_write_16bit_reg(uchar reg, ushort val);
uchar bq40z80_read_16bit_reg(uchar reg, ushort *val);

uchar bq40z80_shutdown(void);

uchar bq40z80_unseal(void);
uchar bq40z80_seal(void);

uchar bq40z80_read_fw_ver(void);

uchar bq40z80_read_soc(void);
ushort bq40z80_read_runtime(void);

ushort bq40z80_read_pack_voltage(void);

ushort bq40z80_read_status(void);
short bq40z80_read_current(void);

short bq40z80_read_da_status(void);
void  bq40z80_init(void);

#endif


