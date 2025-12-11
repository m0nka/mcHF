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
#ifndef UI_CONTROLS_SPECTRUM_H
#define UI_CONTROLS_SPECTRUM_H

#include "GUI.h"
//#include "arm_math.h"

// Definitions in pixels,not Hz!
#define SPECTRUM_MID_POINT			427		//400
#define SPECTRUM_DEF_HALF_BW		20

// This timeout min value is 11!
#define BAND_GUIDE_FADE_TRIG_VAL	10
#define BAND_GUIDE_TIMEOUT			(BAND_GUIDE_FADE_TRIG_VAL + 30)
#define BAND_GUIDE_FADE_FACTOR		8
#define BAND_GUIDE_START_ALPHA		88
//
#define BAND_GUIDE_LEFT_LABLE_X		((SW_FRAME_X +                  0) +  20)
#define BAND_GUIDE_MIDP_LABLE_X		((SW_FRAME_X + SPECTRUM_MID_POINT) - 250)
#define BAND_GUIDE_RIGH_LABLE_X		((SW_FRAME_X +                854) - 150)
//
#define BAND_GUIDE_LEFT_LABLE_Y		(SCOPE_Y + 70)
#define BAND_GUIDE_MIDP_LABLE_Y		(SCOPE_Y + 20)
#define BAND_GUIDE_RIGH_LABLE_Y		(SCOPE_Y + 70)

int ui_controls_spectrum_is_touch(int x, int y);

void ui_controls_spectrum_init(WM_HWIN hParent);
void ui_controls_spectrum_quit(void);

void ui_controls_update_span(void);

//void ui_controls_spectrum_touch(void);
void ui_controls_spectrum_refresh(FAST_REFRESH *cb);

#endif
