/************************************************************************************
**                                                                                 **
**                                 mcHF QRP Transceiver                            **
**                         Krassi Atanassov - M0NKA, 2013-2026                     **
**                                                                                 **
**---------------------------------------------------------------------------------**
**                                                                                 **
**  File name:		marschat_proc.h                                                **
**  Description:	MarsChat prototype process                                     **
**  Last Modified:                                                                 **
**  Licence:		https://github.com/m0nka/mcHF/blob/main/LICENSE                **
************************************************************************************/
//
// WP4 seed - in-tree prototype of the MarsChat personality (the final
// delivery form is a loadable SD app, see claude/MarsChat/PROJECT.md).
// Registers as raw decode consumer with the wspr task and, with
// MARSCHAT_LOOPBACK_BEACON enabled, sends a test beacon over the CLK1
// loopback injector every even minute (radio stays in RX - arm the
// WSPR monitor to capture and decode our own signal)
//
#ifndef __MARSCHAT_PROC_H
#define __MARSCHAT_PROC_H

// Task notification bits
#define MARSCHAT_NOTIFY_WAKE		0x01

void	marschat_proc_task(void const *arg);

#endif
