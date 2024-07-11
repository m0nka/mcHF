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
#ifndef UI_CONTROLS_SMETER_H
#define UI_CONTROLS_SMETER_H

#include "GUI.h"

#define SMETER_EXPAND_VALUE		1

// S-meter public
__attribute__((__common__)) struct S_METER {

	int 	pub_value;
	int 	old_value;
	int 	skip;
	int 	init_done;
	uchar	smet_disabled;		// disabled from UI menu(var in eeprom)
	ulong 	repaints;
	uchar 	use_bmp;
	uchar	is_peak;
	uchar	rotary_block;		// s-meter refresh request from rotary driver (while moving dial)
	ushort	rotary_timer;		// how long to block refresh for

} S_METER;

#define countof(Obj) 			(sizeof(Obj) / sizeof(Obj[0]))
#define DEG2RAD      			(3.1415926f / 180)

typedef struct {
  GUI_AUTODEV_INFO AutoDevInfo;  // Information about what has to be displayed
  GUI_POINT        aPoints[5];   // Polygon data
  float            Angle;
  uchar			   pos;
} PARAM;

// Defines smoothness of needle move
// Upwards direction, fast, caused by high current flowing through the coil
// Downwards direction - slow, caused by release and spring reaction
// - the smaller the value, smoother the movement, but CPU load is enormous
#define S_NEEDLE_STEP_SLOW		4
#define S_NEEDLE_STEP_FAST		(S_NEEDLE_STEP_SLOW * 2)

// Defines how often the ISR call will be stalled (refresh is every S_REFRESH_FREQ milliseconds)
#define S_REFRESH_FREQ			2

// Constants for testing
#define S_NEEDLE_LEFT			0
#define S_NEEDLE_CENTRE			1
#define S_NEEDLE_RIGHT			2

#define S_USE_SHADOW

// Needle centre of axis shift from bmp x and y
// this obviously is not the picture bottom
// centre, so those params needs to be matched if
// changing the scale bitmap
// also sticking out needle graphics below the
// scale bmp needs to be cleared after every repaint
//#define S_NEEDLE_X_SHIFT		175
//#define S_NEEDLE_Y_SHIFT		180
#define S_NEEDLE_X_SHIFT		174
#define S_NEEDLE_Y_SHIFT		195
// Define smoothness of rotation
#define MAG          			10
// ----------------------------------------------------------------
// ----------------------------------------------------------------
// Needle is five point polygon, defined counterclockwise
//
//	if..	#define S_NEEDLE_LOW			-145
//			#define S_NEEDLE_TOP			+190
//
//		y
//		|
//	190	-		(0)
//		|		 _
//	188	-	(1) / \ (4)
//		|	    | |
//		|		| |
//		|		| |
//		|		| |
//	 45	-	(2) |_| (3)
//		|
//		------|--|--|-------->x
//			 -2  0  +2
//
//#define S_NEEDLE_LOW			-115
//#define S_NEEDLE_TOP			+175
#define S_NEEDLE_LOW			-145
#define S_NEEDLE_TOP			+190
// Static data, shape of polygon
static const GUI_POINT NeedleBoundary[] = {
		// point 0
		{ MAG * ( 0), MAG * (  0 		  +	S_NEEDLE_TOP) },
		// point 1
		{ MAG * (-2), MAG * ( -2 		  +	S_NEEDLE_TOP) },
		// point 2
		{ MAG * (-2), MAG * (S_NEEDLE_LOW +	S_NEEDLE_TOP) },
		// point 3
		{ MAG * ( 2), MAG * (S_NEEDLE_LOW +	S_NEEDLE_TOP) },
		// point 4
		{ MAG * ( 2), MAG * ( -2 		  +	S_NEEDLE_TOP) },
};
// ----------------------------------------------------------------
// ----------------------------------------------------------------

// Exports
void ui_controls_smeter_init(void);
void ui_controls_smeter_quit(void);

void ui_controls_smeter_touch(void);
void ui_controls_smeter_refresh(FAST_REFRESH *cb);

#endif
