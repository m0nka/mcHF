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
#ifndef UI_CONTROLS_SPECTRUM_H
#define UI_CONTROLS_SPECTRUM_H

#include "GUI.h"
//#include "arm_math.h"

// Definitions in pixels,not Hz!
//
// Centre of the scope in scope pixel coordinates - the column the tuned
// carrier (FFT bin 512, which is DC/the LO) actually lands on. The bin to
// pixel mapping in ui_controls_spectrum_fft_process_big() drops the centre
// bin at SCOPE_X_SIZE/2, so on this 800 px panel that is 398. It used to be
// 427, which is the middle of the 854 px 5 inch panel (STARTEK_5INCH) - on
// the 4.3 inch build that put the passband strip, the red centre line and
// the centre frequency label 29 px (about 1.4 kHz) to the right of where
// the signal really is
#define SPECTRUM_MID_POINT			(SCOPE_X_SIZE/2)	// 398; was 427 = 854/2
#define SPECTRUM_DEF_HALF_BW		20

// This timeout min value is 11!
#define BAND_GUIDE_FADE_TRIG_VAL	10
#define BAND_GUIDE_TIMEOUT			(BAND_GUIDE_FADE_TRIG_VAL + 30)
#define BAND_GUIDE_FADE_FACTOR		8
#define BAND_GUIDE_START_ALPHA		88
//
#define BAND_GUIDE_LEFT_LABLE_X		((SW_FRAME_X +                  0) +  20)
#define BAND_GUIDE_MIDP_LABLE_X		((SW_FRAME_X + SPECTRUM_MID_POINT) - 250)
#ifndef PCB_V9_REV_A
#define BAND_GUIDE_RIGH_LABLE_X		((SW_FRAME_X +                854) - 150)
#else
#define BAND_GUIDE_RIGH_LABLE_X		((SW_FRAME_X +                800) - 150)
#endif
//
#define BAND_GUIDE_LEFT_LABLE_Y		(SCOPE_Y + 70)
#define BAND_GUIDE_MIDP_LABLE_Y		(SCOPE_Y + 20)
#define BAND_GUIDE_RIGH_LABLE_Y		(SCOPE_Y + 70)

int ui_controls_spectrum_is_touch(int x, int y);

void ui_controls_spectrum_init(WM_HWIN hParent);
void ui_controls_spectrum_quit(void);

void ui_controls_update_span(void);

//void ui_controls_spectrum_touch(void);
uchar ui_controls_spectrum_refresh(FAST_REFRESH *cb, uchar mode);

void ui_controls_spectrum_show_notification(char *text);

#endif
