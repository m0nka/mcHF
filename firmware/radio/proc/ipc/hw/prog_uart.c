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
#include "mchf_pro_board.h"

#ifdef CONTEXT_IPC_PROC

#include "lwrb.h"
#include "prog_uart.h"

/**
 * \brief           Calculate length of statically allocated array
 */
#define ARRAY_LEN(x)            (sizeof(x) / sizeof((x)[0]))

/**
 * \brief           Buffer for USART DMA RX
 * \note            Contains RAW unprocessed data received by UART and transfered by DMA
 */
__attribute__((section("dma_mem"))) __attribute__ ((aligned (32))) uint8_t prog_rx_dma_buffer[64];

/**
 * \brief           Create ring buffer for received data
 */
__attribute__((section("dma_mem"))) __attribute__ ((aligned (32))) static lwrb_t prog_rx_dma_ringbuff;

/**
 * \brief           Ring buffer data array for RX DMA
 */
__attribute__((section("dma_mem"))) __attribute__ ((aligned (32))) static uint8_t prog_rx_dma_lwrb_data[128];

#ifdef PROG_USE_TX
/**
 * \brief           Create ring buffer for TX DMA
 */
__attribute__((section("dma_mem"))) __attribute__ ((aligned (32))) static lwrb_t prog_tx_dma_ringbuff;

/**
 * \brief           Ring buffer data array for TX DMA
 */
__attribute__((section("dma_mem"))) __attribute__ ((aligned (32))) static uint8_t prog_tx_dma_lwrb_data[128];

/**
 * \brief           Length of TX DMA transfer
 */
static size_t prog_tx_dma_current_len;

static uint8_t 	prog_uart_start_tx_dma_transfer(void);
#endif

static void 	prog_uart_rx_check(void);

/**
 * \brief           PROG_UART_DMA stream1 interrupt handler for USART RX
 */
void PROG_UART_DMA_RX_IRQHandler(void)
{
    /* Check half-transfer complete interrupt */
    if (	LL_DMA_IsEnabledIT_HT(PROG_UART_DMA, PROG_UART_RX_DMA_STREAM) &&\
    		LL_DMA_IsActiveFlag_HT0(PROG_UART_DMA))
    {
        LL_DMA_ClearFlag_HT0(PROG_UART_DMA);     /* Clear half-transfer complete flag */
        prog_uart_rx_check();                       /* Check for data to process */
    }

    /* Check transfer-complete interrupt */
    if (	LL_DMA_IsEnabledIT_TC(PROG_UART_DMA, PROG_UART_RX_DMA_STREAM) &&\
    		LL_DMA_IsActiveFlag_TC0(PROG_UART_DMA))
    {
        LL_DMA_ClearFlag_TC0(PROG_UART_DMA);             /* Clear transfer complete flag */
        prog_uart_rx_check();                       /* Check for data to process */
    }

    /* Implement other events when needed */
}

#ifdef PROG_USE_TX
/**
 * \brief           PROG_UART_DMA stream1 interrupt handler for USART TX
 */
void PROG_UART_DMA_TX_IRQHandler(void)
{
    /* Check transfer complete */
    if (LL_DMA_IsEnabledIT_TC(PROG_UART_DMA, PROG_UART_DMA_TX_STREAM) && LL_DMA_IsActiveFlag_TC1(PROG_UART_DMA)) {
        LL_DMA_ClearFlag_TC1(PROG_UART_DMA);             /* Clear transfer complete flag */
        lwrb_skip(&prog_tx_dma_ringbuff, prog_tx_dma_current_len);/* Skip sent data, mark as read */
        prog_tx_dma_current_len = 0;           /* Clear length variable */
        prog_uart_start_tx_dma_transfer();          /* Start sending more data */
    }

    /* Implement other events when needed */
}
#endif

/**
 * \brief           USART global interrupt handler
 */
void PROG_UART_IRQHandler(void)
{
    /* Check for IDLE line interrupt */
    if (LL_USART_IsEnabledIT_IDLE(PROG_UART) && LL_USART_IsActiveFlag_IDLE(PROG_UART))
    {
        LL_USART_ClearFlag_IDLE(PROG_UART);        /* Clear IDLE line flag */
        prog_uart_rx_check();                       /* Check for data to process */
    }

    /* Implement other events when needed */
}

/**
 * \brief           Process received data over UART
 * Data are written to RX ringbuffer for application processing at latter stage
 * \param[in]       data: Data to process
 * \param[in]       len: Length in units of bytes
 */
void prog_uart_process_data(const void* data, size_t len)
{
    lwrb_write(&prog_rx_dma_ringbuff, data, len);  /* Write data to receive buffer */
}

/**
 * \brief           Check for new data received with DMA
 */
static void prog_uart_rx_check(void)
{
    static size_t old_pos;
    size_t pos;

    /* Calculate current position in buffer */
    pos = ARRAY_LEN(prog_rx_dma_buffer) - LL_DMA_GetDataLength(PROG_UART_DMA, PROG_UART_RX_DMA_STREAM);
    if (pos != old_pos) {                       /* Check change in received data */
        if (pos > old_pos) {                    /* Current position is over previous one */

        	SCB_InvalidateDCache_by_Addr((uint32_t *)&prog_rx_dma_buffer[old_pos], pos - old_pos);
            /* We are in "linear" mode */
            /* Process data directly by subtracting "pointers" */
        	prog_uart_process_data(&prog_rx_dma_buffer[old_pos], pos - old_pos);
        } else {

        	SCB_InvalidateDCache_by_Addr((uint32_t *)&prog_rx_dma_buffer[old_pos], ARRAY_LEN(prog_rx_dma_buffer) - old_pos);
            /* We are in "overflow" mode */
            /* First process data to the end of buffer */
        	prog_uart_process_data(&prog_rx_dma_buffer[old_pos], ARRAY_LEN(prog_rx_dma_buffer) - old_pos);
            /* Check and continue with beginning of buffer */
            if (pos > 0) {
            	SCB_InvalidateDCache_by_Addr((uint32_t *)&prog_rx_dma_buffer[0], pos);
            	prog_uart_process_data(&prog_rx_dma_buffer[0], pos);
            }
        }
    }
    old_pos = pos;                              /* Save current position as old */

    /* Check and manually update if we reached end of buffer */
    if (old_pos == ARRAY_LEN(prog_rx_dma_buffer)) {
        old_pos = 0;
    }
}

#ifdef PROG_USE_TX
/**
 * \brief           Check if DMA is active and if not try to send data
 */
static uint8_t prog_uart_start_tx_dma_transfer(void)
{
    //uint32_t old_primask;
    uint8_t started = 0;

    /* Check if transfer active */
    if (prog_tx_dma_current_len > 0) {
        return 0;
    }

    /* Check if DMA is active */
    /* Must be set to 0 */
    //old_primask = __get_PRIMASK();
    //__disable_irq();

    /* Check if transfer is not active */
    if (prog_tx_dma_current_len == 0
            && (prog_tx_dma_current_len = lwrb_get_linear_block_read_length(&prog_tx_dma_ringbuff)) > 0) {
        /* Disable channel if enabled */
        LL_DMA_DisableStream(PROG_UART_DMA, PROG_UART_DMA_TX_STREAM);

        /* Clear all flags */
        LL_DMA_ClearFlag_TC1(PROG_UART_DMA);
        LL_DMA_ClearFlag_HT1(PROG_UART_DMA);
        LL_DMA_ClearFlag_TE1(PROG_UART_DMA);
        LL_DMA_ClearFlag_DME1(PROG_UART_DMA);
        LL_DMA_ClearFlag_FE1(PROG_UART_DMA);

        uint32_t addr =  (uint32_t)lwrb_get_linear_block_read_address(&prog_tx_dma_ringbuff);

        SCB_CleanDCache_by_Addr((uint32_t *)addr, prog_tx_dma_current_len);

        /* Start DMA transfer */
        LL_DMA_SetDataLength(PROG_UART_DMA, PROG_UART_DMA_TX_STREAM, prog_tx_dma_current_len);
        LL_DMA_SetMemoryAddress(PROG_UART_DMA, PROG_UART_DMA_TX_STREAM, addr);

        /* Start new transfer */
        LL_DMA_EnableStream(PROG_UART_DMA, PROG_UART_DMA_TX_STREAM);
        started = 1;
    }

    //__set_PRIMASK(old_primask);
    return started;
}
#endif

/**
 * \brief           USART Initialization Function
 */
static void prog_uart_usart_init(void)
{
	RCC_PeriphCLKInitTypeDef RCC_PeriphClkInit;
    LL_USART_InitTypeDef USART_InitStruct = {0};
    LL_GPIO_InitTypeDef GPIO_InitStruct = {0};

	RCC_PeriphClkInit.PeriphClockSelection  	= RCC_PERIPHCLK_USART1;
	RCC_PeriphClkInit.Usart16ClockSelection     = RCC_USART1CLKSOURCE_D2PCLK2;
	HAL_RCCEx_PeriphCLKConfig(&RCC_PeriphClkInit);

    /* Peripheral clock enable */
    PROG_UART_CLK_ENABLE();
    PROG_DMA_CLK_ENABLE();

    GPIO_InitStruct.Mode 		= LL_GPIO_MODE_ALTERNATE;
    GPIO_InitStruct.Speed 		= LL_GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.OutputType	= LL_GPIO_OUTPUT_PUSHPULL;
    GPIO_InitStruct.Pull 		= LL_GPIO_PULL_NO;

    // PA10 - PROG_UART RX
    GPIO_InitStruct.Pin 		= LL_GPIO_PIN_10;
    GPIO_InitStruct.Alternate 	= LL_GPIO_AF_7;
    LL_GPIO_Init(GPIOA, &GPIO_InitStruct);

#ifdef PROG_USE_TX
    // PB14 - PROG_UART TX
    GPIO_InitStruct.Pin 		= LL_GPIO_PIN_14;
    GPIO_InitStruct.Alternate 	= LL_GPIO_AF_4;
    LL_GPIO_Init(GPIOB, &GPIO_InitStruct);
#endif

    /* USART RX Init */
    LL_DMA_SetPeriphRequest			(PROG_UART_DMA, PROG_UART_RX_DMA_STREAM, PROG_UART_RX_DMA_REQUEST);
    LL_DMA_SetDataTransferDirection	(PROG_UART_DMA, PROG_UART_RX_DMA_STREAM, LL_DMA_DIRECTION_PERIPH_TO_MEMORY);
    LL_DMA_SetStreamPriorityLevel	(PROG_UART_DMA, PROG_UART_RX_DMA_STREAM, LL_DMA_PRIORITY_LOW);
    LL_DMA_SetMode					(PROG_UART_DMA, PROG_UART_RX_DMA_STREAM, LL_DMA_MODE_CIRCULAR);
    LL_DMA_SetPeriphIncMode			(PROG_UART_DMA, PROG_UART_RX_DMA_STREAM, LL_DMA_PERIPH_NOINCREMENT);
    LL_DMA_SetMemoryIncMode			(PROG_UART_DMA, PROG_UART_RX_DMA_STREAM, LL_DMA_MEMORY_INCREMENT);
    LL_DMA_SetPeriphSize			(PROG_UART_DMA, PROG_UART_RX_DMA_STREAM, LL_DMA_PDATAALIGN_BYTE);
    LL_DMA_SetMemorySize			(PROG_UART_DMA, PROG_UART_RX_DMA_STREAM, LL_DMA_MDATAALIGN_BYTE);
    LL_DMA_DisableFifoMode			(PROG_UART_DMA, PROG_UART_RX_DMA_STREAM);
    LL_DMA_SetPeriphAddress			(PROG_UART_DMA, PROG_UART_RX_DMA_STREAM, LL_USART_DMA_GetRegAddr(PROG_UART, LL_USART_DMA_REG_DATA_RECEIVE));
    LL_DMA_SetMemoryAddress			(PROG_UART_DMA, PROG_UART_RX_DMA_STREAM, (uint32_t)prog_rx_dma_buffer);
    LL_DMA_SetDataLength			(PROG_UART_DMA, PROG_UART_RX_DMA_STREAM, ARRAY_LEN(prog_rx_dma_buffer));

#ifdef PROG_USE_TX
    /* USART TX Init */
    LL_DMA_SetPeriphRequest			(PROG_UART_DMA, PROG_UART_DMA_TX_STREAM, PROG_UART_TX_DMA_REQUEST);
    LL_DMA_SetDataTransferDirection	(PROG_UART_DMA, PROG_UART_DMA_TX_STREAM, LL_DMA_DIRECTION_MEMORY_TO_PERIPH);
    LL_DMA_SetStreamPriorityLevel	(PROG_UART_DMA, PROG_UART_DMA_TX_STREAM, LL_DMA_PRIORITY_LOW);
    LL_DMA_SetMode					(PROG_UART_DMA, PROG_UART_DMA_TX_STREAM, LL_DMA_MODE_NORMAL);
    LL_DMA_SetPeriphIncMode			(PROG_UART_DMA, PROG_UART_DMA_TX_STREAM, LL_DMA_PERIPH_NOINCREMENT);
    LL_DMA_SetMemoryIncMode			(PROG_UART_DMA, PROG_UART_DMA_TX_STREAM, LL_DMA_MEMORY_INCREMENT);
    LL_DMA_SetPeriphSize			(PROG_UART_DMA, PROG_UART_DMA_TX_STREAM, LL_DMA_PDATAALIGN_BYTE);
    LL_DMA_SetMemorySize			(PROG_UART_DMA, PROG_UART_DMA_TX_STREAM, LL_DMA_MDATAALIGN_BYTE);
    LL_DMA_DisableFifoMode			(PROG_UART_DMA, PROG_UART_DMA_TX_STREAM);
    LL_DMA_SetPeriphAddress			(PROG_UART_DMA, PROG_UART_DMA_TX_STREAM, LL_USART_DMA_GetRegAddr(PROG_UART, LL_USART_DMA_REG_DATA_TRANSMIT));
#endif

    /* Enable DMA RX HT & TC interrupts */
    LL_DMA_EnableIT_HT				(PROG_UART_DMA, PROG_UART_RX_DMA_STREAM);
    LL_DMA_EnableIT_TC				(PROG_UART_DMA, PROG_UART_RX_DMA_STREAM);

#ifdef PROG_USE_TX
    /* Enable DMA TX TC interrupts */
    LL_DMA_EnableIT_TC				(PROG_UART_DMA, PROG_UART_DMA_TX_STREAM);
#endif

    /* DMA interrupt init */
    NVIC_SetPriority				(PROG_UART_DMA_RX_IRQn, NVIC_EncodePriority(NVIC_GetPriorityGrouping(), 5, 0));
    NVIC_EnableIRQ					(PROG_UART_DMA_RX_IRQn);

#ifdef PROG_USE_TX
    NVIC_SetPriority				(PROG_UART_DMA_TX_IRQn, NVIC_EncodePriority(NVIC_GetPriorityGrouping(), 5, 0));
    NVIC_EnableIRQ					(PROG_UART_DMA_TX_IRQn);
#endif

    /* Configure USART */
    USART_InitStruct.PrescalerValue			= LL_USART_PRESCALER_DIV1;
    USART_InitStruct.BaudRate 				= PROG_UART_SPEED;
    USART_InitStruct.DataWidth 				= LL_USART_DATAWIDTH_8B;
    USART_InitStruct.StopBits 				= LL_USART_STOPBITS_1;
    USART_InitStruct.Parity 				= LL_USART_PARITY_NONE;
#ifdef PROG_USE_TX
    USART_InitStruct.TransferDirection 		= LL_USART_DIRECTION_TX_RX;
#else
    USART_InitStruct.TransferDirection 		= LL_USART_DIRECTION_RX;
#endif
    USART_InitStruct.HardwareFlowControl	= LL_USART_HWCONTROL_NONE;
    USART_InitStruct.OverSampling 			= LL_USART_OVERSAMPLING_16;

    LL_USART_Init					(PROG_UART, &USART_InitStruct);

#ifdef PROG_USE_TX
    LL_USART_SetTXFIFOThreshold		(PROG_UART, LL_USART_FIFOTHRESHOLD_7_8);
#endif
    LL_USART_SetRXFIFOThreshold		(PROG_UART, LL_USART_FIFOTHRESHOLD_7_8);
    LL_USART_EnableFIFO				(PROG_UART);
    LL_USART_ConfigAsyncMode		(PROG_UART);
    LL_USART_EnableDMAReq_RX		(PROG_UART);
#ifdef PROG_USE_TX
    LL_USART_EnableDMAReq_TX		(PROG_UART);
#endif
    LL_USART_EnableIT_IDLE			(PROG_UART);

    /* USART interrupt, same priority as DMA channel */
    NVIC_SetPriority				(PROG_UART_IRQn, NVIC_EncodePriority(NVIC_GetPriorityGrouping(), 5, 0));
    NVIC_EnableIRQ  				(PROG_UART_IRQn);

	SCB_CleanDCache_by_Addr((uint32_t *)prog_rx_dma_buffer, 	ARRAY_LEN(prog_rx_dma_buffer));
	SCB_CleanDCache_by_Addr((uint32_t *)prog_rx_dma_lwrb_data, ARRAY_LEN(prog_rx_dma_lwrb_data));

#ifdef PROG_USE_TX
	SCB_CleanDCache_by_Addr((uint32_t *)prog_tx_dma_lwrb_data, ARRAY_LEN(prog_tx_dma_lwrb_data));
#endif

    /* Enable USART and DMA RX */
    LL_DMA_EnableStream				(PROG_UART_DMA, PROG_UART_RX_DMA_STREAM);	// ToDo: crashes firmware
    LL_USART_Enable					(PROG_UART);

#ifdef PROG_USE_TX
    /* Polling USART initialisation */
    while (!LL_USART_IsActiveFlag_TEACK(PROG_UART) || !LL_USART_IsActiveFlag_REACK(PROG_UART)) {}
#else
    while (!LL_USART_IsActiveFlag_REACK(PROG_UART)) {}
#endif
}

ushort prog_uart_send(uchar *buff, ushort size)
{
#ifdef PROG_USE_TX
	//printf("tx buffer: %d\r\n", prog_tx_dma_current_len);
	if(lwrb_write(&prog_tx_dma_ringbuff, buff, size) != size)
		return 1;

	if(prog_uart_start_tx_dma_transfer() == 0)
		return 2;
#endif

	return 0;
}

//*----------------------------------------------------------------------------
//* Function Name       : prog_uart_flush
//* Object              :
//* Notes    			: Flush ring buffer before command exchange
//* Notes    			:
//* Context    			: CONTEXT_IPC_PROC
//*----------------------------------------------------------------------------
void prog_uart_flush(void)
{
	lwrb_reset(&prog_rx_dma_ringbuff);
}

//*----------------------------------------------------------------------------
//* Function Name       : prog_uart_read
//* Object              :
//* Notes    			: Blocking read from DMA Ring buffer
//* Notes    			:
//* Context    			: CONTEXT_IPC_PROC
//*----------------------------------------------------------------------------
ushort prog_uart_read(uchar *buff, ushort expected, ulong timeout)
{
	ushort read;

	//printf("e=%d\r\n",expected);

	while(timeout)
	{
		read = lwrb_read(&prog_rx_dma_ringbuff, buff, expected);
		//if(read) printf("r=%d\r\n",read);

		if(expected > read)
		{
			expected -= read;
			buff     += read;
		}
		else if(read == expected)
		{
			return 0;
		}

		if(expected == 0)
			return 0;

		#ifdef PROG_USE_OS
		vTaskDelay(1);
		#else
		HAL_Delay(1);
		#endif

		timeout--;
	}

	//printf("timeout\r\n");
	return 1;
}

//*----------------------------------------------------------------------------
//* Function Name       : prog_uart_init
//* Object              :
//* Notes    			: Low level driver init
//* Notes    			:
//* Context    			: CONTEXT_IPC_PROC
//*----------------------------------------------------------------------------
void prog_uart_init(void)
{
    /* Initialise ringbuff for TX & RX */
#ifdef PROG_USE_TX
    lwrb_init(&prog_tx_dma_ringbuff, prog_tx_dma_lwrb_data, sizeof(prog_tx_dma_lwrb_data));
#endif
    lwrb_init(&prog_rx_dma_ringbuff, prog_rx_dma_lwrb_data, sizeof(prog_rx_dma_lwrb_data));

    prog_uart_usart_init();
}

#endif
