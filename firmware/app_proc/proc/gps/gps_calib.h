/************************************************************************************
**                                                                                 **
**                                 mcHF QRP Transceiver                            **
**                         Krassi Atanassov - M0NKA, 2013-2026                     **
**                                                                                 **
**---------------------------------------------------------------------------------**
**                                                                                 **
**  File name:		gps_calib.h                                                    **
**  Description:	LSE trim measurement against GPS PPS (see gps_calib.c)         **
**  Licence:		https://github.com/m0nka/mcHF/blob/main/LICENSE                **
************************************************************************************/
#ifndef __GPS_CALIB_H
#define __GPS_CALIB_H

// Local debug
//#define GPS_CALIB_PRINT

// A PPS edge more than this far from a whole number of seconds is noise,
// not the GPS - drop it and restart the baseline. 1/4 second
#define CALIB_PHASE_REJECT_DIV		4

// Longest gap we will bridge. Beyond this (sky blocked, module reset) the
// pulse count can no longer be trusted to be the true elapsed time
#define CALIB_MAX_GAP_S				30



// Below this many PPS edges no result is offered at all - a short run is
// dominated by the subsecond quantisation and would just be noise
#define GPS_CALIB_MIN_SAMPLES		120u

// Runs longer than this stop accumulating: sum(x^2) grows as n^3/3 and
// would overflow the int64 accumulators somewhere past a month
#define GPS_CALIB_MAX_SPAN_S		1209600UL		// 14 days

// The whole method rests on the frequency error ITSELF sweeping the phase
// across subsecond LSBs - that is what decorrelates the quantisation. Below
// about 16 LSB of total sweep the least squares error bar is a lie:
// simulation (claude/rtc/calib_sim.py) has it optimistic by 2.75x at 300 s
// and by 139x for a 0.3 ppm clock over an hour. So no result is offered
// until the sweep is real
#define GPS_CALIB_MIN_SWEEP_LSB		16

// ...with one escape hatch. An already good clock may never reach that sweep
// (0.3 ppm needs ~58 h), so past this span a result is offered anyway and
// the error bar is floored to a conservative bound instead of the fitted one
#define GPS_CALIB_LONG_SPAN_S		21600UL			// 6 hours

// Everything is scaled integer - the tiny printf in common/print_f.c has
// no %f, and the UI would have to hand format a float anyway
typedef struct
{
	uint8_t		running;			// accumulating right now
	uint8_t		armed;				// GPS fix good enough to trust PPS
	uint8_t		valid;				// enough samples for a result

	uint32_t	samples;			// PPS edges accepted
	uint32_t	span_s;				// GPS seconds from first to last edge
	uint32_t	glitches;			// edges rejected (noise / missed pulses)
	uint32_t	sweep_lsb_x10;		// phase swept, in tenths of a subsecond LSB
	uint32_t	eta_s;				// seconds left to a trustworthy result

	int32_t		residual_ppm_x100;	// measured drift WITH the current trim on
	int32_t		err_ppm_x100;		// 1 sigma uncertainty of the above
	int32_t		trim_now_ppm;		// trim currently in RTC_CALR
	int32_t		trim_new_ppm;		// what to store = trim_now - residual

} gps_calib_stat_t;

typedef struct
{
	volatile uint8_t	run;
	volatile uint8_t	armed;

	uint16_t			prediv_s;			// RTC_PRER synchronous value
	uint16_t			ticks_per_sec;		// prediv_s + 1
	int32_t				minute_ticks;		// 60 * ticks_per_sec

	// Baseline / unwrap state
	uint8_t				have_base;
	int32_t				last_tick;			// tick within the minute
	int64_t				x;					// GPS seconds since baseline
	int64_t				y;					// cumulative RTC phase error, ticks

	// Least squares accumulators (int64, ISR context)
	int64_t				cnt, sx, sy, sxy, sxx, syy;

	volatile uint32_t	glitches;

} gps_calib_t;

// -----------------------------------------------------------------------------------

// One time setup - reads PREDIV_S out of the RTC
void gps_calib_init(void);

// Start / stop a measurement run. Starting clears any previous run
void gps_calib_start(void);
void gps_calib_stop(void);

// Tell the engine whether the GPS fix is good. PPS edges are ignored while
// disarmed, because most modules free run PPS without a lock
void gps_calib_arm(uint8_t on);

// PPS rising edge. CONTEXT_IRQ - reads the RTC and accumulates, nothing else
void gps_calib_pps_edge(void);

// Snapshot for the UI / console. Returns 0 when a result is available
int gps_calib_get(gps_calib_stat_t *out);

// Apply and persist the measured trim. Returns 0 on success
int gps_calib_accept(void);

// One line summary to the debug UART
void gps_calib_print(void);

#endif
