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

#ifndef __IPC_UART_H
#define __IPC_UART_H

#ifdef CONTEXT_IPC_PROC

#include "stm32h7xx_ll_dma.h"
#include "stm32h7xx_ll_rcc.h"
#include "stm32h7xx_ll_utils.h"
#include "stm32h7xx_ll_usart.h"
#include "stm32h7xx_ll_gpio.h"

// Bare metal or OS ?
#define IPC_USE_OS

// Only RX ?
#define IPC_USE_TX

#define IPC_UART						UART7
#define IPC_UART_DMA					DMA1

#define IPC_UART_SPEED					921600			//115200

#define IPC_UART_CLK_ENABLE()           LL_APB1_GRP1_EnableClock(LL_APB1_GRP1_PERIPH_UART7)
#define IPC_DMA_CLK_ENABLE()           	LL_AHB1_GRP1_EnableClock(LL_AHB1_GRP1_PERIPH_DMA1);

#define IPC_UART_IRQn					UART7_IRQn
#define IPC_UART_IRQHandler             UART7_IRQHandler

#define IPC_UART_DMA_TX_STREAM         	LL_DMA_STREAM_1
#define IPC_UART_DMA_TX_IRQn            DMA1_Stream1_IRQn
#define IPC_UART_DMA_TX_IRQHandler      DMA1_Stream1_IRQHandler

#define IPC_UART_RX_DMA_STREAM          LL_DMA_STREAM_0
#define IPC_UART_DMA_RX_IRQn            DMA1_Stream0_IRQn
#define IPC_UART_DMA_RX_IRQHandler      DMA1_Stream0_IRQHandler

#define IPC_UART_TX_DMA_REQUEST         LL_DMAMUX1_REQ_UART7_TX
#define IPC_UART_RX_DMA_REQUEST         LL_DMAMUX1_REQ_UART7_RX

void   ipc_uart_flush(void);
ushort ipc_uart_send(uchar *buff, ushort size);
ushort ipc_uart_read(uchar *buff, ushort expected, ulong timeout);

void ipc_uart_init(void);
void ipc_uart_stop(void);

#endif

#endif
