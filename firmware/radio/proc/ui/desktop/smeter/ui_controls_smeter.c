/************************************************************************************
**                                                                                 **
**                             mcHF Pro QRP Transceiver                            **
**                         Krassi Atanassov - M0NKA, 2013-2024                     **
**                                                                                 **
**---------------------------------------------------------------------------------**
**                                                                                 **
**  File name:                                                                     **
**  Description:                                                                   **
**  Last Modified:                                                                 **
**  Licence:       The mcHF project is released for radio amateurs experimentation **
**               and non-commercial use only.Check 3rd party drivers for licensing **
************************************************************************************/
#include <math.h>

#include "mchf_types.h"
#include "mchf_pro_board.h"
#include "version.h"
#include "mchf_icc_def.h"

#ifdef CONTEXT_VIDEO

//#include "ui_driver.h"
#include "ui_controls_smeter.h"
#include "desktop\ui_controls_layout.h"
#include "desktop\clock\ui_controls_clock.h"

//#define USE_SPRITE

// Externally declared s-meter bmp
extern GUI_CONST_STORAGE GUI_BITMAP bmscale;
// --
extern struct 		UI_SW	ui_sw;
// Public radio state
extern struct	TRANSCEIVER_STATE_UI	tsu;

// ICC fast comm
extern TaskHandle_t hIccTask;

#if 0
// -------------------------
// S-meter publics
int 	pub_value 		= 0;
int 	old_value 		= 0;
int 	skip 			= 0;
int 	init_done		= 0;
uchar	smet_disabled 	= 0;		// disabled from UI menu(var in eeprom)
ulong 	repaints 		= 0;
uchar 	use_bmp 		= 1;
uchar	is_peak			= 0;
uchar	rotary_block	= 0;		// s-meter refresh request from rotary driver (while moving dial)
ushort	rotary_timer	= 0;		// how long to block refresh for
// -------------------------
#endif

// Object for banding memory device
GUI_AUTODEV 	AutoDev;
//
// S-meter publics
struct S_METER	sm;

#ifdef USE_SIDE_ENC_FOR_S_METER
ulong s_met_pos 	= 180;
ulong s_met_pos_loc = 180;
#endif

static void ui_controls_smeter_draw_via_rotate(uchar pos);

//*----------------------------------------------------------------------------
//* Function Name       : ui_controls_get_angle
//* Object              : returns the value value to indicate. In a real
//* Object              : application, this value would somehow be measured.
//* Input Parameters    :
//* Output Parameters   :
//* Functions called    :
//*----------------------------------------------------------------------------
/*static float ui_controls_get_angle(int tDiff)
{
  if (tDiff < 15000) {
    return  225 - 0.006 * tDiff ;
  }
  tDiff -= 15000;
  if (tDiff < 7500) {
    return  225 - 90 + 0.012 * tDiff ;
  }
  return 225;
}*/

//*----------------------------------------------------------------------------
//* Function Name       : ui_controls_smeter_block_on
//* Object              : Handle rotary block timer here
//* Input Parameters    :
//* Output Parameters   :
//* Functions called    :
//*----------------------------------------------------------------------------
static void ui_controls_smeter_block_on(void)
{
	int test_value = (sm.old_value*SMETER_EXPAND_VALUE)/2;

	// Nothing to process
	if(!sm.rotary_block)
		return;

	//--printf("-- s-meter block on --\r\n");

	// Reset needle (graciously) when block is on
	if(test_value > 10)
	{
		test_value -= 10;
		ui_controls_smeter_draw_via_rotate(test_value - sm.rotary_timer);

		sm.old_value = test_value;
	}

	// Update timer
	(sm.rotary_timer)++;
	if(sm.rotary_timer == 5)
	{
		// Reset block flag and timer
		sm.rotary_timer 	= 0;
		sm.rotary_block	= 0;
	}
}

#ifndef USE_SPRITE
//*----------------------------------------------------------------------------
//* Function Name       : ui_controls_draw_needle
//* Object              :
//* Input Parameters    :
//* Output Parameters   :
//* Functions called    :
//*----------------------------------------------------------------------------
static void ui_controls_draw_needle(void * p)
{
	PARAM * pParam;
	char	buf[20];

	pParam = (PARAM *)p;

	// Fixed background
	if (pParam->AutoDevInfo.DrawFixed)
	{
		// Just white background
		GUI_SetColor(GUI_WHITE);
		GUI_FillRect(	(S_METER_X + 0),
						(S_METER_Y + 0),
						(S_METER_X + bmscale.XSize -  3),
						(S_METER_Y + bmscale.YSize + 25));

		if(sm.use_bmp) GUI_DrawBitmap(&bmscale, S_METER_X, S_METER_Y);
	}

	#ifdef S_USE_SHADOW
	if(pParam->pos < 45)
	{
		// Shift shadow left
		pParam->aPoints[0].x -= 2*MAG;
		pParam->aPoints[1].x -= 2*MAG;
		pParam->aPoints[2].x -= 2*MAG;
		pParam->aPoints[3].x -= 2*MAG;
		pParam->aPoints[4].x -= 2*MAG;
	}
	else
	{
		// Shift shadow right
		pParam->aPoints[0].x += 2*MAG;
		pParam->aPoints[1].x += 2*MAG;
		pParam->aPoints[2].x += 2*MAG;
		pParam->aPoints[3].x += 2*MAG;
		pParam->aPoints[4].x += 2*MAG;
	}

	// Moving needle shadow
	GUI_SetColor(GUI_GRAY);
	GUI_AA_FillPolygon(	pParam->aPoints, countof(NeedleBoundary),
						(MAG * (S_METER_X + S_NEEDLE_X_SHIFT)),
						(MAG * (S_METER_Y + S_NEEDLE_Y_SHIFT)));

	// Restore correct x position
	if(pParam->pos < 45)
	{
		pParam->aPoints[0].x += 2*MAG;
		pParam->aPoints[1].x += 2*MAG;
		pParam->aPoints[2].x += 2*MAG;
		pParam->aPoints[3].x += 2*MAG;
		pParam->aPoints[4].x += 2*MAG;
	}
	else
	{
		pParam->aPoints[0].x -= 2*MAG;
		pParam->aPoints[1].x -= 2*MAG;
		pParam->aPoints[2].x -= 2*MAG;
		pParam->aPoints[3].x -= 2*MAG;
		pParam->aPoints[4].x -= 2*MAG;
	}
	#endif

	// Moving needle
	GUI_SetColor(GUI_RED);
	GUI_AA_FillPolygon(	pParam->aPoints, countof(NeedleBoundary),
						(MAG * (S_METER_X + S_NEEDLE_X_SHIFT)),
						(MAG * (S_METER_Y + S_NEEDLE_Y_SHIFT)));

	// Fixed foreground
	if (pParam->AutoDevInfo.DrawFixed)
	{
		#if 0
		// Paint needle area below scale
		GUI_SetAlpha(168);
		GUI_SetColor(GUI_GRAY);
		GUI_FillRect(	S_METER_X,
						(S_METER_Y + bmscale.YSize +  0),
						(S_METER_X + bmscale.XSize -  2),
						(S_METER_Y + bmscale.YSize + 20));
		GUI_SetAlpha(0);
		#endif
		#ifndef USE_SIDE_ENC_FOR_S_METER
		// Clear needle area below scale
		GUI_ClearRect(	S_METER_X  - 2,
						(S_METER_Y + bmscale.YSize +  0),
						(S_METER_X + bmscale.XSize -  0),
						(S_METER_Y + bmscale.YSize + 25));
		// Draw S meter border - this creates the 3D effect that the scale is buried behind the screen
		GUI_SetColor(GUI_WHITE);
		GUI_DrawRoundedFrame(	(S_METER_X - S_METER_FRAME_LEFT),					(S_METER_Y - S_METER_FRAME_TOP),
	   							(bmscale.XSize + S_METER_X + S_METER_FRAME_RIGHT), 	(bmscale.YSize + S_METER_Y + S_METER_FRAME_BOTTOM),
								S_METER_FRAME_CURVE, 								S_METER_FRAME_WIDTH);
		#endif

		// Recover Clock control
		//--ui_controls_clock_restore();

		#if 0
		// Debug only
		GUI_SetColor(GUI_BLUE);
		GUI_SetFont(&GUI_Font8x16_1);
		sprintf(buf,"P=%d",pParam->pos);
		GUI_DispStringAt(buf,S_METER_X + 312,S_METER_Y + 5);
		#endif

		#if 0
		// --------------------------------------------------------------------------------------------------
		//GUI_SetAlpha(168);
		// PEAK/AVER indicator
		GUI_SetColor(GUI_DARKBLUE);
		GUI_SetFont(&GUI_Font8x16_1);
		if(sm.is_peak)
			GUI_DispStringAt("PEAK",S_METER_X + 312,S_METER_Y + 5);
		else
			GUI_DispStringAt("AVER",S_METER_X + 312,S_METER_Y + 5);
		// Debug print CPU firmware version
		//GUI_SetColor(GUI_BLUE);
		//GUI_SetFont(&GUI_Font8x16_1);
		sprintf(buf,"UI:%d.%d.%d.%d",MCHF_R_VER_MAJOR, MCHF_R_VER_MINOR, MCHF_R_VER_RELEASE, MCHF_R_VER_BUILD);
		GUI_DispStringAt(buf,S_METER_X + 5,	S_METER_Y + bmscale.YSize - 16);
		// Debug print DSP firmware version
		if(tsu.dsp_alive)
		{
			#if 0
			if((tsu.dsp_rev1 == 0) && (tsu.dsp_rev2 == 0) && (tsu.dsp_rev3 == 0) && (tsu.dsp_rev4 == 0))
			{
				printf("trying to get DSP revision...\r\n");

				// Post msg to ICC task
				xTaskNotify(hIccTask, ICC_GET_FW_VERSION, eSetValueWithOverwrite);
			}
			#endif
			//GUI_SetColor(GUI_BLUE);
			//GUI_SetFont(&GUI_Font8x16_1);
			sprintf(buf,"DSP:%d.%d.%d.%d",tsu.dsp_rev1,tsu.dsp_rev2,tsu.dsp_rev3,tsu.dsp_rev4);
			GUI_DispStringAt(buf,S_METER_X + 240,S_METER_Y + bmscale.YSize - 16);
		}
		// Repaint count
		//GUI_SetColor(GUI_BLUE);
		//GUI_SetFont(&GUI_Font8x16_1);
		sprintf(buf,"R=%d",sm.repaints);
		GUI_DispStringAt(buf,S_METER_X + 5,S_METER_Y + 5);
		// -------------
		//GUI_SetAlpha(0);
		// --------------------------------------------------------------------------------------------------
		#endif
	}
}

static void ui_controls_smeter_draw_via_rotate(uchar pos)
{
	short 		diff;
	PARAM       Param;

	// Limiter
	if(pos > 90)
		pos = 90;

	Param.Angle = (225 - pos) * DEG2RAD;

	//Param.Angle = ui_controls_get_angle(pos)* DEG2RAD;

	Param.pos = pos;

	// Rotate
	GUI_RotatePolygon(Param.aPoints, NeedleBoundary, countof(NeedleBoundary), Param.Angle);

#if 0
	printf("----------------------------------------------------------------------------------------\r\n");
	printf(	"{%d,%d},{%d,%d},{%d,%d},{%d,%d},{%d,%d}\r\n",
			Param.aPoints[0].x/MAG,Param.aPoints[0].y/MAG,
			Param.aPoints[1].x/MAG,Param.aPoints[1].y/MAG,
			Param.aPoints[2].x/MAG,Param.aPoints[2].y/MAG,
			Param.aPoints[3].x/MAG,Param.aPoints[3].y/MAG,
			Param.aPoints[4].x/MAG,Param.aPoints[4].y/MAG
		  );
#endif

	// Repaint
	GUI_MEMDEV_DrawAuto(&AutoDev, &Param.AutoDevInfo, &ui_controls_draw_needle, &Param);
}
#endif

#ifdef USE_SPRITE
// ------------------------------------------------------------
extern GUI_CONST_STORAGE GUI_BITMAP bmneedle;
ulong flip = 0;
uchar init_bmp_done = 0;
GUI_HSPRITE	s_needle;
// --
static void ui_controls_smeter_draw_via_sprite(PARAM Param)
{
	if(!init_bmp_done)
	{
		GUI_DrawBitmap(&bmscale, S_METER_X, S_METER_Y);

		// Draw S meter border - this creates the 3D effect that the scale is buried behind the screen
		GUI_SetColor(GUI_WHITE);
		GUI_DrawRoundedFrame(	(S_METER_X - S_METER_FRAME_LEFT),					(S_METER_Y - S_METER_FRAME_TOP),
								(bmscale.XSize + S_METER_X + S_METER_FRAME_RIGHT), 	(bmscale.YSize + S_METER_Y + S_METER_FRAME_BOTTOM),
								S_METER_FRAME_CURVE, 								S_METER_FRAME_WIDTH);

		s_needle = GUI_SPRITE_Create(&bmneedle, S_METER_X, S_METER_Y);

		init_bmp_done = 1;
	}

	// Sprite test (instead of rotating polygon
	if(flip < 1000)
	{
		flip++;
		return;
	}

	//flip = 0;

	GUI_SPRITE_Hide(s_needle);
}
#endif

//*----------------------------------------------------------------------------
//* Function Name       : ui_controls_smeter_set_needle
//* Object              : Move needle to limiting position (debug/test)
//* Input Parameters    :
//* Output Parameters   :
//* Functions called    :
//*----------------------------------------------------------------------------
static void ui_controls_smeter_set_needle(uchar pos)
{
	//PARAM       Param;

/*	switch(pos)
	{
		case S_NEEDLE_LEFT:
			Param.Angle = DEG2RAD * 225;	// left limiter engaged
			break;
		case S_NEEDLE_CENTRE:
			Param.Angle = DEG2RAD * 180;	// vertical (centre)
			break;
		case S_NEEDLE_RIGHT:
			Param.Angle = DEG2RAD * 135;	// right limiter engaged
			break;
		default:
			Param.Angle = DEG2RAD * pos;	// any position
			break;
	}*/

	// Move needle
	#ifndef USE_SPRITE
	ui_controls_smeter_draw_via_rotate(pos);
	#endif
	#ifdef USE_SPRITE
	ui_controls_smeter_draw_via_sprite(Param);
	#endif
}

//*----------------------------------------------------------------------------
//* Function Name       : ui_controls_smeter_init
//* Object              : Init
//* Input Parameters    :
//* Output Parameters   :
//* Functions called    :
//*----------------------------------------------------------------------------
void ui_controls_smeter_init(void)
{
	PARAM       Param;      // Parameters for drawing routine
	int         Cnt;
	int         tDiff = 0;
	int         t0;

	// Init public data
	sm.pub_value 		= 0;
	sm.old_value 		= 0;
	sm.skip 			= 0;
	sm.init_done		= 0;
	sm.smet_disabled 	= 0;
	sm.repaints 		= 0;
	sm.use_bmp 			= 1;
	sm.is_peak			= 0;
	sm.rotary_block		= 0;
	sm.rotary_timer		= 0;

	// Enable high resolution for antialiasing
	GUI_AA_EnableHiRes();
	GUI_AA_SetFactor(MAG);

	#ifndef USE_SPRITE
	// Create GUI_AUTODEV-object
	GUI_MEMDEV_CreateAuto(&AutoDev);
	#endif

	// Needle at 0
	#ifndef USE_SIDE_ENC_FOR_S_METER
	ui_controls_smeter_set_needle(S_NEEDLE_LEFT);
	#endif
	#ifdef USE_SIDE_ENC_FOR_S_METER
	ui_controls_smeter_set_needle(S_NEEDLE_CENTRE);
	#endif

	// From Eeprom
	sm.smet_disabled = (*(uchar *)(EEP_BASE + EEP_AN_MET_ON));

	// Debug
	sm.repaints = 0;

	// Ready to refresh
	sm.init_done = 1;
}

//*----------------------------------------------------------------------------
//* Function Name       : ui_controls_smeter_quit
//* Object              :
//* Input Parameters    :
//* Output Parameters   :
//* Functions called    :
//*----------------------------------------------------------------------------
void ui_controls_smeter_quit(void)
{
	#ifndef USE_SPRITE
	GUI_MEMDEV_DeleteAuto(&AutoDev);
	GUI_ClearRect(0, 70, 319, 239);
	#endif

	sm.init_done = 0;
}

#if 0
// test
#include "ft5x06.h"
extern struct TD 				t_d;
#endif

void ui_controls_smeter_touch(void)
{
#if 0
	ushort point;

	point  = t_d.point_x[0];
	point  = point/4;
	point  = 225 - point;

	ui_controls_smeter_set_needle(point);
#endif

	//smet_disabled = !smet_disabled;
	//use_bmp = !use_bmp;
	sm.is_peak = !(sm.is_peak);
}

//*----------------------------------------------------------------------------
//* Function Name       : ui_controls_smeter_refresh
//* Object              : - at 200MHz, 84% CPU usage, speed is good!
//* Input Parameters    :
//* Output Parameters   :
//* Functions called    :
//*----------------------------------------------------------------------------
void ui_controls_smeter_refresh(FAST_REFRESH *cb)
{
	ushort 		i,curr,diff,step,some_val,expanded,bandw,centre_freq,aver,peak;
	uchar		is_up;

	// Control ready ?
	if((!(sm.init_done)) || (sm.smet_disabled) || (sm.rotary_block))
	{
		ui_controls_smeter_block_on();
		return;
	}

	#ifdef USE_SIDE_ENC_FOR_S_METER
	if(s_met_pos != s_met_pos_loc)
	{
		ui_controls_smeter_set_needle(s_met_pos);
		s_met_pos_loc = s_met_pos;
	}
	return;
	#endif

	// Debug
	sm.repaints = 0;

	#if 0
	// Already calculated by spectrum repaint routines
	bandw 		= (ui_sw.bandpass_end - ui_sw.bandpass_start);
	centre_freq = (ui_sw.bandpass_end - ui_sw.bandpass_start)/2 + ui_sw.bandpass_start;

	if(sm.is_peak)
	{
		// Peak point from a bandwidth
		// midpoint is (start_element + count/2)
		for(i = 0, peak = 0; i < bandw; i++)
		{
			// is peak ?
			if(ui_sw.fft_value[(centre_freq - (bandw/2)) + i] > aver)
				peak = ui_sw.fft_value[(centre_freq - (bandw/2)) + i];
		}
		curr = peak;
	}
	else
	{
		// Average for a bandwidth
		// midpoint is (start_element + count/2)
		for(i = 0, aver = 0; i < bandw; i++)
		{
			// Accumulate
			aver += ui_sw.fft_value[(centre_freq - (bandw/2)) + i];
		}
		curr  = aver/bandw;
	}
	#else
	curr = ui_sw.sm_value;	// calc by DSP
	#endif

	#if 1
	// Nothing change, skip repaint
	if(sm.old_value == curr)
		return;
	#endif

	#if 0
	// old and nearby values
	if(
		(old_value == curr - 4)	||
		(old_value == curr - 3)	||
		(old_value == curr - 2)	||
		(old_value == curr - 1)	||
		(old_value == curr) 	||
		(old_value == curr + 1)	||
		(old_value == curr + 2)	||
		(old_value == curr + 3) ||
		(old_value == curr + 4)
		)
		return;
	#endif

	// Remove noise floor
	//if(curr < 25) curr = 0;

	// Expand scale
	expanded = curr*SMETER_EXPAND_VALUE;

	// Decide needle direction, based on old value
	if(sm.old_value > curr)
	{
		is_up = 0;				// needle going down
		diff = ((sm.old_value)*SMETER_EXPAND_VALUE) - expanded;
	}
	else
	{
		is_up = 1;				// needle going up
		diff  = expanded - ((sm.old_value)*SMETER_EXPAND_VALUE);
	}

	// IRL analogue meter emulation by variable step change
	if(is_up)
		step = S_NEEDLE_STEP_FAST;
	else
		step = S_NEEDLE_STEP_SLOW;

	// Debug only
	sm.repaints = diff/step;

	#if 0
	printf("--------------------------------------\r\n");
	printf("curr = %d\r\n",curr);
	printf("expanded = %d\r\n",expanded);
	printf("step = %d\r\n",step);
	printf("dif = %d\r\n",diff);
	printf("repaints = %d\r\n",repaints);
	printf("now loop...\r\n");
	#endif

	#if 1
	// Repaint direct
	ui_controls_smeter_draw_via_rotate(expanded);
	#else
	// Repaint in steps
	for(i = 0; i < diff; i += step)
	{
		if(is_up)
			some_val = ((sm.old_value)*SMETER_EXPAND_VALUE) + i;
		else
			some_val = ((sm.old_value)*SMETER_EXPAND_VALUE) - i;

		// Collapse scale
		some_val = some_val/2;

		//printf("some_val = %d\r\n",some_val);

		ui_controls_smeter_draw_via_rotate(some_val);

		// Fast UI update callback
		if(cb) cb();
	}
	#endif

	// Save to public
	sm.old_value = curr;
}

#endif
