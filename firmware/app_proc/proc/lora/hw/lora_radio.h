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
#else
// Meshtastic UK
#define LORA_SF		SX126X_LORA_SPREADING_FACTOR_11
#define LORA_CR		SX126X_LORA_CODING_RATE_4_5
#define LORA_BW		SX126X_LORA_BANDWIDTH_250
#define LORA_FR		869.525f
//
#define LORA_PL		16
#endif

#define RX_TIMEOUT_MS		2000
#define TX_TIMEOUT_MS		2000

#define RX_TIMEOUT_F		(ulong)(((float)(RX_TIMEOUT_MS * 5)) * 1000.0f)/15.625f
#define TX_TIMEOUT_F		(ulong)(((float)(TX_TIMEOUT_MS * 5)) * 1000.0f)/15.625f

// --------------------------------------------------------------------------------

// Exports
uchar lora_radio_init(void);

void  lora_radio_rx_check(struct LORA_PACKET_RX *lp);
void lora_radio_schedule_tx(void);

#endif
