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

#define SMETER_EXPAND_VALUE		3

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
#if 0
// The following are calibrations for the S-meter based on 6 dB per S-unit, 10 dB per 10 dB mark above S-9
// The numbers within are linear gain values, not logarithmic, starting with a zero signal level of 1
// There are 33 entries, one corresponding with each point on the S-meter
#define	S_Meter_Cal_Size	33	// number of entries in table below
const float S_Meter_Cal[] =
{
// - Dummy variable		1,		//0, S0, 0dB
		14.1,	//1.41,	//1, S0.5, 3dB
		20,		//2,		//2, S1, 6dB
		28.1,	//2.81,	//3, S1.5, 9dB
		30,		//3,		//4, S2, 12dB
		56.2,	//5.62,	//5, S2.5, 15dB
		79.4,	//7.94,	//6, S3, 18dB
		112.2,	//11.22,	//7, S3.5, 21dB
		158.5,	//15.85,	//8, S4, 24dB
		223.9,	//22.39,	//9, S4.5, 27dB
		316.3,	//31.63,	//10, S5, 30dB
		446.7,	//44.67,	//11, S5.5, 33dB
		631,	//63.10,	//12, S6, 36dB
		891.3,	//89.13,	//13, S6.5, 39dB
		1258.9,	//125.89,	//14, S7, 42dB
		1778.3,	//177.83,	//15, S7.5, 45dB
		2511.9,	//251.19,	//16, S8, 48dB
		3548.1,	//354.81,	//17, S8.5, 51dB
		5011.9,	//501.19,	//18, S9, 54dB
		8912.5,	//891.25,	//19, +5, 59dB
		15848.9,	//1584.89,	//20, +10, 64dB
		28183.8,	//2818.38,	//21, +15, 69dB
		50118.7,	//5011.87,	//22, +20, 74dB
		89125.1,	//8912.51,	//23, +25, 79dB
		158489.3,	//15848.93,	//24, +30, 84dB
		281838.2,	//28183.82,	//25, +35, 89dB
		501187.2,	//50118.72,	//26, +35, 94dB
		891250.9,	//89125.09,	//27, +40, 99dB
		1585893.2,	//158489.32,	//28, +45, 104dB
		2818382.9,	//281838.29,	//29, +50, 109dB
		5011872.3,	//501187.23,	//30, +55, 114dB
		8912509.4,	//891250.94,	//31, +60, 119dB
		15848931.9,	//1584893.19,	//32, +65, 124dB
		28183829.3,	//2818382.93	//33, +70, 129dB
};
#endif
// ----------------------------------------------------------------

// Exports
void ui_controls_smeter_init(void);
void ui_controls_smeter_quit(void);

void ui_controls_smeter_touch(void);
void ui_controls_smeter_refresh(FAST_REFRESH *cb);

#endif
