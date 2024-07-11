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
#include "mchf_pro_board.h"

#include "ui_lora_state.h"

struct UI_LORA_STATE uls;

void ui_lora_state_init(void)
{
	uls.power_on 			= 0;
	uls.last_event 			= 0;
	uls.force_ui_repaint 	= 0;
}

uchar ui_lora_str_state(ev_t state, char *text)
{
	if(text == NULL)
		return 1;

	switch(state)
	{
		case EV_JOINING:
			strcpy(text,"JOIN");
			break;
		case EV_JOINED:
			strcpy(text,"JOINED");
			break;
		case EV_TXCOMPLETE:
			strcpy(text,"IDLE");
			break;
		case EV_TXSTART:
			strcpy(text,"TX");
			break;
		default:
			strcpy(text,"UNK");
			break;
	}

	return 0;
}


