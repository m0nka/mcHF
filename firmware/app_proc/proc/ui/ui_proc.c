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

#include "ui_proc.h"

#include "radio_init.h"
#include "ui_actions.h"

#include "gui.h"
#include "dialog.h"

#include "sdram.h"
#include "shared_tim.h"

#include "cpu_trace.h"

#include "ui_tests.h"

// -----------------------------------------------------------------------------------------------
// Desktop Mode
#include "ui_proc_dm.h"

// -----------------------------------------------------------------------------------------------
// Side Encoder Options Menu
//#include "side_enc_menu\ui_side_enc_menu.h"
// -----------------------------------------------------------------------------------------------
// Quick Log Entry Menu
//#include "quick_log_entry\ui_quick_log.h"
// -----------------------------------------------------------------------------------------------
// FT8 Desktop
#include "desktop_ft8\ui_desktop_ft8.h"
// -----------------------------------------------------------------------------------------------
// Menu Mode
#include "menu\ui_menu_module.h"
#include "menu\ui_menu.h"

#include "ui_actions.h"

// Devices public states
#include "ui_lora_state.h"

static void ui_proc_change_mode(void);
static void ui_proc_init_desktop(void);
static void ui_proc_periodic(void);

// UI driver public state
struct	UI_DRIVER_STATE			ui_s;

// Touch data - emWin
GUI_PID_STATE 					TS_State;

// Public radio state
extern struct	TRANSCEIVER_STATE_UI	tsu;

// FreeRTOS process state
extern struct PROC_STATE 				ps;

// Menu items
extern K_ModuleItem_Typedef  	dsp_s;				// Standard DSP Menu
extern K_ModuleItem_Typedef  	menu_pa;			// Extended DSP Menu
extern K_ModuleItem_Typedef  	user_i;				// User Interface
extern K_ModuleItem_Typedef  	clock;				// Clock Settings
extern K_ModuleItem_Typedef  	logbook;			// Logbook
#ifdef CONTEXT_BMS
extern K_ModuleItem_Typedef  	menu_batt;			// Battery
#endif
extern K_ModuleItem_Typedef  	info;				// System Information
extern K_ModuleItem_Typedef  	lora;				// Lora module control
extern K_ModuleItem_Typedef  	file_b;				// File Browser
#ifdef CONTEXT_MARSCHAT
extern K_ModuleItem_Typedef  	marschat;			// MarsChat
#endif

//*----------------------------------------------------------------------------
//* Function Name       : ui_proc_add_menu_items
//* Object              :
//* Input Parameters    :
//* Output Parameters   :
//* Functions called    : CONTEXT_VIDEO
//*----------------------------------------------------------------------------
static void ui_proc_add_menu_items(void)
{
	k_ModuleInit();
	k_ModuleAdd(&dsp_s);				// Standard DSP Menu
	k_ModuleAdd(&menu_pa);				// Extended DSP Menu
	k_ModuleAdd(&user_i);				// User Interface
	k_ModuleAdd(&clock);				// Clock Settings
	#ifdef CONTEXT_BMS
	k_ModuleAdd(&menu_batt);			// Battery
	#endif
	k_ModuleAdd(&logbook);				// Logbook
	k_ModuleAdd(&file_b);				// File Browser
	k_ModuleAdd(&lora);					// Lora
	#ifdef CONTEXT_MARSCHAT
	k_ModuleAdd(&marschat);				// MarsChat
	#endif
	k_ModuleAdd(&info);					// About
}

static void ui_proc_cb(void)
{
	#ifdef DESKTOP_SHOW_FREQUENCY
	ui_controls_frequency_refresh(0);
	#endif

	#ifdef DESKTOP_SHOW_VOLUME
	//ui_controls_volume_refresh();	// blink on constant refresh
	#endif
}

static void ui_proc_cb_sm(void)
{
	#ifdef DESKTOP_SHOW_CLOCK
	//ui_controls_clock_panel_refresh();
	#endif
}

//*----------------------------------------------------------------------------
//* Function Name       : ui_proc_bkg_wnd
//* Object              :
//* Input Parameters    :
//* Output Parameters   :
//* Functions called    : CONTEXT_VIDEO
//*----------------------------------------------------------------------------
static void ui_proc_bkg_wnd(WM_MESSAGE * pMsg)
{
	// Need always to give back focus to background
	// windows, so we can receive keyboard events from
	// the IPC driver
	WM_SetFocus(WM_HBKWIN);

	switch (pMsg->MsgId)
	{
		case WM_TOUCH:
		{
			GUI_PID_STATE TS_State;
			int 		  touch_id;

			GUI_TOUCH_GetState(&TS_State);
			//printf("UI: x: %d, y: %d, state: %d\r\n", TS_State.x, TS_State.y, TS_State.Pressed);

			#if 0
			if((TS_State.x == 0) && (TS_State.y == 0) && (TS_State.Pressed == 0))
			{
				WM_DefaultProc(pMsg);
				break;
			}
			#endif

			#if 1
			// Is it top part of screen (above combined control ?)
			if(TS_State.y < (SW_FRAME_Y - 5))
			{
				#if 0
				printf("Top part of LCD touched.\r\n");

				// Is it the Menu ?
				touch_id = 1;//ui_controls_menu_button_is_touch(TS_State.x, TS_State.y);
				if(touch_id)
				{
					printf("== Menu touch ==\r\n");

					if(!ui_s.active_control_shown)
					{
						#if 1
						//if(*(uchar *)(EEP_BASE + EEP_KEYER_ON))
						//{
						//	ui_controls_keyer_quit();
						//}

						ui_s.req_state = MODE_MENU;
						ui_proc_change_mode();
						ui_s.active_control_shown = 1;
						#else
						// Change demo mode
						tsu.demo_mode = !tsu.demo_mode;
						// Wake up vfo task(use any notif id)
						if((hVfoTask != NULL)&&(tsu.demo_mode))
							xTaskNotify(hVfoTask, 44, eSetValueWithOverwrite);
						#endif
					}
				}
				#endif

				WM_DefaultProc(pMsg);
				break;
			}
			#endif

			touch_id = ui_controls_spectrum_is_touch(TS_State.x, TS_State.y);
			switch(touch_id)
			{
				// BMS
				//case 1:
				//{
					// ----------------------------------------
					//#ifdef CONTEXT_BMS
					// Temp show power dialog
					//if(!ui_s.active_control_shown)
					//{
					//	on_screen_power_init(WM_HBKWIN);
					//	ui_s.active_control_shown = 1;
					//}
					//#endif
					// ---------------------------------------

					//break;
				//}

				// AUDIO
				//case 2:
				//{
				//	if(!ui_s.active_control_shown)
				//	{
				//		on_screen_audio_init(WM_HBKWIN);
				//		ui_s.active_control_shown = 1;
				//	}
				//	break;
				//}

				// VFO
				//case 3:
				//{
				//	printf("Show VFO dialog\r\n");
				//	break;
				//}

				// KEYBOARD
				//case 4:
				//{
				//	if(!ui_s.active_control_shown)
				//	{
				//		on_screen_keyboard_init(WM_HBKWIN);
				//		ui_s.active_control_shown = 1;
				//	}
				//	break;
				//}

				// AGC/ATT
				//case 5:
				//{
				//	if(!ui_s.active_control_shown)
				//	{
				//		on_screen_agc_att_init(WM_HBKWIN);
				//		ui_s.active_control_shown = 1;
				//	}
				//	break;
				//}

				// CENTER/FIX
				//case 6:
				//	ui_actions_change_vfo_mode();
				//	break;

				// SPAN
				//case 7:
				//	ui_actions_change_span();
				//	break;

				// Band guide reaction
				case 8:
					//printf("band guide left\r\n");
					ui_actions_jump_to_band_part(0);
					break;

				case 9:
					//printf("band guide mid\r\n");
					ui_actions_jump_to_band_part(1);
					break;

				case 10:
					//printf("band guide right\r\n");
					ui_actions_jump_to_band_part(2);
					break;

				default:
					break;
			}

			WM_DefaultProc(pMsg);
			break;
		}

		case WM_PAINT:
			//ui_proc_periodic();
			break;

		// Process key messages not supported by ICON_VIEW control
		case WM_KEY:
		{
			//printf("GUI_KEY\r\n");

			switch (((WM_KEY_INFO*)(pMsg->Data.p))->Key)
			{
				#if 0
		        case GUI_KEY_ENTER:
		        {
		        	if(((WM_KEY_INFO*)(pMsg->Data.p))->PressedCnt == 0)
		        	{
		        		printf("GUI_KEY_ENTER press\r\n");
		        		break;
		        	}
		        	else
		        		printf("GUI_KEY_ENTER release\r\n");
		        	break;
		        }
				#endif

		        case 'K':
		        {
		        	//printf("K release\r\n");
		        	if(!ui_s.active_control_shown)
		        	{
		        		on_screen_keyboard_init(WM_HBKWIN);
		        		ui_s.active_control_shown = 1;
		        	}
		        	else
		        		on_screen_keyboard_quit();

		        	break;
		        }

		        case 'A':
		        {
		        	//printf("A release\r\n");
					if(!ui_s.active_control_shown)
					{
						on_screen_audio_init(WM_HBKWIN);
						ui_s.active_control_shown = 1;
					}
					else
						on_screen_audio_quit();
		        	break;
		        }

		        case 'G':
		        {
		        	//printf("G release\r\n");
					if(!ui_s.active_control_shown)
					{
						on_screen_agc_att_init(WM_HBKWIN);
						ui_s.active_control_shown = 1;
					}
					else
						on_screen_agc_att_quit();
		        	break;
		        }

		        case 'L':
		        {
		        	printf("L release\r\n");
					if(!ui_s.active_control_shown)
					{
						on_screen_quick_log_create(WM_HBKWIN);
						ui_s.active_control_shown = 1;
					}
					else
						on_screen_quick_log_destroy();
		        	break;
		        }

		        case '1':
		        	ui_actions_change_band(BAND_MODE_160, 0);
		        	break;
		        case '4':
		        	ui_actions_change_band(BAND_MODE_30, 0);
		        	break;
		        case '7':
		        	ui_actions_change_band(BAND_MODE_12, 0);
		        	break;
		        //case '.':
		        //	ui_actions_change_band(BAND_MODE_2200, 0);
		        //	break;
		        case '2':
		        	ui_actions_change_band(BAND_MODE_80, 0);
		        	break;
		        case '5':
		        	ui_actions_change_band(BAND_MODE_20, 0);
		        	break;
		        case '8':
		        	ui_actions_change_band(BAND_MODE_10, 0);
		        	break;
		        //case '0':
		        //	ui_actions_change_band(BAND_MODE_630, 0);
		        //	break;
		        case '3':
		        	ui_actions_change_band(BAND_MODE_60, 0);
		        	break;
		        case '6':
		        	ui_actions_change_band(BAND_MODE_17, 0);
		        	break;
		        //case '9':
		        //	ui_actions_change_band(BAND_MODE_GEN, 0);
		        //	break;
		        //case 'C':
		        //	ui_actions_change_span();
		        //	break;
		        case 'M':
		        	ui_actions_change_band(BAND_MODE_40, 0);
		        	break;
		        case 'S':
		        	ui_actions_change_band(BAND_MODE_15, 0);
		        	break;

		        case '+':
		        {
					#ifndef PCB_V9_REV_A
		        	ui_actions_change_step(1);
					#else
					(tsu.curr_band)++;
					if(tsu.curr_band > BAND_MODE_10)
						tsu.curr_band = BAND_MODE_10;
					ui_actions_change_band(tsu.curr_band, 0);
					#endif
		        	break;
		        }

		        case '-':
		        {
					#ifndef PCB_V9_REV_A
		        	ui_actions_change_step(0);
					#else
					if(tsu.curr_band > BAND_MODE_160)
						(tsu.curr_band)--;
					ui_actions_change_band(tsu.curr_band, 0);
					#endif
		        	break;
		        }

				case 'F':
				{
					(tsu.band[tsu.curr_band].filter)++;
					if(tsu.band[tsu.curr_band].filter > AUDIO_WIDE)
						tsu.band[tsu.curr_band].filter = AUDIO_300HZ;

					ui_actions_change_filter(tsu.band[tsu.curr_band].filter);
					break;
				}

				case 'W':
					ui_actions_change_power_level();
					break;

				case 'I':
					ui_actions_change_vfo_mode();
					break;

				case 'B':
					ui_actions_change_demod_mode(radio_init_default_mode_from_band());
					break;

				case 'C':
					ui_actions_change_demod_mode(DEMOD_CW);
					break;

				case 'Q':
					ui_actions_change_demod_mode(DEMOD_AM);
					break;

				case 'V':
					ui_actions_change_active_vfo();
					break;
			}
			break;
		}

		default:
			WM_DefaultProc(pMsg);
			break;
	}
}

//*--------------------------------------------------------------------------------------
//* Function Name       : bms_proc_wait_msg
//* Object              : Read pending messages
//* Input Parameters    : Rx Queue ptr and items buffer
//* Output Parameters   : none.
//*--------------------------------------------------------------------------------------
static uchar ui_proc_wait_msg(xQueueHandle pRxQueue, ulong *ulQueueBuffer)
{
	uchar ucNext = 0;

	if(pRxQueue == NULL)
		return 0;

	*ulQueueBuffer = 0;
	while(uxQueueMessagesWaiting(pRxQueue))
	{
		if(xQueueReceive(pRxQueue, (ulQueueBuffer + ucNext), (portTickType)0) == pdPASS)
		{
			ucNext++;
		}
	}

	return ucNext;
}

//*----------------------------------------------------------------------------
//* Function Name       : ui_proc_init_desktop
//* Object              :
//* Input Parameters    :
//* Output Parameters   :
//* Functions called    : CONTEXT_VIDEO
//*----------------------------------------------------------------------------
static void ui_proc_init_desktop(void)
{
	WM_SetCallback(WM_HBKWIN, ui_proc_bkg_wnd);
	//WINDOW_SetDefaultBkColor(GUI_TRANSPARENT);

	GUI_SetBkColor(GUI_BLACK);
	GUI_Clear();

	#ifdef UI_RUN_ALL_TESTS
	ui_tests_init();
	return;
	#endif

	#ifdef DESKTOP_SHOW_SDCARD
	ui_controls_sd_icon_init();
	#endif

	#ifdef DESKTOP_SHOW_VOLUME
	ui_controls_volume_init	  (WM_HBKWIN);
	#endif

	#ifdef DESKTOP_SHOW_CLOCK
	ui_controls_clock_panel_init();
	#endif

	#ifdef DESKTOP_SHOW_SPECTRUM
	ui_controls_spectrum_init (WM_HBKWIN);
	#endif

	#ifdef DESKTOP_SHOW_FREQUENCY
	ui_controls_frequency_init(WM_HBKWIN);
	#endif

	#ifdef DESKTOP_SHOW_SMETER
	ui_controls_smeter_init();
	#endif

	#ifdef DESKTOP_SHOW_FILTER
	ui_controls_filter_init();
	#endif

	#ifdef DESKTOP_SHOW_CPU_STAT
	ui_controls_cpu_stat_init();
	#endif

	//ui_controls_dsp_stat_init();

	#ifdef DESKTOP_SHOW_BATTERY
	ui_controls_battery_init();
	#endif

	#ifdef DESKTOP_SHOW_TX_STAT
	ui_controls_tx_stat_init();
	#endif

	//--ui_controls_menu_button_init();
}

//*----------------------------------------------------------------------------
//* Function Name       : ui_proc_change_mode
//* Object              : change screen mode
//* Input Parameters    :
//* Output Parameters   :
//* Functions called    : CONTEXT_VIDEO
//*----------------------------------------------------------------------------
static void ui_proc_change_mode(void)
{
	uchar state;

	// Take a snapshot of the state
	state = ui_s.req_state;

	// Do we need update ?
	if(ui_s.cur_state == state)
		return;

	// Don't enter Menu if we have virtual dialog shown
	if((ui_s.active_control_shown)&&(ui_s.req_state == MODE_MENU))
		return;

	// Backlight off
	//---HAL_GPIO_WritePin(LCD_BL_CTRL_GPIO_PORT, LCD_BL_CTRL_PIN, GPIO_PIN_RESET);
	shared_tim_change(20);

	switch(state)
	{
		// Switch to menu mode
		case MODE_MENU:
		{
			//printf("Entering Menu mode...\r\n");

			// Destroy desktop controls
			#ifdef DESKTOP_SHOW_VOLUME
			ui_controls_volume_quit();
			#endif

			#ifdef DESKTOP_SHOW_CLOCK
			ui_controls_clock_panel_quit();
			#endif

			#ifdef DESKTOP_SHOW_SPECTRUM
			ui_controls_spectrum_quit();
			#endif

			#ifdef DESKTOP_SHOW_FREQUENCY
			ui_controls_frequency_quit();
			#endif

			#ifdef DESKTOP_SHOW_SMETER
			ui_controls_smeter_quit();
			#endif

			//#ifdef DESKTOP_SHOW_SPECTRUM
			//ui_controls_spectrum_quit();
			//#endif

			WM_SetCallback		(WM_HBKWIN, 0);
			WM_InvalidateWindow	(WM_HBKWIN);

			// Clear screen
			GUI_SetBkColor(GUI_BLACK);
			GUI_Clear();

			// Set General Graphical properties
			ui_menu_set_gui_profile();

			// Show the main menu
			ui_menu_init();

			// Initial paint
			GUI_Exec();

			break;
		}

#if 0
		// Switch to side encoder options mode
		case MODE_SIDE_ENC_MENU:
		{
			printf("Entering Side encoder options mode...\r\n");

			// Destroy desktop controls
			ui_controls_smeter_quit();
			ui_controls_spectrum_quit();

			// Clear screen
			GUI_SetBkColor(GUI_BLACK);
			GUI_Clear();

			// Show popup
			ui_side_enc_menu_create();

			// Initial paint
			GUI_Exec();

			break;
		}
#endif
#if 1
		// Switch to FT8 mode
		case MODE_DESKTOP_FT8:
		{
			printf("Entering FT8 mode...\r\n");

			// Destroy desktop controls
			#ifdef DESKTOP_SHOW_VOLUME
			ui_controls_volume_quit();
			#endif

			#ifdef DESKTOP_SHOW_CLOCK
			ui_controls_clock_panel_quit();
			#endif

			#ifdef DESKTOP_SHOW_SPECTRUM
			ui_controls_spectrum_quit();
			#endif

			#ifdef DESKTOP_SHOW_FREQUENCY
			ui_controls_frequency_quit();
			#endif

			#ifdef DESKTOP_SHOW_SMETER
			ui_controls_smeter_quit();
			#endif

			//ui_controls_spectrum_quit();

			WM_SetCallback		(WM_HBKWIN, 0);
			WM_InvalidateWindow	(WM_HBKWIN);

			// Clear screen
			GUI_SetBkColor(GUI_BLACK);
			GUI_Clear();

			// Show popup
			ui_desktop_ft8_create();

			// Initial paint
			GUI_Exec();

			break;
		}
#endif
#if 0
		case MODE_QUICK_LOG:
		{
			printf("Entering Quick Log Entry mode...\r\n");

			// Destroy desktop controls
			ui_controls_smeter_quit();
			ui_controls_spectrum_quit();

			// Clear screen
			GUI_SetBkColor(GUI_BLACK);
			GUI_Clear();

			// Show popup
			ui_quick_log_create();

			// Initial paint
			GUI_Exec();

			break;
		}
#endif
		// Switch to desktop mode
		case MODE_DESKTOP:
		{
			//printf("Entering Desktop mode...\r\n");

			// Destroy any Window Manager items
			ui_menu_destroy();
			//ui_side_enc_menu_destroy();
			ui_desktop_ft8_destroy();
			//ui_quick_log_destroy();

			// Clear screen
			GUI_SetBkColor(GUI_BLACK);
			GUI_Clear();

			// Init controls
			ui_proc_init_desktop();

			break;
		}

		default:
			break;
	}

	// Update flag
	ui_s.cur_state = state;

	// Release lock
	ui_s.lock_requests = 0;

	// Backlight on
	//---HAL_GPIO_WritePin(LCD_BL_CTRL_GPIO_PORT, LCD_BL_CTRL_PIN, GPIO_PIN_SET);
	shared_tim_change(tsu.brightness);
}

//*----------------------------------------------------------------------------
//* Function Name       : ui_proc_emwin_init
//* Object              :
//* Input Parameters    :
//* Output Parameters   :
//* Functions called    : CONTEXT_VIDEO
//*----------------------------------------------------------------------------
static void ui_proc_emwin_init(void)
{
	int xSize, ySize;

	//printf("ui_driver_emwin_init...\r\n");

	// Initialise the SDRAM memory
	if(BSP_SDRAM_Init(0) != BSP_ERROR_NONE)
	{
		printf("Failed to initialize the SDRAM !!\n");
		return;
	}

	// UI init
	GUI_Init();
	GUI_X_InitOS();
	WM_MULTIBUF_Enable(1);

	// Set default layer
	GUI_SetLayerVisEx (1, 0);
	GUI_SelectLayer(0);

	// Specials
	GUI_EnableAlpha(1);
	GUI_SetTextMode(GUI_TM_TRANS);
	//GUI_SetDrawMode(LCD_DRAWMODE_NORMAL);
	GUI_SetDrawMode(LCD_DRAWMODE_TRANS);

	// Get display dimension
	xSize = LCD_GetXSize();
	ySize = LCD_GetYSize();

	// Limit desktop window to display size
	WM_SetSize(WM_HBKWIN, xSize, ySize);

	//printf("ui_driver_emwin_init...ok\r\n");
}

//*----------------------------------------------------------------------------
//* Function Name       : ui_proc_periodic
//* Object              :
//* Input Parameters    :
//* Output Parameters   :
//* Functions called    : CONTEXT_VIDEO
//*----------------------------------------------------------------------------
static void ui_proc_periodic(void)
{
	uchar sw_res;

	#ifdef PROFILE_UI_REPAINT
	ulong a1, a2;
	ulong b1, b2;
	static ulong disp_timer = 0;
	#endif

	#ifdef UI_RUN_ALL_TESTS
	ui_tests_run_all();
	return;
	#endif

	if(ui_s.cur_state != MODE_DESKTOP)
		return;

	// -----------------------------------
	// Spectrum control repaint
	#ifdef DESKTOP_SHOW_SPECTRUM
	//
	#ifdef PROFILE_UI_REPAINT
	a1 = ps.epoch;
	#endif
	//
	sw_res = ui_controls_spectrum_refresh(ui_proc_cb, 0);	// spectrum
	//
	#ifdef PROFILE_UI_REPAINT
	b1 = (ps.epoch - a1);
	a2 = ps.epoch;
	#endif
	//
	ui_controls_spectrum_refresh(ui_proc_cb, 1);	// waterfall
	//
	#ifdef PROFILE_UI_REPAINT
	b2 = (ps.epoch - a2);
	#endif
	//
	ui_controls_spectrum_refresh(ui_proc_cb, 2);	// clear update flag
	//
	// Statistics(random sampling for now)
	#ifdef PROFILE_UI_REPAINT
	if(disp_timer == 0)
		disp_timer = ps.epoch;
	else if(((disp_timer + 500) < ps.epoch)&&(!sw_res))
	{
		printf("sp: %dmS, wf: %dmS   \r\n", (int)b1, (int)b2);
		disp_timer = ps.epoch;
	}
	#endif
	#endif

	// Update the rest while waiting for data
	if(!sw_res)
		return;

	// -----------------------------------
	// Frequency control repaint
	#ifdef DESKTOP_SHOW_FREQUENCY
	ui_controls_frequency_refresh(0);
	#endif

	// Clock control repaint
	#ifdef DESKTOP_SHOW_CLOCK
	ui_controls_clock_panel_refresh();
	#endif

	// -----------------------------------
	// Volume control repaint
	#ifdef DESKTOP_SHOW_VOLUME
	//--ui_controls_volume_refresh();
	#endif

	// -----------------------------------
	// CPU stats control repaint
	#ifdef DESKTOP_SHOW_CPU_STAT
	ui_controls_cpu_stat_refresh();
	#endif

	//ui_controls_dsp_stat_refresh();

	// -----------------------------------
	// Battery control repaint
	#ifdef DESKTOP_SHOW_BATTERY
	ui_controls_battery_refresh();
	#endif

	// -----------------------------------
	// Filter control repaint
	#ifdef DESKTOP_SHOW_FILTER
	ui_controls_filter_refresh();
	#endif

	// -----------------------------------
	// TX stats control repaint
	#ifdef DESKTOP_SHOW_TX_STAT
	ui_controls_tx_stat_refresh();
	#endif

	// -----------------------------------
	// SD card control repaint
	#ifdef DESKTOP_SHOW_SDCARD
	ui_controls_sd_icon_refresh();
	#endif

	// -----------------------------------
	// S-meter control repaint
	#ifdef DESKTOP_SHOW_SMETER
	ui_controls_smeter_refresh  (ui_proc_cb_sm);
	#endif

	// -----------------------------------
	// BMS control repaint
	#ifdef CONTEXT_BMS
	on_screen_power_refresh();
	#endif
}

//*----------------------------------------------------------------------------
//* Function Name       : ui_proc_notified
//* Object              :
//* Input Parameters    :
//* Output Parameters   :
//* Functions called    : CONTEXT_VIDEO
//*----------------------------------------------------------------------------
static void ui_proc_notified(void const *arg)
{
	ulong 			ulNotificationValue = 0, ulNotif;
	xQueueHandle	*RxQueue;

	#ifdef UI_RUN_ALL_TESTS
	return;
	#endif

	// Get rx queue ptr
	RxQueue = (xQueueHandle *)arg;

	ulNotif = xTaskNotifyWait(0x00, ULONG_MAX, &ulNotificationValue, 0);	// No waiting, just read!
	if((ulNotif)&&(ulNotificationValue))
	{
		//printf("ui task notif, value: %02x\r\n", ulNotificationValue);
		switch(ulNotificationValue)
		{
			// Change mode
			case UI_NEW_MODE_EVENT:
				ui_proc_change_mode();
				break;

			case UI_NEW_FREQ_EVENT:
			{
				//printf("UI_NEW_FREQ_EVENT\r\n");

				#ifdef DESKTOP_SHOW_FREQUENCY
				ui_controls_frequency_refresh(0);
				#endif

				break;
			}

			case UI_NEW_AUDIO_EVENT:
			{
				//printf("UI_NEW_AUDIO_EVENT\r\n");
				#ifdef DESKTOP_SHOW_VOLUME
				ui_controls_volume_refresh();
				#endif
				break;
			}

			#ifdef DESKTOP_SHOW_SPECTRUM
			case UI_LORA_NOTIFICATION:
			{
				ulong ulRxData[10];

				// Only on main screen
				if(ui_s.cur_state != MODE_DESKTOP)
					break;

				// Get notification data
				if(ui_proc_wait_msg(*RxQueue, ulRxData) > 0)
				{
					// Notification router
					switch(ulRxData[0])
					{
						// Text notification - spectrum control
						case 0x55:
						{
							//printf("UI_LORA_NOTIFICATION - text\r\n");
							ui_controls_spectrum_show_notification((char *)ulRxData[1]);
							ui_controls_clock_show_notification(ulRxData[2]);
							break;
						}

						case 0x67:
						{
							//printf("UI_LORA_NOTIFICATION - data\r\n");
							ui_controls_clock_show_notification(ulRxData[2]);
							break;
						}
					}
				}
				break;
			}
			#endif

			default:
				break;
		}
	}
}

//*----------------------------------------------------------------------------
//* Function Name       : ui_proc_task
//* Object              :
//* Input Parameters    :
//* Output Parameters   :
//* Functions called    : CONTEXT_VIDEO
//*----------------------------------------------------------------------------
void ui_proc_task(void const *arg)
{
	uchar del_ms = UI_PROC_SLEEP_TIME;

	vTaskDelay(UI_PROC_START_DELAY);
	printf("start\r\n");

	// Backlight PWM
	shared_tim_init();
	shared_tim_change(tsu.brightness);

	ui_actions_init();
	ui_lora_state_init();

	// Default driver state
	ui_s.req_state 				= MODE_DESKTOP;
	ui_s.cur_state 				= MODE_DESKTOP;
	ui_s.show_band_guide 		= 0;
	ui_s.lock_requests			= 0;
	ui_s.theme_id				= THEME_0;
	ui_s.active_control_shown	= 0;

	// Init graphics lib
	ui_proc_emwin_init();

	// Set AGC, don't care what is the DSP state, just set it here
	//tsu.agc_state 	= READ_EEPROM(EEP_AGC_STATE);
	//tsu.rf_gain		= 50;
	//hw_dsp_eep_set_agc_mode(tsu.agc_state);

	// Add Menu items
	ui_proc_add_menu_items();

	// Prepare Desktop screen
	if(ui_s.cur_state == MODE_DESKTOP)
	{
		ui_proc_init_desktop();

		// Demo mode on after boot up
		if(tsu.demo_mode)
		{
			// Change demo mode
			//tsu.demo_mode = 1;

			// Wake up vfo task(use any notif id)
			if(ps.hVfoTask != NULL)
				xTaskNotify(ps.hVfoTask, 44, eSetValueWithOverwrite);
		}

	}

	// Prepare menu screen
	if(ui_s.cur_state == MODE_MENU)
	{
		ui_menu_set_gui_profile();
		ui_menu_init();

		GUI_Exec();
	}

ui_proc_loop:

	// Loop rate vs consumed FFT frame rate monitor(waterfall sluggishness
	// hunt) - repaint fps = min(fft feed rate, loop rate), so if the
	// waterfall is slow while fft irq shows 13/s, the loop is the brake
	#ifdef PROFILE_UI_REPAINT
	{
		extern struct UI_SW ui_sw;
		static ulong loop_cnt = 0, paint_cnt = 0, rate_t0 = 0;
		ulong now = xTaskGetTickCount();

		loop_cnt++;
		if(ui_sw.updated)
			paint_cnt++;		// this iteration will consume a frame

		if(rate_t0 == 0)
			rate_t0 = now;
		else if((now - rate_t0) >= 1000)
		{
			printf("ui loop %d/s, painted %d/s\r\n", (int)loop_cnt, (int)paint_cnt);
			loop_cnt = 0;
			paint_cnt = 0;
			rate_t0 = now;
		}
	}
	#endif

	// Process notifications
	ui_proc_notified(arg);

	// Adjust yield time
	if(ui_s.cur_state == MODE_MENU)
		del_ms = (UI_PROC_SLEEP_TIME*2);
	else if(ui_s.cur_state == MODE_DESKTOP_FT8)
		del_ms = (UI_PROC_SLEEP_TIME*3);
	else
	{
		ui_proc_periodic();
		del_ms = UI_PROC_SLEEP_TIME;
	}

	// Give control to emWin
	GUI_Exec();
	GUI_Delay(del_ms);

	// Periodic per task CPU load dump(100% CPU bug hunt)
	cpu_trace_poll();

	goto ui_proc_loop;
}

void ui_proc_clear_active(void)
{
	ui_s.active_control_shown = 0;
}

void ui_proc_power_cleanup(void)
{
	// Clear screen
	GUI_SetBkColor(GUI_BLACK);
	GUI_Clear();
	GUI_Exec();

	// Show text
	GUI_SetColor(GUI_WHITE);
	GUI_SetFont(&GUI_Font32B_1);
	#ifndef PCB_V9_REV_A
	GUI_DispStringAt("Good bye!", 350, 200);
	#else
	GUI_DispStringAt("Good bye!", 325, 215);
	#endif
	GUI_Exec();
}

#endif
