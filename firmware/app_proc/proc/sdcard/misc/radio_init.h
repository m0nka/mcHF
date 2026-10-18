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
#ifndef __RADIO_INIT_H
#define __RADIO_INIT_H

#define	AGC_SLOW			0		// Mode setting for slow AGC
#define	AGC_MED				1		// Mode setting for medium AGC
#define	AGC_FAST			2		// Mode setting for fast AGC
#define	AGC_CUSTOM			3		// Mode setting for custom AGC
#define	AGC_OFF				4		// Mode setting for AGC off
#define	AGC_MAX_MODE		4		// Maximum for mode setting for AGC
#define	AGC_DEFAULT			AGC_MED	// Default!

#include "mchf_dsp_settings.h"

// UHSDR DSP settings (Baseband menu), sent to the M4 core by the ICC
// task whenever dsp_settings_dirty is set
extern short			dsp_settings[DSP_SET_COUNT];
extern volatile uchar	dsp_settings_dirty;
extern const short		dsp_settings_def[DSP_SET_COUNT];
extern const short		dsp_settings_min[DSP_SET_COUNT];
extern const short		dsp_settings_max[DSP_SET_COUNT];

short radio_init_dsp_setting_clamp(uchar id, short val);
void radio_init_dsp_settings_save(void);

void radio_init_ui_to_dsp(void);
void radio_init_eep_defaults(void);
uchar radio_init_default_mode_from_band(void);
void radio_init_show_current_demod_mode(uchar mode);

void radio_init_save_before_off(void);
void radio_init_on_reset(void);

#endif
