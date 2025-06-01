/************************************************************************************
**                                                                                 **
**                             mcHF Pro QRP Transceiver                            **
**                         Krassi Atanassov - M0NKA 2012-2019                      **
**                            mail: djchrismarc@gmail.com                          **
**                                 twitter: @bph_co                                **
**---------------------------------------------------------------------------------**
**                                                                                 **
**  File name:                                                                     **
**  Description:                                                                   **
**  Last Modified:                                                                 **
**  Licence:                                                                       **
**          The mcHF project is released for radio amateurs experimentation,       **
**          non-commercial use only. All source files under GPL-3.0, unless        **
**          third party drivers specifies otherwise. Thank you!                    **
************************************************************************************/
#ifndef __UI_PROC_H
#define __UI_PROC_H

// Disable individual controls
#define 	SPECTRUM_WATERFALL
#define 	VFO_BOTH

#define		DESKTOP_SMETER			0
#define		DESKTOP_SPECTRUM		1
#define		DESKTOP_WATERFALL		2

// ----------------------------------------------

void ui_proc_clear_active(void);
void ui_proc_power_cleanup(void);
void ui_proc_task(void const *arg);

#endif
