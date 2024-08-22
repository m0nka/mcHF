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
#ifndef __TOUCH_PROC_H
#define __TOUCH_PROC_H

// 0x5C (0x5D)
#define GT911_I2C_ADDRESS 		0xBA
//#define GT911_I2C_ADDRESS 	0x28

__attribute__((__common__)) struct GT911_CONFIG {

	ushort	x_max;
	ushort 	y_max;

	uchar 	numtouch_max;
	uchar 	switch1;
	uchar 	switch2;
	uchar 	shake_count;

	uchar 	filter;
	uchar 	large_touch;
	uchar 	noise_reduction;
	uchar 	touch_level;

	uchar 	leave_level;

} GT911_CONFIG;

__attribute__((__common__)) struct GT911_TP {

    ushort x;
    ushort y;
    ushort w;
    ushort id;

} GT911_TP;

__attribute__((__common__)) struct GT911_TS {

    uchar 				status;
    uchar 				tid;
    struct GT911_TP 	t[5];

} GT911_TS;

void touch_proc_irq(void);
void touch_proc_hw_init(void);
void touch_proc_power_cleanup(void);
void touch_proc_task(void const *argument);

#endif
