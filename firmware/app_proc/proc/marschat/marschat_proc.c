/************************************************************************************
**                                                                                 **
**                                 mcHF QRP Transceiver                            **
**                         Krassi Atanassov - M0NKA, 2013-2026                     **
**                                                                                 **
**---------------------------------------------------------------------------------**
**                                                                                 **
**  File name:		marschat_proc.c                                                **
**  Description:	MarsChat prototype process                                     **
**  Last Modified:                                                                 **
**  Licence:		https://github.com/m0nka/mcHF/blob/main/LICENSE                **
************************************************************************************/
#include "mchf_pro_board.h"
#include "main.h"

#ifdef CONTEXT_MARSCHAT

#include <stdlib.h>

#include "rtc.h"

#include "wspr_decoder.h"
#include "wspr_encoder.h"
#include "wspr_proc.h"
#include "vfo_mc_gen.h"

#include "mc_frame.h"
#include "mc_tx_build.h"
#include "marschat_proc.h"

#if defined(MARSCHAT_LOOPBACK_BEACON) && defined(MARSCHAT_RADIATED_BEACON)
#error "enable only one of MARSCHAT_LOOPBACK_BEACON / MARSCHAT_RADIATED_BEACON"
#endif

// FreeRTOS process state
extern struct PROC_STATE	ps;

// Public radio state - dial frequency
extern struct TRANSCEIVER_STATE_UI	tsu;

#if defined(MARSCHAT_LOOPBACK_BEACON) || defined(MARSCHAT_RADIATED_BEACON)
// Beacon state, shared by the loopback and the radiated variants
static uchar	mc_beacon_seq = 0;
#endif

#ifdef MARSCHAT_LOOPBACK_BEACON
static uchar	mc_beacon_syms[162];

//*----------------------------------------------------------------------------
//* Function Name       : marschat_current_dial_hz
//* Object              : dial (usb carrier) frequency of the active vfo
//* Context    			: CONTEXT_MARSCHAT
//*----------------------------------------------------------------------------
static ulong marschat_current_dial_hz(void)
{
	struct BAND_INFO *b = &tsu.band[tsu.curr_band];

	if(b->active_vfo == VFO_A)
		return b->vfo_a;

	return b->vfo_b;
}
#endif

// ------------------------------------------------------------------------
// ICC_MC_TX_START payload staging - built here, sent by the icc task
// (which owns all M4 traffic) when it processes UI_ICC_MC_TX_START

static uchar	mc_icc_payload[MC_TX_PAYLOAD_MAX];
static ushort	mc_icc_payload_len = 0;

// ------------------------------------------------------------------------
// Chat UI plumbing - RX frames queued for the gui task, TX sequence and
// busy/queueing for UI-originated free-text sends
//
// The M4 streamer keys the exciter for the whole burst (symbols + gap,
// ~110 s+) and there is no completion message back to this core
// (mchf_icc_def.h has none), so a second start request while one is in
// flight would just be silently rejected by the M4's own busy check
// (icc_mc_tx.c). We know the exact burst length from the payload we
// just built, so track a busy deadline and queue anything sent
// meanwhile instead of dropping it. Free text is split into
// MC_PAYLOAD_CHARS-sized chunks, one chunk = one on-air frame/cycle.
//
// All of mc_icc_payload/mc_tx_busy/mc_ui_seq is owned exclusively by
// this task - the gui task (marschat_ui_send_text) only ever pushes
// chunks into mc_tx_queue and wakes us, it never starts a burst
// itself, so there is a single writer and no cross-task race

typedef struct
{
	uint8_t	len;								// 1..MC_PAYLOAD_CHARS
	char	chars[MC_PAYLOAD_CHARS];			// raw text, not null terminated
} MC_TX_CHUNK;

static xQueueHandle	mc_rx_queue = NULL;
static xQueueHandle	mc_tx_queue = NULL;
static uint8_t			mc_ui_seq = 0;
static uint8_t			mc_ui_last_rx_seq = 0;
static uint8_t			mc_tx_busy = 0;
static TickType_t		mc_tx_busy_until = 0;

xQueueHandle marschat_rx_queue(void)
{
	return mc_rx_queue;
}

uint8_t marschat_tx_busy(void)
{
	return mc_tx_busy;
}

uint16_t marschat_tx_pending_chunks(void)
{
	if(mc_tx_queue == NULL)
		return 0;

	return (uint16_t)uxQueueMessagesWaiting(mc_tx_queue);
}

//*----------------------------------------------------------------------------
//* Function Name       : marschat_tx_start_chunk
//* Object              : build + stage a DATA frame carrying one text
//*						: chunk (literal charset codes), kick the icc
//*						: task's radiated tx path and arm the busy
//*						: deadline for the burst we just started
//* Context    			: CONTEXT_MARSCHAT
//*----------------------------------------------------------------------------
static void marschat_tx_start_chunk(const MC_TX_CHUNK *chunk)
{
	MC_FRAME	f;
	uint8_t		bits[7];

	if(ps.hIccTask == NULL)
		return;

	memset(&f, 0, sizeof(f));
	f.ftype = MC_FTYPE_DATA;
	f.seq   = mc_ui_seq;
	f.ack   = mc_ui_last_rx_seq;

	if(mc_text_to_codes(chunk->chars, f.codes, chunk->len) < 0)
		return;											// shouldn't happen - UI filters input

	mc_frame_pack(&f, bits);

	if(mc_tx_build_payload(bits, 1500, MARSCHAT_CW_ID, MARSCHAT_CW_WPM,
							mc_icc_payload, &mc_icc_payload_len) != 0)
		return;

	xTaskNotify(ps.hIccTask, UI_ICC_MC_TX_START, eSetValueWithOverwrite);

	mc_tx_busy       = 1;
	mc_tx_busy_until = xTaskGetTickCount() +
			pdMS_TO_TICKS(mc_tx_build_duration_ms(mc_icc_payload));

	mc_ui_seq = (mc_ui_seq + 1) & 7;
}

//*----------------------------------------------------------------------------
//* Function Name       : marschat_tx_queue_sm
//* Object              : clear the busy deadline once a burst has had
//*						: time to finish, then start the next queued
//*						: chunk (if any) - also services a freshly
//*						: queued chunk immediately when already idle
//* Context    			: CONTEXT_MARSCHAT
//*----------------------------------------------------------------------------
static void marschat_tx_queue_sm(void)
{
	MC_TX_CHUNK	chunk;

	if(mc_tx_busy)
	{
		if((long)(xTaskGetTickCount() - mc_tx_busy_until) < 0)
			return;									// burst still running

		mc_tx_busy = 0;
	}

	if((mc_tx_queue != NULL) && (xQueueReceive(mc_tx_queue, &chunk, 0) == pdPASS))
		marschat_tx_start_chunk(&chunk);
}

//*----------------------------------------------------------------------------
//* Function Name       : marschat_ui_send_text
//* Object              : split free text into MC_PAYLOAD_CHARS chunks
//*						: and queue them for the marschat task to send,
//*						: one chunk per on-air cycle
//* Notes    			: return 0 fully queued, 4 partially queued (rest
//*						: dropped), 1 empty/bad input, 2 not ready yet,
//*						: 5 queue already full (nothing queued)
//* Context    			: CONTEXT_VIDEO (gui task, chat dialog Send button)
//*----------------------------------------------------------------------------
uint8_t marschat_ui_send_text(const char *text)
{
	int		len = (int)strlen(text);
	int		pos = 0;
	uint8_t	any_queued = 0;

	if(len == 0)
		return 1;

	if(mc_tx_queue == NULL)
		return 2;

	while(pos < len)
	{
		MC_TX_CHUNK	chunk;
		int			n = len - pos;

		if(n > MC_PAYLOAD_CHARS)
			n = MC_PAYLOAD_CHARS;

		chunk.len = (uint8_t)n;
		memcpy(chunk.chars, text + pos, (size_t)n);

		if(xQueueSend(mc_tx_queue, &chunk, 0) != pdPASS)
			break;										// queue full - stop here

		any_queued = 1;
		pos += n;
	}

	if(!any_queued)
		return 5;

	if(ps.hMarschatTask != NULL)
		xTaskNotify(ps.hMarschatTask, MARSCHAT_NOTIFY_WAKE, eSetBits);

	return (pos < len) ? 4 : 0;
}

//*----------------------------------------------------------------------------
//* Function Name       : marschat_icc_tx_payload
//* Object              : staged payload accessor for the icc task
//* Context    			: CONTEXT_ICC
//*----------------------------------------------------------------------------
uchar *marschat_icc_tx_payload(ushort *len)
{
	if(mc_icc_payload_len == 0)
		return NULL;

	*len = mc_icc_payload_len;
	return mc_icc_payload;
}

//*----------------------------------------------------------------------------
//* Function Name       : marschat_rx_raw
//* Object              : raw decode consumer, registered with the wspr task
//* Notes    			: returns 1 when the payload is a MarsChat frame
//*						: (consumed - not logged as a WSPR spot)
//* Context    			: CONTEXT_WSPR (called from the wspr task decode pass)
//*----------------------------------------------------------------------------
static uchar marschat_rx_raw(const WSPR_RAW_DECODE *raw)
{
	MC_FRAME	f;
	char		text[48];
	long		freq_c;

	if(mc_frame_unpack(raw->bits, &f) != 0)
		return 0;									// not ours - try type 1

	mc_codes_to_text(f.codes, MC_PAYLOAD_CHARS, text, sizeof(text));

	freq_c = lroundf(raw->freq_hz * 100.0f);

	// Target printf: %d %u %x %s %c only, no 'l' modifier, no floats
	printf("mc: rx ftype(%d) seq(%d) ack(%d) flags(%x) snr(%d) freq(%d.%02d) \"%s\" \r\n",
			f.ftype, f.seq, f.ack, f.flags,
			(int)lroundf(raw->snr_db),
			(int)(freq_c / 100), (int)labs(freq_c % 100),
			text);

	mc_ui_last_rx_seq = f.seq;

	if(mc_rx_queue != NULL)
	{
		MC_UI_RX_MSG	m;

		m.ftype = f.ftype;
		m.seq   = f.seq;
		m.ack   = f.ack;
		m.flags = f.flags;
		strncpy(m.text, text, sizeof(m.text) - 1);
		m.text[sizeof(m.text) - 1] = 0;

		xQueueSend(mc_rx_queue, &m, 0);		// drop on a full queue
	}

	return 1;
}

#ifdef MARSCHAT_LOOPBACK_BEACON
//*----------------------------------------------------------------------------
//* Function Name       : marschat_beacon_sm
//* Object              : loopback test beacon - "HELLO" every even minute
//* Notes    			: fires at second :01, the WSPR nominal tx start, so
//*						: an armed WSPR monitor captures it with dt near 0.
//*						: Injects via CLK1 at dial + 1500 Hz - no emissions
//* Context    			: CONTEXT_MARSCHAT
//*----------------------------------------------------------------------------
static void marschat_beacon_sm(void)
{
	RTC_TimeTypeDef	tm = {0};
	RTC_DateTypeDef	dt = {0};
	MC_FRAME		f;
	uint8_t			bits[7];

	// Even minute, second :01 ? (date read unlocks the shadow regs)
	k_GetTime(&tm);
	k_GetDate(&dt);

	if((tm.Minutes & 1) || (tm.Seconds != 1))
		return;

	// One trigger per slot - the stream itself runs 110.6 s
	if(vfo_mc_gen_active())
		return;

	memset(&f, 0, sizeof(f));
	f.ftype = MC_FTYPE_BEACON;
	f.seq	= mc_beacon_seq;

	mc_text_to_codes("HELLO", f.codes, MC_PAYLOAD_CHARS);

	mc_frame_pack(&f, bits);
	wspr_encode_raw(bits, mc_beacon_syms);

	if(vfo_mc_gen_start(marschat_current_dial_hz() + 1500, mc_beacon_syms, 162) == 0)
	{
		printf("mc: beacon tx seq(%d) \r\n", mc_beacon_seq);
		mc_beacon_seq = (mc_beacon_seq + 1) & 7;
	}
}
#endif

#ifdef MARSCHAT_RADIATED_BEACON
//*----------------------------------------------------------------------------
//* Function Name       : marschat_radiated_sm
//* Object              : radiated test beacon over the M4 symbol streamer
//* Notes    			: same "HELLO" frame and even minute :01 schedule as
//*						: the loopback beacon, but the tx goes through the
//*						: real tx chain - the M4 keys the exciter itself on
//*						: ICC_MC_TX_START and unkeys when done. TX MIXER
//*						: BENCH ONLY until the PA exists; set the dial and
//*						: USB mode from the UI first
//* Context    			: CONTEXT_MARSCHAT
//*----------------------------------------------------------------------------
static void marschat_radiated_sm(void)
{
	RTC_TimeTypeDef	tm = {0};
	RTC_DateTypeDef	dt = {0};
	MC_FRAME		f;
	uint8_t			bits[7];
	static ulong	mc_tx_start_tick = 0;

	// Even minute, second :01 ? (date read unlocks the shadow regs)
	k_GetTime(&tm);
	k_GetDate(&dt);

	if((tm.Minutes & 1) || (tm.Seconds != 1))
		return;

	// One trigger per slot - symbols alone run 110.6 s, plus gap + CW id.
	// The M4 streamer also refuses a start while one is running
	if((mc_tx_start_tick != 0) &&
	   ((xTaskGetTickCount() - mc_tx_start_tick) < 118000))
		return;

	memset(&f, 0, sizeof(f));
	f.ftype = MC_FTYPE_BEACON;
	f.seq	= mc_beacon_seq;

	mc_text_to_codes("HELLO", f.codes, MC_PAYLOAD_CHARS);
	mc_frame_pack(&f, bits);

	if(mc_tx_build_payload(bits, 1500, MARSCHAT_CW_ID, MARSCHAT_CW_WPM,
							mc_icc_payload, &mc_icc_payload_len) != 0)
	{
		printf("mc: tx payload build failed \r\n");
		return;
	}

	if(ps.hIccTask != NULL)
	{
		xTaskNotify(ps.hIccTask, UI_ICC_MC_TX_START, eSetValueWithOverwrite);

		printf("mc: radiated tx seq(%d) \r\n", mc_beacon_seq);
		mc_beacon_seq    = (mc_beacon_seq + 1) & 7;
		mc_tx_start_tick = xTaskGetTickCount();
	}
}
#endif

//*----------------------------------------------------------------------------
//* Function Name       : marschat_proc_task
//* Object              : MarsChat prototype process
//* Notes    			:
//* Context    			: CONTEXT_MARSCHAT
//*----------------------------------------------------------------------------
void marschat_proc_task(void const *arg)
{
	ulong		ulNotificationValue = 0;
	TickType_t	sleep;

	vTaskDelay(MARSCHAT_PROC_START_DELAY);
	printf("start\r\n");

	mc_rx_queue = xQueueCreate(MARSCHAT_RX_QUEUE_LEN, sizeof(MC_UI_RX_MSG));
	mc_tx_queue = xQueueCreate(MARSCHAT_TX_QUEUE_LEN, sizeof(MC_TX_CHUNK));

	// Consume MarsChat frames from the shared decoder
	wspr_proc_set_raw_hook(marschat_rx_raw);

marschat_proc_loop:

	#if defined(MARSCHAT_LOOPBACK_BEACON) || defined(MARSCHAT_RADIATED_BEACON)
	sleep = 100;									// poll the RTC
	#else
	sleep = mc_tx_busy ? 1000 : MARSCHAT_PROC_SLEEP_TIME;	// poll the tx deadline while busy
	#endif

	xTaskNotifyWait(0x00, ULONG_MAX, &ulNotificationValue, sleep);

	#ifdef MARSCHAT_LOOPBACK_BEACON
	marschat_beacon_sm();
	#endif

	#ifdef MARSCHAT_RADIATED_BEACON
	marschat_radiated_sm();
	#endif

	marschat_tx_queue_sm();

	goto marschat_proc_loop;
}

#endif
