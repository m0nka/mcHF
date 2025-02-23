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
#ifndef UI_CONTROLS_LAYOUT_H
#define UI_CONTROLS_LAYOUT_H

#include "GUI.h"

#define CLASSIC_LAYOUT

#ifdef CLASSIC_LAYOUT

#define ID_WINDOW_VOLUME          	(GUI_ID_USER + 0x00)
#define ID_BUTTON_VOLUMET          	(GUI_ID_USER + 0x01)
#define ID_BUTTON_VOLUMEM          	(GUI_ID_USER + 0x02)
#define ID_BUTTON_VOLUMEP          	(GUI_ID_USER + 0x03)

#define ID_WINDOW_WIFI          	(GUI_ID_USER + 0x10)
#define ID_TEXT_WIFI          		(GUI_ID_USER + 0x11)

#define ID_WINDOW_CLOCK          	(GUI_ID_USER + 0x20)
#define ID_TEXT_CLOCK          		(GUI_ID_USER + 0x21)

#define ID_WINDOW_SPEC          	(GUI_ID_USER + 0x30)
#define ID_TEXT_SPEC         		(GUI_ID_USER + 0x31)

#define ID_WINDOW_FREQ          	(GUI_ID_USER + 0x40)

// ----------------------------------------------------------------------------
// Analogue S-meter
// frame cuts left and top ? ToDo: fix it!
#define S_METER_X					2
#define S_METER_Y					1
//
#define S_METER_FRAME_LEFT			1
#define S_METER_FRAME_RIGHT			0
#define S_METER_FRAME_TOP			1
#define S_METER_FRAME_BOTTOM		0	// ToDo: fix bottom right corner!!
//
#define S_METER_FRAME_WIDTH			3
#define S_METER_FRAME_CURVE			0	// corner radius actually

#define S_METER_SIZE_X 				(349 + S_METER_FRAME_WIDTH)
#define S_METER_SIZE_Y 				(140 + S_METER_FRAME_WIDTH)
//
// ----------------------------------------------------------------------------
// Combined waterfall/spectrum scope control
// -- big size version
// frame position - bottom of the screen
//
#define LCD_WIDTH					854
//
#define SW_FRAME_X					0
#define SW_FRAME_Y					95	//167
//
#define HEADER_Y_SIZE				20
//
#define SW_FRAME_WIDTH				0
#define SW_FRAME_CORNER_R			3
//
#define FOOTER_Y_SIZE				16
//
// spectrum only position
#define SCOPE_X						(SW_FRAME_X + SW_FRAME_WIDTH)
#define SCOPE_Y						(SW_FRAME_Y + HEADER_Y_SIZE + 2)
//
// spectrum only size
#define SCOPE_X_SIZE				(LCD_WIDTH - 0)	// 850, 796
#define SCOPE_Y_SIZE				112				// ~0x6F
//
// waterfall only position
#define WATERFALL_X					SCOPE_X
#define WATERFALL_Y					(SCOPE_Y + SCOPE_Y_SIZE + 0)
//
// waterfall only size
#define WATERFALL_X_SIZE			SCOPE_X_SIZE
#define WATERFALL_Y_SIZE			157 + 4				//SCOPE_Y_SIZE
//
//
// MENU button
#define MENU_X						0
#define MENU_Y						0
#define MENU_X_SIZE					0
#define MENU_Y_SIZE					0
//
// AUDIO button
#define AUDIO_X						0
#define AUDIO_Y						0
#define AUDIO_X_SIZE				0
#define AUDIO_Y_SIZE				0
//
// VFO button
#define VFO_X						0
#define VFO_Y						0
#define VFO_X_SIZE					0
#define VFO_Y_SIZE					0
//
// KEYBOARD button
#define KEYBOARD_X					0
#define KEYBOARD_Y					0
#define KEYBOARD_X_SIZE				0
#define KEYBOARD_Y_SIZE				0
//
// KEYBOARD button
#define KEYBOARD_X					0
#define KEYBOARD_Y					0
#define KEYBOARD_X_SIZE				0
#define KEYBOARD_Y_SIZE				0
//
// AGC/ATT button
#define AGCATT_X					0
#define AGCATT_Y					0
#define AGCATT_X_SIZE				0
#define AGCATT_Y_SIZE				0
//
// center/fixed button
#define CENTER_X					((SW_FRAME_X + SW_FRAME_X_SIZE - 2)/2 + 225)
#define CENTER_Y					(SW_FRAME_Y + 4)
#define CENTER_X_SIZE				65
#define CENTER_Y_SIZE				15
//
// Span control
#define SPAN_X						((SW_FRAME_X + SW_FRAME_X_SIZE - 2)/2 + 290)
#define SPAN_Y						(SW_FRAME_Y + 4)
#define SPAN_X_SIZE					65
#define SPAN_Y_SIZE					15
//
// Total size of control
#define SW_FRAME_X_SIZE				(LCD_WIDTH - 1)	// 853, 799
#define SW_FRAME_Y_SIZE				(HEADER_Y_SIZE + FOOTER_Y_SIZE + SCOPE_Y_SIZE + WATERFALL_Y_SIZE + SW_FRAME_WIDTH*2 + 1)
//
#define SW_CONTROL_X_SIZE			(SW_FRAME_X_SIZE + 1)
#define SW_CONTROL_Y_SIZE			(SW_FRAME_Y_SIZE + 1)
//
//
#if 0
// ----------------------------------------------------------------------------
// Combined waterfall/spectrum scope control
// -- medium size version
// frame position - bottom of the screen
#define SW_FRAME_X_MID				0
#define SW_FRAME_Y_MID				214
//
#define HEADER_Y_SIZE_MID			20
//
#define SW_FRAME_WIDTH_MID			2
#define SW_FRAME_CORNER_R_MID		3
//
#define FOOTER_Y_SIZE_MID			16
//
// spectrum only position
#define SCOPE_X_MID					(SW_FRAME_X_MID + SW_FRAME_WIDTH_MID)
#define SCOPE_Y_MID					(SW_FRAME_Y_MID + HEADER_Y_SIZE_MID + 2)
//
// spectrum only size
#define SCOPE_X_SIZE_MID			672
#define SCOPE_Y_SIZE_MID			112				// ~0x6F
//
// waterfall only position
#define WATERFALL_X_MID				SCOPE_X_MID
#define WATERFALL_Y_MID				(SCOPE_Y_MID + SCOPE_Y_SIZE_MID + 0)
//
// waterfall only size
#define WATERFALL_X_SIZE_MID		SCOPE_X_SIZE_MID
#define WATERFALL_Y_SIZE_MID		SCOPE_Y_SIZE_MID
//
// smooth button
#define SMOOTH_X_MID				((SW_FRAME_X_MID + SW_FRAME_X_SIZE_MID - 2)/2 + 80)
#define SMOOTH_Y_MID				(SW_FRAME_Y_MID + 4)
#define SMOOTH_X_SIZE_MID			65
#define SMOOTH_Y_SIZE_MID			15
//
// center/fixed button
#define CENTER_X_MID				((SW_FRAME_X_MID + SW_FRAME_X_SIZE_MID - 2)/2 + 160)
#define CENTER_Y_MID				(SW_FRAME_Y_MID + 4)
#define CENTER_X_SIZE_MID			65
#define CENTER_Y_SIZE_MID			15
//
// Span control
#define SPAN_X_MID					((SW_FRAME_X_MID + SW_FRAME_X_SIZE_MID - 2)/2 + 230)
#define SPAN_Y_MID					(SW_FRAME_Y_MID + 4)
#define SPAN_X_SIZE_MID				65
#define SPAN_Y_SIZE_MID				15
//
// Total size of control
#define SW_FRAME_X_SIZE_MID			(SCOPE_X_SIZE_MID + 3)
#define SW_FRAME_Y_SIZE_MID			(HEADER_Y_SIZE_MID + FOOTER_Y_SIZE_MID + SCOPE_Y_SIZE_MID + WATERFALL_Y_SIZE_MID + SW_FRAME_WIDTH_MID*2 + 1)
//
#define SW_CONTROL_X_SIZE_MID		(SW_FRAME_X_SIZE_MID + 1)
#define SW_CONTROL_Y_SIZE_MID		(SW_FRAME_Y_SIZE_MID + 1)
//
#endif
// ----------------------------------------------------------------------------
// Iambic keyer control
//
#define IAMB_KEYER_X				678
#define IAMB_KEYER_Y				214
//
#define IAMB_KEYER_SIZE_X			121
#define IAMB_KEYER_SIZE_Y			265
//
#define IAMB_BTN_TOP_X0				(IAMB_KEYER_X + 2)
#define IAMB_BTN_TOP_X1				(IAMB_KEYER_X + IAMB_KEYER_SIZE_X   - 2)
#define IAMB_BTN_TOP_Y0				(IAMB_KEYER_Y + 2)
#define IAMB_BTN_TOP_Y1				(IAMB_KEYER_Y + IAMB_KEYER_SIZE_Y/2 - 2)
//
#define IAMB_BTN_BTM_X0				(IAMB_KEYER_X + 2)
#define IAMB_BTN_BTM_X1				(IAMB_KEYER_X + IAMB_KEYER_SIZE_X   - 2)
#define IAMB_BTN_BTM_Y0				(IAMB_KEYER_Y + IAMB_KEYER_SIZE_Y/2 + 2)
#define IAMB_BTN_BTM_Y1				(IAMB_KEYER_Y + IAMB_KEYER_SIZE_Y   - 2)
//
// ----------------------------------------------------------------------------
// Frequency panel
//
#define FREQ_P_X					354
#define FREQ_P_Y					93
//
#define FREQ_P_SZ_X					445
#define FREQ_P_SZ_Y					70
//
// ----------------------------------------------------------------------------
// Digitizer state control
//
#define DIGITIZER_X					600
#define DIGITIZER_Y					2
//
//
//
//
// ----------------------------------------------------------------------------


// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// Desktop layouts - for now via conditional flags
// ToDo: implement as const struct array declaration, to allow dynamic loading
//
#define DESKTOP_LAYOUT_2
#ifdef DESKTOP_LAYOUT_2
// ----------------------------------------------------------------------------
// CPU Load control
//
#define CPU_L_X						572
#define CPU_L_Y						33
//
// ----------------------------------------------------------------------------
// DSP alive control
#define DSP_POS_X					300//356
#define DSP_POS_Y					30//96
//
#define DSP_POS_SIZE_X				48
#define DSP_POS_SIZE_Y				64
//
// ----------------------------------------------------------------------------
// Clock control
#define CLOCK_X						20
#define CLOCK_Y						1//145
//
#define CLOCK_SIZE_X				350
#define CLOCK_SIZE_Y				20
//
// ----------------------------------------------------------------------------
// SD Card control
#define SD_CARD_X					690
#define SD_CARD_Y					35
//
// ----------------------------------------------------------------------------
// Speaker control
#define SPEAKER_X					682
#define SPEAKER_Y					37
//
#define SPEAKER_SIZE_X				62
#define SPEAKER_SIZE_Y				50
//
// ----------------------------------------------------------------------------
// Filter
#define FILTER_X					357
#define FILTER_Y					1
//
#define FILTER_SIZE_X				438 - 15
#define FILTER_SIZE_Y				24
//
// ----------------------------------------------------------------------------
// Battery icon
#define BATTERY_X					750
#define BATTERY_Y					32
//
#define BATTERY_SIZE_X				30
#define BATTERY_SIZE_Y				54
//
// ----------------------------------------------------------------------------
// TX Status control
//
#define TX_STAT_X					357
#define TX_STAT_Y					32

#define TX_STAT_SIZE_X				210
#define TX_STAT_SIZE_Y				56
//
// ----------------------------------------------------------------------------
// Filter
#define MENU_BTN_X					788
#define MENU_BTN_Y					0
//
#define MENU_BTN_SIZE_X				64
#define MENU_BTN_SIZE_Y				88
//
// ----------------------------------------------------------------------------
#endif

#else	// CLASSIC_LAYOUT
#define ID_WINDOW_VOLUME          	(GUI_ID_USER + 0x00)
#define ID_BUTTON_VOLUMET          	(GUI_ID_USER + 0x01)
#define ID_BUTTON_VOLUMEM          	(GUI_ID_USER + 0x02)
#define ID_BUTTON_VOLUMEP          	(GUI_ID_USER + 0x03)

#define ID_WINDOW_WIFI          	(GUI_ID_USER + 0x10)
#define ID_TEXT_WIFI          		(GUI_ID_USER + 0x11)

#define ID_WINDOW_CLOCK          	(GUI_ID_USER + 0x20)
#define ID_TEXT_CLOCK          		(GUI_ID_USER + 0x21)

#define ID_WINDOW_SPEC          	(GUI_ID_USER + 0x30)
#define ID_TEXT_SPEC         		(GUI_ID_USER + 0x31)

#define ID_WINDOW_FREQ          	(GUI_ID_USER + 0x40)

// ----------------------------------------------------------------------------
// Analogue S-meter
//
#define S_METER_X					1
#define S_METER_Y					337
//
#define S_METER_FRAME_LEFT			1
#define S_METER_FRAME_RIGHT			0
#define S_METER_FRAME_TOP			1
#define S_METER_FRAME_BOTTOM		0	// ToDo: fix bottom right corner!!
//
#define S_METER_FRAME_WIDTH			3
#define S_METER_FRAME_CURVE			0	// corner radius actually

#define S_METER_SIZE_X 				(349 + S_METER_FRAME_WIDTH)
#define S_METER_SIZE_Y 				(140 + S_METER_FRAME_WIDTH)
//
// ----------------------------------------------------------------------------
// Combined waterfall/spectrum scope control
// -- big size version
// frame position - bottom of the screen
//
#define LCD_WIDTH					800  //854
//
#define SW_FRAME_X					1
#define SW_FRAME_Y					1
//
#define HEADER_Y_SIZE				20
//
#define SW_FRAME_WIDTH				2
#define SW_FRAME_CORNER_R			3
//
#define FOOTER_Y_SIZE				16
//
// spectrum only position
#define SCOPE_X						(SW_FRAME_X + SW_FRAME_WIDTH)
#define SCOPE_Y						(SW_FRAME_Y + HEADER_Y_SIZE + 2)
//
// spectrum only size
#define SCOPE_X_SIZE				(LCD_WIDTH - 4)	// 850, 796
#define SCOPE_Y_SIZE				112				// ~0x6F
//
// waterfall only position
#define WATERFALL_X					SCOPE_X
#define WATERFALL_Y					(SCOPE_Y + SCOPE_Y_SIZE + 0)
//
// waterfall only size
#define WATERFALL_X_SIZE			SCOPE_X_SIZE
#define WATERFALL_Y_SIZE			157				//SCOPE_Y_SIZE
//
// smooth button
#define SMOOTH_X					((SW_FRAME_X + SW_FRAME_X_SIZE - 2)/2 + 115)
#define SMOOTH_Y					(SW_FRAME_Y + 4)
#define SMOOTH_X_SIZE				65
#define SMOOTH_Y_SIZE				15
//
// center/fixed button
#define CENTER_X					((SW_FRAME_X + SW_FRAME_X_SIZE - 2)/2 + 210)
#define CENTER_Y					(SW_FRAME_Y + 4)
#define CENTER_X_SIZE				65
#define CENTER_Y_SIZE				15
//
// Span control
#define SPAN_X						((SW_FRAME_X + SW_FRAME_X_SIZE - 2)/2 + 290)
#define SPAN_Y						(SW_FRAME_Y + 4)
#define SPAN_X_SIZE					65
#define SPAN_Y_SIZE					15
//
// Total size of control
#define SW_FRAME_X_SIZE				(LCD_WIDTH - 1)	// 853, 799
#define SW_FRAME_Y_SIZE				(HEADER_Y_SIZE + FOOTER_Y_SIZE + SCOPE_Y_SIZE + WATERFALL_Y_SIZE + SW_FRAME_WIDTH*2 + 1)
//
#define SW_CONTROL_X_SIZE			(SW_FRAME_X_SIZE + 1)
#define SW_CONTROL_Y_SIZE			(SW_FRAME_Y_SIZE + 1)
//
//
// ----------------------------------------------------------------------------
// Iambic keyer control
//
#define IAMB_KEYER_X				678
#define IAMB_KEYER_Y				214
//
#define IAMB_KEYER_SIZE_X			121
#define IAMB_KEYER_SIZE_Y			265
//
#define IAMB_BTN_TOP_X0				(IAMB_KEYER_X + 2)
#define IAMB_BTN_TOP_X1				(IAMB_KEYER_X + IAMB_KEYER_SIZE_X   - 2)
#define IAMB_BTN_TOP_Y0				(IAMB_KEYER_Y + 2)
#define IAMB_BTN_TOP_Y1				(IAMB_KEYER_Y + IAMB_KEYER_SIZE_Y/2 - 2)
//
#define IAMB_BTN_BTM_X0				(IAMB_KEYER_X + 2)
#define IAMB_BTN_BTM_X1				(IAMB_KEYER_X + IAMB_KEYER_SIZE_X   - 2)
#define IAMB_BTN_BTM_Y0				(IAMB_KEYER_Y + IAMB_KEYER_SIZE_Y/2 + 2)
#define IAMB_BTN_BTM_Y1				(IAMB_KEYER_Y + IAMB_KEYER_SIZE_Y   - 2)
//
// ----------------------------------------------------------------------------
// Frequency panel
//
#define FREQ_P_X					354
#define FREQ_P_Y					93
//
#define FREQ_P_SZ_X					445
#define FREQ_P_SZ_Y					70
//
// ----------------------------------------------------------------------------
// Digitizer state control
//
#define DIGITIZER_X					600
#define DIGITIZER_Y					2
//
//
//
//
// ----------------------------------------------------------------------------


// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// Desktop layouts - for now via conditional flags
// ToDo: implement as const struct array declaration, to allow dynamic loading
//
#define DESKTOP_LAYOUT_2
#ifdef DESKTOP_LAYOUT_2
// ----------------------------------------------------------------------------
// CPU Load control
//
#define CPU_L_X						360
#define CPU_L_Y						35
//
// ----------------------------------------------------------------------------
// DSP alive control
#define DSP_POS_X					480
#define DSP_POS_Y					35
//
// ----------------------------------------------------------------------------
// Clock control
#define CLOCK_X						540
#define CLOCK_Y						56
//
#define CLOCK_SIZE_X				90
#define CLOCK_SIZE_Y				18
//
// ----------------------------------------------------------------------------
// SD Card control
#define SD_CARD_X					690
#define SD_CARD_Y					35
//
// ----------------------------------------------------------------------------
// Speaker control
#define SPEAKER_X					738
#define SPEAKER_Y					35
//
#define SPEAKER_SIZE_X				62
#define SPEAKER_SIZE_Y				50
//
// ----------------------------------------------------------------------------
// Filter
#define FILTER_X					360
#define FILTER_Y					1
//
#define FILTER_SIZE_X				438
#define FILTER_SIZE_Y				24
//
// ----------------------------------------------------------------------------
// Speaker control
#define WIFI_X						500
#define WIFI_Y						75
//
#define WIFI_SIZE_X					80
#define WIFI_SIZE_Y					15
//
// ----------------------------------------------------------------------------
#endif

#endif	// CLASSIC_LAYOUT

__attribute__((__common__)) struct WidgetBounds {

	ushort 	x;
	ushort	y;
	ushort	x_size;
	ushort	y_size;

} WidgetBounds;

#endif
