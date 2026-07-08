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

#include "mchf_icc_def.h"

#include "wspr_decoder.h"
#include "wspr_proc.h"

static void wspr_proc_decode_cycle(void);

// FreeRTOS process state
extern struct PROC_STATE	ps;

// Public radio state - dial frequency for the spot log
extern struct TRANSCEIVER_STATE_UI	tsu;

// SD card read chunk, lives in AXI ram like the other disk buffers
__attribute__((section(".axi_mem"))) __attribute__ ((aligned (32))) \
static int16_t	wspr_pcm_buf[4096];

static WSPR_DECODE	wspr_results[WSPR_MAX_DECODES];

// Pending decode request
static char		wspr_capture_path[64] = WSPR_CAPTURE_FILE;
static ulong	wspr_dial_hz = 0;

// ------------------------------------------------------------------------
// Capture streaming from the M4 core
//
// The icc task pulls 12 kHz mono PCM chunks from the M4 core and pushes
// them into this staging ring, this task drains the ring to the SD card.
// The ring gives the SD card write path slack (a card can stall for
// hundreds of ms) without ever blocking the high priority icc task
//
// WSPR rx cycle is 110.6 s starting 1 s past the even minute
#define WSPR_CAPTURE_SECONDS		114
#define WSPR_CAPTURE_BYTES			(WSPR_CAPTURE_SECONDS * 12000UL * 2UL)

// Staging ring size, must be power of two (~2.7 s of audio)
#define WSPR_STAGE_SIZE				(64*1024)

__attribute__((section(".wspr_mem"))) static uchar	wspr_stage[WSPR_STAGE_SIZE];

// Ring indices are free running byte counters, masked on access
static volatile ulong	wspr_stage_wr = 0;
static volatile ulong	wspr_stage_rd = 0;

// M4 capture stream confirmed running (set/cleared by the icc task)
static volatile uchar	wspr_capture_run = 0;

// 1 = M4 chunk ring overflow, 2 = staging ring overflow (SD card stall)
static volatile uchar	wspr_capture_overrun = 0;

// Monitor state machine
enum {
	WSPR_MON_IDLE = 0,		// monitor off, task sleeps forever
	WSPR_MON_WAIT,			// armed, waiting for the even minute
	WSPR_MON_STARTING,		// start requested, waiting for the M4 ack
	WSPR_MON_CAPTURE		// streaming to the SD card
};

static volatile uchar	wspr_monitor_on = 0;
static uchar			wspr_mon_state = WSPR_MON_IDLE;

static FIL				wspr_capture_file;
static uchar			wspr_capture_file_open = 0;
static ulong			wspr_capture_written = 0;
static ulong			wspr_start_req_tick = 0;

//*----------------------------------------------------------------------------
//* Function Name       : wspr_capture_active
//* Object              : true while the M4 capture stream runs - the icc
//*						: task uses it to switch to chunk ring polling
//* Context    			: CONTEXT_ICC
//*----------------------------------------------------------------------------
uchar wspr_capture_active(void)
{
	return wspr_capture_run;
}

//*----------------------------------------------------------------------------
//* Function Name       : wspr_capture_mark_started/stopped
//* Object              : M4 capture stream state tracking
//* Context    			: CONTEXT_ICC
//*----------------------------------------------------------------------------
void wspr_capture_mark_started(void)
{
	wspr_capture_run = 1;
}

void wspr_capture_mark_stopped(void)
{
	wspr_capture_run = 0;
}

//*----------------------------------------------------------------------------
//* Function Name       : wspr_capture_push
//* Object              : one PCM chunk from the M4 core into the staging ring
//* Notes    			: single producer (icc task), single consumer (wspr
//*						: task), so the free running indices need no locking
//* Context    			: CONTEXT_ICC
//*----------------------------------------------------------------------------
void wspr_capture_push(const uchar *data, ushort len, uchar flags)
{
	ulong	wr, free_bytes, off, run;

	if(flags & ICC_WSPR_FLAG_OVERRUN)
		wspr_capture_overrun = 1;

	wr         = wspr_stage_wr;
	free_bytes = WSPR_STAGE_SIZE - (wr - wspr_stage_rd);

	// Staging ring full - SD card write path stalled for seconds
	if(len > free_bytes)
	{
		wspr_capture_overrun = 2;
		return;
	}

	off = wr & (WSPR_STAGE_SIZE - 1);
	run = WSPR_STAGE_SIZE - off;
	if(run > len)
		run = len;

	memcpy(wspr_stage + off, data, run);
	if(run < len)
		memcpy(wspr_stage, data + run, len - run);

	wspr_stage_wr = wr + len;

	if(ps.hWsprTask != NULL)
		xTaskNotify(ps.hWsprTask, WSPR_NOTIFY_DATA, eSetBits);
}

//*----------------------------------------------------------------------------
//* Function Name       : wspr_proc_monitor_set
//* Object              : arm/disarm the background WSPR monitor
//* Context    			: any task
//*----------------------------------------------------------------------------
void wspr_proc_monitor_set(uchar on)
{
	wspr_monitor_on = on;

	// Kick the task out of the forever sleep
	if(ps.hWsprTask != NULL)
		xTaskNotify(ps.hWsprTask, WSPR_NOTIFY_WAKE, eSetBits);
}

//*----------------------------------------------------------------------------
//* Function Name       : wspr_proc_current_dial_hz
//* Object              : dial (usb carrier) frequency for the spot log
//* Context    			: CONTEXT_WSPR
//*----------------------------------------------------------------------------
static ulong wspr_proc_current_dial_hz(void)
{
	struct BAND_INFO *b = &tsu.band[tsu.curr_band];

	if(b->active_vfo == VFO_A)
		return b->vfo_a;

	return b->vfo_b;
}

//*----------------------------------------------------------------------------
//* Function Name       : wspr_proc_capture_drain
//* Object              : staging ring to SD card
//* Context    			: CONTEXT_WSPR
//*----------------------------------------------------------------------------
static void wspr_proc_capture_drain(void)
{
	UINT	bw;

	// No sink - discard, keeps the ring from sticking full
	if(!wspr_capture_file_open)
	{
		wspr_stage_rd = wspr_stage_wr;
		return;
	}

	while(wspr_stage_rd != wspr_stage_wr)
	{
		ulong	rd    = wspr_stage_rd;
		ulong	avail = wspr_stage_wr - rd;
		ulong	off   = rd & (WSPR_STAGE_SIZE - 1);
		ulong	run   = WSPR_STAGE_SIZE - off;

		if(run > avail)
			run = avail;

		// Never write past the wanted capture length
		if(wspr_capture_written >= WSPR_CAPTURE_BYTES)
		{
			wspr_stage_rd = wspr_stage_wr;
			break;
		}

		if(run > (WSPR_CAPTURE_BYTES - wspr_capture_written))
			run = WSPR_CAPTURE_BYTES - wspr_capture_written;

		if(f_write(&wspr_capture_file, wspr_stage + off, run, &bw) != FR_OK)
		{
			printf("wspr: capture write err \r\n");
			f_close(&wspr_capture_file);
			wspr_capture_file_open = 0;
			return;
		}

		wspr_capture_written += bw;
		wspr_stage_rd = rd + run;
	}
}

//*----------------------------------------------------------------------------
//* Function Name       : wspr_proc_capture_finish
//* Object              : stop the M4 stream, close the file, run the decode
//* Context    			: CONTEXT_WSPR
//*----------------------------------------------------------------------------
static void wspr_proc_capture_finish(uchar decode)
{
	// Ask the icc task to stop the M4 stream and pull the tail chunks
	if(ps.hIccTask != NULL)
		xTaskNotify(ps.hIccTask, UI_ICC_WSPR_STOP, eSetValueWithOverwrite);

	// Give it a moment to pull the last chunks over
	vTaskDelay(300);
	wspr_proc_capture_drain();

	wspr_capture_run = 0;

	if(wspr_capture_file_open)
	{
		f_close(&wspr_capture_file);
		wspr_capture_file_open = 0;
	}

	printf("wspr: capture done, %u bytes, overrun(%d) \r\n",
			(uint)wspr_capture_written, wspr_capture_overrun);

	wspr_mon_state = wspr_monitor_on ? WSPR_MON_WAIT : WSPR_MON_IDLE;

	// Decode takes seconds at low priority - a cycle that starts
	// meanwhile is skipped, the state machine only arms on second zero
	if((decode) && (wspr_capture_written != 0))
	{
		strcpy(wspr_capture_path, WSPR_CAPTURE_FILE);
		wspr_proc_decode_cycle();
	}
}

//*----------------------------------------------------------------------------
//* Function Name       : wspr_proc_monitor_sm
//* Object              : even minute capture scheduler
//* Context    			: CONTEXT_WSPR
//*----------------------------------------------------------------------------
static void wspr_proc_monitor_sm(void)
{
	RTC_TimeTypeDef	tm = {0};
	RTC_DateTypeDef	dt = {0};

	switch(wspr_mon_state)
	{
		case WSPR_MON_IDLE:
		{
			if(wspr_monitor_on)
				wspr_mon_state = WSPR_MON_WAIT;

			break;
		}

		case WSPR_MON_WAIT:
		{
			if(!wspr_monitor_on)
			{
				wspr_mon_state = WSPR_MON_IDLE;
				break;
			}

			// Even minute, second zero ? (date read unlocks the shadow regs)
			k_GetTime(&tm);
			k_GetDate(&dt);

			if((tm.Minutes & 1) || (tm.Seconds != 0))
				break;

			// Capture only makes sense with a running DSP core
			if(tsu.dsp_alive != 2)
				break;

			f_mkdir(WSPR_DIR);								// ok if it exists

			if(f_open(&wspr_capture_file, WSPR_CAPTURE_FILE, FA_WRITE | FA_CREATE_ALWAYS) != FR_OK)
			{
				printf("wspr: capture create err \r\n");
				break;
			}

			wspr_capture_file_open	= 1;
			wspr_capture_written	= 0;
			wspr_capture_overrun	= 0;
			wspr_stage_wr			= 0;
			wspr_stage_rd			= 0;

			// Remember the dial for the spot log before anything moves
			wspr_dial_hz = wspr_proc_current_dial_hz();

			wspr_start_req_tick	= xTaskGetTickCount();
			wspr_mon_state		= WSPR_MON_STARTING;

			// Ask the icc task to start the M4 capture stream
			if(ps.hIccTask != NULL)
				xTaskNotify(ps.hIccTask, UI_ICC_WSPR_START, eSetValueWithOverwrite);

			printf("wspr: capture start (dial %u Hz) \r\n", (uint)wspr_dial_hz);
			break;
		}

		case WSPR_MON_STARTING:
		{
			if(wspr_capture_run)
			{
				wspr_mon_state = WSPR_MON_CAPTURE;
				break;
			}

			// The start notification can lose the race against another
			// icc command (value overwrite) - keep retrying, the M4 side
			// start is idempotent. Give up after five seconds
			if((xTaskGetTickCount() - wspr_start_req_tick) > 5000)
			{
				printf("wspr: no capture ack \r\n");
				wspr_proc_capture_finish(0);
				break;
			}

			if(ps.hIccTask != NULL)
				xTaskNotify(ps.hIccTask, UI_ICC_WSPR_START, eSetValueWithOverwrite);

			break;
		}

		case WSPR_MON_CAPTURE:
		{
			// Cycle complete, monitor disarmed, M4 stream died or the
			// SD card write path failed - all end the capture. Decode
			// whatever was recorded unless the user disarmed us
			if(wspr_capture_written >= WSPR_CAPTURE_BYTES)
				wspr_proc_capture_finish(1);
			else if(!wspr_monitor_on)
				wspr_proc_capture_finish(0);
			else if(!wspr_capture_run)
				wspr_proc_capture_finish(1);
			else if(!wspr_capture_file_open)
				wspr_proc_capture_finish(0);

			break;
		}

		default:
			wspr_mon_state = WSPR_MON_IDLE;
			break;
	}
}

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

	// Note: the target snprintf is common/print_f.c which drops the whole
	// line on any specifier outside %d %u %x %s %c - no 'l' modifier!
	snprintf(buf, buflen, "%02d%02d%02d %02d%02d %4d %c%d.%01d %4u.%06u %c%d.%02d  %s\r\n",
			dt.Year, dt.Month, dt.Date,
			tm.Hours, tm.Minutes,
			snr,
			(dt_d    < 0) ? '-' : ' ', (int)(labs(dt_d) / 10),  (int)(labs(dt_d) % 10),
			(uint)(freq_hz / 1000000), (uint)(freq_hz % 1000000),
			(drift_c < 0) ? '-' : ' ', (int)(labs(drift_c) / 100), (int)(labs(drift_c) % 100),
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
	printf("wspr: %d decode(s) in %u ms \r\n", ndec, (uint)(xTaskGetTickCount() - t0));

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
	ulong		ulNotificationValue = 0, ulNotif;
	TickType_t	sleep;

	vTaskDelay(WSPR_PROC_START_DELAY);
	printf("start\r\n");

	#ifdef WSPR_MONITOR_AUTO_START
	wspr_monitor_on = 1;
	#endif

wspr_proc_loop:

	// Sleep forever when idle, poll the RTC/ring while the monitor is armed
	sleep = (wspr_mon_state == WSPR_MON_IDLE) && (!wspr_monitor_on) ? \
			WSPR_PROC_SLEEP_TIME : 100;

	ulNotif = xTaskNotifyWait(0x00, ULONG_MAX, &ulNotificationValue, sleep);
	if(ulNotif)
	{
		// New capture chunks in the staging ring
		if(ulNotificationValue & WSPR_NOTIFY_DATA)
			wspr_proc_capture_drain();

		// Direct decode request (UI or debug)
		if(ulNotificationValue & WSPR_NOTIFY_DECODE)
			wspr_proc_decode_cycle();
	}

	// Even minute capture scheduler
	wspr_proc_monitor_sm();

	goto wspr_proc_loop;
}

#endif
