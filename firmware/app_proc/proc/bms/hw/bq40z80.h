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

uchar bq40z80_mac_read_block(ushort cmd, uchar *buf, uchar len);

uchar bq40z80_write_16bit_reg(uchar reg, ushort val);
uchar bq40z80_read_16bit_reg(uchar reg, ushort *val);

uchar bq40z80_shutdown(void);
uchar bq40z80_unseal(void);
uchar bq40z80_read_fw_ver(void);

uchar bq40z80_read_soc(void);
ushort bq40z80_read_runtime(void);

ushort bq40z80_read_status(void);
short bq40z80_read_current(void);

void  bq40z80_init(void);

#endif


