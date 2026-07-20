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

// Task notification bits
#define MARSCHAT_NOTIFY_WAKE		0x01

void	marschat_proc_task(void const *arg);

// Staged ICC_MC_TX_START payload for the icc task (UI_ICC_MC_TX_START
// handler) - NULL when nothing is staged
uchar	*marschat_icc_tx_payload(ushort *len);

// Decoded frame delivered to the chat UI, one per marschat_rx_raw() hit
typedef struct
{
	uint8_t	ftype;								// MC_FTYPE_xxx
	uint8_t	seq;
	uint8_t	ack;
	uint8_t	flags;
	char	text[48];							// rendered via mc_codes_to_text
} MC_UI_RX_MSG;

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
// chunks and queues them for the marschat task to transmit as ordinary
// DATA frames (literal charset codes, no phrase LUT). A tx burst runs
// ~110 s+ per chunk and the M4 has no completion message, so this
// queues (MARSCHAT_TX_QUEUE_LEN chunks deep) rather than dropping.
// Returns 0 fully queued, 4 partially queued (queue ran out of room,
// remainder dropped), 1 empty/bad input, 2 not ready yet, 5 queue
// already full (nothing queued)
uint8_t			marschat_ui_send_text(const char *text);

#endif
