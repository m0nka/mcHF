/************************************************************************************
**                                                                                 **
**                                 mcHF QRP Transceiver                            **
**                         Krassi Atanassov - M0NKA, 2013-2026                     **
**                                                                                 **
**---------------------------------------------------------------------------------**
**                                                                                 **
**  File name:		mc_contacts.h                                                  **
**  Description:	MeshCore contact book and channel keyring                      **
**  Licence:		https://github.com/m0nka/mcHF/blob/main/LICENSE                **
************************************************************************************/
//
// Two tables:
//
//   contacts - one per node we have heard advertise. Every advert lands
//              here, so the list doubles as "who is on the air"; the
//              saved flag marks the ones the user has actually added,
//              and only those are written to the SD card and offered as
//              direct message destinations
//
//   channels - the group keyring. A channel is a 16 byte AES key; the
//              one byte hash that identifies it on air is the first byte
//              of SHA-256 over that key, which is how an incoming group
//              message is matched to a key
//
#ifndef __MC_CONTACTS_H
#define __MC_CONTACTS_H

#include <stdint.h>

#include "packet.h"
#include "mc_ec.h"
#include "mc_identity.h"

// Note: advert.h is deliberately NOT included here. It pulls in
// <stdbool.h>, and this header reaches most of the firmware, where
// "bool" is the project's own typedef from mchf_types.h - the two
// definitions collide. The role is kept as a plain uint8_t; a file that
// wants the meshcore_device_role_t names includes advert.h itself

#define MC_CONTACT_MAX			24
#define MC_CHANNEL_MAX			8

#define MC_CHANNEL_NAME_MAX		16
#define MC_CHANNEL_KEY_SIZE		16

// A channel authenticates with an HMAC over its 16 byte key; a direct
// message authenticates with an HMAC over the full 32 byte X25519 shared
// secret, while still taking its AES-128 key from the first 16. The two
// paths therefore share the cipher but not the MAC key length
#define MC_DM_MAC_KEY_SIZE		MC_EC_KEY_SIZE

#define MC_CONTACTS_FILE		"0://meshchat/contacts.bin"
#define MC_CHANNELS_FILE		"0://meshchat/channels.bin"

typedef struct
{
	uint8_t		pub_key[MC_EC_KEY_SIZE];
	char		name[MC_NAME_MAX + 1];
	uint8_t		role;

	uint32_t	advert_ts;						// timestamp carried in the advert
	uint32_t	heard_tick;						// our tick count when last heard
	int8_t		snr;
	uint8_t		heard;							// we have seen it at least once this boot

	uint8_t		path_len;						// route back, as learned from the advert
	uint8_t		path[MESHCORE_MAX_PATH_SIZE];

	// Direct message key: the X25519 shared secret with this node,
	// cached because deriving it costs a scalar multiplication.
	//
	// All 32 bytes are kept, not just the 16 the cipher uses. MeshCore
	// takes the AES-128 key from the first half but authenticates with
	// an HMAC over the WHOLE secret - proven by brute forcing a real
	// direct message from a phone against its captured MAC, see
	// claude/meshchat_test/solve_dm_key.py
	uint8_t		shared[MC_EC_KEY_SIZE];
	uint8_t		have_shared;

	uint8_t		saved;							// user added it, persist it
	uint8_t		in_use;

	// Messages that have arrived since this conversation was last
	// looked at. Bumped by the meshchat task, cleared by the dialog -
	// a one word race at worst, and the cost of losing it is a badge
	// that is out by one. Not meaningful on disk, zeroed on load
	uint16_t	unread;

} MC_CONTACT;

typedef struct
{
	char		name[MC_CHANNEL_NAME_MAX + 1];
	uint8_t		key[MC_CHANNEL_KEY_SIZE];
	uint8_t		hash;
	uint8_t		in_use;

	uint16_t	unread;

} MC_CHANNEL;

// Load both tables from the SD card, seeding the default channels when
// there is no file yet
void		mc_contacts_init(void);

// False when the card could not be read at startup. The tables are then
// defaults rather than the user's data, and every save is refused so
// those defaults cannot overwrite good files
uint8_t		mc_store_is_writable(void);

// Look again for a card that was not there at startup. Loads the stores
// and makes saving possible when one has appeared. Returns nonzero on
// the call that takes a newly found card into use
uint8_t		mc_store_recheck(void);

// ---------------------------------------------------------------------
// Contacts

uint8_t		mc_contacts_count(void);
MC_CONTACT	*mc_contacts_at(uint8_t idx);

// Find by full key, or by the one byte node hash a direct message
// carries. The hash is only 8 bits, so a collision is possible - the
// MAC check on the message is what finally decides
MC_CONTACT	*mc_contacts_find(const uint8_t pub_key[MC_EC_KEY_SIZE]);
MC_CONTACT	*mc_contacts_find_by_hash(uint8_t hash);

// Record an advert. Creates the entry if this node is new, refreshes it
// otherwise. Returns the entry, or NULL when the table is full of saved
// contacts and there is nothing to evict.
//
// created, when given, comes back nonzero only for a node we had not
// heard before - the caller uses it to decide whether the table is worth
// writing to the card, so a busy mesh does not cause a write per advert
MC_CONTACT	*mc_contacts_observe(const uint8_t pub_key[MC_EC_KEY_SIZE],
								 const char *name, uint8_t role, uint32_t advert_ts,
								 int8_t snr, const uint8_t *path, uint8_t path_len,
								 uint8_t *created);

// Promote a heard node to a saved contact (this is "add contact"), and
// derive its message key while we are at it
uint8_t		mc_contacts_save_entry(uint8_t idx);
uint8_t		mc_contacts_forget(uint8_t idx);

// Derive and cache the shared secret for a contact. Costs a scalar
// multiplication, so it is done once and off the radio task
uint8_t		mc_contacts_derive_shared(MC_CONTACT *c);

uint8_t		mc_contacts_save(void);

// ---------------------------------------------------------------------
// Channels

uint8_t		mc_channels_count(void);
MC_CHANNEL	*mc_channels_at(uint8_t idx);
MC_CHANNEL	*mc_channels_find_by_hash(uint8_t hash);

// Add a channel from its key. The on-air hash is derived, not supplied
uint8_t		mc_channels_add(const char *name, const uint8_t key[MC_CHANNEL_KEY_SIZE]);

//*----------------------------------------------------------------------------
// Add a channel knowing only its name - everything else follows from it:
//
//   key  = SHA-256(name)[0..16]
//   hash = SHA-256(key)[0]
//
// Verified against the two named channels this radio already had keys
// for: "#test" derives the stored key and its on-air hash 0xD9, and
// "#jokes" derives the stored key byte for byte. The "public" channel is
// the exception - it ships with a fixed key that is not derived from any
// name, so it is added by key instead
uint8_t		mc_channels_add_by_name(const char *name);

// What the remove calls answer when the channel is one that must not
// go away. Distinct from a plain failure so the screen can say why
#define MC_CHANNEL_PROTECTED	2

// The default channel - the one whose key is fixed rather than derived,
// so it cannot be typed back in from its name once it is gone
uint8_t		mc_channel_is_default(const MC_CHANNEL *ch);

uint8_t		mc_channels_remove(uint8_t idx);
uint8_t		mc_channels_remove_by_hash(uint8_t hash);

// hash = SHA-256(key)[0]
uint8_t		mc_channel_hash_of(const uint8_t key[MC_CHANNEL_KEY_SIZE]);

// key = SHA-256(name)[0..16]
void		mc_channel_key_of(const char *name, uint8_t key[MC_CHANNEL_KEY_SIZE]);

uint8_t		mc_channels_save(void);

#endif
