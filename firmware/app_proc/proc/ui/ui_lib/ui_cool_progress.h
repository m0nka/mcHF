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
#ifndef __UI_COOL_PROGRESS_H
#define __UI_COOL_PROGRESS_H

#define COOL_PROG_WIDTH	5
#define COOL_PROG_SIZE	25

void ui_cool_progress_tx_pwr(int x, int y, ushort val, char *txt);
void ui_cool_progress_volume(int x, int y, ushort val, char *txt);
void ui_cool_progress_gain(int x, int y, ushort val, char *txt);

#endif
