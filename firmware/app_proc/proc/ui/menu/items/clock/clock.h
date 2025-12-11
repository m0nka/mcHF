/************************************************************************************
**                                                                                 **
**                             mcHF Pro QRP Transceiver                            **
**                         Krassi Atanassov - M0NKA, 2013-2025                     **
**                                                                                 **
**---------------------------------------------------------------------------------**
**                                                                                 **
**  File name:                                                                     **
**  Description:                                                                   **
**  Last Modified:                                                                 **
**  Licence:               GNU GPLv3                                               **
************************************************************************************/
#ifndef __CLOCK_H
#define __CLOCK_H

#define PI                  	3.14
#define AA_FACTOR           	3

#define CALENDAR_X             	5
#define CALENDAR_Y              30

#define AN_CLOCK_Y              360

// --------------------------------------------

#define ID_WINDOW_0               	(GUI_ID_USER + 0x00)
//#define ID_BUTTON_EXIT            	(GUI_ID_USER + 0x01)

#define ID_SPINBOX_HOUR          	(GUI_ID_USER + 0x02)
#define ID_SPINBOX_MINUTE        	(GUI_ID_USER + 0x03)
#define ID_BUTTON_APPLY			  	(GUI_ID_USER + 0x04)
#define ID_CALENDAR              	(GUI_ID_USER + 0x05)
#define ID_SPINBOX_SEC           	(GUI_ID_USER + 0x06)
#define ID_TEXT_SPIN_0             	(GUI_ID_USER + 0x07)
#define ID_BUTTON_LOCK			  	(GUI_ID_USER + 0x08)

#ifndef PCB_V9_REV_A
#define CLK_X			854
#define SP0X			485
#define SP1X			610
#define SP2X			735
#define AN_CLOCK_X      660
#else
#define CLK_X			800
#define SP0X			431
#define SP1X			556
#define SP2X			681
#define AN_CLOCK_X      606
#endif

#endif
