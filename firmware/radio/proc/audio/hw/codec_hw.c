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
#include "main.h"
#include "mchf_pro_board.h"

#ifdef CONTEXT_AUDIO

#include "shared_i2c.h"
#include "codec_hw.h"

//
// PGA gain table, 49 steps, each 0.5dB (-12 dB to + 12dB)
//
const int pga_gain_tbl[49] = {	0b101000,		// -12 dB
								0b101001,
								0b101010,
								0b101011,
								0b101100,
								0b101101,
								0b101110,
								0b101111,
								0b110000,
								0b110001,
								0b110010,
								0b110011,
								0b110100,
								0b110101,
								0b110110,
								0b110111,
								0b111000,
								0b111001,
								0b111010,
								0b111011,
								0b111100,
								0b111101,
								0b111110,
								0b111111,		// - 0.5 dB
								0b000000,
								0b000001,
								0b000010,
								0b000011,
								0b000100,
								0b000101,
								0b000110,
								0b000111,
								0b001000,
								0b001001,
								0b001010,
								0b001011,
								0b001100,
								0b001101,
								0b001110,
								0b001111,
								0b010000,
								0b010001,
								0b010010,
								0b010011,
								0b010100,
								0b010101,
								0b010110,
								0b010111,
								0b011000		// +12 dB
							};

// Public radio state
extern struct	TRANSCEIVER_STATE_UI	tsu;

// DSP core state
extern struct 	TransceiverState 		ts;

// -------------------------------------------------------------------------------
// Working, need sync up with SAI driver in the DSP
//#define SAMP_FREQ  					AUDIO_FREQUENCY_48K
//
// Working, need sync up with SAI driver in the DSP and maybe updated
// filter coefficients
//#define SAMP_FREQ  					AUDIO_FREQUENCY_96K
//
// This works on SAI/I2C level, but the FFT and data processing in
// the SAI IRQ can't keeep, would need DSP core coded in pure assembler
//
//#define SAMP_FREQ  					AUDIO_FREQUENCY_192K
//
// ------------------------------------------------------------------------------

static void codec_hw_decode_register(uchar data, uchar id)
{
	switch(id)
	{
		case CS4245_CHIP_ID:
		{
			printf("%02x: chip id: %02x, chip rev: %02x\r\n",
					id,
					data & CS4245_CHIP_PART_MASK,
					data & CS4245_CHIP_REV_MASK);
			break;
		}

		case CS4245_POWER_CTRL:
		{
			printf("%02x: FRZ: %d, PDN_MIC: %d, PDN_ADC: %d, PDN_DAC: %d, PDN: %d\r\n",	id,
					data & CS4245_FREEZE,
					data & CS4245_PDN_MIC,
					data & CS4245_PDN_ADC,
					data & CS4245_PDN_DAC,
					data & CS4245_PDN);
			break;
		}

		case CS4245_DAC_CTRL_1:
		{
			printf("%02x: DAC_FM: %d, DAC_DIF: %d, MUTE_DAC: %d, DEEMPH: %d, DAC_MS: %d\r\n",
					id,
					data & CS4245_DAC_FM_MASK,
					data & CS4245_DAC_DIF_MASK,
					data & CS4245_MUTE_DAC,
					data & CS4245_DEEMPH,
					data & CS4245_DAC_MASTER);
			break;
		}

		case CS4245_ADC_CTRL:
		{
			printf("%02x: ADC_FM: %d, ADC_DIF: %d, MUTE_ADC: %d, HPF_FREEZE: %d, ADC_MS: %d\r\n",
					id,
					data & CS4245_ADC_FM_MASK,
					data & CS4245_ADC_DIF_MASK,
					data & CS4245_MUTE_ADC,
					data & CS4245_HPF_FREEZE,
					data & CS4245_ADC_MASTER);
			break;
		}

		case CS4245_MCLK_FREQ:
		{
			printf("%02x: MCLK1: %d, MCLK2: %d\r\n",
					id,
					data & CS4245_MCLK1_MASK,
					data & CS4245_MCLK2_MASK);
			break;
		}

		case CS4245_SIGNAL_SEL:
		{
			printf("%02x: OUT_SEL_PGA: %d, OUT_SEL_DAC: %d, LOOP: %d, ASYNCH: %d\r\n",
					id,
					data & CS4245_A_OUT_SEL_PGA,
					data & CS4245_A_OUT_SEL_DAC,
					data & CS4245_LOOP,
					data & CS4245_ASYNCH);
			break;
		}

		case CS4245_PGA_B_CTRL:
		{
			printf("%02x: PGA_GAIN B: %d\r\n",
					id,
					data & CS4245_PGA_GAIN_MASK);
			break;
		}

		case CS4245_PGA_A_CTRL:
		{
			printf("%02x: PGA_GAIN A: %d\r\n",
					id,
					data & CS4245_PGA_GAIN_MASK);
			break;
		}

		case CS4245_ANALOG_IN:
		{
			switch(data & CS4245_SEL_MASK)
			{
				case CS4245_SEL_MIC:
					printf("%02x: PGA_SOFT: %d, PGA_ZERO: %d, INPUT: %s\r\n", id, data & CS4245_PGA_SOFT, data & CS4245_PGA_ZERO, "MIC");
					break;
				case CS4245_SEL_INPUT_1:
					printf("%02x: PGA_SOFT: %d, PGA_ZERO: %d, INPUT: %s\r\n", id, data & CS4245_PGA_SOFT, data & CS4245_PGA_ZERO, "LINE1");
					break;
				case CS4245_SEL_INPUT_2:
					printf("%02x: PGA_SOFT: %d, PGA_ZERO: %d, INPUT: %s\r\n", id, data & CS4245_PGA_SOFT, data & CS4245_PGA_ZERO, "LINE2");
					break;
				case CS4245_SEL_INPUT_3:
					printf("%02x: PGA_SOFT: %d, PGA_ZERO: %d, INPUT: %s\r\n", id, data & CS4245_PGA_SOFT, data & CS4245_PGA_ZERO, "LINE3");
					break;
				case CS4245_SEL_INPUT_4:
					printf("%02x: PGA_SOFT: %d, PGA_ZERO: %d, INPUT: %s\r\n", id, data & CS4245_PGA_SOFT, data & CS4245_PGA_ZERO, "LINE4");
					break;
				case CS4245_SEL_INPUT_5:
					printf("%02x: PGA_SOFT: %d, PGA_ZERO: %d, INPUT: %s\r\n", id, data & CS4245_PGA_SOFT, data & CS4245_PGA_ZERO, "LINE5");
					break;
				case CS4245_SEL_INPUT_6:
					printf("%02x: PGA_SOFT: %d, PGA_ZERO: %d, INPUT: %s\r\n", id, data & CS4245_PGA_SOFT, data & CS4245_PGA_ZERO, "LINE6");
					break;
			}
			break;
		}

		case CS4245_DAC_A_CTRL:
		{
			printf("%02x: DAC A VOL: %d\r\n",
					id,
					data & CS4245_VOL_MASK);
			break;
		}

		case CS4245_DAC_B_CTRL:
		{
			printf("%02x: DAC B VOL: %d\r\n",
					id,
					data & CS4245_VOL_MASK);
			break;
		}

		case CS4245_DAC_CTRL_2:
		{
			printf("%02x: DAC_SOFT: %d, DAC_ZERO: %d, INVERT_DAC: %d, ACTIVE_HIGH: %d\r\n",
					id,
					data & CS4245_DAC_SOFT,
					data & CS4245_DAC_ZERO,
					data & CS4245_INVERT_DAC,
					data & CS4245_INT_ACTIVE_HIGH);
			break;
		}

		default:
			break;
	}
}

static void codec_hw_show_registers(char *msg)
{
	uchar data[20];
	int i;

	printf("--------------------------\r\n");
	printf(msg);
	printf("--------------------------\r\n");

	// Dump registers
	for(i = 1; i < 0x10; i++)
	{
		if(shared_i2c_read_reg(0x98, i, (data + i), 1) != 0)
			break;

		//printf("%02x: %02x\r\n", i, data[i]);
		codec_hw_decode_register(data[i], i);
	}
}

static void codec_hw_update_register(uchar id, bool force, uchar value)
{
	uchar old;

	// Read current
	if(shared_i2c_read_reg(0x98, id, &old, 1) != BSP_ERROR_NONE)
		goto loc_err;

	// Modify
	if(!force)
		old |= value;
	else
		old = value;

	// Update value
	if(shared_i2c_write_reg(0x98, id, &old, 1) != BSP_ERROR_NONE)
		goto loc_err;

	return;

loc_err:
		printf("i2c error, not handled\r\n");
}

static void codec_hw_set_pga_gain(uchar val)
{
	if(val > (sizeof(pga_gain_tbl) - 1))
		return;

	// PGA gain: -12db, val = 40
	//			   0db, val = 0
	//			 +12dB, val = 24
	codec_hw_update_register(CS4245_PGA_B_CTRL, true, pga_gain_tbl[val]);
	codec_hw_update_register(CS4245_PGA_A_CTRL, true, pga_gain_tbl[val]);
}

// ToDo: check for I2C errors!
static void codec_hw_set_dac(void)
{
	uchar s_rate = 0;

	// Signal selection for AUX output(LINE OUT)
	codec_hw_update_register(CS4245_SIGNAL_SEL, true, CS4245_A_OUT_SEL_DAC);

	if(ts.samp_rate <= 50000)
		s_rate = CS4245_DAC_FM_SINGLE & CS4245_DAC_FM_MASK;
	else if(ts.samp_rate <= 100000)
		s_rate = CS4245_DAC_FM_DOUBLE & CS4245_DAC_FM_MASK;
	else
		s_rate = CS4245_DAC_FM_QUAD & CS4245_DAC_FM_MASK;

	// DAC clocking and frame mode
	codec_hw_update_register(CS4245_DAC_CTRL_1, true, s_rate | CS4245_DAC_DIF_I2S);

	// CS4245_INVERT_DAC
	codec_hw_update_register(CS4245_DAC_CTRL_2, true, CS4245_DAC_SOFT | CS4245_DAC_ZERO | CS4245_INVERT_DAC);

	// Set initial volume
	codec_hw_volume();
}

// ToDo: check for I2C errors!
static void codec_hw_set_adc(void)
{
	uchar s_rate = 0;

	if(ts.samp_rate <= 50000)
		s_rate = CS4245_ADC_FM_SINGLE & CS4245_ADC_FM_MASK;
	else if(ts.samp_rate <= 100000)
		s_rate = CS4245_ADC_FM_DOUBLE & CS4245_ADC_FM_MASK;
	else
		s_rate = CS4245_ADC_FM_QUAD & CS4245_ADC_FM_MASK;

	// ADC clocking and protocol mode
	codec_hw_update_register(CS4245_ADC_CTRL, true, s_rate | CS4245_ADC_DIF_I2S);

	//
	codec_hw_update_register(CS4245_ANALOG_IN, true, CS4245_PGA_SOFT | CS4245_PGA_ZERO);

	// PGA gain: -12db, val = 40
	//			   0db, val = 0
	//			 +12dB, val = 24
	//codec_hw_update_register(CS4245_PGA_B_CTRL, false, 0);
	//codec_hw_update_register(CS4245_PGA_A_CTRL, false, 0);
	codec_hw_set_pga_gain(24); // 0 dB
}

// Change DAC sampling rate
static void codec_hw_set_sampling_rate(void)
{
	if (ts.samp_rate <= 50000)
	{
		codec_hw_update_register(CS4245_MCLK_FREQ, true, 0);
	}
	else if (ts.samp_rate <= 100000)
	{
		codec_hw_update_register(CS4245_MCLK_FREQ, true, 0x22);
	}
	else
	{
		codec_hw_update_register(CS4245_MCLK_FREQ, false, 0x44);
	}
	//printf("sampling rate set to: %d Hz\r\n", ts.samp_rate);
}

void codec_hw_set_audio_route(uchar route_id)
{
	switch(route_id)
	{
		// RX mode
		// INPUT1 -> MUX -> PGA -> ADC -> SAI OUT
		// SAI IN -> DAC -> AUX OUT (TX Exciter power off)
		case CODEC_ROUTE_RX:
		{
			//printf("codec path - rx\r\n");

			// ****** INPUT ROUTING *******
			//
			// RX detector on INPUT1 to ADC/PGA
			codec_hw_update_register(CS4245_ANALOG_IN, true, CS4245_SEL_INPUT_1);

			// ****** OUTPUT ROUTING *******
			//
			// Signal selection for AUX output(LINE OUT + SPEAKER) to DAC
			codec_hw_update_register(CS4245_SIGNAL_SEL, true, CS4245_A_OUT_SEL_DAC);

			// ****** CODEC GAIN *******
			//
			// PGA gain: -12db, val = 40
			//			   0db, val = 0
			//			 +12dB, val = 24
			//codec_hw_update_register(CS4245_PGA_B_CTRL, true, 40);
			//codec_hw_update_register(CS4245_PGA_A_CTRL, true, 40);
			codec_hw_set_pga_gain(0); // -12 dB

			// ****** AUDIO VOLUME *******
			//
			// RX volume is DAC atten
			codec_hw_volume();

			break;
		}

		// TX mode, with Mic
		// INPUT4 -> GAIN AMP -> MUX -> PGA -> ADC -> SAI OUT
		// SAI IN -> DAC -> AOUT (TX Exciter power on)
		// CW mode, check docs
		case CODEC_ROUTE_TX:
		{
			//printf("codec path - tx\r\n");

			// Mic or digitally generated signal for DAC (CW, DIGI)
			//
			if((tsu.band[tsu.curr_band].demod_mode == DEMOD_CW)||(tsu.tune))
			{
				// RX detector on INPUT6 to PGA(ADC), side-tone tap
				codec_hw_update_register(CS4245_ANALOG_IN, true, CS4245_SEL_INPUT_6);

				// Signal selection for AUX output(LINE OUT + SPEAKER) to PGA
				codec_hw_update_register(CS4245_SIGNAL_SEL, true, CS4245_A_OUT_SEL_PGA);

				// ****** AUDIO VOLUME *******
				//
				codec_hw_set_pga_gain(0);
				tsu.band[tsu.curr_band].st_volume = 0;	// ToDo: Put physical attenuator on the TAP

				// ****** CODEC GAIN *******
				//
				// DAC attenuator:   0db, val = 0
				//			   	   -20db, val = 40
				//			 	  -127dB, val = 254
				codec_hw_update_register(CS4245_DAC_A_CTRL, true, 0);
				codec_hw_update_register(CS4245_DAC_B_CTRL, true, 0);
			}
			else
			{
				// Mic to  ADC/PGA
				codec_hw_update_register(CS4245_ANALOG_IN, true, CS4245_SEL_MIC);
			}

			break;
		}

		// Test speaker Amp
		case CODEC_ROUTE_LINEIN_SP:
		{
			// RX detector on INPUT1 to ADC
			codec_hw_update_register(CS4245_ANALOG_IN, false, CS4245_SEL_INPUT_2);

			break;
		}

		// ToDo:
		//		 TX mode + LINE IN (MIC OFF)
		// 		 RX mode + IQ to LINE OUT (ADC/DAC bypass)
		//		 some testing mode with custom routing ?

		// Not supported
		default:
			printf("route ID %d, not supported!\r\n", route_id);
			break;
	}

}

// context reset !
void codec_hw_init(void)
{
	GPIO_InitTypeDef  gpio_init_structure;

	gpio_init_structure.Pull  = GPIO_NOPULL;

	// BT Connect input line
	gpio_init_structure.Pin   = BT_CONNECT_STATUS;
	gpio_init_structure.Mode  = GPIO_MODE_INPUT;
	HAL_GPIO_Init(BT_CONNECT_STATUS_PORT, &gpio_init_structure);

	// Reset line is PC14
	gpio_init_structure.Pin   = CODEC_RESET;
	gpio_init_structure.Mode  = GPIO_MODE_OUTPUT_PP;
	gpio_init_structure.Speed = GPIO_SPEED_FREQ_HIGH;
	HAL_GPIO_Init(CODEC_RESET_PORT, &gpio_init_structure);

	// Keep in reset
	HAL_GPIO_WritePin(CODEC_RESET_PORT, CODEC_RESET, GPIO_PIN_RESET);

	// BT Power Control
	gpio_init_structure.Pin   = RFM_DIO2;
	gpio_init_structure.Mode  = GPIO_MODE_OUTPUT_PP;
	HAL_GPIO_Init(RFM_DIO2_PORT, &gpio_init_structure);

	// 5V on is PG10 - done in bsp.c
	//gpio_init_structure.Pin   = GPIO_PIN_10;
	//HAL_GPIO_Init(GPIOG, &gpio_init_structure);

	// Off on start
	//HAL_GPIO_WritePin(GPIOG, GPIO_PIN_10, GPIO_PIN_SET);

	// I2C init
	if(shared_i2c_init() != 0)
	{
		printf("i2c init err 1!\r\n");
		return;
	}

	// PTT line - moved to bps.c
	//gpio_init_structure.Pin   = GPIO_PIN_8;
	//HAL_GPIO_Init(GPIOI, &gpio_init_structure);

	// RX on start
	//HAL_GPIO_WritePin(PTT_PIN_PORT, PTT_PIN, GPIO_PIN_RESET);

	// 8.2 PCB has MUTE line
	#if 1
	gpio_init_structure.Pin   = CODEC_MUTE;
	HAL_GPIO_Init(CODEC_MUTE_PORT, &gpio_init_structure);
	HAL_GPIO_WritePin(CODEC_MUTE_PORT, CODEC_MUTE, GPIO_PIN_SET);	// unmute
	#endif
}

void codec_hw_power_cleanup(void)
{
	GPIO_InitTypeDef  gpio_init_structure;

	gpio_init_structure.Mode  = GPIO_MODE_OUTPUT_PP;
	gpio_init_structure.Pull  = GPIO_NOPULL;
	gpio_init_structure.Speed = GPIO_SPEED_FREQ_HIGH;

	// Reset line is PC14
	gpio_init_structure.Pin   = CODEC_RESET;
	HAL_GPIO_Init(CODEC_RESET_PORT, &gpio_init_structure);

	// Keep in reset
	HAL_GPIO_WritePin(CODEC_RESET_PORT, CODEC_RESET, GPIO_PIN_RESET);

	// 8.2 PCB has MUTE line
	#if 1
	gpio_init_structure.Pin   = CODEC_MUTE;
	HAL_GPIO_Init(CODEC_MUTE_PORT, &gpio_init_structure);
	HAL_GPIO_WritePin(CODEC_MUTE_PORT, CODEC_MUTE, GPIO_PIN_RESET);	// mute
	#endif

//!	BSP_I2C1_DeInit();
}

// context audio, after dsp core starts MCLK
void codec_hw_reset(void)
{
	//HAL_GPIO_WritePin(CODEC_RESET_PORT, CODEC_RESET, GPIO_PIN_RESET);
	vTaskDelay(100);
	HAL_GPIO_WritePin(CODEC_RESET_PORT, CODEC_RESET, GPIO_PIN_SET);
	vTaskDelay(100);
}

// context audio proc
void codec_task_init(void)
{
	uchar data[20];
	uchar val;
	int i;

	// Read chip ID - always read first, otherwise comms fail next
	if(shared_i2c_read_reg(0x98, CS4245_CHIP_ID, &val, 1) != 0)
	{
		printf("i2c init err 2!\r\n");
		return;
	}
	//printf("chip id: %02x, chip rev: %02x\r\n", val & CS4245_CHIP_PART_MASK, val & CS4245_CHIP_REV_MASK);

	// Debug
	//--codec_hw_show_registers("cs4245 registers on reset \r\n");

	// Set power state, clear PDN bit to start power up sequence
	codec_hw_update_register(CS4245_POWER_CTRL, true, 0);

	// Set Dac params
	codec_hw_set_dac();

	// Set ADC params
	codec_hw_set_adc();

	// Set sampling rate - not working, need to fix MCLK ratios
	codec_hw_set_sampling_rate();

	codec_hw_set_audio_route(CODEC_ROUTE_RX);

	// Debug
	//codec_hw_show_registers("cs4245 modified registers \r\n");

	// 5V on (Speaker amp)
	//HAL_GPIO_WritePin(GPIOG, GPIO_PIN_10, GPIO_PIN_RESET);

	//printf("codec_task_init ok \r\n");
}

//
void codec_hw_volume(void)
{
	if(tsu.rxtx == 0)
	{
		uchar attn_l, attn_r;
		uchar vol = tsu.band[tsu.curr_band].volume;			// current volume
		uchar bal = tsu.band[tsu.curr_band].audio_balance;	// current balance
		int factor_l, factor_r;

		//printf("-----------------\r\n");
		//printf("vol = %d, bal = %d\r\n", vol, bal);

		int range = (vol * 2);
		//printf("ran = %d\r\n", range);
		int base = (vol - range);
		//printf("bas = %d\r\n", base);
		int bal_ofs = (bal - 8);
		//printf("bof = %d\r\n", bal_ofs);

		if(bal_ofs == 0)
		{
			//printf("EQ\r\n");
			factor_l = 0;
			factor_r = 0;
		}
		else if(bal_ofs < 0)
		{
			//printf("< 0, %d:%d\r\n", bal_ofs, base);
			if(bal_ofs >= base)
			{
				 //printf("L\r\n");
				 factor_r = bal_ofs;
			}
			else
				factor_r = base;

			if(factor_r < 0)
				factor_r = -factor_r;

			factor_l = 0;
			//printf("factor R: %d\r\n", factor_r);
		}
		else
		{
			//printf("> 0, %d:%d\r\n", bal_ofs, (base + range));
			if((base + range) >= bal_ofs)
			{
				//printf("R\r\n");
				factor_l = bal_ofs;
			}
			else
				factor_l = (base + range);

			if(factor_l < 0)
				factor_l = -factor_l;

			factor_r = 0;
			//printf("factor L: %d\r\n", factor_l);
		}

		//printf("vol L = %d\r\n", (vol - factor_r));
		//printf("vol R = %d\r\n", (vol - factor_l));

		// DAC attenuator useful values 0 - 85, where 0 is the loudest
		attn_l = CODEC_VOL_MAX_ATTN - ((vol - factor_r) * CODEC_VOL_STEP);
		attn_r = CODEC_VOL_MAX_ATTN - ((vol - factor_l) * CODEC_VOL_STEP);

		//printf("dac L = %d\r\n", attn_l);
		//printf("dac R = %d\r\n", attn_r);

		// DAC attenuator:   0db, val = 0
		//			   	   -20db, val = 40
		//			 	  -127dB, val = 254
		codec_hw_update_register(CS4245_DAC_A_CTRL, true, attn_l);
		codec_hw_update_register(CS4245_DAC_B_CTRL, true, attn_r);

		// ****** OUTPUT ROUTING *******
		//
		// Signal selection for AUX output(LINE OUT + SPEAKER) to DAC
		//if(tsu.band[tsu.curr_band].volume)
		//	codec_hw_update_register(CS4245_SIGNAL_SEL, true, CS4245_A_OUT_SEL_DAC);
		//else
		//	codec_hw_update_register(CS4245_SIGNAL_SEL, true, CS4245_A_OUT_SEL_HIZ);	// disconnect AUX
	}
	else
	{
		// Side-tone volume control via PGA gain
		if(tsu.band[tsu.curr_band].demod_mode == DEMOD_CW)
		{
			uchar volg;

			if(tsu.band[tsu.curr_band].st_volume < 16)
				volg = (tsu.band[tsu.curr_band].st_volume * 3);
			else
				volg = 48;

			printf("pga gain id = %d\r\n", volg);

			// PGA gain: -12db, val = 40
			//			   0db, val = 0
			//			 +12dB, val = 24
			//codec_hw_update_register(CS4245_PGA_B_CTRL, true, pga_gain_tbl[volg]);
			//codec_hw_update_register(CS4245_PGA_A_CTRL, true, pga_gain_tbl[volg]);
			codec_hw_set_pga_gain(volg);
		}
	}
}
#endif
