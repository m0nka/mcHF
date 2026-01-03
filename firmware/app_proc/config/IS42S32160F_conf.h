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
#ifndef IS42S32160F_CONF_H
#define IS42S32160F_CONF_H

#include "stm32h7xx_hal.h"

// SDRAM refresh counter (100Mhz SD clock)
#define REFRESH_COUNT                   ((uint32_t)0x0603)
#define IS42S32160F_TIMEOUT             ((uint32_t)0xFFFF)

#endif
