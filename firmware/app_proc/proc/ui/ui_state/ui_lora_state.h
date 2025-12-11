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
#ifndef __UI_LORA_STATE_H
#define __UI_LORA_STATE_H

struct UI_LORA_STATE{

	uchar power_on;
	uchar last_event;

	ushort force_ui_repaint;
};

// Event types for event callback
enum _ev_t { 	EV_SCAN_TIMEOUT=1,
				EV_BEACON_FOUND,
				EV_BEACON_MISSED,
				EV_BEACON_TRACKED,
				EV_JOINING,
				EV_JOINED,
				EV_RFU1,
				EV_JOIN_FAILED,
				EV_REJOIN_FAILED,
				EV_TXCOMPLETE,
				EV_LOST_TSYNC,
				EV_RESET,
				EV_RXCOMPLETE,
				EV_LINK_DEAD,
				EV_LINK_ALIVE,
				EV_SCAN_FOUND,
				EV_TXSTART,
				EV_TXCANCELED,
				EV_RXSTART,
				EV_JOIN_TXCOMPLETE };

typedef enum _ev_t ev_t;

void  ui_lora_state_init(void);
uchar ui_lora_str_state(ev_t state, char *text);

#endif
