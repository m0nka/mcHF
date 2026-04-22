/* gps_driver.h – GPS task for STM32H747 M7 / FreeRTOS
 *
 * Hardware
 *   USART6  PG9  (RX, AF7)   PG14 (TX, AF7)  38400 baud, DMA circular
 *   PPS     PA8  (EXTI rising edge)            syncs internal RTC
 *   GPS_EN  PB1  (output, active-high)
 *
 * Tested with: u-blox M10 (MAX-M10S, SAM-M10Q)
 */
#ifndef __GPS_DRIVER_H
#define __GPS_DRIVER_H

#include "main.h"
#include "cmsis_os.h"
#include <stdbool.h>
#include <stdint.h>

/* --------------------------------------------------------------------------
 * Pin / peripheral definitions
 * -------------------------------------------------------------------------- */
#define GPS_UART            USART6
#define GPS_UART_BAUD       38400U   /* u-blox M10 factory default            */
#define GPS_UART_IRQn       USART6_IRQn
#define GPS_UART_AF         GPIO_AF7_USART6

#define GPS_RX_PIN          GPIO_PIN_9
#define GPS_RX_PORT         GPIOG

#define GPS_TX_PIN          GPIO_PIN_14
#define GPS_TX_PORT         GPIOG

#define GPS_PPS_PIN         GPIO_PIN_8
#define GPS_PPS_PORT        GPIOA
#define GPS_PPS_EXTI_IRQn   EXTI9_5_IRQn      /* lines 5-9 share this IRQ   */

#define GPS_EN_PIN          GPIO_PIN_1
#define GPS_EN_PORT         GPIOB

/* DMA – adjust stream / channel if your CubeMX project allocates differently */
#define GPS_DMA             DMA2
#define GPS_DMA_STREAM      DMA2_Stream1
#define GPS_DMA_IRQn        DMA2_Stream1_IRQn
#define GPS_DMA_REQUEST     DMA_REQUEST_USART6_RX

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
    bool     rtc_synced;     /* RTC was set at least once via PPS  */
    uint32_t last_pps_ms;    /* HAL_GetTick() at last PPS edge     */
} GPS_Data_t;

/* --------------------------------------------------------------------------
 * Public API
 * -------------------------------------------------------------------------- */

/** One-time hardware + RTOS object initialisation.
 *  Call before GPS_Task() starts, or let GPS_Task() call it internally.   */
void GPS_Init(void);

/** FreeRTOS task entry – pass to osThreadNew().
 *  Stack recommendation: 512 words (2 kB).                                 */
void GPS_Task(void *argument);

/** Thread-safe snapshot of the latest parsed data.
 *  Returns true when the fix is valid.                                     */
bool GPS_GetData(GPS_Data_t *out);

/** Drive the GPS_EN pin.  */
void GPS_Enable(bool enable);

/* --------------------------------------------------------------------------
 * IRQ trampolines – add these calls to stm32h7xx_it.c
 * --------------------------------------------------------------------------
 *   void USART6_IRQHandler(void)       { GPS_UART_IRQHandler(); }
 *   void EXTI9_5_IRQHandler(void)      { GPS_PPS_IRQHandler();  }
 *   void DMA2_Stream1_IRQHandler(void) { GPS_DMA_IRQHandler();  }
 * -------------------------------------------------------------------------- */
void GPS_UART_IRQHandler(void);
void GPS_PPS_IRQHandler(void);
void GPS_DMA_IRQHandler(void);

#endif /* GPS_DRIVER_H */
