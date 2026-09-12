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

// Log every decoded packet on the debug UART. The chat screen is the
// real interface, but on the bench this is the only way to see what the
// mesh is actually sending us
#define MESHCHAT_DEBUG_RX

// Queue depths
#define MESHCHAT_RX_QUEUE_LEN		6
#define MESHCHAT_TX_QUEUE_LEN		4
#define MESHCHAT_REQ_QUEUE_LEN		6

// History ring, shared across all conversations
#define MESHCHAT_MSG_MAX			64
#define MESHCHAT_TEXT_MAX			120
#define MESHCHAT_SENDER_MAX			16

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

} MESHCHAT_MSG;

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

// Messages that have arrived in a conversation since it was last read.
// Shown as a count against each row in the conversation list
uint16_t	meshchat_unread(const MESHCHAT_CONV *conv);

// Clear a conversation's count - called by the dialog for whichever
// conversation is actually on screen
void		meshchat_mark_read(const MESHCHAT_CONV *conv);

// Our own node, for the title bar
const char	*meshchat_node_name(void);
uint8_t		meshchat_node_hash(void);

#endif
