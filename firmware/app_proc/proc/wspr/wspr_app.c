/************************************************************************************
**                                                                                 **
**                                 mcHF QRP Transceiver                            **
**                         Krassi Atanassov - M0NKA, 2013-2026                     **
**                                                                                 **
**---------------------------------------------------------------------------------**
**                                                                                 **
**  File name:		wspr_app.c                                                     **
**  Description:	native WSPR app - beacon/receiver logic behind the WSPR       **
**					screen. Reception is the existing monitor (wspr_proc.c), this  **
**					adds the tx cycle choice, the frame build for the M4 symbol    **
**					streamer, the spot list for the screen and the band/VFO        **
**					save + restore around the app, the same way FT8 does it        **
**  Licence:		https://github.com/m0nka/mcHF/blob/main/LICENSE                **
************************************************************************************/
#include "mchf_pro_board.h"
#include "main.h"

#ifdef CONTEXT_WSPR

#include <math.h>
#include <string.h>

#include "rtc.h"
#include "mchf_icc_def.h"

#include "wspr_proc.h"
#include "wspr_encoder.h"
#include "wspr_app.h"

#include "mc_tx_build.h"
#include "icc_proc.h"
#include "ui_actions.h"

// FreeRTOS process state
extern struct PROC_STATE			ps;

// Public radio state
extern struct TRANSCEIVER_STATE_UI	tsu;

// Standard WSPR dial frequencies (USB carrier), indexed by radio band
// (BAND_MODE_xx), 0 = no WSPR on that band here. The signals sit at
// dial + 1400..1600 Hz
static const ulong wspr_band_dial[MAX_BANDS] =
{
	0,							// 0  - 2200m
	0,							// 1  - 630m
	1836600,					// 2  - 160m
	3568600,					// 3  - 80m
	5287200,					// 4  - 60m
	7038600,					// 5  - 40m
	10138700,					// 6  - 30m
	14095600,					// 7  - 20m
	18104600,					// 8  - 17m
	21094600,					// 9  - 15m
	24924600,					// 10 - 12m
	28124600,					// 11 - 10m
};

// Screen choices
static const uchar	wspr_ratio_pct[WSPR_APP_RATIOS]  = { 10, 20, 33, 50 };
static const char	*wspr_ratio_txt[WSPR_APP_RATIOS] = { "10 %", "20 %", "33 %", "50 %" };

// Audio centre of our signal, 0 = a random one per transmission
static const ushort	wspr_ofs_hz[WSPR_APP_OFFSETS]  = { 0, 1420, 1460, 1500, 1540, 1580 };
static const char	*wspr_ofs_txt[WSPR_APP_OFFSETS] = { "Random", "1420 Hz", "1460 Hz", "1500 Hz", "1540 Hz", "1580 Hz" };

// PA levels that land on a power WSPR can report (dBm ending 0, 3 or 7 -
// 15 W is 41.8 dBm, so it is left out)
static const uchar	wspr_pwr_level[WSPR_APP_POWERS] = { PA_LEVEL_0_5W, PA_LEVEL_1W, PA_LEVEL_2W, PA_LEVEL_5W, PA_LEVEL_10W, PA_LEVEL_20W };
static const uchar	wspr_pwr_dbm[WSPR_APP_POWERS]   = { 27, 30, 33, 37, 40, 43 };
static const char	*wspr_pwr_txt[WSPR_APP_POWERS]  = { "27 dBm  0.5 W", "30 dBm  1 W", "33 dBm  2 W", "37 dBm  5 W", "40 dBm  10 W", "43 dBm  20 W" };

// What the operator had on each band the app touched, restored on exit
static struct
{
	uchar	saved;
	ulong	vfo;						// active VFO frequency
	uchar	demod;
	uchar	filter;
	uchar	tx_power;

} wspr_saved[MAX_BANDS];

static uchar			wspr_home_band = 0xFF;		// band on entry, 0xFF = not entered

static volatile uchar	wspr_on = 0;				// screen is up
static WSPR_APP_CFG		wspr_cfg = { 1, 0, 1, 0, 2 };	// monitor on, tx off, 20 %, random, 0.5 W

// Cycle state - wspr task
static uchar			cyc_minute = 0xFF;			// even minute the tx choice was made for
static volatile uchar	cyc_tx = 0;					// this cycle transmits
static uchar			cyc_tx_sent = 0;
static volatile uchar	cyc_sec = 0;
static volatile uchar	tx_on = 0;					// exciter keyed by the M4
static TickType_t		tx_until = 0;
static volatile uchar	tx_next = 0;				// one shot request from the screen
static volatile uchar	tx_block = WSPR_TX_NO_STATION;
static ushort			tx_count = 0;
static ulong			rnd = 0x2545F491;

// ICC_MC_TX_START payload, staged here and sent by the icc task
static uchar			tx_payload[MC_TX_PAYLOAD_MAX];
static volatile ushort	tx_payload_len = 0;

// Spot list, newest at spot_head - 1
static WSPR_SPOT		spots[WSPR_APP_MAX_SPOTS];
static ushort			spot_head = 0;
static ushort			spot_count = 0;
static volatile ulong	spot_gen = 0;

//*----------------------------------------------------------------------------
//* Function Name       : wspr_app_rand
//* Object              : xorshift32 - tx cycle choice and random offset
//*----------------------------------------------------------------------------
static ulong wspr_app_rand(void)
{
	rnd ^= rnd << 13;
	rnd ^= rnd >> 17;
	rnd ^= rnd << 5;

	return rnd;
}

//*----------------------------------------------------------------------------
//* Function Name       : wspr_app_grid_ll
//* Object              : Maidenhead locator (4 or 6 chars) to the centre
//*						: of its square, degrees. 0 if not a locator
//*----------------------------------------------------------------------------
static uchar wspr_app_grid_ll(const char *g, float *lat, float *lon)
{
	if((strlen(g) < 4) ||
	   (g[0] < 'A') || (g[0] > 'R') || (g[1] < 'A') || (g[1] > 'R') ||
	   (g[2] < '0') || (g[2] > '9') || (g[3] < '0') || (g[3] > '9'))
		return 0;

	*lon = (g[0] - 'A') * 20.0f - 180.0f + (g[2] - '0') * 2.0f;
	*lat = (g[1] - 'A') * 10.0f -  90.0f + (g[3] - '0') * 1.0f;

	if((strlen(g) >= 6) && (g[4] >= 'A') && (g[4] <= 'X') && (g[5] >= 'A') && (g[5] <= 'X'))
	{
		*lon += (g[4] - 'A') * (2.0f / 24.0f) + (1.0f / 24.0f);
		*lat += (g[5] - 'A') * (1.0f / 24.0f) + (0.5f / 24.0f);
	}
	else
	{
		*lon += 1.0f;
		*lat += 0.5f;
	}

	return 1;
}

//*----------------------------------------------------------------------------
//* Function Name       : wspr_app_km
//* Object              : great circle distance between two locators
//*----------------------------------------------------------------------------
static ushort wspr_app_km(const char *a, const char *b)
{
	const float	r = 3.14159265f / 180.0f;
	float		la1, lo1, la2, lo2, c;

	if(!wspr_app_grid_ll(a, &la1, &lo1) || !wspr_app_grid_ll(b, &la2, &lo2))
		return 0xFFFF;

	la1 *= r; lo1 *= r; la2 *= r; lo2 *= r;

	c = sinf(la1) * sinf(la2) + cosf(la1) * cosf(la2) * cosf(lo2 - lo1);
	if(c > 1.0f)
		c = 1.0f;
	if(c < -1.0f)
		c = -1.0f;

	return (ushort)lroundf(acosf(c) * 6371.0f);
}

//*----------------------------------------------------------------------------
//* Function Name       : wspr_app_station
//* Object              : our type 1 message, the reason why not otherwise
//* Notes    			: type 1 only - no /P, no 6 char locator on air (the
//*						: first four of a 6 char settings.ini grid go out)
//*----------------------------------------------------------------------------
static uchar wspr_app_station(uchar *bits)
{
	char grid[5];

	if((tsu.station_call[0] == 0) || (strlen(tsu.station_grid) < 4))
		return WSPR_TX_NO_STATION;

	memcpy(grid, tsu.station_grid, 4);
	grid[4] = 0;

	if(wspr_pack_type1(tsu.station_call, grid, wspr_pwr_dbm[wspr_cfg.power], bits) != 0)
		return WSPR_TX_BAD_STATION;

	return WSPR_TX_OK;
}

//*----------------------------------------------------------------------------
//* Function Name       : wspr_app_decide_tx
//* Object              : does the cycle starting now transmit ?
//* Notes    			: random cycles at the chosen ratio, like WSJT-X, so
//*						: two beacons do not keep landing on each other
//* Context    			: CONTEXT_WSPR
//*----------------------------------------------------------------------------
static uchar wspr_app_decide_tx(void)
{
	if(tx_block != WSPR_TX_OK)
	{
		tx_next = 0;
		return 0;
	}

	if(tx_next)
	{
		tx_next = 0;
		return 1;
	}

	if(!wspr_cfg.tx_enable)
		return 0;

	return ((wspr_app_rand() % 100) < wspr_ratio_pct[wspr_cfg.tx_ratio]);
}

//*----------------------------------------------------------------------------
//* Function Name       : wspr_app_new_cycle
//* Object              : once per even minute - choose rx or tx for it
//* Context    			: CONTEXT_WSPR
//*----------------------------------------------------------------------------
static void wspr_app_new_cycle(uchar minute)
{
	if(cyc_minute == minute)
		return;

	cyc_minute	= minute;
	cyc_tx		= wspr_app_decide_tx();
	cyc_tx_sent = 0;
}

//*----------------------------------------------------------------------------
//* Function Name       : wspr_app_tx_start
//* Object              : build our frame and hand it to the icc task
//* Context    			: CONTEXT_WSPR
//*----------------------------------------------------------------------------
static uchar wspr_app_tx_start(void)
{
	uchar	bits[7];
	ushort	centre, len;

	if(wspr_app_station(bits) != WSPR_TX_OK)
		return 0;

	centre = wspr_ofs_hz[wspr_cfg.offset];
	if(centre == 0)
		centre = 1420 + (ushort)(wspr_app_rand() % 161);

	// The streamer wants tone 0, our centre is between tones 1 and 2
	if(mc_tx_build_payload(bits, centre - 2, NULL, 0, tx_payload, &len) != 0)
		return 0;

	tx_payload_len = len;
	tx_until = xTaskGetTickCount() + mc_tx_build_duration_ms(tx_payload) + 500;
	tx_on	 = 1;
	tx_count++;

	icc_proc_request(UI_ICC_WSPR_TX_START);

	printf("wspr app: tx %s %s %d dBm at %u Hz \r\n", tsu.station_call, tsu.station_grid,
			wspr_pwr_dbm[wspr_cfg.power], (uint)(wspr_app_band_dial(tsu.curr_band) + centre));
	return 1;
}

//*----------------------------------------------------------------------------
//* Function Name       : wspr_app_poll
//* Object              : tx cycle scheduler, every wspr task pass (100 ms)
//* Notes    			: runs before the monitor, so at second zero of an
//*						: even minute the tx choice is made before the
//*						: monitor decides whether to capture
//* Context    			: CONTEXT_WSPR
//*----------------------------------------------------------------------------
void wspr_app_poll(void)
{
	RTC_TimeTypeDef	tm = {0};
	RTC_DateTypeDef	dt = {0};
	uchar			bits[7];
	uchar			blk;

	if(!wspr_on)
		return;

	// Date read unlocks the shadow registers
	k_GetTime(&tm);
	k_GetDate(&dt);

	cyc_sec = (uchar)(((tm.Minutes & 1) * 60) + tm.Seconds);

	// Station and clock - a reset RTC comes up in year 2000
	blk = wspr_app_station(bits);
	if((blk == WSPR_TX_OK) && (dt.Year < 25))
		blk = WSPR_TX_NO_CLOCK;
	tx_block = blk;

	// Exciter unkeyed by now (the M4 has no completion message)
	if(tx_on && ((long)(xTaskGetTickCount() - tx_until) >= 0))
		tx_on = 0;

	// Odd minute - forget the last choice, the same minute number comes
	// back an hour later
	if(tm.Minutes & 1)
	{
		cyc_minute = 0xFF;
		return;
	}

	// New cycle - a second of slack in case this pass missed second zero
	if(tm.Seconds <= 1)
		wspr_app_new_cycle(tm.Minutes);

	// Our frame starts one second into the cycle - too late is a skip,
	// the decoders only look +/- a couple of seconds around it
	if(cyc_tx && !cyc_tx_sent && (tm.Seconds >= 1))
	{
		cyc_tx_sent = 1;

		if((tm.Seconds > 2) || tx_on || !wspr_app_tx_start())
		{
			printf("wspr app: tx cycle skipped \r\n");
			cyc_tx = 0;
		}
	}
}

//*----------------------------------------------------------------------------
//* Function Name       : wspr_app_cycle_is_tx
//* Object              : the monitor asks before capturing a cycle
//* Context    			: CONTEXT_WSPR
//*----------------------------------------------------------------------------
uchar wspr_app_cycle_is_tx(uchar minute)
{
	if(!wspr_on)
		return 0;

	// The monitor can see the new minute before this pass's poll did -
	// whoever gets there first makes the choice
	wspr_app_new_cycle(minute);

	return cyc_tx;
}

//*----------------------------------------------------------------------------
//* Function Name       : wspr_app_on_spot
//* Object              : one decoded type 1 message into the screen list
//* Context    			: CONTEXT_WSPR (decode pass)
//*----------------------------------------------------------------------------
void wspr_app_on_spot(const WSPR_DECODE *d, ulong dial_hz, ushort utc)
{
	WSPR_SPOT s;

	if(!wspr_on)
		return;

	memset(&s, 0, sizeof(s));

	s.utc	  = utc;
	s.snr	  = (short)lroundf(d->snr_db);
	s.dt10	  = (short)lroundf(d->dt_sec * 10.0f);
	s.drift	  = (signed char)lroundf(d->drift_hz);
	s.dbm	  = (signed char)d->dbm;
	s.freq_hz = dial_hz + (ulong)lroundf(d->freq_hz);
	s.km	  = wspr_app_km(tsu.station_grid, d->grid);

	strncpy(s.call, d->call, sizeof(s.call) - 1);
	strncpy(s.grid, d->grid, sizeof(s.grid) - 1);

	vTaskSuspendAll();

	spots[spot_head] = s;
	spot_head = (spot_head + 1) % WSPR_APP_MAX_SPOTS;
	if(spot_count < WSPR_APP_MAX_SPOTS)
		spot_count++;
	spot_gen++;

	xTaskResumeAll();
}

//*----------------------------------------------------------------------------
//* Function Name       : wspr_app_icc_tx_payload
//* Object              : staged frame for the icc task, taken once
//* Context    			: CONTEXT_ICC
//*----------------------------------------------------------------------------
uchar *wspr_app_icc_tx_payload(ushort *len)
{
	if(tx_payload_len == 0)
		return NULL;

	*len = tx_payload_len;
	tx_payload_len = 0;

	return tx_payload;
}

// ------------------------------------------------------------------------
// Screen side
//

uchar wspr_app_active(void)
{
	return wspr_on;
}

ulong wspr_app_band_dial(uchar band)
{
	if(band >= MAX_BANDS)
		return 0;

	return wspr_band_dial[band];
}

uchar wspr_app_band_ok(uchar band)
{
	return (wspr_app_band_dial(band) != 0);
}

int wspr_app_power_dbm(uchar idx)
{
	return (idx < WSPR_APP_POWERS) ? wspr_pwr_dbm[idx] : 0;
}

const char *wspr_app_power_text(uchar idx)
{
	return (idx < WSPR_APP_POWERS) ? wspr_pwr_txt[idx] : "";
}

const char *wspr_app_ratio_text(uchar idx)
{
	return (idx < WSPR_APP_RATIOS) ? wspr_ratio_txt[idx] : "";
}

const char *wspr_app_offset_text(uchar idx)
{
	return (idx < WSPR_APP_OFFSETS) ? wspr_ofs_txt[idx] : "";
}

//*----------------------------------------------------------------------------
//* Function Name       : wspr_app_setup_band
//* Object              : save a band's user settings (once per session) and
//*						: put it on its WSPR dial, USB, 3.6 kHz, our power
//* Context    			: CONTEXT_VIDEO (gui task)
//*----------------------------------------------------------------------------
static void wspr_app_setup_band(uchar band)
{
	struct BAND_INFO *b = &tsu.band[band];

	if(!wspr_saved[band].saved)
	{
		wspr_saved[band].vfo	  = (b->active_vfo == VFO_A) ? b->vfo_a : b->vfo_b;
		wspr_saved[band].demod	  = b->demod_mode;
		wspr_saved[band].filter	  = b->filter;
		wspr_saved[band].tx_power = b->tx_power;
		wspr_saved[band].saved	  = 1;
	}

	if(b->active_vfo == VFO_A)
		b->vfo_a = wspr_band_dial[band];
	else
		b->vfo_b = wspr_band_dial[band];

	b->demod_mode = DEMOD_USB;
	b->filter	  = AUDIO_3P6KHZ;
	b->tx_power	  = wspr_pwr_level[wspr_cfg.power];

	printf("wspr app: band %d -> %u Hz usb \r\n", band, (uint)wspr_band_dial[band]);
}

//*----------------------------------------------------------------------------
//* Function Name       : wspr_app_set_band
//* Object              : band dropdown - refused while our frame is on the
//*						: air or about to be (the filters would switch under
//*						: the PA). A capture running is dropped, its audio is
//*						: from the old band
//* Context    			: CONTEXT_VIDEO (gui task)
//*----------------------------------------------------------------------------
uchar wspr_app_set_band(uchar band)
{
	if(!wspr_app_band_ok(band) || tx_on || cyc_tx)
		return 0;

	if(band == tsu.curr_band)
		return 1;

	wspr_proc_cycle_abort();

	wspr_app_setup_band(band);

	// Filters, VFO, audio and the DSP (mode, filter, power ride along)
	ui_actions_change_band(band, 1);

	return 1;
}

//*----------------------------------------------------------------------------
//* Function Name       : wspr_app_enter
//* Object              : screen entry - tune the current band (or the next
//*						: one with a WSPR frequency) and start listening
//* Context    			: CONTEXT_VIDEO (gui task)
//*----------------------------------------------------------------------------
void wspr_app_enter(void)
{
	uchar	band = tsu.curr_band;
	int		i;

	for(i = 0; i < MAX_BANDS; i++)
		wspr_saved[i].saved = 0;

	wspr_home_band = tsu.curr_band;

	for(i = 0; (i < MAX_BANDS) && !wspr_app_band_ok(band); i++)
		band = (uchar)((band + 1) % MAX_BANDS);

	wspr_app_setup_band(band);

	if(band != tsu.curr_band)
		ui_actions_change_band(band, 1);
	else
	{
		if(ps.hVfoTask != NULL)
			xTaskNotify(ps.hVfoTask, UI_NEW_FREQ_EVENT, eSetValueWithOverwrite);

		icc_proc_request(UI_ICC_CHANGE_BAND);
	}

	rnd ^= HAL_GetTick();
	if(rnd == 0)
		rnd = 0x2545F491;

	cyc_tx		= 0;
	cyc_minute	= 0xFF;
	tx_next		= 0;
	tx_count	= 0;
	wspr_on		= 1;

	wspr_proc_monitor_set(wspr_cfg.monitor);
}

//*----------------------------------------------------------------------------
//* Function Name       : wspr_app_exit
//* Object              : screen exit - stop everything, give every touched
//*						: band its settings back, return to the entry band
//* Context    			: CONTEXT_VIDEO (gui task)
//*----------------------------------------------------------------------------
void wspr_app_exit(void)
{
	struct BAND_INFO	*b;
	int					i;

	if(wspr_home_band == 0xFF)
		return;

	wspr_on = 0;
	cyc_tx	= 0;

	wspr_proc_monitor_set(0);

	// Abort a frame on the air before anything retunes. Queued requests
	// go out in UI_ICC_xx order, a band change (2) would beat the stop
	// (12) - and the band task moves the filters right away - so give
	// the stop a head start
	if(tx_on)
	{
		tx_on = 0;
		icc_proc_request(UI_ICC_MC_TX_STOP);
		vTaskDelay(300);
	}

	for(i = 0; i < MAX_BANDS; i++)
	{
		if(!wspr_saved[i].saved)
			continue;

		b = &tsu.band[i];

		if(b->active_vfo == VFO_A)
			b->vfo_a = wspr_saved[i].vfo;
		else
			b->vfo_b = wspr_saved[i].vfo;

		b->demod_mode = wspr_saved[i].demod;
		b->filter	  = wspr_saved[i].filter;
		b->tx_power	  = wspr_saved[i].tx_power;

		wspr_saved[i].saved = 0;
	}

	if(tsu.curr_band != wspr_home_band)
		ui_actions_change_band(wspr_home_band, 1);
	else
	{
		if(ps.hVfoTask != NULL)
			xTaskNotify(ps.hVfoTask, UI_NEW_FREQ_EVENT, eSetValueWithOverwrite);

		icc_proc_request(UI_ICC_CHANGE_BAND);
	}

	printf("wspr app: radio restored, band %d \r\n", wspr_home_band);

	wspr_home_band = 0xFF;
}

//*----------------------------------------------------------------------------
//* Function Name       : wspr_app_set_cfg / wspr_app_get_cfg
//* Context    			: CONTEXT_VIDEO (gui task)
//*----------------------------------------------------------------------------
void wspr_app_set_cfg(const WSPR_APP_CFG *cfg)
{
	uchar mon = wspr_cfg.monitor;
	uchar pwr = wspr_cfg.power;

	wspr_cfg = *cfg;

	if(wspr_cfg.tx_ratio >= WSPR_APP_RATIOS)
		wspr_cfg.tx_ratio = 0;
	if(wspr_cfg.offset >= WSPR_APP_OFFSETS)
		wspr_cfg.offset = 0;
	if(wspr_cfg.power >= WSPR_APP_POWERS)
		wspr_cfg.power = 0;

	if(wspr_on && (mon != wspr_cfg.monitor))
		wspr_proc_monitor_set(wspr_cfg.monitor);

	// Power is a PA level of the current band
	if(wspr_on && (pwr != wspr_cfg.power))
	{
		tsu.band[tsu.curr_band].tx_power = wspr_pwr_level[wspr_cfg.power];
		icc_proc_request(UI_ICC_POWER_LEVEL);
	}
}

void wspr_app_get_cfg(WSPR_APP_CFG *cfg)
{
	*cfg = wspr_cfg;
}

void wspr_app_tx_next(void)
{
	tx_next = 1;
}

void wspr_app_clear_spots(void)
{
	vTaskSuspendAll();
	spot_head  = 0;
	spot_count = 0;
	spot_gen++;
	xTaskResumeAll();
}

//*----------------------------------------------------------------------------
//* Function Name       : wspr_app_get_status
//* Context    			: CONTEXT_VIDEO (gui task)
//*----------------------------------------------------------------------------
void wspr_app_get_status(WSPR_APP_STATUS *st)
{
	uchar ph = wspr_proc_phase();

	if(tx_on || (cyc_tx && !cyc_tx_sent))
		st->phase = WSPR_PH_TX;
	else if(ph == WSPR_PROC_PH_CAPTURE)
		st->phase = WSPR_PH_RX;
	else if(ph == WSPR_PROC_PH_DECODE)
		st->phase = WSPR_PH_DECODE;
	else if(ph == WSPR_PROC_PH_WAIT)
		st->phase = WSPR_PH_WAIT;
	else
		st->phase = WSPR_PH_IDLE;

	st->cycle_sec	= cyc_sec;
	st->tx_block	= tx_block;
	st->tx_next		= tx_next;
	st->spot_gen	= spot_gen;
	st->spot_count	= spot_count;
	st->tx_count	= tx_count;
}

//*----------------------------------------------------------------------------
//* Function Name       : wspr_app_get_spots
//* Object              : copy of the list, newest first
//* Context    			: CONTEXT_VIDEO (gui task)
//*----------------------------------------------------------------------------
ushort wspr_app_get_spots(WSPR_SPOT *out, ushort max)
{
	ushort i, n;

	vTaskSuspendAll();

	n = (spot_count < max) ? spot_count : max;
	for(i = 0; i < n; i++)
		out[i] = spots[(spot_head + WSPR_APP_MAX_SPOTS - 1 - i) % WSPR_APP_MAX_SPOTS];

	xTaskResumeAll();

	return n;
}

#endif
