/************************************************************************************
**                                                                                 **
**                                 mcHF QRP Transceiver                            **
**                         Krassi Atanassov - M0NKA, 2013-2026                     **
**                                                                                 **
**---------------------------------------------------------------------------------**
**                                                                                 **
**  File name:		icc_mc_tx.h                                                    **
**  Description:	MarsChat/WSPR 4-FSK symbol tx streamer, M4 core                **
**  Licence:		https://github.com/m0nka/mcHF/blob/main/LICENSE                **
************************************************************************************/
#ifndef __ICC_MC_TX_H
#define __ICC_MC_TX_H

#include <stdint.h>

// Wire protocol entry points (called from the icc command dispatcher,
// payload layout in common/mchf_icc_def.h). Start returns 0 = accepted
uint8_t		icc_mc_tx_start(const uint8_t *payload);
void		icc_mc_tx_stop(void);

// TX audio path hook - true while the streamer owns the tx iq buffers
uint8_t		icc_mc_tx_active(void);

// Generate one iq block (called from the SAI tx interrupt path while
// active; for USB sideband pass i = q_buffer, q = i_buffer like the
// CW/tune generator does). Returns 1 = signal generated
uint8_t		icc_mc_tx_gen(float *i_buff, float *q_buff, uint16_t block_size);

// TX exciter key requests, consumed by the icc superloop idle thread:
// 1 = key tx, 2 = unkey (stream done or stopped), 0 = nothing pending
uint8_t		icc_mc_tx_key_request(void);

#endif
