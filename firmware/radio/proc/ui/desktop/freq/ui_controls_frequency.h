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
#ifndef __UI_CONTROLS_FREQUENCY
#define __UI_CONTROLS_FREQUENCY

//#define FRAME_MAIN_DIAL
#define FREQ_ENABLE_SECOND

//#define FREQ_REPAINT_FIRST			0x01
//#define FREQ_REPAINT_SECOND			0x02

#define FREQ_FONT					GUI_FONT_D24X32

#define FREQ_FONT_SIZE_X			24
#define FREQ_FONT_SIZE_Y			32

// VFO B decl
#define FREQ_FONT_SIZE1_X			12
#define FREQ_FONT_SIZE1_Y			17
//
#define VFO_B_SEG_SEL_COLOR			0x00E0E0E0
#define VFO_B_SEG_OFF_COLOR			GUI_LIGHTGRAY
#define VFO_B_SEG_ON_COLOR			GUI_WHITE

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

//#ifdef CLASSIC_LAYOUT
#define M_FREQ_X					531 + 54
#define M_FREQ_Y					122
#define M_FREQ1_X					356	+ 54
#define M_FREQ1_Y					136
//#else
//#define M_FREQ_X					531
//#define M_FREQ_Y					350
//#define M_FREQ1_X					356
//#define M_FREQ1_Y					364
//#endif
//
// ----------------------------------------------------------------------------
// Band control
#define BAND_X						696 + 54
#define BAND_Y						95
// ----------------------------------------------------------------------------
// Step
#define VFO_STEP_X					414 + 54
#define VFO_STEP_Y					95
//
#define VFO_STEP_SIZE_X				52
#define VFO_STEP_SIZE_Y				20
// ----------------------------------------------------------------------------
// RX/TX
#define RADIO_MODE1_X				360 + 54
#define RADIO_MODE1_Y				122
//
// ----------------------------------------------------------------------------
// Decoder
#define DECODER_MODE_X				356 + 54
#define DECODER_MODE_Y				95
//
// ----------------------------------------------------------------------------
// AGC control
#define AGC_X						473 + 54
#define AGC_Y						95

// ----------------------------------------------------------------------------
// RX/TX indicator
#define RXTX_X						415 + 54
#define RXTX_Y						122

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
