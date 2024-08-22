/************************************************************************************
**                                                                                 **
**                             mcHF Pro QRP Transceiver                            **
**                         Krassi Atanassov - M0NKA, 2013-2021                     **
**                                                                                 **
**---------------------------------------------------------------------------------**
**                                                                                 **
**  File name:                                                                     **
**  Description:                                                                   **
**  Last Modified:                                                                 **
**  Licence:       The mcHF project is released for radio amateurs experimentation **
**               and non-commercial use only.Check 3rd party drivers for licensing **
************************************************************************************/

#ifndef __PROG_UART_H
#define __PROG_UART_H

#ifdef CONTEXT_IPC_PROC

#include "stm32h7xx_ll_dma.h"
#include "stm32h7xx_ll_rcc.h"
#include "stm32h7xx_ll_utils.h"
#include "stm32h7xx_ll_usart.h"
#include "stm32h7xx_ll_gpio.h"

// Bare metal or OS ?
#define PROG_USE_OS

// Only RX ?
//#define PROG_USE_TX

#define PROG_UART						USART1
#define PROG_UART_DMA					DMA1

#define PROG_UART_SPEED					115200

#define PROG_UART_CLK_ENABLE()        	LL_APB2_GRP1_EnableClock(LL_APB2_GRP1_PERIPH_USART1)
#define PROG_DMA_CLK_ENABLE()         	LL_AHB1_GRP1_EnableClock(LL_AHB1_GRP1_PERIPH_DMA1);

#define PROG_UART_IRQn					USART1_IRQn
#define PROG_UART_IRQHandler            USART1_IRQHandler

#define PROG_UART_DMA_TX_STREAM       	LL_DMA_STREAM_3
#define PROG_UART_DMA_TX_IRQn           DMA1_Stream3_IRQn
#define PROG_UART_DMA_TX_IRQHandler     DMA1_Stream3_IRQHandler

#define PROG_UART_RX_DMA_STREAM         LL_DMA_STREAM_2
#define PROG_UART_DMA_RX_IRQn           DMA1_Stream2_IRQn
#define PROG_UART_DMA_RX_IRQHandler     DMA1_Stream2_IRQHandler

#define PROG_UART_TX_DMA_REQUEST        LL_DMAMUX1_REQ_USART1_TX
#define PROG_UART_RX_DMA_REQUEST        LL_DMAMUX1_REQ_USART1_RX

void   prog_uart_flush(void);
ushort prog_uart_send(uchar *buff, ushort size);
ushort prog_uart_read(uchar *buff, ushort expected, ulong timeout);

void   prog_uart_init(void);

#endif

#endif
