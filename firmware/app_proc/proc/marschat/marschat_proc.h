/************************************************************************************
**                                                                                 **
**                                 mcHF QRP Transceiver                            **
**                         Krassi Atanassov - M0NKA, 2013-2026                     **
**                                                                                 **
**---------------------------------------------------------------------------------**
**                                                                                 **
**  File name:		marschat_proc.h                                                **
**  Description:	MarsChat prototype process                                     **
**  Last Modified:                                                                 **
**  Licence:		https://github.com/m0nka/mcHF/blob/main/LICENSE                **
************************************************************************************/
//
// WP4 seed - in-tree prototype of the MarsChat personality (the final
// delivery form is a loadable SD app, see claude/MarsChat/PROJECT.md).
// Registers as raw decode consumer with the wspr task and, with
// MARSCHAT_LOOPBACK_BEACON enabled, sends a test beacon over the CLK1
// loopback injector every even minute (radio stays in RX - arm the
// WSPR monitor to capture and decode our own signal)
//
#ifndef __MARSCHAT_PROC_H
#define __MARSCHAT_PROC_H

#include <stdint.h>

#include "mc_session.h"

// Task notification bits
#define MARSCHAT_NOTIFY_WAKE		0x01

void	marschat_proc_task(void const *arg);

// VFO save/restore on MarsChat mode entry/exit. The radio tunes to the
// per-band MarsChat dial frequency on entry and restores the user's
// original frequency when leaving. Called from the GUI task mode switch
void	marschat_vfo_enter(void);
void	marschat_vfo_exit(void);

// Immediate session + TX teardown for mode exit. Aborts any running M4
// burst, disarms the WSPR monitor, and drops the session synchronously
// so the VFO can be safely restored. Called from the GUI task before
// marschat_vfo_exit()
void	marschat_force_stop(void);

// Staged ICC_MC_TX_START payload for the icc task (UI_ICC_MC_TX_START
// handler) - NULL when nothing is staged
uchar	*marschat_icc_tx_payload(ushort *len);

// Line kinds in the chat UI queue
#define MC_UI_LINE_RX			0				// decoded peer text
#define MC_UI_LINE_INFO			1				// session/ARQ notice, text only

// Message delivered to the chat UI - one per marschat_rx_raw() hit, plus
// the session layer's notices (frame delivered, retry limit, link lost)
typedef struct
{
	uint8_t	kind;								// MC_UI_LINE_xxx
	uint8_t	ftype;								// MC_FTYPE_xxx
	uint8_t	seq;
	uint8_t	ack;
	uint8_t	flags;
	int8_t	snr;								// dB, as reported by the decoder
	int16_t	dt_cs;								// start offset vs nominal, hundredths of a second
	char	text[48];							// rendered via mc_codes_to_text
} MC_UI_RX_MSG;

// Snapshot of the session for the chat dialog's status line. Advisory,
// display only - the marschat task is the sole owner of the real state
typedef struct
{
	uint8_t		state;							// MC_SESS_xxx
	uint8_t		role;							// MC_ROLE_xxx
	uint8_t		now_is_ours;					// the slot running now is ours
	uint8_t		next_is_ours;					// the upcoming slot is our tx slot
	uint8_t		tx_busy;						// exciter keyed right now
	uint8_t		rx_active;						// capturing the peer's slot right now
	uint8_t		pending;						// frame in flight, un-acked
	uint8_t		retries;						// transmissions of it so far
	uint8_t		last_rx_seq;
	uint8_t		tx_seq;
	uint16_t	queued_chunks;					// still waiting in the tx queue
	uint16_t	secs_to_slot;					// until the next slot boundary
	uint8_t		tx_progress;					// percent of the burst sent, tx_busy only
	uint32_t	dial_hz;						// active vfo, the tones sit at +1500 Hz
} MC_UI_STATUS;

void	marschat_get_status(MC_UI_STATUS *st);

// Enter/leave a session. Slots alternate every 120 s, the caller owns
// the even slot indices (slot index = UTC minutes / 2) and the peer the
// odd ones - the two stations must pick opposite roles. Transmission
// then happens only in our own slots and the receiver is armed only for
// the peer's, so both are handled by the slot scheduler, not by the
// immediate send path. Request is executed by the marschat task
void	marschat_session_start(uint8_t role);
void	marschat_session_stop(void);

// Queue of MC_UI_RX_MSG, drained by the chat dialog (WM_TIMER poll).
// NULL until the marschat task has created it (small boot window)
xQueueHandle	marschat_rx_queue(void);

// True while a tx burst is keying the exciter (~110 s+) - advisory,
// display only, the marschat task is the sole owner of the real state
uint8_t			marschat_tx_busy(void);

// Chunks (MC_PAYLOAD_CHARS chars each) still waiting in the tx queue,
// not counting one currently in flight (marschat_tx_busy). Advisory,
// display only - lets the chat UI show which part of a submitted
// message is still "waiting its turn"
uint16_t		marschat_tx_pending_chunks(void);

// UI-originated send: splits free text into MC_PAYLOAD_CHARS-sized
// chunks and queues them for the marschat task to transmit as DATA
// frames (literal charset codes, no phrase LUT), one chunk per own slot
// - i.e. one burst every 4 minutes with both stations alternating. A tx
// burst runs ~110 s+ per chunk and the M4 has no completion message, so
// this queues (MARSCHAT_TX_QUEUE_LEN chunks deep) rather than dropping.
// A session must be running for anything to actually go out; the chat
// dialog starts one on SEND.
// Returns 0 fully queued, 4 partially queued (queue ran out of room,
// remainder dropped), 1 empty/bad input, 2 not ready yet, 5 queue
// already full (nothing queued)
uint8_t			marschat_ui_send_text(const char *text);

#endif
