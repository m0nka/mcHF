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
#ifndef __GPS_UART_H
#define __GPS_UART_H

#ifdef CONTEXT_GPS

#include "stm32h7xx_ll_dma.h"
#include "stm32h7xx_ll_rcc.h"
#include "stm32h7xx_ll_utils.h"
#include "stm32h7xx_ll_usart.h"
#include "stm32h7xx_ll_gpio.h"

// Only RX ?
//#define GPS_USE_TX

#define GPS_UART						USART6
#define GPS_UART_DMA					DMA1

#define GPS_UART_SPEED					38400

#define GPS_UART_CLK_ENABLE()           LL_APB2_GRP1_EnableClock(LL_APB2_GRP1_PERIPH_USART6)
#define GPS_DMA_CLK_ENABLE()           	LL_AHB1_GRP1_EnableClock(LL_AHB1_GRP1_PERIPH_DMA1);

#define GPS_UART_IRQn					USART6_IRQn
#define GPS_UART_IRQHandler             USART6_IRQHandler

#define GPS_UART_DMA_TX_STREAM         	LL_DMA_STREAM_2
#define GPS_UART_DMA_TX_IRQn            DMA1_Stream2_IRQn
#define GPS_UART_DMA_TX_IRQHandler      DMA1_Stream2_IRQHandler

#define GPS_UART_RX_DMA_STREAM          LL_DMA_STREAM_1
#define GPS_UART_DMA_RX_IRQn            DMA1_Stream1_IRQn
#define GPS_UART_DMA_RX_IRQHandler      DMA1_Stream1_IRQHandler

#define GPS_UART_TX_DMA_REQUEST         LL_DMAMUX1_REQ_USART6_TX
#define GPS_UART_RX_DMA_REQUEST         LL_DMAMUX1_REQ_USART6_RX

#define ARRAY_LEN(x)            		(sizeof(x) / sizeof((x)[0]))

// ---------------------------------------------------------------------
void   	gps_uart_flush(void);
ushort 	gps_uart_send(uchar *buff, ushort size);
ushort 	gps_uart_read(uchar *buff, ushort expected, ulong timeout);

void 	gps_uart_init(void);
void 	gps_uart_stop(void);

#endif

#endif
