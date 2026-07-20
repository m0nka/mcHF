/************************************************************************************
**                                                                                 **
**                                 mcHF QRP Transceiver                            **
**                         Krassi Atanassov - M0NKA, 2013-2026                     **
**                                                                                 **
**---------------------------------------------------------------------------------**
**                                                                                 **
**  File name:		marschat_ui.h                                                  **
**  Description:	MarsChat chat dialog - widget ids                             **
**  Licence:		https://github.com/m0nka/mcHF/blob/main/LICENSE                **
************************************************************************************/
#ifndef __MARSCHAT_UI_H
#define __MARSCHAT_UI_H

#define ID_WINDOW_0				(GUI_ID_USER + 0x00)
#define ID_LISTBOX_HISTORY		(GUI_ID_USER + 0x01)
#define ID_TEXT_STATUS			(GUI_ID_USER + 0x02)
#define ID_TEXT_DRAFT_SENT		(GUI_ID_USER + 0x03)
#define ID_TEXT_DRAFT_PENDING	(GUI_ID_USER + 0x04)

// 16 character buttons, A-P (row major), then 4 control buttons
#define ID_BUTTON_CHAR_0		(GUI_ID_USER + 0x10)
#define ID_BUTTON_SPACE			(GUI_ID_USER + 0x20)
#define ID_BUTTON_BACKSPACE		(GUI_ID_USER + 0x21)
#define ID_BUTTON_CLEAR			(GUI_ID_USER + 0x22)
#define ID_BUTTON_SEND			(GUI_ID_USER + 0x23)

#endif
