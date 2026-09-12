/************************************************************************************
**                                                                                 **
**                                 mcHF QRP Transceiver                            **
**                         Krassi Atanassov - M0NKA, 2013-2026                     **
**                                                                                 **
**---------------------------------------------------------------------------------**
**                                                                                 **
**  File name:		meshchat_proc.h                                                **
**  Description:	MeshCore chat service - conversations, contacts, tx queue      **
**  Licence:		https://github.com/m0nka/mcHF/blob/main/LICENSE                **
************************************************************************************/
//
// Same split as MarsChat: this task owns the state, the dialog in
// proc/ui/desktop_meshchat only draws it and posts requests back.
//
// Where it sits between the two existing tasks:
//
//   lora task     owns the SX1262. On receive it hands the raw packet
//                 here and goes straight back to listening; on transmit
//                 it drains the packet queue this task fills. It never
//                 decodes and never touches the contact book, so the
//                 slow work (an Ed25519 verify runs into the hundreds of
//                 ms) cannot make it miss the next frame
//
//   gui task      polls the getters below on a timer and paints
//
// Which means every store - contacts, channels, history - has exactly
// one writer, this task, and needs no locking
//
#ifndef __MESHCHAT_PROC_H
#define __MESHCHAT_PROC_H

#include <stdint.h>

#include "mc_contacts.h"
#include "mc_rx.h"
#include "mc_tx.h"

// Task notification bits
#define MESHCHAT_NOTIFY_WAKE		0x01

// How long to wait for the SD card at startup before giving up and
// running with an identity that cannot be saved
#define MESHCHAT_SD_WAIT_TRIES		20
#define MESHCHAT_SD_WAIT_MS			500

// How often to look again for a card, when running without one
#define MESHCHAT_STORE_POLL_MS		3000

// What the service is doing, for the dialog's status line
#define MESHCHAT_STATE_BOOT			0			// queues not up yet
#define MESHCHAT_STATE_WAIT_SD		1			// waiting for the card at startup
#define MESHCHAT_STATE_READY		2

// Log every decoded packet on the debug UART. The chat screen is the
// real interface, but on the bench this is the only way to see what the
// mesh is actually sending us
#define MESHCHAT_DEBUG_RX

// Dump what is needed to work out MeshCore's direct-message key schedule
// offline: the raw packet, both public keys and the X25519 shared secret
// we derived from them. With a real DM from a node whose key we hold,
// the right derivation can be found by trying candidates against the
// captured MAC rather than guessing at the radio.
//
// The long term private key is deliberately NOT dumped - the shared
// secret is enough to test every candidate that post-processes it.
//
// OFF: it did its job. The key schedule and the acknowledgement format
// were both solved from the dumps it produced, and every PATH packet on
// the air was filling the UART with hex. Turn it back on only to chase
// another unknown in the direct message flow
//#define MESHCHAT_DEBUG_DM_KEY

// Queue depths
#define MESHCHAT_RX_QUEUE_LEN		6
#define MESHCHAT_TX_QUEUE_LEN		4
#define MESHCHAT_REQ_QUEUE_LEN		6

// History ring, shared across all conversations
#define MESHCHAT_MSG_MAX			64
#define MESHCHAT_TEXT_MAX			120
#define MESHCHAT_SENDER_MAX			16

// Transmissions whose repeats we are still listening for, and how many
// recently seen payloads are remembered for duplicate suppression
#define MESHCHAT_ECHO_MAX			4
#define MESHCHAT_SEEN_MAX			16

// What came back after a transmission - the confirmation that our signal
// reached a repeater at all
typedef struct
{
	uint32_t	fp;								// payload fingerprint
	uint8_t		in_use;
	uint8_t		repeats;						// times heard rebroadcast
	uint8_t		min_hops;						// fewest repeaters in a returning copy
	int8_t		best_snr;
	uint32_t	tick;							// when it was sent

} MESHCHAT_ECHO;

// ---------------------------------------------------------------------
// Conversations. Identified by what they address rather than by a list
// index, so adding or forgetting a contact cannot silently re-point the
// history of another one

#define MESHCHAT_CONV_CHANNEL		0
#define MESHCHAT_CONV_DIRECT		1

typedef struct
{
	uint8_t		kind;							// MESHCHAT_CONV_xxx
	uint8_t		chan_hash;						// channel conversations
	uint8_t		peer[4];						// direct: leading bytes of the contact key

} MESHCHAT_CONV;

// Message direction
#define MESHCHAT_DIR_RX				0
#define MESHCHAT_DIR_TX				1
#define MESHCHAT_DIR_INFO			2			// local notice, not from the air

typedef struct
{
	MESHCHAT_CONV	conv;
	uint8_t			dir;
	uint8_t			in_use;
	char			time[8];					// "HH:MM"
	char			sender[MESHCHAT_SENDER_MAX + 1];
	char			text[MESHCHAT_TEXT_MAX];
	int8_t			snr;

	// Delivery state of an outgoing direct message. ack holds what the
	// far end will answer with; a matching PATH reply sets delivered
	uint8_t			ack[4];
	uint8_t			ack_wait;
	uint8_t			delivered;

} MESHCHAT_MSG;

// ---------------------------------------------------------------------
// Where every packet the modem hands over ends up.
//
// A mesh that looks lossy next to another client is either really
// losing packets on the air, or being thrown away somewhere inside this
// radio - and nothing about the chat screen tells the two apart. These
// counters do: queued against dropped says whether this task kept up,
// decoded against unreadable says whether the packet was for us at all,
// and the radio layer's own counters (lora_radio_stats) say what the
// modem saw before any of it
typedef struct
{
	uint32_t	queued;							// handed over by the lora task
	uint32_t	q_drop;							// queue was full - our own loss
	uint32_t	decoded;						// understood, whoever it was for
	uint32_t	unreadable;						// not ours to read, or malformed
	uint32_t	dup;							// already seen, another route
	uint32_t	echo;							// one of ours, repeated back

} MESHCHAT_STAT;

// How often the service prints what it has seen. Long enough not to
// clutter the log, short enough to bracket a conversation
#define MESHCHAT_STAT_PERIOD_MS		60000

const MESHCHAT_STAT *meshchat_stats(void);

// ---------------------------------------------------------------------
// Task

void		meshchat_proc_task(void const *arg);

// ---------------------------------------------------------------------
// Called by the lora task

// Hand over a received packet. Copies what it needs and returns at once
void		meshchat_rx_packet(const uint8_t *data, uint16_t size, int8_t snr);

// Take the next packet waiting to go out, 0 when there is one
uint8_t		meshchat_tx_dequeue(MC_TX_PACKET *pkt);

// ---------------------------------------------------------------------
// Called by the gui task

// Bumped whenever anything the dialog draws has changed, so the UI can
// poll one value instead of diffing the whole model
uint32_t	meshchat_revision(void);

uint8_t		meshchat_ready(void);
uint8_t		meshchat_tx_pending(void);

// MESHCHAT_STATE_xxx, and how many seconds of the startup card wait are
// left - so a radio with no card in it shows a countdown rather than
// looking like it has hung
uint8_t		meshchat_state(void);
uint8_t		meshchat_sd_wait_left(void);

// Conversation list - channels first, then saved contacts
uint8_t		meshchat_conv_count(void);
uint8_t		meshchat_conv_at(uint8_t idx, MESHCHAT_CONV *out);
void		meshchat_conv_label(const MESHCHAT_CONV *conv, char *buf, uint16_t len);

// History of one conversation, oldest first
uint8_t				meshchat_msg_count(const MESHCHAT_CONV *conv);
const MESHCHAT_MSG	*meshchat_msg_at(const MESHCHAT_CONV *conv, uint8_t idx);

// Requests. All of these only queue work - the task does it
// Returns 0 when the request was queued
uint8_t		meshchat_send_text(const MESHCHAT_CONV *conv, const char *text);
uint8_t		meshchat_send_advert(void);
uint8_t		meshchat_add_contact(uint8_t contact_idx);
uint8_t		meshchat_forget_contact(uint8_t contact_idx);

// Add a channel by name - the key and the on-air hash are derived from
// it (see mc_channels_add_by_name). A leading '#' is added when missing,
// since the name is hashed verbatim and the mesh convention includes it
uint8_t		meshchat_add_channel(const char *name);

// Drop a channel. Addressed by conversation rather than list position,
// which shifts as things are added and removed. Cheap to undo - the key
// derives from the name, so re-adding it by name gets the same channel
uint8_t		meshchat_remove_channel(const MESHCHAT_CONV *conv);

// Messages that have arrived in a conversation since it was last read.
// Shown as a count against each row in the conversation list
uint16_t	meshchat_unread(const MESHCHAT_CONV *conv);

// Clear a conversation's count - called by the dialog for whichever
// conversation is actually on screen
void		meshchat_mark_read(const MESHCHAT_CONV *conv);

// Our own node, for the title bar
const char	*meshchat_node_name(void);
uint8_t		meshchat_node_hash(void);

// The most recent transmission and whatever has been heard back of it.
// Returns 0 when nothing has been sent yet
uint8_t		meshchat_last_echo(MESHCHAT_ECHO *out);

#endif
