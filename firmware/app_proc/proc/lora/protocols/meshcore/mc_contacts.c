/************************************************************************************
**                                                                                 **
**                                 mcHF QRP Transceiver                            **
**                         Krassi Atanassov - M0NKA, 2013-2026                     **
**                                                                                 **
**---------------------------------------------------------------------------------**
**                                                                                 **
**  File name:		mc_contacts.c                                                  **
**  Description:	MeshCore contact book and channel keyring                      **
**  Licence:		https://github.com/m0nka/mcHF/blob/main/LICENSE                **
************************************************************************************/
#include "main.h"
#include "mchf_pro_board.h"

#if defined (CONTEXT_LORA) && defined(MESHCORE)

#include <string.h>

#include "ff.h"

#include "sha256.h"
#include "mc_ec.h"
#include "mc_identity.h"

#include "mc_contacts.h"

#define MC_STORE_MAGIC			0x54434B4DUL		// "MKCT"
#define MC_STORE_VERSION		1

static MC_CONTACT	mc_contacts[MC_CONTACT_MAX];
static MC_CHANNEL	mc_channels[MC_CHANNEL_MAX];

static FIL			mc_store_fil;

// Set once the card has been read successfully. While it is clear the
// tables in RAM are defaults, not the user's data, and writing them out
// would overwrite good files with those defaults - which is exactly what
// would happen on a boot where the SD fails to initialise (seen on this
// radio as "card init failed(252)"). So writes are refused until a read
// has proved the filesystem is there
static uint8_t		mc_store_ok = 0;

// Set when loading had to re-key a channel whose name was stored before
// names were folded to lower case - the corrected table is written back
// once the file is closed
static uint8_t		mc_chan_migrated = 0;

static uint8_t		mc_store_probe(void);

uint8_t mc_store_is_writable(void)
{
	return mc_store_ok;
}

// "public" is the MeshCore default channel. It is the one channel whose
// key is not derived from its name - every node ships with this fixed
// key - so it is the only one that has to be written out here. The named
// channels (#test, #jokes, anything the user adds) all derive
//
// The old decoder's table listed #jokes against channel hash 0x84. That
// was wrong: the key it stored is exactly SHA-256("#jokes")[0..16], and
// that key hashes to 0x9F. The derivation is what reproduces every
// channel this radio has ever decoded, so 0x9F is the real hash
static const uint8_t	mc_key_public[MC_CHANNEL_KEY_SIZE] =
{
	0x8B, 0x33, 0x87, 0xE9, 0xC5, 0xCD, 0xEA, 0x6A,
	0xC9, 0xE5, 0xED, 0xBA, 0xA1, 0x15, 0xCD, 0x72
};

//*----------------------------------------------------------------------------
//* Function Name       : mc_channel_hash_of
//* Object              : the one byte channel id that travels on air
//* Notes    			: derived, not configured - checked against two
//*						: captured channels (public -> 0x11, #test -> 0xD9)
//* Context    			: any
//*----------------------------------------------------------------------------
uint8_t mc_channel_hash_of(const uint8_t key[MC_CHANNEL_KEY_SIZE])
{
	Sha256Context	ctx;
	SHA256_HASH		hash;

	Sha256Initialise(&ctx);
	Sha256Update(&ctx, (void *)key, MC_CHANNEL_KEY_SIZE);
	Sha256Finalise(&ctx, &hash);

	return hash.bytes[0];
}

// ---------------------------------------------------------------------
// Channels

uint8_t mc_channels_count(void)
{
	uint8_t	i, n = 0;

	for(i = 0; i < MC_CHANNEL_MAX; i++)
		if(mc_channels[i].in_use)
			n++;

	return n;
}

MC_CHANNEL *mc_channels_at(uint8_t idx)
{
	uint8_t	i, n = 0;

	for(i = 0; i < MC_CHANNEL_MAX; i++)
	{
		if(!mc_channels[i].in_use)
			continue;

		if(n == idx)
			return &mc_channels[i];

		n++;
	}

	return NULL;
}

MC_CHANNEL *mc_channels_find_by_hash(uint8_t hash)
{
	uint8_t	i;

	for(i = 0; i < MC_CHANNEL_MAX; i++)
		if((mc_channels[i].in_use) && (mc_channels[i].hash == hash))
			return &mc_channels[i];

	return NULL;
}

void mc_channel_key_of(const char *name, uint8_t key[MC_CHANNEL_KEY_SIZE])
{
	Sha256Context	ctx;
	SHA256_HASH		hash;

	if((name == NULL) || (key == NULL))
		return;

	Sha256Initialise(&ctx);
	Sha256Update(&ctx, (void *)name, (uint32_t)strlen(name));
	Sha256Finalise(&ctx, &hash);

	memcpy(key, hash.bytes, MC_CHANNEL_KEY_SIZE);
}

//*----------------------------------------------------------------------------
//* Function Name       : mc_channels_add_by_name
//* Object              : add a channel knowing only its name
//* Notes    			: the name is folded to lower case first, because
//*						: the key is a hash OF the name - so "#mcHF" and
//*						: "#mchf" are not two spellings of one channel,
//*						: they are two different keys that cannot read
//*						: each other. The phone apps normalise the same
//*						: way, so this is what makes them interoperate
//* Context    			: CONTEXT_MESHCHAT
//*----------------------------------------------------------------------------
uint8_t mc_channels_add_by_name(const char *name)
{
	char	lower[MC_CHANNEL_NAME_MAX + 1];
	uint8_t	key[MC_CHANNEL_KEY_SIZE];
	uint8_t	i;

	if((name == NULL) || (name[0] == 0))
		return 1;

	for(i = 0; (i < MC_CHANNEL_NAME_MAX) && (name[i] != 0); i++)
		lower[i] = ((name[i] >= 'A') && (name[i] <= 'Z')) ? (char)(name[i] + ('a' - 'A'))
														 : name[i];

	lower[i] = 0;

	mc_channel_key_of(lower, key);

	// Stored lower case too, so what the screen shows is the name the
	// rest of the mesh is actually using
	return mc_channels_add(lower, key);
}

uint8_t mc_channels_add(const char *name, const uint8_t key[MC_CHANNEL_KEY_SIZE])
{
	uint8_t	i, hash;

	if((name == NULL) || (key == NULL))
		return 1;

	hash = mc_channel_hash_of(key);

	// Already got this key ? Just rename it
	for(i = 0; i < MC_CHANNEL_MAX; i++)
	{
		if((mc_channels[i].in_use) &&
		   (memcmp(mc_channels[i].key, key, MC_CHANNEL_KEY_SIZE) == 0))
		{
			strncpy(mc_channels[i].name, name, MC_CHANNEL_NAME_MAX);
			mc_channels[i].name[MC_CHANNEL_NAME_MAX] = 0;
			return 0;
		}
	}

	for(i = 0; i < MC_CHANNEL_MAX; i++)
	{
		if(mc_channels[i].in_use)
			continue;

		memset(&mc_channels[i], 0, sizeof(MC_CHANNEL));

		strncpy(mc_channels[i].name, name, MC_CHANNEL_NAME_MAX);
		mc_channels[i].name[MC_CHANNEL_NAME_MAX] = 0;

		memcpy(mc_channels[i].key, key, MC_CHANNEL_KEY_SIZE);

		mc_channels[i].hash	  = hash;
		mc_channels[i].in_use = 1;

		return 0;
	}

	return 2;									// keyring full
}

uint8_t mc_channels_remove(uint8_t idx)
{
	MC_CHANNEL	*ch = mc_channels_at(idx);

	if(ch == NULL)
		return 1;

	memset(ch, 0, sizeof(MC_CHANNEL));

	return mc_channels_save();
}

// ---------------------------------------------------------------------
// Contacts

uint8_t mc_contacts_count(void)
{
	uint8_t	i, n = 0;

	for(i = 0; i < MC_CONTACT_MAX; i++)
		if(mc_contacts[i].in_use)
			n++;

	return n;
}

MC_CONTACT *mc_contacts_at(uint8_t idx)
{
	uint8_t	i, n = 0;

	for(i = 0; i < MC_CONTACT_MAX; i++)
	{
		if(!mc_contacts[i].in_use)
			continue;

		if(n == idx)
			return &mc_contacts[i];

		n++;
	}

	return NULL;
}

MC_CONTACT *mc_contacts_find(const uint8_t pub_key[MC_EC_KEY_SIZE])
{
	uint8_t	i;

	for(i = 0; i < MC_CONTACT_MAX; i++)
		if((mc_contacts[i].in_use) &&
		   (memcmp(mc_contacts[i].pub_key, pub_key, MC_EC_KEY_SIZE) == 0))
			return &mc_contacts[i];

	return NULL;
}

MC_CONTACT *mc_contacts_find_by_hash(uint8_t hash)
{
	uint8_t	i;

	// Saved contacts first - a collision between a saved contact and
	// some node we merely overheard should resolve to the saved one
	for(i = 0; i < MC_CONTACT_MAX; i++)
		if((mc_contacts[i].in_use) && (mc_contacts[i].saved) &&
		   (mc_contacts[i].pub_key[0] == hash))
			return &mc_contacts[i];

	for(i = 0; i < MC_CONTACT_MAX; i++)
		if((mc_contacts[i].in_use) && (mc_contacts[i].pub_key[0] == hash))
			return &mc_contacts[i];

	return NULL;
}

//*----------------------------------------------------------------------------
//* Function Name       : mc_contacts_evict
//* Object              : make room by dropping the stalest unsaved entry
//* Notes    			: saved contacts are never evicted - the table
//*						: only recycles nodes we just overheard
//* Context    			: CONTEXT_LORA / CONTEXT_MESHCHAT
//*----------------------------------------------------------------------------
static MC_CONTACT *mc_contacts_evict(void)
{
	MC_CONTACT	*oldest = NULL;
	uint8_t		i;

	for(i = 0; i < MC_CONTACT_MAX; i++)
	{
		if(!mc_contacts[i].in_use)
			return &mc_contacts[i];

		if(mc_contacts[i].saved)
			continue;

		if((oldest == NULL) || ((long)(mc_contacts[i].heard_tick - oldest->heard_tick) < 0))
			oldest = &mc_contacts[i];
	}

	return oldest;
}

MC_CONTACT *mc_contacts_observe(const uint8_t pub_key[MC_EC_KEY_SIZE],
								const char *name, uint8_t role, uint32_t advert_ts,
								int8_t snr, const uint8_t *path, uint8_t path_len,
								uint8_t *created)
{
	MC_CONTACT	*c;

	if(created != NULL)
		*created = 0;

	if(pub_key == NULL)
		return NULL;

	// Never file ourselves away as a contact
	if(memcmp(pub_key, mc_identity_get()->pub, MC_EC_KEY_SIZE) == 0)
		return NULL;

	c = mc_contacts_find(pub_key);

	if(c == NULL)
	{
		c = mc_contacts_evict();

		if(c == NULL)
			return NULL;

		memset(c, 0, sizeof(MC_CONTACT));
		memcpy(c->pub_key, pub_key, MC_EC_KEY_SIZE);

		c->in_use = 1;

		if(created != NULL)
			*created = 1;
	}

	// A rename in a later advert is legitimate, an empty one is not
	if((name != NULL) && (name[0] != 0))
	{
		strncpy(c->name, name, MC_NAME_MAX);
		c->name[MC_NAME_MAX] = 0;
	}
	else if(c->name[0] == 0)
	{
		snprintf(c->name, sizeof(c->name), "node %02X%02X", pub_key[0], pub_key[1]);
	}

	c->role			= role;
	c->advert_ts	= advert_ts;
	c->heard_tick	= (uint32_t)xTaskGetTickCount();
	c->snr			= snr;
	c->heard		= 1;

	if((path != NULL) && (path_len > 0) && (path_len <= MESHCORE_MAX_PATH_SIZE))
	{
		memcpy(c->path, path, path_len);
		c->path_len = path_len;
	}

	return c;
}

uint8_t mc_contacts_derive_shared(MC_CONTACT *c)
{
	uint8_t	secret[MC_EC_KEY_SIZE];

	if(c == NULL)
		return 1;

	if(c->have_shared)
		return 0;

	if(mc_ec_shared_secret(secret, mc_identity_get()->seed, c->pub_key))
		return 2;

	// MeshCore's ciphers are AES-128, so the message key is the leading
	// half of the X25519 output
	memcpy(c->shared, secret, MC_CHANNEL_KEY_SIZE);
	c->have_shared = 1;

	memset(secret, 0, sizeof(secret));

	return 0;
}

uint8_t mc_contacts_save_entry(uint8_t idx)
{
	MC_CONTACT	*c = mc_contacts_at(idx);

	if(c == NULL)
		return 1;

	c->saved = 1;

	mc_contacts_derive_shared(c);

	return mc_contacts_save();
}

uint8_t mc_contacts_forget(uint8_t idx)
{
	MC_CONTACT	*c = mc_contacts_at(idx);

	if(c == NULL)
		return 1;

	memset(c, 0, sizeof(MC_CONTACT));

	return mc_contacts_save();
}

// ---------------------------------------------------------------------
// Persistence. Only saved contacts are written - the overheard ones are
// rebuilt from the air on the next boot

typedef struct
{
	uint32_t	magic;
	uint16_t	version;
	uint16_t	count;
	uint16_t	entry_size;
	uint16_t	pad;

} MC_STORE_HEADER;

//*----------------------------------------------------------------------------
//* Function Name       : mc_contacts_save
//* Object              : write the contact table to the SD card
//* Notes    			: heard nodes are written too, not just the ones
//*						: the user added. A node only enters the table
//*						: when it advertises, and adverts are ten minutes
//*						: apart, so without this the list sits empty for
//*						: a long while after every boot and ADD has
//*						: nothing to work with. The saved flag is stored,
//*						: so the two kinds stay distinguishable
//* Context    			: CONTEXT_MESHCHAT
//*----------------------------------------------------------------------------
uint8_t mc_contacts_save(void)
{
	MC_STORE_HEADER	hdr;
	UINT			put = 0;
	uint8_t			i;
	FRESULT			res;

	if(!mc_store_ok)
	{
		printf("meshchat: card not readable, refusing to overwrite contacts \r\n");
		return 1;
	}

	memset(&hdr, 0, sizeof(hdr));

	hdr.magic		= MC_STORE_MAGIC;
	hdr.version		= MC_STORE_VERSION;
	hdr.entry_size	= sizeof(MC_CONTACT);

	for(i = 0; i < MC_CONTACT_MAX; i++)
		if(mc_contacts[i].in_use)
			hdr.count++;

	f_mkdir("0://meshchat");

	res = f_open(&mc_store_fil, MC_CONTACTS_FILE, FA_WRITE | FA_CREATE_ALWAYS);
	if(res != FR_OK)
	{
		printf("meshchat: contacts save err(%d) \r\n", res);
		return 1;
	}

	f_write(&mc_store_fil, &hdr, sizeof(hdr), &put);

	for(i = 0; i < MC_CONTACT_MAX; i++)
		if(mc_contacts[i].in_use)
			f_write(&mc_store_fil, &mc_contacts[i], sizeof(MC_CONTACT), &put);

	f_close(&mc_store_fil);

	return 0;
}

static void mc_contacts_load(void)
{
	MC_STORE_HEADER	hdr;
	MC_CONTACT		rec;
	UINT			got = 0;
	uint16_t		i;

	if(f_open(&mc_store_fil, MC_CONTACTS_FILE, FA_READ) != FR_OK)
		return;

	if((f_read(&mc_store_fil, &hdr, sizeof(hdr), &got) != FR_OK) || (got != sizeof(hdr)))
	{
		f_close(&mc_store_fil);
		return;
	}

	if((hdr.magic != MC_STORE_MAGIC) || (hdr.version != MC_STORE_VERSION) ||
	   (hdr.entry_size != sizeof(MC_CONTACT)))
	{
		printf("meshchat: contacts file version mismatch, ignoring \r\n");
		f_close(&mc_store_fil);
		return;
	}

	// Merged rather than written by index - this can run after the radio
	// has already heard nodes on air (a card inserted mid session), and
	// those entries must survive
	for(i = 0; (i < hdr.count) && (i < MC_CONTACT_MAX); i++)
	{
		MC_CONTACT	*slot;

		if((f_read(&mc_store_fil, &rec, sizeof(rec), &got) != FR_OK) || (got != sizeof(rec)))
			break;

		rec.in_use		= 1;
		rec.heard		= 0;					// not heard on air yet this boot
		rec.heard_tick	= 0;					// so it evicts ahead of anything live

		// The cached message key is derived from our own identity, so a
		// key file that has been replaced invalidates it. Cheap to redo
		rec.have_shared = 0;
		rec.unread		= 0;				// activity badge is per session

		slot = mc_contacts_find(rec.pub_key);

		if(slot != NULL)
		{
			// Already heard this node - keep the live entry, just adopt
			// the fact that the user had added it
			slot->saved = rec.saved;
			continue;
		}

		slot = mc_contacts_evict();

		if(slot == NULL)
			break;

		*slot = rec;
	}

	f_close(&mc_store_fil);

	{
		uint8_t	saved = 0, n;

		for(n = 0; n < MC_CONTACT_MAX; n++)
			if((mc_contacts[n].in_use) && (mc_contacts[n].saved))
				saved++;

		printf("meshchat: %d contacts loaded (%d added) \r\n", (int)i, (int)saved);
	}
}

uint8_t mc_channels_save(void)
{
	MC_STORE_HEADER	hdr;
	UINT			put = 0;
	uint8_t			i;
	FRESULT			res;

	if(!mc_store_ok)
	{
		printf("meshchat: card not readable, refusing to overwrite channels \r\n");
		return 1;
	}

	memset(&hdr, 0, sizeof(hdr));

	hdr.magic		= MC_STORE_MAGIC;
	hdr.version		= MC_STORE_VERSION;
	hdr.entry_size	= sizeof(MC_CHANNEL);
	hdr.count		= mc_channels_count();

	f_mkdir("0://meshchat");

	res = f_open(&mc_store_fil, MC_CHANNELS_FILE, FA_WRITE | FA_CREATE_ALWAYS);
	if(res != FR_OK)
	{
		printf("meshchat: channels save err(%d) \r\n", res);
		return 1;
	}

	f_write(&mc_store_fil, &hdr, sizeof(hdr), &put);

	for(i = 0; i < MC_CHANNEL_MAX; i++)
		if(mc_channels[i].in_use)
			f_write(&mc_store_fil, &mc_channels[i], sizeof(MC_CHANNEL), &put);

	f_close(&mc_store_fil);

	return 0;
}

//*----------------------------------------------------------------------------
//* Function Name       : mc_channels_load
//* Object              : read the keyring, or seed the defaults
//* Notes    			: returns nonzero when nothing was loaded and the
//*						: caller should install the built in channels
//* Context    			: CONTEXT_MESHCHAT
//*----------------------------------------------------------------------------
static uint8_t mc_channels_load(void)
{
	MC_STORE_HEADER	hdr;
	MC_CHANNEL		rec;
	UINT			got = 0;
	uint16_t		i;
	uint8_t			n = 0;

	if(f_open(&mc_store_fil, MC_CHANNELS_FILE, FA_READ) != FR_OK)
		return 1;

	if((f_read(&mc_store_fil, &hdr, sizeof(hdr), &got) != FR_OK) || (got != sizeof(hdr)))
	{
		f_close(&mc_store_fil);
		return 1;
	}

	if((hdr.magic != MC_STORE_MAGIC) || (hdr.version != MC_STORE_VERSION) ||
	   (hdr.entry_size != sizeof(MC_CHANNEL)))
	{
		f_close(&mc_store_fil);
		return 1;
	}

	for(i = 0; (i < hdr.count) && (i < MC_CHANNEL_MAX); i++)
	{
		uint8_t	from_name[MC_CHANNEL_KEY_SIZE];
		uint8_t	mixed_case = 0, c;

		if((f_read(&mc_store_fil, &rec, sizeof(rec), &got) != FR_OK) || (got != sizeof(rec)))
			break;

		for(c = 0; rec.name[c] != 0; c++)
			if((rec.name[c] >= 'A') && (rec.name[c] <= 'Z'))
				mixed_case = 1;

		// A channel added before names were folded to lower case carries
		// a key hashed from the mixed case spelling, which no other node
		// shares. Spotted by the key still matching its own name; that
		// is what distinguishes a derived channel from "public", whose
		// key is fixed and must be left alone
		mc_channel_key_of(rec.name, from_name);

		if((mixed_case) && (memcmp(from_name, rec.key, MC_CHANNEL_KEY_SIZE) == 0))
		{
			printf("meshchat: channel '%s' re-keyed to lower case \r\n", rec.name);

			mc_channels_add_by_name(rec.name);		// lowercases and re-derives
			mc_chan_migrated = 1;
		}
		else
		{
			// Added rather than written by index - the table may already
			// hold the seeded defaults when a card turns up mid session,
			// and mc_channels_add dedups by key and picks a free slot
			mc_channels_add(rec.name, rec.key);
		}

		n++;
	}

	f_close(&mc_store_fil);

	printf("meshchat: %d channels loaded \r\n", (int)n);

	return (n == 0) ? 1 : 0;
}

//*----------------------------------------------------------------------------
//* Function Name       : mc_store_probe
//* Object              : is there a filesystem to read and write ?
//* Notes    			: a file that is simply absent still counts as a
//*						: working card - what we are ruling out is the
//*						: card being down altogether
//* Context    			: CONTEXT_MESHCHAT
//*----------------------------------------------------------------------------
static uint8_t mc_store_probe(void)
{
	FATFS	*fs;
	DWORD	clusters;

	return (f_getfree("0://", &clusters, &fs) == FR_OK) ? 1 : 0;
}


//*----------------------------------------------------------------------------
//* Function Name       : mc_store_recheck
//* Object              : has a card turned up since we last looked ?
//* Notes    			: the store starts read-only on a boot with no
//*						: card, and stays that way until this says
//*						: otherwise - which is what makes insert-card-
//*						: later work instead of needing a restart.
//*						: Returns nonzero when the card has just been
//*						: taken into use
//* Context    			: CONTEXT_MESHCHAT
//*----------------------------------------------------------------------------
uint8_t mc_store_recheck(void)
{
	if(mc_store_ok)
		return 0;

	if(!mc_store_probe())
		return 0;

	mc_store_ok = 1;

	printf("meshchat: card detected, loading stores \r\n");

	// Whatever the card holds wins for channels; if it holds none, the
	// set we have been running on is written out
	if(mc_channels_load())
		mc_channels_save();
	else if(mc_chan_migrated)
		mc_channels_save();

	// Contacts merge - anything heard while there was no card is kept
	mc_contacts_load();
	mc_contacts_save();

	return 1;
}

void mc_contacts_init(void)
{
	memset(mc_contacts, 0, sizeof(mc_contacts));
	memset(mc_channels, 0, sizeof(mc_channels));

	mc_store_ok = mc_store_probe();

	if(!mc_store_ok)
	{
		// Run from defaults this boot, but never write them back - the
		// card may hold channels and contacts the user added, and this
		// is a card that did not come up, not a card that is empty
		printf("meshchat: no card - running on defaults, nothing will be saved \r\n");

		mc_channels_add("public", mc_key_public);
		mc_channels_add_by_name("#test");
		mc_channels_add_by_name("#jokes");

		return;
	}

	if(mc_channels_load())
	{
		// Card is there but has no keyring yet - seed the channels this
		// radio was already listening to. "public" ships with a fixed
		// key; the named ones derive from their names
		mc_channels_add("public", mc_key_public);
		mc_channels_add_by_name("#test");
		mc_channels_add_by_name("#jokes");

		mc_channels_save();
	}
	else if(mc_chan_migrated)
	{
		// A channel was re-keyed on the way in - put the corrected
		// keyring back on the card
		mc_channels_save();
	}

	mc_contacts_load();
}

#endif
