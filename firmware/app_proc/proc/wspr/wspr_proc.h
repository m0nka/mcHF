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

#include "wspr_decoder.h"

// Task notification bits
#define WSPR_NOTIFY_DECODE			0x01
#define WSPR_NOTIFY_DATA			0x02
#define WSPR_NOTIFY_WAKE			0x04

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

// Arm/disarm the background WSPR monitor - when armed, the task captures
// the rx audio streamed from the M4 core every even minute, saves it to
// the SD card and decodes it. Callable from any task (UI hook)
void	wspr_proc_monitor_set(uchar on);

// Register a raw decode consumer - called for every raw 50 bit decode
// before the type 1 unpack; return 1 to consume the decode (it will not
// be logged as a WSPR spot). Lets another personality (MarsChat) share
// the decoder without this subsystem knowing about it
void	wspr_proc_set_raw_hook(uchar (*hook)(const WSPR_RAW_DECODE *raw));

// Capture stream interface, called by the icc task only
uchar	wspr_capture_active(void);
void	wspr_capture_mark_started(void);
void	wspr_capture_mark_stopped(void);
void	wspr_capture_push(const uchar *data, ushort len, uchar flags);

#endif
