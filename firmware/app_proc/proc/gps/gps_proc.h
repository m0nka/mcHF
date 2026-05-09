
#ifndef __GPS_PROC_H
#define __GPS_PROC_H

#include "main.h"

//#include <stdbool.h>
//#include <stdint.h>

// --------------------------------------------------------------------------
// Unit tests
//#define GPS_TEST_GPIO
// ----------------------
// Noise reduction steps:
//
// 1. Backlight timer disabled, use GPIO mode
// 2. [--Turn off 58V--]
// 3. Remove mixer board
// 4. [--Disable BMS--]
// 5. [--Disable LORA--]
// 6. Disable SD
#define GPS_INT_NOISE_TEST

// Only RX ?
//#define GPS_USE_TX

/* --------------------------------------------------------------------------
 * Pin / peripheral definitions
 * -------------------------------------------------------------------------- */
#define GPS_PPS_EXTI_IRQn   EXTI9_5_IRQn      /* lines 5-9 share this IRQ   */

/* --------------------------------------------------------------------------
 * Sizing
 * -------------------------------------------------------------------------- */
#define GPS_DMA_BUF_SIZE    256U
#define GPS_NMEA_MAX_LEN    100U
#define GPS_NMEA_QUEUE_DEPTH  8U

/* --------------------------------------------------------------------------
 * Parsed data
 * -------------------------------------------------------------------------- */
typedef struct {
    /* UTC time & date */
    uint8_t  hour;
    uint8_t  min;
    uint8_t  sec;
    uint16_t msec;
    uint8_t  day;
    uint8_t  month;
    uint16_t year;

    /* Position (decimal degrees; negative = South / West) */
    double   latitude;
    double   longitude;
    float    altitude_m;

    /* Quality */
    uint8_t  fix_quality;    /* 0=none 1=GPS 2=DGPS */
    uint8_t  satellites;
    float    hdop;

    /* Motion */
    float    speed_kn;
    float    course_deg;

    /* Status */
    bool     valid;          /* RMC status field = 'A'             */
    bool	 time_valid;	// Seems the time is the first thing showing up, even with bad lock
    bool     rtc_synced;     /* RTC was set at least once via PPS  */
    uint32_t last_pps_ms;    /* HAL_GetTick() at last PPS edge     */
} GPS_Data_t;

/* --------------------------------------------------------------------------
 * Public API
 * -------------------------------------------------------------------------- */

/** One-time hardware + RTOS object initialisation.
 *  Call before GPS_Task() starts, or let GPS_Task() call it internally.   */
void gps_proc_init(void);
void gps_proc_message(char *msg, ushort size);

/** FreeRTOS task entry – pass to osThreadNew().
 *  Stack recommendation: 512 words (2 kB).                                 */
void gps_proc(void *argument);

/** Thread-safe snapshot of the latest parsed data.
 *  Returns true when the fix is valid.                                     */
bool GPS_GetData(GPS_Data_t *out);

/** Drive the GPS_EN pin.  */
void GPS_Enable(bool enable);

void GPS_UART_IRQHandler(void);
void GPS_PPS_IRQHandler(void);
void GPS_DMA_IRQHandler(void);

uchar gps_proc_sats_cnt(void);

#endif
