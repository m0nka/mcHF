/************************************************************************************
**                                                                                 **
**                                 mcHF QRP Transceiver                            **
**                         Krassi Atanassov - M0NKA, 2013-2026                     **
**                                                                                 **
**---------------------------------------------------------------------------------**
**                                                                                 **
**  File name:		meshchat_proc.c                                                **
**  Description:	MeshCore chat service - conversations, contacts, tx queue      **
**  Licence:		https://github.com/m0nka/mcHF/blob/main/LICENSE                **
************************************************************************************/
#include "main.h"
#include "mchf_pro_board.h"

#ifdef CONTEXT_MESHCHAT

#include <string.h>

#include "rtc.h"
#include "ff.h"

#include "mc_ec.h"
#include "mc_identity.h"
#include "mc_contacts.h"
#include "mc_rx.h"
#include "mc_tx.h"
#include "lora_radio.h"

#include "meshchat_proc.h"

// FreeRTOS process state
extern struct PROC_STATE		ps;

// UI driver public state
extern struct UI_DRIVER_STATE	ui_s;

// A received packet on its way from the radio task to here. The raw
// bytes are copied rather than referenced - the lora task reuses its
// buffer as soon as it has handed this over
typedef struct
{
	uint8_t		data[MESHCORE_MAX_TRANS_UNIT];
	uint16_t	size;
	int8_t		snr;

} MESHCHAT_RX_RAW;

// Work posted by the gui task
#define MESHCHAT_REQ_SEND		1
#define MESHCHAT_REQ_ADVERT		2
#define MESHCHAT_REQ_ADD		3
#define MESHCHAT_REQ_FORGET		4
#define MESHCHAT_REQ_ADD_CHAN	5
#define MESHCHAT_REQ_DEL_CHAN	6

typedef struct
{
	uint8_t			kind;
	uint8_t			arg;						// contact index, for add/forget
	MESHCHAT_CONV	conv;
	char			text[MESHCHAT_TEXT_MAX];

} MESHCHAT_REQ;

static xQueueHandle		mc_rx_q;
static xQueueHandle		mc_tx_q;
static xQueueHandle		mc_req_q;

static MESHCHAT_MSG		mc_msgs[MESHCHAT_MSG_MAX];
static uint8_t			mc_msg_head;			// next slot to write
static uint8_t			mc_msg_used;

static volatile uint32_t	mc_revision;
static volatile uint8_t		mc_started;
static volatile uint8_t		mc_state;			// MESHCHAT_STATE_xxx
static volatile uint8_t		mc_sd_waited;		// half seconds spent waiting for a card

// When the card was last looked for again, while running without one
static uint32_t				mc_store_poll_tick;

static uint8_t			mc_queue_tx(const MC_TX_PACKET *pkt);

// Scratch owned by this task - none of it is touched anywhere else
static MESHCHAT_RX_RAW	mc_raw;
static MC_RX_EVENT		mc_ev;
static MC_TX_PACKET		mc_pkt;

// ---------------------------------------------------------------------
// Hearing ourselves come back
//
// A flood packet is rebroadcast by every repeater in range, each adding
// itself to the path while the payload stays untouched. So when one of
// our own transmissions comes back we can recognise it by its payload
// fingerprint - and that is the only confirmation a transmitting station
// gets that its signal actually reached anything. It is what the phone
// apps show, and on a quiet mesh it is the difference between "nobody is
// talking" and "my antenna is disconnected"
//
// The same fingerprints are what lets us drop the duplicate copies of
// other people's messages that arrive via different repeaters

static MESHCHAT_ECHO	mc_echo[MESHCHAT_ECHO_MAX];
static uint8_t			mc_echo_head;
static uint8_t			mc_echo_last = 0xFF;	// most recent transmission

// Payloads we have already dealt with, so a message repeated by three
// repeaters is shown once
// Where packets go once the modem has handed them over. Counted so a
// mesh that looks lossy can be told apart from one we are throwing away
// ourselves - the two need completely different fixes
static MESHCHAT_STAT	mc_stat;

static uint32_t			mc_seen_fp[MESHCHAT_SEEN_MAX];
static uint8_t			mc_seen_head;

//*----------------------------------------------------------------------------
//* Function Name       : mc_echo_track
//* Object              : remember a payload we have just sent
//* Context    			: CONTEXT_MESHCHAT
//*----------------------------------------------------------------------------
static void mc_echo_track(uint32_t fp)
{
	MESHCHAT_ECHO	*e = &mc_echo[mc_echo_head];

	memset(e, 0, sizeof(MESHCHAT_ECHO));

	e->fp		= fp;
	e->in_use	= 1;
	e->min_hops	= 0xFF;
	e->tick		= (uint32_t)xTaskGetTickCount();

	mc_echo_last = mc_echo_head;
	mc_echo_head = (uint8_t)((mc_echo_head + 1) % MESHCHAT_ECHO_MAX);

	mc_revision++;
}

//*----------------------------------------------------------------------------
//* Function Name       : mc_echo_match
//* Object              : is this one of ours coming back ?
//* Notes    			: returns nonzero when it was, having counted it
//* Context    			: CONTEXT_MESHCHAT
//*----------------------------------------------------------------------------
static uint8_t mc_echo_match(const MC_RX_EVENT *ev)
{
	uint8_t	i;

	for(i = 0; i < MESHCHAT_ECHO_MAX; i++)
	{
		MESHCHAT_ECHO	*e = &mc_echo[i];

		if((!e->in_use) || (e->fp != ev->payload_fp))
			continue;

		e->repeats++;

		if(ev->path_len < e->min_hops)
			e->min_hops = ev->path_len;

		if((e->repeats == 1) || (ev->snr > e->best_snr))
			e->best_snr = ev->snr;

		mc_revision++;

		printf("meshchat: heard own message repeated, %d hop(s), snr %d, %d total \r\n",
				(int)ev->path_len, (int)ev->snr, (int)e->repeats);

		return 1;
	}

	return 0;
}

//*----------------------------------------------------------------------------
//* Function Name       : mc_seen_check
//* Object              : have we already handled this payload ?
//* Notes    			: records it either way
//* Context    			: CONTEXT_MESHCHAT
//*----------------------------------------------------------------------------
static uint8_t mc_seen_check(uint32_t fp)
{
	uint8_t	i;

	if(fp == 0)
		return 0;

	for(i = 0; i < MESHCHAT_SEEN_MAX; i++)
		if(mc_seen_fp[i] == fp)
			return 1;

	mc_seen_fp[mc_seen_head] = fp;
	mc_seen_head = (uint8_t)((mc_seen_head + 1) % MESHCHAT_SEEN_MAX);

	return 0;
}

uint8_t meshchat_last_echo(MESHCHAT_ECHO *out)
{
	if((out == NULL) || (mc_echo_last >= MESHCHAT_ECHO_MAX))
		return 0;

	if(!mc_echo[mc_echo_last].in_use)
		return 0;

	*out = mc_echo[mc_echo_last];

	return 1;
}

//*----------------------------------------------------------------------------
//* Function Name       : mc_now_epoch
//* Object              : UTC seconds since 1970, from the RTC
//* Notes    			: MeshCore stamps every message with one, and a
//*						: receiver that finds it wildly out of step may
//*						: discard the message - so this is only as good
//*						: as the clock. See the GPS calibration work for
//*						: how the RTC is disciplined on this radio
//* Context    			: CONTEXT_MESHCHAT
//*----------------------------------------------------------------------------
static uint32_t mc_now_epoch(void)
{
	RTC_TimeTypeDef	tm = {0};
	RTC_DateTypeDef	dt = {0};
	int32_t			y, m, era, yoe, doy, doe;
	int32_t			days;

	k_GetTime(&tm);
	k_GetDate(&dt);

	// Days from civil (Howard Hinnant's algorithm), shifted from the
	// 0000-03-01 era origin to the Unix one
	y = 2000 + (int32_t)dt.Year;
	m = (int32_t)dt.Month;

	y -= (m <= 2) ? 1 : 0;

	era = ((y >= 0) ? y : (y - 399)) / 400;
	yoe = y - era * 400;
	doy = (153 * (m + ((m > 2) ? -3 : 9)) + 2) / 5 + (int32_t)dt.Date - 1;
	doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;

	days = era * 146097 + doe - 719468;

	return (uint32_t)(days * 86400 + tm.Hours * 3600 + tm.Minutes * 60 + tm.Seconds);
}

uint32_t meshchat_revision(void)
{
	return mc_revision;
}

uint8_t meshchat_ready(void)
{
	return mc_started;
}

uint8_t meshchat_state(void)
{
	return mc_state;
}

// Seconds still to go on the startup card wait, 0 once it is over
uint8_t meshchat_sd_wait_left(void)
{
	if(mc_state != MESHCHAT_STATE_WAIT_SD)
		return 0;

	if(mc_sd_waited >= MESHCHAT_SD_WAIT_TRIES)
		return 0;

	return (uint8_t)(((MESHCHAT_SD_WAIT_TRIES - mc_sd_waited) * MESHCHAT_SD_WAIT_MS) / 1000);
}

uint8_t meshchat_tx_pending(void)
{
	if(mc_tx_q == NULL)
		return 0;

	return (uint8_t)uxQueueMessagesWaiting(mc_tx_q);
}

const char *meshchat_node_name(void)
{
	return mc_identity_get()->name;
}

uint8_t meshchat_node_hash(void)
{
	return mc_identity_hash();
}

// ---------------------------------------------------------------------
// Conversation helpers

static uint8_t mc_conv_same(const MESHCHAT_CONV *a, const MESHCHAT_CONV *b)
{
	if(a->kind != b->kind)
		return 0;

	if(a->kind == MESHCHAT_CONV_CHANNEL)
		return (a->chan_hash == b->chan_hash) ? 1 : 0;

	return (memcmp(a->peer, b->peer, sizeof(a->peer)) == 0) ? 1 : 0;
}

static void mc_conv_from_contact(MESHCHAT_CONV *out, const MC_CONTACT *c)
{
	memset(out, 0, sizeof(*out));

	out->kind = MESHCHAT_CONV_DIRECT;
	memcpy(out->peer, c->pub_key, sizeof(out->peer));
}

static void mc_conv_from_channel(MESHCHAT_CONV *out, uint8_t hash)
{
	memset(out, 0, sizeof(*out));

	out->kind		= MESHCHAT_CONV_CHANNEL;
	out->chan_hash	= hash;
}

//*----------------------------------------------------------------------------
//* Function Name       : meshchat_conv_count / _at
//* Object              : the conversation list the dialog shows - every
//*						: channel we hold a key for, then every saved
//*						: contact
//* Context    			: CONTEXT_VIDEO (gui task)
//*----------------------------------------------------------------------------
uint8_t meshchat_conv_count(void)
{
	uint8_t	i, n = mc_channels_count();

	for(i = 0; i < mc_contacts_count(); i++)
	{
		MC_CONTACT	*c = mc_contacts_at(i);

		if((c != NULL) && (c->saved))
			n++;
	}

	return n;
}

uint8_t meshchat_conv_at(uint8_t idx, MESHCHAT_CONV *out)
{
	uint8_t	nch = mc_channels_count();
	uint8_t	i, n;

	if(out == NULL)
		return 1;

	if(idx < nch)
	{
		MC_CHANNEL	*ch = mc_channels_at(idx);

		if(ch == NULL)
			return 1;

		mc_conv_from_channel(out, ch->hash);

		return 0;
	}

	n = nch;

	for(i = 0; i < mc_contacts_count(); i++)
	{
		MC_CONTACT	*c = mc_contacts_at(i);

		if((c == NULL) || (!c->saved))
			continue;

		if(n == idx)
		{
			mc_conv_from_contact(out, c);
			return 0;
		}

		n++;
	}

	return 1;
}

//*----------------------------------------------------------------------------
//* Function Name       : mc_conv_contact
//* Object              : the contact a direct conversation points at
//* Context    			: any
//*----------------------------------------------------------------------------
static MC_CONTACT *mc_conv_contact(const MESHCHAT_CONV *conv)
{
	uint8_t	i;

	if(conv->kind != MESHCHAT_CONV_DIRECT)
		return NULL;

	for(i = 0; i < mc_contacts_count(); i++)
	{
		MC_CONTACT	*c = mc_contacts_at(i);

		if((c != NULL) && (memcmp(c->pub_key, conv->peer, sizeof(conv->peer)) == 0))
			return c;
	}

	return NULL;
}

uint16_t meshchat_unread(const MESHCHAT_CONV *conv)
{
	if(conv == NULL)
		return 0;

	if(conv->kind == MESHCHAT_CONV_CHANNEL)
	{
		MC_CHANNEL	*ch = mc_channels_find_by_hash(conv->chan_hash);

		return (ch != NULL) ? ch->unread : 0;
	}

	{
		MC_CONTACT	*c = mc_conv_contact(conv);

		return (c != NULL) ? c->unread : 0;
	}
}

void meshchat_mark_read(const MESHCHAT_CONV *conv)
{
	if(conv == NULL)
		return;

	if(conv->kind == MESHCHAT_CONV_CHANNEL)
	{
		MC_CHANNEL	*ch = mc_channels_find_by_hash(conv->chan_hash);

		if((ch != NULL) && (ch->unread))
		{
			ch->unread = 0;
			mc_revision++;
		}

		return;
	}

	{
		MC_CONTACT	*c = mc_conv_contact(conv);

		if((c != NULL) && (c->unread))
		{
			c->unread = 0;
			mc_revision++;
		}
	}
}

//*----------------------------------------------------------------------------
//* Function Name       : meshchat_conv_label
//* Object              : one row of the conversation list, with the
//*						: activity count when the conversation has
//*						: anything unread
//* Context    			: CONTEXT_VIDEO (gui task)
//*----------------------------------------------------------------------------
void meshchat_conv_label(const MESHCHAT_CONV *conv, char *buf, uint16_t len)
{
	uint16_t	unread;
	char		tail[12];

	if((conv == NULL) || (buf == NULL) || (len == 0))
		return;

	*buf = 0;

	unread = meshchat_unread(conv);

	if(unread)
		snprintf(tail, sizeof(tail), "  (%u)", (unsigned int)unread);
	else
		tail[0] = 0;

	if(conv->kind == MESHCHAT_CONV_CHANNEL)
	{
		MC_CHANNEL	*ch = mc_channels_find_by_hash(conv->chan_hash);

		if(ch == NULL)
			snprintf(buf, len, "# %02X%s", conv->chan_hash, tail);
		else if(ch->name[0] == '#')
			snprintf(buf, len, "%s%s", ch->name, tail);		// already marked
		else
			snprintf(buf, len, "# %s%s", ch->name, tail);

		return;
	}

	{
		MC_CONTACT	*c = mc_conv_contact(conv);

		if(c != NULL)
			snprintf(buf, len, "@ %s%s", c->name, tail);
		else
			snprintf(buf, len, "@ %02X%02X%s", conv->peer[0], conv->peer[1], tail);
	}
}

// ---------------------------------------------------------------------
// History

//*----------------------------------------------------------------------------
//* Function Name       : mc_history_add
//* Object              : append to the shared ring, oldest falls off
//* Context    			: CONTEXT_MESHCHAT
//*----------------------------------------------------------------------------
static void mc_history_add(const MESHCHAT_CONV *conv, uint8_t dir,
						   const char *sender, const char *text, int8_t snr)
{
	RTC_TimeTypeDef	tm = {0};
	RTC_DateTypeDef	dt = {0};
	MESHCHAT_MSG	*m = &mc_msgs[mc_msg_head];

	k_GetTime(&tm);
	k_GetDate(&dt);

	memset(m, 0, sizeof(MESHCHAT_MSG));

	m->conv		= *conv;
	m->dir		= dir;
	m->snr		= snr;
	m->in_use	= 1;

	snprintf(m->time, sizeof(m->time), "%02d:%02d", tm.Hours, tm.Minutes);

	if(sender != NULL)
	{
		strncpy(m->sender, sender, MESHCHAT_SENDER_MAX);
		m->sender[MESHCHAT_SENDER_MAX] = 0;
	}

	if(text != NULL)
	{
		strncpy(m->text, text, MESHCHAT_TEXT_MAX - 1);
		m->text[MESHCHAT_TEXT_MAX - 1] = 0;
	}

	mc_msg_head = (uint8_t)((mc_msg_head + 1) % MESHCHAT_MSG_MAX);

	if(mc_msg_used < MESHCHAT_MSG_MAX)
		mc_msg_used++;

	mc_revision++;
}

// Walk the ring oldest first
static uint8_t mc_history_index(uint8_t n)
{
	uint8_t	start;

	if(mc_msg_used < MESHCHAT_MSG_MAX)
		start = 0;
	else
		start = mc_msg_head;

	return (uint8_t)((start + n) % MESHCHAT_MSG_MAX);
}

uint8_t meshchat_msg_count(const MESHCHAT_CONV *conv)
{
	uint8_t	i, n = 0;

	if(conv == NULL)
		return 0;

	for(i = 0; i < mc_msg_used; i++)
	{
		MESHCHAT_MSG	*m = &mc_msgs[mc_history_index(i)];

		if((m->in_use) && (mc_conv_same(&m->conv, conv)))
			n++;
	}

	return n;
}

const MESHCHAT_MSG *meshchat_msg_at(const MESHCHAT_CONV *conv, uint8_t idx)
{
	uint8_t	i, n = 0;

	if(conv == NULL)
		return NULL;

	for(i = 0; i < mc_msg_used; i++)
	{
		MESHCHAT_MSG	*m = &mc_msgs[mc_history_index(i)];

		if((!m->in_use) || (!mc_conv_same(&m->conv, conv)))
			continue;

		if(n == idx)
			return m;

		n++;
	}

	return NULL;
}

// ---------------------------------------------------------------------
// From the radio task

const MESHCHAT_STAT *meshchat_stats(void)
{
	return &mc_stat;
}

void meshchat_rx_packet(const uint8_t *data, uint16_t size, int8_t snr)
{
	MESHCHAT_RX_RAW	raw;

	if((mc_rx_q == NULL) || (data == NULL) || (size == 0))
		return;

	if(size > MESHCORE_MAX_TRANS_UNIT)
		return;

	memcpy(raw.data, data, size);

	raw.size	= size;
	raw.snr		= snr;

	// Drop rather than block - the radio task must get back to listening.
	// Counted, because this is the one loss the radio inflicts on itself:
	// the queue only fills when this task is busy, and an Ed25519 advert
	// verify is long enough to do it
	if(xQueueSend(mc_rx_q, &raw, 0) != pdPASS)
	{
		mc_stat.q_drop++;
		printf("meshchat: RX QUEUE FULL, packet dropped (%d so far) \r\n",
				(int)mc_stat.q_drop);
		return;
	}

	mc_stat.queued++;

	if(ps.hMeshchatTask != NULL)
		xTaskNotify(ps.hMeshchatTask, MESHCHAT_NOTIFY_WAKE, eSetBits);
}

uint8_t meshchat_tx_dequeue(MC_TX_PACKET *pkt)
{
	if((mc_tx_q == NULL) || (pkt == NULL))
		return 1;

	if(xQueueReceive(mc_tx_q, pkt, 0) != pdPASS)
		return 1;

	return 0;
}

// ---------------------------------------------------------------------
// From the gui task

static uint8_t mc_post_req(const MESHCHAT_REQ *req)
{
	if(mc_req_q == NULL)
		return 1;

	if(xQueueSend(mc_req_q, req, 0) != pdPASS)
		return 2;

	if(ps.hMeshchatTask != NULL)
		xTaskNotify(ps.hMeshchatTask, MESHCHAT_NOTIFY_WAKE, eSetBits);

	return 0;
}

uint8_t meshchat_send_text(const MESHCHAT_CONV *conv, const char *text)
{
	MESHCHAT_REQ	req;

	if((conv == NULL) || (text == NULL) || (text[0] == 0))
		return 1;

	memset(&req, 0, sizeof(req));

	req.kind = MESHCHAT_REQ_SEND;
	req.conv = *conv;

	strncpy(req.text, text, MESHCHAT_TEXT_MAX - 1);

	return mc_post_req(&req);
}

uint8_t meshchat_send_advert(void)
{
	MESHCHAT_REQ	req;

	memset(&req, 0, sizeof(req));
	req.kind = MESHCHAT_REQ_ADVERT;

	return mc_post_req(&req);
}

uint8_t meshchat_add_contact(uint8_t contact_idx)
{
	MESHCHAT_REQ	req;

	memset(&req, 0, sizeof(req));

	req.kind	= MESHCHAT_REQ_ADD;
	req.arg		= contact_idx;

	return mc_post_req(&req);
}

uint8_t meshchat_forget_contact(uint8_t contact_idx)
{
	MESHCHAT_REQ	req;

	memset(&req, 0, sizeof(req));

	req.kind	= MESHCHAT_REQ_FORGET;
	req.arg		= contact_idx;

	return mc_post_req(&req);
}

uint8_t meshchat_remove_channel(const MESHCHAT_CONV *conv)
{
	MESHCHAT_REQ	req;

	if((conv == NULL) || (conv->kind != MESHCHAT_CONV_CHANNEL))
		return 1;

	memset(&req, 0, sizeof(req));

	req.kind = MESHCHAT_REQ_DEL_CHAN;
	req.conv = *conv;

	return mc_post_req(&req);
}

uint8_t meshchat_add_channel(const char *name)
{
	MESHCHAT_REQ	req;

	if((name == NULL) || (name[0] == 0))
		return 1;

	memset(&req, 0, sizeof(req));

	req.kind = MESHCHAT_REQ_ADD_CHAN;

	// The name is hashed verbatim to make the key, and the mesh writes
	// these channels with the hash in front - so add it when the user
	// did not, or we would derive a key nobody else has
	if(name[0] != '#')
		snprintf(req.text, sizeof(req.text), "#%s", name);
	else
		strncpy(req.text, name, sizeof(req.text) - 1);

	return mc_post_req(&req);
}

// ---------------------------------------------------------------------
// Work

//*----------------------------------------------------------------------------
//* Function Name       : mc_notify_ui
//* Object              : push the one line summary to the spectrum
//*						: display, the way the lora task used to
//* Notes    			: the text lives in a static here, not on a stack
//*						: that is about to unwind
//* Context    			: CONTEXT_MESHCHAT
//*----------------------------------------------------------------------------
static void mc_notify_ui(const MC_RX_EVENT *ev)
{
	static char				notif[MC_RX_FORMAT_MIN];
	static LORA_PACKET_RX	lprx;
	ulong					ulData[10];
	ulong					ulDummy;

	if(ps.hUiTask == NULL)
		return;

	// The notification control only exists on the main desktop
	if(ui_s.cur_state != MODE_DESKTOP)
		return;

	mc_rx_format(ev, notif, sizeof(notif));

	if(notif[0] == 0)
		return;

	memset(&lprx, 0, sizeof(lprx));

	lprx.avail		= 1;
	lprx.mesh_id	= MESH_ID_MC;

	strncpy(lprx.msg_type, ev->type_short, sizeof(lprx.msg_type) - 1);
	snprintf(lprx.sig_snr, sizeof(lprx.sig_snr), "%d", (int)ev->snr);

	ulData[0] = 0x55;
	ulData[1] = (ulong)notif;
	ulData[2] = (ulong)&lprx;

	// Same contract as the old lora path - clear anything stale, then
	// post the three words the UI expects
	while(uxQueueMessagesWaiting(ps.xUiNotifRxQueue))
		xQueueReceive(ps.xUiNotifRxQueue, (void *)&ulDummy, (portTickType)0);

	xQueueSend(ps.xUiNotifRxQueue, (void *)&ulData[0], (portTickType)0);
	xQueueSend(ps.xUiNotifRxQueue, (void *)&ulData[1], (portTickType)0);
	xQueueSend(ps.xUiNotifRxQueue, (void *)&ulData[2], (portTickType)0);

	xTaskNotify(ps.hUiTask, UI_LORA_NOTIFICATION, eSetValueWithOverwrite);
}

#ifdef MESHCHAT_DEBUG_DM_KEY
//*----------------------------------------------------------------------------
//* Function Name       : mc_dump_hex
//* Object              : hex for the debug UART, in chunks - the tiny
//*						: printf is not happy with very long lines
//* Context    			: CONTEXT_MESHCHAT
//*----------------------------------------------------------------------------
static void mc_dump_hex(const char *label, const uint8_t *p, uint16_t len)
{
	uint16_t	i;

	printf("meshchat dm-debug: %s ", label);

	for(i = 0; i < len; i++)
	{
		printf("%02X", p[i]);

		if(((i + 1) % 32) == 0)
			printf(" \r\n                   ");
	}

	printf(" \r\n");
}

//*----------------------------------------------------------------------------
//* Function Name       : mc_dump_dm_attempt
//* Object              : everything needed to solve the direct message
//*						: key schedule offline against a real packet
//* Context    			: CONTEXT_MESHCHAT
//*----------------------------------------------------------------------------
static void mc_dump_dm_attempt(void)
{
	const MC_IDENTITY	*id = mc_identity_get();
	MC_CONTACT			*c  = mc_contacts_find_by_hash(mc_ev.src_hash);
	uint8_t				secret[MC_EC_KEY_SIZE];

	mc_dump_hex("pkt     ", mc_raw.data, mc_raw.size);
	mc_dump_hex("our pub ", id->pub, MC_EC_KEY_SIZE);

	if(c == NULL)
	{
		printf("meshchat dm-debug: sender %02X is not in the contact table \r\n",
				mc_ev.src_hash);
		return;
	}

	mc_dump_hex("their pub", c->pub_key, MC_EC_KEY_SIZE);

	// The full 32 byte X25519 output - the contact only caches the 16
	// bytes we currently use as the key, and the answer may well be a
	// hash of the whole thing
	if(mc_ec_shared_secret(secret, id->seed, c->pub_key) == 0)
		mc_dump_hex("shared  ", secret, MC_EC_KEY_SIZE);

	memset(secret, 0, sizeof(secret));
}
#endif

//*----------------------------------------------------------------------------
//* Function Name       : mc_handle_rx
//* Object              : decode one packet and file it
//* Context    			: CONTEXT_MESHCHAT
//*----------------------------------------------------------------------------
static void mc_handle_rx(void)
{
	MESHCHAT_CONV	conv;

	mc_rx_decode(mc_raw.data, mc_raw.size, mc_raw.snr, &mc_ev);

	// Understood at all ? A packet on a channel we do not hold the key
	// for, or a direct message for somebody else, is not a loss - it is
	// simply not ours, and lumping the two together is what makes a
	// mesh look lossier than it is
	if((mc_ev.kind == MC_RX_ADVERT) || (mc_ev.kind == MC_RX_CHANNEL) ||
	   (mc_ev.kind == MC_RX_DIRECT))
		mc_stat.decoded++;
	else
		mc_stat.unreadable++;

	#ifdef MESHCHAT_DEBUG_RX
	printf("meshchat: rx %d bytes, %s, snr %d, kind %d \r\n",
			(int)mc_raw.size, mc_ev.type_short, (int)mc_ev.snr, (int)mc_ev.kind);

	// A direct message sent to us that would not open. Called out
	// loudly because it is the one thing that separates "nobody has
	// messaged this radio" from "the direct message key schedule is
	// wrong" - the one part of the protocol never checked against
	// another node
	#ifdef MESHCHAT_DEBUG_DM_KEY
	// Every ACK on the air, with the value it carries. Paired with the
	// dump of what we sent, this is what identifies the quantity an ACK
	// is computed over - the last unknown in the direct message flow
	if(mc_ev.kind == MC_RX_ACK)
	{
		printf("meshchat dm-debug: ACK crc %08X \r\n", (unsigned int)mc_ev.ack_crc);
		mc_dump_hex("ack pkt ", mc_raw.data, mc_raw.size);
	}

	// PATH is what a MeshCore node actually answers a flooded direct
	// message with - it returns the route AND acknowledges, which is
	// why the phone keeps retrying at us. Its payload is encrypted with
	// the same pairwise key, so dumping the bytes next to the message
	// we sent is enough to work the format out
	if(mc_ev.type == MESHCORE_PAYLOAD_TYPE_PATH)
		mc_dump_hex("path pkt", mc_raw.data, mc_raw.size);
	#endif

	if((mc_ev.addressed_to_us) && (mc_ev.kind != MC_RX_DIRECT))
	{
		printf("meshchat:   DM addressed to us from %02X, could NOT decrypt "
			   "(sender not an added contact, or wrong key schedule) \r\n",
			   mc_ev.src_hash);

		#ifdef MESHCHAT_DEBUG_DM_KEY
		mc_dump_dm_attempt();
		#endif
	}
	#endif

	// An acknowledgement for something we sent - mark it delivered
	if(mc_ev.has_path_ack)
	{
		uint8_t	i;

		for(i = 0; i < mc_msg_used; i++)
		{
			MESHCHAT_MSG	*m = &mc_msgs[mc_history_index(i)];

			if((!m->in_use) || (!m->ack_wait))
				continue;

			if(memcmp(m->ack, mc_ev.path_ack, 4) != 0)
				continue;

			m->ack_wait	 = 0;
			m->delivered = 1;

			mc_revision++;

			printf("meshchat:   delivered: '%s' acked by %s \r\n", m->text, mc_ev.sender);
			break;
		}
	}

	// One of ours coming back off a repeater. Counted as proof the
	// signal got out, and deliberately not filed as an incoming message
	// - it is the message we already showed as sent
	if(mc_echo_match(&mc_ev))
	{
		mc_stat.echo++;
		return;
	}

	// A copy of something already handled, arriving by another route.
	// Adverts are exempt: a repeated advert is still a live sighting of
	// that node and should refresh its entry
	if((mc_ev.kind != MC_RX_ADVERT) && (mc_seen_check(mc_ev.payload_fp)))
	{
		// A repeat of a direct message still has to be answered. The
		// sender is repeating precisely because it has not heard our
		// acknowledgement - if the first one was lost, staying silent
		// on every retry means it never gets through
		if((mc_ev.kind == MC_RX_DIRECT) && (mc_ev.needs_ack))
		{
			MC_CONTACT	*c = mc_contacts_find(mc_ev.pub_key);

			if((c != NULL) && (mc_tx_build_path_ack(&mc_pkt, c, mc_ev.ack_reply) == 0))
			{
				mc_queue_tx(&mc_pkt);
				printf("meshchat:   repeat of a dm, ack re-sent \r\n");
			}

			mc_stat.dup++;

			return;
		}

		mc_stat.dup++;

		#ifdef MESHCHAT_DEBUG_RX
		printf("meshchat:   duplicate, ignored \r\n");
		#endif

		return;
	}

	switch(mc_ev.kind)
	{
		case MC_RX_ADVERT:
		{
			MC_CONTACT	*c;
			uint8_t		created = 0;

			// An advert that does not verify is either corruption or
			// someone claiming a key that is not theirs. Either way it
			// must not reach the contact book
			if(!mc_ev.sig_ok)
			{
				printf("meshchat: advert with bad signature from %02X, dropped \r\n",
						mc_ev.pub_key[0]);
				break;
			}

			c = mc_contacts_observe(mc_ev.pub_key, mc_ev.name, mc_ev.role,
									mc_ev.timestamp, mc_ev.snr,
									mc_ev.path, mc_ev.path_len, &created);

			#ifdef MESHCHAT_DEBUG_RX
			printf("meshchat:   advert %02X '%s' sig ok%s \r\n",
					mc_ev.pub_key[0], mc_ev.name, (c == NULL) ? ", table full" : "");
			#endif

			if(c != NULL)
				mc_revision++;

			// Only a node we had never heard is worth a card write. A
			// refresh of one we already know changes nothing that has
			// to survive a reboot
			if(created)
			{
				mc_contacts_save();
				printf("meshchat: new node %02X '%s' stored \r\n",
						mc_ev.pub_key[0], mc_ev.name);
			}

			break;
		}

		case MC_RX_CHANNEL:
		{
			MC_CHANNEL	*ch = mc_channels_find_by_hash(mc_ev.channel_hash);

			mc_conv_from_channel(&conv, mc_ev.channel_hash);

			#ifdef MESHCHAT_DEBUG_RX
			printf("meshchat:   [%s] %s: %s \r\n",
					mc_ev.channel_name, mc_ev.sender, mc_ev.text);
			#endif

			mc_history_add(&conv, MESHCHAT_DIR_RX,
						   mc_ev.sender[0] ? mc_ev.sender : "?",
						   mc_ev.text, mc_ev.snr);

			// Activity badge against the channel row. The dialog clears
			// it for whichever conversation is actually on screen
			if(ch != NULL)
				ch->unread++;

			break;
		}

		case MC_RX_DIRECT:
		{
			MC_CONTACT	*c = mc_contacts_find(mc_ev.pub_key);

			if(c == NULL)
				break;

			mc_conv_from_contact(&conv, c);

			#ifdef MESHCHAT_DEBUG_RX
			printf("meshchat:   dm from %s: %s \r\n", c->name, mc_ev.text);
			#endif

			mc_history_add(&conv, MESHCHAT_DIR_RX, c->name, mc_ev.text, mc_ev.snr);

			c->unread++;

			// Answer it, or the sender keeps retransmitting. MeshCore
			// acknowledges a flooded direct message with a PATH packet
			// that carries the ack nested inside
			if(mc_ev.needs_ack)
			{
				if(mc_tx_build_path_ack(&mc_pkt, c, mc_ev.ack_reply) == 0)
				{
					mc_queue_tx(&mc_pkt);

					printf("meshchat:   ack %02X%02X%02X%02X sent to %s \r\n",
							mc_ev.ack_reply[0], mc_ev.ack_reply[1],
							mc_ev.ack_reply[2], mc_ev.ack_reply[3], c->name);
				}
			}

			break;
		}

		default:
			break;
	}

	mc_notify_ui(&mc_ev);
}

//*----------------------------------------------------------------------------
//* Function Name       : mc_queue_tx
//* Object              : hand a built packet to the radio task
//* Context    			: CONTEXT_MESHCHAT
//*----------------------------------------------------------------------------
static uint8_t mc_queue_tx(const MC_TX_PACKET *pkt)
{
	meshcore_message_t	msg;

	if(mc_tx_q == NULL)
		return 1;

	if(xQueueSend(mc_tx_q, pkt, 0) != pdPASS)
		return 2;

	// Fingerprint what we are about to put on the air, so the repeats
	// can be recognised when they come back. Taken from the payload
	// after deserialising rather than from the raw packet - a repeater
	// grows the path, which would change any hash over the whole frame
	if(meshcore_deserialize((uint8_t *)pkt->data, pkt->len, &msg) >= 0)
		mc_echo_track(mc_rx_fingerprint(msg.payload, msg.payload_length));

	mc_revision++;

	return 0;
}

//*----------------------------------------------------------------------------
//* Function Name       : mc_handle_send
//* Object              : build and queue an outgoing chat message
//* Context    			: CONTEXT_MESHCHAT
//*----------------------------------------------------------------------------
static void mc_handle_send(const MESHCHAT_REQ *req)
{
	uint32_t	ts = mc_now_epoch();
	uint8_t		expect_ack[4];
	uint8_t		have_expect = 0;
	uint8_t		err;

	if(req->conv.kind == MESHCHAT_CONV_CHANNEL)
	{
		MC_CHANNEL	*ch = mc_channels_find_by_hash(req->conv.chan_hash);

		if(ch == NULL)
		{
			mc_history_add(&req->conv, MESHCHAT_DIR_INFO, NULL, "no key for channel", 0);
			return;
		}

		err = mc_tx_build_group_text(&mc_pkt, ch, mc_identity_get()->name, req->text, ts);

		if(err)
		{
			printf("meshchat: group build err %d \r\n", err);
			mc_history_add(&req->conv, MESHCHAT_DIR_INFO, NULL, "send failed", 0);
			return;
		}
	}
	else
	{
		MC_CONTACT	*c = mc_conv_contact(&req->conv);

		if(c == NULL)
		{
			mc_history_add(&req->conv, MESHCHAT_DIR_INFO, NULL, "contact gone", 0);
			return;
		}

		// Deriving costs a scalar multiplication, so it is done here on
		// the slow task and cached for the life of the contact
		if(mc_contacts_derive_shared(c))
		{
			mc_history_add(&req->conv, MESHCHAT_DIR_INFO, NULL, "no key for contact", 0);
			return;
		}

		err = mc_tx_build_direct_text(&mc_pkt, c, req->text, ts, expect_ack);

		if(!err)
			have_expect = 1;

		#ifdef MESHCHAT_DEBUG_DM_KEY
		// What we put on the air, so the ACK that comes back can be
		// matched against a message whose bytes we know exactly
		if(!err)
			mc_dump_hex("sent dm ", mc_pkt.data, mc_pkt.len);
		#endif

		if(err)
		{
			printf("meshchat: direct build err %d \r\n", err);
			mc_history_add(&req->conv, MESHCHAT_DIR_INFO, NULL, "send failed", 0);
			return;
		}
	}

	if(mc_queue_tx(&mc_pkt))
	{
		mc_history_add(&req->conv, MESHCHAT_DIR_INFO, NULL, "tx queue full", 0);
		return;
	}

	mc_history_add(&req->conv, MESHCHAT_DIR_TX, mc_identity_get()->name, req->text, 0);

	// Remember what will acknowledge it, so the reply can be matched
	// back to this line and shown as delivered
	if(have_expect)
	{
		MESHCHAT_MSG	*m = &mc_msgs[(mc_msg_head + MESHCHAT_MSG_MAX - 1) % MESHCHAT_MSG_MAX];

		memcpy(m->ack, expect_ack, sizeof(m->ack));
		m->ack_wait = 1;
	}
}

//*----------------------------------------------------------------------------
//* Function Name       : mc_handle_req
//* Object              : run one request from the gui task
//* Context    			: CONTEXT_MESHCHAT
//*----------------------------------------------------------------------------
static void mc_handle_req(const MESHCHAT_REQ *req)
{
	switch(req->kind)
	{
		case MESHCHAT_REQ_SEND:
			mc_handle_send(req);
			break;

		case MESHCHAT_REQ_ADVERT:
		{
			uint8_t	err = mc_tx_build_advert(&mc_pkt, mc_now_epoch());

			if(err)
			{
				printf("meshchat: advert build err %d \r\n", err);
				break;
			}

			if(mc_queue_tx(&mc_pkt) == 0)
				printf("meshchat: advert queued, %d bytes \r\n", (int)mc_pkt.len);

			break;
		}

		case MESHCHAT_REQ_ADD:
		{
			MC_CONTACT	*c = mc_contacts_at(req->arg);

			if(c == NULL)
				break;

			printf("meshchat: add contact %s \r\n", c->name);

			mc_contacts_save_entry(req->arg);
			mc_revision++;
			break;
		}

		case MESHCHAT_REQ_FORGET:
		{
			mc_contacts_forget(req->arg);
			mc_revision++;
			break;
		}

		case MESHCHAT_REQ_DEL_CHAN:
		{
			MC_CHANNEL	*ch = mc_channels_find_by_hash(req->conv.chan_hash);
			char		was[MC_CHANNEL_NAME_MAX + 1];
			uint8_t		err;

			if(ch == NULL)
				break;

			// The name has to be taken before the entry is cleared
			strncpy(was, ch->name, MC_CHANNEL_NAME_MAX);
			was[MC_CHANNEL_NAME_MAX] = 0;

			err = mc_channels_remove_by_hash(req->conv.chan_hash);

			// Refused rather than failed. Said in the conversation
			// itself, not just the log - the screen is where the user
			// pressed the button
			if(err == MC_CHANNEL_PROTECTED)
			{
				printf("meshchat: channel '%s' is the default, not removed \r\n", was);

				mc_history_add(&req->conv, MESHCHAT_DIR_INFO, NULL,
							   "the default channel cannot be deleted", 0);

				mc_revision++;
				break;
			}

			printf("meshchat: channel '%s' removed \r\n", was);

			mc_revision++;
			break;
		}

		case MESHCHAT_REQ_ADD_CHAN:
		{
			uint8_t	err = mc_channels_add_by_name(req->text);

			if(err)
			{
				printf("meshchat: add channel '%s' failed (%d) \r\n", req->text, (int)err);
				break;
			}

			mc_channels_save();
			mc_revision++;

			{
				MC_CHANNEL	*ch = mc_channels_at(mc_channels_count() - 1);

				printf("meshchat: channel '%s' added, hash 0x%02X \r\n",
						req->text, (ch != NULL) ? ch->hash : 0);
			}
			break;
		}

		default:
			break;
	}
}

//*----------------------------------------------------------------------------
//* Function Name       : meshchat_proc_task
//* Object              :
//* Notes    			: notification driven - the radio task wakes us
//*						: on receive, the gui task on a request
//* Context    			: CONTEXT_MESHCHAT
//*----------------------------------------------------------------------------
void meshchat_proc_task(void const *arg)
{
	ulong			ulNotificationValue = 0;
	MESHCHAT_REQ	req;

	vTaskDelay(MESHCHAT_PROC_START_DELAY);

	mc_rx_q		= xQueueCreate(MESHCHAT_RX_QUEUE_LEN,  sizeof(MESHCHAT_RX_RAW));
	mc_tx_q		= xQueueCreate(MESHCHAT_TX_QUEUE_LEN,  sizeof(MC_TX_PACKET));
	mc_req_q	= xQueueCreate(MESHCHAT_REQ_QUEUE_LEN, sizeof(MESHCHAT_REQ));

	if((mc_rx_q == NULL) || (mc_tx_q == NULL) || (mc_req_q == NULL))
	{
		printf("meshchat: queue alloc failed \r\n");
		vTaskSuspend(NULL);
	}

	// Wait for the card before touching the identity. The storage task
	// starts long before us, but a slow card can still be mounting at
	// our start delay - and an identity created while the filesystem is
	// down cannot be saved, so the radio would come up as a different
	// node to everyone who has added it. Seen on the bench: "card init
	// failed" then "new identity ... key save open err(12)"
	//
	// The wait is visible to the dialog rather than silent, so a radio
	// with no card in it does not look like it has hung for ten seconds
	{
		FATFS	*fs;
		DWORD	clusters;

		mc_state = MESHCHAT_STATE_WAIT_SD;
		mc_revision++;

		for(mc_sd_waited = 0; mc_sd_waited < MESHCHAT_SD_WAIT_TRIES; mc_sd_waited++)
		{
			if(f_getfree("0://", &clusters, &fs) == FR_OK)
				break;

			vTaskDelay(MESHCHAT_SD_WAIT_MS);

			mc_revision++;						// so the countdown redraws
		}

		if(mc_sd_waited >= MESHCHAT_SD_WAIT_TRIES)
			printf("meshchat: no filesystem - identity will not persist this boot \r\n");
	}

	// The identity has to exist before anything can be signed or any
	// contact key derived, so this is done first and once
	mc_identity_init();
	mc_contacts_init();

	// Derive the message key for every saved contact now. The cached
	// secret is not persisted (it depends on our identity, which the
	// key file could have replaced), so without this an incoming direct
	// message from a contact we have not sent to since boot would find
	// have_shared clear and be dropped as unreadable. One scalar
	// multiplication each, on this task, before we start listening
	{
		uint8_t	i, n = 0;

		for(i = 0; i < mc_contacts_count(); i++)
		{
			MC_CONTACT	*c = mc_contacts_at(i);

			if((c == NULL) || (!c->saved))
				continue;

			if(mc_contacts_derive_shared(c) == 0)
				n++;
		}

		if(n)
			printf("meshchat: %d contact key(s) derived \r\n", (int)n);
	}

	mc_started	= 1;
	mc_state	= MESHCHAT_STATE_READY;
	mc_revision++;

meshchat_proc_loop:

	// Normally notification driven, but while there is no card we have
	// to come round on our own to look for one
	// Normally a packet or a request wakes us. The cap is what makes the
	// statistics below come round on their own on a quiet mesh - a
	// report that only prints when traffic arrives cannot show that no
	// traffic arrived, which is exactly the case being investigated
	xTaskNotifyWait(0x00, ULONG_MAX, &ulNotificationValue,
					mc_store_is_writable() ? MESHCHAT_STAT_PERIOD_MS
										   : MESHCHAT_STORE_POLL_MS);

	// Received packets first - decoding is what feeds the rest
	while((mc_rx_q != NULL) && (xQueueReceive(mc_rx_q, &mc_raw, 0) == pdPASS))
		mc_handle_rx();

	while((mc_req_q != NULL) && (xQueueReceive(mc_req_q, &req, 0) == pdPASS))
		mc_handle_req(&req);

	// Running without a card ? Look again every few seconds, so putting
	// one in is enough - no restart, and no leaving the screen
	if(!mc_store_is_writable())
	{
		uint32_t	now = (uint32_t)xTaskGetTickCount();

		if((now - mc_store_poll_tick) >= MESHCHAT_STORE_POLL_MS)
		{
			mc_store_poll_tick = now;

			if(mc_store_recheck())
			{
				// Card is in use now - reconcile the identity with it
				// and re-derive the message keys, which depend on it
				uint8_t	i;

				mc_identity_recheck_card();

				for(i = 0; i < mc_contacts_count(); i++)
				{
					MC_CONTACT	*c = mc_contacts_at(i);

					if((c != NULL) && (c->saved))
					{
						c->have_shared = 0;
						mc_contacts_derive_shared(c);
					}
				}

				mc_revision++;
			}
		}
	}

	// ------------------------------------------------------------
	// Periodic account of where the traffic went. Printed from this
	// task rather than the radio one so both halves can be shown
	// together - the modem's view and ours - which is what makes the
	// difference between the mesh losing packets and this radio
	// losing them visible at a glance
	// ------------------------------------------------------------
	{
		static uint32_t		stat_tick = 0;
		uint32_t			now = (uint32_t)xTaskGetTickCount();

		if((stat_tick == 0) || ((now - stat_tick) >= MESHCHAT_STAT_PERIOD_MS))
		{
			const LORA_RX_STAT *r = lora_radio_stats();

			stat_tick = now;

			printf("meshchat rx stats: modem done %d, crc err %d, hdr err %d, "
				   "timeout %d, rearm %d, max poll gap %d ms \r\n",
					(int)r->rx_done, (int)r->crc_err, (int)r->hdr_err,
					(int)r->rx_timeout, (int)r->rearm, (int)r->max_gap_ms);

			printf("meshchat rx stats: queued %d, QUEUE DROPS %d, decoded %d, "
				   "not ours %d, dup %d, echo %d; tx ok %d, tx fail %d \r\n",
					(int)mc_stat.queued, (int)mc_stat.q_drop, (int)mc_stat.decoded,
					(int)mc_stat.unreadable, (int)mc_stat.dup, (int)mc_stat.echo,
					(int)r->tx_ok, (int)r->tx_fail);
		}
	}

	goto meshchat_proc_loop;
}

#endif
