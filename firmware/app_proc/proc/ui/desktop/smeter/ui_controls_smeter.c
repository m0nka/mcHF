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
#include <math.h>

#include "mchf_types.h"
#include "mchf_pro_board.h"
#include "version.h"
#include "mchf_icc_def.h"
#include "adc.h"

#ifdef CONTEXT_VIDEO

//#include "ui_driver.h"
#include "ui_controls_smeter.h"
#include "desktop\ui_controls_layout.h"
#include "desktop\clock_panel\ui_controls_clock_panel.h"

// Object for banding memory device
GUI_AUTODEV 	AutoDev;
//
// S-meter publics
struct S_METER	sm;

// Externally declared s-meter bmp
extern GUI_CONST_STORAGE GUI_BITMAP bmscale;
// --
extern struct 		UI_SW	ui_sw;

// Public radio state
extern struct	TRANSCEIVER_STATE_UI	tsu;

// ICC fast comm
extern TaskHandle_t hIccTask;

#ifdef USE_SIDE_ENC_FOR_S_METER
ulong s_met_pos 	= 180;
ulong s_met_pos_loc = 180;
#endif

#ifdef USE_SPRITE
// ------------------------------------------------------------
extern GUI_CONST_STORAGE GUI_BITMAP bmneedle;
ulong flip = 0;
uchar init_bmp_done = 0;
GUI_HSPRITE	s_needle;
// --
#endif

static void ui_controls_smeter_draw_via_rotate(float pos);

#ifdef USE_SPRITE
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
#if 0
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
#endif

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
	//char	buf[20];

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

static void ui_controls_smeter_draw_via_rotate(float pos)
{
	//short 		diff;
	PARAM       Param;

	// Limiter
	if(pos > S_DEG_MAX)
		pos = S_DEG_MAX;
	if(pos < 0.0f)
		pos = 0.0f;

	// Fractional degrees - the polygon is anti-aliased at MAG sub-pixel
	// resolution, whole degree steps make the needle visibly tick
	Param.Angle = (225.0f - pos) * DEG2RAD;

	//Param.Angle = ui_controls_get_angle(pos)* DEG2RAD;

	Param.pos = (uchar)(pos + 0.5f);

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

	// Repaint, clipped to the meter itself.
	//
	// The needle polygon carries a 45 px tail on the far side of the pivot
	// (which sits at y 196, well below the scale), so the tail sweeps y 151 to
	// 166 - through the 7 px gap under the scale AND through the top of the
	// clock panel, which starts at y 148. That used to be handled by drawing
	// it, wiping the band with the ClearRect in the fixed pass, and then
	// repainting the whole clock panel (rect + time + date, 32 px font) after
	// every single animation step. A fast needle move is a burst of those, and
	// the panel is erased and redrawn straight into the live framebuffer - the
	// LTDC catches it mid-way and the clock appears to blink at random.
	//
	// Clipping means the tail is never drawn below the scale in the first
	// place, which is exactly what the ClearRect was faking, so nothing below
	// needs repairing. The clip bottom is the scale bottom edge, which also
	// carries the 3 px meter frame (S_METER_FRAME_BOTTOM is 0, the frame is
	// drawn inside the rect), so the frame is not cut
	GUI_RECT clip_r;

	clip_r.x0 = 0;
	clip_r.y0 = 0;
	clip_r.x1 = (S_METER_X + bmscale.XSize);
	clip_r.y1 = (S_METER_Y + bmscale.YSize);

	GUI_SetClipRect(&clip_r);

	GUI_MEMDEV_DrawAuto(&AutoDev, &Param.AutoDevInfo, &ui_controls_draw_needle, &Param);

	GUI_SetClipRect(NULL);
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
//* Function Name       : ui_controls_smeter_target_deg
//* Object              : signal level to needle position on the scale bitmap
//* Input Parameters    :
//* Output Parameters   : degrees from the left stop
//* Functions called    :
//*----------------------------------------------------------------------------
static float ui_controls_smeter_target_deg(void)
{
	float dbm, deg;

	if(ui_sw.sm_dbm_valid)
		dbm = (float)ui_sw.sm_dbm - (float)ICC_SMETER_DBM_OFS;
	else if(ui_sw.sm_value <= 9)
		dbm = S_DBM_S9 - 6.0f*(float)(9 - ui_sw.sm_value);		// older M4 image, S-units only
	else
		dbm = S_DBM_S9 + 10.0f*(float)(ui_sw.sm_value - 9);

	if(dbm <= S_DBM_S9)
		deg = S_DEG_S9 + (dbm - S_DBM_S9)*S_DEG_PER_DB_LO;
	else
		deg = S_DEG_S9 + (dbm - S_DBM_S9)*S_DEG_PER_DB_HI;

	if(deg < 0.0f)
		deg = 0.0f;
	if(deg > S_DEG_MAX)
		deg = S_DEG_MAX;

	return deg;
}

//*----------------------------------------------------------------------------
//* Function Name       : ui_controls_smeter_analogue_refresh
//* Object              : advance the needle physics, repaint if it moved
//* Input Parameters    :
//* Output Parameters   :
//* Functions called    :
//*----------------------------------------------------------------------------
static void ui_controls_smeter_analogue_refresh(void)
{
	const float	w = 2.0f*3.1415926f*S_NEEDLE_NAT_FREQ_HZ;
	float		target, acc, h;
	ulong		now, dt, t;

	//
	// ToDo: Implement TX mode...
	//

	// Time based, so the needle moves at the same speed no matter how often
	// the UI loop gets round to us (it only does on ticks without an FFT
	// frame, and those come in irregular bursts)
	now = xTaskGetTickCount()*portTICK_PERIOD_MS;
	dt  = now - sm.last_ms;
	sm.last_ms = now;

	if(dt == 0)
		return;
	if(dt > S_NEEDLE_MAX_DT_MS)
		dt = S_NEEDLE_MAX_DT_MS;

	target = ui_controls_smeter_target_deg();

	// Drive - instant attack, exponential release
	if(target >= sm.drive_deg)
		sm.drive_deg = target;
	else
		sm.drive_deg = target + (sm.drive_deg - target)*expf(-(float)dt/S_NEEDLE_RELEASE_MS);

	// Movement - damped spring towards the drive
	for(t = 0; t < dt; t += S_NEEDLE_SUBSTEP_MS)
	{
		h = (float)(((dt - t) < S_NEEDLE_SUBSTEP_MS) ? (dt - t) : S_NEEDLE_SUBSTEP_MS)/1000.0f;

		acc = w*w*(sm.drive_deg - sm.needle_deg) - 2.0f*S_NEEDLE_DAMPING*w*sm.needle_vel;

		sm.needle_vel += acc*h;
		sm.needle_deg += sm.needle_vel*h;
	}

	// Mechanical stops
	if(sm.needle_deg < 0.0f)
	{
		sm.needle_deg = 0.0f;
		if(sm.needle_vel < 0.0f)
			sm.needle_vel = 0.0f;
	}
	if(sm.needle_deg > S_DEG_MAX)
	{
		sm.needle_deg = S_DEG_MAX;
		if(sm.needle_vel > 0.0f)
			sm.needle_vel = 0.0f;
	}

	// Repaint only on visible movement, and at no more than the panel rate
	if(fabsf(sm.needle_deg - sm.drawn_deg) < S_NEEDLE_MIN_MOVE)
		return;
	if((now - sm.drawn_ms) < S_NEEDLE_FRAME_MS)
		return;

	ui_controls_smeter_draw_via_rotate(sm.needle_deg);

	sm.drawn_deg = sm.needle_deg;
	sm.drawn_ms  = now;
}

//*----------------------------------------------------------------------------
//* Function Name       : ui_controls_smeter_analogue_init
//* Object              :
//* Input Parameters    :
//* Output Parameters   :
//* Functions called    :
//*----------------------------------------------------------------------------
static void ui_controls_smeter_analogue_init(void)
{
	//PARAM       Param;      // Parameters for drawing routine
	//int         Cnt;
	//int         tDiff = 0;
	//int         t0;

	// Init public data
	sm.pub_value 		= 0;
	sm.old_value 		= 0;
	sm.skip 			= 0;
	sm.init_done		= 0;
	//sm.smet_disabled 	= 0;
	sm.repaints 		= 0;
	sm.use_bmp 			= 1;
	sm.is_peak			= 0;
	sm.rotary_block		= 0;
	sm.rotary_timer		= 0;
	sm.drive_deg		= 0.0f;
	sm.needle_deg		= 0.0f;
	sm.needle_vel		= 0.0f;
	sm.drawn_deg		= 0.0f;
	sm.last_ms			= xTaskGetTickCount()*portTICK_PERIOD_MS;
	sm.drawn_ms			= sm.last_ms;

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

	// Debug
	sm.repaints = 0;
}

//*----------------------------------------------------------------------------
//* Function Name       : ui_controls_smeter_panels_refresh
//* Object              :
//* Input Parameters    :
//* Output Parameters   :
//* Functions called    :
//*----------------------------------------------------------------------------
static void ui_controls_smeter_panels_refresh(void)
{
	// Enable/Disable panels
	if(sm.loc_tx_state != tsu.rxtx)
	{
		GUI_SetFont(&GUI_Font8x16_1);

		// RX panel
		GUI_SetColor(GUI_DARKCYAN);
		GUI_DispStringAt(	"S  1   3   5   7   9   +20   +40  +60  dB",
							S_METER_X + 12,
							S_METER_Y + S_METER_SY - 10);

		GUI_SetColor(GUI_LIGHTGRAY);
		GUI_DispStringAt(	"S  1   3   5   7   9",
							S_METER_X + 12,
							S_METER_Y + S_METER_SY - 10);

		// TX panel
		GUI_SetColor(HOT_PINK);
		GUI_DispStringAt(	"P  1   2   5       10         15   20   W",
							S_METER_X + 12,
							S_METER_Y + S_METER_SY + 30);
		GUI_SetColor(GUI_LIGHTGRAY);
		GUI_DispStringAt(	"P  1   2   5",
							S_METER_X + 12,
							S_METER_Y + S_METER_SY + 30);

		GUI_SetColor(HOT_PINK);
		GUI_DispStringAt(	"SWR 1  3   5       10         30         ",
							S_METER_X + 12,
							S_METER_Y + S_METER_SY + 85);

		GUI_SetColor(GUI_LIGHTGRAY);
		GUI_DispStringAt(	"SWR 1  3",
							S_METER_X + 12,
							S_METER_Y + S_METER_SY + 85);

		// Mask old S-meter progress
		GUI_SetColor(GUI_DARKGRAY);
		GUI_FillRoundedRect((S_METER_X + 10),
							(S_METER_Y + S_METER_SY + 10),
							(S_METER_X + 10 + S_METER_MAX),
							(S_METER_Y + S_METER_SY + 17),
							2);

		// Mask old Power progress
		GUI_SetColor(GUI_DARKGRAY);
		GUI_FillRoundedRect((S_METER_X + 10),
							(S_METER_Y + S_METER_SY + 50),
							(S_METER_X + 10 + S_METER_MAX),
							(S_METER_Y + S_METER_SY + 57),
							2);

		// Mask old SWR progress
		GUI_SetColor(GUI_DARKGRAY);
		GUI_FillRoundedRect((S_METER_X + 10),
							(S_METER_Y + S_METER_SY + 70),
							(S_METER_X + 10 + S_METER_MAX),
							(S_METER_Y + S_METER_SY + 77),
							2);


		sm.loc_tx_state = tsu.rxtx;
	}
}

//*----------------------------------------------------------------------------
//* Function Name       : ui_controls_smeter_digital_refresh
//* Object              :
//* Input Parameters    :
//* Output Parameters   :
//* Functions called    :
//*----------------------------------------------------------------------------
static void ui_controls_smeter_digital_refresh(void)
{
	// Repaint based on mode
	if(!tsu.rxtx)
	{
		ushort s_sta 	= 0;
		ushort curr 	= ui_sw.sm_value;
		ushort s_val;

		s_val = (curr * 10);		// progress bar
		//s_sta = (curr * 10) - 20;	// moving dot

		// Limiter S-meter
		if(s_val > S_METER_MAX)
			s_val = S_METER_MAX;

		// Mask old S-value progress
		GUI_SetColor(GUI_DARKGRAY);
		GUI_FillRoundedRect((S_METER_X + 10),
							(S_METER_Y + S_METER_SY + 10),
							(S_METER_X + 10 + S_METER_MAX),
							(S_METER_Y + S_METER_SY + 17),
							2);

		// New S-meter value
		GUI_SetColor(GUI_LIGHTGREEN);
		GUI_FillRoundedRect((S_METER_X + 10 + s_sta),
							(S_METER_Y + S_METER_SY + 10),
							(S_METER_X + (10 + s_val)),
							(S_METER_Y + S_METER_SY + 17),
							2);

		// Save to public
		sm.old_value = curr;
	}
	else
	{
		ushort t_val_p = 0;
		ushort t_val_r = 0;
		ushort f_volts = 0;
		ushort r_volts = 0;

		// Forward voltage on the bridge
		f_volts = adc_read_fwd_power();
		if(f_volts == 0xFFFF)
		{}//sprintf(buf, "FWD %d.%dV", 0, 0);
		else
		{
			//sprintf(buf, "FWD %d.%dV", f_volts/1000, (f_volts%1000)/10);
			//printf("%4dmV(fwd) \r\n", f_volts);

			// Temp!
			t_val_p = f_volts/10;
		}

		// Forward voltage on the bridge
		r_volts = adc_read_ref_power();
		if(r_volts == 0xFFFF)
		{}//sprintf(buf, "FWD %d.%dV", 0, 0);
		else
		{
			//sprintf(buf, "FWD %d.%dV", f_volts/1000, (f_volts%1000)/10);
			//printf("%4dmV(ref) \r\n", r_volts);

			// Temp!
			t_val_r = r_volts/10;
		}

		// Limiter Power
		if(t_val_p > S_METER_MAX)
			t_val_p = S_METER_MAX;

		// Limiter SWR
		if(t_val_r > S_METER_MAX)
			t_val_r = S_METER_MAX;

		// Mask old Power progress
		GUI_SetColor(GUI_DARKGRAY);
		GUI_FillRoundedRect((S_METER_X + 10),
							(S_METER_Y + S_METER_SY + 50),
							(S_METER_X + 10 + S_METER_MAX),
							(S_METER_Y + S_METER_SY + 57),
							2);

		// Repaint new Power progress
		GUI_SetColor(GUI_LIGHTGREEN);
		GUI_FillRoundedRect((S_METER_X + 10 + 0),
							(S_METER_Y + S_METER_SY + 50),
							(S_METER_X + (10 + t_val_r)),	// swapped! ToDo: need proper impl
							(S_METER_Y + S_METER_SY + 57),
							2);

		// Mask old SWR progress
		GUI_SetColor(GUI_DARKGRAY);
		GUI_FillRoundedRect((S_METER_X + 10),
							(S_METER_Y + S_METER_SY + 70),
							(S_METER_X + 10 + S_METER_MAX),
							(S_METER_Y + S_METER_SY + 77),
							2);

		// Repaint new SWR progress
		GUI_SetColor(GUI_LIGHTRED);
		GUI_FillRoundedRect((S_METER_X + 10 + 0),
							(S_METER_Y + S_METER_SY + 70),
							(S_METER_X + (10 + t_val_p)),	// swapped! ToDo: need proper impl
							(S_METER_Y + S_METER_SY + 77),
							2);

	}
}

//*----------------------------------------------------------------------------
//* Function Name       : ui_controls_smeter_digital_init
//* Object              :
//* Input Parameters    :
//* Output Parameters   :
//* Functions called    :
//*----------------------------------------------------------------------------
static void ui_controls_smeter_digital_init(void)
{
	// Reset
	sm.loc_tx_state = UNDEF_TX_STATE;

	// Frame
	GUI_SetColor(GUI_DARKCYAN);
	GUI_DrawRoundedFrame(	(S_METER_X - S_METER_FRAME_LEFT),
							(S_METER_Y - S_METER_FRAME_TOP),
							(bmscale.XSize + S_METER_X + S_METER_FRAME_RIGHT),
							(bmscale.YSize + S_METER_Y + S_METER_FRAME_BOTTOM),
							S_METER_FRAME_CURVE,
							S_METER_FRAME_WIDTH);

	// Top/Mid/Bottom background
	GUI_SetColor(GUI_DARKGRAY);
	GUI_FillRoundedRect((S_METER_X + 10),
						(S_METER_Y + S_METER_SY + 10),
						(S_METER_X + 10 + S_METER_MAX),
						(S_METER_Y + S_METER_SY + 17),
						2);

	GUI_FillRoundedRect((S_METER_X + 10),
						(S_METER_Y + S_METER_SY + 50),
						(S_METER_X + 10 + S_METER_MAX),
						(S_METER_Y + S_METER_SY + 57),
						2);

	GUI_FillRoundedRect((S_METER_X + 10),
						(S_METER_Y + S_METER_SY + 70),
						(S_METER_X + 10 + S_METER_MAX),
						(S_METER_Y + S_METER_SY + 77), 2);

#if 0
	// S-meter bar frame
	GUI_SetColor(GUI_WHITE);
	GUI_FillRoundedRect((S_METER_X + 8),
						(S_METER_Y + S_METER_SY + 8),
						(S_METER_X + 12 + S_METER_MAX),
						(S_METER_Y + S_METER_SY + 19),
						2);

	// Power meter frame
	GUI_FillRoundedRect((S_METER_X + 8),
						(S_METER_Y + S_METER_SY + 48),
						(S_METER_X + 12 + S_METER_MAX),
						(S_METER_Y + S_METER_SY + 59),
						2);

	// SWR meter progress
	GUI_FillRoundedRect((S_METER_X + 8),
						(S_METER_Y + S_METER_SY + 68),
						(S_METER_X + 12 + S_METER_MAX),
						(S_METER_Y + S_METER_SY + 79),
						2);
#endif

	// Initial paint of actual values
	ui_controls_smeter_panels_refresh();
	ui_controls_smeter_digital_refresh();
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
	if(tsu.smet_type)
		ui_controls_smeter_analogue_init();
	else
		ui_controls_smeter_digital_init();

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
	//#ifndef USE_SPRITE
	//GUI_MEMDEV_DeleteAuto(&AutoDev);
	//GUI_ClearRect(0, 70, 319, 239);
	//#endif

	// Clear public data
	sm.pub_value 		= 0;
	sm.old_value 		= 0;
	sm.skip 			= 0;
	sm.init_done		= 0;
	//sm.smet_disabled 	= 0;
	sm.repaints 		= 0;
	sm.use_bmp 			= 1;
	sm.is_peak			= 0;
	sm.rotary_block		= 0;
	sm.rotary_timer		= 0;
	sm.drive_deg		= 0.0f;
	sm.needle_deg		= 0.0f;
	sm.needle_vel		= 0.0f;
	sm.drawn_deg		= 0.0f;
	//
	sm.loc_tx_state 	= UNDEF_TX_STATE;
	//
	sm.init_done = 0;
}

//*----------------------------------------------------------------------------
//* Function Name       : ui_controls_smeter_touch
//* Object              :
//* Input Parameters    :
//* Output Parameters   :
//* Functions called    :
//*----------------------------------------------------------------------------
void ui_controls_smeter_touch(void)
{
	// ...
}

//*----------------------------------------------------------------------------
//* Function Name       : ui_controls_smeter_refresh
//* Object              :
//* Input Parameters    :
//* Output Parameters   :
//* Functions called    :
//*----------------------------------------------------------------------------
void ui_controls_smeter_refresh(FAST_REFRESH *cb)
{
	// Control ready ?
	if((!(sm.init_done))||(sm.smet_disabled)/*||(sm.rotary_block)*/)
	{
		//--ui_controls_smeter_block_on();
		return;
	}

	// Analogue needle keeps moving after the value settles, so it runs
	// every call and decides for itself whether to repaint
	if(tsu.smet_type)
	{
		ui_controls_smeter_analogue_refresh();
		return;
	}

	// Always repaint on RX/TX change
	ui_controls_smeter_panels_refresh();

	// Nothing changed, skip repaint
	if((sm.old_value == ui_sw.sm_value)&&(!tsu.rxtx))
		return;

	ui_controls_smeter_digital_refresh();
}

#endif
