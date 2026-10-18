/************************************************************************************
**                                                                                 **
**                                 mcHF QRP Transceiver                            **
**                         Krassi Atanassov - M0NKA, 2013-2026                     **
**                                                                                 **
**---------------------------------------------------------------------------------**
**                                                                                 **
**  File name:		icc_ft8.h                                                      **
**  Description:	FT8 waterfall front end - ft8_lib monitor.c STFT on the M4,    **
**					rows streamed to the M7 core which runs the decoder            **
**  Licence:		https://github.com/m0nka/mcHF/blob/main/LICENSE                **
************************************************************************************/
#ifndef __ICC_FT8_H
#define __ICC_FT8_H

#include <stdint.h>

// ICC command handlers (superloop context)
uint8_t		icc_ft8_start(uint8_t source);
void		icc_ft8_stop(void);
uint16_t	icc_ft8_get_row(uint8_t *buffer);
uint8_t		icc_ft8_feed(const uint8_t *payload);

// Stop and drop buffered rows - the WSPR stream takes the shared RAM over
void		icc_ft8_release(void);

// Superloop worker - turns buffered live audio into waterfall rows
void		icc_ft8_thread(void);

// Rx audio tap - interleaved 16 bit stereo frames at 48 kHz, called from
// the SAI DMA block handler next to the WSPR tap. tx_mode != 0 feeds
// silence so the slot time base stays intact
void		icc_ft8_collect(volatile int16_t *src, uint32_t num_frames, uint32_t tx_mode);

#endif
