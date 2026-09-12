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
	uint8_t			check[MC_EC_KEY_SIZE];
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

	// The public key is stored, but it is derived data - recompute it and
	// refuse a file whose halves do not belong together
	mc_ec_ed25519_pubkey(check, rec.seed);

	if(memcmp(check, rec.pub, MC_EC_KEY_SIZE) != 0)
	{
		printf("meshchat: key file mismatch, ignoring \r\n");
		return 5;
	}

	memcpy(mc_id.seed, rec.seed, MC_EC_SEED_SIZE);
	memcpy(mc_id.pub,  rec.pub,  MC_EC_KEY_SIZE);

	memcpy(mc_id.name, rec.name, MC_NAME_MAX);
	mc_id.name[MC_NAME_MAX] = 0;

	mc_id.role			= rec.role;
	mc_id.weak_entropy	= rec.weak_entropy;
	mc_id.valid			= 1;

	if(mc_id.name[0] == 0)
		mc_identity_default_name();

	return 0;
}

uint8_t mc_identity_save(void)
{
	MC_ID_RECORD	rec;
	UINT			put = 0;
	FRESULT			res;

	if(!mc_id.valid)
		return 1;

	memset(&rec, 0, sizeof(rec));

	rec.magic	= MC_ID_MAGIC;
	rec.version	= MC_ID_VERSION;
	rec.len		= sizeof(rec) - 8;

	memcpy(rec.seed, mc_id.seed, MC_EC_SEED_SIZE);
	memcpy(rec.pub,  mc_id.pub,  MC_EC_KEY_SIZE);
	memcpy(rec.name, mc_id.name, MC_NAME_MAX);

	rec.role			= mc_id.role;
	rec.weak_entropy	= mc_id.weak_entropy;

	// The directory is normally there already, but a fresh card has
	// nothing on it - ignore "exists"
	f_mkdir("0://meshchat");

	res = f_open(&mc_id_fil, MC_IDENTITY_FILE, FA_WRITE | FA_CREATE_ALWAYS);
	if(res != FR_OK)
	{
		printf("meshchat: key save open err(%d) \r\n", res);
		return 2;
	}

	res = f_write(&mc_id_fil, &rec, sizeof(rec), &put);

	f_close(&mc_id_fil);

	if((res != FR_OK) || (put != sizeof(rec)))
	{
		printf("meshchat: key save write err(%d) \r\n", res);
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
		printf("meshchat: WARNING - TRNG unavailable, identity seeded from timing \r\n");

	mc_ec_ed25519_pubkey(mc_id.pub, mc_id.seed);

	mc_id.role	= MESHCORE_DEVICE_ROLE_CHAT_NODE;
	mc_id.valid	= 1;

	mc_identity_default_name();

	printf("meshchat: new identity %02X%02X%02X%02X, name %s \r\n",
			mc_id.pub[0], mc_id.pub[1], mc_id.pub[2], mc_id.pub[3], mc_id.name);

	return mc_identity_save();
}

uint8_t mc_identity_init(void)
{
	if(mc_id.valid)
		return 0;

	if(mc_identity_load() == 0)
	{
		printf("meshchat: identity %02X%02X%02X%02X, name %s \r\n",
				mc_id.pub[0], mc_id.pub[1], mc_id.pub[2], mc_id.pub[3], mc_id.name);
		return 0;
	}

	return mc_identity_create();
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
