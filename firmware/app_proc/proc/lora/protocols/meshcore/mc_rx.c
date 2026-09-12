/************************************************************************************
**                                                                                 **
**                                 mcHF QRP Transceiver                            **
**                         Krassi Atanassov - M0NKA, 2013-2026                     **
**                                                                                 **
**---------------------------------------------------------------------------------**
**                                                                                 **
**  File name:		mc_rx.c                                                        **
**  Description:	Structured MeshCore receive decode                             **
**  Licence:		https://github.com/m0nka/mcHF/blob/main/LICENSE                **
************************************************************************************/
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

#include "mc_ec.h"
#include "mc_identity.h"
#include "mc_contacts.h"

#include "mc_rx.h"

// Fixed part of an advert payload: pub_key | timestamp | signature
#define MC_ADVERT_FIXED_LEN		(MESHCORE_PUB_KEY_SIZE + 4 + MESHCORE_SIGNATURE_SIZE)

const char *mc_rx_type_short(uint8_t type)
{
	switch(type)
	{
		case MESHCORE_PAYLOAD_TYPE_REQ:			return "Req";
		case MESHCORE_PAYLOAD_TYPE_RESPONSE:	return "Resp";
		case MESHCORE_PAYLOAD_TYPE_TXT_MSG:		return "TxtM";
		case MESHCORE_PAYLOAD_TYPE_ACK:			return "Ack";
		case MESHCORE_PAYLOAD_TYPE_ADVERT:		return "Adv";
		case MESHCORE_PAYLOAD_TYPE_GRP_TXT:		return "GTxt";
		case MESHCORE_PAYLOAD_TYPE_GRP_DATA:	return "GDat";
		case MESHCORE_PAYLOAD_TYPE_ANON_REQ:	return "AnReq";
		case MESHCORE_PAYLOAD_TYPE_PATH:		return "Path";
		case MESHCORE_PAYLOAD_TYPE_TRACE:		return "Trace";
		case MESHCORE_PAYLOAD_TYPE_MULTIPART:	return "Mprt";
		case MESHCORE_PAYLOAD_TYPE_RAW_CUSTOM:	return "Raw";
		default:								return "Unk";
	}
}

const char *mc_rx_role_name(uint8_t role)
{
	switch(role)
	{
		case MESHCORE_DEVICE_ROLE_CHAT_NODE:	return "Chat Node";
		case MESHCORE_DEVICE_ROLE_REPEATER:		return "Repeater";
		case MESHCORE_DEVICE_ROLE_ROOM_SERVER:	return "Room Server";
		case MESHCORE_DEVICE_ROLE_SENSOR:		return "Sensor";
		default:								return "Unknown";
	}
}

//*----------------------------------------------------------------------------
//* Function Name       : mc_rx_open
//* Object              : authenticate and decrypt a text body
//* Notes    			: the MAC is checked before anything is decrypted,
//*						: so a wrong key costs one HMAC and no more.
//*						: Returns 0 when the text came out
//* Context    			: CONTEXT_LORA
//*----------------------------------------------------------------------------
static uint8_t mc_rx_open(const uint8_t *cipher, uint8_t cipher_len,
						  const uint8_t *mac, const uint8_t key[MC_CHANNEL_KEY_SIZE],
						  uint32_t *timestamp, char *out, uint16_t out_len)
{
	struct AES_ctx	ctx;
	uint8_t			plain[MESHCORE_MAX_PAYLOAD_SIZE];
	uint8_t			check[MESHCORE_CIPHER_MAC_SIZE];
	uint8_t			blocks, i;
	uint16_t		text_len;

	if((cipher_len == 0) || (cipher_len > sizeof(plain)))
		return 1;

	// Whole blocks only - anything else is not ours
	if(cipher_len % MESHCORE_CIPHER_BLOCK_SIZE)
		return 2;

	hmac_sha256(key, MC_CHANNEL_KEY_SIZE, cipher, cipher_len, check, MESHCORE_CIPHER_MAC_SIZE);

	if(memcmp(check, mac, MESHCORE_CIPHER_MAC_SIZE) != 0)
		return 3;								// not this key

	memcpy(plain, cipher, cipher_len);

	AES_init_ctx(&ctx, key);

	blocks = (uint8_t)(cipher_len / MESHCORE_CIPHER_BLOCK_SIZE);

	for(i = 0; i < blocks; i++)
		AES_ECB_decrypt(&ctx, plain + (i * MESHCORE_CIPHER_BLOCK_SIZE));

	// timestamp(4) | text_type(1) | text, zero padded to the block size
	if(cipher_len < 5)
		return 4;

	if(timestamp != NULL)
		memcpy(timestamp, plain, sizeof(uint32_t));

	text_len = (uint16_t)(cipher_len - 5);

	if(text_len >= out_len)
		text_len = (uint16_t)(out_len - 1);

	memcpy(out, plain + 5, text_len);
	out[text_len] = 0;

	// The padding is zeroes, so the string ends itself - but trim any
	// stray control bytes so they cannot reach the display
	for(i = 0; out[i]; i++)
		if((uint8_t)out[i] < 0x20)
		{
			out[i] = 0;
			break;
		}

	memset(plain, 0, sizeof(plain));

	return 0;
}

//*----------------------------------------------------------------------------
//* Function Name       : mc_rx_split_sender
//* Object              : a group message carries "name: text" inside the
//*						: encrypted body - pull the two apart
//* Context    			: CONTEXT_LORA
//*----------------------------------------------------------------------------
static void mc_rx_split_sender(MC_RX_EVENT *ev)
{
	char	*sep = strstr(ev->text, ": ");
	uint16_t	n;

	if(sep == NULL)
		return;

	n = (uint16_t)(sep - ev->text);

	if((n == 0) || (n > MC_NAME_MAX))
		return;

	memcpy(ev->sender, ev->text, n);
	ev->sender[n] = 0;

	// Shuffle the message down over the name
	memmove(ev->text, sep + 2, strlen(sep + 2) + 1);
}

//*----------------------------------------------------------------------------
//* Function Name       : mc_rx_do_advert
//* Object              : decode and authenticate a node advertisement
//* Context    			: CONTEXT_LORA
//*----------------------------------------------------------------------------
static void mc_rx_do_advert(MC_RX_EVENT *ev, meshcore_message_t *msg)
{
	meshcore_advert_t	adv;
	uint8_t				signed_msg[MESHCORE_MAX_PAYLOAD_SIZE];
	uint8_t				app_len, n;

	if(meshcore_advert_deserialize(msg->payload, msg->payload_length, &adv) < 0)
		return;

	if(msg->payload_length < MC_ADVERT_FIXED_LEN)
		return;

	memcpy(ev->pub_key, adv.pub_key, MESHCORE_PUB_KEY_SIZE);

	ev->timestamp	= adv.timestamp;
	ev->role		= (uint8_t)adv.role;
	ev->kind		= MC_RX_ADVERT;

	if(adv.name_valid)
	{
		strncpy(ev->name, adv.name, MC_NAME_MAX);
		ev->name[MC_NAME_MAX] = 0;
	}
	else
		snprintf(ev->name, sizeof(ev->name), "node %02X%02X", adv.pub_key[0], adv.pub_key[1]);

	// Signature covers pub_key | timestamp | app_data. Rebuilt from the
	// raw payload rather than from the parsed struct, so re-serialising
	// quirks cannot change the bytes we check
	app_len = (uint8_t)(msg->payload_length - MC_ADVERT_FIXED_LEN);

	n = 0;

	memcpy(signed_msg + n, adv.pub_key, MESHCORE_PUB_KEY_SIZE);
	n += MESHCORE_PUB_KEY_SIZE;

	memcpy(signed_msg + n, &adv.timestamp, sizeof(uint32_t));
	n += sizeof(uint32_t);

	if(app_len)
	{
		memcpy(signed_msg + n, msg->payload + MC_ADVERT_FIXED_LEN, app_len);
		n = (uint8_t)(n + app_len);
	}

	ev->sig_ok = (mc_ec_ed25519_verify(adv.signature, signed_msg, n, adv.pub_key) == 0) ? 1 : 0;
}

//*----------------------------------------------------------------------------
//* Function Name       : mc_rx_do_group
//* Object              : group text on one of our channels
//* Context    			: CONTEXT_LORA
//*----------------------------------------------------------------------------
static void mc_rx_do_group(MC_RX_EVENT *ev, meshcore_message_t *msg)
{
	meshcore_grp_txt_t	grp;
	MC_CHANNEL			*ch;

	if(meshcore_grp_txt_deserialize(msg->payload, msg->payload_length, &grp) < 0)
		return;

	ev->channel_hash = grp.channel_hash;
	ev->kind		 = MC_RX_OTHER;

	ch = mc_channels_find_by_hash(grp.channel_hash);

	if(ch == NULL)
	{
		// Traffic on a channel we have no key for. Worth showing that
		// the mesh is alive, but there is nothing to read
		snprintf(ev->channel_name, sizeof(ev->channel_name), "ch %02X", grp.channel_hash);
		return;
	}

	strncpy(ev->channel_name, ch->name, MC_CHANNEL_NAME_MAX);
	ev->channel_name[MC_CHANNEL_NAME_MAX] = 0;

	if(mc_rx_open(grp.data, grp.data_length, grp.mac, ch->key,
				  &ev->timestamp, ev->text, sizeof(ev->text)) != 0)
		return;

	ev->mac_ok	= 1;
	ev->kind	= MC_RX_CHANNEL;

	mc_rx_split_sender(ev);
}

//*----------------------------------------------------------------------------
//* Function Name       : mc_rx_do_direct
//* Object              : a direct message - ours only if the destination
//*						: hash is our node hash and the MAC checks out
//*						: under the shared secret with the sender
//* Context    			: CONTEXT_LORA
//*----------------------------------------------------------------------------
static void mc_rx_do_direct(MC_RX_EVENT *ev, meshcore_message_t *msg)
{
	meshcore_request_t	req;
	MC_CONTACT			*c;
	uint8_t				i;

	if(meshcore_request_deserialize(msg->payload, msg->payload_length, &req) < 0)
		return;

	ev->dst_hash = req.destination_hash;
	ev->src_hash = req.source_hash;
	ev->kind	 = MC_RX_OTHER;

	// Addressed to someone else - we still saw it, we just relay-ignore it
	if(req.destination_hash != mc_identity_hash())
		return;

	// The source hash is 8 bits, so more than one contact can answer to
	// it. Try each candidate and let the MAC decide
	for(i = 0; i < mc_contacts_count(); i++)
	{
		c = mc_contacts_at(i);

		if(c == NULL)
			continue;

		if(c->pub_key[0] != req.source_hash)
			continue;

		if(!c->have_shared)
			continue;							// key not derived yet, cannot read it

		if(mc_rx_open(req.ciphertext, req.ciphertext_length, req.ciphher_mac,
					  c->shared, &ev->timestamp, ev->text, sizeof(ev->text)) != 0)
			continue;

		memcpy(ev->pub_key, c->pub_key, MC_EC_KEY_SIZE);

		strncpy(ev->sender, c->name, MC_NAME_MAX);
		ev->sender[MC_NAME_MAX] = 0;

		ev->mac_ok	= 1;
		ev->kind	= MC_RX_DIRECT;

		return;
	}

	// For us by address but we could not open it - an unknown sender, or
	// one we have not added as a contact yet
	snprintf(ev->sender, sizeof(ev->sender), "node %02X", req.source_hash);
}

uint8_t mc_rx_decode(const uint8_t *data, uint16_t size, int8_t snr, MC_RX_EVENT *ev)
{
	meshcore_message_t	msg;

	if((ev == NULL) || (data == NULL) || (size == 0))
		return MC_RX_NONE;

	memset(ev, 0, sizeof(MC_RX_EVENT));

	ev->snr = snr;

	if(size > MESHCORE_MAX_TRANS_UNIT)
		return MC_RX_NONE;

	// meshcore_deserialize takes a non-const pointer but only reads
	if(meshcore_deserialize((uint8_t *)data, (uint8_t)size, &msg) < 0)
		return MC_RX_NONE;

	ev->type	= (uint8_t)msg.type;
	ev->route	= (uint8_t)msg.route;
	ev->kind	= MC_RX_OTHER;

	strncpy(ev->type_short, mc_rx_type_short(ev->type), sizeof(ev->type_short) - 1);

	if(msg.path_length && (msg.path_length <= MESHCORE_MAX_PATH_SIZE))
	{
		memcpy(ev->path, msg.path, msg.path_length);
		ev->path_len = msg.path_length;
	}

	switch(msg.type)
	{
		case MESHCORE_PAYLOAD_TYPE_ADVERT:
			mc_rx_do_advert(ev, &msg);
			break;

		case MESHCORE_PAYLOAD_TYPE_GRP_TXT:
			mc_rx_do_group(ev, &msg);
			break;

		case MESHCORE_PAYLOAD_TYPE_TXT_MSG:
			mc_rx_do_direct(ev, &msg);
			break;

		case MESHCORE_PAYLOAD_TYPE_ACK:
			ev->kind = MC_RX_ACK;
			break;

		default:
			break;
	}

	return ev->kind;
}

//*----------------------------------------------------------------------------
//* Function Name       : mc_rx_format
//* Object              : the one line summary for the spectrum notice
//* Context    			: CONTEXT_LORA
//*----------------------------------------------------------------------------
void mc_rx_format(const MC_RX_EVENT *ev, char *buf, uint16_t buf_len)
{
	if((ev == NULL) || (buf == NULL) || (buf_len == 0))
		return;

	*buf = 0;

	switch(ev->kind)
	{
		case MC_RX_ADVERT:
			snprintf(buf, buf_len, "[%02X] %s(%s advert%s)",
					 ev->pub_key[0], ev->name, mc_rx_role_name(ev->role),
					 ev->sig_ok ? "" : ", BAD SIG");
			break;

		case MC_RX_CHANNEL:
			if(ev->sender[0])
				snprintf(buf, buf_len, "[%s] %s: %s", ev->channel_name, ev->sender, ev->text);
			else
				snprintf(buf, buf_len, "[%s] %s", ev->channel_name, ev->text);
			break;

		case MC_RX_DIRECT:
			snprintf(buf, buf_len, "[dm] %s: %s", ev->sender, ev->text);
			break;

		case MC_RX_ACK:
			snprintf(buf, buf_len, "ack");
			break;

		default:
			// A packet we cannot read. Say why, where we know
			if((ev->type == MESHCORE_PAYLOAD_TYPE_GRP_TXT) && (ev->channel_name[0]))
				snprintf(buf, buf_len, "%s, no key", ev->channel_name);
			break;
	}
}

#endif
