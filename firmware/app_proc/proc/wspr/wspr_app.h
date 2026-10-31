/************************************************************************************
**                                                                                 **
**                                 mcHF QRP Transceiver                            **
**                         Krassi Atanassov - M0NKA, 2013-2026                     **
**                                                                                 **
**---------------------------------------------------------------------------------**
**                                                                                 **
**  File name:		wspr_app.h                                                     **
**  Description:	native WSPR app - beacon/receiver logic behind the WSPR       **
**					screen (MODE_DESKTOP_WSPR), runs in the wspr task              **
**  Licence:		https://github.com/m0nka/mcHF/blob/main/LICENSE                **
************************************************************************************/
#ifndef __WSPR_APP_H
#define __WSPR_APP_H

#include "wspr_decoder.h"

// Spots kept for the screen, newest first. Older ones are only in the
// SD card log (0://wspr/decodes.txt)
#define WSPR_APP_MAX_SPOTS			100

// Choices on the screen - indexes into these tables
#define WSPR_APP_RATIOS				4		// 10, 20, 33, 50 %
#define WSPR_APP_OFFSETS			6		// random, 1420..1580 Hz
#define WSPR_APP_POWERS				6		// 27..43 dBm

// Cycle phase, for the progress bar
#define WSPR_PH_IDLE				0		// monitor off, no tx this cycle
#define WSPR_PH_RX					1		// capturing
#define WSPR_PH_DECODE				2		// capture done, decoder running
#define WSPR_PH_TX					3		// our frame is on the air
#define WSPR_PH_WAIT				4		// armed, waiting for the even minute

// Why we may not transmit, WSPR_TX_xx
#define WSPR_TX_OK					0
#define WSPR_TX_NO_STATION			1		// settings.ini [station] call/grid empty
#define WSPR_TX_BAD_STATION			2		// call/grid not a type 1 message
#define WSPR_TX_NO_CLOCK			3		// RTC was never set

typedef struct
{
	ushort	utc;							// hhmm, start of the cycle
	short	snr;							// dB
	short	dt10;							// time offset, 0.1 s
	signed char	drift;						// Hz (plain char is unsigned on ARM)
	signed char	dbm;
	ulong	freq_hz;						// rf frequency of the signal
	ushort	km;								// distance from our grid, 0xFFFF = unknown
	char	call[8];
	char	grid[6];

} WSPR_SPOT;

// Operator settings - written by the screen only
typedef struct
{
	uchar	monitor;						// receive and decode every cycle
	uchar	tx_enable;						// beacon in random cycles
	uchar	tx_ratio;						// index, WSPR_APP_RATIOS
	uchar	offset;							// index, WSPR_APP_OFFSETS
	uchar	power;							// index, WSPR_APP_POWERS

} WSPR_APP_CFG;

// Snapshot for the screen
typedef struct
{
	uchar	phase;							// WSPR_PH_xx
	uchar	cycle_sec;						// 0..119 into the two minute cycle
	uchar	tx_block;						// WSPR_TX_xx
	uchar	tx_next;						// one shot tx armed
	ulong	spot_gen;						// bumps on every new batch of spots
	ushort	spot_count;
	ushort	tx_count;						// frames sent since entry

} WSPR_APP_STATUS;

// Screen side - gui task
void		wspr_app_enter(void);
void		wspr_app_exit(void);
uchar		wspr_app_active(void);
void		wspr_app_set_cfg(const WSPR_APP_CFG *cfg);
void		wspr_app_get_cfg(WSPR_APP_CFG *cfg);
void		wspr_app_tx_next(void);
void		wspr_app_clear_spots(void);
void		wspr_app_get_status(WSPR_APP_STATUS *st);
ushort		wspr_app_get_spots(WSPR_SPOT *out, ushort max);
uchar		wspr_app_set_band(uchar band);
uchar		wspr_app_band_ok(uchar band);
ulong		wspr_app_band_dial(uchar band);
int			wspr_app_power_dbm(uchar idx);
const char	*wspr_app_power_text(uchar idx);
const char	*wspr_app_ratio_text(uchar idx);
const char	*wspr_app_offset_text(uchar idx);

// Engine side - wspr task
void		wspr_app_poll(void);
uchar		wspr_app_cycle_is_tx(uchar minute);
void		wspr_app_on_spot(const WSPR_DECODE *d, ulong dial_hz, ushort utc);

// icc task - the staged ICC_MC_TX_START payload, NULL if none
uchar		*wspr_app_icc_tx_payload(ushort *len);

#endif
