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
#ifndef __PROC_CONF_H
#define __PROC_CONF_H

// Are we running the OS and the HAL tick increase from the same
// system timer ?
//
#define USE_SEPARATE_TIMER_FOR_HAL

// -----------------------------------------------------------------------------
// Firmware execution context (On/Off of any system process)
//
// Nothing much here except quick initial stack setup, basic hw init, MMU On
// clocks and start the OS (IRQ stack usage as well)
#define CONTEXT_RESET_VECTOR

// -----------------------------------------------------------------------------------------------
// High level video driver
//
//
#define CONTEXT_VIDEO

// -----------------------------------------------------------------------------------------------
// Touch screen process
//
//
#define CONTEXT_TOUCH

// -----------------------------------------------------------------------------------------------
// Core to core communication: 	M7 <-> M4
//
//
#define CONTEXT_ICC

// -----------------------------------------------------------------------------------------------
// Encoders input processing
//
//
#define CONTEXT_ROTARY

// -----------------------------------------------------------------------------------------------
// VFO control
//
//
#define CONTEXT_VFO

// -----------------------------------------------------------------------------------------------
// Codec I2C control(M7, while SAI streaming in DSP core)
//
//
#define CONTEXT_AUDIO
//
// Codec output to control final audio PA mute line(or CPU)
//#define USE_HARD_MUTE

// -----------------------------------------------------------------------------------------------
// Battery Management System
//
//
#define CONTEXT_BMS

// -----------------------------------------------------------------------------------------------
// Band switching process
//
//
#define CONTEXT_BAND

// -----------------------------------------------------------------------------------------------
// Transmitter HW control/monitor
//
//
#define CONTEXT_TRX

// -----------------------------------------------------------------------------------------------
// Fan HW control
//
//
#define CONTEXT_FAN

// -----------------------------------------------------------------------------------------------
// Physical keyboard
//
//
#define CONTEXT_KEYPAD

// -----------------------------------------------------------------------------------------------
// Lora transceiver
//
//
#define CONTEXT_LORA

// -----------------------------------------------------------------------------------------------
// GNSS driver
//
//
//#define CONTEXT_GPS

// -----------------------------------------------------------------------------------------------
// Storage process
//
//
#ifndef CONTEXT_GPS
#define CONTEXT_SD
#endif

// -----------------------------------------------------------------------------------------------
// Application loader
//
//
#ifdef CONTEXT_SD
#define CONTEXT_APP
#endif

// -----------------------------------------------------------------------------------------------
// WSPR decoder (works on captures from the SD card)
//
//
#ifdef CONTEXT_SD
#define CONTEXT_WSPR
#endif

// -----------------------------------------------------------------------------------------------
// MarsChat prototype (in-tree seed of the loadable app, claude/MarsChat/PROJECT.md)
// Shares the WSPR decoder via the raw decode hook
//
#ifdef CONTEXT_WSPR
#define CONTEXT_MARSCHAT
#endif

// -------------------------------------------------------------------------------------------
// Process parameters template
//xx_PROC_START_DELAY					We can delay the startup of the process, to prevent
//										unwanted interaction with other, more important ones
//
//xx_PROC_SLEEP_TIME					Each task needs to either sleep for a bit or wait a
//										Semaphore/Notification. Looping 100% of the time will
//										hog the system and hinder the real time performance
//
//xx_PROC_PRIORITY						FreeRTOS priority value
//
//xx_PROC_STACK_SIZE					All functions called by the process context use the
//										stack allocated here
// -------------------------------------------------------------------------------------------

// Keypad process parameters
#define SD_PROC_START_NAME				"sdc"
#define SD_PROC_START_DELAY				0
#define SD_PROC_SLEEP_TIME				portMAX_DELAY
#define SD_PROC_PRIORITY				osPriorityNormal
#define SD_PROC_STACK_SIZE				(configMINIMAL_STACK_SIZE * 16)

// UI(video) process parameters
#define UI_PROC_START_NAME				"gui"
#define UI_PROC_START_DELAY				100
#define UI_PROC_SLEEP_TIME				5
#define UI_PROC_PRIORITY				osPriorityNormal
#define UI_PROC_STACK_SIZE				(configMINIMAL_STACK_SIZE * 64)

// VFO control process parameters
#define AUDIO_PROC_START_NAME			"aud"
#define AUDIO_PROC_START_DELAY			200
#define AUDIO_PROC_SLEEP_TIME			portMAX_DELAY
#define AUDIO_PROC_PRIORITY				osPriorityNormal
#define AUDIO_PROC_STACK_SIZE			(configMINIMAL_STACK_SIZE * 4)

// LCD Touch process parameters
#define TOUCH_PROC_START_NAME			"tch"
#define TOUCH_PROC_START_DELAY			300
#define TOUCH_PROC_SLEEP_TIME			portMAX_DELAY
#define TOUCH_PROC_PRIORITY				osPriorityNormal
#define TOUCH_PROC_STACK_SIZE			(configMINIMAL_STACK_SIZE * 4)

// VFO control process parameters
#define VFO_PROC_START_NAME				"vfo"
#define VF0_PROC_START_DELAY			400
#define VFO_PROC_SLEEP_TIME				portMAX_DELAY
#define VFO_PROC_PRIORITY				osPriorityNormal
#define VFO_PROC_STACK_SIZE				(configMINIMAL_STACK_SIZE * 4)

// Battery management system process parameters
#define BMS_PROC_START_NAME				"bms"
#define BMS_PROC_START_DELAY			500
#define BMS_PROC_SLEEP_TIME				500
#define BMS_PROC_PRIORITY				osPriorityNormal
#define BMS_PROC_STACK_SIZE				(configMINIMAL_STACK_SIZE * 16)	// FatFS access from gold file backup/flash

// Rotary encoders process parameters
#define ROTARY_PROC_START_NAME			"rot"
#define ROTARY_PROC_START_DELAY			600
#define ROTARY_PROC_SLEEP_TIME			50
#define ROTARY_PROC_PRIORITY			osPriorityNormal
#define ROTARY_PROC_STACK_SIZE			(configMINIMAL_STACK_SIZE * 4)

// Keypad process parameters
#define KEYPAD_PROC_START_NAME			"kbd"
#define KEYPAD_PROC_START_DELAY			700
#define KEYPAD_PROC_SLEEP_TIME			portMAX_DELAY
#define KEYPAD_PROC_PRIORITY			osPriorityNormal
#define KEYPAD_PROC_STACK_SIZE			(configMINIMAL_STACK_SIZE * 4)

// Band switching process parameters
#define BAND_PROC_START_NAME			"bnd"
#define BAND_PROC_START_DELAY			800
#define BAND_PROC_SLEEP_TIME			portMAX_DELAY
#define BAND_PROC_PRIORITY				osPriorityNormal
#define BAND_PROC_STACK_SIZE			(configMINIMAL_STACK_SIZE * 4)

// TRX control process parameters
#define TRX_PROC_START_NAME				"trx"
#define TRX_PROC_START_DELAY			900
#define TRX_PROC_SLEEP_TIME				portMAX_DELAY
#define TRX_PROC_PRIORITY				osPriorityNormal
#define TRX_PROC_STACK_SIZE				(configMINIMAL_STACK_SIZE * 4)

// FAN control process parameters
#define FAN_PROC_START_NAME				"fan"
#define FAN_PROC_START_DELAY			950
#define FAN_PROC_SLEEP_TIME				portMAX_DELAY
#define FAN_PROC_PRIORITY				osPriorityNormal
#define FAN_PROC_STACK_SIZE				(configMINIMAL_STACK_SIZE * 2)

// ICC(inter-core comms) process parameters
#define ICC_PROC_START_NAME				"icc"
#define ICC_PROC_START_DELAY			1000
#define ICC_PROC_SLEEP_TIME				portMAX_DELAY
#define ICC_PROC_PRIORITY				osPriorityAboveNormal
#define ICC_PROC_STACK_SIZE				(configMINIMAL_STACK_SIZE * 8)
// Sleep time while a WSPR capture is streaming from the M4 core - the
// icc task polls the M4 chunk ring instead of sleeping forever
#define ICC_WSPR_POLL_TIME				20

// App loader service parameters
#define APP_PROC_START_NAME				"app"
#define APP_PROC_START_DELAY			2000
#define APP_PROC_SLEEP_TIME				portMAX_DELAY
#define APP_PROC_PRIORITY				osPriorityNormal
#define APP_PROC_STACK_SIZE				(configMINIMAL_STACK_SIZE * 64)

// Lora driver parameters
#define LORA_PROC_START_NAME			"lor"
#define LORA_PROC_START_DELAY			3000
#define LORA_PROC_SLEEP_TIME			portMAX_DELAY
#define LORA_PROC_PRIORITY				osPriorityNormal
#define LORA_PROC_STACK_SIZE			(configMINIMAL_STACK_SIZE * 16)

// WSPR decoder process parameters
// Priority below normal - decode takes seconds of pure number crunching
// and must not disturb the UI or the real time tasks
//
// Note: priorities here go to raw xTaskCreate, not osThreadCreate. The
// CMSIS osPriorityLow enum is -3, which wraps unsigned and clamps to the
// HIGHEST FreeRTOS priority - the exact opposite of the intent. Use the
// raw FreeRTOS idle priority so the decode round-robins with the other
// normal tasks instead of starving the whole system
#define WSPR_PROC_START_NAME			"wpr"
#define WSPR_PROC_START_DELAY			3500
#define WSPR_PROC_SLEEP_TIME			portMAX_DELAY
#define WSPR_PROC_PRIORITY				tskIDLE_PRIORITY
#define WSPR_PROC_STACK_SIZE			(configMINIMAL_STACK_SIZE * 16)
// Uncomment to arm the WSPR monitor automatically at boot (bench testing,
// no UI hook needed) - captures every even minute and decodes to SD
//#define WSPR_MONITOR_AUTO_START

// MarsChat prototype process parameters (same priority reasoning as WSPR)
#define MARSCHAT_PROC_START_NAME		"mch"
#define MARSCHAT_PROC_START_DELAY		4000
#define MARSCHAT_PROC_SLEEP_TIME		portMAX_DELAY
#define MARSCHAT_PROC_PRIORITY			tskIDLE_PRIORITY
#define MARSCHAT_PROC_STACK_SIZE		(configMINIMAL_STACK_SIZE * 16)
// Uncomment for the closed-loop bench test: sends a MarsChat "HELLO"
// beacon over the CLK1 loopback injector at every even minute +1s.
// Arm WSPR_MONITOR_AUTO_START too - the radio then captures and decodes
// its own signal (no emissions, PA never keyed)
//#define MARSCHAT_LOOPBACK_BEACON

// GNSS driver parameters
#define GPS_PROC_START_NAME				"gps"
#define GPS_PROC_START_DELAY			5000
#define GPS_PROC_SLEEP_TIME				portMAX_DELAY
#define GPS_PROC_PRIORITY				osPriorityNormal
#define GPS_PROC_STACK_SIZE				(configMINIMAL_STACK_SIZE * 8)

#endif
