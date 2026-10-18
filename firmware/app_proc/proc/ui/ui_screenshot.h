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
#ifndef __UI_SCREENSHOT_H
#define __UI_SCREENSHOT_H

// Screenshots go here as scr_NNNN.bmp, 800x480 24 bpp
#define SCREENSHOT_DIR				"0://screens"

void ui_screenshot_request(void);
void ui_screenshot_poll(void);

#endif
