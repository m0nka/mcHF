/************************************************************************************
**                                                                                 **
**                                 mcHF QRP Transceiver                            **
**                         Krassi Atanassov - M0NKA, 2013-2026                     **
**                                                                                 **
**---------------------------------------------------------------------------------**
**                                                                                 **
**  File name:		mc_tx_build.h                                                  **
**  Description:	ICC_MC_TX_START payload builder - raw 50 bit frame to the      **
**					wire payload the M4 symbol streamer consumes (162 packed       **
**					symbols + optional morse CW id element bitstream).             **
**					No OS or HAL dependencies - host buildable for the test rig    **
**  Licence:		https://github.com/m0nka/mcHF/blob/main/LICENSE                **
************************************************************************************/
#ifndef __MC_TX_BUILD_H
#define __MC_TX_BUILD_H

#include <stdint.h>

// Wire payload dimensioning (layout in common/mchf_icc_def.h:
// 6 byte header + 41 symbol bytes + up to 32 CW bitstream bytes)
#define MC_TX_PAYLOAD_MAX		(6 + 41 + 32)

// Build the ICC_MC_TX_START payload
//   bits50   packed 50 bit frame (7 bytes, MSB first - mc_frame_pack output)
//   tone_hz  audio tone base, e.g. 1500
//   cw_call  callsign for the CW id, NULL or "" = no CW id segment
//   cw_wpm   CW id speed, words per minute (e.g. 25)
//   out      payload buffer, at least MC_TX_PAYLOAD_MAX bytes
//   out_len  filled with the payload length
// Returns 0 ok, 1 bad argument, 2 callsign has a non-morse character
// or is too long for the wire element budget
int mc_tx_build_payload(const uint8_t bits50[7], uint16_t tone_hz,
						const char *cw_call, uint8_t cw_wpm,
						uint8_t *out, uint16_t *out_len);

// Morse element bitstream for a text (exposed for the host test rig):
// one bit per element unit, bit = tone on; letters spaced by 3 off
// units, symbols inside a letter by 1, dot = 1 on, dash = 3 on.
// Returns the element count, or -1 on a non-morse character / overflow
int mc_tx_cw_elements(const char *text, uint8_t *bits, int max_elem);

// On-air duration of a built payload (mc_tx_build_payload output), in
// milliseconds - symbols + gap + CW id elements + tail ramp, matching
// the M4 streamer's timing exactly (baseband icc_mc_tx.c). Lets a
// caller know how long the exciter will stay keyed without needing a
// completion message back from the M4
uint32_t mc_tx_build_duration_ms(const uint8_t *payload);

#endif
