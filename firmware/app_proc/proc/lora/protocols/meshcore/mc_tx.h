/************************************************************************************
**                                                                                 **
**                                 mcHF QRP Transceiver                            **
**                         Krassi Atanassov - M0NKA, 2013-2026                     **
**                                                                                 **
**---------------------------------------------------------------------------------**
**                                                                                 **
**  File name:		mc_tx.h                                                        **
**  Description:	Builds outgoing MeshCore packets                               **
**  Licence:		https://github.com/m0nka/mcHF/blob/main/LICENSE                **
************************************************************************************/
//
// Everything here is pure buffer work - nothing touches the radio. The
// meshchat task builds a packet, hands the bytes to the lora task, and
// that task is the only thing that ever talks to the SX1262
//
#ifndef __MC_TX_H
#define __MC_TX_H

#include <stdint.h>

#include "packet.h"
#include "mc_contacts.h"

// A built packet, ready for the modem
#define MC_TX_MAX_LEN			MESHCORE_MAX_TRANS_UNIT

typedef struct
{
	uint8_t		data[MC_TX_MAX_LEN];
	uint8_t		len;

} MC_TX_PACKET;

// How much text fits in one packet. The payload holds the channel hash,
// a 2 byte MAC and the AES blocks; each block carries a 4 byte timestamp
// and a 1 byte text type before the text itself. Kept well short of the
// on air maximum so a long node name still leaves room for a message
#define MC_TX_TEXT_MAX			100

// Plain text message flavour, the byte MeshCore calls text_type
#define MC_TEXT_TYPE_PLAIN		0

//*----------------------------------------------------------------------------
// Group (channel) text. The wire form is "sender: message" encrypted
// with the channel key, which is why the sender name is part of the
// payload rather than the header
//
// Returns 0 on success
uint8_t	mc_tx_build_group_text(MC_TX_PACKET *pkt, const MC_CHANNEL *ch,
							   const char *sender, const char *text, uint32_t timestamp);

//*----------------------------------------------------------------------------
// Direct message to a contact. Addressed by node hash and encrypted
// under the X25519 shared secret, so the contact must have its key
// cached (mc_contacts_derive_shared)
//
// Returns 0 on success
// ack_out, when given, comes back with the acknowledgement this message
// will be answered with - store it and a matching PATH reply confirms
// the message was delivered
uint8_t	mc_tx_build_direct_text(MC_TX_PACKET *pkt, const MC_CONTACT *to,
								const char *text, uint32_t timestamp,
								uint8_t ack_out[4]);

//*----------------------------------------------------------------------------
// Acknowledge a direct message, the way MeshCore actually does it.
//
// A flooded direct message is answered with a PATH packet, not an ACK
// packet: it returns the route home AND carries the acknowledgement
// nested inside. Its payload is encrypted with the same pairwise key as
// the message, and the plaintext is
//
//     path_len | 0x03 (ACK) | ack[4] | zero padding
//
// Without this the sender retransmits the same message indefinitely.
// Established from a phone's own replies, see solve notes in mc_rx.c
//
// Returns 0 on success
uint8_t	mc_tx_build_path_ack(MC_TX_PACKET *pkt, const MC_CONTACT *to,
							 const uint8_t ack[4]);

//*----------------------------------------------------------------------------
// Our own advertisement - this is what puts the radio on other people's
// contact lists. Signed with the identity key
//
// Returns 0 on success
uint8_t	mc_tx_build_advert(MC_TX_PACKET *pkt, uint32_t timestamp);

#endif
