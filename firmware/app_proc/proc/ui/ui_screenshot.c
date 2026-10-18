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
#include "mchf_pro_board.h"
#include "main.h"

#ifdef CONTEXT_VIDEO

#include <stdio.h>
#include <string.h>

#include "ff.h"
#include "gui.h"

#include "LCDConf.h"
#include "ili9806e.h"
#include "ui_proc.h"
#include "ui_screenshot.h"

#ifdef DESKTOP_SHOW_SPECTRUM
#include "spectrum/ui_controls_spectrum.h"
#endif

// Screen capture to the SD card, for the user manual and bug reports.
// Hold F1 to request one; the UI task takes it between two repaints, so
// nothing draws while the frame buffers are read back. The picture is
// what the LTDC scans out - both layers blended the way the hardware does
// it - so it matches the glass, not emWin's back buffer

// Logical(landscape) screen size
#define SCR_W						ILI9806E_HEIGHT
#define SCR_H						ILI9806E_WIDTH
// Physical frame buffer line length, in pixels
#define SCR_PITCH					ILI9806E_WIDTH

#define SCR_MAX_INDEX				9999

// Public UI driver state
extern struct	UI_DRIVER_STATE			ui_s;

static volatile uchar 	scr_req = 0;
static ushort			scr_index = 1;

static FIL 				scr_fil;
static uchar 			scr_line[SCR_W * 3];

//*----------------------------------------------------------------------------
//* Function Name       : ui_screenshot_request
//* Object              : called from the keypad task
//* Context    			: CONTEXT_KEYPAD
//*----------------------------------------------------------------------------
void ui_screenshot_request(void)
{
	scr_req = 1;
}

static void scr_put16(uchar *p, ushort v)
{
	p[0] = (uchar)(v);
	p[1] = (uchar)(v >> 8);
}

static void scr_put32(uchar *p, ulong v)
{
	p[0] = (uchar)(v);
	p[1] = (uchar)(v >> 8);
	p[2] = (uchar)(v >> 16);
	p[3] = (uchar)(v >> 24);
}

//*----------------------------------------------------------------------------
//* Function Name       : scr_pixel_addr
//* Object              : logical(x,y) -> physical frame buffer word index
//* Notes    			: GUIDRV_LIN_OSY: physX = y, physY = xSize - 1 - x
//* Context    			: CONTEXT_VIDEO
//*----------------------------------------------------------------------------
static inline ulong scr_pixel_addr(int x, int y)
{
	#ifdef LCD_LANDSCAPE
	return (ulong)(SCR_W - 1 - x) * SCR_PITCH + (ulong)y;
	#else
	return (ulong)y * SCR_W + (ulong)x;
	#endif
}

//*----------------------------------------------------------------------------
//* Function Name       : scr_write_header
//* Object              : 24 bpp bottom-up BMP header
//* Context    			: CONTEXT_VIDEO
//*----------------------------------------------------------------------------
static uchar scr_write_header(void)
{
	uchar	h[54];
	ulong	img_size = sizeof(scr_line) * SCR_H;
	UINT	bw;

	memset(h, 0, sizeof(h));

	h[0] = 'B';
	h[1] = 'M';
	scr_put32(&h[2],  sizeof(h) + img_size);	// file size
	scr_put32(&h[10], sizeof(h));				// pixel data offset
	scr_put32(&h[14], 40);						// BITMAPINFOHEADER
	scr_put32(&h[18], SCR_W);
	scr_put32(&h[22], SCR_H);
	scr_put16(&h[26], 1);						// planes
	scr_put16(&h[28], 24);						// bpp
	scr_put32(&h[34], img_size);
	scr_put32(&h[38], 3780);					// 96 dpi
	scr_put32(&h[42], 3780);

	if(f_write(&scr_fil, h, sizeof(h), &bw) != FR_OK)
		return 1;

	return (bw != sizeof(h));
}

//*----------------------------------------------------------------------------
//* Function Name       : scr_capture
//* Object              :
//* Context    			: CONTEXT_VIDEO
//*----------------------------------------------------------------------------
static void scr_capture(void)
{
	char		path[48];
	ulong		*fb[GUI_NUM_LAYERS];
	uchar		en[GUI_NUM_LAYERS], ca[GUI_NUM_LAYERS];
	ulong		t0 = xTaskGetTickCount();
	FILINFO		fno;
	UINT		bw;
	int			i, x, y;
	uchar		err = 0;

	// Next free name - the index is only remembered until reset
	f_mkdir(SCREENSHOT_DIR);								// ok if it exists
	for(; scr_index <= SCR_MAX_INDEX; scr_index++)
	{
		sprintf(path, "%s/scr_%04d.bmp", SCREENSHOT_DIR, scr_index);
		if(f_stat(path, &fno) == FR_NO_FILE)
			break;
	}

	if(scr_index > SCR_MAX_INDEX)
	{
		printf("screenshot: no free file name\r\n");
		return;
	}

	if(f_open(&scr_fil, path, FA_WRITE | FA_CREATE_ALWAYS) != FR_OK)
	{
		printf("screenshot: can't create %s (no SD card?)\r\n", path);
		return;
	}

	for(i = 0; i < GUI_NUM_LAYERS; i++)
	{
		fb[i] = (ulong *)lcd_get_front_buffer(i, &en[i], &ca[i]);

		// The CPU must see what DMA2D and the LTDC see
		if(fb[i] != NULL)
			SCB_CleanInvalidateDCache_by_Addr(fb[i], LAYER_MEM_REQUIRED);
		else
			en[i] = 0;
	}

	err = scr_write_header();

	// Bottom-up rows
	for(y = SCR_H - 1; (y >= 0) && (!err); y--)
	{
		uchar *p = scr_line;

		for(x = 0; x < SCR_W; x++)
		{
			ulong	idx = scr_pixel_addr(x, y);
			ulong	r = 0, g = 0, b = 0;

			// Same as the LTDC blender with PAxCA factors, over the black
			// background colour: c = a * c_layer + (1 - a) * c_below
			for(i = 0; i < GUI_NUM_LAYERS; i++)
			{
				ulong px, a;

				if(!en[i])
					continue;

				px = fb[i][idx];
				a  = ((px >> 24) * ca[i]) / 255;

				r = (((px >> 16) & 0xFF) * a + r * (255 - a)) / 255;
				g = (((px >>  8) & 0xFF) * a + g * (255 - a)) / 255;
				b = (((px      ) & 0xFF) * a + b * (255 - a)) / 255;
			}

			*p++ = (uchar)b;
			*p++ = (uchar)g;
			*p++ = (uchar)r;
		}

		if((f_write(&scr_fil, scr_line, sizeof(scr_line), &bw) != FR_OK) || (bw != sizeof(scr_line)))
			err = 1;
	}

	f_close(&scr_fil);

	if(err)
	{
		printf("screenshot: write to %s failed\r\n", path);
		f_unlink(path);
		return;
	}

	printf("screenshot: %s saved in %d ms\r\n", path, (int)(xTaskGetTickCount() - t0));

	#ifdef DESKTOP_SHOW_SPECTRUM
	// After the capture, so it is not in the picture
	if(ui_s.cur_state == MODE_DESKTOP)
	{
		sprintf(path, "Saved screens/scr_%04d.bmp", scr_index);
		ui_controls_spectrum_show_notification(path);
	}
	#endif

	scr_index++;
}

//*----------------------------------------------------------------------------
//* Function Name       : ui_screenshot_poll
//* Object              : from the UI task loop, between two repaints
//* Context    			: CONTEXT_VIDEO
//*----------------------------------------------------------------------------
void ui_screenshot_poll(void)
{
	if(!scr_req)
		return;

	// Nobody else draws while the frame is read back
	GUI_Lock();
	scr_capture();
	GUI_Unlock();

	scr_req = 0;
}

#endif
