/************************************************************************************
**                                                                                 **
**                                 mcHF QRP Transceiver                            **
**                         Krassi Atanassov - M0NKA, 2013-2026                     **
**                                                                                 **
**---------------------------------------------------------------------------------**
**                                                                                 **
**  File name:		ft8_qso.c                                                      **
**  Description:	FT8 QSO sequencer - the standard WSJT-X exchange               **
**  Last Modified:                                                                 **
**  Licence:		https://github.com/m0nka/mcHF/blob/main/LICENSE                **
************************************************************************************/
#include "mchf_pro_board.h"
#include "main.h"

#ifdef CONTEXT_FT8

#include <stdlib.h>
#include <string.h>

#include "ff.h"
#include "rtc.h"

#include "ft8_proc.h"
#include "ft8_qso.h"

// Public radio state - dial frequency for the log
extern struct TRANSCEIVER_STATE_UI	tsu;

// Unit serial number, read from the battery pack at boot
extern struct BMSState				bmss;

#define FT8_QSO_LOG					"0://ft8/qso.txt"

static FT8_QSO_STATUS	qso;
static uchar			qso_parity;			// our tx parity in this QSO
static uchar			qso_logged;

// What the last word of a message is
#define FT8_X_NONE					0		// nothing (e.g. "CQ CALL")
#define FT8_X_GRID					1
#define FT8_X_REPORT				2		// -12, +05
#define FT8_X_RREPORT				3		// R-12
#define FT8_X_RRR					4		// RRR or RR73
#define FT8_X_73					5

//*----------------------------------------------------------------------------
//* Function Name       : ft8_my_call
//* Object              : the callsign this radio operates as
//* Notes    			: BENCH STOPGAP - both prototypes run one build and
//*						: need two callsigns to work each other, so SN 0002
//*						: is MM0NKA (a standard shape call - "M0NKA1" would
//*						: go out hashed as <...> and never match). Goes when
//*						: the callsign has a settings home
//* Context    			: any
//*----------------------------------------------------------------------------
const char *ft8_my_call(void)
{
	return (bmss.sn == 0x0002) ? "MM0NKA" : FT8_MY_CALL;
}

//*----------------------------------------------------------------------------
//* Function Name       : ft8_qso_words
//* Object              : split a message into up to four words
//* Context    			: any
//*----------------------------------------------------------------------------
static int ft8_qso_words(const char *msg, char w[4][14])
{
	int	n = 0, k;

	memset(w, 0, 4 * 14);

	while(*msg && (n < 4))
	{
		while(*msg == ' ')
			msg++;

		if(*msg == 0)
			break;

		for(k = 0; *msg && (*msg != ' '); msg++)
		{
			if(k < 13)
				w[n][k++] = *msg;
		}
		n++;
	}

	return n;
}

//*----------------------------------------------------------------------------
//* Function Name       : ft8_qso_extra
//* Object              : classify the last word, report value in *val
//* Notes    			: RR73 matches the grid pattern too - checked first
//* Context    			: any
//*----------------------------------------------------------------------------
static int ft8_qso_extra(const char *x, int *val)
{
	if(x[0] == 0)
		return FT8_X_NONE;

	if((strcmp(x, "RR73") == 0) || (strcmp(x, "RRR") == 0))
		return FT8_X_RRR;

	if(strcmp(x, "73") == 0)
		return FT8_X_73;

	if(((x[0] == '-') || (x[0] == '+')) && (x[1] >= '0') && (x[1] <= '9'))
	{
		*val = atoi(x);
		return FT8_X_REPORT;
	}

	if((x[0] == 'R') && ((x[1] == '-') || (x[1] == '+')))
	{
		*val = atoi(x + 1);
		return FT8_X_RREPORT;
	}

	if((strlen(x) == 4) && (x[0] >= 'A') && (x[0] <= 'R') && (x[1] >= 'A') && (x[1] <= 'R') &&
	   (x[2] >= '0') && (x[2] <= '9') && (x[3] >= '0') && (x[3] <= '9'))
		return FT8_X_GRID;

	return FT8_X_NONE;
}

//*----------------------------------------------------------------------------
//* Function Name       : ft8_qso_report
//*----------------------------------------------------------------------------
int8_t ft8_qso_report(int16_t snr)
{
	int	r = (int)snr - 26;

	if(r < -24)
		r = -24;
	else if(r > 20)
		r = 20;

	return (int8_t)r;
}

//*----------------------------------------------------------------------------
//* Function Name       : ft8_qso_rpt_text
//* Object              : "-07" / "+05", with an optional leading R
//*----------------------------------------------------------------------------
static void ft8_qso_rpt_text(char *out, int8_t rpt, uchar r)
{
	int	a = (rpt < 0) ? -rpt : rpt;

	snprintf(out, 6, "%s%c%02d", r ? "R" : "", (rpt < 0) ? '-' : '+', a);
}

//*----------------------------------------------------------------------------
//* Function Name       : ft8_qso_send
//* Object              : queue "THEM US <extra>" in our parity
//*----------------------------------------------------------------------------
static void ft8_qso_send(const char *extra)
{
	char	msg[FT8_MSG_LEN];

	snprintf(msg, sizeof(msg), "%s %s %s", qso.call, ft8_my_call(), extra);
	ft8_proc_tx_set(msg, qso_parity);
}

//*----------------------------------------------------------------------------
//* Function Name       : ft8_qso_log
//* Object              : completed QSO to the SD card (ADIF is WP8)
//* Context    			: CONTEXT_FT8
//*----------------------------------------------------------------------------
static void ft8_qso_log(void)
{
	RTC_TimeTypeDef	tm = {0};
	RTC_DateTypeDef	dt = {0};
	FIL				file;
	UINT			bw;
	char			line[96], rs[6], rr[6];
	ulong			dial = (tsu.band[tsu.curr_band].active_vfo == VFO_A) ?
						   tsu.band[tsu.curr_band].vfo_a : tsu.band[tsu.curr_band].vfo_b;

	if(qso_logged)
		return;

	qso_logged = 1;

	k_GetTime(&tm);
	k_GetDate(&dt);

	ft8_qso_rpt_text(rs, qso.rpt_sent, 0);
	ft8_qso_rpt_text(rr, qso.rpt_rcvd, 0);

	snprintf(line, sizeof(line), "20%02d-%02d-%02d %02d%02d %s %s %u FT8 sent %s rcvd %s\r\n",
			dt.Year, dt.Month, dt.Date, tm.Hours, tm.Minutes,
			qso.call, qso.grid[0] ? qso.grid : "----", (uint)dial, rs, qso.have_rcvd ? rr : "?");

	printf("ft8 qso: logged %s", line);

	if(f_open(&file, FT8_QSO_LOG, FA_WRITE | FA_OPEN_APPEND) == FR_OK)
	{
		f_write(&file, line, strlen(line), &bw);
		f_close(&file);
	}
}

//*----------------------------------------------------------------------------
//* Function Name       : ft8_qso_cq / answer / stop / get
//* Object              : screen side
//* Context    			: any task
//*----------------------------------------------------------------------------
void ft8_qso_cq(void)
{
	vTaskSuspendAll();
	memset(&qso, 0, sizeof(qso));
	qso.state  = FT8_QSO_CQ;
	qso_parity = 0;						// even slots until there is a choice
	qso_logged = 0;
	xTaskResumeAll();

	{
		char	msg[FT8_MSG_LEN];

		snprintf(msg, sizeof(msg), "CQ %s %s", ft8_my_call(), FT8_MY_GRID);
		ft8_proc_tx_set(msg, qso_parity);
	}
}

int ft8_qso_answer(const char *cq_text, uchar their_parity, int16_t their_snr)
{
	char	w[4][14];
	int		n, v, call_i;

	n = ft8_qso_words(cq_text, w);

	// CQ CALL [GRID] or CQ MODIFIER CALL [GRID]
	if((n < 2) || (strcmp(w[0], "CQ") != 0))
		return 1;

	call_i = 1;
	if((n >= 3) && (ft8_qso_extra(w[2], &v) == FT8_X_GRID))
		call_i = 1;
	else if(n >= 3)
		call_i = 2;

	if(w[call_i][0] == 0)
		return 2;

	vTaskSuspendAll();
	memset(&qso, 0, sizeof(qso));
	strncpy(qso.call, w[call_i], sizeof(qso.call) - 1);
	if(ft8_qso_extra(w[call_i + 1], &v) == FT8_X_GRID)
		strncpy(qso.grid, w[call_i + 1], sizeof(qso.grid) - 1);
	qso.rpt_sent = ft8_qso_report(their_snr);
	qso.state    = FT8_QSO_CALLING;
	qso_parity   = their_parity ^ 1;		// the slots they listen in
	qso_logged   = 0;
	xTaskResumeAll();

	ft8_qso_send(FT8_MY_GRID);

	printf("ft8 qso: answering %s in %s slots \r\n", qso.call, qso_parity ? "odd" : "even");
	return 0;
}

void ft8_qso_stop(void)
{
	qso.state = FT8_QSO_IDLE;
}

void ft8_qso_get(FT8_QSO_STATUS *st)
{
	*st = qso;
}

//*----------------------------------------------------------------------------
//* Function Name       : ft8_qso_is_ours
//* Context    			: any
//*----------------------------------------------------------------------------
uchar ft8_qso_is_ours(const char *msg)
{
	char	w[4][14];
	int		n = ft8_qso_words(msg, w);

	if(n < 2)
		return 0;

	if((strcmp(w[0], ft8_my_call()) == 0) || (strcmp(w[1], ft8_my_call()) == 0))
		return 1;

	// The station we are working, whoever they are talking to
	if((qso.call[0] != 0) && (qso.state != FT8_QSO_IDLE) && (qso.state != FT8_QSO_CQ) &&
	   ((strcmp(w[1], qso.call) == 0) || ((strcmp(w[0], "CQ") == 0) && (strstr(msg, qso.call) != NULL))))
		return 1;

	return 0;
}

//*----------------------------------------------------------------------------
//* Function Name       : ft8_qso_slot
//* Object              : advance the QSO on this slot's decodes
//* Notes    			: slot_parity is the parity of the slot decoded
//* Context    			: CONTEXT_FT8, right after the slot decode
//*----------------------------------------------------------------------------
void ft8_qso_slot(const FT8_DECODE_RESULT *res, int n, uchar slot_parity)
{
	char	w[4][14], rpt[6];
	int		i, cnt, kind, val = 0;

	if(qso.state == FT8_QSO_IDLE)
		return;

	for(i = 0; i < n; i++)
	{
		cnt = ft8_qso_words(res[i].text, w);

		// Only messages to us: US THEM EXTRA
		if((cnt < 2) || (strcmp(w[0], ft8_my_call()) != 0))
			continue;

		// Mid QSO only the station we are working counts
		if((qso.state != FT8_QSO_CQ) && (strcmp(w[1], qso.call) != 0))
			continue;

		kind = ft8_qso_extra(w[2], &val);

		switch(qso.state)
		{
			// Somebody answered our CQ
			case FT8_QSO_CQ:
			{
				strncpy(qso.call, w[1], sizeof(qso.call) - 1);
				qso.rpt_sent = ft8_qso_report(res[i].snr);
				qso_parity   = slot_parity ^ 1;
				qso_logged   = 0;

				if(kind == FT8_X_GRID)
					strncpy(qso.grid, w[2], sizeof(qso.grid) - 1);

				if((kind == FT8_X_GRID) || (kind == FT8_X_NONE))
				{
					ft8_qso_rpt_text(rpt, qso.rpt_sent, 0);
					qso.state = FT8_QSO_REPORT;
				}
				else if(kind == FT8_X_REPORT)
				{
					qso.rpt_rcvd  = (int8_t)val;
					qso.have_rcvd = 1;
					ft8_qso_rpt_text(rpt, qso.rpt_sent, 1);
					qso.state = FT8_QSO_RREPORT;
				}
				else
					continue;

				printf("ft8 qso: %s answered our cq \r\n", qso.call);
				ft8_qso_send(rpt);
				return;
			}

			// We answered their CQ
			case FT8_QSO_CALLING:
			{
				if(kind == FT8_X_REPORT)
				{
					qso.rpt_rcvd  = (int8_t)val;
					qso.have_rcvd = 1;
					qso.rpt_sent  = ft8_qso_report(res[i].snr);
					ft8_qso_rpt_text(rpt, qso.rpt_sent, 1);
					qso.state = FT8_QSO_RREPORT;
					ft8_qso_send(rpt);
				}
				else if(kind == FT8_X_RREPORT)
				{
					qso.rpt_rcvd  = (int8_t)val;
					qso.have_rcvd = 1;
					qso.state = FT8_QSO_RR73;
					ft8_qso_send("RR73");
				}
				else if((kind == FT8_X_RRR) || (kind == FT8_X_73))
				{
					qso.state = FT8_QSO_SEND73;
					ft8_qso_send("73");
				}
				return;
			}

			// We sent their report, waiting for R+report
			case FT8_QSO_REPORT:
			{
				if((kind == FT8_X_RREPORT) || (kind == FT8_X_REPORT))
				{
					qso.rpt_rcvd  = (int8_t)val;
					qso.have_rcvd = 1;
					qso.state = FT8_QSO_RR73;
					ft8_qso_send("RR73");
				}
				else if((kind == FT8_X_RRR) || (kind == FT8_X_73))
				{
					qso.state = FT8_QSO_SEND73;
					ft8_qso_send("73");
				}
				return;
			}

			// We sent R+report, waiting for RR73
			case FT8_QSO_RREPORT:
			{
				if(kind == FT8_X_RRR)
				{
					qso.state = FT8_QSO_SEND73;
					ft8_qso_send("73");
				}
				else if(kind == FT8_X_73)
				{
					ft8_qso_log();
					qso.state = FT8_QSO_DONE;
					ft8_proc_tx_arm(0);
				}
				return;
			}

			// They did not hear our RR73 and repeat R+report - send it again
			case FT8_QSO_DONE:
			{
				if(kind == FT8_X_RREPORT)
				{
					qso.state = FT8_QSO_RR73;
					ft8_qso_send("RR73");
				}
				return;
			}

			default:
				return;
		}
	}
}

//*----------------------------------------------------------------------------
//* Function Name       : ft8_qso_tx_sent
//* Object              : a frame went to the M4 - the one shot messages
//*						: (RR73, 73) complete the QSO
//* Context    			: CONTEXT_FT8
//*----------------------------------------------------------------------------
void ft8_qso_tx_sent(void)
{
	if((qso.state == FT8_QSO_RR73) || (qso.state == FT8_QSO_SEND73))
	{
		ft8_qso_log();
		qso.state = FT8_QSO_DONE;
		ft8_proc_tx_arm(0);
	}
}

#endif
