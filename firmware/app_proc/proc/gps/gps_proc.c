/* GPS task for STM32H747 M7 / FreeRTOS
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
#include "gps_uart.h"

#include "gps_proc.h"
#include "rtc.h"
#include <string.h>
#include <stdlib.h>
#include <math.h>

#ifdef GPS_INT_NOISE_TEST
#include "shared_tim.h"
#endif

/* --------------------------------------------------------------------------
 * Internal types
 * -------------------------------------------------------------------------- */
typedef struct {
    char data[GPS_NMEA_MAX_LEN];
} NMEA_Line_t;

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
static volatile bool     pps_pending = true;    /* PPS fired, waiting for next RMC  */
static volatile uint32_t pps_pending_ms; /* HAL_GetTick() at PPS edge        */

/* --------------------------------------------------------------------------
 * Shared data
 * -------------------------------------------------------------------------- */
static GPS_Data_t gps_data;      /* published, mutex-protected              */
static GPS_Data_t gps_pending;   /* staging, written only by GPS_Task()     */

extern RTC_HandleTypeDef RtcHandle;

// FreeRTOS process state
extern struct PROC_STATE 				ps;

/* --------------------------------------------------------------------------
 * Forward declarations
 * -------------------------------------------------------------------------- */
static void GPS_GPIO_Init(void);
//static void GPS_EXTI_Init(void);
//static void GPS_DrainDMA(void);
static void GPS_ProcessLine(const char *line);
static bool GPS_ParseRMC(const char *line);
static bool GPS_ParseGGA(const char *line);
static void GPS_SyncRTC(const GPS_Data_t *d);
static uint8_t GPS_Checksum(const char *s);
static double  GPS_NMEADeg(const char *field, char hemi);
static int     GPS_Split(const char *src, char *buf, char **fields, int max);
static uint8_t GPS_DayOfWeek(uint16_t y, uint8_t m, uint8_t d);

//void EXTI9_5_IRQHandler(void)
//{
//	GPS_PPS_IRQHandler();
//}

/* -------------------------------------------------------------------------- */
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
}

/* -------------------------------------------------------------------------- */
#ifndef CONTEXT_KEYPAD
static void GPS_EXTI_Init(void)
{
    /* GPIO already configured in GPIO_Init; just enable the NVIC line */
    HAL_NVIC_SetPriority(GPS_PPS_EXTI_IRQn, 5, 0);  /* higher than UART    */
    HAL_NVIC_EnableIRQ(GPS_PPS_EXTI_IRQn);
}
#endif

void GPS_Enable(bool enable)
{
    HAL_GPIO_WritePin(GPS_EN_PORT, GPS_EN_PIN,
                      enable ? GPIO_PIN_SET : GPIO_PIN_RESET);
}

// ============================================================================
// ENABLE ALL GNSS SYSTEMS (GPS, GLONASS, Galileo, BeiDou)
// ============================================================================
#ifdef GPS_USE_TX
void GPS_EnableGNSS_Systems(void)
{
	// UBX-CFG-GNSS command structure
	// This enables: GPS, GLONASS, Galileo, BeiDou
	uint8_t ubx_cfg_gnss[] = {
			0xB5, 0x62, // UBX header
			0x06, 0x3E, // Class 0x06 (CFG), ID 0x3E (GNSS)
			0x2C, 0x00, // Payload length (44 bytes)
			// Payload
			0x00, // Message version 0
			0x00, // Reserved
			0x20, // numTrkChHw (32 tracking channels)
			0x07, // numTrkChUse (7 concurrent constellations)
			// GPS (GNSS ID 0)
			0x00, 0x08, 0x10, 0x00, 0x01, 0x00, 0x01, 0x01,
			// SBAS (GNSS ID 1) - disable0x01, 0x01, 0x03, 0x00, 0x00, 0x00, 0x01, 0x03,
			// Galileo (GNSS ID 2)
			0x02, 0x04, 0x08, 0x00, 0x01, 0x00, 0x01, 0x01,
			// BeiDou (GNSS ID 3)
			0x03, 0x08, 0x10, 0x00, 0x01, 0x00, 0x01, 0x01,
			// GLONASS (GNSS ID 6)
			0x06, 0x08, 0x0E, 0x00, 0x01, 0x00, 0x01, 0x01,
			// Checksum (to be calculated)
			0x00, 0x00
	};

	// Calculate checksum
	uint8_t ck_a = 0, ck_b = 0;

	for (int i = 2; i < 46; i++) {
		ck_a += ubx_cfg_gnss[i];
		ck_b += ck_a;
	}

	ubx_cfg_gnss[46] = ck_a;
	ubx_cfg_gnss[47] = ck_b;

	gps_uart_send(ubx_cfg_gnss, sizeof(ubx_cfg_gnss));
}
#endif

bool GPS_GetData(GPS_Data_t *out)
{
    if (!out) return false;
    xSemaphoreTake(data_mutex, portMAX_DELAY);
    *out = gps_data;
    xSemaphoreGive(data_mutex);
    return gps_data.valid;
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

    if (GPS_Split(line, buf, f, 20) < 10)
    	return false;

    //printf("%s", line);

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

    //printf("%d:%d:%d \r\n", gps_pending.hour, gps_pending.min, gps_pending.sec);

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
        //if ((HAL_GetTick() - pps_pending_ms) < 1500u)
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
	static ulong upd_timer = 0;
    char  buf[GPS_NMEA_MAX_LEN];
    char *f[20];

    if (GPS_Split(line, buf, f, 20) < 10)
    	return false;

    printf("%s", line);

    /* Time: hhmmss[.ss] */
    const char *t = f[1];
    if (strlen(t) >= 6)
    {
    	gps_pending.hour = (uint8_t)((t[0]-'0') * 10 + (t[1]-'0'));
    	gps_pending.min  = (uint8_t)((t[2]-'0') * 10 + (t[3]-'0'));
    	gps_pending.sec  = (uint8_t)((t[4]-'0') * 10 + (t[5]-'0'));
    	gps_pending.msec = (strlen(t) > 7)
                    		 ? (uint16_t)(atof(t + 6) * 1000.0)
                    				 : 0u;

    	//printf("%d:%d:%d \r\n", gps_pending.hour, gps_pending.min, gps_pending.sec);
    	gps_pending.time_valid = true;

    	// Every 30s
    	if(upd_timer == 0)
    		upd_timer = ps.epoch;
    	else if((upd_timer + 30000) < ps.epoch)
    	{
    		printf("%d:%d:%d \r\n", gps_pending.hour, gps_pending.min, gps_pending.sec);

    		// Schedule clock sync
    		xSemaphoreGive(pps_sem);

    		// Restart
    		upd_timer = ps.epoch;
    	}
    }
    else
    {
    	gps_pending.time_valid = false;
    	upd_timer = 0;
    }

    gps_pending.fix_quality = (uint8_t)atoi(f[6]);
    gps_pending.satellites  = (uint8_t)atoi(f[7]);
    gps_pending.hdop        = (float)  atof(f[8]);
    gps_pending.altitude_m  = (float)  atof(f[9]);

    //if(gps_pending.satellites)
    //	printf("sats: %d \r\n", gps_pending.satellites);

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

    if(d->valid)
    {
    	printf("full sync clock \r\n");

    	rt.DayLightSaving = RTC_DAYLIGHTSAVING_NONE;
    	rt.StoreOperation = RTC_STOREOPERATION_RESET;
    	rd.Date    = d->day;
    	rd.Month   = d->month;              /* HAL binary format: 1-12           */
    	rd.Year    = (uint8_t)(d->year - 2000u);
    	rd.WeekDay = GPS_DayOfWeek(d->year, d->month, d->day);
    }
    else
    {
    	printf("time sync only \r\n");

    	// Reload existing
    	HAL_RTC_GetDate(&RtcHandle, &rd, RTC_FORMAT_BIN);
    }

    /* HAL requires time to be set before date */
    HAL_RTC_SetTime(&RtcHandle, &rt, RTC_FORMAT_BIN);
    HAL_RTC_SetDate(&RtcHandle, &rd, RTC_FORMAT_BIN);
}

void gps_proc_message(char *msg, ushort size)
{
    for(int i = 0; i < size; i++)
    {
        char c = msg[i];

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

            //--printf("data: %s \r\n", msg.data);

            // Non-blocking put from ISR context
            xQueueSendFromISR(nmea_queue, &msg, NULL);
            nmea_asm_idx     = 0;
            nmea_in_sentence = false;
        }
    }
}

void gps_proc(void *argument)
{
    (void)argument;
    NMEA_Line_t line;

	vTaskDelay(GPS_PROC_START_DELAY);
	printf("start\r\n");

	// Init
	gps_proc_init();

	#ifdef GPS_INT_NOISE_TEST
	// Kill backlight
	shared_tim_change(0);
	// 58V off
	//--HAL_GPIO_WritePin(VCC_5V_ON_PORT, VCC_5V_ON, GPIO_PIN_RESET);
	#endif

gps_proc_loop:

	#ifndef GPS_TEST_GPIO
    // Process any waiting NMEA sentences
	while (xQueueReceive(nmea_queue, &line, 0) == pdTRUE)
    {
      	//--printf("%s", line.data);
        GPS_ProcessLine(line.data);
    }

    // Block up to 100 ms for the next sentence
    if (xQueueReceive(nmea_queue, &line, pdMS_TO_TICKS(100)) == pdTRUE)
    {
     	//--printf("%s  \r\n", line.data);
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
        else if (gps_pending.time_valid)
        {
            GPS_SyncRTC(&gps_pending);
        }

        xSemaphoreGive(data_mutex);
     }
	#else
    gps_test_run();
	#endif

    goto gps_proc_loop;
}

void gps_proc_init(void)
{
    // RTOS primitives
    nmea_queue = xQueueCreate(GPS_NMEA_QUEUE_DEPTH, sizeof(NMEA_Line_t));
    pps_sem    = xSemaphoreCreateBinary();
    data_mutex = xSemaphoreCreateMutex();

	#ifdef GPS_TEST_GPIO
    gps_test_init();
    return;
	#endif

    GPS_GPIO_Init();

	// Low level driver
	gps_uart_init();

    // EXTI mapping, to be resolved...
	#ifndef CONTEXT_KEYPAD
    GPS_EXTI_Init();
	#endif

    // Power on
    GPS_Enable(true);

    // M10 needs ~100 ms to boot before it starts sending NMEA
    vTaskDelay(150);

	#ifdef GPS_USE_TX
    GPS_EnableGNSS_Systems();
	#endif
}

uchar gps_proc_sats_cnt(void)
{
	#if 0
	static uchar cnt = 0;
	cnt++;
	if(cnt == 10) cnt = 0;
	return cnt;
	#else
	return gps_pending.satellites;
	#endif
}

uchar gps_proc_time_set(void)
{
	if(gps_pending.time_valid)
		return 1;
	else
		return 0;
}

#endif
