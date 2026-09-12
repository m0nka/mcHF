/************************************************************************************
**                                                                                 **
**                                 mcHF QRP Transceiver                            **
**                         Krassi Atanassov - M0NKA, 2013-2026                     **
**                                                                                 **
**---------------------------------------------------------------------------------**
**                                                                                 **
**  File name:		mc_rx.h                                                        **
**  Description:	Structured MeshCore receive decode                             **
**  Licence:		https://github.com/m0nka/mcHF/blob/main/LICENSE                **
************************************************************************************/
//
// One decoder for the whole radio. It turns a received packet into a
// filled MC_RX_EVENT; the chat app files that away as a message or a
// contact sighting, and the old one line notification on the spectrum
// display is rendered from the same struct (mc_client.c)
//
#ifndef __MC_RX_H
#define __MC_RX_H

#include <stdint.h>

#include "packet.h"
#include "mc_identity.h"
#include "mc_contacts.h"

// What came in
#define MC_RX_NONE				0				// nothing usable
#define MC_RX_ADVERT			1				// a node announcing itself
#define MC_RX_CHANNEL			2				// group text on a channel we hold the key to
#define MC_RX_DIRECT			3				// a direct message addressed to us
#define MC_RX_ACK				4
#define MC_RX_OTHER				5				// valid packet, nothing for us in it

#define MC_RX_TEXT_MAX			140

typedef struct
{
	uint8_t		kind;							// MC_RX_xxx
	uint8_t		type;							// raw meshcore_payload_type_t
	uint8_t		route;
	char		type_short[8];					// "Adv", "GTxt", ... for the status line

	// Advert
	uint8_t		pub_key[MC_EC_KEY_SIZE];
	char		name[MC_NAME_MAX + 1];
	uint8_t		role;
	uint32_t	timestamp;
	uint8_t		sig_ok;							// signature verified

	// Text, channel or direct
	uint8_t		channel_hash;
	char		channel_name[MC_CHANNEL_NAME_MAX + 1];
	uint8_t		src_hash;
	uint8_t		dst_hash;
	char		sender[MC_NAME_MAX + 1];		// who it is from, as far as we can tell
	char		text[MC_RX_TEXT_MAX];
	uint8_t		mac_ok;							// key matched, payload authentic

	// A direct message whose destination hash is ours. Set even when it
	// could not be decrypted, which is the distinction that matters when
	// testing: "nobody is sending to us" and "somebody is, and our key
	// schedule is wrong" otherwise look identical
	uint8_t		addressed_to_us;

	// ACK payload of an incoming ACK packet
	uint32_t	ack_crc;

	// For a direct message we could read: the acknowledgement the
	// sender is waiting for, already computed. MeshCore defines it as
	//
	//    SHA-256(plaintext without padding || sender public key)[0..4]
	//
	// where the plaintext is timestamp|flags|text. Established by
	// brute forcing four real phone messages against the acks it sent
	// back - see the note in mc_rx_do_direct
	uint8_t		ack_reply[4];
	uint8_t		needs_ack;

	// An acknowledgement addressed to us, carried in a PATH reply -
	// this is how the radio learns one of its own direct messages
	// actually arrived
	uint8_t		path_ack[4];
	uint8_t		has_path_ack;

	// Radio side
	int8_t		snr;
	uint8_t		path_len;						// repeaters that have touched it
	uint8_t		path[MESHCORE_MAX_PATH_SIZE];

	// Hash of the payload alone. A flood packet keeps its payload while
	// repeaters append themselves to the path, so this is the same value
	// every time the same message comes round - which is what lets us
	// recognise a repeat of our own transmission, and drop the copies
	// that arrive by other routes
	uint32_t	payload_fp;

} MC_RX_EVENT;

// The same hash, for a payload we built ourselves
uint32_t	mc_rx_fingerprint(const uint8_t *data, uint16_t len);

// Decode one received packet. ev is always cleared first; the return is
// ev->kind for convenience
uint8_t	mc_rx_decode(const uint8_t *data, uint16_t size, int8_t snr, MC_RX_EVENT *ev);

// Render the one line summary the spectrum display shows. The buffer
// must be at least MC_RX_FORMAT_MIN bytes: snprintf in this tree does
// not bound a %s (PutString in common/print_f.c just copies), so the
// caller's buffer has to fit the worst case rather than rely on the
// length argument
#define MC_RX_FORMAT_MIN	(MC_CHANNEL_NAME_MAX + MC_NAME_MAX + MC_RX_TEXT_MAX + 16)

void	mc_rx_format(const MC_RX_EVENT *ev, char *buf, uint16_t buf_len);

#endif
