/************************************************************************************
**                                                                                 **
**                                 mcHF QRP Transceiver                            **
**                         Krassi Atanassov - M0NKA, 2013-2026                     **
**                                                                                 **
**---------------------------------------------------------------------------------**
**                                                                                 **
**  File name:                                                                     **
**  Description:                                                                   **
**  Last Modified:                                                                 **
**  Licence:			https://github.com/m0nka/mcHF/blob/main/LICENSE            **
************************************************************************************/
#ifndef __LORA_RADIO_H
#define __LORA_RADIO_H

// Temp as local const
#ifdef MESHCORE
// Meshcore UK
#define LORA_SF		SX126X_LORA_SPREADING_FACTOR_8
#define LORA_CR		SX126X_LORA_CODING_RATE_4_8
#define LORA_BW		SX126X_LORA_BANDWIDTH_62
#define LORA_FR		869.618f
//
#define LORA_PL		8
//
// Same settings in a form that can be shown on screen. Kept next to the
// values themselves so the two cannot drift apart
#define LORA_CFG_TXT	"SF8 BW62 CR4:8"
#else
// Meshtastic UK
#define LORA_SF		SX126X_LORA_SPREADING_FACTOR_11
#define LORA_CR		SX126X_LORA_CODING_RATE_4_5
#define LORA_BW		SX126X_LORA_BANDWIDTH_250
#define LORA_FR		869.525f
//
#define LORA_PL		16
//
#define LORA_CFG_TXT	"SF11 BW250 CR4:5"
#endif

// Keep the receiver armed permanently rather than re-arming it after
// every packet.
//
// SetRx(0xFFFFFF) is continuous mode: the modem carries on listening
// once a packet is done, so the only time it is not receiving is while
// we transmit. The single shot mode this replaced was torn down and
// re-issued on every packet, every crc error and every timeout, and
// each re-arm restarts reception - a packet whose preamble had already
// started during that window was lost. A phone running the same mesh
// never leaves receive, which is the comparison that raised the
// question in the first place
//
// Comment out to go back to single shot, for comparison
#define LORA_RX_CONTINUOUS

#define RX_CONTINUOUS_F		0x00FFFFFF

#define RX_TIMEOUT_MS		2000
#define TX_TIMEOUT_MS		2000

#define RX_TIMEOUT_F		(ulong)(((float)(RX_TIMEOUT_MS * 5)) * 1000.0f)/15.625f
#define TX_TIMEOUT_F		(ulong)(((float)(TX_TIMEOUT_MS * 5)) * 1000.0f)/15.625f

// --------------------------------------------------------------------------------

// Exports
uchar lora_radio_init(void);

void  lora_radio_rx_check(struct LORA_PACKET_RX *lp);
void lora_radio_schedule_tx(void);

// What the radio layer has seen since boot. Counted here rather than
// further up because this is the only place that knows the difference
// between a packet that never arrived and one that arrived broken -
// which is the whole question when a mesh looks like it is dropping
// traffic
typedef struct
{
	uint32_t	rx_done;						// packets handed up
	uint32_t	crc_err;						// arrived, failed its crc
	uint32_t	hdr_err;						// arrived, bad header
	uint32_t	rx_timeout;						// receiver found idle
	uint32_t	rearm;							// times the receiver was re-armed
	uint32_t	tx_ok;
	uint32_t	tx_fail;

	// Longest the lora task went between two looks at the modem. Every
	// millisecond of it is time the radio was either deaf or holding a
	// finished packet, so it is the number to watch when packets go
	// missing while the screen is busy
	uint32_t	max_gap_ms;

} LORA_RX_STAT;

const LORA_RX_STAT *lora_radio_stats(void);
void  lora_radio_stats_gap(uint32_t gap_ms);

// Send one packet, then re-arm the receiver. Returns 0 on success
uchar lora_radio_transmit(const uchar *data, uchar len);

// "869.618 MHz  SF8 BW62 CR4:8" - what the modem is actually set to,
// for the chat screen's title bar. Formatted here rather than in the UI
// because the frequency is a float and the tiny printf has no %f
void  lora_radio_config_text(char *buf, ushort len);

#endif
