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

// Bambu Basic Light Gray (10104) - #D1D3D5
#define APPLE_MAC_GREY	GUI_MAKE_COLOR(0x00D5D3D1)

// Bambu Basic Hot Pink (10204) - #F5547C
#define HOT_PINK		GUI_MAKE_COLOR(0x007C54F5)

#define GUI_STCOLOR_LIGHTBLUE   		0x00DCA939
#define GUI_STCOLOR_DARKBLUE    		0x00522000

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
#define S_METER_SY					25
//
#define S_METER_MAX					330
//
#define S_METER_FRAME_LEFT			1
#define S_METER_FRAME_RIGHT			0
#define S_METER_FRAME_TOP			1
#define S_METER_FRAME_BOTTOM		0	// ToDo: fix bottom right corner!!
//
#define S_METER_FRAME_WIDTH			3
#define S_METER_FRAME_CURVE			0	// corner radius actually

#define S_METER_SIZE_X 				(S_METER_MAX + 26)
#define S_METER_SIZE_Y 				100
//
// ----------------------------------------------------------------------------
// Combined waterfall/spectrum scope control
// -- big size version
// frame position - bottom of the screen
//
#define LCD_WIDTH					854
//
#define SW_FRAME_X					0
#define SW_FRAME_Y					202		//112
//
#define HEADER_Y_SIZE				0
//
#define SW_FRAME_WIDTH				2
#define SW_FRAME_CORNER_R			5
//
#define FOOTER_Y_SIZE				0
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
#define WATERFALL_Y_SIZE			146	// (157 + 4) , SCOPE_Y_SIZE
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

//#ifdef CLASSIC_LAYOUT
#define M_FREQ_X					450
#define M_FREQ_Y					130

#define M_FREQ1_X					365
#define M_FREQ1_Y					68
//
#define M_FREQ1_X_SZ				188
//#else
//#define M_FREQ_X					531
//#define M_FREQ_Y					350
//#define M_FREQ1_X					356
//#define M_FREQ1_Y					364
//#endif
//
// ----------------------------------------------------------------------------
// Band control
#define BAND_X						(M_FREQ_X + FREQ_FONT_SIZE_X*4 + 7)
#define BAND_Y						(M_FREQ_Y - 24)
// ----------------------------------------------------------------------------
// Step
#define VFO_STEP_X					(M_FREQ_X + FREQ_FONT_SIZE_X*7 + 12)
#define VFO_STEP_Y					(M_FREQ_Y - 24)
//
#define VFO_STEP_SIZE_X				52
#define VFO_STEP_SIZE_Y				20

#define VFO_A_X						(M_FREQ_X + FREQ_FONT_SIZE_X*9 + 7)
#define VFO_A_Y						(M_FREQ_Y - 22)
// ----------------------------------------------------------------------------
// RX/TX
#define RADIO_MODE1_X				360 + 54
#define RADIO_MODE1_Y				440
//
// ----------------------------------------------------------------------------
// Decoder
#define DECODER_MODE_X				481
#define DECODER_MODE_Y				38
//
#define DEC_MODE_X_SZ				72
//
// ----------------------------------------------------------------------------
// AGC control
#define AGC_X						365
#define AGC_Y						10

// ----------------------------------------------------------------------------
// RX/TX indicator
#define RXTX_X						(M_FREQ_X + FREQ_FONT_SIZE_X*1 + 12)// 80
#define RXTX_Y						(M_FREQ_Y - 15)						// 86

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
#define CLOCK_X						30
#define CLOCK_Y						S_METER_SIZE_Y + 65
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
#define SPEAKER_X					645
#define SPEAKER_Y					50
//
#define SPEAKER_SIZE_X				62
#define SPEAKER_SIZE_Y				50

#define RF_GAIN_X					725
#define RF_GAIN_Y					50

#define TX_STAT_X					805
#define TX_STAT_Y					50

#define TX_STAT_SIZE_X				60
#define TX_STAT_SIZE_Y				56
//
// ----------------------------------------------------------------------------
// Filter
#define FILTER_X					366
#define FILTER_Y					38
//
#define FILTER_SIZE_X				110
#define FILTER_SIZE_Y				22
//
// ----------------------------------------------------------------------------
// Battery icon
#define BATTERY_X					365
#define BATTERY_Y					110
//
#define BATTERY_SIZE_X				75
#define BATTERY_SIZE_Y				72
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
