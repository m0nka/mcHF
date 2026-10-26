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
#include "icc_proc.h"

#ifdef CONTEXT_ROTARY

#include "rotary_proc.h"

//#include "api_driver.h"
#include "desktop\smeter\ui_controls_smeter.h"

#include "gui.h"
#include "dialog.h"

#include "stm32h7xx_hal.h"
#include "stm32h7xx_hal_gpio.h"
#include "stm32h7xx_hal_rcc.h"
#include "stm32h7xx_hal_rcc_ex.h"
#include "stm32h7xx_hal_tim.h"
#include "stm32h7xx_hal_tim_ex.h"
//
TIM_HandleTypeDef htim2;
TIM_HandleTypeDef htim3;

#define TIM_PERIOD	0xFFFF

// Timer counts on every edge of both channels(x4), the knob clicks every
// full quadrature cycle. If tuning feels half speed, this is 2
#define ROTARY_COUNTS_PER_DETENT	4

// Input capture filter, fDTS/32 N=8 - kills spikes on the long wires
// to the back PCB, contact bounce is handled by the detent accumulator
#define ROTARY_IC_FILTER			0xF

ushort audio_old = 0;
ushort freq_old = 0;

int audio_acc = 0;
int freq_acc = 0;

uchar front_init_done = 0;

// Public radio state
extern struct	TRANSCEIVER_STATE_UI	tsu;

// DSP core state
extern struct 	TransceiverState 		ts;

extern struct	UI_DRIVER_STATE			ui_s;

// FreeRTOS process state
extern struct PROC_STATE 				ps;

//*----------------------------------------------------------------------------
//* Function Name       : rotary_read_detents
//* Object              : signed counter delta since last call, collected into
//* Object              : whole detents, the remainder stays for the next call
//* Notes    			: a step only fires after a full detent of travel from
//* Notes    			: the last one, so the knob settling back into its
//* Notes    			: detent(one edge backwards) can not undo the last step
//* Context    			: CONTEXT_ROTARY
//*----------------------------------------------------------------------------
static int rotary_read_detents(TIM_HandleTypeDef *htim, ushort *old, int *acc)
{
	ushort 	cnt = __HAL_TIM_GET_COUNTER(htim);
	short	delta;
	int		steps;

	// 16 bit wrap
	delta = (short)(ushort)(cnt - *old);
	*old  = cnt;

	*acc += delta;

	// Truncates toward zero, remainder keeps the sign
	steps = *acc / ROTARY_COUNTS_PER_DETENT;
	*acc -= steps * ROTARY_COUNTS_PER_DETENT;

	return steps;
}

static void rotary_update_audio_publics(int pot_diff)
{
	uchar vol;

	// RX volume runs past the codec attenuator range into DSP gain, the CW
	// side tone is a codec PGA gain and stops where the hardware does
	uchar vol_max = (tsu.rxtx == 0) ? MAX_AUDIO_LEVEL : MAX_SIDETONE_LEVEL;

	if(tsu.curr_band > BAND_MODE_GEN)
	{
		printf("rotary_update_audio_publics, curr band: %d, CRITICAL ERROR!\r\n", tsu.curr_band);
		return;
	}

	if(tsu.rxtx == 0)
	{
		vol = tsu.band[tsu.curr_band].volume;				// load from public
		if(vol > vol_max)										// overflow check
		{
			tsu.band[tsu.curr_band].volume = vol_max;			// overwrite
			vol = tsu.band[tsu.curr_band].volume;				// refresh
			//save_band_info();									// Save band info to eeprom
		}
		//printf("rotary_update_audio_publics, curr band: %d, vol: %d\r\n", tsu.curr_band, vol);
	}
	else
	{
		vol = tsu.band[tsu.curr_band].st_volume;				// load from public
		if(vol > vol_max)										// overflow check
		{
			tsu.band[tsu.curr_band].st_volume = vol_max;			// overwrite
			vol = tsu.band[tsu.curr_band].st_volume;				// refresh
			//save_band_info();									// Save band info to eeprom
		}
		//printf("rotary_update_audio_publics, curr band: %d, vol: %d\r\n", tsu.curr_band, vol);
	}

	// Update public volume
	if(pot_diff < 0)
	{
		if(vol > 0)
			vol += pot_diff;
	}
	else
	{
		if(vol < vol_max)
			vol += pot_diff;
	}

	// Set request flag - now in volume control, so 'Mute' works better!
	//tsu.update_audio_dsp_req = 1;

	if(tsu.rxtx == 0)
	{
		//printf("audio volume = %d\r\n",tsu.band[tsu.curr_band].volume);
		tsu.band[tsu.curr_band].volume = vol;				// write back
		//save_band_info();									// Save band info to eeprom
	}
	else
	{
		//printf("audio volume = %d\r\n",tsu.band[tsu.curr_band].volume);
		tsu.band[tsu.curr_band].st_volume = vol;				// write back
		//save_band_info();									// Save band info to eeprom
	}

	if(ps.hAudioTask != NULL)
	{
		xTaskNotify(ps.hAudioTask, UI_NEW_AUDIO_EVENT, eSetValueWithOverwrite);
	}
}

static void rotary_update_side_enc_menu_publics(int pot_diff)
{
	if(pot_diff > 0)
	{
		GUI_StoreKeyMsg(GUI_KEY_RIGHT,1);
		GUI_StoreKeyMsg(GUI_KEY_RIGHT,0);
	}
	else
	{
		GUI_StoreKeyMsg(GUI_KEY_LEFT,1);
		GUI_StoreKeyMsg(GUI_KEY_LEFT,0);
	}
}

static void rotary_proc_check_side_enc(void)
{
	int		steps;
	int		pot_diff;

	steps = rotary_read_detents(&htim3, &audio_old, &audio_acc);
	if(steps == 0)
		return;

	pot_diff = (steps < 0) ? -1 : +1;

	//printf("steps = %d\r\n",steps);

	// Normally directly change volume
	// but in dialogs with slider we can send KEY msgs (left/right)
	// and move them with the encoder
	for(; steps != 0; steps -= pot_diff)
	{
		if(tsu.active_side_enc_id == 0)
			rotary_update_audio_publics(pot_diff);
		else
			rotary_update_side_enc_menu_publics(pot_diff);
	}
}

static void rotary_notify_freq(void)
{
	if(tsu.band[tsu.curr_band].fixed_mode == 0)
	{
		if(ps.hVfoTask != NULL)
			xTaskNotify(ps.hVfoTask, UI_NEW_FREQ_EVENT, eSetValueWithOverwrite);
	}
	else
	{
		if(ps.hIccTask != NULL)
			icc_proc_request(UI_ICC_NCO_FREQ);
	}
}

static void rotary_update_freq_publics(int pot_diff)
{
	// Block s-meter refresh
	//sm.rotary_timer = 0;
	//sm.rotary_block	= 1;

	// This is work in progress, no idea how to actually implement ;(
	//
	#if 0
	// If using frequency translate, in fixed mode
	if(		(tsu.use_full_span == 0) &&								// use freq translate
			(tsu.band[tsu.curr_band].fixed_mode == 0) &&			// is fixed mode
			(tsu.band[tsu.curr_band].nco_freq == 0) &&				// NCO base frequency hasn't been already set
			(hIccTask != NULL) &&									// ICC task is alive
			(tsu.dsp_alive > 1))									// DSP ready to take commands
	{
		// It is quarter of sampling rate as positive shift
		tsu.band[tsu.curr_band].nco_freq = 12000;//(short)ts.samp_rate/4;
		tsu.band[tsu.curr_band].vfo_a -= 12000;

		xTaskNotify(hIccTask, UI_ICC_NCO_FREQ, eSetValueWithOverwrite);
	}
	#endif

	// Update public volume
	if(pot_diff < 0)
	{
		if(tsu.band[tsu.curr_band].fixed_mode)
		{
			// ----------------------------------------
			// Limiter based on decoder mode
			if(tsu.band[tsu.curr_band].demod_mode == DEMOD_LSB)
			{
				if(tsu.band[tsu.curr_band].nco_freq < 20000)
					tsu.band[tsu.curr_band].nco_freq += tsu.band[tsu.curr_band].step;
			}
			if(tsu.band[tsu.curr_band].demod_mode == DEMOD_USB)
			{
				if(tsu.band[tsu.curr_band].nco_freq < 16000)
					tsu.band[tsu.curr_band].nco_freq += tsu.band[tsu.curr_band].step;
			}
			// Other modes handling...

			// Set request
			//tsu.update_nco_dsp_req = 1;

			//printf("nco freq: %d\r\n",tsu.nco_freq);
		}
		else
		{
			if(tsu.band[tsu.curr_band].active_vfo == VFO_A)
				tsu.band[tsu.curr_band].vfo_a += tsu.band[tsu.curr_band].step;
			else
				tsu.band[tsu.curr_band].vfo_b += tsu.band[tsu.curr_band].step;

			// Set request
			//tsu.update_freq_dsp_req  = 1;

			// ToDo: Range check...
		}
	}
	else
	{
		if(tsu.band[tsu.curr_band].fixed_mode)
		{
			// ----------------------------------------
			// Limiter based on decoder mode
			if(tsu.band[tsu.curr_band].demod_mode == DEMOD_LSB)
			{
				if(tsu.band[tsu.curr_band].nco_freq > -16000)
					tsu.band[tsu.curr_band].nco_freq -= tsu.band[tsu.curr_band].step;
			}
			if(tsu.band[tsu.curr_band].demod_mode == DEMOD_USB)
			{
				if(tsu.band[tsu.curr_band].nco_freq > -20000)
					tsu.band[tsu.curr_band].nco_freq -= tsu.band[tsu.curr_band].step;
			}
			// Other modes handling...

			// Set request
			//tsu.update_nco_dsp_req = 1;

			//printf("nco freq: %d\r\n",tsu.band[tsu.curr_band].nco_freq);
		}
		else
		{
			if(tsu.band[tsu.curr_band].active_vfo == VFO_A)
				tsu.band[tsu.curr_band].vfo_a -= tsu.band[tsu.curr_band].step;
			else
				tsu.band[tsu.curr_band].vfo_b -= tsu.band[tsu.curr_band].step;

			// Set request
			//tsu.update_freq_dsp_req  = 1;

			// ToDo: Range check...
		}
	}
	//printf("encoder freq = %d\r\n", tsu.band[tsu.curr_band].vfo_a);

	rotary_notify_freq();

	// Save band info to eeprom
	//save_band_info();
}

//*----------------------------------------------------------------------------
//* Function Name       : rotary_check_shake
//* Object              : quick back and forth of the VFO dial toggles the
//* Object              : step 1kHz <-> 500Hz(contest spacing). The dial
//* Object              : goes back to where it was before the shake, then
//* Object              : snaps to the new step grid(centre mode)
//* Notes    			: only from 1kHz or 500Hz, so a fast rock around a
//* Notes    			: station with a fine step does nothing
//* Context    			: CONTEXT_ROTARY
//*----------------------------------------------------------------------------
#define SHAKE_REVERSALS			4		// direction changes needed...
#define SHAKE_WINDOW_MS			800		// ...within this time
#define SHAKE_REST_MS			300		// dial idle this long = new movement starts
#define SHAKE_LOCKOUT_MS		1500	// no second toggle while still shaking

static uchar rotary_check_shake(int pot_diff)
{
	static int		last_dir 		= 0;
	static ulong	last_move 		= 0;
	static ulong	lockout 		= 0;
	static ulong	rev_time[SHAKE_REVERSALS];
	static uchar	rev_idx			= 0;
	static ulong	rest_vfo_a, rest_vfo_b;
	static short	rest_nco;

	struct BAND_INFO *b	= &tsu.band[tsu.curr_band];
	ulong		now 	= ps.epoch;
	ulong		step;
	ulong		*vfo;

	// Movement after a pause - remember where the dial was
	if((last_move == 0)||((now - last_move) > SHAKE_REST_MS))
	{
		rest_vfo_a 	= b->vfo_a;
		rest_vfo_b 	= b->vfo_b;
		rest_nco 	= b->nco_freq;
		last_dir 	= 0;
		memset(rev_time, 0, sizeof(rev_time));
	}
	last_move = now;

	if(pot_diff == last_dir)
		return 0;

	if(last_dir == 0)
	{
		last_dir = pot_diff;
		return 0;
	}
	last_dir = pot_diff;

	// Direction reversal, keep the last few
	rev_time[rev_idx] = now;
	rev_idx = (rev_idx + 1) % SHAKE_REVERSALS;

	// The oldest one(next to be overwritten) has to be inside the window
	if((rev_time[rev_idx] == 0)||((now - rev_time[rev_idx]) > SHAKE_WINDOW_MS))
		return 0;

	if(now < lockout)
		return 0;

	if(b->step == T_STEP_1KHZ)
		step = T_STEP_500HZ;
	else if(b->step == T_STEP_500HZ)
		step = T_STEP_1KHZ;
	else
		return 0;

	// Undo the shake
	b->vfo_a 	= rest_vfo_a;
	b->vfo_b 	= rest_vfo_b;
	b->nco_freq = rest_nco;

	// On the new grid, so 500Hz stations are reachable and back on 1kHz
	// the dial does not stay on the half
	if(b->fixed_mode == 0)
	{
		vfo = (b->active_vfo == VFO_A) ? &b->vfo_a : &b->vfo_b;
		*vfo = ((*vfo + (step / 2)) / step) * step;
	}

	b->step = step;

	printf("vfo step %dHz(shake)\r\n", (int)step);

	memset(rev_time, 0, sizeof(rev_time));
	lockout = now + SHAKE_LOCKOUT_MS;

	rotary_notify_freq();

	return 1;
}

static void rotary_proc_check_freq_enc(void)
{
	int		steps;
	int		pot_diff;

	if(front_init_done == 0)
		return;

	// No update on invalid local copy of the frequency
	if(tsu.band[tsu.curr_band].active_vfo == VFO_A)
	{
		if(tsu.band[tsu.curr_band].vfo_a == 0xFFFFFFFF)
			return;
	}
	else
	{
		if(tsu.band[tsu.curr_band].vfo_b == 0xFFFFFFFF)
			return;
	}

	// No update on invalid local copy of the vfo step
	if(tsu.band[tsu.curr_band].step == 0xFFFFFFFF)
		return;

	steps = rotary_read_detents(&htim2, &freq_old, &freq_acc);
	if(steps == 0)
		return;

	pot_diff = (steps < 0) ? -1 : +1;

	//printf("steps = %d\r\n",steps);

	// Do not update publics while in Menu(or other non desktop modes, where UI is not painted and processed)
	#ifdef CONTEXT_VIDEO
	if(ui_s.cur_state == MODE_DESKTOP)
	{
		for(; steps != 0; steps -= pot_diff)
		{
			// Shake toggles the step and puts the dial back, no tuning then
			if(rotary_check_shake(pot_diff))
				break;

			rotary_update_freq_publics(pot_diff);
		}
	}
	#endif
}

static uchar rotary_proc_freq_enc_init(void)
{
	TIM_Encoder_InitTypeDef 	tim_config;
	TIM_MasterConfigTypeDef 	tim_master;
	GPIO_InitTypeDef 			GPIO_InitStruct;

	__HAL_RCC_TIM2_CLK_ENABLE();

	// PA1, TIM2, Q channel
	GPIO_InitStruct.Pin 				= ENC1_I;
	GPIO_InitStruct.Mode 				= GPIO_MODE_AF_PP;
	GPIO_InitStruct.Pull	 			= GPIO_PULLUP;
	GPIO_InitStruct.Speed 				= GPIO_SPEED_FREQ_LOW;
	GPIO_InitStruct.Alternate 			= ENC1_I_AF;
	HAL_GPIO_Init(ENC1_I_PORT, &GPIO_InitStruct);

	// PA15, TIM2, I channel
	GPIO_InitStruct.Pin 				= ENC1_Q;
	GPIO_InitStruct.Alternate 			= ENC1_Q_AF;
	HAL_GPIO_Init(ENC1_Q_PORT, &GPIO_InitStruct);

	htim2.Instance 						= TIM2;
	htim2.Init.Prescaler 				= 0;
	htim2.Init.CounterMode 				= TIM_COUNTERMODE_UP;
	htim2.Init.Period 					= TIM_PERIOD;
	htim2.Init.ClockDivision 			= TIM_CLOCKDIVISION_DIV4;
	//htim3.Init.AutoReloadPreload 		= TIM_AUTORELOAD_PRELOAD_DISABLE;

	tim_config.EncoderMode 				= TIM_ENCODERMODE_TI12;
	tim_config.IC1Polarity 				= TIM_ICPOLARITY_FALLING;
	tim_config.IC1Selection 			= TIM_ICSELECTION_DIRECTTI;
	tim_config.IC1Prescaler 			= TIM_ICPSC_DIV1;
	tim_config.IC1Filter 				= ROTARY_IC_FILTER;
	tim_config.IC2Polarity 				= TIM_ICPOLARITY_FALLING;
	tim_config.IC2Selection 			= TIM_ICSELECTION_DIRECTTI;
	tim_config.IC2Prescaler 			= TIM_ICPSC_DIV1;
	tim_config.IC2Filter 				= ROTARY_IC_FILTER;

	if (HAL_TIM_Encoder_Init(&htim2, &tim_config) != HAL_OK)
		return 1;

	tim_master.MasterOutputTrigger 		= TIM_TRGO_RESET;
	tim_master.MasterSlaveMode 			= TIM_MASTERSLAVEMODE_DISABLE;

	if (HAL_TIMEx_MasterConfigSynchronization(&htim2, &tim_master) != HAL_OK)
		return 2;

	if(HAL_TIM_Encoder_Start(&htim2, TIM_CHANNEL_1 | TIM_CHANNEL_2) != HAL_OK)
		return 3;

	return 0;
}

static uchar rotary_proc_gain_enc_init(void)
{
	TIM_Encoder_InitTypeDef 	tim_config;
	TIM_MasterConfigTypeDef 	tim_master;
	GPIO_InitTypeDef 			GPIO_InitStruct;

	__HAL_RCC_TIM3_CLK_ENABLE();

	// PB4, TIM3, CH1 (I channel)
	GPIO_InitStruct.Pin 				= ENC2_I_PIN;
	GPIO_InitStruct.Mode 				= GPIO_MODE_AF_PP;
	GPIO_InitStruct.Pull	 			= GPIO_PULLUP;
	GPIO_InitStruct.Speed 				= GPIO_SPEED_FREQ_LOW;
	GPIO_InitStruct.Alternate 			= GPIO_AF2_TIM3;
	HAL_GPIO_Init(ENC2_I_PORT, &GPIO_InitStruct);

	// PC7, TIM3, CH2 (Q channel)
	GPIO_InitStruct.Pin 				= ENC2_Q_PIN;
	GPIO_InitStruct.Alternate 			= GPIO_AF2_TIM3;
	HAL_GPIO_Init(ENC2_Q_PORT, &GPIO_InitStruct);

	htim3.Instance 						= TIM3;
	htim3.Init.Prescaler 				= 0;
	htim3.Init.CounterMode 				= TIM_COUNTERMODE_UP;
	htim3.Init.Period 					= TIM_PERIOD;
	htim3.Init.ClockDivision 			= TIM_CLOCKDIVISION_DIV4;
	//htim3.Init.AutoReloadPreload 		= TIM_AUTORELOAD_PRELOAD_DISABLE;
	tim_config.EncoderMode 				= TIM_ENCODERMODE_TI12;
	tim_config.IC1Polarity 				= TIM_ICPOLARITY_FALLING;
	tim_config.IC1Selection 			= TIM_ICSELECTION_DIRECTTI;
	tim_config.IC1Prescaler 			= TIM_ICPSC_DIV1;
	tim_config.IC1Filter 				= ROTARY_IC_FILTER;
	tim_config.IC2Polarity 				= TIM_ICPOLARITY_FALLING;
	tim_config.IC2Selection 			= TIM_ICSELECTION_DIRECTTI;
	tim_config.IC2Prescaler 			= TIM_ICPSC_DIV1;
	tim_config.IC2Filter 				= ROTARY_IC_FILTER;

	if (HAL_TIM_Encoder_Init(&htim3, &tim_config) != HAL_OK)
		return 1;

	tim_master.MasterOutputTrigger 		= TIM_TRGO_RESET;
	tim_master.MasterSlaveMode 			= TIM_MASTERSLAVEMODE_DISABLE;

	if (HAL_TIMEx_MasterConfigSynchronization(&htim3, &tim_master) != HAL_OK)
		return 2;

	if(HAL_TIM_Encoder_Start(&htim3, TIM_CHANNEL_1 | TIM_CHANNEL_2) != HAL_OK)
		return 3;

	front_init_done = 1;
	return 0;
}

//*----------------------------------------------------------------------------
//* Function Name       :
//* Object              :
//* Notes    			:
//* Notes   			:
//* Notes    			:
//* Context    			: CONTEXT_ROTARY
//*----------------------------------------------------------------------------
static void rotary_worker(void)
{
	// Encoders
	rotary_proc_check_side_enc();
	rotary_proc_check_freq_enc();
}

// call from main() on startup
void rotary_proc_hw_init(void)
{
	// Encoders
	rotary_proc_freq_enc_init();
	rotary_proc_gain_enc_init();

	// Push buttons, redundant - side switch is power, freq button routed to ESP32!
	//rotary_init_side_encoder_switch_pin();
}

void rotary_proc_power_cleanup(void)
{
	// nothing
}

//*----------------------------------------------------------------------------
//* Function Name       :
//* Object              :
//* Notes    			:
//* Notes   			:
//* Notes    			:
//* Context    			: CONTEXT_ROTARY
//*----------------------------------------------------------------------------
void rotary_proc_task(void const *arg)
{
	vTaskDelay(ROTARY_PROC_START_DELAY);
	//printf("start\r\n");

rotary_driver_loop:
	rotary_worker();
	vTaskDelay(ROTARY_PROC_SLEEP_TIME);			// use IRQ and local notification ???
	goto rotary_driver_loop;
}

#endif
