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
#ifndef __MCHF_HW_I2C_H
#define __MCHF_HW_I2C_H

void 	mchf_hw_i2c_init(void);
void 	mchf_hw_i2c_reset(void);

uchar 	mchf_hw_i2c_WriteRegister(uchar I2CAddr,uchar RegisterAddr, uchar RegisterValue);
uchar 	mchf_hw_i2c_WriteBlock(uchar I2CAddr,uchar RegisterAddr, uchar *data,ulong size);
uchar 	mchf_hw_i2c_ReadRegister (uchar I2CAddr,uchar RegisterAddr, uchar *RegisterValue);
uchar 	mchf_hw_i2c_ReadData(uchar I2CAddr,uchar RegisterAddr, uchar *data, ulong size);

#endif
