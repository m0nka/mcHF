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

	// Consume MarsChat frames from the shared decoder
	wspr_proc_set_raw_hook(marschat_rx_raw);

marschat_proc_loop:

	#if defined(MARSCHAT_LOOPBACK_BEACON) || defined(MARSCHAT_RADIATED_BEACON)
	sleep = 100;									// poll the RTC
	#else
	sleep = MARSCHAT_PROC_SLEEP_TIME;				// nothing scheduled
	#endif

	xTaskNotifyWait(0x00, ULONG_MAX, &ulNotificationValue, sleep);

	#ifdef MARSCHAT_LOOPBACK_BEACON
	marschat_beacon_sm();
	#endif

	#ifdef MARSCHAT_RADIATED_BEACON
	marschat_radiated_sm();
	#endif

	goto marschat_proc_loop;
}

#endif
