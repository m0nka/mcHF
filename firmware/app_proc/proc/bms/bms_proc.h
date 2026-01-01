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

	//ulong a[10];	// adc channel voltage
	//ulong s[5];		// branch voltage
	ulong c[5];		// calculated cell voltage
	//ulong e[5];		// channel errors, accumulated
	ulong t[5];		// cell temperature

	//ulong ba[5];	// balancer accumulator

	//ulong k[4];
	//ulong f[4];	// filtered values (long averaging)

	//ulong chgr;
	//ulong load;
	//ulong curr;

	//ulong t_err;	// total error, last cycle

	//ulong ulCH1;
	//ulong ulCH2;
	//ulong ulCH3;
	//ulong ulCH4;
	//ulong ulCH5;
	//ulong ulCH6;
	//ulong ulCH8;
	//ulong ulCH9;
	//ulong ulCH10;
	//ulong ulCH11;

	//ulong usBalID[4];
	//ulong usCV[4];
	//ulong usChargeValue;
	//ulong usLoadValue;

	//short cal_adc[10];	// calibration value for ADC mV trimming
	//short cal_res[10];	// calibration value for resistor divider trimming

	//short vref;

	// Reading ready
	uchar rr;

	//uchar lac;		// filter lenght

	uchar perc;			// % value of SOC left
	ushort mins;

	uchar charger_on;
	//uchar h_prot_on;
	uchar run_on_dc;

	uchar shutdown_req;

} BMSState;

void bms_proc_hw_init(void);
void bms_proc_power_cleanup(void);
void bms_proc_task(void const *arg);

#endif


