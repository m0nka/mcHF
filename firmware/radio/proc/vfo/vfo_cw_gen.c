/************************************************************************************
**                                                                                 **
**                             mcHF Pro QRP Transceiver                            **
**                         Krassi Atanassov - M0NKA, 2013-2021                     **
**                                                                                 **
**---------------------------------------------------------------------------------**
**                                                                                 **
**                                                                                 **
**  File name:      vfo_cw_gen.c                                                   **
**  Description:    Two tones CW generator                                         **
**  Date Created:   25 Aug 2021, The Idle Residence, Thailand                      **
**  Licence:                                                                       **
**          The mcHF project is released for radio amateurs experimentation,       **
**          non-commercial use only. All source files under GPL-3.0, unless        **
**          third party drivers specifies otherwise. Thank you!                    **
************************************************************************************/

#include "mchf_pro_board.h"

#ifdef CONTEXT_VFO

// Major inspiration for this implementation is from the 2010 article by
// Dr.Walt Fair, PE
// https://www.codeproject.com/Articles/85339/Morse-Code-Generation-from-Text

#include "si5351.h"
#include "vfo_cw_gen.h"

// Symbol table
const struct VFO_CW_GEN_CHARS cw_tbl[] = {

		{'A', 2, 0b01000000},
		{'B', 4, 0b10000000},
		{'C', 4, 0b10100000},
		{'D', 3, 0b10000000},
		{'E', 1, 0b00000000},
		{'F', 4, 0b00100000},
		{'G', 3, 0b11000000},
		{'H', 4, 0b00000000},
		{'I', 2, 0b00000000},
		{'J', 4, 0b01110000},
		{'K', 3, 0b10100000},
		{'L', 4, 0b01000000},
		{'M', 2, 0b11000000},
		{'N', 2, 0b10000000},
		{'O', 3, 0b11100000},
		{'P', 4, 0b01100000},
		{'Q', 4, 0b11010000},
		{'R', 3, 0b01000000},
		{'S', 3, 0b00000000},
		{'T', 1, 0b10000000},
		{'U', 3, 0b00100000},
		{'V', 4, 0b00010000},
		{'W', 3, 0b01100000},
		{'X', 4, 0b10010000},
		{'Y', 4, 0b10110000},
		{'Z', 4, 0b11000000},
		{'0', 5, 0b11111000},
		{'1', 5, 0b01111000},
		{'2', 5, 0b00111000},
		{'3', 5, 0b00011000},
		{'4', 5, 0b00001000},
		{'5', 5, 0b00000000},
		{'6', 5, 0b10000000},
		{'7', 5, 0b11000000},
		{'8', 5, 0b11100000},
		{'9', 5, 0b11110000},
		{'.', 6, 0b01010100},
		{',', 6, 0b11001100},
		{'/', 5, 0b10010000},
		{'-', 5, 0b10001000},
		{'~', 5, 0b01010000},
		{'?', 6, 0b00110000},
		{'@', 5, 0b00101000}
};

// CW state machine instance
struct VFO_CW_STATE cw_s[2];

//*----------------------------------------------------------------------------
//* Function Name       : vfo_cw_gen_symbol_reset
//* Object              :
//* Notes    			: next symbol
//* Notes   			:
//* Notes    			:
//* Context    			: CONTEXT_VFO
//*----------------------------------------------------------------------------
static void vfo_cw_gen_symbol_reset(uchar vfo_id, uchar id)
{
	uchar i;

	//printf("%d: %c\r\n", vfo_id, cw_s[vfo_id].text[id]);

	// Look for match
	for(i = 0; i < sizeof(cw_tbl); i++)
	{
		if(cw_s[vfo_id].text[id] == cw_tbl[i].code)
			break;
	}

	if(i == sizeof(cw_tbl))
	{
		//printf("no match\r\n");
		return;
	}

	cw_s[vfo_id].symbol	= cw_tbl[i].symbol;
	cw_s[vfo_id].e_size	= cw_tbl[i].size;
	cw_s[vfo_id].e_gap	= 0;
}

//*----------------------------------------------------------------------------
//* Function Name       : vfo_cw_gen_symbol
//* Object              :
//* Notes    			: send symbol state machine
//* Notes   			:
//* Notes    			:
//* Context    			: CONTEXT_VFO
//*----------------------------------------------------------------------------
static void vfo_cw_gen_symbol(uchar vfo_id)
{
	if(cw_s[vfo_id].e_gap == 0)							// == elements ==
	{
		if(vfo_id == 0)
			Si5351_output_enable(SI5351_CLK1, 1);		// carrier on
		else
			Si5351_output_enable(SI5351_CLK2, 1);

		if((cw_s[vfo_id].symbol & 0x80) != 0x80)
			cw_s[vfo_id].e_gap = 1;						// dit
		else
			cw_s[vfo_id].e_gap = 3;						// dah
	}
	else												// == gaps ==
	{
		if(--(cw_s[vfo_id].e_gap) == 0)
		{
			if(vfo_id == 0)
				Si5351_output_enable(SI5351_CLK1, 0);	// carrier off
			else
				Si5351_output_enable(SI5351_CLK2, 0);

			cw_s[vfo_id].symbol = cw_s[vfo_id].symbol << 1;
			cw_s[vfo_id].e_size--;
		}
	}
}

//*----------------------------------------------------------------------------
//* Function Name       : vfo_cw_gen_string
//* Object              :
//* Notes    			: send string state machine
//* Notes   			:
//* Notes    			:
//* Context    			: CONTEXT_VFO
//*----------------------------------------------------------------------------
void vfo_cw_gen_string(uchar vfo_id)
{
	if(cw_s[vfo_id].on == 0)
		return;

	// Process inter symbol gaps
	if(cw_s[vfo_id].t_gap)
	{
		cw_s[vfo_id].t_gap--;
		return;
	}

	// Call symbol state machine
	if(cw_s[vfo_id].e_size)
	{
		vfo_cw_gen_symbol(vfo_id);
		return;
	}

	// String sent, long gap, then restart
	if(cw_s[vfo_id].curr == cw_s[vfo_id].t_size)
	{
		//printf("string %d sent\r\n", vfo_id);
		cw_s[vfo_id].t_gap	= 100;
		cw_s[vfo_id].curr	= 0;				// start from first
		cw_s[vfo_id].e_size	= 0;				// prevent TX
		return;
	}

	// Load next symbol
	if((cw_s[vfo_id].e_size == 0) && (cw_s[vfo_id].curr < cw_s[vfo_id].t_size))
	{
		if(cw_s[vfo_id].text[cw_s[vfo_id].curr] == 0x20)
			cw_s[vfo_id].t_gap = 7;	// insert gap between words
		else
		{
			vfo_cw_gen_symbol_reset(vfo_id, cw_s[vfo_id].curr);

			if(cw_s[vfo_id].curr > 0)
				cw_s[vfo_id].t_gap = 3;	// insert gap between symbols(but not on first)
		}

		// Next symbol(on next reset)
		(cw_s[vfo_id].curr)++;
	}
}

//*----------------------------------------------------------------------------
//* Function Name       : vfo_cw_gen_proc
//* Object              :
//* Notes    			: cw periodic process
//* Notes   			:
//* Notes    			:
//* Context    			: CONTEXT_VFO
//*----------------------------------------------------------------------------
void vfo_cw_gen_proc(void)
{
	vfo_cw_gen_string(0);
	vfo_cw_gen_string(1);
}

//*----------------------------------------------------------------------------
//* Function Name       : vfo_cw_gen_start
//* Object              :
//* Notes    			: start carrier instance
//* Notes   			:
//* Notes    			:
//* Context    			: CONTEXT_VFO
//*----------------------------------------------------------------------------
void vfo_cw_gen_start(uchar vfo_id, ulong freq, char *text)
{
	// Need valid string to TX
	if((text == NULL)||(strlen(text) == 0))
		return;

	uint64_t f = (uint64_t)freq * 100ULL;

	switch(vfo_id)
	{
		case 0:
			Si5351_set_freq(f, SI5351_CLK1);
			break;
		case 1:
			Si5351_set_freq(f, SI5351_CLK2);
			break;
		default:
			return;
	}

	// Copy text
	memset(cw_s[vfo_id].text, 0, sizeof(cw_s[vfo_id].text));
	strcpy(cw_s[vfo_id].text, text);

	// Init publics
	cw_s[vfo_id].t_size = strlen(text);
	cw_s[vfo_id].t_gap	= 0;				// no gaps on start
	cw_s[vfo_id].curr	= 0;				// start from first
	cw_s[vfo_id].e_size = 0;				// prevent TX
	cw_s[vfo_id].on 	= 1;				// enable instance
}

//*----------------------------------------------------------------------------
//* Function Name       : vfo_cw_gen_init
//* Object              :
//* Notes    			:
//* Notes   			:
//* Notes    			:
//* Context    			: CONTEXT_VFO
//*----------------------------------------------------------------------------
void vfo_cw_gen_init(void)
{
	for(int i = 0; i < 2; i++)
	{
		cw_s[i].t_size 	= 0;
		cw_s[i].t_gap  	= 0;
		cw_s[i].curr   	= 0;
		cw_s[i].e_size 	= 0;
		cw_s[i].symbol 	= 0;
		cw_s[i].e_gap 	= 0;
		cw_s[i].on 		= 0;
	}
}

#endif
