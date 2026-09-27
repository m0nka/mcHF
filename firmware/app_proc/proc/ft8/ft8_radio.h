/************************************************************************************
**                                                                                 **
**                                 mcHF QRP Transceiver                            **
**                         Krassi Atanassov - M0NKA, 2013-2026                     **
**                                                                                 **
**---------------------------------------------------------------------------------**
**                                                                                 **
**  File name:		ft8_radio.h                                                    **
**  Description:	FT8 mode radio setup - dial, mode and filter per band          **
**  Last Modified:                                                                 **
**  Licence:		https://github.com/m0nka/mcHF/blob/main/LICENSE                **
************************************************************************************/
//
// The FT8 screen owns the HF radio (proc/hf_app). On entry it tunes the
// band the radio is on to that band's FT8 dial frequency, USB, 3.6 kHz
// filter - after a reflash the VFO can sit anywhere in the last saved band,
// so this is not optional. BAND steps through the bands that have an FT8
// frequency. On exit every band touched gets its own VFO, mode and filter
// back and the radio returns to the band it was on.
//
#ifndef __FT8_RADIO_H
#define __FT8_RADIO_H

// FT8 dial frequency (USB carrier) of a radio band, 0 = none
ulong		ft8_radio_band_dial(uchar band);

// Short band name ("40M"), "" when the band has no FT8 frequency
const char	*ft8_radio_band_name(uchar band);

// Mode switch hooks and the BAND button. CONTEXT_VIDEO (gui task)
void		ft8_radio_enter(void);
void		ft8_radio_exit(void);
void		ft8_radio_next_band(void);

#endif
