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
#ifndef MT48LC4M32B2P_CONF_H
#define MT48LC4M32B2P_CONF_H

#include "stm32h7xx_hal.h"

// SDRAM refresh counter (100Mhz SD clock)
#define REFRESH_COUNT                   ((uint32_t)0x0603)
#define MT48LC4M32B2P_TIMEOUT           ((uint32_t)0xFFFF)

#endif
