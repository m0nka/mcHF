/************************************************************************************
**                                                                                 **
**                                 mcHF QRP Transceiver                            **
**                         Krassi Atanassov - M0NKA, 2013-2026                     **
**                                                                                 **
**---------------------------------------------------------------------------------**
**                                                                                 **
**  File name:		vfo_mc_gen.h                                                   **
**  Description:	MarsChat 4-FSK generator on the Si5351 CLK1 test injector      **
**  Last Modified:                                                                 **
**  Licence:		https://github.com/m0nka/mcHF/blob/main/LICENSE                **
************************************************************************************/
//
// Internal loopback test rig, NOT a transmitter - CLK1 is attenuated into
// a short wire inside the radio (built for RX IMD testing), so the radio
// receives its own WSPR-rate 4-FSK stream while staying in RX. Used for
// emission-free bring-up of the whole MarsChat stack and as a permanent
// self test of the WSPR capture/decode path
//
// Timing: one WSPR symbol is 8192/12000 s = 682.667 ms; three symbols are
// exactly 2048 ms, so integer-ms deadlines carry no cumulative drift.
// Do not run together with the Demo Mode CW generator (shares CLK1)
//
#ifndef __VFO_MC_GEN_H
#define __VFO_MC_GEN_H

void	vfo_mc_gen_init(void);

// Start streaming - center_hz is the RF center of the tone group
// (dial + audio offset, e.g. dial + 1500), syms are 162 values 0..3
// Returns 0 ok, 1 busy/invalid
uchar	vfo_mc_gen_start(ulong center_hz, const uchar *syms, ushort nsym);

// Abort a running stream (carrier off)
void	vfo_mc_gen_stop(void);

// Stream in progress ?
uchar	vfo_mc_gen_active(void);

// Ticks until the next symbol deadline, for the vfo task sleep time
ulong	vfo_mc_gen_next_delay(void);

// Periodic process, called from the vfo task loop
void	vfo_mc_gen_proc(void);

#endif
