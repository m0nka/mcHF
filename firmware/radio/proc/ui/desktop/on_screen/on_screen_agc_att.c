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
#include "main.h"

#ifdef CONTEXT_VIDEO

#include "on_screen_agc_att.h"
#include "ui_actions.h"

#include "radio_init.h"
#include "codec_hw.h"

//#define KEYB_IS_TRANPARENT

#define ID_WINDOW_AUD	          	(GUI_ID_USER + 0x50)

#define ID_BUTTON_EXIT          	(GUI_ID_USER + 0x51)

#define ID_TEXT_LIST_2             	(GUI_ID_USER + 0x52)
#define ID_LISTBOX_2         		(GUI_ID_USER + 0x53)

#define GUI_ID_BTN1 			  	(GUI_ID_USER + 0x54)
#define GUI_ID_BTN2 			  	(GUI_ID_USER + 0x55)
#define GUI_ID_BTN3 			  	(GUI_ID_USER + 0x56)
#define GUI_ID_BTN4 			  	(GUI_ID_USER + 0x57)
#define GUI_ID_BTN5 			  	(GUI_ID_USER + 0x58)

#define AUD_X						254
#define AUD_Y						222

#define AUD_SIZE_X				 	595
#define AUD_SIZE_Y					250

#ifdef KEYB_IS_TRANPARENT
#define A_CF						WM_CF_SHOW|WM_CF_MEMDEV|WM_CF_HASTRANS
#else
#define A_CF						0x64
#endif

static const GUI_WIDGET_CREATE_INFO AgcDialog[] =
{
	// -----------------------------------------------------------------------------------------------------------------------------
	//							name			id					x		y		xsize		ysize			?		?		?
	// -----------------------------------------------------------------------------------------------------------------------------
	// Self
	{ WINDOW_CreateIndirect,	"", 			ID_WINDOW_AUD,		0,		0,		AUD_SIZE_X,	AUD_SIZE_Y, 	0, 		A_CF, 	0 },

	{ BUTTON_CreateIndirect,  	"Quit",			ID_BUTTON_EXIT,		440, 	200, 	140, 		40, 			0, 		0x0, 	0 },

	// First slider
	{ TEXT_CreateIndirect,     	"Audio Gain" ,	0,                	10,		10,  	100,  		20, 			TEXT_CF_LEFT 				},
	{ EDIT_CreateIndirect,     	NULL,     		GUI_ID_EDIT0,   	10,  	35,  	40,  		40, 			EDIT_CI_DISABELD,	3 	},
	{ SLIDER_CreateIndirect,   	NULL,     		GUI_ID_SLIDER0,		60, 	35, 	360,  		40 									},

	// Second slider
	{ TEXT_CreateIndirect,     	"RF Gain",		0,              	10,  	95,  	100,  		20, 			TEXT_CF_LEFT 				},
	{ EDIT_CreateIndirect,     	NULL,     		GUI_ID_EDIT1,   	10,  	120,  	40,  		40, 			EDIT_CI_DISABELD,	3 	},
	{ SLIDER_CreateIndirect,   	NULL,     		GUI_ID_SLIDER1,  	60, 	120, 	360,  		40 									},

	// List box
	{ TEXT_CreateIndirect, 		"AGC Mode",		ID_TEXT_LIST_2,		440,	10,		140, 		20,  			0, 		0x0,	0 	},
	{ LISTBOX_CreateIndirect, 	"", 			ID_LISTBOX_2, 		440, 	28, 	140, 		150, 			0, 		0x0, 	0 	},

	{ BUTTON_CreateIndirect, 	"ATT OFF",		GUI_ID_BTN1,		10, 	200, 	70, 		40,				0, 		0x0, 	0 },
	{ BUTTON_CreateIndirect, 	"ATT 4 dB",		GUI_ID_BTN2,		95, 	200, 	70, 		40,				0, 		0x0, 	0 },
	{ BUTTON_CreateIndirect, 	"ATT 8 dB",		GUI_ID_BTN3,		180, 	200, 	70, 		40,				0, 		0x0, 	0 },
	{ BUTTON_CreateIndirect, 	"ATT 16 dB",	GUI_ID_BTN4,		265, 	200, 	70, 		40,				0, 		0x0, 	0 },
	{ BUTTON_CreateIndirect, 	"ATT 32 dB",	GUI_ID_BTN5,		350, 	200, 	70, 		40,				0, 		0x0, 	0 },
};

// Public radio state
extern struct	TRANSCEIVER_STATE_UI	tsu;

WM_HWIN 	hAgcDialog = 0;

// Make sure the encoder is always mapped to Audio volume
//
static void on_screen_audio_default_focus(WM_MESSAGE * pMsg)
{
	WM_HWIN hItem = WM_GetDialogItem(pMsg->hWin, GUI_ID_SLIDER0);
	WM_SetFocus(hItem);
}

static void AU_cbControl(WM_MESSAGE * pMsg, int Id, int NCode)
{
	WM_HWIN hItem;
	WM_HWIN hSlider;
	WM_HWIN hEdit;

	ulong 	v = 0;
	int 	sel;
	char 	buf[50];

	switch(Id)
	{
		case ID_BUTTON_EXIT:
		{
			if(NCode == WM_NOTIFICATION_RELEASED)
				GUI_EndDialog(pMsg->hWin, 0);
			break;
		}

		// Audio gain slider
		case GUI_ID_SLIDER0:
		{
			if(NCode != WM_NOTIFICATION_VALUE_CHANGED)
				break;

			// Audio mute on, ignore events
			if(tsu.audio_mute_flag)
				break;

			hSlider = WM_GetDialogItem(pMsg->hWin, GUI_ID_SLIDER0);
			hEdit   = WM_GetDialogItem(pMsg->hWin, GUI_ID_EDIT0);

			// Get slider position
			v = SLIDER_GetValue(hSlider);

			// Update Edit box
			EDIT_SetValue(hEdit, v);

			ui_actions_change_audio_volume(v);
			break;
		}

		// RF gain slider
		case GUI_ID_SLIDER1:
		{
			if(NCode != WM_NOTIFICATION_VALUE_CHANGED)
				break;

			hSlider = WM_GetDialogItem(pMsg->hWin, GUI_ID_SLIDER1);
			hEdit   = WM_GetDialogItem(pMsg->hWin, GUI_ID_EDIT1);

			// Get slider position
			v = SLIDER_GetValue(hSlider);

			// Update Edit box
			EDIT_SetValue(hEdit, v);

			ui_actions_change_rf_gain(v);
			break;
		}

		case ID_LISTBOX_2:
		{
			if(NCode != WM_NOTIFICATION_SEL_CHANGED)
				break;

			hItem = WM_GetDialogItem(pMsg->hWin, ID_LISTBOX_2);
			sel = LISTBOX_GetSel(hItem);
			if(sel != LISTBOX_ALL_ITEMS)
			{
				LISTBOX_GetItemText(hItem,sel,buf,sizeof(buf));
				//--printf("list item=%s\r\n",buf);

				switch(buf[0])
				{
					case 'O':
					{
						// Limit RF gain to prevent loud hiss
						ui_actions_change_rf_gain(25);

						hItem = WM_GetDialogItem(pMsg->hWin, GUI_ID_SLIDER1);	// rf gain slider handle
						SLIDER_SetValue(hItem,20);								// reflect new RF gain value on screen
						WM_SetFocus(hItem);										// give focus to slider

						// AGC off
						ui_actions_change_agc_mode(AGC_OFF);
						goto finished;
					}
					case 'S':
						ui_actions_change_agc_mode(AGC_SLOW);
						break;
					case 'F':
						ui_actions_change_agc_mode(AGC_FAST);
						break;
					case 'C':
						ui_actions_change_agc_mode(AGC_CUSTOM);
						break;
					default:
						ui_actions_change_agc_mode(AGC_MED);
						break;
					}
			}
			else
				ui_actions_change_agc_mode(AGC_MED);

			on_screen_audio_default_focus(pMsg);
			break;
		}

		case GUI_ID_BTN1:
		{
			if(NCode == WM_NOTIFICATION_RELEASED)
				ui_actions_change_atten(ATTEN_0DB);

			on_screen_audio_default_focus(pMsg);
			break;
		}

		case GUI_ID_BTN2:
		{
			if(NCode == WM_NOTIFICATION_RELEASED)
				ui_actions_change_atten(ATTEN_4DB);

			on_screen_audio_default_focus(pMsg);
			break;
		}

		case GUI_ID_BTN3:
		{
			if(NCode == WM_NOTIFICATION_RELEASED)
				ui_actions_change_atten(ATTEN_8DB);

			on_screen_audio_default_focus(pMsg);
			break;
		}

		case GUI_ID_BTN4:
		{
			if(NCode == WM_NOTIFICATION_RELEASED)
				ui_actions_change_atten(ATTEN_16DB);

			on_screen_audio_default_focus(pMsg);
			break;
		}

		case GUI_ID_BTN5:
		{
			if(NCode == WM_NOTIFICATION_RELEASED)
				ui_actions_change_atten(ATTEN_32DB);

			on_screen_audio_default_focus(pMsg);
			break;
		}

		default:
			break;
	}

finished:
	WM_InvalidateWindow(WM_GetClientWindow(pMsg->hWin));
}

static void AgcHandler(WM_MESSAGE *pMsg)
{
	WM_HWIN hItem, hSlider, hEdit;
	WM_HWIN hDlg;
	int 	Id, NCode;
	//GUI_RECT	Rect;

	hDlg = pMsg->hWin;

	switch (pMsg->MsgId)
	{
		case WM_INIT_DIALOG:
		{
			#ifdef KEYB_IS_TRANPARENT
			WINDOW_SetBkColor(pMsg->hWin, GUI_INVALID_COLOR);
			#else
			WINDOW_SetBkColor(pMsg->hWin, GUI_LIGHTGRAY);
			#endif

    		for (int i = 0; i < 2; i++)
    		{
    			// Fix edit box
    			hEdit = WM_GetDialogItem(hDlg, GUI_ID_EDIT0 + i);
    			EDIT_SetFont(hEdit,&GUI_Font16_1);
    			EDIT_SetBkColor(hEdit,EDIT_CI_ENABLED,GUI_LIGHTBLUE);
    			EDIT_SetTextColor(hEdit,EDIT_CI_ENABLED,GUI_WHITE);
    			EDIT_SetTextAlign(hEdit,TEXT_CF_HCENTER|TEXT_CF_VCENTER);

    			hSlider = WM_GetDialogItem(hDlg, GUI_ID_SLIDER0 + i);
    			SLIDER_SetWidth(hSlider, 20);

    			if(i == 0)
    			{
    				SLIDER_SetRange(hSlider,0, 16);

    				if(tsu.band[tsu.curr_band].volume <= 16)
    				{
    					SLIDER_SetValue(hSlider, tsu.band[tsu.curr_band].volume);
    					EDIT_SetDecMode(hEdit,   tsu.band[tsu.curr_band].volume,   0, 16, 0, 0);
    				}
    				else
    				{
    					SLIDER_SetValue(hSlider, 16);
    					EDIT_SetDecMode(hEdit, 16,   0, 16, 0, 0);
    				}

    				WM_SetFocus(hSlider);
    			}
    			else
    			{
    				SLIDER_SetRange(hSlider,0, 50);
    				if(tsu.rf_gain <= 50)
    				{
    					SLIDER_SetValue(hSlider, tsu.rf_gain);
    					EDIT_SetDecMode(hEdit,   tsu.rf_gain,   0, 50, 0, 0);
    				}
    				else
    				{
    					SLIDER_SetValue(hSlider, 50);
    					EDIT_SetDecMode(hEdit, 0, 0, 50, 0, 0);
    				}
    			}
    		}


    		// Init Listbox
    		hItem = WM_GetDialogItem(pMsg->hWin, ID_TEXT_LIST_2);
    		TEXT_SetFont(hItem,&GUI_Font16_1);
    		TEXT_SetBkColor(hItem,GUI_LIGHTBLUE);
    		TEXT_SetTextColor(hItem,GUI_WHITE);
    		TEXT_SetTextAlign(hItem,TEXT_CF_HCENTER|TEXT_CF_VCENTER);
    		hItem = WM_GetDialogItem(pMsg->hWin, ID_LISTBOX_2);
    		LISTBOX_SetFont(hItem, &GUI_Font24B_1);
    		LISTBOX_SetTextColor(hItem,LISTBOX_CI_UNSEL,GUI_LIGHTBLUE);
    		LISTBOX_AddString(hItem, "SLOW");
    		LISTBOX_AddString(hItem, "MEDIUM");
    		LISTBOX_AddString(hItem, "FAST");
    		LISTBOX_AddString(hItem, "CUSTOM");
    		LISTBOX_AddString(hItem, "OFF");

    		// Selection on start (list box)
    		if(tsu.agc_mode < 5)
    		{
    			LISTBOX_SetSel(hItem,tsu.agc_mode);
    		}
    		else
    		{
    			LISTBOX_SetSel(hItem, 0);
    		}

			//hTimerWiFi = WM_CreateTimer(pMsg->hWin, 0, WIFI_TIMER_RESOLUTION, 0);
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

				WM_InvalidateWindow(hWiFiDialog);
			}
			#endif

			//TEXT_SetText(hItem, "test");
			//WM_InvalidateWindow(hWiFiDialog);

			//WM_RestartTimer(pMsg->Data.v, WIFI_TIMER_RESOLUTION);
			break;
		}

		case WM_PAINT:
			//WM_GetClientRect(&Rect);	// will create border when transparent
			//GUI_DrawRectEx(&Rect);
			break;


		case WM_DELETE:
		{
			//WM_DeleteTimer(hTimerWiFi);

			WM_HideWindow(hAgcDialog);

			// Clear screen
			//GUI_SetBkColor(GUI_BLACK);
			//GUI_Clear();

			// Init controls
			//ui_proc_init_desktop();
			ui_controls_spectrum_init(WM_HBKWIN);
			ui_proc_clear_active();

			hAgcDialog = 0;

			// Side encoder changes volume directly
			tsu.active_side_enc_id = 0;

			break;
		}

		case WM_NOTIFY_PARENT:
		{
			Id    = WM_GetId(pMsg->hWinSrc);    /* Id of widget */
			NCode = pMsg->Data.v;               /* Notification code */

			AU_cbControl(pMsg,Id,NCode);
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

uchar on_screen_agc_att_init(WM_HWIN hParent)
{
	if(hAgcDialog == 0)
	{
		BUTTON_SetDefaultSkin(BUTTON_SKIN_FLEX);
		CHECKBOX_SetDefaultSkin(CHECKBOX_SKIN_FLEX);

	    LISTVIEW_SetDefaultGridColor(GUI_WHITE);
	    LISTVIEW_SetDefaultBkColor(LISTVIEW_CI_SEL, GUI_LIGHTBLUE);
	    LISTVIEW_SetDefaultBkColor(LISTVIEW_CI_SELFOCUS, GUI_LIGHTBLUE);

	    HEADER_SetDefaultBkColor(GUI_LIGHTBLUE);
	    HEADER_SetDefaultTextColor(GUI_WHITE);
	    HEADER_SetDefaultFont(GUI_FONT_16_1);
	    //HEADER_SetDefaultSTSkin();

	    TEXT_SetDefaultTextColor(GUI_LIGHTBLUE);
	  	TEXT_SetDefaultFont(&GUI_Font20_1);

		// List box defaults
	  	// ToDo: Not avail in 6.10f lib file, check if this is a problem!
	  	//LISTBOX_SetDefaultScrollMode(LISTBOX_CF_AUTOSCROLLBAR_V);

	    SLIDER_SetDefaultBkColor   (GUI_LIGHTBLUE);
	    SLIDER_SetDefaultBarColor  (GUI_LIGHTBLUE);
	    SLIDER_SetDefaultFocusColor(GUI_LIGHTBLUE);
	    SLIDER_SetDefaultTickColor (GUI_LIGHTBLUE);

		// Side encoder changes key msg
		tsu.active_side_enc_id = 1;

		hAgcDialog = GUI_CreateDialogBox(AgcDialog, GUI_COUNTOF(AgcDialog), AgcHandler, hParent, AUD_X, AUD_Y);
		return 1;
	}
	//else
	//	WM_ShowWindow(hAgcDialog);

	return 0;
}

void on_screen_agc_att_quit(void)
{
	WM_HideWindow(hAgcDialog);
	GUI_EndDialog(hAgcDialog, 0);
	hAgcDialog = 0;
}

void on_screen_agc_att_refresh(void)
{
	if(hAgcDialog == 0)
		WM_InvalidateWindow(hAgcDialog);
}

#endif
