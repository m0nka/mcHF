/************************************************************************************
**                                                                                 **
**                                 mcHF QRP Transceiver                            **
**                         Krassi Atanassov - M0NKA, 2013-2026                     **
**                                                                                 **
**---------------------------------------------------------------------------------**
**                                                                                 **
**  File name:		ft8_qso.h                                                      **
**  Description:	FT8 QSO sequencer - the standard WSJT-X exchange               **
**  Last Modified:                                                                 **
**  Licence:		https://github.com/m0nka/mcHF/blob/main/LICENSE                **
************************************************************************************/
//
//	CQ		CQ M0NKA IO91			calling
//	Tx1		THEM M0NKA IO91			answering their CQ
//	Tx2		THEM M0NKA -12			their grid came in (they answered our CQ)
//	Tx3		THEM M0NKA R-12			their report came in
//	Tx4		THEM M0NKA RR73			their R+report came in - QSO complete
//	Tx5		THEM M0NKA 73			their RR73/RRR came in - sent once, done
//
// Runs in the ft8 task: after each decoded slot it looks for messages to
// us, advances the QSO and queues the next message before our next slot.
// No reply means the same message again, until the transmitter watchdog.
//
#ifndef __FT8_QSO_H
#define __FT8_QSO_H

#include <stdint.h>

#include "ft8_decoder.h"

// QSO states - named after what we sent last
#define FT8_QSO_IDLE				0
#define FT8_QSO_CQ					1		// calling CQ
#define FT8_QSO_CALLING				2		// sent Tx1, waiting for their report
#define FT8_QSO_REPORT				3		// sent Tx2, waiting for R+report
#define FT8_QSO_RREPORT				4		// sent Tx3, waiting for RR73/RRR
#define FT8_QSO_RR73				5		// sending RR73 (once)
#define FT8_QSO_SEND73				6		// sending 73 (once)
#define FT8_QSO_DONE				7		// logged

typedef struct
{
	uchar		state;
	char		call[12];				// the other station
	char		grid[8];				// theirs, when they sent it
	int8_t		rpt_sent;				// dB
	int8_t		rpt_rcvd;
	uchar		have_rcvd;

} FT8_QSO_STATUS;

// The callsign this radio operates as (see ft8_qso.c - bench stopgap)
const char	*ft8_my_call(void);

// Screen side (any task)
void	ft8_qso_cq(void);
// their_snr is the decode's raw ft8_lib figure (FT8_DECODE_RESULT.snr)
int		ft8_qso_answer(const char *cq_text, uchar their_parity, int16_t their_snr);
void	ft8_qso_stop(void);
void	ft8_qso_get(FT8_QSO_STATUS *st);

// Does a decoded message belong on the RX FREQUENCY pane (to or from us,
// or from the station we are working)
uchar	ft8_qso_is_ours(const char *msg);

// Report value for a decode, from ft8_lib's candidate score. That score is
// a sync strength, not dB - the mapping is a stand-in until there is a
// real noise floor based SNR
int8_t	ft8_qso_report(int16_t snr);

// ft8 task side
void	ft8_qso_slot(const FT8_DECODE_RESULT *res, int n, uchar slot_parity);
void	ft8_qso_tx_sent(void);

#endif
