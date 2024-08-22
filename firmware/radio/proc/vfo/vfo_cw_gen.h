/************************************************************************************
**                                                                                 **
**                             mcHF Pro QRP Transceiver                            **
**                         Krassi Atanassov - M0NKA, 2013-2021                     **
**                                                                                 **
**---------------------------------------------------------------------------------**
**                                                                                 **
**  File name:                                                                     **
**  Description:                                                                   **
**  Last Modified:                                                                 **
**  Licence:       The mcHF project is released for radio amateurs experimentation **
**               and non-commercial use only.Check 3rd party drivers for licensing **
************************************************************************************/
#ifndef __VFO_CW_GEN_H
#define __VFO_CW_GEN_H

// Min symbol resolution in mS
#define CW_DIT_RESOLUTION		50

__attribute__((__common__)) struct VFO_CW_GEN_CHARS {

	// Ascii code
	char	code;

	// Number of valid bits
	uchar	size;

	// Symbol bits
	uchar	symbol;

} VFO_CW_GEN_CHARS;

// dit						dt
// inter-symbol space		dt
// dah						3*dt
// inter-character space	3*dt
// inter-word space			7*dt
//
__attribute__((__common__)) struct VFO_CW_STATE {

	// Symbol vars
	char	e_gap;
	uchar	e_size;
	uchar	symbol;

	// Text publics
	char 	text[32];
	uchar	t_size;
	uchar	curr;
	uchar	t_gap;

	uchar 	on;

} VFO_CW_STATE;

void vfo_cw_gen_proc(void);
void vfo_cw_gen_start(uchar vfo_id, ulong freq, char *text);
void vfo_cw_gen_init(void);

#endif



