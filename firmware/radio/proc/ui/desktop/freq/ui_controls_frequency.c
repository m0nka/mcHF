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

#include "gui.h"
#include "dialog.h"
#include "ui_cool_progress.h"

#include "desktop\ui_controls_layout.h"
#include "radio_init.h"

#include "ui_controls_frequency.h"

//#define FREQ_USE_WM

#ifdef FREQ_USE_WM
static const GUI_WIDGET_CREATE_INFO FreqDialog[] =
{
	// ---------------------------------------------------------------------------------------------------------------------------------------------
	//							name		id					x		y		xsize			ysize				?		?		?
	// ---------------------------------------------------------------------------------------------------------------------------------------------
	// Self
	{ WINDOW_CreateIndirect,	"", 		ID_WINDOW_FREQ,		0,		0,		2,	2, 		0, 		0x64, 	0 },
	//
	//{ TEXT_CreateIndirect, 		"----",		ID_TEXT_WIFI,		1,		1,		78, 			13,  				0, 		0x0,	0 },
};

WM_HWIN 						hFreqDialog;

#define	FREQ_TIMER_RESOLUTION	10
WM_HTIMER 						hTimerFreq;
#endif

// ------------------------------------------------
// Frequency public
__IO DialFrequency 				df;
// ------------------------------------------------

// Public radio state
extern struct	TRANSCEIVER_STATE_UI	tsu;

extern ulong tune_steps[];

ulong old_vfo = 0;
uchar loc_band;
ulong loc_step = 0;
uchar loc_demod_mode = 0;

//*----------------------------------------------------------------------------
//* Function Name       :
//* Object              :
//* Notes    			:
//* Notes   			:
//* Notes    			:
//* Context    			: CONTEXT_VIDEO
//*----------------------------------------------------------------------------
static void ui_controls_repaint_state(void)
{
	char  		buff[20];

	// Clear AGC gain part of control
	GUI_SetColor(GUI_WHITE);
	GUI_FillRoundedRect((AGC_X + 43),(AGC_Y + 2),(AGC_X + 100),(AGC_Y + 19), 2);

	GUI_SetColor(GUI_GRAY);
	GUI_SetFont(&GUI_Font20_1);

	// Display AGC state
	switch(tsu.agc_mode)
	{
		case AGC_SLOW:
			GUI_DispStringAt("SLOW",	(AGC_X + 45),(AGC_Y + 2));
			break;
		case AGC_MED:
			GUI_DispStringAt("MED",		(AGC_X + 54),(AGC_Y + 2));
			break;
		case AGC_FAST:
			GUI_DispStringAt("FAST",	(AGC_X + 50),(AGC_Y + 2));
			break;
		case AGC_CUSTOM:
			GUI_DispStringAt("CUST",	(AGC_X + 47),(AGC_Y + 2));
			break;
		case AGC_OFF:
			GUI_DispStringAt("OFF",		(AGC_X + 55),(AGC_Y + 2));
			break;
		default:
			break;
	}


	// Clear ATT part of control
	GUI_SetColor(GUI_BLACK);
	GUI_FillRoundedRect((AGC_X + 140), (AGC_Y + 2), (AGC_X + 185), (AGC_Y + 19), 2);

	GUI_SetColor(GUI_WHITE);
	GUI_DispStringAt("ATT", (AGC_X + 103), (AGC_Y + 2));

	// RF attenuator value
	switch(tsu.band[tsu.curr_band].atten)
	{
		case ATTEN_0DB:
			GUI_SetColor(GUI_GRAY);
			GUI_DispStringAt("OFF ", (AGC_X + 145), (AGC_Y + 2));
			break;
		case ATTEN_4DB:
			GUI_SetColor(GUI_WHITE);
			GUI_DispStringAt("4dB ", (AGC_X + 147), (AGC_Y + 2));
			break;
		case ATTEN_8DB:
			GUI_SetColor(GUI_WHITE);
			GUI_DispStringAt("8dB ", (AGC_X + 147), (AGC_Y + 2));
			break;
		case ATTEN_16DB:
			GUI_SetColor(GUI_RED);
			GUI_DispStringAt("16dB", (AGC_X + 142), (AGC_Y + 2));
			break;
		case ATTEN_32DB:
			GUI_SetColor(GUI_RED);
			GUI_DispStringAt("32dB", (AGC_X + 142), (AGC_Y + 2));
			break;
		default:
			break;
	}

	// Display RF gain, as cool progress bar
	sprintf(buff,"%2d",tsu.rf_gain);
	ui_cool_progress_gain(RF_GAIN_X, RF_GAIN_Y, tsu.rf_gain, buff);
}

//*----------------------------------------------------------------------------
//* Function Name       :
//* Object              :
//* Notes    			:
//* Notes   			:
//* Notes    			:
//* Context    			: CONTEXT_VIDEO
//*----------------------------------------------------------------------------
void ui_controls_agc_init(void)
{
	// Create control
	GUI_SetColor(GUI_GRAY);
	GUI_FillRoundedRect((AGC_X + 0), (AGC_Y + 0), (AGC_X + 190), (AGC_Y + 21), 2);
	GUI_SetFont(&GUI_Font20_1);
	GUI_SetColor(GUI_WHITE);
	GUI_DispStringAt("AGC",(AGC_X + 2),(AGC_Y + 2));

	// Initial paint
	ui_controls_repaint_state();
}

static void ui_controls_demod_change_screen_demod_mode(uchar on_init)
{
	uchar demod = tsu.band[tsu.curr_band].demod_mode;

	if(!on_init)
	{
		if((loc_demod_mode == demod)&&(demod != DEMOD_CW))
			return;
	}

	GUI_SetColor(GUI_BLUE);
	GUI_FillRoundedRect((DECODER_MODE_X + 2),(DECODER_MODE_Y + 2),(DECODER_MODE_X + DEC_MODE_X_SZ),(DECODER_MODE_Y + 20), 2);

	GUI_SetColor(GUI_WHITE);
	GUI_SetFont(&GUI_Font20_1);

	switch(demod)
	{
		case DEMOD_USB:
			GUI_DispStringAt("USB",(DECODER_MODE_X + 18),(DECODER_MODE_Y + 3));
			break;
		case DEMOD_LSB:
			GUI_DispStringAt("LSB",(DECODER_MODE_X + 20),(DECODER_MODE_Y + 3));
			break;
		case DEMOD_CW:
		{
			if(tsu.keyer_mode == CW_MODE_IAM_B)
				GUI_DispStringAt("CWb",(DECODER_MODE_X + 18),(DECODER_MODE_Y + 3));
			else if(tsu.keyer_mode == CW_MODE_IAM_A)
				GUI_DispStringAt("CWa",(DECODER_MODE_X + 18),(DECODER_MODE_Y + 3));
			else
				GUI_DispStringAt("CWs",(DECODER_MODE_X + 18),(DECODER_MODE_Y + 3));
			break;
		}
		case DEMOD_AM:
			GUI_DispStringAt("AM",(DECODER_MODE_X + 24),(DECODER_MODE_Y + 3));
			break;
		case DEMOD_FM:
			GUI_DispStringAt("FM",(DECODER_MODE_X + 24),(DECODER_MODE_Y + 3));
			break;
		//case DEMOD_DIGI:	- no point really, as we are going to repaint entirely different Desktop
		//	GUI_DispStringAt("FT8",(DECODER_MODE_X + 10),(DECODER_MODE_Y + 1));
		//	break;
		default:
			break;
	}

	loc_demod_mode = demod;
}

//*----------------------------------------------------------------------------
//* Function Name       : ui_controls_demod_init
//* Object              :
//* Notes    			:
//* Notes   			:
//* Notes    			:
//* Context    			: CONTEXT_VIDEO
//*----------------------------------------------------------------------------
void ui_controls_demod_init(void)
{
	GUI_SetColor(GUI_GRAY);
	GUI_DrawRoundedRect((DECODER_MODE_X + 0),(DECODER_MODE_Y + 0),(DECODER_MODE_X + DEC_MODE_X_SZ + 2),(DECODER_MODE_Y + 24),2);
	GUI_DrawRoundedRect((DECODER_MODE_X + 1),(DECODER_MODE_Y + 1),(DECODER_MODE_X + DEC_MODE_X_SZ + 1),(DECODER_MODE_Y + 23),2);
	GUI_SetColor(GUI_BLUE);
	GUI_FillRoundedRect((DECODER_MODE_X + 2),(DECODER_MODE_Y + 2),(DECODER_MODE_X + DEC_MODE_X_SZ),(DECODER_MODE_Y + 22),2);
	GUI_SetColor(GUI_WHITE);
	GUI_SetFont(&GUI_Font20_1);

	// Initial paint
	ui_controls_demod_change_screen_demod_mode(1);
}

//*----------------------------------------------------------------------------
//* Function Name       :
//* Object              :
//* Notes    			:
//* Notes   			:
//* Notes    			:
//* Context    			: CONTEXT_VIDEO
//*----------------------------------------------------------------------------
void ui_controls_demod_refresh(void)
{
	ui_controls_demod_change_screen_demod_mode(0);
}

//*----------------------------------------------------------------------------
//* Function Name       :
//* Object              :
//* Notes    			:
//* Notes   			:
//* Notes    			:
//* Context    			: CONTEXT_VIDEO
//*----------------------------------------------------------------------------
void ui_controls_rx_tx_init(uchar is_init)
{
	static uchar rx_tx_cntr_state = 0;

	if((rx_tx_cntr_state == tsu.rxtx)&&(!is_init))
		return;

	if(!tsu.rxtx)
		GUI_SetColor(GUI_LIGHTGREEN);
	else
		GUI_SetColor(GUI_RED);

	GUI_FillRoundedRect((RXTX_X + 0),(RXTX_Y + 0),(RXTX_X + 51),(RXTX_Y + 7),2);

	rx_tx_cntr_state = tsu.rxtx;
}

static void ui_controls_vfo_init_encoder_switch_pin(void)
{
#if 0
	GPIO_InitTypeDef  	GPIO_InitStruct;

	GPIO_InitStruct.Pin 		= GPIO_PIN_12;
	GPIO_InitStruct.Mode 		= GPIO_MODE_INPUT;
	GPIO_InitStruct.Pull 		= GPIO_PULLUP;
	GPIO_InitStruct.Speed 		= GPIO_SPEED_FREQ_LOW;
	HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
#endif
}

void ui_controls_vfo_step_update_step(void)
{
	ulong step = tsu.band[tsu.curr_band].step;

	// No need to update if nothing changed
	if(step == loc_step)
		return;

	//printf("control step %d\r\n",tsu.band[tsu.curr_band].step);

	GUI_SetFont(&GUI_Font8x16_1);
	GUI_SetColor(GUI_BLACK);

	switch(step)
	{
		case T_STEP_1HZ:
			GUI_FillRoundedRect((VFO_STEP_X + 1),(VFO_STEP_Y + 1),(VFO_STEP_X + VFO_STEP_SIZE_X - 1),(VFO_STEP_Y + VFO_STEP_SIZE_Y - 1),2);
			GUI_SetColor(GUI_ORANGE);
			GUI_DispStringAt("1Hz",(VFO_STEP_X + 16),(VFO_STEP_Y + 3));
			break;
		case T_STEP_10HZ:
			GUI_FillRoundedRect((VFO_STEP_X + 1),(VFO_STEP_Y + 1),(VFO_STEP_X + VFO_STEP_SIZE_X - 1),(VFO_STEP_Y + VFO_STEP_SIZE_Y - 1),2);
			GUI_SetColor(GUI_ORANGE);
			GUI_DispStringAt("10Hz",(VFO_STEP_X + 10),(VFO_STEP_Y + 3));
			break;
		case T_STEP_100HZ:
			GUI_FillRoundedRect((VFO_STEP_X + 1),(VFO_STEP_Y + 1),(VFO_STEP_X + VFO_STEP_SIZE_X - 1),(VFO_STEP_Y + VFO_STEP_SIZE_Y - 1),2);
			GUI_SetColor(GUI_ORANGE);
			GUI_DispStringAt("100Hz",(VFO_STEP_X + 8),(VFO_STEP_Y + 3));
			break;
		case T_STEP_1KHZ:
			GUI_FillRoundedRect((VFO_STEP_X + 1),(VFO_STEP_Y + 1),(VFO_STEP_X + VFO_STEP_SIZE_X - 1),(VFO_STEP_Y + VFO_STEP_SIZE_Y - 1),2);
			GUI_SetColor(GUI_ORANGE);
			GUI_DispStringAt("1kHz",(VFO_STEP_X + 12),(VFO_STEP_Y + 3));
			break;
		case T_STEP_10KHZ:
			GUI_FillRoundedRect((VFO_STEP_X + 1),(VFO_STEP_Y + 1),(VFO_STEP_X + VFO_STEP_SIZE_X - 1),(VFO_STEP_Y + VFO_STEP_SIZE_Y - 1),2);
			GUI_SetColor(GUI_ORANGE);
			GUI_DispStringAt("10kHz",(VFO_STEP_X + 8),(VFO_STEP_Y + 3));
			break;
		case T_STEP_100KHZ:
			GUI_FillRoundedRect((VFO_STEP_X + 1),(VFO_STEP_Y + 1),(VFO_STEP_X + VFO_STEP_SIZE_X - 1),(VFO_STEP_Y + VFO_STEP_SIZE_Y - 1),2);
			GUI_SetColor(GUI_ORANGE);
			GUI_DispStringAt("100kHz",(VFO_STEP_X + 3),(VFO_STEP_Y + 3));
			break;
		case T_STEP_1MHZ:
			GUI_FillRoundedRect((VFO_STEP_X + 1),(VFO_STEP_Y + 1),(VFO_STEP_X + VFO_STEP_SIZE_X - 1),(VFO_STEP_Y + VFO_STEP_SIZE_Y - 1),2);
			GUI_SetColor(GUI_ORANGE);
			GUI_DispStringAt("1MHz",(VFO_STEP_X + 13),(VFO_STEP_Y + 3));
			break;
		case T_STEP_10MHZ:
			GUI_FillRoundedRect((VFO_STEP_X + 1),(VFO_STEP_Y + 1),(VFO_STEP_X + VFO_STEP_SIZE_X - 1),(VFO_STEP_Y + VFO_STEP_SIZE_Y - 1),2);
			GUI_SetColor(GUI_ORANGE);
			GUI_DispStringAt("10MHz",(VFO_STEP_X + 8),(VFO_STEP_Y + 3));
			break;
		default:
			break;
	}

	// Save for next call, to prevent over repaint
	loc_step = step;
}

//*----------------------------------------------------------------------------
//* Function Name       :
//* Object              :
//* Notes    			:
//* Notes   			:
//* Notes    			:
//* Context    			: CONTEXT_VIDEO
//*----------------------------------------------------------------------------
void ui_controls_vfo_step_refresh(void)
{
#if 0
	// Encoder button clicked ?
	if(!HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_12))
	{
		while(!HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_12));

		//printf("encoder clicked\r\n");

		// toggle 100 Hz and 1 khz
		if(tsu.band[tsu.curr_band].step != T_STEP_100HZ)
		{
			tsu.band[tsu.curr_band].step = T_STEP_100HZ;
			//tsu.band[tsu.curr_band].step_idx = 2;
		}
		else
		{
			tsu.band[tsu.curr_band].step = T_STEP_1KHZ;
			//tsu.band[tsu.curr_band].step_idx = 3;
		}
	}
#endif

	//printf("dsp step %d\r\n",tsu.dsp_step);
	//printf("on control refresh - tsu step %d\r\n",tsu.band[tsu.curr_band].step);

	ui_controls_vfo_step_update_step();

}

//*----------------------------------------------------------------------------
//* Function Name       :
//* Object              :
//* Notes    			:
//* Notes   			:
//* Notes    			:
//* Context    			: CONTEXT_VIDEO
//*----------------------------------------------------------------------------
void ui_controls_vfo_step_init(void)
{
	loc_step = 0;

	// If using the frequency encoder switch
	ui_controls_vfo_init_encoder_switch_pin();

	GUI_SetColor(GUI_ORANGE);
	GUI_DrawRoundedRect((VFO_STEP_X + 0),(VFO_STEP_Y + 0),(VFO_STEP_X + VFO_STEP_SIZE_X),(VFO_STEP_Y + VFO_STEP_SIZE_Y),2);
	GUI_SetFont(&GUI_Font8x16_1);
	//GUI_DispStringAt("1kHz",(VFO_STEP_X + 10),(VFO_STEP_Y + 3));

	//printf("on control init - dsp step %d\r\n",tsu.dsp_step);
	//printf("on control init - tsu step %d\r\n",tsu.band[tsu.curr_band].step);

//!	tsu.band[tsu.curr_band].step = tune_steps[tsu.dsp_step_idx];
	ui_controls_vfo_step_update_step();
}

//*----------------------------------------------------------------------------
//* Function Name       :
//* Object              :
//* Notes    			:
//* Notes   			:
//* Notes    			:
//* Context    			: CONTEXT_VIDEO
//*----------------------------------------------------------------------------
void ui_controls_band_refresh(void)
{
	// Does keypad driver detected band change
	// and change public flag ?
	if(loc_band == tsu.curr_band)
		return;

	// -----------------------------------------------------------
	// Controls that need complete repaint - call them directly
	// the rest - auto update via public flags (actually the tsu.curr_band index)
	ui_controls_frequency_change_band();
	// ...

	GUI_SetColor(GUI_WHITE);
	GUI_FillRoundedRect((BAND_X + 50),(BAND_Y + 2),(BAND_X + 100),(BAND_Y + 17),2);

	GUI_SetColor(GUI_GRAY);
	GUI_SetFont(&GUI_Font20_1);

	switch(tsu.curr_band)
	{
		case BAND_MODE_2200:
			GUI_DispStringAt("2200",(BAND_X + 54),(BAND_Y + 0));
			break;
		case BAND_MODE_630:
			GUI_DispStringAt("630m",(BAND_X + 54),(BAND_Y + 0));
			break;
		case BAND_MODE_160:
			GUI_DispStringAt("160m",(BAND_X + 54),(BAND_Y + 0));
			break;
		case BAND_MODE_80:
			GUI_DispStringAt("80m",(BAND_X + 58),(BAND_Y + 0));
			break;
		case BAND_MODE_60:
			GUI_DispStringAt("60m",(BAND_X + 58),(BAND_Y + 0));
			break;
		case BAND_MODE_40:
			GUI_DispStringAt("40m",(BAND_X + 58),(BAND_Y + 0));
			break;
		case BAND_MODE_30:
			GUI_DispStringAt("30m",(BAND_X + 58),(BAND_Y + 0));
			break;
		case BAND_MODE_20:
			GUI_DispStringAt("20m",(BAND_X + 58),(BAND_Y + 0));
			break;
		case BAND_MODE_17:
			GUI_DispStringAt("17m",(BAND_X + 58),(BAND_Y + 0));
			break;
		case BAND_MODE_15:
			GUI_DispStringAt("15m",(BAND_X + 58),(BAND_Y + 0));
			break;
		case BAND_MODE_12:
			GUI_DispStringAt("12m",(BAND_X + 58),(BAND_Y + 0));
			break;
		case BAND_MODE_10:
			GUI_DispStringAt("10m",(BAND_X + 58),(BAND_Y + 0));
			break;
		case BAND_MODE_GEN:
			GUI_DispStringAt("GEN",(BAND_X + 56),(BAND_Y + 0));
			break;
		default:
			break;
	}
	loc_band = tsu.curr_band;

	//WRITE_EEPROM(EEP_CURR_BAND,tsu.curr_band);
}

//*----------------------------------------------------------------------------
//* Function Name       :
//* Object              :
//* Notes    			:
//* Notes   			:
//* Notes    			:
//* Context    			: CONTEXT_VIDEO
//*----------------------------------------------------------------------------
static void ui_controls_band_init(void)
{
	loc_band = 55;

	GUI_SetColor(GUI_GRAY);
	GUI_FillRoundedRect((BAND_X + 0),(BAND_Y + 0),(BAND_X + 102),(BAND_Y + 19),2);
	GUI_SetFont(&GUI_Font20_1);
	GUI_SetColor(GUI_WHITE);
	GUI_DispStringAt("Band",(BAND_X + 4),(BAND_Y + 0));
	GUI_SetColor(GUI_WHITE);
	GUI_FillRoundedRect((BAND_X + 50),(BAND_Y + 2),(BAND_X + 100),(BAND_Y + 17),2);
	//GUI_SetColor(GUI_GRAY);
	//GUI_DispStringAt("40m",(BAND_X + 58),(BAND_Y + 0));

	ui_controls_band_refresh();
}

//*----------------------------------------------------------------------------
//* Function Name       : ui_controls_frequency_update_vfo_a
//* Object              :
//* Notes    			: 1. this function needs to be really, really fast
//* Notes   			: 2. do not use too much stack, as callers chain could
//* Notes    			:    be long(all screen coordinates as const !!)
//* Notes    			: 3. no C lib calls whatsoever
//* Notes    			:
//* Context    			: CONTEXT_VIDEO
//*----------------------------------------------------------------------------
static int ui_controls_frequency_update_vfo_a(ulong freq)
{
	uchar		d_100mhz,d_10mhz,d_1mhz;
	uchar		d_100khz,d_10khz,d_1khz;
	uchar		d_100hz,d_10hz,d_1hz;
	char		digit[2];

	// As this function would be called very often(lag bigger than 10mS is
	// visible on the frequency display), we need to make sure no needless
	// updates. So only when there is actual change of the frequency
	if(df.vfo_a_scr_freq == freq)
		return 0;

	//printf("freq a: %d\r\n",freq);

	// Did step change ? Then do full repaint
	// Still not perfect, as highlighted digit
	// changes only when dial is moved
	if(df.last_screen_step != tsu.band[tsu.curr_band].step)
	{
		// Invalidate all
		df.vfo_a_segments_invalid = 1;

		// Save to prevent needless repaint
		df.last_screen_step = tsu.band[tsu.curr_band].step;
	}

	// Terminate
	digit[1] = 0;

	// Set Digit font
	GUI_SetFont(FREQ_FONT);

	// -----------------------
	// See if 100 Mhz needs update
	d_100mhz = (freq/100000000);
	if((d_100mhz != df.dial_100_mhz) || (df.vfo_a_segments_invalid))
	{
		// Delete segment
		#ifdef SHOW_MASKING
		GUI_SetColor(MASKING_COLOR);
		#else
		GUI_SetColor(GUI_BLACK);
		#endif
		GUI_FillRect((M_FREQ_X + 1 + (FREQ_FONT_SIZE_X * 0)),(M_FREQ_Y + 5),(M_FREQ_X + FREQ_FONT_SIZE_X + 1 + (FREQ_FONT_SIZE_X * 0)),(M_FREQ_Y + 5 + FREQ_FONT_SIZE_Y));
		if(tsu.band[tsu.curr_band].active_vfo == VFO_A)
			GUI_SetColor(VFO_A_SEL_DIGIT_COLOR);
		else
			GUI_SetColor(GUI_WHITE);
		// To string
		digit[0] = 0x30 + (d_100mhz & 0x0F);
		// Update segment
		if(d_100mhz) GUI_DispStringAt(digit,(M_FREQ_X + 1 + (FREQ_FONT_SIZE_X * 0)),(M_FREQ_Y + 5));
		// Save value
		df.dial_100_mhz = d_100mhz;

	}
	// -----------------------
	// See if 10 Mhz needs update
	d_10mhz = (freq%100000000)/10000000;
	if((d_10mhz != df.dial_010_mhz) || (df.vfo_a_segments_invalid))
	{
		// Delete segment
		#ifdef SHOW_MASKING
		GUI_SetColor(MASKING_COLOR);
		#else
		GUI_SetColor(GUI_BLACK);
		#endif
		GUI_FillRect((M_FREQ_X + 1 + (FREQ_FONT_SIZE_X * 1)),(M_FREQ_Y + 5),(M_FREQ_X + FREQ_FONT_SIZE_X + 1 + (FREQ_FONT_SIZE_X * 1)),(M_FREQ_Y + 5 + FREQ_FONT_SIZE_Y));
		if(tsu.band[tsu.curr_band].active_vfo == VFO_A)
		{
			if(tsu.band[tsu.curr_band].step == T_STEP_10MHZ)
				GUI_SetColor(VFO_A_SEL_DIGIT_COLOR);
			else
				GUI_SetColor(GUI_WHITE);
		}
		else
			GUI_SetColor(GUI_GRAY);
		// To string
		digit[0] = 0x30 + (d_10mhz & 0x0F);
		// Update segment
		if(d_100mhz)	// update if 100 MHz digit is being displayed
			GUI_DispStringAt(digit,(M_FREQ_X + 1 + (FREQ_FONT_SIZE_X * 1)),(M_FREQ_Y + 5));
		else
		{
			if(d_10mhz)
				GUI_DispStringAt(digit,(M_FREQ_X + 1 + (FREQ_FONT_SIZE_X * 1)),(M_FREQ_Y + 5));
		}
		// Save value
		df.dial_010_mhz = d_10mhz;
	}
	// -----------------------
	// See if 1 Mhz needs update
	d_1mhz = (freq%10000000)/1000000;
	if((d_1mhz != df.dial_001_mhz) || (df.vfo_a_segments_invalid))
	{
		// Delete segment
		#ifdef SHOW_MASKING
		GUI_SetColor(MASKING_COLOR);
		#else
		GUI_SetColor(GUI_BLACK);
		#endif
		GUI_FillRect((M_FREQ_X + 1 + (FREQ_FONT_SIZE_X * 2)),(M_FREQ_Y + 5),(M_FREQ_X + FREQ_FONT_SIZE_X + 1 + (FREQ_FONT_SIZE_X * 2)),(M_FREQ_Y + 5 + FREQ_FONT_SIZE_Y));
		if(tsu.band[tsu.curr_band].active_vfo == VFO_A)
		{
			if(tsu.band[tsu.curr_band].step == T_STEP_1MHZ)
				GUI_SetColor(VFO_A_SEL_DIGIT_COLOR);
			else
				GUI_SetColor(GUI_WHITE);
		}
		else
			GUI_SetColor(GUI_GRAY);
		// To string
		digit[0] = 0x30 + (d_1mhz & 0x0F);
		// Update segment
		GUI_DispStringAt(digit,(M_FREQ_X + 1 + (FREQ_FONT_SIZE_X * 2)),(M_FREQ_Y + 5));
		// Save value
		df.dial_001_mhz = d_1mhz;
	}
	// -----------------------
	// See if 100 khz needs update
	d_100khz = (freq%1000000)/100000;
	if((d_100khz != df.dial_100_khz) || (df.vfo_a_segments_invalid))
	{
		// Delete segment
		#ifdef SHOW_MASKING
		GUI_SetColor(MASKING_COLOR);
		#else
		GUI_SetColor(GUI_BLACK);
		#endif
		GUI_FillRect((M_FREQ_X + 1 + (FREQ_FONT_SIZE_X * 4)),(M_FREQ_Y + 5),(M_FREQ_X + FREQ_FONT_SIZE_X + 1 + (FREQ_FONT_SIZE_X * 4)),(M_FREQ_Y + 5 + FREQ_FONT_SIZE_Y));
		if(tsu.band[tsu.curr_band].active_vfo == VFO_A)
		{
			if(tsu.band[tsu.curr_band].step == T_STEP_100KHZ)
				GUI_SetColor(VFO_A_SEL_DIGIT_COLOR);
			else
				GUI_SetColor(GUI_WHITE);
		}
		else
			GUI_SetColor(GUI_GRAY);
		// To string
		digit[0] = 0x30 + (d_100khz & 0x0F);
		// Update segment
		GUI_DispStringAt(digit,(M_FREQ_X + 1 + (FREQ_FONT_SIZE_X * 4)),(M_FREQ_Y + 5));
		// Save value
		df.dial_100_khz = d_100khz;
	}
	// -----------------------
	// See if 10 khz needs update
	d_10khz = (freq%100000)/10000;
	if((d_10khz != df.dial_010_khz) || (df.vfo_a_segments_invalid))
	{
		// Delete segment
		#ifdef SHOW_MASKING
		GUI_SetColor(MASKING_COLOR);
		#else
		GUI_SetColor(GUI_BLACK);
		#endif
		GUI_FillRect((M_FREQ_X + 1 + (FREQ_FONT_SIZE_X * 5)),(M_FREQ_Y + 5),(M_FREQ_X + FREQ_FONT_SIZE_X + 1 + (FREQ_FONT_SIZE_X * 5)),(M_FREQ_Y + 5 + FREQ_FONT_SIZE_Y));
		if(tsu.band[tsu.curr_band].active_vfo == VFO_A)
		{
			if(tsu.band[tsu.curr_band].step == T_STEP_10KHZ)
				GUI_SetColor(VFO_A_SEL_DIGIT_COLOR);
			else
				GUI_SetColor(GUI_WHITE);
		}
		else
			GUI_SetColor(GUI_GRAY);
		// To string
		digit[0] = 0x30 + (d_10khz & 0x0F);
		// Update segment
		GUI_DispStringAt(digit,(M_FREQ_X + 1 + (FREQ_FONT_SIZE_X * 5)),(M_FREQ_Y + 5));
		// Save value
		df.dial_010_khz = d_10khz;
	}
	// -----------------------
	// See if 1 khz needs update
	d_1khz = (freq%10000)/1000;
	if((d_1khz != df.dial_001_khz) || (df.vfo_a_segments_invalid))
	{
		//printf("df.dial_001_khz    : %08x\r\n",df.dial_001_khz);
		//printf("tsu.band[tsu.curr_band].step    : %08x\r\n",tsu.band[tsu.curr_band].step);

		// Delete segment
		#ifdef SHOW_MASKING
		GUI_SetColor(MASKING_COLOR);
		#else
		GUI_SetColor(GUI_BLACK);
		#endif
		GUI_FillRect((M_FREQ_X + 1 + (FREQ_FONT_SIZE_X * 6)),(M_FREQ_Y + 5),(M_FREQ_X + FREQ_FONT_SIZE_X + 1 + (FREQ_FONT_SIZE_X * 6)),(M_FREQ_Y + 5 + FREQ_FONT_SIZE_Y));
		if(tsu.band[tsu.curr_band].active_vfo == VFO_A)
		{
			if(tsu.band[tsu.curr_band].step == T_STEP_1KHZ)
				GUI_SetColor(VFO_A_SEL_DIGIT_COLOR);
			else
				GUI_SetColor(GUI_WHITE);
		}
		else
			GUI_SetColor(GUI_GRAY);
		// To string
		digit[0] = 0x30 + (d_1khz & 0x0F);
		// Update segment
		GUI_DispStringAt(digit,(M_FREQ_X + 1 + (FREQ_FONT_SIZE_X * 6)),(M_FREQ_Y + 5));
		// Save value
		df.dial_001_khz = d_1khz;
	}
	// -----------------------
	// See if 100 hz needs update
	d_100hz = (freq%1000)/100;
	if((d_100hz != df.dial_100_hz) || (df.vfo_a_segments_invalid))
	{
		// Delete segment
		#ifdef SHOW_MASKING
		GUI_SetColor(MASKING_COLOR);
		#else
		GUI_SetColor(GUI_BLACK);
		#endif
		GUI_FillRect((M_FREQ_X + 1 + (FREQ_FONT_SIZE_X * 8)),(M_FREQ_Y + 5),(M_FREQ_X + FREQ_FONT_SIZE_X + 1 + (FREQ_FONT_SIZE_X * 8)),(M_FREQ_Y + 5 + FREQ_FONT_SIZE_Y));
		if(tsu.band[tsu.curr_band].active_vfo == VFO_A)
		{
			if(tsu.band[tsu.curr_band].step == T_STEP_100HZ)
				GUI_SetColor(VFO_A_SEL_DIGIT_COLOR);
			else
				GUI_SetColor(GUI_WHITE);
		}
		else
			GUI_SetColor(GUI_GRAY);
		// To string
		digit[0] = 0x30 + (d_100hz & 0x0F);
		// Update segment
		GUI_DispStringAt(digit,(M_FREQ_X + 1 + (FREQ_FONT_SIZE_X * 8)),(M_FREQ_Y + 5));
		// Save value
		df.dial_100_hz = d_100hz;
	}
	// -----------------------
	// See if 10 hz needs update
	d_10hz = (freq%100)/10;
	if((d_10hz != df.dial_010_hz) || (df.vfo_a_segments_invalid))
	{
		// Delete segment
		#ifdef SHOW_MASKING
		GUI_SetColor(MASKING_COLOR);
		#else
		GUI_SetColor(GUI_BLACK);
		#endif
		GUI_FillRect((M_FREQ_X + 1 + (FREQ_FONT_SIZE_X * 9)),(M_FREQ_Y + 5),(M_FREQ_X + FREQ_FONT_SIZE_X + 1 + (FREQ_FONT_SIZE_X * 9)),(M_FREQ_Y + 5 + FREQ_FONT_SIZE_Y));
		if(tsu.band[tsu.curr_band].active_vfo == VFO_A)
		{
			if(tsu.band[tsu.curr_band].step == T_STEP_10HZ)
				GUI_SetColor(VFO_A_SEL_DIGIT_COLOR);
			else
				GUI_SetColor(GUI_WHITE);
		}
		else
			GUI_SetColor(GUI_GRAY);
		// To string
		digit[0] = 0x30 + (d_10hz & 0x0F);
		// Update segment
		GUI_DispStringAt(digit,(M_FREQ_X + 1 + (FREQ_FONT_SIZE_X * 9)),(M_FREQ_Y + 5));
		// Save value
		df.dial_010_hz = d_10hz;
	}
	// -----------------------
	// See if 1 hz needs update
	d_1hz = (freq%10)/1;
	if((d_1hz != df.dial_001_hz) || (df.vfo_a_segments_invalid))
	{
		// Delete segment
		#ifdef SHOW_MASKING
		GUI_SetColor(MASKING_COLOR);
		#else
		GUI_SetColor(GUI_BLACK);
		#endif
		GUI_FillRect((M_FREQ_X + 1 + (FREQ_FONT_SIZE_X * 10)),(M_FREQ_Y + 5),(M_FREQ_X + FREQ_FONT_SIZE_X + 1 + (FREQ_FONT_SIZE_X * 10)),(M_FREQ_Y + 5 + FREQ_FONT_SIZE_Y));
		if(tsu.band[tsu.curr_band].active_vfo == VFO_A)
		{
			if(tsu.band[tsu.curr_band].step == T_STEP_1HZ)
				GUI_SetColor(VFO_A_SEL_DIGIT_COLOR);
			else
				GUI_SetColor(GUI_WHITE);
		}
		else
			GUI_SetColor(GUI_GRAY);
		// To string
		digit[0] = 0x30 + (d_1hz & 0x0F);
		// Update segment
		GUI_DispStringAt(digit,(M_FREQ_X + 1 + (FREQ_FONT_SIZE_X * 10)),(M_FREQ_Y + 5));
		// Save value
		df.dial_001_hz = d_1hz;
	}

	// Invalidation valid only for one call
	if(df.vfo_a_segments_invalid)
		df.vfo_a_segments_invalid 	= 0;

	// Save to public, to prevent stalling the task
	df.vfo_a_scr_freq = freq;

	return 1;
}

//*----------------------------------------------------------------------------
//* Function Name       : ui_controls_frequency_update_vfo_b
//* Object              :
//* Notes    			: 1. this function needs to be really, really fast
//* Notes   			: 2. do not use too much stack, as callers chain could
//* Notes    			:    be long(all screen coordinates as const !!)
//* Notes    			: 3. no C lib calls whatsoever
//* Notes    			:
//* Context    			: CONTEXT_VIDEO
//*----------------------------------------------------------------------------
static void ui_controls_frequency_update_vfo_b(ulong freq)
{
	uchar		d_100mhz,d_10mhz,d_1mhz;
	uchar		d_100khz,d_10khz,d_1khz;
	uchar		d_100hz,d_10hz,d_1hz;
	char		digit[2];

	// As this function would be called very often(lag bigger than 10mS is
	// visible on the frequency display), we need to make sure no needless
	// updates. So only when there is actual change of the frequency
	if(df.vfo_b_scr_freq == freq)
		return;

	//printf("freq b: %d\r\n",freq);

	// Did step change ? Then do full repaint
	// by resetting saved segment values
	// Still not perfect, as highlighted digit
	// changes only when dial is moved
	if(df.last_screen_step != tsu.band[tsu.curr_band].step)
	{
		// Invalidate all
		df.vfo_b_segments_invalid = 1;

		// Save to prevent needless repaint
		df.last_screen_step = tsu.band[tsu.curr_band].step;
	}

	// Terminate
	digit[1] = 0;

	// Set Digit font
	GUI_SetFont(GUI_FONT_24B_1);

	// -----------------------
	// See if 100 Mhz needs update
	d_100mhz = (freq/100000000);
	if((d_100mhz != df.sdial_100_mhz) || (df.vfo_b_segments_invalid))
	{
		// Delete segment
		//GUI_SetColor(GUI_BLUE);
		GUI_SetColor(GUI_GRAY);
		GUI_FillRect((M_FREQ1_X + 47 + (FREQ_FONT_SIZE1_X * 0)),(M_FREQ1_Y + 4),(M_FREQ1_X + 46 + (FREQ_FONT_SIZE1_X * 1)),(M_FREQ1_Y + 4 + FREQ_FONT_SIZE1_Y));
		if(tsu.band[tsu.curr_band].active_vfo == VFO_B)
			GUI_SetColor(VFO_B_SEG_ON_COLOR);
		else
			GUI_SetColor(VFO_B_SEG_OFF_COLOR);
		// To string
		digit[0] = 0x30 + (d_100mhz & 0x0F);
		// Update segment
		if(d_100mhz) GUI_DispStringAt(digit,(M_FREQ1_X + 48 + (FREQ_FONT_SIZE1_X * 0)),(M_FREQ1_Y + 1));
		// Save value
		df.sdial_100_mhz = d_100mhz;

	}
	// -----------------------
	// See if 10 Mhz needs update
	d_10mhz = (freq%100000000)/10000000;
	if((d_10mhz != df.sdial_010_mhz) || (df.vfo_b_segments_invalid))
	{
		// Delete segment
		//GUI_SetColor(GUI_YELLOW);
		GUI_SetColor(GUI_GRAY);
		GUI_FillRect((M_FREQ1_X + 47 + (FREQ_FONT_SIZE1_X * 1)),(M_FREQ1_Y + 4),(M_FREQ1_X + 46 + (FREQ_FONT_SIZE1_X * 2)),(M_FREQ1_Y + 4 + FREQ_FONT_SIZE1_Y));
		if(tsu.band[tsu.curr_band].active_vfo == VFO_B)
		{
			if(tsu.band[tsu.curr_band].step == T_STEP_10MHZ)
				GUI_SetColor(VFO_B_SEG_SEL_COLOR);
			else
				GUI_SetColor(VFO_B_SEG_ON_COLOR);
		}
		else
			GUI_SetColor(VFO_B_SEG_OFF_COLOR);
		// To string
		digit[0] = 0x30 + (d_10mhz & 0x0F);
		// Update segment
		if(d_100mhz)	// update if 100 MHz digit is being displayed
			GUI_DispStringAt(digit,(M_FREQ1_X + 48 + (FREQ_FONT_SIZE1_X * 1)),(M_FREQ1_Y + 1));
		else
		{
			if(d_10mhz)
				GUI_DispStringAt(digit,(M_FREQ1_X + 48 + (FREQ_FONT_SIZE1_X * 1)),(M_FREQ1_Y + 1));
		}
		// Save value
		df.sdial_010_mhz = d_10mhz;
	}
	// -----------------------
	// See if 1 Mhz needs update
	d_1mhz = (freq%10000000)/1000000;
	if((d_1mhz != df.sdial_001_mhz) || (df.vfo_b_segments_invalid))
	{
		// Delete segment
		//GUI_SetColor(GUI_BLUE);
		GUI_SetColor(GUI_GRAY);
		GUI_FillRect((M_FREQ1_X + 47 + (FREQ_FONT_SIZE1_X * 2)),(M_FREQ1_Y + 4),(M_FREQ1_X + 46 + (FREQ_FONT_SIZE1_X * 3)),(M_FREQ1_Y + 4 + FREQ_FONT_SIZE1_Y));
		if(tsu.band[tsu.curr_band].active_vfo == VFO_B)
		{
			if(tsu.band[tsu.curr_band].step == T_STEP_1MHZ)
				GUI_SetColor(VFO_B_SEG_SEL_COLOR);
			else
				GUI_SetColor(VFO_B_SEG_ON_COLOR);
		}
		else
			GUI_SetColor(VFO_B_SEG_OFF_COLOR);
		// To string
		digit[0] = 0x30 + (d_1mhz & 0x0F);
		// Update segment
		GUI_DispStringAt(digit,(M_FREQ1_X + 48 + (FREQ_FONT_SIZE1_X * 2)),(M_FREQ1_Y + 1));
		// Save value
		df.sdial_001_mhz = d_1mhz;
	}
	// -----------------------
	// See if 100 khz needs update
	d_100khz = (freq%1000000)/100000;
	if((d_100khz != df.sdial_100_khz) || (df.vfo_b_segments_invalid))
	{
		// Delete segment
		//GUI_SetColor(GUI_YELLOW);
		GUI_SetColor(GUI_GRAY);
		GUI_FillRect((M_FREQ1_X + 41 + (FREQ_FONT_SIZE1_X * 4)),(M_FREQ1_Y + 4),(M_FREQ1_X + 40 + (FREQ_FONT_SIZE1_X * 5)),(M_FREQ1_Y + 4 + FREQ_FONT_SIZE1_Y));
		if(tsu.band[tsu.curr_band].active_vfo == VFO_B)
		{
			if(tsu.band[tsu.curr_band].step == T_STEP_100KHZ)
				GUI_SetColor(VFO_B_SEG_SEL_COLOR);
			else
				GUI_SetColor(VFO_B_SEG_ON_COLOR);
		}
		else
			GUI_SetColor(VFO_B_SEG_OFF_COLOR);
		// To string
		digit[0] = 0x30 + (d_100khz & 0x0F);
		// Update segment
		GUI_DispStringAt(digit,(M_FREQ1_X + 42 + (FREQ_FONT_SIZE1_X * 4)),(M_FREQ1_Y + 1));
		// Save value
		df.sdial_100_khz = d_100khz;
	}
	// -----------------------
	// See if 10 khz needs update
	d_10khz = (freq%100000)/10000;
	if((d_10khz != df.sdial_010_khz) || (df.vfo_b_segments_invalid))
	{
		// Delete segment
		//GUI_SetColor(GUI_BLUE);
		GUI_SetColor(GUI_GRAY);
		GUI_FillRect((M_FREQ1_X + 41 + (FREQ_FONT_SIZE1_X * 5)),(M_FREQ1_Y + 4),(M_FREQ1_X + 40 + (FREQ_FONT_SIZE1_X * 6)),(M_FREQ1_Y + 4 + FREQ_FONT_SIZE1_Y));
		if(tsu.band[tsu.curr_band].active_vfo == VFO_B)
		{
			if(tsu.band[tsu.curr_band].step == T_STEP_10KHZ)
				GUI_SetColor(VFO_B_SEG_SEL_COLOR);
			else
				GUI_SetColor(VFO_B_SEG_ON_COLOR);
		}
		else
			GUI_SetColor(VFO_B_SEG_OFF_COLOR);
		// To string
		digit[0] = 0x30 + (d_10khz & 0x0F);
		// Update segment
		GUI_DispStringAt(digit,(M_FREQ1_X + 42 + (FREQ_FONT_SIZE1_X * 5)),(M_FREQ1_Y + 1));
		// Save value
		df.sdial_010_khz = d_10khz;
	}
	// -----------------------
	// See if 1 khz needs update
	d_1khz = (freq%10000)/1000;
	if((d_1khz != df.sdial_001_khz) || (df.vfo_b_segments_invalid))
	{
		// Delete segment
		//GUI_SetColor(GUI_YELLOW);
		GUI_SetColor(GUI_GRAY);
		GUI_FillRect((M_FREQ1_X + 41 + (FREQ_FONT_SIZE1_X * 6)),(M_FREQ1_Y + 4),(M_FREQ1_X + 40 + (FREQ_FONT_SIZE1_X * 7)),(M_FREQ1_Y + 4 + FREQ_FONT_SIZE1_Y));
		if(tsu.band[tsu.curr_band].active_vfo == VFO_B)
		{
			if(tsu.band[tsu.curr_band].step == T_STEP_1KHZ)
				GUI_SetColor(VFO_B_SEG_SEL_COLOR);
			else
				GUI_SetColor(VFO_B_SEG_ON_COLOR);
		}
		else
			GUI_SetColor(VFO_B_SEG_OFF_COLOR);
		// To string
		digit[0] = 0x30 + (d_1khz & 0x0F);
		// Update segment
		GUI_DispStringAt(digit,(M_FREQ1_X + 42 + (FREQ_FONT_SIZE1_X * 6)),(M_FREQ1_Y + 1));
		// Save value
		df.sdial_001_khz = d_1khz;
	}
	// -----------------------
	// See if 100 hz needs update
	d_100hz = (freq%1000)/100;
	if((d_100hz != df.sdial_100_hz) || (df.vfo_b_segments_invalid))
	{
		// Delete segment
		//GUI_SetColor(GUI_BLUE);
		GUI_SetColor(GUI_GRAY);
		GUI_FillRect((M_FREQ1_X + 35 + (FREQ_FONT_SIZE1_X * 8)),(M_FREQ1_Y + 4),(M_FREQ1_X + 34 + (FREQ_FONT_SIZE1_X * 9)),(M_FREQ1_Y + 4 + FREQ_FONT_SIZE1_Y));
		if(tsu.band[tsu.curr_band].active_vfo == VFO_B)
		{
			if(tsu.band[tsu.curr_band].step == T_STEP_100HZ)
				GUI_SetColor(VFO_B_SEG_SEL_COLOR);
			else
				GUI_SetColor(VFO_B_SEG_ON_COLOR);
		}
		else
			GUI_SetColor(VFO_B_SEG_OFF_COLOR);
		// To string
		digit[0] = 0x30 + (d_100hz & 0x0F);
		// Update segment
		GUI_DispStringAt(digit,(M_FREQ1_X + 36 + (FREQ_FONT_SIZE1_X * 8)),(M_FREQ1_Y + 1));
		// Save value
		df.sdial_100_hz = d_100hz;
	}
	// -----------------------
	// See if 10 hz needs update
	d_10hz = (freq%100)/10;
	if((d_10hz != df.sdial_010_hz) || (df.vfo_b_segments_invalid))
	{
		// Delete segment
		//GUI_SetColor(GUI_YELLOW);
		GUI_SetColor(GUI_GRAY);
		GUI_FillRect((M_FREQ1_X + 35 + (FREQ_FONT_SIZE1_X * 9)),(M_FREQ1_Y + 4),(M_FREQ1_X + 34 + (FREQ_FONT_SIZE1_X * 10)),(M_FREQ1_Y + 4 + FREQ_FONT_SIZE1_Y));
		if(tsu.band[tsu.curr_band].active_vfo == VFO_B)
		{
			if(tsu.band[tsu.curr_band].step == T_STEP_10HZ)
				GUI_SetColor(VFO_B_SEG_SEL_COLOR);
			else
				GUI_SetColor(VFO_B_SEG_ON_COLOR);
		}
		else
			GUI_SetColor(VFO_B_SEG_OFF_COLOR);
		// To string
		digit[0] = 0x30 + (d_10hz & 0x0F);
		// Update segment
		GUI_DispStringAt(digit,(M_FREQ1_X + 36 + (FREQ_FONT_SIZE1_X * 9)),(M_FREQ1_Y + 1));
		// Save value
		df.sdial_010_hz = d_10hz;
	}
	// -----------------------
	// See if 1 hz needs update
	d_1hz = (freq%10)/1;
	if((d_1hz != df.sdial_001_hz) || (df.vfo_b_segments_invalid))
	{
		// Delete segment
		//GUI_SetColor(GUI_BLUE);
		GUI_SetColor(GUI_GRAY);
		GUI_FillRect((M_FREQ1_X + 35 + (FREQ_FONT_SIZE1_X * 10)),(M_FREQ1_Y + 4),(M_FREQ1_X + 34 + (FREQ_FONT_SIZE1_X * 11)),(M_FREQ1_Y + 4 + FREQ_FONT_SIZE1_Y));
		if(tsu.band[tsu.curr_band].active_vfo == VFO_B)
		{
			if(tsu.band[tsu.curr_band].step == T_STEP_1HZ)
				GUI_SetColor(VFO_B_SEG_SEL_COLOR);
			else
				GUI_SetColor(VFO_B_SEG_ON_COLOR);
		}
		else
			GUI_SetColor(VFO_B_SEG_OFF_COLOR);
		// To string
		digit[0] = 0x30 + (d_1hz & 0x0F);
		// Update segment
		GUI_DispStringAt(digit,(M_FREQ1_X + 36 + (FREQ_FONT_SIZE1_X * 10)),(M_FREQ1_Y + 1));
		// Save value
		df.sdial_001_hz = d_1hz;
	}

	// Invalidation valid only for one call
	if(df.vfo_b_segments_invalid)
		df.vfo_b_segments_invalid 	= 0;

	// Save to public, to prevent stalling the task
	df.vfo_b_scr_freq = freq;
}

//*----------------------------------------------------------------------------
//* Function Name       : ui_controls_frequency_vfo_a_initial_paint
//* Object              :
//* Notes    			:
//* Notes   			:
//* Notes    			:
//* Context    			: CONTEXT_VIDEO
//*----------------------------------------------------------------------------
static void ui_controls_frequency_vfo_a_initial_paint(uchar is_init)
{
	//printf("ui_controls_frequency_vfo_a_initial_paint\r\n");
	//printf("active vfo is %d\r\n",tsu.band[tsu.curr_band].active_vfo);

	// Set virtual segments initial value (diff than zero!)
	df.vfo_a_scr_freq 	= 0;
	df.last_screen_step	= 0xFFFFFF99;
	//
	df.dial_100_mhz		= 9;
	df.dial_010_mhz		= 9;
	df.dial_001_mhz		= 9;
	df.dial_100_khz		= 9;
	df.dial_010_khz		= 9;
	df.dial_001_khz		= 9;
	df.dial_100_hz		= 9;
	df.dial_010_hz		= 9;
	df.dial_001_hz		= 9;

	// Mask the whole control
	#if 0
	GUI_SetColor(GUI_GRAY);
	#else
	GUI_SetColor(GUI_BLACK);
	#endif
	GUI_FillRoundedRect((M_FREQ_X + 0),
						(M_FREQ_Y + 0),
						(M_FREQ_X + FREQ_FONT_SIZE_X*11 + 2),
						(M_FREQ_Y + 55),
						2);

	// Frame
	#ifdef FRAME_MAIN_DIAL
	GUI_SetColor(GUI_DARKGRAY);
	//GUI_SetColor(GUI_WHITE);
	GUI_DrawRoundedRect((M_FREQ_X + 0),(M_FREQ_Y + 0),(M_FREQ_X + 267),(M_FREQ_Y + 40),2);
	#endif

	if(tsu.band[tsu.curr_band].active_vfo == VFO_A)
		GUI_SetColor(GUI_LIGHTGREEN);
	else
		GUI_SetColor(GUI_GRAY);
	GUI_SetFont(&GUI_Font20B_1);
	GUI_DispStringAt("VFO A",VFO_A_X, VFO_A_Y);

	// Digits colour
	GUI_SetFont(FREQ_FONT);
	if(tsu.band[tsu.curr_band].active_vfo == VFO_A)
		GUI_SetColor(GUI_WHITE);
	else
		GUI_SetColor(GUI_GRAY);

	// Digits text
	if(is_init)
		GUI_DispStringAt("999.999.999",(M_FREQ_X + 1),(M_FREQ_Y + 5));
	else
		GUI_DispStringAt("___.___.___",(M_FREQ_X + 1),(M_FREQ_Y + 5));

	// Update frequency, but only if not active
//	if(tsu.band[tsu.curr_band].active_vfo == VFO_B)
//		ui_controls_frequency_update_vfo_a(tsu.band[tsu.curr_band].vfo_a);
}

//*----------------------------------------------------------------------------
//* Function Name       : ui_controls_frequency_vfo_b_initial_paint
//* Object              :
//* Notes    			:
//* Notes   			:
//* Notes    			:
//* Context    			: CONTEXT_VIDEO
//*----------------------------------------------------------------------------
static void ui_controls_frequency_vfo_b_initial_paint(uchar is_init)
{
	//printf("ui_controls_frequency_vfo_b_initial_paint\r\n");
	//printf("active vfo is %d\r\n",tsu.active_vfo);

	// Publics reset
	df.vfo_b_scr_freq 	= 0;
	//
	df.sdial_100_mhz	= 9;
	df.sdial_010_mhz	= 9;
	df.sdial_001_mhz	= 9;
	df.sdial_100_khz	= 9;
	df.sdial_010_khz	= 9;
	df.sdial_001_khz	= 9;
	df.sdial_100_hz		= 9;
	df.sdial_010_hz		= 9;
	df.sdial_001_hz		= 9;

	// Frame
	GUI_SetColor(GUI_GRAY);
	GUI_FillRoundedRect((M_FREQ1_X + 46),(M_FREQ1_Y + 0),(M_FREQ1_X + M_FREQ1_X_SZ),(M_FREQ1_Y + 24),2);

	if(tsu.band[tsu.curr_band].active_vfo == VFO_B)
		GUI_SetColor(GUI_WHITE);
	else
		GUI_SetColor(GUI_LIGHTGRAY);

	GUI_DrawRoundedRect((M_FREQ1_X + 0),(M_FREQ1_Y + 0),(M_FREQ1_X + M_FREQ1_X_SZ),(M_FREQ1_Y + 24),2);

	// Leading text background
	if(tsu.band[tsu.curr_band].active_vfo == VFO_B)
		GUI_SetColor(GUI_RED);
	else
		GUI_SetColor(GUI_LIGHTGRAY);

	GUI_FillRoundedRect((M_FREQ1_X + 1),(M_FREQ1_Y + 1),(M_FREQ1_X + 46),(M_FREQ1_Y + 23),2);

	// Leading text colour
	if(tsu.band[tsu.curr_band].active_vfo == VFO_B)
		GUI_SetColor(GUI_WHITE);
	else
		GUI_SetColor(GUI_GRAY);

	// Leading text
	GUI_SetFont(GUI_FONT_16B_1);
	GUI_DispStringAt("VFO B",(M_FREQ1_X + 3),(M_FREQ1_Y + 5));

	// Digits
	if(tsu.band[tsu.curr_band].active_vfo == VFO_B)
		GUI_SetColor(VFO_B_SEG_ON_COLOR);
	else
		GUI_SetColor(VFO_B_SEG_OFF_COLOR);

	GUI_SetFont(GUI_FONT_24B_1);
	GUI_DispStringAt("999.999.999",(M_FREQ1_X + 48),(M_FREQ1_Y + 1));

	// Update frequency
	if(tsu.band[tsu.curr_band].active_vfo == VFO_A)
		ui_controls_frequency_update_vfo_b(tsu.band[tsu.curr_band].vfo_b);
}

//*----------------------------------------------------------------------------
//* Function Name       : ui_controls_frequency_refresh_a
//* Object              :
//* Notes    			:
//* Notes    			:
//* Notes    			:
//* Context    			: CONTEXT_VIDEO
//*----------------------------------------------------------------------------
static int ui_controls_frequency_refresh_a(uchar flags)
{
	ulong vfo_af;

	#if 0
	if(old_vfo != tsu.band[tsu.curr_band].vfo_a)
	{
		//printf("..\r\n");
		printf("freq = %d\r\n", tsu.band[tsu.curr_band].vfo_a);

		old_vfo = tsu.band[tsu.curr_band].vfo_a;
	}
	#endif

	// Did the active VFO changed since last repaint ?
	if(df.last_active_vfo != tsu.band[tsu.curr_band].active_vfo)
	{
		printf("vfo changed\r\n");

		// ToDo: If switching from center to fixed mode, copy VFO A to VFO B
		// 		 and on band changes, etc...


		// -----------------------------------------------------------
		// Update main oscillator frequency on vfo change
		//tsu.update_freq_dsp_req = 1;

		ui_controls_frequency_vfo_a_initial_paint(0);
		ui_controls_frequency_vfo_b_initial_paint(0);

		// Save
		df.last_active_vfo = tsu.band[tsu.curr_band].active_vfo;

		return 1;
	}

	// Normal dial repaint
	if(tsu.band[tsu.curr_band].active_vfo == VFO_A)
	{
		vfo_af = tsu.band[tsu.curr_band].vfo_a;

		// On screen frequency = (Osc + NCO)
		if(tsu.band[tsu.curr_band].fixed_mode)
			vfo_af += tsu.band[tsu.curr_band].nco_freq;

		//printf("vfo a upd\r\n");

		return ui_controls_frequency_update_vfo_a(vfo_af);
	}
	//else
	//{
	//	ui_controls_frequency_update_vfo_b(tsu.band[tsu.curr_band].vfo_b);
	//	return 1;
	//}

	return 0;
}

//*----------------------------------------------------------------------------
//* Function Name       : ui_controls_frequency_change_band
//* Object              : call from band control
//* Notes    			: - full repaint on both VFOs
//* Notes    			:
//* Notes    			:
//* Context    			: CONTEXT_VIDEO
//*----------------------------------------------------------------------------
void ui_controls_frequency_change_band(void)
{
#ifndef FREQ_USE_WM
	ui_controls_frequency_vfo_a_initial_paint(0);
	ui_controls_frequency_vfo_b_initial_paint(0);

	ui_controls_frequency_update_vfo_a(tsu.band[tsu.curr_band].vfo_a);
	ui_controls_frequency_update_vfo_b(tsu.band[tsu.curr_band].vfo_b);
#endif
}

static void ui_controls_frequency_create(void)
{
	// Create controls
	ui_controls_frequency_vfo_a_initial_paint(1);
	ui_controls_frequency_vfo_b_initial_paint(1);

	ui_controls_frequency_update_vfo_a(tsu.band[tsu.curr_band].vfo_a);

	ui_controls_band_init();
	ui_controls_vfo_step_init();
	ui_controls_rx_tx_init(1);
	ui_controls_demod_init();
	ui_controls_agc_init();
}

#ifdef FREQ_USE_WM
static void ui_controls_frequency_wm_handler(WM_MESSAGE *pMsg)
{
	WM_HWIN hItem;
	int 	Id, NCode;

	switch (pMsg->MsgId)
	{
		case WM_INIT_DIALOG:
		{
			// Initial paint
			ui_controls_frequency_create();

			hTimerFreq = WM_CreateTimer(pMsg->hWin, 0, FREQ_TIMER_RESOLUTION, 0);
			break;
		}

		case WM_TIMER:
		{
			//if(ui_controls_frequency_refresh_a(0))
			//{
				//printf("VFO A repaint\r\n");
				//WM_InvalidateWindow(hFreqDialog);
			//}

			WM_RestartTimer(pMsg->Data.v, FREQ_TIMER_RESOLUTION);
			break;
		}

		case WM_PAINT:
		{
			printf("WM_PAINT FREQ\r\n");

			ui_controls_frequency_refresh_a(0);
			WM_InvalidateWindow(hFreqDialog);

			//GUI_SetColor(GUI_RED);
			//GUI_DispStringAt("1Hz",(FREQ_P_X + 10),(FREQ_P_Y + 10));
			//GUI_DispStringInRect(acText, &TRect, GUI_TA_RIGHT | GUI_TA_VCENTER);
			break;
		}

		case WM_DELETE:
			WM_DeleteTimer(hTimerFreq);
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
#endif

// for none WM implementation only
int ui_controls_frequency_refresh(uchar flags)
{
	#ifndef FREQ_USE_WM
	ui_controls_frequency_refresh_a(0);
	ui_controls_rx_tx_init(0);
	#endif
}

//*----------------------------------------------------------------------------
//* Function Name       : ui_controls_frequency_init
//* Object              : Reset locals
//* Notes    			:
//* Notes   			:
//* Notes    			:
//* Context    			: CONTEXT_VIDEO
//*----------------------------------------------------------------------------
WM_HWIN ui_controls_frequency_init(WM_HWIN hParent)
{
	// Initial values
	df.last_active_vfo 			= tsu.band[tsu.curr_band].active_vfo;
	df.vfo_a_segments_invalid 	= 1;
	df.vfo_b_segments_invalid 	= 1;

	#ifdef FREQ_USE_WM
	hFreqDialog = GUI_CreateDialogBox(FreqDialog, GUI_COUNTOF(FreqDialog), ui_controls_frequency_wm_handler, hParent, FREQ_P_X, FREQ_P_Y);
	return hFreqDialog;
	#else
	ui_controls_frequency_create();
	#endif

	return 0;
}

void ui_controls_frequency_quit(void)
{
	#ifdef FREQ_USE_WM
	GUI_EndDialog(hFreqDialog, 0);
	#endif
}

#endif
