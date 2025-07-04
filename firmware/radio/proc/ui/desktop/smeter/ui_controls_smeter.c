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

static void ui_controls_smeter_draw_via_rotate(uchar pos);

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

static void ui_controls_smeter_draw_via_rotate(uchar pos)
{
	//short 		diff;
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

	// Recover Clock control
	ui_controls_clock_panel_restore();
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
//* Function Name       : ui_controls_smeter_analogue_refresh
//* Object              :
//* Input Parameters    :
//* Output Parameters   :
//* Functions called    :
//*----------------------------------------------------------------------------
static void ui_controls_smeter_analogue_refresh(FAST_REFRESH *cb)
{
	ushort 		diff, step, some_val, expanded, curr;
	uchar		is_up;

	curr = ui_sw.sm_value;	// calc by DSP

	//
	// ToDo: Implement TX mode...
	//

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
	#endif
	#if 0
	// Repaint direct
	ui_controls_smeter_draw_via_rotate(expanded);
	#else
	// Repaint in steps
	for(int i = 0; i < diff; i += step)
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

	// Save to public
	sm.old_value = curr;
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
		if(!tsu.rxtx)
			GUI_SetColor(GUI_LIGHTGRAY);
		else
			GUI_SetColor(GUI_DARKGRAY);

		GUI_DispStringAt(	"S  1   3   5   7   9   +20   +40  +60  dB",
							S_METER_X + 12,
							S_METER_Y + S_METER_SY - 10);

		// TX panel
		if(tsu.rxtx)
			GUI_SetColor(GUI_LIGHTGRAY);
		else
			GUI_SetColor(GUI_DARKGRAY);

		GUI_DispStringAt(	"P  1   2   5       10         15   20   W",
							S_METER_X + 12,
							S_METER_Y + S_METER_SY + 30);

		GUI_DispStringAt(	"SWR 1  3   5       10         30         ",
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
			printf("%4dmV(fwd) \r\n", f_volts);

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
			printf("%4dmV(ref) \r\n", r_volts);

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
		GUI_SetColor(GUI_LIGHTGREEN);
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

	// Always repaint on RX/TX change
	if(!tsu.smet_type)
		ui_controls_smeter_panels_refresh();

	// Nothing changed, skip repaint
	if((sm.old_value == ui_sw.sm_value)&&(!tsu.rxtx))
		return;

	if(!tsu.smet_type)
		ui_controls_smeter_digital_refresh();
	else
		ui_controls_smeter_analogue_refresh(cb);
}

#endif
