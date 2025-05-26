/************************************************************************************
**                                                                                 **
**                             mcHF Pro QRP Transceiver                            **
**                         Krassi Atanassov - M0NKA, 2013-2025                     **
**                                                                                 **
**---------------------------------------------------------------------------------**
**                                                                                 **
**  File name:                                                                     **
**  Description:                                                                   **
**  Last Modified:                                                                 **
**  Licence:               GNU GPLv3                                               **
************************************************************************************/
#ifndef __UI_CONTROLS_FREQUENCY
#define __UI_CONTROLS_FREQUENCY

// ----------------------------------------------------------------------------
// Allow for segment masking to be visible, so we can adjust size(testing only)
//#define SHOW_MASKING
#define MASKING_COLOR				GUI_LIGHTBLUE
//
// ----------------------------------------------------------------------------
//#define FRAME_MAIN_DIAL
#define FREQ_ENABLE_SECOND

//#define FREQ_REPAINT_FIRST			0x01
//#define FREQ_REPAINT_SECOND			0x02

#define FREQ_FONT					GUI_FONT_D36X48//GUI_FONT_D24X32

#define FREQ_FONT_SIZE_X			36	//24
#define FREQ_FONT_SIZE_Y			48	//32

// VFO B decl
#define FREQ_FONT_SIZE1_X			12
#define FREQ_FONT_SIZE1_Y			17
//
#define VFO_B_SEG_SEL_COLOR			GUI_MAKE_COLOR(0x00DADADA)	//0x00E0E0E0
#define VFO_B_SEG_OFF_COLOR			GUI_LIGHTGRAY
#define VFO_B_SEG_ON_COLOR			GUI_WHITE

#define VFO_A_SEL_DIGIT_COLOR		GUI_GRAY

// Frequency public structure
typedef struct DialFrequency
{
	uchar	last_active_vfo;
	ulong	last_screen_step;

	// VFO A
	ulong 	vfo_a_scr_freq;
	uchar	vfo_a_segments_invalid;
	//
	uchar	dial_100_mhz;
	uchar	dial_010_mhz;
	uchar	dial_001_mhz;
	uchar	dial_100_khz;
	uchar	dial_010_khz;
	uchar	dial_001_khz;
	uchar	dial_100_hz;
	uchar	dial_010_hz;
	uchar	dial_001_hz;

	// VFO B
	ulong 	vfo_b_scr_freq;
	uchar	vfo_b_segments_invalid;
	//
	uchar	sdial_100_mhz;
	uchar	sdial_010_mhz;
	uchar	sdial_001_mhz;
	uchar	sdial_100_khz;
	uchar	sdial_010_khz;
	uchar	sdial_001_khz;
	uchar	sdial_100_hz;
	uchar	sdial_010_hz;
	uchar	sdial_001_hz;

} DialFrequency;

// Exports
void ui_controls_agc_init(void);
WM_HWIN ui_controls_frequency_init(WM_HWIN hParent);
void ui_controls_frequency_quit(void);
void ui_controls_vfo_step_refresh(void);

int ui_controls_frequency_refresh(uchar flags);
void ui_controls_demod_refresh(void);

void ui_controls_frequency_change_band(void);
void ui_controls_band_refresh(void);

//void ui_controls_frequency_rebuild_second(void);

#endif
