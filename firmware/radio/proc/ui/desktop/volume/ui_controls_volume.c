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
#include "mchf_pro_board.h"
#include "main.h"

#ifdef CONTEXT_VIDEO

//#define VOL_USE_WM

#include "gui.h"
#include "dialog.h"

#include "ui_controls_volume.h"
#include "desktop\ui_controls_layout.h"

// Speaker icon in C file as binary resource
extern GUI_CONST_STORAGE GUI_BITMAP bmtechrubio;
extern GUI_CONST_STORAGE GUI_BITMAP bmtechrubio_mute;

extern TaskHandle_t hTouchTask;

// Public radio state
extern struct	TRANSCEIVER_STATE_UI	tsu;

// UI driver public state
extern struct	UI_DRIVER_STATE			ui_s;

#ifdef VOL_USE_WM
static const GUI_WIDGET_CREATE_INFO VolumeDialog[] =
{
	// -----------------------------------------------------------------------------------------------------------------------------
	//							name		id					x		y		xsize				ysize				?		?		?
	// -----------------------------------------------------------------------------------------------------------------------------
	// Self
	{ WINDOW_CreateIndirect,	"", 		ID_WINDOW_VOLUME,	0,		0,		(SPEAKER_SIZE_X+100),	SPEAKER_SIZE_Y, 	0, 		0x64, 	0 },
	// Mute Button
	{ BUTTON_CreateIndirect, 	"",			ID_BUTTON_VOLUMET, 	101,	1, 		(SPEAKER_SIZE_X-1), 	(SPEAKER_SIZE_Y-1), 0, 		0x0, 	0 },
	{ BUTTON_CreateIndirect, 	"-",		ID_BUTTON_VOLUMEM, 	1,		1, 		50,		 				(SPEAKER_SIZE_Y-2), 0, 		0x0, 	0 },
	{ BUTTON_CreateIndirect, 	"+",		ID_BUTTON_VOLUMEP, 	51,		1,		50, 					(SPEAKER_SIZE_Y-2), 0, 		0x0, 	0 },
};

WM_HWIN 	hVolumeDialog;
#endif

// control flags
//uchar loc_volume 		= 0;
uchar mute_flag	 		= 0;
uchar mute_saved_vol 	= 0;
uchar mute_debounce 	= 0;

#ifdef VOL_USE_WM
static void VDCHandler(WM_MESSAGE * pMsg, int Id, int NCode)
{
	WM_HWIN hItem;

	switch(Id)
	{
		// -------------------------------------------------------------
		// Button
		case ID_BUTTON_VOLUMET:
		{
			switch(NCode)
			{
				case WM_NOTIFICATION_CLICKED:
				{
					//printf("volume mute\r\n");

					hItem = WM_GetDialogItem(pMsg->hWin, ID_BUTTON_VOLUMET);

					if(mute_flag)
					{
						xTaskNotify(hTouchTask, 0x01, eSetValueWithOverwrite);
						BUTTON_SetBitmap(hItem, 0, &bmtechrubio_mute);
					}
					else
					{
						xTaskNotify(hTouchTask, 0x02, eSetValueWithOverwrite);
						BUTTON_SetBitmap(hItem, 0, &bmtechrubio);
					}

					mute_flag = !mute_flag;
					break;
				}
				case WM_NOTIFICATION_RELEASED:
					//printf("release\r\n");
					break;
			}
			break;
		}

		case ID_BUTTON_VOLUMEP:
		{
			switch(NCode)
			{
				case WM_NOTIFICATION_CLICKED:
				{
					//printf("volume up\r\n");

					if(tsu.band[tsu.curr_band].volume < 90)
						tsu.band[tsu.curr_band].volume += 10;

					xTaskNotify(hTouchTask, 0x03, eSetValueWithOverwrite);
					break;
				}
				default:
					break;
			}
			break;
		}

		case ID_BUTTON_VOLUMEM:
		{
			switch(NCode)
			{
				case WM_NOTIFICATION_CLICKED:
				{
					//printf("volume down\r\n");

					if(tsu.band[tsu.curr_band].volume > 10)
						tsu.band[tsu.curr_band].volume -= 10;

					xTaskNotify(hTouchTask, 0x03, eSetValueWithOverwrite);
					break;
				}
				default:
					break;
			}
			break;
		}

		// -------------------------------------------------------------
		default:
			break;
	}
}

static void VDHandler(WM_MESSAGE *pMsg)
{
	WM_HWIN 			hItem;
	int 				Id, NCode;

	switch (pMsg->MsgId)
	{
		case WM_INIT_DIALOG:
		{
			hItem = WM_GetDialogItem(pMsg->hWin, ID_BUTTON_VOLUMET);
			BUTTON_SetBitmap(hItem, 0, &bmtechrubio);
			break;
		}

		case WM_PAINT:
			break;

		case WM_DELETE:
			break;

		case WM_NOTIFY_PARENT:
		{
			Id    = WM_GetId(pMsg->hWinSrc);    // Id of widget
			NCode = pMsg->Data.v;               // Notification code

			VDCHandler(pMsg,Id,NCode);
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
#endif

//*----------------------------------------------------------------------------
//* Function Name       :
//* Object              :
//* Notes    			:
//* Notes   			:
//* Notes    			:
//* Context    			: CONTEXT_VIDEO
//*----------------------------------------------------------------------------
void ui_controls_volume_init(WM_HWIN hParent)
{
	#ifdef VOL_USE_WM
	hVolumeDialog = GUI_CreateDialogBox(VolumeDialog, GUI_COUNTOF(VolumeDialog), VDHandler, hParent, (SPEAKER_X - 100), SPEAKER_Y);
	#else

	char  buff[20];
	uchar volume;

	if(!mute_flag)
		GUI_DrawBitmap(&bmtechrubio, (SPEAKER_X + 1), (SPEAKER_Y + 1));
	else
		GUI_DrawBitmap(&bmtechrubio_mute, (SPEAKER_X + 1), (SPEAKER_Y + 1));

	// Frame
	GUI_SetColor(GUI_ORANGE);
	GUI_DrawRect((SPEAKER_X + 0),(SPEAKER_Y + 0),(SPEAKER_X + 61),(SPEAKER_Y + 49));
	GUI_DrawRect((SPEAKER_X + 1),(SPEAKER_Y + 1),(SPEAKER_X + 60),(SPEAKER_Y + 50));
	GUI_DrawRect((SPEAKER_X + 2),(SPEAKER_Y + 2),(SPEAKER_X + 59),(SPEAKER_Y + 51));

	// Extra filler on top
	GUI_FillRect((SPEAKER_X + 0),(SPEAKER_Y - 5), (SPEAKER_X + 61), (SPEAKER_Y + 0));

	if(tsu.curr_band > BAND_MODE_GEN)
	{
		printf("ui_controls_volume_init, curr band: %d, CRITICAL ERROR!\r\n", tsu.curr_band);
	}

	if((tsu.band[tsu.curr_band].demod_mode == DEMOD_CW)&&(tsu.rxtx))
		volume = tsu.band[tsu.curr_band].st_volume;						// Side tone volume (TX + CW)
	else
		volume = tsu.band[tsu.curr_band].volume;						// Audio volume (RX)

	if(volume > MAX_AUDIO_LEVEL)
	{
		printf("ui_controls_volume_init, volume: %d, CRITICAL ERROR!\r\n", volume);
	}

	if(!mute_flag)
		sprintf(buff,"%2d",volume);
	else
		sprintf(buff,"%2d",mute_saved_vol);

	#if 0
	// Clear area
	GUI_SetColor(GUI_WHITE);
	GUI_FillRect((SPEAKER_X + 2),(SPEAKER_Y + 37),(SPEAKER_X + 18),(SPEAKER_Y + 46));

	GUI_SetColor(GUI_BLACK);
	GUI_SetFont(&GUI_Font8x8_ASCII);
	GUI_DispStringAt(buff,(SPEAKER_X + 2),(SPEAKER_Y + 38));
	#endif

	GUI_SetColor(GUI_BLUE);
	GUI_SetFont(&GUI_Font8x8_ASCII);

	GUI_RECT 	Rect;
	Rect.x0 = (SPEAKER_X + 4);
	Rect.y0 = (SPEAKER_Y + 37);
	Rect.x1 = (SPEAKER_X + 16);
	Rect.y1 = (SPEAKER_Y + 46);

	// Set a clip rectangle to save performance, otherwise the whole background would be redrawn
	GUI_SetClipRect(&Rect);
	GUI_DispStringInRect(buff, &Rect, GUI_TA_HCENTER | GUI_TA_VCENTER);
	GUI_SetClipRect(NULL);
	#endif
}

//*----------------------------------------------------------------------------
//* Function Name       :
//* Object              :
//* Notes    			:
//* Notes   			:
//* Notes    			:
//* Context    			: CONTEXT_VIDEO
//*----------------------------------------------------------------------------
void ui_controls_volume_quit(void)
{
	#ifdef VOL_USE_WM
	GUI_EndDialog(hVolumeDialog, 0);
	#endif
}

#if 0
//*----------------------------------------------------------------------------
//* Function Name       :
//* Object              :
//* Notes    			:
//* Notes   			:
//* Notes    			:
//* Context    			: CONTEXT_VIDEO
//*----------------------------------------------------------------------------
void ui_controls_volume_touch(void)
{
	//printf("volume touch\r\n");

	if(mute_debounce)
	{
		//printf("timer on\r\n");

		mute_debounce--;
		if(mute_debounce)
			return;
	}

	//printf("volume mute process...\r\n");

	// Flip flag
	mute_flag = !mute_flag;

	if(mute_flag)
	{
		// Save volume
		mute_saved_vol = tsu.band[tsu.curr_band].volume;

		// Mute
		tsu.band[tsu.curr_band].volume = 0;

		// Prevent local refresh
		loc_volume = 0;
	}
	else
	{
		// Restore volume
		tsu.band[tsu.curr_band].volume = mute_saved_vol;

		// Prevent local refresh
		loc_volume = mute_saved_vol;
	}

	// Pass request
	tsu.update_audio_dsp_req = 1;

	// Redraw
	ui_controls_volume_init();

	mute_debounce = 3;

	tsu.audio_mute_flag = mute_flag;
}
#endif

//*----------------------------------------------------------------------------
//* Function Name       :
//* Object              :
//* Notes    			:
//* Notes   			:
//* Notes    			:
//* Context    			: CONTEXT_VIDEO
//*----------------------------------------------------------------------------
void ui_controls_volume_refresh(void)
{
	#ifndef VOL_USE_WM
	uchar volume;
	char  buff[20];

	// Leave volume digit on screen while mute
	if(mute_flag)
		return;

	if(ui_s.cur_state != MODE_DESKTOP)
		return;

	if(tsu.curr_band > BAND_MODE_GEN)
	{
		printf("ui_controls_volume_refresh, curr band: %d, CRITICAL ERROR!\r\n", tsu.curr_band);
		return;
	}

	if((tsu.band[tsu.curr_band].demod_mode == DEMOD_CW)&&(tsu.rxtx))
		volume = tsu.band[tsu.curr_band].st_volume;						// Side tone volume (TX + CW)
	else
		volume = tsu.band[tsu.curr_band].volume;						// Audio volume (RX)

	if(volume > MAX_AUDIO_LEVEL)
	{
		printf("ui_controls_volume_refresh, volume: %d, CRITICAL ERROR!\r\n", volume);
		return;
	}

	// Clear area
	GUI_SetColor(GUI_WHITE);
	GUI_FillRect((SPEAKER_X + 4),(SPEAKER_Y + 37),(SPEAKER_X + 16),(SPEAKER_Y + 46));

	sprintf(buff,"%2d",volume);

	GUI_SetColor(GUI_BLUE);
	GUI_SetFont(&GUI_Font8x8_ASCII);

	#if 0
	GUI_DispStringAt(buff,(SPEAKER_X + 4),(SPEAKER_Y + 38));
	#else
	GUI_RECT 	Rect;
	Rect.x0 = (SPEAKER_X + 4);
	Rect.y0 = (SPEAKER_Y + 37);
	Rect.x1 = (SPEAKER_X + 16);
	Rect.y1 = (SPEAKER_Y + 46);
	GUI_SetClipRect(&Rect);
	GUI_DispStringInRect(buff, &Rect, GUI_TA_HCENTER | GUI_TA_VCENTER);
	GUI_SetClipRect(NULL);
	#endif

	#endif
}

#endif
