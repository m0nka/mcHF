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
#ifndef __MENU_BATT_H
#define __MENU_BATT_H

#define	BTIMER_PERIOD				1000

#define ID_WINDOW_0               	(GUI_ID_USER + 0x00)
#define ID_MULTIPAGE_0     			(GUI_ID_USER + 0x01)
#define ID_LISTVIEW					(GUI_ID_USER + 0x02)
#define ID_RADIO_0         			(GUI_ID_USER + 0x03)
#define ID_RADIO_1         			(GUI_ID_USER + 0x04)

#define SOPTS						SLIDER_CF_HORIZONTAL

#define BATT_MAX_COLUMN				8

#define COLUMN_ADC_CH				0
#define COLUMN_DESCR				1
#define COLUMN_INPUT				2
#define COLUMN_ADJ					3
#define COLUMN_CALC					4
#define COLUMN_TRUNC				5
#define COLUMN_CAL1					6
#define COLUMN_CAL2					7

#define BATT_MAX_ROW				11

#define ROW_CELL1					0
#define ROW_CELL2					1
#define ROW_CELL3					2
#define ROW_CELL4					3
#define ROW_BLOAD					4
#define ROW_DCIN					5
#define ROW_BCURR					6
#define ROW_CELL1T					7
#define ROW_CELL2T					8
#define ROW_CELL3T					9
#define ROW_CELL4T					10

#endif
