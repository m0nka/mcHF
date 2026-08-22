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

// Significant charge current threshold to put the fan on
//
// We have two threshold points as we turn off the fan while
// the charger is in CV mode, and current can go up on fan off
// thus creating a bif of a feedback loop
//
#define PACK_CURR_CHARGE_ON					1350
#define PACK_CURR_CHARGE_OFF				1300

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

	// Gold file backup/flash progress(see bms_gold.h)
	uchar  gold_state;
	uchar  gold_perc;
	uchar  gold_err;
	ushort gold_line;

	// USB-PD
	ushort max_curr;
	uchar  usbpd_status;

	// Charger publics
	ushort ch_stat;
	ushort ch_chv;
	ushort ch_dcv;
	ushort ch_curr;
	ushort ch_vsys;
	ushort ch_vbat;
	ushort ch_vbus;

} BMSState;

void bms_proc_hw_init(void);
void bms_proc_power_cleanup(void);
void bms_proc_task(void const *arg);

#endif


