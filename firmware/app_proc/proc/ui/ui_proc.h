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

// Profile this driver execution timings
//#define PROFILE_UI_REPAINT

// Unit test this driver
//#define UI_RUN_ALL_TESTS

// Splash screen time
#define SPLASH_STAY_ON_SCREEN		2000

// Enable individual desktop controls
#define 	DESKTOP_SHOW_BATTERY
#define 	DESKTOP_SHOW_SMETER
#define 	DESKTOP_SHOW_SPECTRUM
#define 	DESKTOP_SHOW_FREQUENCY
#define 	DESKTOP_SHOW_SDCARD
#define 	DESKTOP_SHOW_CLOCK
#define 	DESKTOP_SHOW_VOLUME
#define 	DESKTOP_SHOW_FILTER
#define 	DESKTOP_SHOW_TX_STAT
#define 	DESKTOP_SHOW_CPU_STAT

// Disable individual controls
#define 	VFO_BOTH

#define		DESKTOP_SMETER			0
#define		DESKTOP_SPECTRUM		1
#define		DESKTOP_WATERFALL		2

#define		UI_CLEANUP				0
#define		UI_BACKLIGHT_OFF		1

// ----------------------------------------------

void ui_proc_clear_active(void);
void ui_proc_power_cleanup(uchar mode);
void ui_proc_task(void const *arg);

#endif
