/************************************************************************************
**                                                                                 **
**                                 mcHF QRP Transceiver                            **
**                         Krassi Atanassov - M0NKA, 2013-2026                     **
**                                                                                 **
**---------------------------------------------------------------------------------**
**                                                                                 **
**  File name:		wspr_proc.h                                                    **
**  Description:	WSPR decoder process                                           **
**  Last Modified:                                                                 **
**  Licence:		https://github.com/m0nka/mcHF/blob/main/LICENSE                **
************************************************************************************/
#ifndef __WSPR_PROC_H
#define __WSPR_PROC_H

// Task notification bits
#define WSPR_NOTIFY_DECODE			0x01

// Default file locations on the SD card
#define WSPR_CAPTURE_FILE			"0://wspr/capture.raw"
#define WSPR_DECODES_FILE			"0://wspr/decodes.txt"
#define WSPR_DIR					"0://wspr"

// Capture file format: 16 bit signed LE mono PCM, 12000 Hz, no header,
// recorded from the start of an even minute for up to 120 seconds

void	wspr_proc_task(void const *arg);

// Queue a decode of a finished rx cycle. Called by the recorder (or UI)
// after the two minute cycle is up. path == NULL uses WSPR_CAPTURE_FILE,
// dial_freq_hz (the rf carrier of the usb passband) is used to log the
// absolute frequency of each spot, pass 0 to log audio frequency only.
// Returns 0 if the request was posted.
uchar	wspr_proc_request_decode(const char *path, ulong dial_freq_hz);

#endif
