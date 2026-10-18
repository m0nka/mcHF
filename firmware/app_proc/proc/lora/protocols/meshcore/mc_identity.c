/************************************************************************************
**                                                                                 **
**                                 mcHF QRP Transceiver                            **
**                         Krassi Atanassov - M0NKA, 2013-2026                     **
**                                                                                 **
**---------------------------------------------------------------------------------**
**                                                                                 **
**  File name:		mc_identity.c                                                  **
**  Description:	This radio's MeshCore node identity                            **
**  Licence:		https://github.com/m0nka/mcHF/blob/main/LICENSE                **
************************************************************************************/
#include "main.h"
#include "mchf_pro_board.h"

#if defined (CONTEXT_LORA) && defined(MESHCORE)

#include <string.h>

#include "ff.h"
#include "rtc.h"							// RTC backup registers, the key mirror
#include "virt_eeprom.h"					// only to undo where v0.53.96 put the key

#include "advert.h"
#include "mc_ec.h"
#include "mc_rand.h"

#include "mc_identity.h"

// On disk header - the magic catches a stale or foreign file and the
// version lets the record grow later without bricking the identity
#define MC_ID_MAGIC				0x4B484349UL		// "ICHK"
#define MC_ID_VERSION			1

typedef struct
{
	uint32_t	magic;
	uint16_t	version;
	uint16_t	len;								// bytes that follow
	uint8_t		seed[MC_EC_SEED_SIZE];
	uint8_t		pub[MC_EC_KEY_SIZE];
	char		name[MC_NAME_MAX + 1];
	uint8_t		role;
	uint8_t		weak_entropy;
	uint8_t		pad[2];

} MC_ID_RECORD;

static MC_IDENTITY	mc_id;
static FIL			mc_id_fil;

// ---------------------------------------------------------------------
// Battery backed SRAM mirror

//*----------------------------------------------------------------------------
//* Function Name       : mc_identity_bkp_load
//* Object              : recover the seed from the backup registers
//* Notes    			: returns 0 when a seed was there. Only the seed
//*						: is mirrored - the public key is derived from it
//*						: and the name comes off the card
//* Context    			: CONTEXT_MESHCORE
//*----------------------------------------------------------------------------
static uint8_t mc_identity_bkp_load(uint8_t seed[MC_EC_SEED_SIZE])
{
	uint32_t	w;
	uint8_t		i;

	if(k_BkupRestoreParameter(MC_IDENTITY_BKP_MAGIC_REG) != MC_IDENTITY_BKP_MAGIC)
		return 1;

	for(i = 0; i < (MC_EC_SEED_SIZE / 4); i++)
	{
		w = k_BkupRestoreParameter(MC_IDENTITY_BKP_SEED_REG + i);

		seed[(i * 4) + 0] = (uint8_t)(w);
		seed[(i * 4) + 1] = (uint8_t)(w >> 8);
		seed[(i * 4) + 2] = (uint8_t)(w >> 16);
		seed[(i * 4) + 3] = (uint8_t)(w >> 24);
	}

	return 0;
}

static void mc_identity_bkp_store(const uint8_t seed[MC_EC_SEED_SIZE])
{
	uint32_t	w;
	uint8_t		i;

	// Magic cleared first, so a reset partway through leaves the mirror
	// invalid rather than holding half of one key and half of another
	k_BkupSaveParameter(MC_IDENTITY_BKP_MAGIC_REG, 0);

	for(i = 0; i < (MC_EC_SEED_SIZE / 4); i++)
	{
		w =  ((uint32_t)seed[(i * 4) + 0])		 |
			 ((uint32_t)seed[(i * 4) + 1] << 8)  |
			 ((uint32_t)seed[(i * 4) + 2] << 16) |
			 ((uint32_t)seed[(i * 4) + 3] << 24);

		k_BkupSaveParameter(MC_IDENTITY_BKP_SEED_REG + i, w);
	}

	k_BkupSaveParameter(MC_IDENTITY_BKP_MAGIC_REG, MC_IDENTITY_BKP_MAGIC);
}

//*----------------------------------------------------------------------------
//* Function Name       : mc_identity_fill
//* Object              : build the on disk / in backup record from the
//*						: identity we are holding
//* Context    			: CONTEXT_MESHCORE
//*----------------------------------------------------------------------------
static void mc_identity_fill(MC_ID_RECORD *rec)
{
	memset(rec, 0, sizeof(*rec));

	rec->magic		= MC_ID_MAGIC;
	rec->version	= MC_ID_VERSION;
	rec->len		= sizeof(MC_ID_RECORD) - 8;

	memcpy(rec->seed, mc_id.seed, MC_EC_SEED_SIZE);
	memcpy(rec->pub,  mc_id.pub,  MC_EC_KEY_SIZE);
	memcpy(rec->name, mc_id.name, MC_NAME_MAX);

	rec->role			= mc_id.role;
	rec->weak_entropy	= mc_id.weak_entropy;
}

//*----------------------------------------------------------------------------
//* Function Name       : mc_identity_adopt
//* Object              : take a validated record as our identity
//* Notes    			: the public key is stored but derived data, so
//*						: it is recomputed and a record whose halves do
//*						: not belong together is rejected
//* Context    			: CONTEXT_MESHCORE
//*----------------------------------------------------------------------------
static uint8_t mc_identity_adopt(const MC_ID_RECORD *rec)
{
	uint8_t	check[MC_EC_KEY_SIZE];

	mc_ec_ed25519_pubkey(check, rec->seed);

	if(memcmp(check, rec->pub, MC_EC_KEY_SIZE) != 0)
		return 1;

	memcpy(mc_id.seed, rec->seed, MC_EC_SEED_SIZE);
	memcpy(mc_id.pub,  rec->pub,  MC_EC_KEY_SIZE);

	memcpy(mc_id.name, rec->name, MC_NAME_MAX);
	mc_id.name[MC_NAME_MAX] = 0;

	mc_id.role			= rec->role;
	mc_id.weak_entropy	= rec->weak_entropy;
	mc_id.valid			= 1;

	return 0;
}

//*----------------------------------------------------------------------------
//* Function Name       : mc_identity_default_name
//* Object              : a name to start from, distinct per radio so two
//*						: units on the same bench are telling apart
//* Context    			: CONTEXT_LORA
//*----------------------------------------------------------------------------
static void mc_identity_default_name(void)
{
	snprintf(mc_id.name, sizeof(mc_id.name), "mcHF-%02X%02X",
			 mc_id.pub[0], mc_id.pub[1]);
}

//*----------------------------------------------------------------------------
//* Function Name       : mc_identity_load
//* Object              : read the key file
//* Notes    			: returns 0 when a usable identity was loaded
//* Context    			: CONTEXT_LORA
//*----------------------------------------------------------------------------
static uint8_t mc_identity_load(void)
{
	MC_ID_RECORD	rec;
	UINT			got = 0;

	if(f_open(&mc_id_fil, MC_IDENTITY_FILE, FA_READ) != FR_OK)
		return 1;

	if(f_read(&mc_id_fil, &rec, sizeof(rec), &got) != FR_OK)
	{
		f_close(&mc_id_fil);
		return 2;
	}

	f_close(&mc_id_fil);

	if(got != sizeof(rec))
		return 3;

	if((rec.magic != MC_ID_MAGIC) || (rec.version != MC_ID_VERSION))
		return 4;

	if(mc_identity_adopt(&rec))
	{
		printf("meshcore: key file mismatch, ignoring \r\n");
		return 5;
	}

	if(mc_id.name[0] == 0)
		mc_identity_default_name();

	return 0;
}

//*----------------------------------------------------------------------------
//* Function Name       : mc_identity_save
//* Object              : persist the key - backup SRAM first, then the
//*						: card if it is there
//* Notes    			: the mirror is written unconditionally because
//*						: it is the copy that survives a boot where the
//*						: card does not come up. A failure to write the
//*						: card is reported but does not lose the key
//* Context    			: CONTEXT_MESHCORE
//*----------------------------------------------------------------------------
uint8_t mc_identity_save(void)
{
	MC_ID_RECORD	rec;
	UINT			put = 0;
	FRESULT			res;

	if(!mc_id.valid)
		return 1;

	mc_identity_fill(&rec);

	// Mirror first - it cannot fail and needs no filesystem
	mc_identity_bkp_store(mc_id.seed);

	// The directory is normally there already, but a fresh card has
	// nothing on it - ignore "exists"
	f_mkdir("0://meshcore");

	res = f_open(&mc_id_fil, MC_IDENTITY_FILE, FA_WRITE | FA_CREATE_ALWAYS);
	if(res != FR_OK)
	{
		printf("meshcore: key save open err(%d) \r\n", res);
		return 2;
	}

	res = f_write(&mc_id_fil, &rec, sizeof(rec), &put);

	f_close(&mc_id_fil);

	if((res != FR_OK) || (put != sizeof(rec)))
	{
		printf("meshcore: key save write err(%d) \r\n", res);
		return 3;
	}

	return 0;
}

//*----------------------------------------------------------------------------
//* Function Name       : mc_identity_create
//* Object              : make a brand new keypair
//* Context    			: CONTEXT_LORA
//*----------------------------------------------------------------------------
static uint8_t mc_identity_create(void)
{
	memset(&mc_id, 0, sizeof(mc_id));

	mc_id.weak_entropy = mc_rand_bytes(mc_id.seed, MC_EC_SEED_SIZE);

	if(mc_id.weak_entropy)
		printf("meshcore: WARNING - TRNG unavailable, identity seeded from timing \r\n");

	mc_ec_ed25519_pubkey(mc_id.pub, mc_id.seed);

	mc_id.role	= MESHCORE_DEVICE_ROLE_CHAT_NODE;
	mc_id.valid	= 1;

	mc_identity_default_name();

	mc_id.source = MC_ID_SRC_NEW;

	printf("meshcore: new identity %02X%02X%02X%02X, name %s \r\n",
			mc_id.pub[0], mc_id.pub[1], mc_id.pub[2], mc_id.pub[3], mc_id.name);

	return mc_identity_save();
}

//*----------------------------------------------------------------------------
//* Function Name       : mc_identity_init
//* Object              : find this radio's identity, or make one
//* Notes    			: card first - it is the portable, backup-able
//*						: copy - then the battery backed mirror, and
//*						: only then a new key. Whichever copy is found,
//*						: the other is brought up to date, so the two
//*						: heal each other
//* Context    			: CONTEXT_MESHCORE
//*----------------------------------------------------------------------------
//*----------------------------------------------------------------------------
//* Function Name       : mc_identity_scrub_old_mirror
//* Object              : clear the backup SRAM bytes v0.53.96 briefly used
//* Notes    			: that build mirrored the key at EEP offset 0x400,
//*						: which is inside the region radio_init sums into
//*						: the settings checksum - so it invalidated the
//*						: radio's own stored bands. The region was zero
//*						: before, so zeroing it restores that checksum.
//*						: Safe to delete once no radio is running .96
//* Context    			: CONTEXT_MESHCORE
//*----------------------------------------------------------------------------
static void mc_identity_scrub_old_mirror(void)
{
	uint16_t	i;
	uint8_t		dirty = 0;

	for(i = 0; i < 128; i++)
	{
		if(virt_eeprom_read((ushort)(0x400 + i)) != 0)
		{
			dirty = 1;
			break;
		}
	}

	if(!dirty)
		return;

	for(i = 0; i < 128; i++)
		virt_eeprom_write((ushort)(0x400 + i), 0);

	printf("meshcore: cleared old key mirror from backup SRAM \r\n");
}

uint8_t mc_identity_init(void)
{
	if(mc_id.valid)
		return 0;

	mc_identity_scrub_old_mirror();

	if(mc_identity_load() == 0)
	{
		mc_id.source = MC_ID_SRC_CARD;

		printf("meshcore: identity %02X%02X%02X%02X from card, name %s \r\n",
				mc_id.pub[0], mc_id.pub[1], mc_id.pub[2], mc_id.pub[3], mc_id.name);

		// Refresh the mirror so a later card-less boot finds this key
		mc_identity_bkp_store(mc_id.seed);

		return 0;
	}

	// No card, or nothing on it. The mirror is what stops this radio
	// becoming a different node every time the SD fails to come up
	{
		uint8_t	seed[MC_EC_SEED_SIZE];

		if(mc_identity_bkp_load(seed) == 0)
		{
			memset(&mc_id, 0, sizeof(mc_id));

			memcpy(mc_id.seed, seed, MC_EC_SEED_SIZE);
			mc_ec_ed25519_pubkey(mc_id.pub, mc_id.seed);

			mc_id.role		= MESHCORE_DEVICE_ROLE_CHAT_NODE;
			mc_id.valid		= 1;
			mc_id.source	= MC_ID_SRC_BACKUP;

			// The name is not mirrored - it comes back with the card.
			// Until then the derived one is used, and it is derived from
			// the same key, so it is the same name as before
			mc_identity_default_name();

			printf("meshcore: identity %02X%02X%02X%02X from backup, name %s \r\n",
					mc_id.pub[0], mc_id.pub[1], mc_id.pub[2], mc_id.pub[3], mc_id.name);

			// Put it back on the card when there is one again
			mc_identity_save();

			memset(seed, 0, sizeof(seed));

			return 0;
		}
	}

	return mc_identity_create();
}

//*----------------------------------------------------------------------------
//* Function Name       : mc_identity_recheck_card
//* Object              : a card has appeared - reconcile with it
//* Notes    			: the card is authoritative. If it holds a key we
//*						: adopt it, even though that changes our node id
//*						: mid session: it is the identity everyone who
//*						: has added this radio already knows. If it holds
//*						: none, the running key is written to it
//* Context    			: CONTEXT_MESHCORE
//*----------------------------------------------------------------------------
void mc_identity_recheck_card(void)
{
	uint8_t	was[MC_EC_KEY_SIZE];

	if(!mc_id.valid)
	{
		mc_identity_init();
		return;
	}

	if(mc_id.source == MC_ID_SRC_CARD)
		return;										// already using it

	memcpy(was, mc_id.pub, MC_EC_KEY_SIZE);

	if(mc_identity_load() == 0)
	{
		mc_id.source = MC_ID_SRC_CARD;

		// Mirror the card's key so a later card-less boot agrees
		mc_identity_bkp_store(mc_id.seed);

		if(memcmp(was, mc_id.pub, MC_EC_KEY_SIZE) != 0)
			printf("meshcore: identity %02X%02X%02X%02X restored from card (was %02X%02X) \r\n",
					mc_id.pub[0], mc_id.pub[1], mc_id.pub[2], mc_id.pub[3], was[0], was[1]);

		return;
	}

	// Card has no key of its own - keep ours and put it there
	mc_identity_save();
}

const MC_IDENTITY *mc_identity_get(void)
{
	return &mc_id;
}

uint8_t mc_identity_hash(void)
{
	return mc_id.pub[0];
}

uint8_t mc_identity_set_name(const char *name)
{
	if((name == NULL) || (!mc_id.valid))
		return 1;

	strncpy(mc_id.name, name, MC_NAME_MAX);
	mc_id.name[MC_NAME_MAX] = 0;

	if(mc_id.name[0] == 0)
		mc_identity_default_name();

	return mc_identity_save();
}

uint8_t mc_identity_regenerate(void)
{
	mc_id.valid = 0;

	return mc_identity_create();
}

#endif
