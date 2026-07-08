/************************************************************************************
**                                                                                 **
**                                 mcHF QRP Transceiver                            **
**                         Krassi Atanassov - M0NKA, 2013-2026                     **
**                                                                                 **
**---------------------------------------------------------------------------------**
**                                                                                 **
**  File name:		icc_wspr.h                                                     **
**  Description:	WSPR rx audio capture streaming to the M7 core                 **
**  Licence:		https://github.com/m0nka/mcHF/blob/main/LICENSE                **
************************************************************************************/
#ifndef __ICC_WSPR_H
#define __ICC_WSPR_H

#include <stdint.h>

// ICC command handlers (superloop context)
void		icc_wspr_start(void);
void		icc_wspr_stop(void);
uint16_t	icc_wspr_get_buffer(uint8_t *buffer);

// Rx audio tap - interleaved 16 bit stereo frames at 48 kHz, called from
// the SAI DMA block handler. tx_mode != 0 substitutes silence to keep
// the capture time base intact while transmitting
void		icc_wspr_collect(volatile int16_t *src, uint32_t num_frames, uint32_t tx_mode);

#endif
