/************************************************************************************
**                                                                                 **
**                                 mcHF QRP Transceiver                            **
**                         Krassi Atanassov - M0NKA, 2013-2026                     **
**                                                                                 **
**---------------------------------------------------------------------------------**
**                                                                                 **
**  File name:		icc_hf_ram.h                                                   **
**  Description:	Working RAM shared by the mutually exclusive HF digital mode   **
**					streams (WSPR capture tap, FT8 waterfall front end)            **
**  Licence:		https://github.com/m0nka/mcHF/blob/main/LICENSE                **
************************************************************************************/
//
// M4 counterpart of the M7 HF app arena (app_proc proc/hf_app). Only one HF
// digital mode owns the radio at a time, so their big buffers overlay one
// another here. Each stream keeps its own run state OUTSIDE the arena (an
// irq checks it) and stops the other stream before it takes the arena over.
//
#ifndef __ICC_HF_RAM_H
#define __ICC_HF_RAM_H

#include <stdint.h>

// Largest user: the FT8 STFT front end (~83 KB). WSPR needs 16 KB
#define ICC_HF_RAM_SIZE				(84 * 1024)

extern uint8_t		icc_hf_ram[ICC_HF_RAM_SIZE];

#endif
