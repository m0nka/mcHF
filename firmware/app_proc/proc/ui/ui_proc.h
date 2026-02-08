/************************************************************************************
**                                                                                 **
**                                 mcHF QRP Transceiver                            **
**                         Krassi Atanassov - M0NKA, 2013-2026                     **
**                                                                                 **
**---------------------------------------------------------------------------------**
**                                                                                 **
**  File name:                                                                     **
**  Description:                                                                   **
**  Last Modified:                                                                 **
**  Licence:			https://github.com/m0nka/mcHF/blob/main/LICENSE            **
************************************************************************************/
#ifndef __UI_PROC_H
#define __UI_PROC_H

// Enable individual desktop controls
#define 	DESKTOP_SHOW_BATTERY
#define 	DESKTOP_SHOW_SMETER
#define 	DESKTOP_SHOW_SPECTRUM
#define 	DESKTOP_SHOW_FREQUENCY
#define 	DESKTOP_SHOW_SDCARD
#define 	DESKTOP_SHOW_CLOCK
#define 	DESKTOP_SHOW_VOLUME

// Disable individual controls
#define 	SPECTRUM_WATERFALL
#define 	VFO_BOTH

#define		DESKTOP_SMETER			0
#define		DESKTOP_SPECTRUM		1
#define		DESKTOP_WATERFALL		2

// ----------------------------------------------

void ui_proc_clear_active(void);
void ui_proc_power_cleanup(void);
void ui_proc_task(void const *arg);

#endif
