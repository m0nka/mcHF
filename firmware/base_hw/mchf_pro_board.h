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
#ifndef __MCHF_PRO_BOARD_H
#define __MCHF_PRO_BOARD_H

#define USE_DSP_CORE

#include "mchf_types.h"

#ifndef BOOTLOADER
//#include "mchf_radio_decl.h"
#include "mchf_icc_def.h"
#include "cmsis_os.h"
#include "task.h"
#include "cpu_utils.h"
#include "virt_eeprom.h"
#endif

#include "mchf_pro_pinmap.h"
#include "proc_conf.h"
#include "mchf_icc_def.h"
//
#include <stdio.h>
#include <stddef.h>
//#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <limits.h>

#define USE_PWR_LDO_SUPPLY

//
// ToDo: Overclocking causes LCD driver to HardFault!
//
#define M7_CLOCK			400			//480
//#define	USE_LSE                     // xtal/caps no good on proto?

#define RADIO_FIRM_ADDR		0x08020000

// -----------------------------------------
//
// To change between version:
//
//		1. Change define here
//		2. Update include path (in Project Settings)
//		3. Update libs path    (in Project Settings)
//		4. Update emWin lib	   (in Project Settings)
//
//#define USE_EMWIN_5_40
//
//#define USE_EMWIN_5_44
//
#define USE_EMWIN_6_10

// -----------------------------------------------------------------------------
// Are we using internal CPU RAM or external SDRAM
//
// - bad soldering/connection with SDRAM will cause the emWin to hang in GUI_init()
// - might be safe to boot up with a simple UI, run some HW tests, then do full boot
//
//#define USE_INT_RAM
#define USE_SDRAM
//
#define	EMWIN_RAM_SIZE 				(1024*1024)
//
#define		UI_REFRESH_25HZ			(1000/25)
#define		UI_REFRESH_60HZ			(1000/60)

//#define 	LCD_LANE_CLK			62500
#define 	LCD_LANE_CLK			58750				// 58750
#define	 	ST7701_PIXEL_CLK  		(LCD_LANE_CLK/2)
//
// -----------------------------------------------------------------------------
// Oscillators configuration
//
// - Main
#define INT_16MHZ					0
#define EXT_16MHZ_XTAL				1
#define EXT_16MHZ_TCXO				2
// - Real Time Clock
#define INT_32KHZ					0
#define EXT_32KHZ_XTAL				1
//
//#define HSE_VALUE    				((uint32_t)16000000)
//#define HSI_VALUE    				((uint32_t)16000000)
//#define CSI_VALUE    				((uint32_t)4000000)
//
//#define VECT_TAB_OFFSET  			0x00				// alpha stage, base of flash
//#define VECT_TAB_OFFSET  			0x20000				// sector 1, we have bootloader at base
//

#define BKP_REG_RESET_REASON   		(RTC->BKP0R)
#define RESET_CLEAR					0
#define RESET_JUMP_TO_FW			1
#define RESET_POWER_OFF				2
#define RESET_UPDATE_FW				3
#define RESET_DSP_RELOAD			4

#define BKP_REG_DSP_ID   			(RTC->BKP1R)
#define DSP_IDLE					0
#define DSP_LOADED_CLINT			1
#define DSP_LOADED_UHSDR			2

// -----------------------------------------------------------------------------
//
#define		CHIP_UNIQUE_ID			0x1FF1E800
#define		CHIP_FLS_SIZE			0x1FF1E880
//
// -----------------------------------------------------------------------------
// Some better looking OS macros
//
#define EnterCriticalSection		taskENTER_CRITICAL
#define ExitCriticalSection			taskEXIT_CRITICAL
//#define OsDelayMs					osDelay
//
// -----------------------------------------------------------------------------
// Drivers start delays
// - start the UI driver first, the rest with delay, so UI can be painted first
//
#define BOOT_UP_LCD_DELAY			2000
//
//
//#define H7_200MHZ
#define H7_400MHZ
//
// -----------------------------------------------------------------------------
// Nice facility for testing the analogue s-meter via side encoder
//
//#define USE_SIDE_ENC_FOR_S_METER
//

#ifndef BOOTLOADER
// -----------------------------------------------------------------------------
//
#define	VFO_A					0
#define	VFO_B					1
//
// -----------------------------------------------------------------------------
// Public UI driver state
#define	MODE_DESKTOP			0
#define MODE_MENU				1
#define MODE_SIDE_ENC_MENU		2
#define MODE_DESKTOP_FT8		3
#define MODE_QUICK_LOG			4
//
#define THEME_0					0
#define THEME_1					1
#define THEME_2					2
#define MAX_THEME				3
//
__attribute__((__common__)) struct UI_DRIVER_STATE {
	// Current LCD state - Desktop or Menu
	// Request flag, updated by keypad driver, read only for the UI task
	uchar	req_state;
	// Accepted state by the UI driver(read/write local)
	uchar	cur_state;
	// Flag to lock out keyboard driver request updates while GUI repaints
	uchar	lock_requests;
	//
	uchar	theme_id;
	//
	uchar 	show_band_guide;
	//
} UI_DRIVER_STATE;
//
//
// SAI HW running on DSP core
#define	UI_NEW_SAI_INIT_DONE		1
// New FFT data available in buffer - obsolete
#define	UI_RXTX_SWITCH				2
// change of UI mode (desktop/menu/etc...)
#define	UI_NEW_MODE_EVENT			3
// Frequency updated
#define	UI_NEW_FREQ_EVENT			4
// Audio updated
#define	UI_NEW_AUDIO_EVENT			5
// Cleanup, for codec reload
#define	UI_NEW_SAI_CLEANUP			6
//
// -----------------------------------------------------------------------------
// DSP API codes
//
#define UI_ICC_CHANGE_CORE			0xFF
#define UI_ICC_PROC_BROADCART		1
#define UI_ICC_CHANGE_BAND			2
#define UI_ICC_NCO_FREQ				3
#define UI_ICC_DEMOD_MODE			4
#define UI_ICC_AGC_MODE				5
#define UI_ICC_FITER				6
#define UI_ICC_STEREO				7
#define UI_ICC_TUNE					8

#if 0
// The 16 bit msg id is used in the DSP handler directly
// to switch execution, the data buffer is for extra values
// So, 14 bytes might not be enough, but every task(control?) should allocate
// own static RAM copy, which reserves extra memory
// but is needed as this struct is passed in queue via pointer and
// needs to be valid while the message is propagating, so can't be temporary
// function stack that might not be valid in few uS, specially in fast
// UI redrawing routines
//
#define API_MAX_PAYLOAD					13
//
struct APIMessage {

	ushort 	usMessageID;
	uchar	ucPayload;
	uchar 	ucData[API_MAX_PAYLOAD];

} APIMessage;
// -----------------------------------------------------------------------------
// calls to DPS driver
//
#define DSP_MAX_PAYLOAD					50
//
struct DSPMessage {

	uchar 	ucMessageID;
	uchar	ucProcessDone;
	uchar	ucDataReady;
	char 	cData[DSP_MAX_PAYLOAD];

} DSPMessage;
#endif

#define	TASK_PROC_IDLE				0
#define	TASK_PROC_WORK				1
#define	TASK_PROC_DONE				2

__attribute__((__common__)) struct ESPMessage {

	uchar 	ucMessageID;
	uchar	ucProcStatus;
	uchar	ucDataReady;
	uchar	ucExecResult;

	ushort	usPayloadSize;

	uchar 	ucData[128];

} ESPMessage;

// -----------------------------------------------------------------------------
// Hardware regs, read before MMU init
__attribute__((__common__)) struct CM7_CORE_DETAILS {

	// X and Y coordinates on the wafer
	ulong 	wafer_coord;
	// Bits 31:8 UID[63:40]: LOT_NUM[23:0] 	- Lot number (ASCII encoded)
	// Bits 7:0 UID[39:32]: WAF_NUM[7:0] 	- Wafer number (8-bit unsigned number)
	ulong	wafer_number;
	// Bits 31:0 UID[95:64]: LOT_NUM[55:24]	- Lot number (ASCII encoded)
	ulong	lot_number;
	// Flash size
	ushort 	fls_size;
	ushort	dummy;

} CM7_CORE_DETAILS;
//

// Per band settings
__attribute__((__common__)) struct BAND_INFO {

	// Frequency boundary
	ulong 	band_start;
	ulong	band_end;

	// Common controls
	uchar 	volume;
	uchar 	demod_mode;
	uchar 	filter;
	uchar 	atten;

	ulong	step;

	// Frequency
	ulong	vfo_a;
	ulong	vfo_b;

	short 	nco_freq;
	uchar	fixed_mode;
	uchar   active_vfo;			// A, B

	ulong   span;

	uchar 	st_volume;
	uchar	audio_balance;
	// Align four!

	// Transmiter
	uchar	power_factor;
	uchar	tx_power;

} BAND_INFO;
//
// Public structure to hold current radio state
__attribute__((__common__)) struct TRANSCEIVER_STATE_UI {

	// Per band info
	struct BAND_INFO	band[MAX_BANDS];

	// -----------------------------
	// Clock to use
	uchar 	main_clk;
	// RCC clock to use
	uchar 	rcc_clk;
	ushort 	reset_reason;
	// --
	// -----------------------------
	// DSP status
	uchar 	dsp_alive;
	uchar 	dsp_seq_number_old;
	uchar 	dsp_seq_number;
	uchar 	dsp_blinker;
	// --
	uchar 	dsp_rev1;
	uchar 	dsp_rev2;
	uchar 	dsp_rev3;
	uchar 	dsp_rev4;
	// --
	ulong 	dsp_freq;
	short 	dsp_nco_freq;
	uchar 	dsp_step_idx;
	uchar 	step_idx;
	// --
	uchar 	dsp_volume;
	uchar 	dsp_filter;
	uchar   dsp_band;
	uchar 	dsp_demod;

	uchar	pcb_rev;
	// --
	// -----------------------------
	// Local status
	uchar	agc_mode;
	uchar	rf_gain;
	// --
	uchar 	audio_mute_flag;
	// ---
	uchar 	curr_band;
	// --
	ulong	step;
	// --
	uchar	cw_tx_state;		// 0 - idle, 1 - on, 2 - release
	uchar	cw_iamb_type;

	uchar	eeprom_init_done;

	uchar 	rxtx;

	int		wifi_rssi;
	uchar	wifi_on;
	uchar	esp32_alive;
	uchar	esp32_blink;
	char    esp32_version[16];

	uchar	active_side_enc_id;
	uchar	stereo_mode;
	uchar	keyer_mode;
	uchar 	tune;

	ushort	bias0;
	ushort 	bias1;

	uchar	demo_mode;

	// We need new definition called full span, which is opposite
	// what we called 'frequency translate' in lower than v 0.7.
	// This would allow showing full span of the sampling rate,
	// half span (the half sampling rate + translate shift, so DC offset is removed,
	// and even smaller, zoomed in chunks of spectrum
	uchar	use_full_span;

	// Always align last member!

} TRANSCEIVER_STATE_UI;
//
#define	SW_CONTROL_BIG				0
#define	SW_CONTROL_MID				1
#define	SW_CONTROL_SMALL			2
//
// Spectrum/Waterfall control publics
__attribute__((__common__)) struct UI_SW {
	//
	// screen pixel data
	uchar 	fft_value[854];
	// FFT array from DSP
	uchar 	fft_dsp[1024];
	//
	uchar 	ctrl_type;
	//
	uchar	updated;
	// --
	ushort 	bandpass_start;
	ushort 	bandpass_end;

	uchar   sm_value;

} UI_SW;
//
#endif

#if 1
#define 	T_STEP_1HZ						1
#define 	T_STEP_10HZ						10
#define 	T_STEP_100HZ					100
#define 	T_STEP_1KHZ						1000
#define 	T_STEP_10KHZ					10000
#define 	T_STEP_100KHZ					100000
#define		T_STEP_1MHZ						1000000		// Used for transverter offset adjust
#define		T_STEP_10MHZ					10000000	// Used for transverter offset adjust
//
enum {
	T_STEP_1HZ_IDX = 0,
	T_STEP_10HZ_IDX,
	T_STEP_100HZ_IDX,
	T_STEP_1KHZ_IDX,
	T_STEP_10KHZ_IDX,
	T_STEP_100KHZ_IDX,
	T_STEP_1MHZ_IDX,
	T_STEP_10MHZ_IDX,
	T_STEP_MAX_STEPS
};
#endif


#define MAX_AUDIO_LEVEL			16

// -----------------------------------------------------------------------------
// HAL compatibility
//#define assert_param(expr) ((void)0)
//
//typedef enum
//{
//  RESET = 0,
//  SET = !RESET
//} FlagStatus, ITStatus;
//
//typedef enum
//{
//  DISABLE = 0,
//  ENABLE = !DISABLE
//} FunctionalState;
#define IS_FUNCTIONAL_STATE(STATE) (((STATE) == DISABLE) || ((STATE) == ENABLE))
//
//typedef enum
//{
//  ERROR = 0,
//  SUCCESS = !ERROR
//} ErrorStatus;
//
#define SET_BIT(REG, BIT)     ((REG) |= (BIT))
#define CLEAR_BIT(REG, BIT)   ((REG) &= ~(BIT))
#define READ_BIT(REG, BIT)    ((REG) & (BIT))
#define CLEAR_REG(REG)        ((REG) = (0x0))
#define WRITE_REG(REG, VAL)   ((REG) = (VAL))
#define READ_REG(REG)         ((REG))
#define MODIFY_REG(REG, CLEARMASK, SETMASK)  WRITE_REG((REG), (((READ_REG(REG)) & (~(CLEARMASK))) | (SETMASK)))
#define POSITION_VAL(VAL)     (__CLZ(__RBIT(VAL)))

// -----------------------------------------------------------------------------

typedef void FAST_REFRESH(void);

// Use FreeRTOS allocator
#ifdef RADIO
#define malloc 					pvPortMalloc
#define free					vPortFree
#endif

// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
// CPU exceptions - main.c
void 	NMI_Handler(void);
void 	HardFault_Handler(void);
void 	MemManage_Handler(void);
void 	BusFault_Handler(void);
void 	UsageFault_Handler(void);
void 	SVC_Handler(void);
void 	PendSV_Handler(void);
void 	SysTick_Handler(void);

// Exports in board file
void 	mchf_pro_board_debug_led_init(void);
void 	mchf_pro_board_blink_if_alive(uchar flags);

void 	mchf_pro_board_read_cpu_details(void);
void 	mchf_pro_board_start_gpio_clocks(void);

void 	mchf_pro_board_mpu_config(void);
void 	mchf_pro_board_cpu_cache_enable(void);

uchar 	mchf_pro_board_system_clock_config(uchar clk_src);
uchar 	mchf_pro_board_rtc_clock_config(uchar clk_src);
void 	mchf_pro_board_rtc_clock_disable(void);

void 	mchf_pro_board_swo_init(void);
void 	mchf_pro_board_mco2_on(void);

void 	mchf_pro_board_sensitive_hw_init(void);

void 	SystemClockChange_Handler(void);
void 	SystemClock_Config(void);
void 	MPU_Config(void);
void 	CPU_CACHE_Enable(void);

// in main.c !
void 	transceiver_init_eep_defaults(void);

void 	printf_init(uchar is_shared);
void 	print_hex_array(uchar *pArray, ushort aSize);

// bsp.c
void power_off(void);
void power_off_a(void);
#endif
