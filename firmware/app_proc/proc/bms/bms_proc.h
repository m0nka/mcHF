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
#ifndef __BMS_PROC_H
#define __BMS_PROC_H

// Somewhat this value should allow to detect if the radio
// is running on dc or batteries
#define PACK_CURR_THRSH						-50

__attribute__((__common__)) struct BMSState {

	// read cell voltage
	ulong c[5];

	// read cell temperature
	ulong t[5];

	short curr;
	ulong pack_v;

	// Reading ready
	uchar rr;

	// % value of SOC left
	uchar  perc;
	ushort mins;

	uchar charger_on;
	uchar run_on_dc;

	uchar shutdown_req;

	// Seal/Unseal status
	uchar bms_unlock_state;

} BMSState;

void bms_proc_hw_init(void);
void bms_proc_power_cleanup(void);
void bms_proc_task(void const *arg);

#endif


