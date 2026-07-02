/************************************************************************************
**                                                                                 **
**                                 mcHF QRP Transceiver                            **
**                         Krassi Atanassov - M0NKA, 2013-2026                     **
**                                                                                 **
**---------------------------------------------------------------------------------**
**                                                                                 **
**  File name:		icc_spectrum.h                                                 **
**  Description:	self contained 2048 point FFT spectrum/waterfall processor    **
**					for the M7 UI, output format identical to the CLINT project   **
**  Licence:		https://github.com/m0nka/mcHF/blob/main/LICENSE                **
************************************************************************************/
#ifndef __ICC_SPECTRUM_H
#define __ICC_SPECTRUM_H

#include <stdint.h>
#include "icc_radio_if.h"

// Init FFT instances and publics - called on ICC_START_I2S_PROC
void	icc_spectrum_init(void);

// Load display processing settings from the wire state
void	icc_spectrum_apply_settings(const icc_radio_settings_t *st);

// Collect raw IQ samples, called from the audio DMA block handler (IRQ context).
// src points to interleaved 16 bit L/R (I/Q) data, num_frames = stereo frames
void	icc_spectrum_collect(volatile int16_t *src, uint32_t num_frames);

// Background FFT processing, called from the superloop. Notifies the
// M7 core via HSEM when a new spectrum frame is ready for collection
void	icc_spectrum_thread(void);

// Fill the ICC broadcast packet (10 byte header + 1024 bins), returns size (1034)
uint16_t icc_spectrum_get_buffer(uint8_t *buffer);

// S-meter value (1..34) shown by the M7 UI, updated by the glue code
void	icc_spectrum_set_smeter(uint8_t s_value);

#endif
