/************************************************************************************
**                                                                                 **
**                                 mcHF QRP Transceiver                            **
**                         Krassi Atanassov - M0NKA, 2013-2026                     **
**                                                                                 **
**---------------------------------------------------------------------------------**
**                                                                                 **
**  File name:		icc_hf_ram.c                                                   **
**  Description:	Working RAM shared by the mutually exclusive HF digital mode   **
**					streams (WSPR capture tap, FT8 waterfall front end)            **
**  Licence:		https://github.com/m0nka/mcHF/blob/main/LICENSE                **
************************************************************************************/

// Compiled only for the STM32H747 CM4 baseband build
#ifdef H7_M4_CORE

#include "icc_hf_ram.h"

uint8_t		icc_hf_ram[ICC_HF_RAM_SIZE] __attribute__ ((aligned (8)));

#endif // H7_M4_CORE
