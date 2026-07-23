//************************************************************************************
//**                                                                                 **
//**                                 mcHF QRP Transceiver                            **
//**                         Krassi Atanassov - M0NKA, 2013-2026                     **
//**                                                                                 **
//**---------------------------------------------------------------------------------**
//**                                                                                 **
//**  File name:		bms_gold.h                                                     **
//**  Description:	BQ40Z80 data flash backup/program from SD card gold file       **
//**  Last Modified:                                                                 **
//**  Licence:			https://github.com/m0nka/mcHF/blob/main/LICENSE            **
//************************************************************************************
#ifndef __BMS_GOLD_H
#define __BMS_GOLD_H

// SD card locations
#define BMS_GOLD_DIR				"0://bms"
#define BMS_GOLD_FILE				"0://bms/gold.fs"
#define BMS_GOLD_BACKUP_FILE		"0://bms/backup.fs"
#define BMS_GOLD_UNDO_FILE			"0://bms/undo.fs"

// bmss.gold_state values
#define BMS_GOLD_IDLE				0
#define BMS_GOLD_BUSY_BACKUP		1
#define BMS_GOLD_BUSY_FLASH			2
#define BMS_GOLD_DONE				3
#define BMS_GOLD_ERROR				4

// bmss.gold_err values
#define BMS_GOLD_ERR_NONE			0
#define BMS_GOLD_ERR_UNSEAL			1
#define BMS_GOLD_ERR_FULL_ACCESS	2
#define BMS_GOLD_ERR_FILE_OPEN		3
#define BMS_GOLD_ERR_FILE_WRITE		4
#define BMS_GOLD_ERR_FILE_READ		5
#define BMS_GOLD_ERR_DF_READ		6
#define BMS_GOLD_ERR_LINE_SYNTAX	7
#define BMS_GOLD_ERR_LINE_TOO_LONG	8
#define BMS_GOLD_ERR_I2C_WRITE		9
#define BMS_GOLD_ERR_VERIFY			10
#define BMS_GOLD_ERR_NO_GOLD_FILE	11
#define BMS_GOLD_ERR_ACTIVATE		12

uchar bms_gold_backup(void);
uchar bms_gold_flash(void);

#endif
