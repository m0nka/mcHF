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
#include "ipc_uart.h"

/**
 * \brief           Calculate length of statically allocated array
 */
#define ARRAY_LEN(x)            (sizeof(x) / sizeof((x)[0]))

/**
 * \brief           Buffer for USART DMA RX
 * \note            Contains RAW unprocessed data received by UART and transfered by DMA
 */
__attribute__((section("dma_mem"))) __attribute__ ((aligned (32))) uint8_t ipc_rx_dma_buffer[64];

/**
 * \brief           Create ring buffer for received data
 */
__attribute__((section("dma_mem"))) __attribute__ ((aligned (32))) static lwrb_t ipc_rx_dma_ringbuff;

/**
 * \brief           Ring buffer data array for RX DMA
 */
__attribute__((section("dma_mem"))) __attribute__ ((aligned (32))) static uint8_t ipc_rx_dma_lwrb_data[128];

/**
 * \brief           Create ring buffer for TX DMA
 */
__attribute__((section("dma_mem"))) __attribute__ ((aligned (32))) static lwrb_t ipc_tx_dma_ringbuff;

/**
 * \brief           Ring buffer data array for TX DMA
 */
__attribute__((section("dma_mem"))) __attribute__ ((aligned (32))) static uint8_t ipc_tx_dma_lwrb_data[128];

/**
 * \brief           Length of TX DMA transfer
 */
static size_t ipc_tx_dma_current_len;

static uint8_t 	ipc_uart_start_tx_dma_transfer(void);
static void 	ipc_uart_rx_check(void);

/**
 * \brief           IPC_UART_DMA stream1 interrupt handler for USART RX
 */
void IPC_UART_DMA_RX_IRQHandler(void)
{
    /* Check half-transfer complete interrupt */
    if (	LL_DMA_IsEnabledIT_HT(IPC_UART_DMA, IPC_UART_RX_DMA_STREAM) &&\
    		LL_DMA_IsActiveFlag_HT0(IPC_UART_DMA))
    {
        LL_DMA_ClearFlag_HT0(IPC_UART_DMA);     /* Clear half-transfer complete flag */
        ipc_uart_rx_check();                       /* Check for data to process */
    }

    /* Check transfer-complete interrupt */
    if (	LL_DMA_IsEnabledIT_TC(IPC_UART_DMA, IPC_UART_RX_DMA_STREAM) &&\
    		LL_DMA_IsActiveFlag_TC0(IPC_UART_DMA))
    {
        LL_DMA_ClearFlag_TC0(IPC_UART_DMA);             /* Clear transfer complete flag */
        ipc_uart_rx_check();                       /* Check for data to process */
    }

    /* Implement other events when needed */
}

/**
 * \brief           IPC_UART_DMA stream1 interrupt handler for USART TX
 */
void IPC_UART_DMA_TX_IRQHandler(void)
{
    /* Check transfer complete */
    if (LL_DMA_IsEnabledIT_TC(IPC_UART_DMA, IPC_UART_DMA_TX_STREAM) && LL_DMA_IsActiveFlag_TC1(IPC_UART_DMA)) {
        LL_DMA_ClearFlag_TC1(IPC_UART_DMA);             /* Clear transfer complete flag */
        lwrb_skip(&ipc_tx_dma_ringbuff, ipc_tx_dma_current_len);/* Skip sent data, mark as read */
        ipc_tx_dma_current_len = 0;           /* Clear length variable */
        ipc_uart_start_tx_dma_transfer();          /* Start sending more data */
    }

    /* Implement other events when needed */
}

/**
 * \brief           USART global interrupt handler
 */
void IPC_UART_IRQHandler(void)
{
    /* Check for IDLE line interrupt */
    if (LL_USART_IsEnabledIT_IDLE(IPC_UART) && LL_USART_IsActiveFlag_IDLE(IPC_UART)) {
        LL_USART_ClearFlag_IDLE(IPC_UART);        /* Clear IDLE line flag */
        ipc_uart_rx_check();                       /* Check for data to process */
    }

    /* Implement other events when needed */
}

/**
 * \brief           Process received data over UART
 * Data are written to RX ringbuffer for application processing at latter stage
 * \param[in]       data: Data to process
 * \param[in]       len: Length in units of bytes
 */
void ipc_uart_process_data(const void* data, size_t len)
{
    lwrb_write(&ipc_rx_dma_ringbuff, data, len);  /* Write data to receive buffer */
}

/**
 * \brief           Check for new data received with DMA
 */
static void ipc_uart_rx_check(void)
{
    static size_t old_pos;
    size_t pos;

    /* Calculate current position in buffer */
    pos = ARRAY_LEN(ipc_rx_dma_buffer) - LL_DMA_GetDataLength(IPC_UART_DMA, IPC_UART_RX_DMA_STREAM);
    if (pos != old_pos) {                       /* Check change in received data */
        if (pos > old_pos) {                    /* Current position is over previous one */

        	SCB_InvalidateDCache_by_Addr((uint32_t *)&ipc_rx_dma_buffer[old_pos], pos - old_pos);
            /* We are in "linear" mode */
            /* Process data directly by subtracting "pointers" */
        	ipc_uart_process_data(&ipc_rx_dma_buffer[old_pos], pos - old_pos);
        } else {

        	SCB_InvalidateDCache_by_Addr((uint32_t *)&ipc_rx_dma_buffer[old_pos], ARRAY_LEN(ipc_rx_dma_buffer) - old_pos);
            /* We are in "overflow" mode */
            /* First process data to the end of buffer */
        	ipc_uart_process_data(&ipc_rx_dma_buffer[old_pos], ARRAY_LEN(ipc_rx_dma_buffer) - old_pos);
            /* Check and continue with beginning of buffer */
            if (pos > 0) {
            	SCB_InvalidateDCache_by_Addr((uint32_t *)&ipc_rx_dma_buffer[0], pos);
            	ipc_uart_process_data(&ipc_rx_dma_buffer[0], pos);
            }
        }
    }
    old_pos = pos;                              /* Save current position as old */

    /* Check and manually update if we reached end of buffer */
    if (old_pos == ARRAY_LEN(ipc_rx_dma_buffer)) {
        old_pos = 0;
    }
}

/**
 * \brief           Check if DMA is active and if not try to send data
 */
static uint8_t ipc_uart_start_tx_dma_transfer(void)
{
    uint32_t old_primask;
    uint8_t started = 0;

    /* Check if transfer active */
    if (ipc_tx_dma_current_len > 0) {
        return 0;
    }

    /* Check if DMA is active */
    /* Must be set to 0 */
    //old_primask = __get_PRIMASK();
    //__disable_irq();

    /* Check if transfer is not active */
    if (ipc_tx_dma_current_len == 0
            && (ipc_tx_dma_current_len = lwrb_get_linear_block_read_length(&ipc_tx_dma_ringbuff)) > 0) {
        /* Disable channel if enabled */
        LL_DMA_DisableStream(IPC_UART_DMA, IPC_UART_DMA_TX_STREAM);

        /* Clear all flags */
        LL_DMA_ClearFlag_TC1(IPC_UART_DMA);
        LL_DMA_ClearFlag_HT1(IPC_UART_DMA);
        LL_DMA_ClearFlag_TE1(IPC_UART_DMA);
        LL_DMA_ClearFlag_DME1(IPC_UART_DMA);
        LL_DMA_ClearFlag_FE1(IPC_UART_DMA);

        uint32_t addr =  (uint32_t)lwrb_get_linear_block_read_address(&ipc_tx_dma_ringbuff);

        SCB_CleanDCache_by_Addr((uint32_t *)addr, ipc_tx_dma_current_len);

        /* Start DMA transfer */
        LL_DMA_SetDataLength(IPC_UART_DMA, IPC_UART_DMA_TX_STREAM, ipc_tx_dma_current_len);
        LL_DMA_SetMemoryAddress(IPC_UART_DMA, IPC_UART_DMA_TX_STREAM, addr);

        /* Start new transfer */
        LL_DMA_EnableStream(IPC_UART_DMA, IPC_UART_DMA_TX_STREAM);
        started = 1;
    }

    //__set_PRIMASK(old_primask);
    return started;
}

/**
 * \brief           USART Initialization Function
 */
static void ipc_uart_usart_init(void)
{
	RCC_PeriphCLKInitTypeDef RCC_PeriphClkInit;
    LL_USART_InitTypeDef USART_InitStruct = {0};
    LL_GPIO_InitTypeDef GPIO_InitStruct = {0};

	RCC_PeriphClkInit.PeriphClockSelection  	= RCC_PERIPHCLK_USART234578;
	RCC_PeriphClkInit.Usart234578ClockSelection = RCC_USART234578CLKSOURCE_D2PCLK1;
	HAL_RCCEx_PeriphCLKConfig(&RCC_PeriphClkInit);

    /* Peripheral clock enable */
    IPC_UART_CLK_ENABLE();
    IPC_DMA_CLK_ENABLE();

    GPIO_InitStruct.Mode 		= LL_GPIO_MODE_ALTERNATE;
    GPIO_InitStruct.Speed 		= LL_GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.OutputType	= LL_GPIO_OUTPUT_PUSHPULL;
    GPIO_InitStruct.Pull 		= LL_GPIO_PULL_NO;

    // PA8 - IPC_UART RX
    GPIO_InitStruct.Pin 		= LL_GPIO_PIN_8;
    GPIO_InitStruct.Alternate 	= LL_GPIO_AF_11;
    LL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    // PF7 - IPC_UART TX
    GPIO_InitStruct.Pin 		= LL_GPIO_PIN_7;
    GPIO_InitStruct.Alternate 	= LL_GPIO_AF_7;
    LL_GPIO_Init(GPIOF, &GPIO_InitStruct);

    /* USART RX Init */
    LL_DMA_SetPeriphRequest			(IPC_UART_DMA, IPC_UART_RX_DMA_STREAM, IPC_UART_RX_DMA_REQUEST);
    LL_DMA_SetDataTransferDirection	(IPC_UART_DMA, IPC_UART_RX_DMA_STREAM, LL_DMA_DIRECTION_PERIPH_TO_MEMORY);
    LL_DMA_SetStreamPriorityLevel	(IPC_UART_DMA, IPC_UART_RX_DMA_STREAM, LL_DMA_PRIORITY_LOW);
    LL_DMA_SetMode					(IPC_UART_DMA, IPC_UART_RX_DMA_STREAM, LL_DMA_MODE_CIRCULAR);
    LL_DMA_SetPeriphIncMode			(IPC_UART_DMA, IPC_UART_RX_DMA_STREAM, LL_DMA_PERIPH_NOINCREMENT);
    LL_DMA_SetMemoryIncMode			(IPC_UART_DMA, IPC_UART_RX_DMA_STREAM, LL_DMA_MEMORY_INCREMENT);
    LL_DMA_SetPeriphSize			(IPC_UART_DMA, IPC_UART_RX_DMA_STREAM, LL_DMA_PDATAALIGN_BYTE);
    LL_DMA_SetMemorySize			(IPC_UART_DMA, IPC_UART_RX_DMA_STREAM, LL_DMA_MDATAALIGN_BYTE);
    LL_DMA_DisableFifoMode			(IPC_UART_DMA, IPC_UART_RX_DMA_STREAM);
    LL_DMA_SetPeriphAddress			(IPC_UART_DMA, IPC_UART_RX_DMA_STREAM, LL_USART_DMA_GetRegAddr(IPC_UART, LL_USART_DMA_REG_DATA_RECEIVE));
    LL_DMA_SetMemoryAddress			(IPC_UART_DMA, IPC_UART_RX_DMA_STREAM, (uint32_t)ipc_rx_dma_buffer);
    LL_DMA_SetDataLength			(IPC_UART_DMA, IPC_UART_RX_DMA_STREAM, ARRAY_LEN(ipc_rx_dma_buffer));

    /* USART TX Init */
    LL_DMA_SetPeriphRequest			(IPC_UART_DMA, IPC_UART_DMA_TX_STREAM, IPC_UART_TX_DMA_REQUEST);
    LL_DMA_SetDataTransferDirection	(IPC_UART_DMA, IPC_UART_DMA_TX_STREAM, LL_DMA_DIRECTION_MEMORY_TO_PERIPH);
    LL_DMA_SetStreamPriorityLevel	(IPC_UART_DMA, IPC_UART_DMA_TX_STREAM, LL_DMA_PRIORITY_LOW);
    LL_DMA_SetMode					(IPC_UART_DMA, IPC_UART_DMA_TX_STREAM, LL_DMA_MODE_NORMAL);
    LL_DMA_SetPeriphIncMode			(IPC_UART_DMA, IPC_UART_DMA_TX_STREAM, LL_DMA_PERIPH_NOINCREMENT);
    LL_DMA_SetMemoryIncMode			(IPC_UART_DMA, IPC_UART_DMA_TX_STREAM, LL_DMA_MEMORY_INCREMENT);
    LL_DMA_SetPeriphSize			(IPC_UART_DMA, IPC_UART_DMA_TX_STREAM, LL_DMA_PDATAALIGN_BYTE);
    LL_DMA_SetMemorySize			(IPC_UART_DMA, IPC_UART_DMA_TX_STREAM, LL_DMA_MDATAALIGN_BYTE);
    LL_DMA_DisableFifoMode			(IPC_UART_DMA, IPC_UART_DMA_TX_STREAM);
    LL_DMA_SetPeriphAddress			(IPC_UART_DMA, IPC_UART_DMA_TX_STREAM, LL_USART_DMA_GetRegAddr(IPC_UART, LL_USART_DMA_REG_DATA_TRANSMIT));

    /* Enable DMA RX HT & TC interrupts */
    LL_DMA_EnableIT_HT				(IPC_UART_DMA, IPC_UART_RX_DMA_STREAM);
    LL_DMA_EnableIT_TC				(IPC_UART_DMA, IPC_UART_RX_DMA_STREAM);

    /* Enable DMA TX TC interrupts */
    LL_DMA_EnableIT_TC				(IPC_UART_DMA, IPC_UART_DMA_TX_STREAM);

    /* DMA interrupt init */
    NVIC_SetPriority				(IPC_UART_DMA_RX_IRQn, NVIC_EncodePriority(NVIC_GetPriorityGrouping(), 5, 0));
    NVIC_EnableIRQ					(IPC_UART_DMA_RX_IRQn);
    NVIC_SetPriority				(IPC_UART_DMA_TX_IRQn, NVIC_EncodePriority(NVIC_GetPriorityGrouping(), 5, 0));
    NVIC_EnableIRQ					(IPC_UART_DMA_TX_IRQn);

    /* Configure USART */
    USART_InitStruct.PrescalerValue			= LL_USART_PRESCALER_DIV1;
    USART_InitStruct.BaudRate 				= IPC_UART_SPEED;
    USART_InitStruct.DataWidth 				= LL_USART_DATAWIDTH_8B;
    USART_InitStruct.StopBits 				= LL_USART_STOPBITS_1;
    USART_InitStruct.Parity 				= LL_USART_PARITY_NONE;
    USART_InitStruct.TransferDirection 		= LL_USART_DIRECTION_TX_RX;
    USART_InitStruct.HardwareFlowControl	= LL_USART_HWCONTROL_NONE;
    USART_InitStruct.OverSampling 			= LL_USART_OVERSAMPLING_16;

    LL_USART_Init					(IPC_UART, &USART_InitStruct);

    LL_USART_SetTXFIFOThreshold		(IPC_UART, LL_USART_FIFOTHRESHOLD_7_8);
    LL_USART_SetRXFIFOThreshold		(IPC_UART, LL_USART_FIFOTHRESHOLD_7_8);
    LL_USART_EnableFIFO				(IPC_UART);
    LL_USART_ConfigAsyncMode		(IPC_UART);
    LL_USART_EnableDMAReq_RX		(IPC_UART);
    LL_USART_EnableDMAReq_TX		(IPC_UART);
    LL_USART_EnableIT_IDLE			(IPC_UART);

    /* USART interrupt, same priority as DMA channel */
    NVIC_SetPriority				(IPC_UART_IRQn, NVIC_EncodePriority(NVIC_GetPriorityGrouping(), 5, 0));
    NVIC_EnableIRQ  				(IPC_UART_IRQn);

	SCB_CleanDCache_by_Addr((uint32_t *)ipc_rx_dma_buffer, 	ARRAY_LEN(ipc_rx_dma_buffer));
	SCB_CleanDCache_by_Addr((uint32_t *)ipc_rx_dma_lwrb_data, ARRAY_LEN(ipc_rx_dma_lwrb_data));
	SCB_CleanDCache_by_Addr((uint32_t *)ipc_tx_dma_lwrb_data, ARRAY_LEN(ipc_tx_dma_lwrb_data));

    /* Enable USART and DMA RX */
    LL_DMA_EnableStream				(IPC_UART_DMA, IPC_UART_RX_DMA_STREAM);
    LL_USART_Enable					(IPC_UART);

    /* Polling USART initialisation */
    while (!LL_USART_IsActiveFlag_TEACK(IPC_UART) || !LL_USART_IsActiveFlag_REACK(IPC_UART)) {}
}

static void ipc_uart_usart_cleanup(void)
{
	LL_USART_Disable		(IPC_UART);
	LL_DMA_DisableStream	(IPC_UART_DMA, IPC_UART_RX_DMA_STREAM);
	NVIC_DisableIRQ  		(IPC_UART_IRQn);
}

ushort ipc_uart_send(uchar *buff, ushort size)
{
	//printf("tx buffer: %d\r\n", ipc_tx_dma_current_len);
	if(lwrb_write(&ipc_tx_dma_ringbuff, buff, size) != size)
		return 1;

	if(ipc_uart_start_tx_dma_transfer() == 0)
		return 2;

	return 0;
}

//*----------------------------------------------------------------------------
//* Function Name       : ipc_uart_flush
//* Object              :
//* Notes    			: Flush ring buffer before command exchange
//* Notes    			:
//* Context    			: CONTEXT_IPC_PROC
//*----------------------------------------------------------------------------
void ipc_uart_flush(void)
{
	lwrb_reset(&ipc_rx_dma_ringbuff);
}

//*----------------------------------------------------------------------------
//* Function Name       : ipc_uart_read
//* Object              :
//* Notes    			: Blocking read from DMA Ring buffer
//* Notes    			:
//* Context    			: CONTEXT_IPC_PROC
//*----------------------------------------------------------------------------
ushort ipc_uart_read(uchar *buff, ushort expected, ulong timeout)
{
	ushort read;

	//printf("e=%d\r\n",expected);

	while(timeout)
	{
		read = lwrb_read(&ipc_rx_dma_ringbuff, buff, expected);
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

		#ifdef IPC_USE_OS
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
//* Function Name       : ipc_uart_init
//* Object              :
//* Notes    			: Low level driver init
//* Notes    			:
//* Context    			: CONTEXT_IPC_PROC
//*----------------------------------------------------------------------------
void ipc_uart_init(void)
{
    /* Initialise ringbuff for TX & RX */
    lwrb_init(&ipc_tx_dma_ringbuff, ipc_tx_dma_lwrb_data, sizeof(ipc_tx_dma_lwrb_data));
    lwrb_init(&ipc_rx_dma_ringbuff, ipc_rx_dma_lwrb_data, sizeof(ipc_rx_dma_lwrb_data));

    ipc_uart_usart_init();
}

void ipc_uart_stop(void)
{
	ipc_uart_usart_cleanup();
}

#endif
