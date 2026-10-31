/************************************************************************************
**                                                                                 **
**                                 mcHF QRP Transceiver                            **
**                         Krassi Atanassov - M0NKA, 2013-2026                     **
**                                                                                 **
**---------------------------------------------------------------------------------**
**                                                                                 **
**  File name:		wspr_ui.h                                                      **
**  Description:	WSPR app screen - plain emWin widgets, owns the display for    **
**					as long as MODE_DESKTOP_WSPR is the current UI state           **
**  Licence:		https://github.com/m0nka/mcHF/blob/main/LICENSE                **
************************************************************************************/
#ifndef __WSPR_UI_H
#define __WSPR_UI_H

void wspr_ui_create(void);
void wspr_ui_destroy(void);

#endif
