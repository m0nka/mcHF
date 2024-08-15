/************************************************************************************
**                                                                                 **
**                             mcHF Pro QRP Transceiver                            **
**                         Krassi Atanassov - M0NKA, 2013-2021                     **
**                                                                                 **
**---------------------------------------------------------------------------------**
**                                                                                 **
**  File name:                                                                     **
**  Description:                                                                   **
**  Last Modified:                                                                 **
**  Licence:       The mcHF project is released for radio amateurs experimentation **
**               and non-commercial use only.Check 3rd party drivers for licensing **
************************************************************************************/
#ifndef __PROC_CONF_H
#define __PROC_CONF_H

// -----------------------------------------------------------------------------
// Firmware execution context (On/Off of any system process)
//
// Nothing much here except quick initial stack setup, basic hw init, MMU On
// clocks and start the OS (IRQ stack usage as well)
#define CONTEXT_RESET_VECTOR

// DISCO BOARD
#ifdef BOARD_EVAL_747
#define CONTEXT_VIDEO					// Video driver
//#define CONTEXT_DRIVER_KEYPAD			// ??
//#define CONTEXT_ROTARY				// Encoders input processing
//#define CONTEXT_IPC_PROC				// Processor communication: 	STM32 <-> ESP32
#define CONTEXT_ICC						// Core to core communication: 	M7 <-> M4
//#define CONTEXT_TOUCH					// Touch screen
#endif

// BMS TEST BOARD
#ifdef BOARD_TEST_BMS
#define CONTEXT_BMS						// Battery Management System
#endif

//
// -----------------------------------------------------------------------------------------------
// High level video driver
//
#define CONTEXT_VIDEO
//
// -----------------------------------------------------------------------------------------------
// Touch screen process
//
#define CONTEXT_TOUCH
//
// -----------------------------------------------------------------------------------------------
// Core to core communication: 	M7 <-> M4
//
#define CONTEXT_ICC
//
// -----------------------------------------------------------------------------------------------
// Processor communication: 	STM32 <-> ESP32
//
// == Keeping this off, with rev 0.8.1 HW causes the ESP32 to reset ==
// == constantly and generate huge QRM !!!                          ==
//
//#define CONTEXT_IPC_PROC
//
// -----------------------------------------------------------------------------------------------
// Encoders input processing
//
#define CONTEXT_ROTARY
//
// -----------------------------------------------------------------------------------------------
// VFO control
//
#define CONTEXT_VFO
//
// -----------------------------------------------------------------------------------------------
// Codec I2C control(M7, while SAI streaming in DSP core)
//
#define CONTEXT_AUDIO
//
// -----------------------------------------------------------------------------------------------
// Battery Management System
//
#define CONTEXT_BMS
//
// -----------------------------------------------------------------------------------------------
// Backlight control process
//
//#define CONTEXT_PWM
//
// -----------------------------------------------------------------------------------------------
// Band switching process
//
#define CONTEXT_BAND
//
// -----------------------------------------------------------------------------------------------
// Transmitter HW control/monitor
//
//
#define CONTEXT_TRX
//

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

// UI(video) process parameters
#define UI_PROC_START_DELAY				0
#define UI_PROC_SLEEP_TIME				UI_REFRESH_60HZ
#define UI_PROC_PRIORITY				osPriorityNormal
#define UI_PROC_STACK_SIZE				(configMINIMAL_STACK_SIZE * 64)

// LCD Touch process parameters
#define TOUCH_PROC_START_DELAY			500
#define TOUCH_PROC_SLEEP_TIME			portMAX_DELAY
#define TOUCH_PROC_PRIORITY				osPriorityNormal
#define TOUCH_PROC_STACK_SIZE			(configMINIMAL_STACK_SIZE * 4)

// ICC(inter-core comms) process parameters
#define ICC_PROC_START_DELAY			3000
#define ICC_PROC_SLEEP_TIME				portMAX_DELAY
#define ICC_PROC_PRIORITY				osPriorityAboveNormal
#define ICC_PROC_STACK_SIZE				(configMINIMAL_STACK_SIZE * 8)

// IPC(inter-processor comms) process parameters
#define IPC_PROC_START_DELAY			5000
#define IPC_PROC_SLEEP_TIME				50
#define IPC_PROC_PRIORITY				osPriorityNormal
#define IPC_PROC_STACK_SIZE				(configMINIMAL_STACK_SIZE * 8)

// Rotary encoders process parameters
#define ROTARY_PROC_START_DELAY			0
#define ROTARY_PROC_SLEEP_TIME			50
#define ROTARY_PROC_PRIORITY			osPriorityNormal
#define ROTARY_PROC_STACK_SIZE			(configMINIMAL_STACK_SIZE * 4)

// VFO control process parameters
#define VF0_PROC_START_DELAY			500
#define VFO_PROC_SLEEP_TIME				portMAX_DELAY
#define VFO_PROC_PRIORITY				osPriorityNormal
#define VFO_PROC_STACK_SIZE				(configMINIMAL_STACK_SIZE * 4)

// VFO control process parameters
#define AUDIO_PROC_START_DELAY			0
#define AUDIO_PROC_SLEEP_TIME			portMAX_DELAY
#define AUDIO_PROC_PRIORITY				osPriorityNormal
#define AUDIO_PROC_STACK_SIZE			(configMINIMAL_STACK_SIZE * 4)

// Battery management system process parameters
#define BMS_PROC_START_DELAY			10000
#define BMS_PROC_SLEEP_TIME				500
#define BMS_PROC_PRIORITY				osPriorityNormal
#define BMS_PROC_STACK_SIZE				(configMINIMAL_STACK_SIZE * 4)

// PWM ...
//..

// Band switching process parameters
#define BAND_PROC_START_DELAY			1000
#define BAND_PROC_SLEEP_TIME			portMAX_DELAY
#define BAND_PROC_PRIORITY				osPriorityNormal
#define BAND_PROC_STACK_SIZE			(configMINIMAL_STACK_SIZE * 4)

// TRX control process parameters
#define TRX_PROC_START_DELAY			2000
#define TRX_PROC_SLEEP_TIME				portMAX_DELAY
#define TRX_PROC_PRIORITY				osPriorityNormal
#define TRX_PROC_STACK_SIZE				(configMINIMAL_STACK_SIZE * 4)


#endif
