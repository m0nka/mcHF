/* gps_driver.c – GPS task for STM32H747 M7 / FreeRTOS
 *
 * Design overview
 * ---------------
 *  • USART6 RX uses circular DMA.  The UART IDLE-line interrupt fires at
 *    the end of each burst and drains whatever DMA has written since the
 *    last read, assembling complete NMEA sentences.
 *
 *  • Complete sentences are posted to a FreeRTOS message queue.  No parsing
 *    occurs in IRQ context, so there are no re-entrancy hazards.
 *
 *  • GPS_Task() dequeues sentences, parses $GNRMC / $GNGGA, and keeps a
 *    staging buffer (gps_pending) that is only touched by the task.
 *
 *  • u-blox M10 PPS timing: the PPS pulse fires at the exact UTC second
 *    boundary; the NMEA sentence for that same second is transmitted
 *    50-200 ms AFTERWARDS.  The driver therefore uses a two-step latch:
 *
 *      1. PPS ISR sets pps_pending = true and records the tick timestamp.
 *      2. When GPS_ParseRMC() succeeds and pps_pending is set (within a
 *         1.5 s window), it releases pps_sem and clears pps_pending.
 *      3. GPS_Task() sees pps_sem and calls GPS_SyncRTC() with the freshly
 *         parsed time — which is exactly the second whose boundary PPS
 *         just marked.
 *
 *    This avoids the off-by-one-second error that occurs if the RTC is
 *    written at PPS time using the *previous* second's NMEA data.
 */
#include "main.h"
#include "mchf_pro_board.h"

#ifdef CONTEXT_GPS

#include "gps_test.h"

#include "gps_proc.h"
#include "rtc.h"        /* CubeMX-generated hrtc extern                     */
#include <string.h>
#include <stdlib.h>
#include <math.h>

/* --------------------------------------------------------------------------
 * Internal types
 * -------------------------------------------------------------------------- */
typedef struct {
    char data[GPS_NMEA_MAX_LEN];
} NMEA_Line_t;

/* --------------------------------------------------------------------------
 * Peripheral handles
 * -------------------------------------------------------------------------- */
static UART_HandleTypeDef  huart6;
static DMA_HandleTypeDef   hdma_rx;

/* --------------------------------------------------------------------------
 * DMA receive buffer (circular, filled by hardware)
 * -------------------------------------------------------------------------- */
__attribute__((section(".axi_mem"))) __attribute__ ((aligned (32))) uint8_t dma_buf[GPS_DMA_BUF_SIZE];
static uint16_t dma_rd;           /* read pointer (software side)            */

/* --------------------------------------------------------------------------
 * NMEA line assembler (ISR-local state)
 * -------------------------------------------------------------------------- */
static char    nmea_asm[GPS_NMEA_MAX_LEN];
static uint8_t nmea_asm_idx;
static bool    nmea_in_sentence;   /* saw '$', waiting for '\n'              */

/* --------------------------------------------------------------------------
 * RTOS objects
 * -------------------------------------------------------------------------- */
static QueueHandle_t     nmea_queue;   /* NMEA_Line_t items                */
static SemaphoreHandle_t pps_sem;      /* binary, released by GPS_ParseRMC */
static SemaphoreHandle_t data_mutex;   /* protects gps_data for readers    */

/* --------------------------------------------------------------------------
 * PPS latch (written in ISR, read in task)
 * -------------------------------------------------------------------------- */
static volatile bool     pps_pending;    /* PPS fired, waiting for next RMC  */
static volatile uint32_t pps_pending_ms; /* HAL_GetTick() at PPS edge        */

/* --------------------------------------------------------------------------
 * Shared data
 * -------------------------------------------------------------------------- */
static GPS_Data_t gps_data;      /* published, mutex-protected              */
static GPS_Data_t gps_pending;   /* staging, written only by GPS_Task()     */

extern RTC_HandleTypeDef RtcHandle;

/* --------------------------------------------------------------------------
 * Forward declarations
 * -------------------------------------------------------------------------- */
static void GPS_GPIO_Init(void);
static void GPS_DMA_Init(void);
static void GPS_UART_Init(void);
static void GPS_EXTI_Init(void);
static void GPS_DrainDMA(void);
static void GPS_ProcessLine(const char *line);
static bool GPS_ParseRMC(const char *line);
static bool GPS_ParseGGA(const char *line);
static void GPS_SyncRTC(const GPS_Data_t *d);
static uint8_t GPS_Checksum(const char *s);
static double  GPS_NMEADeg(const char *field, char hemi);
static int     GPS_Split(const char *src, char *buf, char **fields, int max);
static uint8_t GPS_DayOfWeek(uint16_t y, uint8_t m, uint8_t d);

void USART6_IRQHandler(void)
{
	//--printf("uart \r\n");
	GPS_UART_IRQHandler();
}

void DMA1_Stream2_IRQHandler(void)
{
	//--printf("dma \r\n");
	GPS_DMA_IRQHandler();
}

//void EXTI9_5_IRQHandler(void)
//{
//	GPS_PPS_IRQHandler();
//}

/* ==========================================================================
 * Initialisation
 * ========================================================================== */

void gps_proc_init(void)
{
    // RTOS primitives
    nmea_queue = xQueueCreate(GPS_NMEA_QUEUE_DEPTH, sizeof(NMEA_Line_t));
    pps_sem    = xSemaphoreCreateBinary();
    data_mutex = xSemaphoreCreateMutex();

	#ifndef GPS_TEST_GPIO
    GPS_GPIO_Init();
    GPS_DMA_Init();
    GPS_UART_Init();

    // EXTI mapping, to be resolved...
	#ifndef CONTEXT_KEYPAD
    GPS_EXTI_Init();
	#endif

    GPS_Enable(true);

    // Flush
    HAL_UART_Receive(&huart6, dma_buf, 1, 100);

    vTaskDelay(pdMS_TO_TICKS(150));  /* M10 needs ~100 ms to boot before it starts sending NMEA */
	#else
    gps_test_init();
	#endif

    // Test, config command
	#if 0
    uint8_t ubx_cfg_prt_nmea[] = {
        0xB5, 0x62, // UBX header
        0x06, 0x00, // Class 0x06 = CFG, ID 0x00 = PRT
        0x14, 0x00, // Length = 20 bytes
        0x01, 0x00, // PortID = UART1
        0x00, 0x00, // reserved
        0x00, 0x00, // TX Ready
        0x08, 0x00, 0x00, 0x00, // 9600 baud (unchanged)
        0x07, 0x00, // 8N1
        0x00, 0x00, // inFlags: no in
        0x00, 0x00, // outFlags: NMEA
        0x00, 0x00, // reserved
        0x00, 0x00, // reserved
        0x00, 0x00, // reserved
        0x00, 0x00, // reserved
        0x96, 0x90 // CK_A, CK_B checksum
    };
    HAL_UART_Transmit(&huart6, ubx_cfg_prt_nmea, sizeof(ubx_cfg_prt_nmea), 100);

    //__HAL_UART_DISABLE(&huart6);
    //huart6.Init.BaudRate     = 115200;
    //HAL_UART_Init(&huart6);
    //__HAL_UART_ENABLE(&huart6);
	#endif

    // Clear buffer
    SCB_InvalidateDCache_by_Addr((uint32_t*)dma_buf, GPS_DMA_BUF_SIZE);
    memset(dma_buf, 0, GPS_DMA_BUF_SIZE);

    // Start RX
    HAL_UART_Receive_DMA(&huart6, dma_buf, GPS_DMA_BUF_SIZE);
    dma_rd = 0;
}

/* -------------------------------------------------------------------------- */
#ifndef GPS_TEST_GPIO
static void GPS_GPIO_Init(void)
{
    GPIO_InitTypeDef cfg = {0};

    /* PB1 – GPS_EN, push-pull output, start LOW (disabled) */
    cfg.Pin   = GPS_EN_PIN;
    cfg.Mode  = GPIO_MODE_OUTPUT_PP;
    cfg.Pull  = GPIO_NOPULL;
    cfg.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPS_EN_PORT, &cfg);
    HAL_GPIO_WritePin(GPS_EN_PORT, GPS_EN_PIN, GPIO_PIN_RESET);

    /* PG9 – USART6_RX */
    cfg.Pin       = GPS_RX_PIN;
    cfg.Mode      = GPIO_MODE_AF_PP;
    cfg.Pull      = GPIO_PULLUP;
    cfg.Speed     = GPIO_SPEED_FREQ_VERY_HIGH;
    cfg.Alternate = GPS_UART_AF;
    HAL_GPIO_Init(GPS_RX_PORT, &cfg);

    /* PG14 – USART6_TX */
    cfg.Pin  = GPS_TX_PIN;
    cfg.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPS_TX_PORT, &cfg);

    /* PA8 – PPS input, rising-edge EXTI, no pull (module drives it) */
    cfg.Pin  = GPS_PPS_PIN;
    cfg.Mode = GPIO_MODE_IT_RISING;
    cfg.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPS_PPS_PORT, &cfg);
}

/* -------------------------------------------------------------------------- */
static void GPS_DMA_Init(void)
{
    __HAL_RCC_DMA1_CLK_ENABLE();

    hdma_rx.Instance                 = GPS_DMA_STREAM;
    hdma_rx.Init.Request             = GPS_DMA_REQUEST;
    hdma_rx.Init.Direction           = DMA_PERIPH_TO_MEMORY;
    hdma_rx.Init.PeriphInc           = DMA_PINC_DISABLE;
    hdma_rx.Init.MemInc              = DMA_MINC_ENABLE;
    hdma_rx.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
    hdma_rx.Init.MemDataAlignment    = DMA_MDATAALIGN_BYTE;
    hdma_rx.Init.Mode                = DMA_CIRCULAR;
    hdma_rx.Init.Priority            = DMA_PRIORITY_HIGH;
    hdma_rx.Init.FIFOMode            = DMA_FIFOMODE_DISABLE;
    HAL_DMA_Init(&hdma_rx);

    __HAL_LINKDMA(&huart6, hdmarx, hdma_rx);

    HAL_NVIC_SetPriority(GPS_DMA_IRQn, 5, 0);
    HAL_NVIC_EnableIRQ  (GPS_DMA_IRQn);
}

/* -------------------------------------------------------------------------- */
static void GPS_UART_Init(void)
{
    RCC_PeriphCLKInitTypeDef PeriphClkInitStruct = {0};

    // USART6 clock config - H7 has separate kernel clock
    PeriphClkInitStruct.PeriphClockSelection = RCC_PERIPHCLK_USART6;
    PeriphClkInitStruct.Usart16ClockSelection = RCC_USART16CLKSOURCE_D2PCLK2;
    HAL_RCCEx_PeriphCLKConfig(&PeriphClkInitStruct);

    __HAL_RCC_USART6_CLK_ENABLE();

    huart6.Instance          = GPS_UART;
    huart6.Init.BaudRate     = GPS_UART_BAUD;
    huart6.Init.WordLength   = UART_WORDLENGTH_8B;
    huart6.Init.StopBits     = UART_STOPBITS_1;
    huart6.Init.Parity       = UART_PARITY_NONE;
    huart6.Init.Mode         = UART_MODE_TX_RX;
    huart6.Init.HwFlowCtl    = UART_HWCONTROL_NONE;
    huart6.Init.OverSampling = UART_OVERSAMPLING_16;
    huart6.Init.ClockPrescaler = UART_PRESCALER_DIV1;
    HAL_UART_Init(&huart6);

    __HAL_UART_ENABLE_IT(&huart6, UART_IT_IDLE);

    HAL_NVIC_SetPriority(GPS_UART_IRQn, 5, 0);
    HAL_NVIC_EnableIRQ(GPS_UART_IRQn);

    //HAL_UART_Receive_DMA(&huart6, dma_buf, GPS_DMA_BUF_SIZE);
    //dma_rd = 0;
}
#endif

/* -------------------------------------------------------------------------- */
#ifndef CONTEXT_KEYPAD
static void GPS_EXTI_Init(void)
{
    /* GPIO already configured in GPIO_Init; just enable the NVIC line */
    HAL_NVIC_SetPriority(GPS_PPS_EXTI_IRQn, 5, 0);  /* higher than UART    */
    HAL_NVIC_EnableIRQ(GPS_PPS_EXTI_IRQn);
}
#endif

/* ==========================================================================
 * Public API
 * ========================================================================== */

void GPS_Enable(bool enable)
{
    HAL_GPIO_WritePin(GPS_EN_PORT, GPS_EN_PIN,
                      enable ? GPIO_PIN_SET : GPIO_PIN_RESET);
}

bool GPS_GetData(GPS_Data_t *out)
{
    if (!out) return false;
    xSemaphoreTake(data_mutex, portMAX_DELAY);
    *out = gps_data;
    xSemaphoreGive(data_mutex);
    return gps_data.valid;
}

// Drain DMA ring buffer, assemble complete NMEA lines, queue them
static void GPS_DrainDMA(void)
{
	uint16_t cnt = __HAL_DMA_GET_COUNTER(&hdma_rx);
    uint16_t wr = (uint16_t)(GPS_DMA_BUF_SIZE - cnt);

    //printf("cnt %d, %d \r\n", (int)cnt, (int)wr);

    //SCB_InvalidateDCache_by_Addr((uint32_t*)(dma_buf + dma_rd), cnt);
    SCB_InvalidateDCache_by_Addr((uint32_t*)dma_buf, GPS_DMA_BUF_SIZE);

    while (dma_rd != wr)
    {
        char c = (char)dma_buf[dma_rd];
        dma_rd = (uint16_t)((dma_rd + 1u) % GPS_DMA_BUF_SIZE);

        //--printf("byte %x \r\n" , c);

        if (c == '$')
        {
            nmea_asm_idx    = 0;
            nmea_in_sentence = true;
        }

        if (!nmea_in_sentence) continue;

        if (nmea_asm_idx < GPS_NMEA_MAX_LEN - 1)
            nmea_asm[nmea_asm_idx++] = c;

        if (c == '\n')
        {
            nmea_asm[nmea_asm_idx] = '\0';
            NMEA_Line_t msg;
            memcpy(msg.data, nmea_asm, GPS_NMEA_MAX_LEN);

            printf("data: %s \r\n", msg.data);

            // Non-blocking put from ISR context
            xQueueSendFromISR(nmea_queue, &msg, NULL);
            nmea_asm_idx     = 0;
            nmea_in_sentence = false;
        }
    }
}

void GPS_UART_IRQHandler(void)
{
#if 0
    /* Service DMA completion / error flags first */
    HAL_UART_IRQHandler(&huart6);

    if (__HAL_UART_GET_FLAG(&huart6, UART_FLAG_IDLE))
    {
        __HAL_UART_CLEAR_IDLEFLAG(&huart6);
        GPS_DrainDMA();
    }
#else
	// Snapshot IDLE before HAL_UART_IRQHandler clears it
    bool idle = __HAL_UART_GET_FLAG(&huart6, UART_FLAG_IDLE)
               && __HAL_UART_GET_IT_SOURCE(&huart6, UART_IT_IDLE);

    if(idle)
    	__HAL_UART_CLEAR_IDLEFLAG(&huart6);

    HAL_UART_IRQHandler(&huart6);

   if(huart6.ErrorCode)
	   printf("error: %d \r\n", (int)huart6.ErrorCode);
   else if(idle)
      GPS_DrainDMA();
#endif
}

void GPS_PPS_IRQHandler(void)
{
    if (__HAL_GPIO_EXTI_GET_IT(GPS_PPS_PIN))
    {
        __HAL_GPIO_EXTI_CLEAR_IT(GPS_PPS_PIN);
        /* Latch the timestamp; semaphore is released by GPS_ParseRMC()
         * once the NMEA sentence for this second has been received.        */
        pps_pending_ms           = HAL_GetTick();
        gps_pending.last_pps_ms  = pps_pending_ms;
        pps_pending              = true;
    }
}

void GPS_DMA_IRQHandler(void)
{
    HAL_DMA_IRQHandler(&hdma_rx);
}

/* ==========================================================================
 * FreeRTOS task
 * ========================================================================== */

void gps_proc(void *argument)
{
    (void)argument;
    NMEA_Line_t line;

	vTaskDelay(GPS_PROC_START_DELAY);
	printf("start\r\n");

	gps_proc_init();

gps_proc_loop:

	#ifndef GPS_TEST_GPIO
    // Process any waiting NMEA sentences
	while (xQueueReceive(nmea_queue, &line, 0) == pdTRUE)
    {
      	printf("%s  \r\n", line.data);
        GPS_ProcessLine(line.data);
    }

    // Block up to 100 ms for the next sentence
    if (xQueueReceive(nmea_queue, &line, pdMS_TO_TICKS(100)) == pdTRUE)
    {
     	printf("%s  \r\n", line.data);
        GPS_ProcessLine(line.data);
    }

    // Non-blocking PPS check
    if (xSemaphoreTake(pps_sem, 0) == pdTRUE)
    {
        xSemaphoreTake(data_mutex, portMAX_DELAY);

        if (gps_pending.valid)
        {
            GPS_SyncRTC(&gps_pending);
            gps_pending.rtc_synced = true;
            gps_data = gps_pending;
        }

        xSemaphoreGive(data_mutex);
     }
	#else
    gps_test_run();
	#endif

    goto gps_proc_loop;
}

/* ==========================================================================
 * NMEA parsing
 * ========================================================================== */

/* XOR checksum of characters between '$' and '*' (exclusive) */
static uint8_t GPS_Checksum(const char *s)
{
    uint8_t cs = 0;
    for (const char *p = s + 1; *p && *p != '*'; ++p)
        cs ^= (uint8_t)*p;
    return cs;
}

/* Destructively split comma-separated NMEA into field pointers */
static int GPS_Split(const char *src, char *buf, char **fields, int max)
{
    strncpy(buf, src, GPS_NMEA_MAX_LEN - 1);
    buf[GPS_NMEA_MAX_LEN - 1] = '\0';
    int n = 0;
    char *p = buf;
    while (*p && n < max)
    {
        fields[n++] = p;
        char *comma = strchr(p, ',');
        if (!comma) break;
        *comma = '\0';
        p = comma + 1;
    }
    return n;
}

/* Verify checksum and dispatch to sentence-specific parser */
static void GPS_ProcessLine(const char *line)
{
    const char *star = strchr(line, '*');
    if (!star) return;

    uint8_t expected = GPS_Checksum(line);
    uint8_t received = (uint8_t)strtol(star + 1, NULL, 16);
    if (expected != received) return;

    if      (strncmp(line, "$GNRMC", 6) == 0 ||
             strncmp(line, "$GPRMC", 6) == 0)  GPS_ParseRMC(line);
    else if (strncmp(line, "$GNGGA", 6) == 0 ||
             strncmp(line, "$GPGGA", 6) == 0)  GPS_ParseGGA(line);
}

/* Convert NMEA ddmm.mmmmm + N/S or E/W to decimal degrees */
static double GPS_NMEADeg(const char *field, char hemi)
{
    if (!field || !field[0]) return 0.0;
    double raw = atof(field);
    int    deg = (int)(raw / 100.0);
    double min = raw - (deg * 100.0);
    double dec = (double)deg + min / 60.0;
    if (hemi == 'S' || hemi == 'W') dec = -dec;
    return dec;
}

/*
 * $GNRMC,hhmmss.ss,A,llll.ll,N,yyyyy.yy,E,x.x,x.x,ddmmyy,,,A*hh
 *  [1] UTC time    [2] Status (A/V)
 *  [3] Lat         [4] N/S
 *  [5] Lon         [6] E/W
 *  [7] Speed (kn)  [8] Course
 *  [9] Date ddmmyy
 */
static bool GPS_ParseRMC(const char *line)
{
    char  buf[GPS_NMEA_MAX_LEN];
    char *f[20];
    if (GPS_Split(line, buf, f, 20) < 10) return false;

    if (f[2][0] != 'A')
    {
        gps_pending.valid = false;
        return false;
    }

    /* Time: hhmmss[.ss] */
    const char *t = f[1];
    if (strlen(t) < 6) return false;
    gps_pending.hour = (uint8_t)((t[0]-'0') * 10 + (t[1]-'0'));
    gps_pending.min  = (uint8_t)((t[2]-'0') * 10 + (t[3]-'0'));
    gps_pending.sec  = (uint8_t)((t[4]-'0') * 10 + (t[5]-'0'));
    gps_pending.msec = (strlen(t) > 7)
                     ? (uint16_t)(atof(t + 6) * 1000.0)
                     : 0u;

    /* Date: ddmmyy */
    const char *d = f[9];
    if (strlen(d) >= 6)
    {
        gps_pending.day   = (uint8_t) ((d[0]-'0') * 10 + (d[1]-'0'));
        gps_pending.month = (uint8_t) ((d[2]-'0') * 10 + (d[3]-'0'));
        gps_pending.year  = (uint16_t)(2000 + (d[4]-'0') * 10 + (d[5]-'0'));
    }

    gps_pending.latitude    = GPS_NMEADeg(f[3], f[4][0]);
    gps_pending.longitude   = GPS_NMEADeg(f[5], f[6][0]);
    gps_pending.speed_kn    = (float)atof(f[7]);
    gps_pending.course_deg  = (float)atof(f[8]);
    gps_pending.valid       = true;

    /* M10 sends NMEA after PPS: if PPS fired recently this sentence carries
     * the time for that second, so it is safe to sync the RTC now.         */
    if (pps_pending)
    {
        pps_pending = false;
        if ((HAL_GetTick() - pps_pending_ms) < 1500u)
            xSemaphoreGive(pps_sem);  // triggers RTC write in task
        /* else: PPS was stale (>1.5 s old) – discard silently             */
    }

    return true;
}

/*
 * $GNGGA,hhmmss.ss,llll.ll,N,yyyyy.yy,E,x,xx,x.x,x.x,M,...*hh
 *  [6] Fix quality  [7] Satellites  [8] HDOP  [9] Altitude (M)
 */
static bool GPS_ParseGGA(const char *line)
{
    char  buf[GPS_NMEA_MAX_LEN];
    char *f[20];
    if (GPS_Split(line, buf, f, 20) < 10) return false;

    gps_pending.fix_quality = (uint8_t)atoi(f[6]);
    gps_pending.satellites  = (uint8_t)atoi(f[7]);
    gps_pending.hdop        = (float)  atof(f[8]);
    gps_pending.altitude_m  = (float)  atof(f[9]);
    return true;
}

/* ==========================================================================
 * RTC synchronisation
 * ========================================================================== */

/* Tomohiko Sakamoto's compact day-of-week: 0=Sun … 6=Sat.
 * HAL uses RTC_WEEKDAY_MONDAY=1 … RTC_WEEKDAY_SUNDAY=7.                    */
static uint8_t GPS_DayOfWeek(uint16_t y, uint8_t m, uint8_t d)
{
    static const uint8_t t[] = {0,3,2,5,0,3,5,1,4,6,2,4};
    if (m < 3) y--;
    uint8_t dow = (uint8_t)((y + y/4 - y/100 + y/400 + t[m-1] + d) % 7);
    /* Convert: 0=Sun→7, 1=Mon→1 … 6=Sat→6                                 */
    return (dow == 0) ? RTC_WEEKDAY_SUNDAY : dow;
}

static void GPS_SyncRTC(const GPS_Data_t *d)
{
    RTC_TimeTypeDef rt = {0};
    RTC_DateTypeDef rd = {0};

    rt.Hours          = d->hour;
    rt.Minutes        = d->min;
    rt.Seconds        = d->sec;
    rt.DayLightSaving = RTC_DAYLIGHTSAVING_NONE;
    rt.StoreOperation = RTC_STOREOPERATION_RESET;

    rd.Date    = d->day;
    rd.Month   = d->month;              /* HAL binary format: 1-12           */
    rd.Year    = (uint8_t)(d->year - 2000u);
    rd.WeekDay = GPS_DayOfWeek(d->year, d->month, d->day);

    /* HAL requires time to be set before date */
    HAL_RTC_SetTime(&RtcHandle, &rt, RTC_FORMAT_BIN);
    HAL_RTC_SetDate(&RtcHandle, &rd, RTC_FORMAT_BIN);
}

#endif
