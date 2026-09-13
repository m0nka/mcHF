/************************************************************************************
**                                                                                 **
**                                 mcHF QRP Transceiver                            **
**                         Krassi Atanassov - M0NKA, 2013-2026                     **
**                                                                                 **
**---------------------------------------------------------------------------------**
**                                                                                 **
**  File name:		mc_tx.c                                                        **
**  Description:	Builds outgoing MeshCore packets                               **
**  Licence:		https://github.com/m0nka/mcHF/blob/main/LICENSE                **
************************************************************************************/
//
// The encrypted payloads mirror exactly what the decoder in mc_rx.c
// takes apart, which is itself modelled on real traffic:
//
//   plaintext  = timestamp(4, LE) | text_type(1) | text
//   ciphertext = AES-128-ECB(key) over that, zero padded to 16 bytes
//   mac        = HMAC-SHA256(key, ciphertext), first 2 bytes
//
// Zero padding is safe because the receiver treats the tail as a C
// string - the trailing zeroes simply terminate it
//
#include "main.h"
#include "mchf_pro_board.h"

#if defined (CONTEXT_LORA) && defined(MESHCORE)

#include <string.h>

#include "aes.h"
#include "hmac_sha256.h"
#include "packet.h"
#include "advert.h"
#include "grp_txt.h"
#include "request.h"

#include "sha256.h"
#include "mc_ec.h"
#include "mc_identity.h"
#include "mc_contacts.h"

#include "mc_tx.h"

//*----------------------------------------------------------------------------
//* Function Name       : mc_tx_seal
//* Object              : build and encrypt the common text body
//* Notes    			: returns the ciphertext length, 0 on error. out
//*						: must have room for MESHCORE_MAX_PAYLOAD_SIZE
//* Context    			: CONTEXT_MESHCHAT
//*----------------------------------------------------------------------------
static uint8_t mc_tx_seal(uint8_t *out, uint8_t *mac,
						  const uint8_t *key, uint8_t mac_key_len,
						  const char *body, uint32_t timestamp)
{
	struct AES_ctx	ctx;
	uint8_t			plain[MESHCORE_MAX_PAYLOAD_SIZE];
	uint16_t		len;
	uint8_t			blocks, i;

	if((out == NULL) || (mac == NULL) || (key == NULL) || (body == NULL))
		return 0;

	len = (uint16_t)strlen(body);

	if(len > MC_TX_TEXT_MAX)
		len = MC_TX_TEXT_MAX;

	memset(plain, 0, sizeof(plain));

	memcpy(plain, &timestamp, sizeof(uint32_t));
	plain[4] = MC_TEXT_TYPE_PLAIN;

	memcpy(plain + 5, body, len);

	len += 5;

	// Round up to whole AES blocks
	blocks = (uint8_t)((len + (MESHCORE_CIPHER_BLOCK_SIZE - 1)) / MESHCORE_CIPHER_BLOCK_SIZE);

	if((blocks * MESHCORE_CIPHER_BLOCK_SIZE) > MESHCORE_MAX_PAYLOAD_SIZE)
		return 0;

	AES_init_ctx(&ctx, key);

	for(i = 0; i < blocks; i++)
		AES_ECB_encrypt(&ctx, plain + (i * MESHCORE_CIPHER_BLOCK_SIZE));

	len = (uint16_t)(blocks * MESHCORE_CIPHER_BLOCK_SIZE);

	memcpy(out, plain, len);

	// 16 for a channel key, 32 for a direct message's shared secret -
	// the cipher above always used the first 16
	hmac_sha256(key, mac_key_len, out, len, mac, MESHCORE_CIPHER_MAC_SIZE);

	memset(plain, 0, sizeof(plain));

	return (uint8_t)len;
}

uint8_t mc_tx_build_group_text(MC_TX_PACKET *pkt, const MC_CHANNEL *ch,
							   const char *sender, const char *text, uint32_t timestamp)
{
	meshcore_message_t	msg;
	meshcore_grp_txt_t	grp;

	// Sized for the worst case both parts can actually be, not for what
	// they usually are. snprintf in this tree does NOT bound a %s - see
	// PutString in common/print_f.c - so the buffer has to be big enough
	// on its own, and the copies below are length limited by hand
	char				body[MC_NAME_MAX + 2 + MC_TX_TEXT_MAX + 1];
	uint16_t			n = 0;

	if((pkt == NULL) || (ch == NULL) || (text == NULL))
		return 1;

	// MeshCore carries the sender inside the encrypted text, as
	// "name: message" - there is no sender field in a group header
	if((sender != NULL) && (sender[0] != 0))
	{
		strncpy(body, sender, MC_NAME_MAX);
		body[MC_NAME_MAX] = 0;

		n = (uint16_t)strlen(body);

		body[n++] = ':';
		body[n++] = ' ';
	}

	strncpy(body + n, text, MC_TX_TEXT_MAX);
	body[n + MC_TX_TEXT_MAX] = 0;

	memset(&grp, 0, sizeof(grp));

	grp.channel_hash = ch->hash;
	grp.data_length  = mc_tx_seal(grp.data, grp.mac, ch->key, MC_CHANNEL_KEY_SIZE,
								  body, timestamp);

	if(grp.data_length == 0)
		return 2;

	memset(&msg, 0, sizeof(msg));

	msg.type		= MESHCORE_PAYLOAD_TYPE_GRP_TXT;
	msg.route		= MESHCORE_ROUTE_TYPE_FLOOD;
	msg.version		= 0;
	msg.path_length	= 0;

	if(meshcore_grp_txt_serialize(&grp, msg.payload, &msg.payload_length) < 0)
		return 3;

	if(meshcore_serialize(&msg, pkt->data, &pkt->len) < 0)
		return 4;

	return 0;
}

uint8_t mc_tx_build_direct_text(MC_TX_PACKET *pkt, const MC_CONTACT *to,
								const char *text, uint32_t timestamp,
								uint8_t ack_out[4])
{
	meshcore_message_t	msg;
	meshcore_request_t	req;

	if((pkt == NULL) || (to == NULL) || (text == NULL))
		return 1;

	if(!to->have_shared)
		return 2;

	memset(&req, 0, sizeof(req));

	req.destination_hash	= to->pub_key[0];
	req.source_hash			= mc_identity_hash();
	req.ciphertext_length	= mc_tx_seal(req.ciphertext, req.ciphher_mac,
										 to->shared, MC_DM_MAC_KEY_SIZE,
										 text, timestamp);

	if(req.ciphertext_length == 0)
		return 3;

	// What the far end will answer with, computed the same way it will
	// compute it: over the unpadded plaintext and OUR public key
	if(ack_out != NULL)
	{
		Sha256Context	sha;
		SHA256_HASH		dg;
		uint8_t			plain[5 + MC_TX_TEXT_MAX];
		uint16_t		n = (uint16_t)strlen(text);

		if(n > MC_TX_TEXT_MAX)
			n = MC_TX_TEXT_MAX;

		memcpy(plain, &timestamp, sizeof(uint32_t));
		plain[4] = MC_TEXT_TYPE_PLAIN;
		memcpy(plain + 5, text, n);

		Sha256Initialise(&sha);
		Sha256Update(&sha, plain, (uint32_t)(5 + n));
		Sha256Update(&sha, (void *)mc_identity_get()->pub, MC_EC_KEY_SIZE);
		Sha256Finalise(&sha, &dg);

		memcpy(ack_out, dg.bytes, 4);
	}

	memset(&msg, 0, sizeof(msg));

	msg.type	= MESHCORE_PAYLOAD_TYPE_TXT_MSG;
	msg.version	= 0;

	// Use the route the advert came back on when we have one, otherwise
	// flood and let the mesh work it out
	if(to->path_len > 0)
	{
		msg.route		= MESHCORE_ROUTE_TYPE_DIRECT;
		msg.path_length	= to->path_len;
		memcpy(msg.path, to->path, to->path_len);
	}
	else
	{
		msg.route		= MESHCORE_ROUTE_TYPE_FLOOD;
		msg.path_length	= 0;
	}

	if(meshcore_request_serialize(&req, msg.payload, &msg.payload_length) < 0)
		return 4;

	if(meshcore_serialize(&msg, pkt->data, &pkt->len) < 0)
		return 5;

	return 0;
}

//*----------------------------------------------------------------------------
//* Function Name       : mc_tx_build_path_ack
//* Object              : answer a direct message with a PATH carrying the
//*						: acknowledgement the sender is waiting for
//* Notes    			: the returned path is empty - we send back the
//*						: route we know, and a node that reached us by
//*						: flood learns the direct one from the reply
//* Context    			: CONTEXT_MESHCHAT
//*----------------------------------------------------------------------------
uint8_t mc_tx_build_path_ack(MC_TX_PACKET *pkt, const MC_CONTACT *to, const uint8_t ack[4])
{
	struct AES_ctx		ctx;
	meshcore_message_t	msg;
	meshcore_request_t	req;
	uint8_t				plain[MESHCORE_CIPHER_BLOCK_SIZE];

	if((pkt == NULL) || (to == NULL) || (ack == NULL))
		return 1;

	if(!to->have_shared)
		return 2;

	// path_len | extra type | ack, zero padded to one AES block
	memset(plain, 0, sizeof(plain));

	plain[0] = 0;									// no path of our own to offer
	plain[1] = MESHCORE_PAYLOAD_TYPE_ACK;

	memcpy(plain + 2, ack, 4);

	AES_init_ctx(&ctx, to->shared);
	AES_ECB_encrypt(&ctx, plain);

	memset(&req, 0, sizeof(req));

	req.destination_hash	= to->pub_key[0];
	req.source_hash			= mc_identity_hash();
	req.ciphertext_length	= sizeof(plain);

	memcpy(req.ciphertext, plain, sizeof(plain));

	hmac_sha256(to->shared, MC_DM_MAC_KEY_SIZE, req.ciphertext, req.ciphertext_length,
				req.ciphher_mac, MESHCORE_CIPHER_MAC_SIZE);

	memset(&msg, 0, sizeof(msg));

	msg.type		= MESHCORE_PAYLOAD_TYPE_PATH;
	msg.route		= MESHCORE_ROUTE_TYPE_FLOOD;
	msg.version		= 0;
	msg.path_length	= 0;

	if(meshcore_request_serialize(&req, msg.payload, &msg.payload_length) < 0)
		return 3;

	if(meshcore_serialize(&msg, pkt->data, &pkt->len) < 0)
		return 4;

	return 0;
}

//*----------------------------------------------------------------------------
//* Function Name       : mc_tx_build_advert
//* Object              : our own node advertisement
//* Notes    			: the signature covers pub_key | timestamp |
//*						: app_data. That convention is not guesswork - it
//*						: is what verifies against the two real off-air
//*						: adverts captured by this radio, see
//*						: claude/meshchat_test/test_advert.c
//* Context    			: CONTEXT_MESHCHAT
//*----------------------------------------------------------------------------
uint8_t mc_tx_build_advert(MC_TX_PACKET *pkt, uint32_t timestamp)
{
	const MC_IDENTITY	*id = mc_identity_get();
	meshcore_message_t	msg;
	meshcore_advert_t	adv;
	uint8_t				signed_msg[MC_EC_KEY_SIZE + 4 + 1 + MC_NAME_MAX];
	uint8_t				app_data[1 + MC_NAME_MAX];
	uint8_t				app_len, name_len, n;

	if((pkt == NULL) || (!id->valid))
		return 1;

	name_len = (uint8_t)strlen(id->name);

	if(name_len > MESHCORE_MAX_NAME_SIZE)
		name_len = MESHCORE_MAX_NAME_SIZE;

	// app_data is the flags byte followed by the optional fields. We
	// advertise a name and a role, nothing else
	app_data[0] = (uint8_t)((id->role & 0x0F) | 0x80);		// 0x80 = name present
	memcpy(app_data + 1, id->name, name_len);

	app_len = (uint8_t)(1 + name_len);

	// Signature input
	n = 0;

	memcpy(signed_msg + n, id->pub, MC_EC_KEY_SIZE);
	n += MC_EC_KEY_SIZE;

	memcpy(signed_msg + n, &timestamp, sizeof(uint32_t));
	n += sizeof(uint32_t);

	memcpy(signed_msg + n, app_data, app_len);
	n = (uint8_t)(n + app_len);

	memset(&adv, 0, sizeof(adv));

	memcpy(adv.pub_key, id->pub, MESHCORE_PUB_KEY_SIZE);
	adv.timestamp = timestamp;

	mc_ec_ed25519_sign(adv.signature, signed_msg, n, id->seed, id->pub);

	adv.role = (meshcore_device_role_t)id->role;

	adv.name_valid = true;
	memcpy(adv.name, id->name, name_len);
	adv.name[name_len] = 0;

	memset(&msg, 0, sizeof(msg));

	msg.type		= MESHCORE_PAYLOAD_TYPE_ADVERT;
	msg.route		= MESHCORE_ROUTE_TYPE_FLOOD;
	msg.version		= 0;
	msg.path_length	= 0;

	if(meshcore_advert_serialize(&adv, msg.payload, &msg.payload_length) < 0)
		return 2;

	if(meshcore_serialize(&msg, pkt->data, &pkt->len) < 0)
		return 3;

	return 0;
}

#endif
