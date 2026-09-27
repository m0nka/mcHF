/************************************************************************************
**                                                                                 **
**                                 mcHF QRP Transceiver                            **
**                         Krassi Atanassov - M0NKA, 2013-2026                     **
**                                                                                 **
**---------------------------------------------------------------------------------**
**                                                                                 **
**  File name:		hf_app.h                                                       **
**  Description:	HF digital mode ownership and shared working RAM               **
**  Last Modified:                                                                 **
**  Licence:		https://github.com/m0nka/mcHF/blob/main/LICENSE                **
************************************************************************************/
//
// The HF radio has one owner at a time. The desktop gives the user full
// control; the digital mode screens (FT8, MarsChat on the WSPR engine) take
// the HF hardware over and are mutually exclusive. So their big working
// buffers overlay one another in a single SDRAM arena instead of each mode
// holding its own for the lifetime of the firmware.
//
// Two layers:
//
//	- selection	: the UI mode switch says which app owns HF. HF_APP_NONE is the
//				  desktop - no screen owns HF, first come first served (debug
//				  paths like a WSPR decode of an SD capture)
//	- arena lock: a task takes the arena around every use of it (a capture
//				  cycle, a decode). A handover waits for the previous owner to
//				  finish its current operation, it never pulls the RAM from
//				  under it. The lock is a mutex - acquire and release from the
//				  same task
//
// The aux LoRa radio (MeshCore) is independent of all this and keeps running.
//
#ifndef __HF_APP_H
#define __HF_APP_H

#include "mchf_types.h"

#include "FreeRTOS.h"

// HF digital mode owners
#define HF_APP_NONE					0		// desktop, nobody owns HF
#define HF_APP_WSPR					1		// WSPR engine (MarsChat, WSPR monitor)
#define HF_APP_FT8					2

// Shared arena size - the largest single user. WSPR: 64 KB capture staging
// ring + 1,072,704 bytes of decoder buffers. FT8: 167,028 byte waterfall
#define HF_APP_RAM_SIZE				(1120 * 1024)

void	hf_app_init(void);

// UI mode switch: which app owns HF from now on. Does not stop anything -
// the caller winds the previous owner down, the arena lock makes the new
// owner wait for it
void	hf_app_select(uchar id);
uchar	hf_app_selected(void);

// May this app use HF digital resources right now
uchar	hf_app_allowed(uchar id);

// Lock the arena for 'id'. Returns the arena (32 byte aligned, contents
// undefined) or NULL if 'id' is not allowed, size does not fit, or the
// previous holder did not let go within 'wait' ticks
void	*hf_app_ram_acquire(uchar id, ulong size, TickType_t wait);
void	hf_app_ram_release(uchar id);

#endif
