/************************************************************************************
**                                                                                 **
**                                 mcHF QRP Transceiver                            **
**                         Krassi Atanassov - M0NKA, 2013-2026                     **
**                                                                                 **
**---------------------------------------------------------------------------------**
**                                                                                 **
**  File name:       ffconf.h                                                      **
**  Description:     FatFs R0.16 configuration for bootloader                      **
**  Last Modified:                                                                 **
**  Licence:         https://github.com/m0nka/mcHF/blob/main/LICENSE              **
************************************************************************************/

#define FFCONF_DEF	80386	// must match FF_DEFINED in ff.h (R0.16)

// ---------------------------------------------------------------------------
// Function Configurations
// ---------------------------------------------------------------------------

#define FF_FS_READONLY	0
#define FF_FS_MINIMIZE	0
#define FF_USE_FIND		0
#define FF_USE_MKFS		0
#define FF_USE_FASTSEEK	1
#define FF_USE_EXPAND	0
#define FF_USE_CHMOD	0
#define FF_USE_LABEL	0
#define FF_USE_FORWARD	0

#define FF_USE_STRFUNC	0
#define FF_PRINT_LLI	0
#define FF_PRINT_FLOAT	0
#define FF_STRF_ENCODE	0

// ---------------------------------------------------------------------------
// Locale and Namespace Configurations
// ---------------------------------------------------------------------------

#define FF_CODE_PAGE	850

#define FF_USE_LFN		2		// LFN on stack (no malloc needed)
#define FF_MAX_LFN		255

#define FF_LFN_UNICODE	0		// ANSI/OEM
#define FF_LFN_BUF		255
#define FF_SFN_BUF		12

#define FF_FS_RPATH		0

#define FF_PATH_DEPTH	10

// ---------------------------------------------------------------------------
// Drive/Volume Configurations
// ---------------------------------------------------------------------------

#define FF_VOLUMES		2		// 0 = SD card, 1 = USB stick

#define FF_STR_VOLUME_ID	0
#define FF_VOLUME_STRS		"SD","USB"

#define FF_MULTI_PARTITION	0

#define FF_MIN_SS		512
#define FF_MAX_SS		512

#define FF_LBA64		0		// keeps LBA_t = DWORD, compatible with existing diskio
#define FF_MIN_GPT		0x10000000

#define FF_USE_TRIM		0

// ---------------------------------------------------------------------------
// System Configurations
// ---------------------------------------------------------------------------

#define FF_FS_TINY		0

#define FF_FS_EXFAT		1		// exFAT support (the whole reason for this port)

#define FF_FS_NORTC		1		// no RTC in bootloader
#define FF_NORTC_MON	1
#define FF_NORTC_MDAY	1
#define FF_NORTC_YEAR	2026

#define FF_FS_CRTIME	0

#define FF_FS_NOFSINFO	0

#define FF_FS_LOCK		0

#define FF_FS_REENTRANT	0		// bare-metal bootloader, no RTOS

// ---------------------------------------------------------------------------
// Compatibility — old R0.12c exposed ff_malloc/ff_free as macros;
// hw_flash.c still uses them.
// ---------------------------------------------------------------------------
#include <stdlib.h>
#ifndef ff_malloc
#define ff_malloc	malloc
#endif
#ifndef ff_free
#define ff_free		free
#endif

// --- End of configuration options ---
