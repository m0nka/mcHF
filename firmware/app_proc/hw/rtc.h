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
#ifndef __RTC_H
#define __RTC_H

typedef void (*k_AlarmCallback)(void);

void     k_CalendarBkupInit(void);
void     k_BkupSaveParameter(uint32_t address, uint32_t data);
uint32_t k_BkupRestoreParameter(uint32_t address);

void k_SetTime  (RTC_TimeTypeDef *Time);
void k_GetTime  (RTC_TimeTypeDef *Time);
void k_SetDate  (RTC_DateTypeDef *Date);
void k_GetDate  (RTC_DateTypeDef *Date);
void k_SetAlarm (RTC_AlarmTypeDef *Alarm);
void k_SetAlarmCallback (k_AlarmCallback alarmCallback);

void k_rtc_stop(void);

// LSE trim (RTC_CALR smooth calibration). Positive ppm = clock runs slow
// and is sped up. The measured per unit value lives in the backup domain
// and overrides the compiled in default at boot - see proc/gps/gps_calib.c
int     rtc_calib_ppm_apply(int32_t ppm);
int     rtc_calib_ppm_save (int32_t ppm);
int32_t rtc_calib_ppm_get  (void);
int32_t rtc_calib_ppm_load (void);
void    rtc_calib_ppm_clear(void);

#endif
