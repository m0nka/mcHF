/************************************************************************************
**                                                                                 **
**                                 mcHF QRP Transceiver                            **
**                         Krassi Atanassov - M0NKA, 2013-2026                     **
**                                                                                 **
**---------------------------------------------------------------------------------**
**                                                                                 **
**  File name:		radio_ini.h                                                    **
**  Description:	radio settings in a text .ini file on the SD card              **
**  Licence:		https://github.com/m0nka/mcHF/blob/main/LICENSE                **
************************************************************************************/
#ifndef __RADIO_INI_H
#define __RADIO_INI_H

#define RADIO_INI_PATH			"0://system/settings.ini"
#define RADIO_INI_TMP_PATH		"0://system/settings.tmp"

// Result codes
#define RADIO_INI_OK			0
#define RADIO_INI_NO_FILE		1		// no card or no file
#define RADIO_INI_ERROR			2		// read/write failed, or too big

uchar radio_ini_file_sig(ulong *sig);
uchar radio_ini_load(void);
uchar radio_ini_save(ulong *sig);

#endif
