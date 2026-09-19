/************************************************************************************
**                                                                                 **
**                                 mcHF QRP Transceiver                            **
**                         Krassi Atanassov - M0NKA, 2013-2026                     **
**                                                                                 **
**---------------------------------------------------------------------------------**
**                                                                                 **
**  File name:		mc_identity.h                                                  **
**  Description:	This radio's MeshCore node identity                            **
**  Licence:		https://github.com/m0nka/mcHF/blob/main/LICENSE                **
************************************************************************************/
//
// A MeshCore node is its Ed25519 keypair. The public half is what goes
// out in an ADVERT and what other stations file away as our "contact";
// the first byte of it is the node hash that addresses direct messages.
//
// The keypair is generated once, on the first run that finds no key file
// on the SD card, and kept from then on - regenerating it makes this
// radio a different node to everyone who already has us as a contact
//
#ifndef __MC_IDENTITY_H
#define __MC_IDENTITY_H

#include <stdint.h>

#include "mc_ec.h"

#define MC_NAME_MAX				32

#define MC_IDENTITY_FILE		"0://meshcore/node.key"

// Mirror of the key in the RTC backup registers. The SD card is the
// authoritative copy - portable, and what the user can back up - but the
// card does not always come up, and a node that invents a new identity
// on every card-less boot is a different node to everyone who has added
// it. The mirror is what makes the identity stable regardless.
//
// Deliberately NOT in the backup SRAM virtual EEPROM: radio_init sums
// that whole 4K region into one checksum, so anything stored there
// invalidates the radio's own settings. The backup registers are a
// separate resource in the same always-on domain.
//
// Only the seed is kept - the public key derives from it, and the name
// comes back off the card
#define MC_IDENTITY_BKP_MAGIC	0x4D434B59UL		// "MCKY"

#define MC_IDENTITY_BKP_MAGIC_REG	RTC_BKP_DR12
#define MC_IDENTITY_BKP_SEED_REG	RTC_BKP_DR13	// DR13..DR20, 8 words

// Where the identity in use came from
#define MC_ID_SRC_NONE			0
#define MC_ID_SRC_CARD			1
#define MC_ID_SRC_BACKUP		2
#define MC_ID_SRC_NEW			3

typedef struct
{
	uint8_t		seed[MC_EC_SEED_SIZE];			// private, never leaves the radio
	uint8_t		pub[MC_EC_KEY_SIZE];			// advertised
	char		name[MC_NAME_MAX + 1];
	uint8_t		role;							// meshcore_device_role_t
	uint8_t		valid;
	uint8_t		weak_entropy;					// TRNG was unavailable at keygen
	uint8_t		source;							// MC_ID_SRC_xxx, where it was loaded from

} MC_IDENTITY;

// Load the identity from the SD card, or generate and save a new one.
// Returns 0 on success. Safe to call more than once - later calls are
// no-ops once an identity is loaded
uint8_t				mc_identity_init(void);

const MC_IDENTITY	*mc_identity_get(void);

// Called when a card turns up after startup. The card is authoritative,
// so a key found on it replaces the one in use; if it has none, the one
// in use is written to it
void				mc_identity_recheck_card(void);

// Node hash - the first byte of the public key, which is how MeshCore
// addresses a node in a direct message
uint8_t				mc_identity_hash(void);

// Rename. The new name only reaches the mesh with the next advert
uint8_t				mc_identity_set_name(const char *name);

// Throw the keypair away and make a new one. Everyone who has us as a
// contact will see a stranger afterwards
uint8_t				mc_identity_regenerate(void);

uint8_t				mc_identity_save(void);

#endif
