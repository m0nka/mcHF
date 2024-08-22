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
	.syntax unified
	.cpu cortex-m7
	.fpu softvfp
	.thumb

	.section		.text.get_pcb_rev
	.global			get_pcb_rev
	.type			get_pcb_rev, %function

get_pcb_rev:

			PUSH    {R1-R7,LR}
			
			ldr		r0, fls_const_hw_def
			ldrh   	r0, [r0,#0x00]
			b		exit_get_pcb_rev

fls_const_hw_def:	.word   0x0800FFF0

exit_get_pcb_rev:
			POP     {R1-R7,PC}

	.size	get_pcb_rev,	.-get_pcb_rev
