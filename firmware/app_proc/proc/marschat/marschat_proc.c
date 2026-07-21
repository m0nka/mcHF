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
#include "mc_session.h"
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
#endif

//*----------------------------------------------------------------------------
//* Function Name       : marschat_current_dial_hz
//* Object              : dial (usb carrier) frequency of the active vfo
//* Context    			: CONTEXT_MARSCHAT / CONTEXT_VIDEO (status snapshot)
//*----------------------------------------------------------------------------
static ulong marschat_current_dial_hz(void)
{
	struct BAND_INFO *b = &tsu.band[tsu.curr_band];

	if(b->active_vfo == VFO_A)
		return b->vfo_a;

	return b->vfo_b;
}

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
#ifdef MARSCHAT_IMMEDIATE_SEND
static uint8_t			mc_ui_seq = 0;				// bench path frame counter
#endif
static uint8_t			mc_ui_last_rx_seq = 0;
static uint8_t			mc_tx_busy = 0;
static TickType_t		mc_tx_busy_until = 0;
static uint32_t			mc_tx_burst_ms = 0;			// length of the burst in flight

// ------------------------------------------------------------------------
// Session / slot scheduling (PROTOCOL.md section 7)
//
// Two decision points per 120 s slot, and they cannot be merged:
//
//   odd minute :MARSCHAT_ARM_SEC   arm the receiver when the NEXT slot
//                                  belongs to the peer. Depends on the
//                                  clock and our role only, and must
//                                  happen before the wspr monitor's own
//                                  even minute arm point
//
//   even minute :00                ask the session layer what to do with
//                                  the slot that just started. This one
//                                  has to be late: a frame captured in
//                                  the peer's slot only finishes decoding
//                                  ~118.5 s into that slot (the decoder's
//                                  slot deadline in wspr_proc.c), and its
//                                  ack decides whether we send something
//                                  new or repeat what is in flight
//
// The transmission itself starts at :01, the WSPR nominal start, so our
// dt lands near zero at the peer.
//
// Burst budget against the 120 s slot: 110.6 s of symbols + 0.5 s gap
// from :01 puts the unkey at ~112 s, i.e. 8 s before the peer's slot -
// and the receiver for that slot is armed at :MARSCHAT_ARM_SEC while we
// are still keyed (harmless, the capture only starts on the even minute).
// A CW id segment is spent out of those 8 s, so a long callsign at a low
// speed is what would break the schedule, not the symbols

static MC_SESSION		mc_sess;
static uint8_t			mc_slot_tx_armed = 0;		// payload staged, waiting for :01
static uint8_t			mc_slot_dec_min = 0xFF;		// minute of the last slot decision
static uint8_t			mc_slot_arm_min = 0xFF;		// minute of the last rx arm

// Session start/stop is requested from the gui task and executed here -
// the state machine has a single owner
static volatile uint8_t	mc_sess_req = 0;			// 0 none, 1 start, 2 stop
static volatile uint8_t	mc_sess_req_role = MC_ROLE_CALLER;

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
//* Function Name       : marschat_slot_parity
//* Object              : parity of the slot the given minute belongs to
//* Notes    			: slot index = UTC minutes / 2, and an hour holds
//*						: 30 slots (an even number), so the alternation
//*						: runs unbroken across the hour boundary
//* Context    			: any task
//*----------------------------------------------------------------------------
static uint8_t marschat_slot_parity(uint8_t minute)
{
	return (uint8_t)(((minute / 2) & 1) ? 1 : 0);
}

//*----------------------------------------------------------------------------
//* Function Name       : marschat_ui_post
//* Object              : push a line to the chat dialog, dropping it if
//*						: the UI is not draining (queue full)
//* Context    			: CONTEXT_MARSCHAT
//*----------------------------------------------------------------------------
static void marschat_ui_post_info(const char *text)
{
	MC_UI_RX_MSG	m;

	if(mc_rx_queue == NULL)
		return;

	memset(&m, 0, sizeof(m));
	m.kind = MC_UI_LINE_INFO;
	strncpy(m.text, text, sizeof(m.text) - 1);

	xQueueSend(mc_rx_queue, &m, 0);
}

//*----------------------------------------------------------------------------
//* Function Name       : marschat_session_start / _stop
//* Object              : session requests from the gui task
//* Context    			: CONTEXT_VIDEO (gui task)
//*----------------------------------------------------------------------------
void marschat_session_start(uint8_t role)
{
	mc_sess_req_role	= role;
	mc_sess_req			= 1;

	// Printed here, in the caller's context - the matching "session
	// start" from the task proves the request was actually executed
	printf("mc: session request (role %d) \r\n", role);

	if(ps.hMarschatTask != NULL)
		xTaskNotify(ps.hMarschatTask, MARSCHAT_NOTIFY_WAKE, eSetBits);
}

void marschat_session_stop(void)
{
	mc_sess_req = 2;

	printf("mc: session stop request \r\n");

	if(ps.hMarschatTask != NULL)
		xTaskNotify(ps.hMarschatTask, MARSCHAT_NOTIFY_WAKE, eSetBits);
}

//*----------------------------------------------------------------------------
//* Function Name       : marschat_get_status
//* Object              : session snapshot for the chat dialog status line
//* Context    			: CONTEXT_VIDEO (gui task) - display only
//*----------------------------------------------------------------------------
void marschat_get_status(MC_UI_STATUS *st)
{
	RTC_TimeTypeDef	tm = {0};
	RTC_DateTypeDef	dt = {0};
	uint16_t		into_slot;

	if(st == NULL)
		return;

	memset(st, 0, sizeof(MC_UI_STATUS));

	st->state			= mc_sess.state;
	st->role			= mc_sess.role;
	st->pending			= mc_sess.pending;
	st->retries			= mc_sess.retries;
	st->last_rx_seq		= mc_sess.last_rx_seq;
	st->tx_seq			= mc_sess.tx_seq;
	st->tx_busy			= mc_tx_busy;
	st->rx_active		= wspr_capture_active();
	st->queued_chunks	= marschat_tx_pending_chunks();
	st->dial_hz			= (uint32_t)marschat_current_dial_hz();

	// How far into the burst we are - the M4 sends no progress, but the
	// payload we built told us exactly how long it would key for
	if((mc_tx_busy) && (mc_tx_burst_ms != 0))
	{
		long left = (long)(mc_tx_busy_until - xTaskGetTickCount());

		if(left < 0)
			left = 0;

		st->tx_progress = (uint8_t)(100 - ((uint32_t)left * 100) / pdMS_TO_TICKS(mc_tx_burst_ms));
	}

	// Seconds left of the current slot, and who owns this one and the next
	k_GetTime(&tm);
	k_GetDate(&dt);

	into_slot			= (uint16_t)(((tm.Minutes & 1) ? 60 : 0) + tm.Seconds);
	st->secs_to_slot	= (uint16_t)(120 - into_slot);
	st->now_is_ours		= mc_session_owns_slot(&mc_sess, marschat_slot_parity(tm.Minutes));
	st->next_is_ours	= mc_session_owns_slot(&mc_sess,
							marschat_slot_parity((uint8_t)((tm.Minutes + 2) % 60)));

	return;
}

#ifdef MARSCHAT_IMMEDIATE_SEND
//*----------------------------------------------------------------------------
//* Function Name       : marschat_tx_start_chunk
//* Object              : build + stage a DATA frame carrying one text
//*						: chunk (literal charset codes), kick the icc
//*						: task's radiated tx path and arm the busy
//*						: deadline for the burst we just started
//* Notes    			: bench path only - no slot timing, no ARQ, the
//*						: seq is a free running counter of its own
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
#endif

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

	// In a session the frame goes through the ARQ layer first: it may be
	// a pure ack keep-alive, or the peer repeating a frame we already
	// took because our own ack has not reached it yet. Either way it is
	// still proof of life, so the session sees it - only the chat window
	// is spared the repeat
	if(mc_sess.state == MC_SESS_ACTIVE)
	{
		if(!mc_session_on_rx(&mc_sess, &f))
		{
			printf("mc: rx not new (ack or repeat) \r\n");
			return 1;
		}
	}

	if(mc_rx_queue != NULL)
	{
		MC_UI_RX_MSG	m;

		m.kind  = MC_UI_LINE_RX;
		m.ftype = f.ftype;
		m.seq   = f.seq;
		m.ack   = f.ack;
		m.flags = f.flags;
		m.snr   = (int8_t)lroundf(raw->snr_db);
		m.dt_cs = (int16_t)lroundf(raw->dt_sec * 100.0f);
		strncpy(m.text, text, sizeof(m.text) - 1);
		m.text[sizeof(m.text) - 1] = 0;

		xQueueSend(mc_rx_queue, &m, 0);		// drop on a full queue
	}

	return 1;
}

//*----------------------------------------------------------------------------
//* Function Name       : marschat_slot_tx_kick
//* Object              : hand the staged payload to the icc task and arm
//*						: the busy deadline for the burst
//* Context    			: CONTEXT_MARSCHAT
//*----------------------------------------------------------------------------
static void marschat_slot_tx_kick(void)
{
	if((ps.hIccTask == NULL) || (mc_icc_payload_len == 0))
		return;

	xTaskNotify(ps.hIccTask, UI_ICC_MC_TX_START, eSetValueWithOverwrite);

	mc_tx_burst_ms   = mc_tx_build_duration_ms(mc_icc_payload);
	mc_tx_busy       = 1;
	mc_tx_busy_until = xTaskGetTickCount() + pdMS_TO_TICKS(mc_tx_burst_ms);
}

//*----------------------------------------------------------------------------
//* Function Name       : marschat_slot_results
//* Object              : report what the session layer concluded about
//*						: the frame that just left the tx window
//* Context    			: CONTEXT_MARSCHAT
//*----------------------------------------------------------------------------
static void marschat_slot_results(void)
{
	static uint8_t	lost_reported = 0;
	char			line[48];
	uint8_t			seq = 0;
	uint8_t			res = mc_session_take_result(&mc_sess, &seq);

	if(mc_sess.state != MC_SESS_LOST)
		lost_reported = 0;

	if(res == MC_XFER_DELIVERED)
	{
		snprintf(line, sizeof(line), "seq %d delivered", seq);
		printf("mc: %s \r\n", line);
		marschat_ui_post_info(line);
	}
	else if(res == MC_XFER_FAILED)
	{
		snprintf(line, sizeof(line), "seq %d lost after %d tries", seq, MC_ARQ_RETRIES);
		printf("mc: %s \r\n", line);
		marschat_ui_post_info(line);
	}

	if((mc_sess.state == MC_SESS_LOST) && (!lost_reported))
	{
		lost_reported = 1;

		printf("mc: session lost - peer silent \r\n");
		marschat_ui_post_info("session lost - peer silent");

		mc_slot_tx_armed = 0;
	}
}

//*----------------------------------------------------------------------------
//* Function Name       : marschat_slot_sm
//* Object              : the 120 s slot scheduler - arms the receiver for
//*						: the peer's slots, drives the session layer at
//*						: our own slot boundaries and starts the burst at
//*						: the WSPR nominal :01
//* Context    			: CONTEXT_MARSCHAT
//*----------------------------------------------------------------------------
static void marschat_slot_sm(void)
{
	RTC_TimeTypeDef	tm = {0};
	RTC_DateTypeDef	dt = {0};
	MC_TX_CHUNK		chunk;
	MC_FRAME		f;
	uint8_t			codes[MC_PAYLOAD_CHARS];
	uint8_t			bits[7];
	uint8_t			have_new = 0;
	uint8_t			act;

	if(mc_sess.state != MC_SESS_ACTIVE)
		return;

	// Date read unlocks the shadow registers
	k_GetTime(&tm);
	k_GetDate(&dt);

	// --- receiver arm point, MARSCHAT_ARM_SEC into the odd minute ------
	// The wspr monitor starts its capture on the even minute itself, so
	// the request has to be in before that
	if((tm.Minutes & 1) && (tm.Seconds >= MARSCHAT_ARM_SEC) && (mc_slot_arm_min != tm.Minutes))
	{
		mc_slot_arm_min = tm.Minutes;

		if(!mc_session_owns_slot(&mc_sess, marschat_slot_parity((uint8_t)((tm.Minutes + 1) % 60))))
		{
			wspr_proc_monitor_once();
			printf("mc: slot rx armed at %02d:%02d:%02d \r\n", tm.Hours, tm.Minutes, tm.Seconds);
		}
	}

	// --- slot boundary, even minute :00 --------------------------------
	if(((tm.Minutes & 1) == 0) && (tm.Seconds <= 1) && (mc_slot_dec_min != tm.Minutes))
	{
		mc_slot_dec_min = tm.Minutes;

		// The peer's frame from the previous slot has been decoded by
		// now (the decoder is cut at 118.5 s), so its ack is already in
		memset(codes, 0, sizeof(codes));

		if((mc_tx_queue != NULL) && (xQueuePeek(mc_tx_queue, &chunk, 0) == pdPASS))
		{
			if(mc_text_to_codes(chunk.chars, codes, chunk.len) >= 0)
				have_new = 1;
		}

		act = mc_session_slot(&mc_sess, marschat_slot_parity(tm.Minutes),
								have_new ? codes : NULL, 0, &f);

		if(act == MC_SLOT_TX_NEW)					// the peeked chunk is now ours
			xQueueReceive(mc_tx_queue, &chunk, 0);

		if(MC_SLOT_IS_TX(act))
		{
			mc_frame_pack(&f, bits);

			if(mc_tx_build_payload(bits, 1500, MARSCHAT_CW_ID, MARSCHAT_CW_WPM,
									mc_icc_payload, &mc_icc_payload_len) == 0)
			{
				mc_slot_tx_armed = 1;

				printf("mc: slot tx ftype(%d) seq(%d) ack(%d) try(%d) at %02d:%02d:%02d \r\n",
						f.ftype, f.seq, f.ack, mc_sess.retries,
						tm.Hours, tm.Minutes, tm.Seconds);
			}
			else
				printf("mc: tx payload build failed \r\n");
		}
	}

	// --- burst start, :01 ----------------------------------------------
	if(mc_slot_tx_armed && ((tm.Minutes & 1) == 0) && (tm.Seconds >= 1) && (tm.Seconds < 10))
	{
		mc_slot_tx_armed = 0;

		marschat_slot_tx_kick();
	}
}

//*----------------------------------------------------------------------------
//* Function Name       : marschat_session_sm
//* Object              : execute a start/stop request from the gui task
//* Context    			: CONTEXT_MARSCHAT
//*----------------------------------------------------------------------------
static void marschat_session_sm(void)
{
	uint8_t	req = mc_sess_req;

	if(req == 0)
		return;

	mc_sess_req = 0;

	if(req == 1)
	{
		// A continuously armed wspr monitor would capture straight
		// through our own tx slot - the scheduler arms per slot instead
		wspr_proc_monitor_set(0);

		mc_session_start(&mc_sess, mc_sess_req_role);

		mc_slot_tx_armed	= 0;
		mc_slot_dec_min		= 0xFF;
		mc_slot_arm_min		= 0xFF;

		printf("mc: session start as %s \r\n",
				(mc_sess.role == MC_ROLE_CALLER) ? "caller (even slots)" : "peer (odd slots)");

		marschat_ui_post_info((mc_sess.role == MC_ROLE_CALLER) ?
								"session started - caller, even slots" :
								"session started - peer, odd slots");
	}
	else
	{
		mc_session_stop(&mc_sess);

		wspr_proc_monitor_set(0);

		mc_slot_tx_armed = 0;

		printf("mc: session stop \r\n");
		marschat_ui_post_info("session stopped");
	}
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
	// A session polls the RTC for its slot boundaries, otherwise only the
	// tx deadline of an immediate send needs watching
	sleep = (mc_sess.state == MC_SESS_ACTIVE) ? 100 :
			(mc_tx_busy ? 1000 : MARSCHAT_PROC_SLEEP_TIME);
	#endif

	xTaskNotifyWait(0x00, ULONG_MAX, &ulNotificationValue, sleep);

	#ifdef MARSCHAT_LOOPBACK_BEACON
	marschat_beacon_sm();
	#endif

	#ifdef MARSCHAT_RADIATED_BEACON
	marschat_radiated_sm();
	#endif

	marschat_session_sm();
	marschat_slot_sm();
	marschat_slot_results();

	#ifdef MARSCHAT_IMMEDIATE_SEND
	// Bench path: outside a session the queue is drained as fast as the
	// exciter allows, with no slot discipline and no ARQ
	if(mc_sess.state == MC_SESS_OFF)
		marschat_tx_queue_sm();
	#endif

	// Burst over ? In a session the scheduler owns the next one
	if(mc_tx_busy && ((long)(xTaskGetTickCount() - mc_tx_busy_until) >= 0))
		mc_tx_busy = 0;

	goto marschat_proc_loop;
}

#endif
