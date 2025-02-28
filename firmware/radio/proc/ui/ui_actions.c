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

#include "spectrum\ui_controls_spectrum.h"
#include "freq\ui_controls_frequency.h"
#include "keyer\ui_controls_keyer.h"
#include "tx_status\ui_controls_tx_stat.h"

#include "ui_actions.h"

// Public radio state
extern struct	TRANSCEIVER_STATE_UI	tsu;

// UI driver public state
extern struct	UI_DRIVER_STATE			ui_s;

extern TaskHandle_t 					hIccTask;
extern TaskHandle_t 					hBandTask;
extern TaskHandle_t 					hVfoTask;
extern TaskHandle_t 					hAudioTask;

extern 	osMessageQId 			hEspMessage;
struct 	ESPMessage				esp_msg_a;

void ui_actions_init(void)
{
	esp_msg_a.ucProcStatus = TASK_PROC_IDLE;
}

//*----------------------------------------------------------------------------
//* Function Name       : ui_proc_ipc_msg
//* Object              :
//* Input Parameters    :
//* Output Parameters   :
//* Functions called    :
//* Context    			: CONTEXT_VIDEO
//*----------------------------------------------------------------------------
uchar ui_actions_ipc_msg(uchar send, uchar msg, uchar *in_buff)
{
	if(send == 0)
		goto msg_read;

	// Driver busy with previous request ?
	if(esp_msg_a.ucProcStatus != TASK_PROC_IDLE)
		return 1;

	// -----------------------------------------------------------
	// -- Send msg
	esp_msg_a.ucMessageID  = msg;
	esp_msg_a.ucProcStatus = TASK_PROC_WORK;

	//strcpy((char *)(esp_msg_a.ucData + 1), "theme");
	esp_msg_a.ucData[0] 		= 1;
	esp_msg_a.usPayloadSize 	= 1;

	if(osMessagePut(hEspMessage, (ulong)&esp_msg_a, osWaitForever) != osOK)
		return 2;

	return 0;

	// -----------------------------------------------------------
	// -- Read status
msg_read:

	// Are we checking last msg sent ?
	if(esp_msg_a.ucMessageID != msg)
		return 3;

	//while(esp_msg_a.ucProcStatus != TASK_PROC_DONE)
	//{
	//	vTaskDelay(10);
	//}

	if(esp_msg_a.ucProcStatus != TASK_PROC_DONE)
		return 4;

	//printf("ui_driver_ipc_msg resp: %02x\r\n", esp_msg_a.ucData[0]);
	//print_hex_array(esp_msg_a.ucData + 10, esp_msg_a.ucDataReady - 11);		// remove header in driver ???

	//if(esp_msg_a.ucData[10] == 0)
	//	ui_s.theme_id = esp_msg_a.ucData[14];

	if(in_buff != NULL)
		memcpy(in_buff, esp_msg_a.ucData, esp_msg_a.ucDataReady);

	// Reset for next call
	esp_msg_a.ucProcStatus = TASK_PROC_IDLE;

	return 0;
}

//*----------------------------------------------------------------------------
//* Function Name       : ui_actions_change_demod_mode
//* Object              :
//* Input Parameters    :
//* Output Parameters   :
//* Functions called    : CONTEXT_VIDEO
//*----------------------------------------------------------------------------
void ui_actions_change_demod_mode(uchar mode)
{
	if(hIccTask == NULL)
	{
		printf("not all drivers running, can't change band\r\n");
		return;
	}

	//printf("ui_actions_change_demod_mode to %d\r\n", mode);
	//radio_init_show_current_demod_mode(mode);

	// Sub-modes toggle
	if(tsu.band[tsu.curr_band].demod_mode == mode)
	{
		switch(mode)
		{
			// Change keyer type
			case DEMOD_CW:
			{
				if(tsu.keyer_mode < (CW_MAX_MODE - 1))
					tsu.keyer_mode++;
				else
					tsu.keyer_mode = CW_MODE_IAM_B;

				break;
			}

			// Change to opposite
			case DEMOD_USB:
			{
				mode = DEMOD_LSB;
				break;
			}

			// Change to opposite
			case DEMOD_LSB:
			{
				mode = DEMOD_USB;
				break;
			}

			// Change to opposite
			case DEMOD_AM:
			{
				mode = DEMOD_FM;
				break;
			}

			// Change to opposite
			case DEMOD_FM:
			{
				mode = DEMOD_AM;
				break;
			}

			default:
				break;
		}
	}

	// Update mode
	tsu.band[tsu.curr_band].demod_mode = mode;

	// Show/Hide iambic keyer
	if(mode == DEMOD_CW)
	{
		if((tsu.keyer_mode == CW_MODE_IAM_B)||(tsu.keyer_mode == CW_MODE_IAM_A))
			ui_controls_keyer_init(WM_HBKWIN);
		else
			ui_controls_keyer_quit();
	}
	else
		ui_controls_keyer_quit();

	// Notify ICC dispatcher
	xTaskNotify(hIccTask, UI_ICC_DEMOD_MODE, eSetValueWithOverwrite);

	// Repaint UI
	ui_controls_demod_refresh();
}

//*----------------------------------------------------------------------------
//* Function Name       : ui_actions_change_band
//* Object              :
//* Input Parameters    :
//* Output Parameters   :
//* Functions called    : CONTEXT_VIDEO
//*----------------------------------------------------------------------------
void ui_actions_change_band(uchar band, uchar skip_destop_upd)
{
	if((hBandTask == NULL)||(hVfoTask == NULL)||(hAudioTask == NULL)||(hIccTask == NULL))
	{
		printf("not all drivers running, can't change band\r\n");
		return;
	}

	tsu.curr_band = band;													// New band

	xTaskNotify(hBandTask,	1, 				 	eSetValueWithOverwrite);	// Update analogue filters
	xTaskNotify(hVfoTask, 	UI_NEW_FREQ_EVENT, 	eSetValueWithOverwrite);	// Update VFO
	xTaskNotify(hAudioTask, UI_NEW_AUDIO_EVENT, eSetValueWithOverwrite);	// Update volume

	// Change all DSP parameters relating to band change together
	xTaskNotify(hIccTask, 	UI_ICC_CHANGE_BAND, eSetValueWithOverwrite);	// Update DSP

	// Save band info to eeprom
	//save_band_info();

	if(skip_destop_upd)
		return;

	ui_s.show_band_guide = 1;	// Show band guide

	// Repaint UI
	ui_controls_band_refresh();
	ui_controls_demod_refresh();
	ui_controls_tx_stat_refresh();
}

//*----------------------------------------------------------------------------
//* Function Name       : ui_actions_change_vfo_mode
//* Object              :
//* Input Parameters    :
//* Output Parameters   :
//* Functions called    : CONTEXT_VIDEO
//*----------------------------------------------------------------------------
void ui_actions_change_vfo_mode(void)
{
	if(hIccTask == NULL)
	{
		printf("not all drivers running, can't change VFO mode\r\n");
		return;
	}

	if(tsu.band[tsu.curr_band].fixed_mode)
		tsu.band[tsu.curr_band].fixed_mode = 0;
	else
		tsu.band[tsu.curr_band].fixed_mode = 1;

	// Notify ICC dispatcher
	xTaskNotify(hIccTask, UI_ICC_NCO_FREQ, eSetValueWithOverwrite);	// Update NCO frequency(DSP)

	// Change '0' to center frequency in Fixed mode
	//--ui_controls_update_span();
}

//*----------------------------------------------------------------------------
//* Function Name       : ui_actions_change_span
//* Object              :
//* Input Parameters    :
//* Output Parameters   :
//* Functions called    : CONTEXT_VIDEO
//*----------------------------------------------------------------------------
void ui_actions_change_span(void)
{
	if(tsu.band[tsu.curr_band].span == 40000)
		tsu.band[tsu.curr_band].span /= 2;
	else if (tsu.band[tsu.curr_band].span == 20000)
		tsu.band[tsu.curr_band].span /= 2;
	else
		tsu.band[tsu.curr_band].span *= 4;

	ui_controls_update_span();
}

//*----------------------------------------------------------------------------
//* Function Name       : ui_actions_change_step
//* Object              :
//* Input Parameters    :
//* Output Parameters   :
//* Functions called    : CONTEXT_VIDEO
//*----------------------------------------------------------------------------
void ui_actions_change_step(void)
{
	if(tsu.band[tsu.curr_band].step < T_STEP_10MHZ)
		tsu.band[tsu.curr_band].step *= 10;
	else
		tsu.band[tsu.curr_band].step = T_STEP_1HZ;

	// Repaint UI
	ui_controls_vfo_step_refresh();
}

//*----------------------------------------------------------------------------
//* Function Name       : ui_actions_jump_to_band_part
//* Object              :
//* Input Parameters    :
//* Output Parameters   :
//* Functions called    : CONTEXT_VIDEO
//*----------------------------------------------------------------------------
void ui_actions_jump_to_band_part(uchar band_part_id)
{
	ulong vfo;
	short nco;

	if((hVfoTask == NULL)||(hIccTask == NULL))
	{
		printf("not all drivers running, can't jump to frequency\r\n");
		return;
	}

	switch(tsu.curr_band)
	{
		case BAND_MODE_160:
			return;

		case BAND_MODE_80:
		{
			if(band_part_id == 0)
			{
				vfo = 3520000;
				nco = 0;
			}
			else if (band_part_id == 1)
			{
				vfo = 3574000;
				nco = 0;
			}
			else
			{
				vfo = 3700000;
				nco = 0;
			}
			break;
		}

		case BAND_MODE_60:
			return;

		case BAND_MODE_40:
		{
			if(band_part_id == 0)
			{
				vfo = 7020000;
				nco = 0;
			}
			else if (band_part_id == 1)
			{
				vfo = 7074000;
				nco = 0;
			}
			else
			{
				vfo = 7150000;
				nco = 0;
			}
			break;
		}

		case BAND_MODE_30:
			return;

		case BAND_MODE_20:
		{
			if(band_part_id == 0)
			{
				vfo = 14020000;
				nco = 0;
			}
			else if (band_part_id == 1)
			{
				vfo = 14074000;
				nco = 0;
			}
			else
			{
				vfo = 14200000;
				nco = 0;
			}
			break;
		}

		case BAND_MODE_17:
			return;

		case BAND_MODE_15:
			return;

		case BAND_MODE_12:
			return;

		case BAND_MODE_10:
			return;

		default:
			return;
	}

	if(tsu.band[tsu.curr_band].fixed_mode)
	{
		// ToDo: implement
		#if 0
		// ----------------------------------------
		// Limiter based on decoder mode
		if(tsu.band[tsu.curr_band].demod_mode == DEMOD_LSB)
		{
			if(tsu.band[tsu.curr_band].nco_freq < 20000)
				tsu.band[tsu.curr_band].nco_freq = nco;
		}
		if(tsu.band[tsu.curr_band].demod_mode == DEMOD_USB)
		{
			if(tsu.band[tsu.curr_band].nco_freq < 16000)
				tsu.band[tsu.curr_band].nco_freq = nco;
		}

		xTaskNotify(hIccTask, UI_ICC_NCO_FREQ, eSetValueWithOverwrite);

		//printf("nco freq: %d\r\n",tsu.nco_freq);
		#endif
	}
	else
	{
		if(tsu.band[tsu.curr_band].active_vfo == 0)
			tsu.band[tsu.curr_band].vfo_a = vfo;
		else
			tsu.band[tsu.curr_band].vfo_b = vfo;

		// Notify VFO controller
		xTaskNotify(hVfoTask, UI_NEW_FREQ_EVENT, eSetValueWithOverwrite);
	}
}

//*----------------------------------------------------------------------------
//* Function Name       : ui_actions_toggle_atten
//* Object              :
//* Input Parameters    :
//* Output Parameters   :
//* Functions called    : CONTEXT_VIDEO
//*----------------------------------------------------------------------------
void ui_actions_toggle_atten(void)
{
	uchar old = tsu.band[tsu.curr_band].atten;

	if(tsu.band[tsu.curr_band].atten < (ATTEN_MAX - 1))
		tsu.band[tsu.curr_band].atten++;
	else
		tsu.band[tsu.curr_band].atten = ATTEN_0DB;

	//printf("updated=%d\r\n", tsu.band[tsu.curr_band].atten);

	// ToDo: Fix messaging between tasks!
	uchar s_r = ui_actions_ipc_msg(1, 7, NULL);
	vTaskDelay(100);
	uchar w_r = ui_actions_ipc_msg(0, 7, NULL);

	if((s_r == 0)&&(w_r == 0))
	{
		switch(tsu.band[tsu.curr_band].atten)
		{
		case 0:
			printf("ATT OFF\r\n");
			break;
		case 1:
			printf("ATT 6dB\r\n");
			break;
		case 2:
			printf("ATT 12dB\r\n");
			break;
		case 3:
			printf("ATT 18dB\r\n");
			break;
		}
		return;
	}

	// restore public
	tsu.band[tsu.curr_band].atten = old;
}

//*----------------------------------------------------------------------------
//* Function Name       : ui_actions_change_atten
//* Object              :
//* Input Parameters    :
//* Output Parameters   :
//* Functions called    : CONTEXT_VIDEO
//*----------------------------------------------------------------------------
void ui_actions_change_atten(uchar val)
{
	uchar old = tsu.band[tsu.curr_band].atten;

	//if(tsu.band[tsu.curr_band].atten < (ATTEN_MAX - 1))
	//	tsu.band[tsu.curr_band].atten++;
	//else
	//	tsu.band[tsu.curr_band].atten = ATTEN_0DB;

	tsu.band[tsu.curr_band].atten = val;
	//printf("updated=%d\r\n", tsu.band[tsu.curr_band].atten);

	// ToDo: Fix messaging between tasks!
	uchar s_r = ui_actions_ipc_msg(1, 7, NULL);
	vTaskDelay(100);
	uchar w_r = ui_actions_ipc_msg(0, 7, NULL);

	if((s_r == 0)&&(w_r == 0))
	{
		switch(tsu.band[tsu.curr_band].atten)
		{
		case 0:
			printf("ATT OFF\r\n");
			break;
		case 1:
			printf("ATT 6dB\r\n");
			break;
		case 2:
			printf("ATT 12dB\r\n");
			break;
		case 3:
			printf("ATT 18dB\r\n");
			break;
		}
		return;
	}

	// restore public
	tsu.band[tsu.curr_band].atten = old;
}

//*----------------------------------------------------------------------------
//* Function Name       : ui_actions_change_audio_balance
//* Object              :
//* Input Parameters    :
//* Output Parameters   :
//* Functions called    : CONTEXT_VIDEO
//*----------------------------------------------------------------------------
void ui_actions_change_audio_balance(uchar bal)
{
	if(hAudioTask == NULL)
		return;

	tsu.band[tsu.curr_band].audio_balance = bal;
  	//printf("val %d\r\n", tsu.band[tsu.curr_band].audio_balance);

	// Notify Codec controller
  	xTaskNotify(hAudioTask, UI_NEW_AUDIO_EVENT, eSetValueWithOverwrite);	// Wake process up
}

//*----------------------------------------------------------------------------
//* Function Name       : ui_actions_change_audio_volume
//* Object              :
//* Input Parameters    :
//* Output Parameters   :
//* Functions called    : CONTEXT_VIDEO
//*----------------------------------------------------------------------------
void ui_actions_change_audio_volume(uchar vol)
{
	if(hAudioTask == NULL)
		return;

	tsu.band[tsu.curr_band].volume = vol;
  	//printf("val %d\r\n", tsu.band[tsu.curr_band].volume);

	// Notify Codec controller
  	xTaskNotify(hAudioTask, UI_NEW_AUDIO_EVENT, eSetValueWithOverwrite);
}

//*----------------------------------------------------------------------------
//* Function Name       : ui_actions_change_stereo_mode
//* Object              :
//* Input Parameters    :
//* Output Parameters   :
//* Functions called    : CONTEXT_VIDEO
//*----------------------------------------------------------------------------
void ui_actions_change_stereo_mode(uchar mode)
{
	if(hIccTask == NULL)
		return;

  	//printf("stereo mode %d\r\n", mode);
	tsu.stereo_mode = mode;

	// Notify ICC dispatcher
	xTaskNotify(hIccTask, UI_ICC_STEREO, eSetValueWithOverwrite);
}

//*----------------------------------------------------------------------------
//* Function Name       : ui_actions_change_filter
//* Object              :
//* Input Parameters    :
//* Output Parameters   :
//* Functions called    : CONTEXT_VIDEO
//*----------------------------------------------------------------------------
void ui_actions_change_filter(uchar id)
{
	if(hIccTask == NULL)
		return;

  	//printf("filter id %d\r\n", id);
  	tsu.band[tsu.curr_band].filter = id;

  	// Notify ICC dispatcher
	xTaskNotify(hIccTask, UI_ICC_FITER, eSetValueWithOverwrite);
}

//*----------------------------------------------------------------------------
//* Function Name       : ui_actions_change_agc_mode
//* Object              :
//* Input Parameters    :
//* Output Parameters   :
//* Functions called    : CONTEXT_VIDEO
//*----------------------------------------------------------------------------
void ui_actions_change_agc_mode(uchar mode)
{
	if(hIccTask == NULL)
		return;

  	//printf("agc mode %d\r\n", mode);
	tsu.agc_mode = mode;

	// Notify ICC dispatcher
	xTaskNotify(hIccTask, UI_ICC_AGC_MODE, eSetValueWithOverwrite);

	// UI repaint
	ui_controls_agc_init();
}

//*----------------------------------------------------------------------------
//* Function Name       : ui_actions_change_rf_gain
//* Object              :
//* Input Parameters    :
//* Output Parameters   :
//* Functions called    : CONTEXT_VIDEO
//*----------------------------------------------------------------------------
void ui_actions_change_rf_gain(uchar gain)
{
	if(hIccTask == NULL)
		return;

  	//printf("rf gain %d\r\n", gain);
	tsu.rf_gain = gain;

	// Notify ICC dispatcher
	xTaskNotify(hIccTask, UI_ICC_AGC_MODE, eSetValueWithOverwrite);

	// UI repaint
	ui_controls_agc_init();
}

//*----------------------------------------------------------------------------
//* Function Name       : ui_actions_change_power_level
//* Object              :
//* Input Parameters    :
//* Output Parameters   :
//* Functions called    : CONTEXT_VIDEO
//*----------------------------------------------------------------------------
void ui_actions_change_power_level(void)
{
	(tsu.band[tsu.curr_band].tx_power)++;
	if(tsu.band[tsu.curr_band].tx_power >= PA_LEVEL_MAX_ENTRY)
		tsu.band[tsu.curr_band].tx_power = PA_LEVEL_0_5W;

	// Notify DSP ?
	// ...

	// UI repaint
	ui_controls_tx_stat_refresh();
}

//*----------------------------------------------------------------------------
//* Function Name       : ui_actions_change_dsp_core
//* Object              :
//* Input Parameters    :
//* Output Parameters   :
//* Functions called    : CONTEXT_VIDEO
//*----------------------------------------------------------------------------
void ui_actions_change_dsp_core(void)
{
	if(hIccTask == NULL)
		return;

	// Notify ICC dispatcher
	xTaskNotify(hIccTask, UI_ICC_CHANGE_CORE, eSetValueWithOverwrite);

	// UI repaint
	ui_controls_agc_init();
}

#endif
