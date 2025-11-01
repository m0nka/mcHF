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
#include "mchf_pro_board.h"

#ifdef CONTEXT_VIDEO

#include "gui.h"
#include "dialog.h"

#include <math.h>
#include <stddef.h>

#include "ui_proc.h"
#include "ui_controls_spectrum.h"
#include "desktop\ui_controls_layout.h"

// -------------------------
//
// Causing hard fault :(
#define USE_MEM_DEVICE
//
//#define SPEC_USE_WM
//
// -------------------------

#ifdef SPEC_USE_WM
static const GUI_WIDGET_CREATE_INFO SpectrumDialog[] =
{
	// --------------------------------------------------------------------------------------------------------------------------------------------------------
	//							name		id					x		y		xsize				ysize				?		?		?
	// --------------------------------------------------------------------------------------------------------------------------------------------------------
	// Self
	{ WINDOW_CreateIndirect,	"", 		ID_WINDOW_SPEC,		0,		0,		SW_CONTROL_X_SIZE,	SW_CONTROL_Y_SIZE, 	WM_CF_MEMDEV, 0, 0 },
	//
	//{ TEXT_CreateIndirect, 		"----",		ID_TEXT_SPEC,	1,		1,		78, 				13,  				0, 		0x0,	0 },
};

WM_HWIN 	hSpectrumDialog;

#define	SPEC_TIMER_RESOLUTION	0
//
WM_HTIMER 						hTimerSpec;
#endif

// ------------------------------
//#include "touch_driver.h"
//extern struct TD 		t_d;
// ------------------------------

#define MEMDEV_SP_X				(SW_FRAME_X + SW_FRAME_WIDTH)
#define MEMDEV_SP_Y				SCOPE_Y
#define MEMDEV_SP_X_SZ			SCOPE_X_SIZE
#define MEMDEV_SP_Y_SZ			SCOPE_Y_SIZE

// Memory device created for the combined Spectrum/Waterfall
// control
#ifdef USE_MEM_DEVICE
//
GUI_MEMDEV_Handle hMemSpWf = 0;
#endif

const ulong waterfall_blue[64] =
{
    0x00002e,
    0x020231,
    0x050534,
    0x070837,
    0x0a0b3b,
    0x0c0d3e,
    0x0f1041,
    0x121345,
    0x141648,
    0x17194b,
    0x191b4f,
    0x1c1e52,
    0x1f2155,
    0x212459,
    0x24275c,
    0x26295f,
    0x292c63,
    0x2b2f66,
    0x2e3269,
    0x31356d,
    0x333770,
    0x363a73,
    0x383d76,
    0x3b407a,
    0x3e437d,
    0x404580,
    0x434884,
    0x454b87,
    0x484e8a,
    0x4b518e,
    0x4d5391,
    0x505694,
    0x525998,
    0x555c9b,
    0x575e9e,
    0x5a61a2,
    0x5d64a5,
    0x5f67a8,
    0x626aac,
    0x646caf,
    0x676fb2,
    0x6a72b6,
    0x6c75b9,
    0x6f78bc,
    0x717abf,
    0x747dc3,
    0x7780c6,
    0x7983c9,
    0x7c86cd,
    0x7e88d0,
    0x818dd3,
    0x838ed7,
    0x8691da,
    0x8994dd,
    0x8b96e1,
    0x8e99e4,
    0x909ce7,
    0x939feb,
    0x96a2ee,
    0x98a4f1,
    0x9ba7f5,
    0x9ddaf8,
    0xa0dffb,
    //0xa3e5ff,
    0xffffff
};

static void ui_controls_create_header_big(void);
static void ui_controls_create_bottom_bar(void);

struct UI_SW	ui_sw;

// Public radio state
extern struct	TRANSCEIVER_STATE_UI	tsu;

// UI driver public state
extern struct	UI_DRIVER_STATE			ui_s;

// This control bounds
struct WidgetBounds						sb;

// temp public
//unsigned char fft_value[1024];

// Default values
//int 		bandpass_start 	= 360;
//int 		bandpass_end   	= 400;

ushort		old_dif			= 0;

uchar 		loc_vfo_mode;
// api driver test
//extern uchar api_conv_type;

uchar 	api_conv_type 	= 1;						// smooth waterfall
uchar 	sw_light		= 1;						// simplified scope (less resources)

// -------------------------------
//
// Use a spare RAM region as backup
// for quick waterfall restore
// we don't save waterfall values, but
// rather compressed pointer indexes
// to brightness table
//
//
#define USE_WF_BACKUP_BUFFER
//
#ifdef USE_WF_BACKUP_BUFFER
//
// Around (WATERFALL_Y_SIZE * WATERFALL_X_SIZE) bytes(reserved 140k), as 790kB used by GUIConf.c as emWin heap!
// Note: Using the #defines as count result in smaller buffer(compiler bug)
//
#define WF_BKP_SIZE			136850
//
__attribute__((section(".STemWinMemPool"))) __attribute__ ((aligned (32))) uchar wf_bkp[WF_BKP_SIZE];
uchar wf_init = 0;
#endif
//
// -------------------------------

static ushort chk_x(ushort x)
{
	ushort ret = x;

	if(ret >= (MEMDEV_SP_X + MEMDEV_SP_X_SZ))
		ret = (MEMDEV_SP_X + MEMDEV_SP_X_SZ) - 2;

	if(ret <= MEMDEV_SP_X)
		ret = (MEMDEV_SP_X + 2);

	return ret;
}

static ushort chk_y(ushort y)
{
	ushort ret = y;

	if(ret >= (MEMDEV_SP_Y + MEMDEV_SP_Y_SZ))
		ret = (MEMDEV_SP_Y + MEMDEV_SP_Y_SZ) - 2;

	if(ret <= MEMDEV_SP_Y)
		ret = (MEMDEV_SP_Y + 2);
	return ret;
}

static void ui_controls_spectrum_fft_process_big(void)
{
	ulong i, shift;
	uchar pixel;

	// Debug only - monitor FFT data
	#if 0
	if((ui_sw.fft_dsp[400] == ui_sw.fft_dsp[401]) &&\
	   (ui_sw.fft_dsp[401] == ui_sw.fft_dsp[402]) &&\
	   (ui_sw.fft_dsp[402] == ui_sw.fft_dsp[403]) &&\
	   (ui_sw.fft_dsp[403] == ui_sw.fft_dsp[404]))
	{
		static uchar fft_mon_skip = 0;
		fft_mon_skip++;
		if(fft_mon_skip > 40)
		{
			print_hex_array(ui_sw.fft_dsp + 400, 8);
			fft_mon_skip = 0;
		}
	}
	#endif

	for(i = 0; i < SCOPE_X_SIZE; i++)
	{
		// 40 kHz span
		if(tsu.band[tsu.curr_band].span == 40000)
		{
			shift = (1024 - SCOPE_X_SIZE)/2;
			pixel = ui_sw.fft_dsp[i + shift];

			ui_sw.fft_value[i] = pixel;
		}

		// 20 kHz span
		if(tsu.band[tsu.curr_band].span == 20000)
		{
			if(i < 427)
			{
				shift = (1024 - SCOPE_X_SIZE)/2 + SCOPE_X_SIZE/4;
				pixel = ui_sw.fft_dsp[i + shift];

				ui_sw.fft_value[i*2 + 0] = pixel;
				ui_sw.fft_value[i*2 + 1] = pixel;
			}
		}

		// 10 kHz span
		if(tsu.band[tsu.curr_band].span == 10000)
		{
			if(i < 213)
			{
				shift = (1024 - SCOPE_X_SIZE)/2 + 321;
				pixel = ui_sw.fft_dsp[i + shift];

				ui_sw.fft_value[i*4 + 1] = pixel;	// +1 shift to get it centred!
				ui_sw.fft_value[i*4 + 2] = pixel;
				ui_sw.fft_value[i*4 + 3] = pixel;
				ui_sw.fft_value[i*4 + 4] = pixel;
			}
		}
	}
}

static void ui_controls_spectrum_decide_bandpass(void)
{
	ushort bandpass_center 		= SPECTRUM_MID_POINT;
	uchar  bandpass_halfwidth  	= 43;	//20;

	// Calculate strip width
	//switch(tsu.dsp_filter)
	switch(tsu.band[tsu.curr_band].filter)
	{
		case AUDIO_300HZ:
			bandpass_halfwidth  = 3;
			break;
		case AUDIO_500HZ:
			bandpass_halfwidth  = 5;
			break;
		case AUDIO_1P8KHZ:
			bandpass_halfwidth  = 15;
			break;
		case AUDIO_2P3KHZ:
			bandpass_halfwidth  = 20;
			break;
		case AUDIO_3P6KHZ:
			bandpass_halfwidth  = 30;
			break;
		case AUDIO_WIDE:
			bandpass_halfwidth  = 80;
			break;
		default:
			break;
	}

	// If NCO is on, will move the bandstrip around
	if(tsu.band[tsu.curr_band].nco_freq != 0)
		bandpass_center = SPECTRUM_MID_POINT + ((tsu.band[tsu.curr_band].nco_freq/1000)*22);	//16

	if(tsu.band[tsu.curr_band].demod_mode == DEMOD_LSB)
		bandpass_center -= bandpass_halfwidth;

	if(tsu.band[tsu.curr_band].demod_mode == DEMOD_USB)
		bandpass_center += bandpass_halfwidth;

	// Calculate boundary
	ui_sw.bandpass_start = (bandpass_center - bandpass_halfwidth);
	ui_sw.bandpass_end   = (bandpass_center + bandpass_halfwidth);
}

// Show VFO centre frequency in Fixed mode, as Alpha blended text
static void ui_controls_spectrum_show_centre_freq(void)
{
	char buf[30];
	ulong freq = tsu.band[tsu.curr_band].vfo_a;

	if(!tsu.band[tsu.curr_band].fixed_mode)
		return;

	GUI_SetAlpha(88);						// Alpha is inverted!!!
	GUI_SetFont(&GUI_Font24B_ASCII);
	GUI_SetColor(HOT_PINK);

	sprintf(buf,"%d.%03d.%03d", (int)(freq/1000000), (int)((freq%1000000)/1000), (int)((freq%1000)/1));

	GUI_DispStringAt(buf, ((SW_FRAME_X + SPECTRUM_MID_POINT) - 47), (SCOPE_Y + 19));
	GUI_SetAlpha(255);
}

// After band change we would ideally wanna show band guide, and give the user
// a chance to go to to specific part of the band, instead of rotating the VFO
// like mad. Touch will be enabled as long as control is visible
static void ui_controls_spectrum_show_band_guide(void)
{
	static uchar band_guide_timeout = 0;

	if(!ui_s.show_band_guide)
		return;

	// Make it fade away
	if(band_guide_timeout > (BAND_GUIDE_TIMEOUT - BAND_GUIDE_FADE_TRIG_VAL))
	{
		uchar fade_alpha = BAND_GUIDE_START_ALPHA - ((band_guide_timeout - BAND_GUIDE_TIMEOUT + BAND_GUIDE_FADE_TRIG_VAL) * BAND_GUIDE_FADE_FACTOR);
		//printf("fade: %d\r\n", fade_alpha);
		GUI_SetAlpha(fade_alpha);
	}
	else
		GUI_SetAlpha(BAND_GUIDE_START_ALPHA);

	// Label frame
	GUI_SetColor(GUI_WHITE);
	GUI_FillRoundedRect(BAND_GUIDE_LEFT_LABLE_X - 4, BAND_GUIDE_LEFT_LABLE_Y, (BAND_GUIDE_LEFT_LABLE_X + 130), (BAND_GUIDE_LEFT_LABLE_Y + 22), 3);
	GUI_FillRoundedRect(BAND_GUIDE_MIDP_LABLE_X - 4, BAND_GUIDE_MIDP_LABLE_Y, (BAND_GUIDE_MIDP_LABLE_X + 138), (BAND_GUIDE_MIDP_LABLE_Y + 22), 3);
	GUI_FillRoundedRect(BAND_GUIDE_RIGH_LABLE_X - 4, BAND_GUIDE_RIGH_LABLE_Y, (BAND_GUIDE_RIGH_LABLE_X + 134), (BAND_GUIDE_RIGH_LABLE_Y + 22), 3);

	// Label text
	GUI_SetFont(&GUI_Font24B_ASCII);
	GUI_SetColor(GUI_BLACK);
	GUI_DispStringAt("Jump to CW", 		BAND_GUIDE_LEFT_LABLE_X, BAND_GUIDE_LEFT_LABLE_Y);
	GUI_DispStringAt("Jump to Data", 	BAND_GUIDE_MIDP_LABLE_X, BAND_GUIDE_MIDP_LABLE_Y);
	GUI_DispStringAt("Jump to SSB", 	BAND_GUIDE_RIGH_LABLE_X, BAND_GUIDE_RIGH_LABLE_Y);

	GUI_SetAlpha(255);

	// Increase timer
	band_guide_timeout++;

	// Hide it
	if(band_guide_timeout > BAND_GUIDE_TIMEOUT)
	{
		ui_s.show_band_guide = 0;
		band_guide_timeout = 0;
	}
}

// Bandpass strip and needle
static void ui_controls_spectrum_show_band_strip(void)
{
	GUI_RECT 	Rect;

	// Calculate bandpass strip boundaries
	ui_controls_spectrum_decide_bandpass();

	if(!tsu.band[tsu.curr_band].fixed_mode)
	{
		// Draw alpha effect for bandpass
		Rect.x0 = (SW_FRAME_X + SW_FRAME_WIDTH);
		Rect.x1 = (SW_FRAME_X + SW_FRAME_WIDTH) + 3;
		Rect.y0 = SCOPE_Y;
		Rect.y1 = SCOPE_Y + SCOPE_Y_SIZE;
		GUI_SetClipRect(&Rect);
		GUI_SetColor(GUI_LIGHTGRAY);	//0xd99100
		GUI_SetAlpha(88);		// Alpha is inverted!!!
		GUI_FillRoundedRect((SW_FRAME_X + SW_FRAME_WIDTH) + 4, SCOPE_Y, (SW_FRAME_X + SW_FRAME_WIDTH) + SCOPE_X_SIZE, SCOPE_Y + SCOPE_Y_SIZE, 4);
		GUI_SetClipRect(NULL);
		// passband strip
		GUI_FillRect((SW_FRAME_X + SW_FRAME_WIDTH) + ui_sw.bandpass_start, SCOPE_Y + 1, (SW_FRAME_X + SW_FRAME_WIDTH) + ui_sw.bandpass_end,   SCOPE_Y + SCOPE_Y_SIZE);
		// Red line - middle of passband
		GUI_SetColor(GUI_RED);
		GUI_DrawVLine(((SW_FRAME_X + SW_FRAME_WIDTH) + (ui_sw.bandpass_end - ui_sw.bandpass_start)/2 + ui_sw.bandpass_start),SCOPE_Y,SCOPE_Y + SCOPE_Y_SIZE);
		GUI_SetAlpha(255);	//0
	}
	else
	{
		// Red needle, double width
		GUI_SetColor(GUI_RED);
		GUI_DrawVLine(((SW_FRAME_X + SW_FRAME_WIDTH) + (ui_sw.bandpass_end - ui_sw.bandpass_start)/2 + ui_sw.bandpass_start),SCOPE_Y,SCOPE_Y + SCOPE_Y_SIZE);
		GUI_DrawVLine(((SW_FRAME_X + SW_FRAME_WIDTH) + (ui_sw.bandpass_end - ui_sw.bandpass_start)/2 + ui_sw.bandpass_start + 1),SCOPE_Y,SCOPE_Y + SCOPE_Y_SIZE);
	}
}

// Are we moving bandpass strip ?
// Just a basic digitizer test
static void ui_controls_spectrum_move_band_strip(void)
{
	//ulong 		i;//,j;
	//int 		old_x,old_y,new_x,new_y;
	//uchar		val;
	//ushort		dif;

	// Check boundary - is touch event into this control at all ?
	/*if((t_d.point_y[0] < (SCOPE_Y + SCOPE_Y_SIZE)) && (t_d.point_y[0] > (SCOPE_Y + 0)))
	{
		// --------------------------------------------
		// Move bandpass - one finger
		if(t_d.count == 1)
		{
			bandpass_start = t_d.point_x[0];
			bandpass_end   = bandpass_start + 40;
		}

		// --------------------------------------------
		// Resizing bandpass via pinch/stretch ?
		if(t_d.count == 2)
		{
			if(t_d.point_x[0] > t_d.point_x[1])
				dif = t_d.point_x[0] - t_d.point_x[1];
			else
				dif = t_d.point_x[1] - t_d.point_x[0];

			// Expanding or contracting ?
			if(old_dif < dif)
			{
				bandpass_start -= 10;
				bandpass_end   += 10;
			}
			else
			{
				bandpass_start += 10;
				bandpass_end   -= 10;
			}
			old_dif = dif;
		}
	}*/
}

//*----------------------------------------------------------------------------
//* Function Name       : ui_controls_spectrum_repaint
//* Object              : - at 200MHz, 99% CPU usage and still jerky! -
//* Input Parameters    :
//* Output Parameters   :
//* Functions called    :
//*----------------------------------------------------------------------------
static void ui_controls_spectrum_repaint_big(FAST_REFRESH *cb)
{
	ulong 		i;
	ushort 		old_x, old_y, new_x, new_y;
	uchar		val;

	// Not implemented
	ui_controls_spectrum_move_band_strip();

	// Select device
	#ifdef USE_MEM_DEVICE
	GUI_MEMDEV_Select(hMemSpWf);
	#endif

	// Draw background
	GUI_SetColor(GUI_BLACK);
	GUI_FillRect(	((SW_FRAME_X + SW_FRAME_WIDTH)				),
					(SCOPE_Y				),
					((SW_FRAME_X + SW_FRAME_WIDTH) + SCOPE_X_SIZE ),
					(SCOPE_Y + SCOPE_Y_SIZE )
				);

	// Draw horizontal grid lines
	#if 1
	GUI_SetColor(GUI_DARKGRAY);
	for (i = 0; i < 5; i++)
	{
		GUI_DrawHLine(((SCOPE_Y + SCOPE_Y_SIZE - 2) - (i * 23)),(sb.x + 4),(sb.x + SW_FRAME_X_SIZE - 4));
	}
	#endif

	// Draw vertical grid lines
	#if 1
	GUI_SetColor(GUI_DARKGRAY);
	for (i = 0; i < 7; i++)
	{
	    GUI_DrawVLine((43 + i*128), SCOPE_Y, (SCOPE_Y + SCOPE_Y_SIZE - 2));
	}
	#endif

	// Draw bars
	for (i = 0; i < SCOPE_X_SIZE; i++)
	{
		val  = ui_sw.fft_value[i];

		// Top clipping
		//if(val > SCOPE_Y_SIZE)
		//	val = SCOPE_Y_SIZE;

		// New point position
		new_x = chk_x(((SW_FRAME_X + SW_FRAME_WIDTH) + i));
		new_y = chk_y(SCOPE_Y + SCOPE_Y_SIZE - val);

		// Gradient vertical line
		#if 1
		GUI_DrawGradientV(new_x, new_y, new_x, chk_y(SCOPE_Y + SCOPE_Y_SIZE), GUI_LIGHTRED, GUI_LIGHTGREEN);
		#endif

		// Print vertical line for each point, transparent, to fill the spectrum
		#if 0
		GUI_SetColor(GUI_WHITE);
		GUI_SetAlpha(128);
		GUI_DrawVLine(new_x, new_y, chk_y(SCOPE_Y + SCOPE_Y_SIZE));
		GUI_SetAlpha(255);
		#endif

		// Draw point
		// Causes draw outside of MEMDEV!!!
		#if 0
		GUI_SetColor(GUI_WHITE);
		GUI_DrawPixel(new_x, new_y);
		#endif

		// Draw line between old and new point
		// Causes draw outside of MEMDEV!!!
		#if 0
		GUI_SetColor(GUI_WHITE);
		if(i)
		{
			if((old_x < new_x)&&(old_y < new_y))
				GUI_DrawLine(old_x, old_y, new_x, new_y);
			else if((new_x < old_x)&&(new_y < old_y))
				GUI_DrawLine(new_x, new_y, old_x, old_y);
		}
		#endif

		// Save old point
		old_x = new_x;
		old_y = new_y;

		// Fast UI update callback
		if(cb)
		{
			#ifdef USE_MEM_DEVICE
			GUI_MEMDEV_Select(0);
			cb();
			GUI_MEMDEV_Select(hMemSpWf);
			#else
			cb();
			#endif
		}
	}

	// Show VFO centre frequency in Fixed mode, as Alpha blended text
	ui_controls_spectrum_show_centre_freq();

	// Show band guide control
	ui_controls_spectrum_show_band_guide();

	// Bandpass strip and needle
	ui_controls_spectrum_show_band_strip();

	#ifdef USE_MEM_DEVICE
	GUI_MEMDEV_CopyToLCD(hMemSpWf);		// Execute
	GUI_MEMDEV_Select(0);				// Allow normal text print in other controls
	#endif
}

//*----------------------------------------------------------------------------
//* Function Name       : ui_controls_spectrum_wf_repaint
//* Object              : - at 200MHz, 86% CPU usage, speed is good!
//* Input Parameters    :
//* Output Parameters   :
//* Functions called    :
//*----------------------------------------------------------------------------
static void ui_controls_spectrum_wf_repaint_big(FAST_REFRESH *cb)
{
	ulong 		i, j, m, val;

	// Initial fill(from backup table)
	#ifdef USE_WF_BACKUP_BUFFER
	if(cb == NULL)
	{
		m = 0;
		for (j = 0; j < WATERFALL_Y_SIZE; j++)
		{
			for (i = 0; i < WATERFALL_X_SIZE; i++)
			{
				val = wf_bkp[m++] & 0x3F;
				GUI_SetColor ((0xFF << 24) | waterfall_blue[val]);	// add Alpha value
				GUI_DrawPixel(((SW_FRAME_X + SW_FRAME_WIDTH) + i), WATERFALL_Y + j);
			}
		}
		return;
	}
	#endif

	#if 0
	// Move down - single line
	for (i = WATERFALL_Y_SIZE; i > 0; i -= 1)
	{
		GUI_CopyRect(SW_FRAME_X + SW_FRAME_WIDTH,
					 WATERFALL_Y - 1 + i,
					 SW_FRAME_X + SW_FRAME_WIDTH,
					 WATERFALL_Y + i,
					 WATERFALL_X_SIZE,
					 1);
	}
	#else
	// -----------------------------------------------------------------------------------------------------------
	// Move waterfall down - rect copy
	GUI_CopyRect(SW_FRAME_X + SW_FRAME_WIDTH,	// Upper left X-position of the source rectangle.
				 WATERFALL_Y,					// Upper left Y-position of the source rectangle.
				 SW_FRAME_X + SW_FRAME_WIDTH,	// Upper left X-position of the destination rectangle.
				 WATERFALL_Y + 1,				// Upper left Y-position of the destination rectangle.
				 WATERFALL_X_SIZE,				// X-size of the rectangle.
				 WATERFALL_Y_SIZE - 1);			// Y-size of the rectangle.

	#endif

	// Move backup memory
	#ifdef USE_WF_BACKUP_BUFFER
	for(int i = 0; i < (WATERFALL_Y_SIZE - 1); i++)
	{
		memcpy( wf_bkp + ((WATERFALL_Y_SIZE - i - 1)*WATERFALL_X_SIZE),
				wf_bkp + ((WATERFALL_Y_SIZE - i - 2)*WATERFALL_X_SIZE),
				WATERFALL_X_SIZE);
	}
	#endif

	// Update top line
	for (i = 0; i < WATERFALL_X_SIZE; i++)
	{
		val  = ui_sw.fft_value[i];
		val &= 0x3F;

		// Save line
		#ifdef USE_WF_BACKUP_BUFFER
		wf_bkp[i] = val;
		#endif

		// ToDo: Fix gradient table addressing
		GUI_SetColor ((0xFF << 24) | waterfall_blue[val]);	// add Alpha value
		GUI_DrawPixel(((SW_FRAME_X + SW_FRAME_WIDTH) + i), WATERFALL_Y);

		// Fast UI update callback
		if(cb)
		{
			//GUI_SelectLayer(0);
			cb();
			//GUI_MEMDEV_Select(hMemSpWf);
		}
	}
}

static void ui_controls_update_smooth_control(uchar init)
{
#if 0
	uchar pub_smooth;

	pub_smooth = *(uchar *)(EEP_BASE + EEP_SW_SMOOTH);

	if(!init)
	{
		if(api_conv_type == pub_smooth)
			return;
	}
	//printf("smooth=%d\r\n",api_conv_type);

	// 'Smooth' indicator holder
	GUI_SetColor(GUI_LIGHTBLUE);

	//if(ui_sw.ctrl_type == SW_CONTROL_BIG)
		GUI_FillRoundedRect(SMOOTH_X,SMOOTH_Y,(SMOOTH_X + SMOOTH_X_SIZE),(SMOOTH_Y + SMOOTH_Y_SIZE),2);
	//else
	//	GUI_FillRoundedRect(SMOOTH_X_MID,SMOOTH_Y_MID,(SMOOTH_X_MID + SMOOTH_X_SIZE_MID),(SMOOTH_Y_MID + SMOOTH_Y_SIZE_MID),2);

	// Color based on public state
	if(pub_smooth)
		GUI_SetColor(GUI_WHITE);
	else
		GUI_SetColor(GUI_GRAY);

	// 'Smooth' indicator text
	GUI_SetFont(&GUI_Font16B_ASCII);

	//if(ui_sw.ctrl_type == SW_CONTROL_BIG)
		GUI_DispStringAt("SMOOTH",(SMOOTH_X + 5),(SMOOTH_Y + 0));
	//else
	//	GUI_DispStringAt("SMOOTH",(SMOOTH_X_MID + 5),(SMOOTH_Y_MID + 0));

	//loc_smooth_val = api_conv_type;

	// Flip flag
	//api_conv_type = !api_conv_type;

	api_conv_type = pub_smooth;
#endif
}

// Center/Fixed
//
static void ui_controls_update_vfo_mode(bool is_init)
{
#if 0
	// Skip needless repaint
	if((!is_init) && (loc_vfo_mode == tsu.band[tsu.curr_band].fixed_mode))
		return;

	//printf("update vfo mode\r\n");

	// 'Smooth' indicator holder
	GUI_SetColor(GUI_WHITE);

	//if(ui_sw.ctrl_type == SW_CONTROL_BIG)
		GUI_FillRect(CENTER_X,CENTER_Y,(CENTER_X + CENTER_X_SIZE),(CENTER_Y + CENTER_Y_SIZE));
	//else
	//	GUI_FillRoundedRect(CENTER_X_MID,CENTER_Y_MID,(CENTER_X_MID + CENTER_X_SIZE_MID),(CENTER_Y_MID + CENTER_Y_SIZE_MID),2);

	GUI_SetColor(GUI_LIGHTBLUE);
	GUI_SetFont(&GUI_Font16B_ASCII);

	if(ui_sw.ctrl_type == SW_CONTROL_BIG)
	{
		if(tsu.band[tsu.curr_band].fixed_mode)
			GUI_DispStringAt("FIXED",(CENTER_X + 14),(CENTER_Y + 0));
		else
			GUI_DispStringAt("CENTER",(CENTER_X + 8),(CENTER_Y + 0));
	}
	//else
	//{
	//	if(tsu.band[tsu.curr_band].fixed_mode)
	//		GUI_DispStringAt("FIXED",(CENTER_X_MID + 14),(CENTER_Y_MID + 0));
	//	else
	//		GUI_DispStringAt("CENTER",(CENTER_X_MID + 8),(CENTER_Y_MID + 0));
	//}

	loc_vfo_mode = tsu.band[tsu.curr_band].fixed_mode;
#endif
}

void ui_controls_update_span(void)
{
	return;

	ui_controls_create_header_big();
	ui_controls_create_bottom_bar();
}

static void ui_spectrum_create_span(void)
{
	GUI_SetColor(GUI_WHITE);
	GUI_SetFont(&GUI_Font16B_ASCII);
	char buf[50];
	sprintf(buf, "SPAN %dkHz", (int)(tsu.band[tsu.curr_band].span/1000));
	GUI_DispStringAt(buf,(SPAN_X + 55),(SPAN_Y + 0));
}

static void ui_controls_create_header_big(void)
{
#if 0
	// Left part
	GUI_DrawGradientH(	(sb.x + SW_FRAME_WIDTH),
						(sb.y + 2),
						(sb.x + SW_FRAME_X_SIZE - 0)/2 + 0,
						(sb.y + 22),
						GUI_DARKGRAY,
						GUI_GRAY
						);

	// Right part
	GUI_DrawGradientH(	(sb.x + SW_FRAME_X_SIZE - 0)/2,
						(sb.y + 2),
						(sb.x + SW_FRAME_X_SIZE - 0),
						(sb.y + 22),
						GUI_GRAY,
						GUI_DARKGRAY
						);
#endif

#if 1
	// Common labels colour and font
	GUI_SetFont(&GUI_Font20B_ASCII);
	GUI_SetColor(GUI_WHITE);

	GUI_SetAlpha(30);
	//GUI_DispStringAt("BMS",		(sb.x + 18),	(sb.y + 2));						// BMS label
	//GUI_DispStringAt("AUDIO",	(sb.x + 140),	(sb.y + 2));						// AUDIO label
	//GUI_DispStringAt("VFO",		(sb.x + 283),	(sb.y + 2));						// VFO label
	//GUI_DispStringAt("KEYBOARD",((sb.x + SW_FRAME_X_SIZE - 2)/2 - 50),(sb.y + 2));	// KEYBOARD label
	//GUI_DispStringAt("AGC/ATT",	((sb.x + SW_FRAME_X_SIZE - 2)/2 + 90),(sb.y + 2));	// AGC/ATT label
	GUI_SetAlpha(255);
#endif

	//ui_controls_update_vfo_mode(true);												// CENTER/FIX
	//ui_spectrum_create_span();														// Span  control

}

// Is specific part of top bar touched ?
int ui_controls_spectrum_is_touch(int x, int y)
{
	int bar_x, bar_y;

	//-------------------------------------------
	// BMS position
	//bar_x = (sb.x + 18);
	//bar_y = (sb.y +  2);

	// Is BMS label touched ?
	//if((x > bar_x) && (x < (bar_x + 80)) && (y > (bar_y - 20)) && (y < bar_y + 30))
	//	return 1;

	//-------------------------------------------
	// AUDIO position
	//bar_x = (sb.x + 135);
	//bar_y = (sb.y +  2);

	// Is AUDIO label touched ?
	//if((x > bar_x) && (x < (bar_x + 80)) && (y > (bar_y - 20)) && (y < bar_y + 30))
	//	return 2;

	//-------------------------------------------
	// VFO position
	//bar_x = (sb.x + 283);
	//bar_y = (sb.y +  2);

	// Is VFO label touched ?
	//if((x > bar_x) && (x < (bar_x + 80)) && (y > (bar_y - 20)) && (y < bar_y + 30))
	//	return 3;

	//-------------------------------------------
	// KEYBOARD position
	//bar_x = (sb.x + SW_FRAME_X_SIZE - 2)/2 - 50;
	//bar_y = (sb.y + 2);

	// Is KEYBOARD label touched ?
	//if((x > bar_x) && (x < (bar_x + 120)) && (y > (bar_y - 20)) && (y < bar_y + 30))
	//	return 4;

	//-------------------------------------------
	// AGC/ATT position
	//bar_x = (sb.x + SW_FRAME_X_SIZE - 2)/2 + 90;
	//bar_y = (sb.y + 2);

	// Is AGC/ATT label touched ?
	//if((x > bar_x) && (x < (bar_x + 120)) && (y > (bar_y - 20)) && (y < bar_y + 30))
	//	return 5;

	//-------------------------------------------
	// Center/Fix position
	//bar_x = CENTER_X;
	//bar_y = CENTER_Y;

	// Is VFO label touched ?
	//if((x > bar_x) && (x < (bar_x + 80)) && (y > (bar_y - 20)) && (y < bar_y + 30))
	//	return 6;

	//-------------------------------------------
	// SPAN position
	//bar_x = SPAN_X + 55;
	//bar_y = SPAN_Y;

	// Is VFO label touched ?
	//if((x > bar_x) && (x < (bar_x + 80)) && (y > (bar_y - 20)) && (y < bar_y + 30))
	//	return 7;

	// Band guide labels touchable only if visible
	if(ui_s.show_band_guide)
	{
		bar_x = BAND_GUIDE_LEFT_LABLE_X;
		bar_y = BAND_GUIDE_LEFT_LABLE_Y;

		// Is VFO label touched ?
		if((x > bar_x) && (x < (bar_x + 80)) && (y > (bar_y - 20)) && (y < bar_y + 30))
			return 8;

		bar_x = BAND_GUIDE_MIDP_LABLE_X;
		bar_y = BAND_GUIDE_MIDP_LABLE_Y;

		// Is VFO label touched ?
		if((x > bar_x) && (x < (bar_x + 80)) && (y > (bar_y - 20)) && (y < bar_y + 30))
			return 9;

		bar_x = BAND_GUIDE_RIGH_LABLE_X;
		bar_y = BAND_GUIDE_RIGH_LABLE_Y;

		// Is VFO label touched ?
		if((x > bar_x) && (x < (bar_x + 80)) && (y > (bar_y - 20)) && (y < bar_y + 30))
			return 10;
	}

	return 0;
}

static void ui_controls_create_sw_big(void)
{
	#ifdef USE_MEM_DEVICE
	// Create memory device (for spectrum only!)
	if (hMemSpWf == 0)
	{
		hMemSpWf = GUI_MEMDEV_Create(MEMDEV_SP_X, MEMDEV_SP_Y,
									 MEMDEV_SP_X_SZ, MEMDEV_SP_Y_SZ);
		if (hMemSpWf == 0)
		{
			printf("err mem dev!\r\n");

			// ToDo: stall or handle error!
			return; // Not enough memory available
		}
	}

	// Select device
	GUI_MEMDEV_Select(hMemSpWf);
	#endif

	// Initial clear of control
	GUI_SetColor(GUI_BLACK);
	GUI_FillRect(	(SW_FRAME_X + SW_FRAME_WIDTH),
					SCOPE_Y,
					((SW_FRAME_X + SW_FRAME_WIDTH) + SCOPE_X_SIZE),
					(SCOPE_Y + SCOPE_Y_SIZE)
				);

	// Execute
	#ifdef USE_MEM_DEVICE
	GUI_MEMDEV_CopyToLCD(hMemSpWf);
	#endif

	// Initial control paint
	ui_controls_spectrum_repaint_big(NULL);
	ui_controls_spectrum_wf_repaint_big(NULL);

    // Allow normal text print in other controls
	#ifdef USE_MEM_DEVICE
	//GUI_SelectLayer(0);
	GUI_MEMDEV_Select(0);
	#endif

	//printf("%d, %d \r\n",sb.x ,sb.y );

	// Draw frame/lines
	GUI_SetColor(GUI_DARKGRAY);			//HOT_PINK
	#if 1
	//GUI_DrawHLine((sb.y + 1), 					sb.x, (sb.x + SW_FRAME_X_SIZE));
	//GUI_DrawHLine((sb.y + 0), 					sb.x, (sb.x + SW_FRAME_X_SIZE));
	//GUI_DrawHLine((sb.y - 1), 					sb.x, (sb.x + SW_FRAME_X_SIZE));
	//GUI_DrawHLine((sb.y - 2), 					sb.x, (sb.x + SW_FRAME_X_SIZE));
/*	GUI_DrawRoundedFrame(	sb.x,
							sb.y,
							(sb.x + SW_FRAME_X_SIZE),
							(sb.y + SW_FRAME_Y_SIZE),
							SW_FRAME_CORNER_R,
							SW_FRAME_WIDTH
						);*/
	#else
	// Top
	GUI_DrawHLine((sb.y + 21), 					sb.x, (sb.x + SW_FRAME_X_SIZE));
	GUI_DrawHLine((sb.y + 22), 					sb.x, (sb.x + SW_FRAME_X_SIZE));
	// Bottom
	GUI_DrawHLine((sb.y - 14 + SW_FRAME_Y_SIZE), sb.x, (sb.x + SW_FRAME_X_SIZE));
	GUI_DrawHLine((sb.y - 13 + SW_FRAME_Y_SIZE), sb.x, (sb.x + SW_FRAME_X_SIZE));
	GUI_SetAlpha(128);
	GUI_DrawHLine((sb.y - 12 + SW_FRAME_Y_SIZE), sb.x, (sb.x + SW_FRAME_X_SIZE));
	GUI_SetAlpha(88);
	GUI_DrawHLine((sb.y - 11 + SW_FRAME_Y_SIZE), sb.x, (sb.x + SW_FRAME_X_SIZE));
	GUI_SetAlpha(255);
	#endif

	// Draw header
	ui_controls_create_header_big();

	#if 0
	int i,j,x0,y0,FontSizeY;
	// Bottom divider below waterfall
	GUI_SetColor(GUI_DARKGRAY);
	//GUI_DrawHLine((WATERFALL_Y + WATERFALL_Y_SIZE), ((SW_FRAME_X + SW_FRAME_WIDTH) + SW_FRAME_WIDTH),((SW_FRAME_X + SW_FRAME_WIDTH) + WATERFALL_X_SIZE - SW_FRAME_WIDTH));
	GUI_FillRect((SW_FRAME_X + SW_FRAME_WIDTH),
				 (WATERFALL_Y + WATERFALL_Y_SIZE),
				 (SW_FRAME_X + SW_FRAME_WIDTH + WATERFALL_X_SIZE),
				 (WATERFALL_Y + WATERFALL_Y_SIZE) + 17);

	// Bottom Frequency Span markers
	GUI_SetFont(&GUI_Font8x16_1);
	GUI_SetColor(GUI_ORANGE);
	FontSizeY = GUI_GetFontSizeY();

	int sm;
	if(tsu.band[tsu.curr_band].span == 20000)
		sm = -9;
	else if (tsu.band[tsu.curr_band].span == 10000)
		sm = -6;
	else
		sm = -18;

	// 1024 FFT points, sample frequency 48 kHz, spectrum X size 854, ~47 Hz per pixel -> 40031 Hz span
	// Start mark is -(Span/2 - 2)
	for (i = 0, j = sm; i < 7; i++)
	{
		// Pixels spread
	    x0 = 36 + i*128;

	    y0 = (WATERFALL_Y + WATERFALL_Y_SIZE) + 4;
	    GUI_GotoXY(x0 + 8, y0 + 7 - FontSizeY / 2);
	    GUI_SetTextAlign(GUI_TA_HCENTER);

	    // Digit values
	    int v = j + (-sm/3)*i;
	    //printf("v=%d\r\n",v);
	    GUI_DispDecMin(v);

	    // line markers above digits
	    GUI_DrawVLine((x0 + 7),(y0 - 4),(y0 - 1));
	}
	#else
	ui_controls_create_bottom_bar();
	#endif
}

static void ui_controls_create_bottom_bar(void)
{
	int i,j,x0,y0,FontSizeY;


	//return;


	// Bottom divider below waterfall
	//GUI_SetColor(GUI_DARKGRAY);
	//GUI_DrawHLine((WATERFALL_Y + WATERFALL_Y_SIZE), ((SW_FRAME_X + SW_FRAME_WIDTH) + SW_FRAME_WIDTH),((SW_FRAME_X + SW_FRAME_WIDTH) + WATERFALL_X_SIZE - SW_FRAME_WIDTH));
	GUI_DrawGradientH((SW_FRAME_X + SW_FRAME_WIDTH),
				 	 (WATERFALL_Y + WATERFALL_Y_SIZE),
					 (SW_FRAME_X + SW_FRAME_WIDTH + WATERFALL_X_SIZE),
					 (WATERFALL_Y + WATERFALL_Y_SIZE) + 17,
					 GUI_DARKGRAY, GUI_GRAY		);

	// Bottom Frequency Span markers
	GUI_SetFont(&GUI_Font8x16_1);
	//GUI_SetColor(GUI_BLACK);
	FontSizeY = GUI_GetFontSizeY();

	int sm;
	if(tsu.band[tsu.curr_band].span == 20000)
		sm = -9000;
	else if (tsu.band[tsu.curr_band].span == 10000)
		sm = -4500;
	else
		sm = -18000;

	// 1024 FFT points, sample frequency 48 kHz, spectrum X size 854, ~47 Hz per pixel -> 40031 Hz span
	// Start mark is -(Span/2 - 2)
	for (i = 0, j = sm; i < 7; i++)
	{
		// Pixels spread
		#ifdef STARTEK_5INCH
	    x0 = 36 + i*128;
		#else
	    x0 = 36 + i*120;
		#endif

	    y0 = (WATERFALL_Y + WATERFALL_Y_SIZE) + 4;
	    GUI_GotoXY(x0 + 8, y0 + 7 - FontSizeY / 2);
	    GUI_SetTextAlign(GUI_TA_HCENTER);

	    // Digit values
	    int v = j + (-sm/3)*i;
	    //printf("v=%d\r\n",v);

	    // Change '0' to center frequency in Fixed mode
		#if 0
	    if((tsu.band[tsu.curr_band].fixed_mode)&&(v == 0))
	    {
	    	GUI_DispDecMin(tsu.band[tsu.curr_band].vfo_a);
	    }
	    else
	    	GUI_DispDecMin(v);
		#else
	    GUI_SetColor(GUI_WHITE);
	    GUI_DispDecMin(v);
	    #endif

	    // line markers above digits
	    GUI_SetColor(HOT_PINK);
	    GUI_DrawVLine((x0 + 6),(y0 - 4),(y0 - 1));
	    GUI_DrawVLine((x0 + 7),(y0 - 4),(y0 - 1));
	    GUI_DrawVLine((x0 + 8),(y0 - 4),(y0 - 1));
	}
}

#ifdef SPEC_USE_WM
// ToDo:
// 1. activate mem device per windows : https://forum.segger.com/index.php/Thread/4859-Selectively-activate-MEMDEV-for-each-window/
// 2. Add memory device support in LCD driver
//
static void WDHandler(WM_MESSAGE *pMsg)
{
	WM_HWIN hItem;
	int 	Id, NCode;

	switch (pMsg->MsgId)
	{
		case WM_INIT_DIALOG:
		{
			#if 0
			// Initial clear of control
			GUI_SetColor(GUI_BLACK);
			GUI_FillRect(	sb.x,
							sb.y,
							(sb.x + sb.x_size),
							(sb.y + sb.y_size)
			);
			#endif

			ui_sw.ctrl_type 		= SW_CONTROL_BIG;
			ui_sw.bandpass_start 	= SPECTRUM_MID_POINT - SPECTRUM_DEF_HALF_BW*2;
			ui_sw.bandpass_end 		= SPECTRUM_MID_POINT;

			ui_controls_create_sw_big();

			hTimerSpec = WM_CreateTimer(pMsg->hWin, 0, SPEC_TIMER_RESOLUTION, 0);
			break;
		}

		case WM_TIMER:
		{
			#if 0
			if(tsu.wifi_rssi)
			{
				char buf[30];
				hItem = WM_GetDialogItem(pMsg->hWin, ID_TEXT_WIFI);
				sprintf(buf, "%d dBm", tsu.wifi_rssi);
				TEXT_SetText(hItem, buf);
			}
			#endif

			ui_controls_spectrum_fft_process_big();
			ui_controls_spectrum_repaint_big(NULL);
			//ui_controls_spectrum_wf_repaint_big();

			WM_InvalidateWindow(hSpectrumDialog);
			WM_RestartTimer(pMsg->Data.v, SPEC_TIMER_RESOLUTION);

			break;
		}

		case WM_PAINT:
			//ui_controls_spectrum_fft_process_big();
			//ui_controls_spectrum_repaint_big(NULL);
			//ui_controls_spectrum_wf_repaint_big();
			break;

		case WM_DELETE:
			//WM_DeleteTimer(hTimerWiFi);
			break;

		case WM_NOTIFY_PARENT:
		{
			Id    = WM_GetId(pMsg->hWinSrc);    // Id of widget
			NCode = pMsg->Data.v;               // Notification code

			//VDCHandler(pMsg,Id,NCode);
			break;
		}

		// Trap keyboard messages
		case WM_KEY:
		{
			switch (((WM_KEY_INFO*)(pMsg->Data.p))->Key)
			{
		        // Return from menu
		        case GUI_KEY_HOME:
		        {
		        	//printf("GUI_KEY_HOME\r\n");
		        	break;
		        }
			}
			break;
		}

		default:
			WM_DefaultProc(pMsg);
			break;
	}
}
#else
//*----------------------------------------------------------------------------
//* Function Name       : ui_controls_spectrum_refresh
//* Object              :
//* Notes    			: fast repaint ends up here
//* Notes   			:
//* Notes    			:
//* Context    			: CONTEXT_VIDEO
//*----------------------------------------------------------------------------
void ui_controls_spectrum_refresh(FAST_REFRESH *cb)
{
	ui_controls_update_vfo_mode(false);
	ui_controls_update_smooth_control(0);

	switch(ui_sw.ctrl_type)
	{
		case SW_CONTROL_BIG:
		{
			if(ui_sw.updated)
			{
				ui_controls_spectrum_fft_process_big();
				if(tsu.sc_enabled) ui_controls_spectrum_repaint_big(cb);
				if(tsu.wf_enabled) ui_controls_spectrum_wf_repaint_big(cb); // - super laggy
				ui_sw.updated = 0;
			}
			break;
		}
		#if 0
		case SW_CONTROL_MID:
			ui_controls_spectrum_fft_process_mid();
			ui_controls_spectrum_repaint_mid(cb);
			ui_controls_spectrum_wf_repaint_mid();
			break;
		case SW_CONTROL_SMALL:
			//ui_controls_create_sw_big();
			break;
		#endif
		default:
			break;
	}
}
#endif

//*----------------------------------------------------------------------------
//* Function Name       : ui_controls_spectrum_init
//* Object              :
//* Input Parameters    :
//* Output Parameters   :
//* Functions called    :
//*----------------------------------------------------------------------------
void ui_controls_spectrum_init(WM_HWIN hParent)
{
	int i;

	// Backup table init
	#ifdef USE_WF_BACKUP_BUFFER
	if(!wf_init)
	{
		for (i = 0; i < sizeof(wf_bkp); i++)
			wf_bkp[i] = 0;

		wf_init = 1;
	}
	#endif

	#ifdef SPEC_USE_WM
	hSpectrumDialog = GUI_CreateDialogBox(SpectrumDialog, GUI_COUNTOF(SpectrumDialog), WDHandler, hParent, sb.x, sb.y);
	#ifdef USE_MEM_DEVICE
	//--WM_EnableMemdev(hSpectrumDialog);
	#endif
	#else
	loc_vfo_mode = 0x99;

	// Clear waterfall
	GUI_SetColor(GUI_BLACK);
	GUI_FillRect(	((WATERFALL_X)				),
					(WATERFALL_Y				),
					((WATERFALL_X) + WATERFALL_X_SIZE ),
					(WATERFALL_Y + WATERFALL_Y_SIZE )
				);

	// Init bounds
	sb.x 		= SW_FRAME_X;
	sb.y 		= SW_FRAME_Y;
	sb.x_size 	= SW_CONTROL_X_SIZE;
	sb.y_size 	= SW_CONTROL_Y_SIZE;

	//printf("sw bounds: x: %d, y: %d, sx: %d, sy: %d\r\n",sb.x, sb.y, sb.x_size , sb.y_size );

	// Control size based on saved eeprom value - only in CW mode ??
	//if(*(uchar *)(EEP_BASE + EEP_KEYER_ON))
	//	ui_sw.ctrl_type = SW_CONTROL_MID;
	//else
	ui_sw.ctrl_type = SW_CONTROL_BIG;

	ui_sw.bandpass_start 	= SPECTRUM_MID_POINT - SPECTRUM_DEF_HALF_BW*2;
	ui_sw.bandpass_end 		= SPECTRUM_MID_POINT;

	//switch(ui_sw.ctrl_type)
	//{
	//	case SW_CONTROL_BIG:
	ui_controls_create_sw_big();
	//		break;
	//	case SW_CONTROL_MID:
			//ui_controls_create_sw_mid();
	//		break;
	//	case SW_CONTROL_SMALL:
	//		//ui_controls_create_sw_big();
	//		break;
	//	default:
	//		break;
	//}
	#endif
}

//*----------------------------------------------------------------------------
//* Function Name       : ui_controls_spectrum_quit
//* Object              :
//* Input Parameters    :
//* Output Parameters   :
//* Functions called    :
//*----------------------------------------------------------------------------
void ui_controls_spectrum_quit(void)
{
	#ifdef SPEC_USE_WM
	GUI_EndDialog(hSpectrumDialog, 0);
	#else
	#ifdef USE_MEM_DEVICE
	GUI_MEMDEV_Delete(hMemSpWf);
	hMemSpWf = 0;
	#endif
	#endif
}

#endif
