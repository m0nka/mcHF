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
#include "main.h"
#include "mchf_pro_board.h"

#ifdef CONTEXT_GPS

#include "gps_proc.h"
#include "lwrb.h"
#include "gps_uart.h"

// Contains RAW unprocessed data received by UART and transfered by DMA
__attribute__((section(".axi_mem"))) __attribute__ ((aligned (32))) \
uint8_t gps_rx_dma_buffer[256];

// Create ring buffer for received data
//__attribute__((section(".axi_mem"))) __attribute__ ((aligned (32)))
//static lwrb_t gps_rx_dma_ringbuff;

// Ring buffer data array for RX DMA
//__attribute__((section(".axi_mem"))) __attribute__ ((aligned (32)))
//static uint8_t gps_rx_dma_lwrb_data[256];

#ifdef GPS_USE_TX
// Create ring buffer for TX DMA
__attribute__((section(".axi_mem"))) __attribute__ ((aligned (32))) \
static lwrb_t gps_tx_dma_ringbuff;

// Ring buffer data array for TX DMA
__attribute__((section(".axi_mem"))) __attribute__ ((aligned (32))) \
static uint8_t gps_tx_dma_lwrb_data[256];

// Length of TX DMA transfer
static size_t gps_tx_dma_current_len;

static uint8_t 	gps_uart_start_tx_dma_transfer(void);
#endif

static void 	gps_uart_rx_check(void);

/**
 * \brief           GPS_UART_DMA stream1 interrupt handler for USART RX
 */
void GPS_UART_DMA_RX_IRQHandler(void)
{
    if(	LL_DMA_IsEnabledIT_HT(GPS_UART_DMA, GPS_UART_RX_DMA_STREAM) &&\
    	LL_DMA_IsActiveFlag_HT1(GPS_UART_DMA))
    {
    	// Check half-transfer complete interrupt
    	//--printf("dma ht \r\n");

        LL_DMA_ClearFlag_HT1(GPS_UART_DMA);     /* Clear half-transfer complete flag */
        gps_uart_rx_check();                       /* Check for data to process */
    }
    else if(LL_DMA_IsEnabledIT_TC(GPS_UART_DMA, GPS_UART_RX_DMA_STREAM) &&\
    		LL_DMA_IsActiveFlag_TC1(GPS_UART_DMA))
    {
    	// Check transfer-complete interrupt
    	//--printf("dma tc \r\n");

        LL_DMA_ClearFlag_TC1(GPS_UART_DMA);             /* Clear transfer complete flag */
        gps_uart_rx_check();                       /* Check for data to process */
    }
    else
    {
    	// Implement other events when needed
    	//--printf("dma else \r\n");
    }
}

/**
 * \brief           GPS_UART_DMA stream1 interrupt handler for USART TX
 */
#ifdef GPS_USE_TX
void GPS_UART_DMA_TX_IRQHandler(void)
{
    /* Check transfer complete */
    if (LL_DMA_IsEnabledIT_TC(GPS_UART_DMA, GPS_UART_DMA_TX_STREAM) && LL_DMA_IsActiveFlag_TC2(GPS_UART_DMA))
    {
        LL_DMA_ClearFlag_TC2(GPS_UART_DMA);             /* Clear transfer complete flag */
        lwrb_skip(&gps_tx_dma_ringbuff, gps_tx_dma_current_len);/* Skip sent data, mark as read */
        gps_tx_dma_current_len = 0;           /* Clear length variable */
        gps_uart_start_tx_dma_transfer();          /* Start sending more data */
    }

    /* Implement other events when needed */
}
#endif

/**
 * \brief           USART global interrupt handler
 */
void GPS_UART_IRQHandler(void)
{
    /* Check for IDLE line interrupt */
    if (LL_USART_IsEnabledIT_IDLE(GPS_UART) && LL_USART_IsActiveFlag_IDLE(GPS_UART))
    {
    	//--printf("idle \r\n");

        LL_USART_ClearFlag_IDLE(GPS_UART);        /* Clear IDLE line flag */
        gps_uart_rx_check();                       /* Check for data to process */
    }
    else
    {
    	// Implement other events when needed
    	//--printf("uart else \r\n");
    }
}

/**
 * \brief           Process received data over UART
 * Data are written to RX ringbuffer for application processing at latter stage
 * \param[in]       data: Data to process
 * \param[in]       len: Length in units of bytes
 */
void gps_uart_process_data(const void* data, size_t len)
{
	//--printf("data %d %s \r\n", len, data);
    //--lwrb_write(&gps_rx_dma_ringbuff, data, len);  /* Write data to receive buffer */

    gps_proc_message((char *)data, len);
}

/**
 * \brief           Check for new data received with DMA
 */
static void gps_uart_rx_check(void)
{
    static size_t old_pos;
    size_t pos;

    /* Calculate current position in buffer */
    pos = ARRAY_LEN(gps_rx_dma_buffer) - LL_DMA_GetDataLength(GPS_UART_DMA, GPS_UART_RX_DMA_STREAM);
    if (pos != old_pos) {                       /* Check change in received data */
        if (pos > old_pos) {                    /* Current position is over previous one */

        	SCB_InvalidateDCache_by_Addr((uint32_t *)&gps_rx_dma_buffer[old_pos], pos - old_pos);
            /* We are in "linear" mode */
            /* Process data directly by subtracting "pointers" */
        	gps_uart_process_data(&gps_rx_dma_buffer[old_pos], pos - old_pos);
        } else {

        	SCB_InvalidateDCache_by_Addr((uint32_t *)&gps_rx_dma_buffer[old_pos], ARRAY_LEN(gps_rx_dma_buffer) - old_pos);
            /* We are in "overflow" mode */
            /* First process data to the end of buffer */
        	gps_uart_process_data(&gps_rx_dma_buffer[old_pos], ARRAY_LEN(gps_rx_dma_buffer) - old_pos);
            /* Check and continue with beginning of buffer */
            if (pos > 0) {
            	SCB_InvalidateDCache_by_Addr((uint32_t *)&gps_rx_dma_buffer[0], pos);
            	gps_uart_process_data(&gps_rx_dma_buffer[0], pos);
            }
        }
    }
    old_pos = pos;                              /* Save current position as old */

    /* Check and manually update if we reached end of buffer */
    if (old_pos == ARRAY_LEN(gps_rx_dma_buffer)) {
        old_pos = 0;
    }
}

/**
 * \brief           Check if DMA is active and if not try to send data
 */
#ifdef GPS_USE_TX
static uint8_t gps_uart_start_tx_dma_transfer(void)
{
    uint32_t old_primask;
    uint8_t started = 0;

    /* Check if transfer active */
    if (gps_tx_dma_current_len > 0) {
        return 0;
    }

    /* Check if DMA is active */
    /* Must be set to 0 */
    //old_primask = __get_PRIMASK();
    //__disable_irq();

    /* Check if transfer is not active */
    if (gps_tx_dma_current_len == 0
            && (gps_tx_dma_current_len = lwrb_get_linear_block_read_length(&gps_tx_dma_ringbuff)) > 0) {
        /* Disable channel if enabled */
        LL_DMA_DisableStream(GPS_UART_DMA, GPS_UART_DMA_TX_STREAM);

        /* Clear all flags */
        LL_DMA_ClearFlag_TC1(GPS_UART_DMA);
        LL_DMA_ClearFlag_HT1(GPS_UART_DMA);
        LL_DMA_ClearFlag_TE1(GPS_UART_DMA);
        LL_DMA_ClearFlag_DME1(GPS_UART_DMA);
        LL_DMA_ClearFlag_FE1(GPS_UART_DMA);

        uint32_t addr =  (uint32_t)lwrb_get_linear_block_read_address(&gps_tx_dma_ringbuff);

        SCB_CleanDCache_by_Addr((uint32_t *)addr, gps_tx_dma_current_len);

        /* Start DMA transfer */
        LL_DMA_SetDataLength(GPS_UART_DMA, GPS_UART_DMA_TX_STREAM, gps_tx_dma_current_len);
        LL_DMA_SetMemoryAddress(GPS_UART_DMA, GPS_UART_DMA_TX_STREAM, addr);

        /* Start new transfer */
        LL_DMA_EnableStream(GPS_UART_DMA, GPS_UART_DMA_TX_STREAM);
        started = 1;
    }

    //__set_PRIMASK(old_primask);
    return started;
}
#endif

/**
 * \brief           USART Initialization Function
 */
static void gps_uart_usart_init(void)
{
	RCC_PeriphCLKInitTypeDef RCC_PeriphClkInit;
    LL_USART_InitTypeDef USART_InitStruct = {0};
    LL_GPIO_InitTypeDef GPIO_InitStruct = {0};

	// Set uart clock source
    RCC_PeriphClkInit.PeriphClockSelection  = RCC_PERIPHCLK_USART6;
    RCC_PeriphClkInit.Usart16ClockSelection = RCC_USART16CLKSOURCE_D2PCLK2;
    HAL_RCCEx_PeriphCLKConfig(&RCC_PeriphClkInit);

    // Peripheral clock enable
    GPS_UART_CLK_ENABLE();
    GPS_DMA_CLK_ENABLE();

    GPIO_InitStruct.Mode 		= LL_GPIO_MODE_ALTERNATE;
    GPIO_InitStruct.Speed 		= LL_GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.OutputType	= LL_GPIO_OUTPUT_PUSHPULL;
    GPIO_InitStruct.Alternate 	= LL_GPIO_AF_7;

    // PG9 - GPS_UART RX
    GPIO_InitStruct.Pin 		= GPS_RX_PIN;
    GPIO_InitStruct.Pull 		= LL_GPIO_PULL_UP;
    LL_GPIO_Init(GPS_RX_PORT, &GPIO_InitStruct);

    // PG14 - GPS_UART TX
	#ifdef GPS_USE_TX
    GPIO_InitStruct.Pin 		= GPS_TX_PIN;
    GPIO_InitStruct.Pull 		= LL_GPIO_PULL_NO;
    LL_GPIO_Init(GPS_TX_PORT, &GPIO_InitStruct);
	#endif

    // USART RX Init
    LL_DMA_SetPeriphRequest			(GPS_UART_DMA, GPS_UART_RX_DMA_STREAM, GPS_UART_RX_DMA_REQUEST);
    LL_DMA_SetDataTransferDirection	(GPS_UART_DMA, GPS_UART_RX_DMA_STREAM, LL_DMA_DIRECTION_PERIPH_TO_MEMORY);
    LL_DMA_SetStreamPriorityLevel	(GPS_UART_DMA, GPS_UART_RX_DMA_STREAM, LL_DMA_PRIORITY_LOW);
    LL_DMA_SetMode					(GPS_UART_DMA, GPS_UART_RX_DMA_STREAM, LL_DMA_MODE_CIRCULAR);
    LL_DMA_SetPeriphIncMode			(GPS_UART_DMA, GPS_UART_RX_DMA_STREAM, LL_DMA_PERIPH_NOINCREMENT);
    LL_DMA_SetMemoryIncMode			(GPS_UART_DMA, GPS_UART_RX_DMA_STREAM, LL_DMA_MEMORY_INCREMENT);
    LL_DMA_SetPeriphSize			(GPS_UART_DMA, GPS_UART_RX_DMA_STREAM, LL_DMA_PDATAALIGN_BYTE);
    LL_DMA_SetMemorySize			(GPS_UART_DMA, GPS_UART_RX_DMA_STREAM, LL_DMA_MDATAALIGN_BYTE);
    LL_DMA_DisableFifoMode			(GPS_UART_DMA, GPS_UART_RX_DMA_STREAM);
    LL_DMA_SetPeriphAddress			(GPS_UART_DMA, GPS_UART_RX_DMA_STREAM, LL_USART_DMA_GetRegAddr(GPS_UART, LL_USART_DMA_REG_DATA_RECEIVE));
    LL_DMA_SetMemoryAddress			(GPS_UART_DMA, GPS_UART_RX_DMA_STREAM, (uint32_t)gps_rx_dma_buffer);
    LL_DMA_SetDataLength			(GPS_UART_DMA, GPS_UART_RX_DMA_STREAM, ARRAY_LEN(gps_rx_dma_buffer));

    // USART TX Init
	#ifdef GPS_USE_TX
    LL_DMA_SetPeriphRequest			(GPS_UART_DMA, GPS_UART_DMA_TX_STREAM, GPS_UART_TX_DMA_REQUEST);
    LL_DMA_SetDataTransferDirection	(GPS_UART_DMA, GPS_UART_DMA_TX_STREAM, LL_DMA_DIRECTION_MEMORY_TO_PERIPH);
    LL_DMA_SetStreamPriorityLevel	(GPS_UART_DMA, GPS_UART_DMA_TX_STREAM, LL_DMA_PRIORITY_LOW);
    LL_DMA_SetMode					(GPS_UART_DMA, GPS_UART_DMA_TX_STREAM, LL_DMA_MODE_NORMAL);
    LL_DMA_SetPeriphIncMode			(GPS_UART_DMA, GPS_UART_DMA_TX_STREAM, LL_DMA_PERIPH_NOINCREMENT);
    LL_DMA_SetMemoryIncMode			(GPS_UART_DMA, GPS_UART_DMA_TX_STREAM, LL_DMA_MEMORY_INCREMENT);
    LL_DMA_SetPeriphSize			(GPS_UART_DMA, GPS_UART_DMA_TX_STREAM, LL_DMA_PDATAALIGN_BYTE);
    LL_DMA_SetMemorySize			(GPS_UART_DMA, GPS_UART_DMA_TX_STREAM, LL_DMA_MDATAALIGN_BYTE);
    LL_DMA_DisableFifoMode			(GPS_UART_DMA, GPS_UART_DMA_TX_STREAM);
    LL_DMA_SetPeriphAddress			(GPS_UART_DMA, GPS_UART_DMA_TX_STREAM, LL_USART_DMA_GetRegAddr(GPS_UART, LL_USART_DMA_REG_DATA_TRANSMIT));
	#endif

    // Enable DMA RX HT & TC interrupts
    LL_DMA_EnableIT_HT				(GPS_UART_DMA, GPS_UART_RX_DMA_STREAM);
    LL_DMA_EnableIT_TC				(GPS_UART_DMA, GPS_UART_RX_DMA_STREAM);

    // Enable DMA TX TC interrupts
	#ifdef GPS_USE_TX
    LL_DMA_EnableIT_TC				(GPS_UART_DMA, GPS_UART_DMA_TX_STREAM);
	#endif

    // DMA interrupt init
    NVIC_SetPriority				(GPS_UART_DMA_RX_IRQn, NVIC_EncodePriority(NVIC_GetPriorityGrouping(), 5, 0));
    NVIC_EnableIRQ					(GPS_UART_DMA_RX_IRQn);
	#ifdef GPS_USE_TX
    NVIC_SetPriority				(GPS_UART_DMA_TX_IRQn, NVIC_EncodePriority(NVIC_GetPriorityGrouping(), 5, 0));
    NVIC_EnableIRQ					(GPS_UART_DMA_TX_IRQn);
	#endif

    // Configure USART
    USART_InitStruct.PrescalerValue			= LL_USART_PRESCALER_DIV1;
    USART_InitStruct.BaudRate 				= GPS_UART_SPEED;
    USART_InitStruct.DataWidth 				= LL_USART_DATAWIDTH_8B;
    USART_InitStruct.StopBits 				= LL_USART_STOPBITS_1;
    USART_InitStruct.Parity 				= LL_USART_PARITY_NONE;
    USART_InitStruct.TransferDirection 		= LL_USART_DIRECTION_TX_RX;
    USART_InitStruct.HardwareFlowControl	= LL_USART_HWCONTROL_NONE;
    USART_InitStruct.OverSampling 			= LL_USART_OVERSAMPLING_16;

    LL_USART_Init					(GPS_UART, &USART_InitStruct);

    LL_USART_SetTXFIFOThreshold		(GPS_UART, LL_USART_FIFOTHRESHOLD_7_8);
    LL_USART_SetRXFIFOThreshold		(GPS_UART, LL_USART_FIFOTHRESHOLD_7_8);
    LL_USART_EnableFIFO				(GPS_UART);
    LL_USART_ConfigAsyncMode		(GPS_UART);
    LL_USART_EnableDMAReq_RX		(GPS_UART);
	#ifdef GPS_USE_TX
    LL_USART_EnableDMAReq_TX		(GPS_UART);
	#endif
    LL_USART_EnableIT_IDLE			(GPS_UART);

    // USART interrupt, same priority as DMA channel
    NVIC_SetPriority				(GPS_UART_IRQn, NVIC_EncodePriority(NVIC_GetPriorityGrouping(), 5, 0));
    NVIC_EnableIRQ  				(GPS_UART_IRQn);

	SCB_CleanDCache_by_Addr((uint32_t *)gps_rx_dma_buffer, 	  ARRAY_LEN(gps_rx_dma_buffer));
	//--SCB_CleanDCache_by_Addr((uint32_t *)gps_rx_dma_lwrb_data, ARRAY_LEN(gps_rx_dma_lwrb_data));
	#ifdef GPS_USE_TX
	SCB_CleanDCache_by_Addr((uint32_t *)gps_tx_dma_lwrb_data, ARRAY_LEN(gps_tx_dma_lwrb_data));
	#endif

    // Enable USART and DMA RX
    LL_DMA_EnableStream				(GPS_UART_DMA, GPS_UART_RX_DMA_STREAM);
    LL_USART_Enable					(GPS_UART);

    // Polling USART initialisation
    while (!LL_USART_IsActiveFlag_TEACK(GPS_UART) || !LL_USART_IsActiveFlag_REACK(GPS_UART)) {}
}

static void gps_uart_usart_cleanup(void)
{
	LL_USART_Disable		(GPS_UART);
	LL_DMA_DisableStream	(GPS_UART_DMA, GPS_UART_RX_DMA_STREAM);
	NVIC_DisableIRQ  		(GPS_UART_IRQn);
}

#ifdef GPS_USE_TX
ushort gps_uart_send(uchar *buff, ushort size)
{
	//printf("tx buffer: %d\r\n", gps_tx_dma_current_len);
	if(lwrb_write(&gps_tx_dma_ringbuff, buff, size) != size)
		return 1;

	if(gps_uart_start_tx_dma_transfer() == 0)
		return 2;

	return 0;
}
#endif

//*----------------------------------------------------------------------------
//* Function Name       : gps_uart_flush
//* Object              :
//* Notes    			: Flush ring buffer before command exchange
//* Notes    			:
//* Context    			: CONTEXT_GPS_PROC
//*----------------------------------------------------------------------------
void gps_uart_flush(void)
{
	//--lwrb_reset(&gps_rx_dma_ringbuff);
}

//*----------------------------------------------------------------------------
//* Function Name       : gps_uart_init
//* Object              :
//* Notes    			: Low level driver init
//* Notes    			:
//* Context    			: CONTEXT_GPS_PROC
//*----------------------------------------------------------------------------
void gps_uart_init(void)
{
    // Initialise ringbuff for TX & RX
	#ifdef GPS_USE_TX
	lwrb_init(&gps_tx_dma_ringbuff, gps_tx_dma_lwrb_data, sizeof(gps_tx_dma_lwrb_data));
	#endif
    //--lwrb_init(&gps_rx_dma_ringbuff, gps_rx_dma_lwrb_data, sizeof(gps_rx_dma_lwrb_data));

    gps_uart_usart_init();

    //--printf("init done\r\n");
}

void gps_uart_stop(void)
{
	gps_uart_usart_cleanup();
}

#endif
