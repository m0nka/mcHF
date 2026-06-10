/************************************************************************************
**                                                                                 **
**                                 mcHF QRP Transceiver                            **
**                         Krassi Atanassov - M0NKA, 2013-2026                     **
**                                                                                 **
**---------------------------------------------------------------------------------**
**                                                                                 **
**  File name:		wspr_proc.c                                                    **
**  Description:	WSPR decoder process                                           **
**  Last Modified:                                                                 **
**  Licence:		https://github.com/m0nka/mcHF/blob/main/LICENSE                **
************************************************************************************/
#include "mchf_pro_board.h"
#include "main.h"

#ifdef CONTEXT_WSPR

#include <math.h>
#include <stdlib.h>

#include "ff.h"
#include "rtc.h"

#include "wspr_decoder.h"
#include "wspr_proc.h"

// FreeRTOS process state
extern struct PROC_STATE	ps;

// SD card read chunk, lives in AXI ram like the other disk buffers
__attribute__((section(".axi_mem"))) __attribute__ ((aligned (32))) \
static int16_t	wspr_pcm_buf[4096];

static WSPR_DECODE	wspr_results[WSPR_MAX_DECODES];

// Pending decode request
static char		wspr_capture_path[64] = WSPR_CAPTURE_FILE;
static ulong	wspr_dial_hz = 0;

//*----------------------------------------------------------------------------
//* Function Name       : wspr_proc_request_decode
//* Object              : post a decode request to the wspr task
//* Notes    			: called from other tasks (recorder, UI)
//* Context    			: any task
//*----------------------------------------------------------------------------
uchar wspr_proc_request_decode(const char *path, ulong dial_freq_hz)
{
	if(ps.hWsprTask == NULL)
		return 1;

	if(path != NULL)
	{
		if(strlen(path) >= sizeof(wspr_capture_path))
			return 2;

		strcpy(wspr_capture_path, path);
	}
	else
		strcpy(wspr_capture_path, WSPR_CAPTURE_FILE);

	wspr_dial_hz = dial_freq_hz;

	xTaskNotify(ps.hWsprTask, WSPR_NOTIFY_DECODE, eSetBits);

	return 0;
}

//*----------------------------------------------------------------------------
//* Function Name       : wspr_proc_format_decode
//* Object              : one decode as a text line (integer math only,
//*						: small printf has no float support)
//* Context    			: CONTEXT_WSPR
//*----------------------------------------------------------------------------
static void wspr_proc_format_decode(char *buf, int buflen, WSPR_DECODE *d)
{
	RTC_TimeTypeDef	tm = {0};
	RTC_DateTypeDef	dt = {0};
	ulong	freq_hz;
	long	dt_d, drift_c;
	int		snr;

	k_GetTime(&tm);
	k_GetDate(&dt);

	// RF spot frequency if the dial is known, else audio frequency
	freq_hz = wspr_dial_hz + (ulong)lroundf(d->freq_hz);

	snr     = (int)lroundf(d->snr_db);
	dt_d    = lroundf(d->dt_sec * 10.0f);				// deciseconds
	drift_c = lroundf(d->drift_hz * 100.0f);			// centi-Hz

	snprintf(buf, buflen, "%02d%02d%02d %02d%02d %4d %c%ld.%01ld %4lu.%06lu %c%ld.%02ld  %s\r\n",
			dt.Year, dt.Month, dt.Date,
			tm.Hours, tm.Minutes,
			snr,
			(dt_d    < 0) ? '-' : ' ', labs(dt_d) / 10,  labs(dt_d) % 10,
			freq_hz / 1000000, freq_hz % 1000000,
			(drift_c < 0) ? '-' : ' ', labs(drift_c) / 100, labs(drift_c) % 100,
			d->message);
}

//*----------------------------------------------------------------------------
//* Function Name       : wspr_proc_decode_cycle
//* Object              : read capture from SD, decode, append decodes.txt
//* Context    			: CONTEXT_WSPR
//*----------------------------------------------------------------------------
static void wspr_proc_decode_cycle(void)
{
	FIL			file;
	UINT		br;
	FRESULT		res;
	int			i, ndec;
	ulong		t0;
	char		line[96];

	printf("wspr: decoding %s \r\n", wspr_capture_path);

	// Stream the capture file into the decoder front end
	res = f_open(&file, wspr_capture_path, FA_READ);
	if(res != FR_OK)
	{
		printf("wspr: capture open err(%d) \r\n", res);
		return;
	}

	wspr_decoder_reset();

	for(;;)
	{
		res = f_read(&file, wspr_pcm_buf, sizeof(wspr_pcm_buf), &br);
		if((res != FR_OK) || (br == 0))
			break;

		wspr_decoder_feed(wspr_pcm_buf, (int)(br / 2));

		if(br < sizeof(wspr_pcm_buf))
			break;
	}

	f_close(&file);

	// Heavy lifting - several seconds at osPriorityLow
	t0 = xTaskGetTickCount();
	ndec = wspr_decoder_run(wspr_results, WSPR_MAX_DECODES);
	printf("wspr: %d decode(s) in %lu ms \r\n", ndec, (ulong)(xTaskGetTickCount() - t0));

	if(ndec == 0)
		return;

	// Append to the decodes text file
	f_mkdir(WSPR_DIR);									// ok if it exists

	res = f_open(&file, WSPR_DECODES_FILE, FA_WRITE | FA_OPEN_APPEND);
	if(res != FR_OK)
	{
		printf("wspr: decodes open err(%d) \r\n", res);
		return;
	}

	for(i = 0; i < ndec; i++)
	{
		UINT bw;

		wspr_proc_format_decode(line, sizeof(line), &wspr_results[i]);
		printf("wspr: %s", line);

		f_write(&file, line, strlen(line), &bw);
	}

	f_close(&file);
}

//*----------------------------------------------------------------------------
//* Function Name       : wspr_proc_task
//* Object              : decode requests after each two minute rx cycle
//* Notes    			:
//* Context    			: CONTEXT_WSPR
//*----------------------------------------------------------------------------
void wspr_proc_task(void const *arg)
{
	ulong	ulNotificationValue = 0, ulNotif;

	vTaskDelay(WSPR_PROC_START_DELAY);
	printf("start\r\n");

wspr_proc_loop:

	ulNotif = xTaskNotifyWait(0x00, ULONG_MAX, &ulNotificationValue, WSPR_PROC_SLEEP_TIME);
	if((ulNotif) && (ulNotificationValue & WSPR_NOTIFY_DECODE))
	{
		wspr_proc_decode_cycle();
	}

	goto wspr_proc_loop;
}

#endif
