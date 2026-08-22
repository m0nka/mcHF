/************************************************************************************
**                                                                                 **
**                                 mcHF QRP Transceiver                            **
**                         Krassi Atanassov - M0NKA, 2013-2026                     **
**                                                                                 **
**---------------------------------------------------------------------------------**
**                                                                                 **
**  File name:		gps_calib.c                                                    **
**  Description:	Measures the LSE trim of THIS unit against GPS PPS.            **
**  Licence:		https://github.com/m0nka/mcHF/blob/main/LICENSE                **
************************************************************************************/
//
// HOW THIS WORKS, AND WHY IT IS FAST
//
// Reading the wall clock by eye against a reference resolves 1 second, so
// telling 1 ppm apart from 2 needs about 12 days. The RTC subsecond register
// resolves 1/(PREDIV_S+1) = 1/256 s = 3.9 ms, which is 256 times finer, and
// GPS PPS marks the true second boundary to well under a microsecond.
//
// So instead of counting whole seconds we record the RTC PHASE ERROR at each
// PPS edge and fit a straight line through it. The slope is the frequency
// error directly. Over an hour (3600 samples) the slope standard error from
// the 3.9 ms quantisation is about 0.02 ppm - three orders better than the
// manual method, in 1/144 of the time.
//
// The accumulated value is the RESIDUAL, i.e. what is left over on top of
// whatever RTC_CALR is already applying. The new trim is therefore
//
//		trim_new = trim_now - residual
//
// Getting that subtraction backwards is exactly what happened by hand on
// 2026-07-22 (a +29 ppm trim was applied to a crystal that was already
// running fast, doubling the error instead of removing it). Doing it here
// removes that whole class of mistake.
//
// WHAT LIMITS IT
//
// Not the measurement - temperature. A 32.768 kHz tuning fork is parabolic
// at about -0.034 ppm/degC^2 either side of a +25 degC turnover, so +/-10 degC
// is already several ppm. An hour on the bench gives a very precise answer
// for BENCH temperature only. That is the argument for leaving a run going
// for days: it averages over the temperatures the radio actually sees.
//
// One consequence worth knowing: the precision above relies on the frequency
// error itself sweeping the quantisation. At 20 ppm the phase crosses a full
// LSB every ~195 s and the errors decorrelate nicely. Once trimmed to 0.3 ppm
// one LSB takes 3.6 hours, so VERIFYING a good clock is much slower than
// fixing a bad one - expect to run verification for hours, not minutes.
//
#include "main.h"
#include "mchf_pro_board.h"

#ifdef CONTEXT_GPS

#include "rtc.h"
#include "gps_calib.h"

#include <string.h>
#include <math.h>

extern RTC_HandleTypeDef RtcHandle;

static gps_calib_t	gc;

//*----------------------------------------------------------------------------
//* Function Name       : gps_calib_init
//* Object              : read the subsecond resolution actually in force
//* Notes    			: PREDIV_S is whatever rtc.c programmed - do not
//*						: hardcode it here, the two would drift apart
//* Context    			: CONTEXT_GPS
//*----------------------------------------------------------------------------
void gps_calib_init(void)
{
	memset(&gc, 0, sizeof(gc));

	gc.prediv_s      = (uint16_t)(RtcHandle.Instance->PRER & RTC_PRER_PREDIV_S);
	gc.ticks_per_sec = (uint16_t)(gc.prediv_s + 1u);
	gc.minute_ticks  = (int32_t)gc.ticks_per_sec * 60;

	#ifdef GPS_CALIB_PRINT
	printf("gps calib: %u subsecond ticks (%u us resolution)\r\n",
			(unsigned)gc.ticks_per_sec,
			(unsigned)(1000000UL / gc.ticks_per_sec));
	#endif
}

//*----------------------------------------------------------------------------
//* Function Name       : gps_calib_start / _stop / _arm
//* Context    			: CONTEXT_GPS
//*----------------------------------------------------------------------------
void gps_calib_start(void)
{
	// Stop first so a PPS edge landing mid clear cannot accumulate into a
	// half reset state
	gc.run = 0;

	gc.have_base = 0;
	gc.x         = 0;
	gc.y         = 0;
	gc.cnt       = 0;
	gc.sx        = 0;
	gc.sy        = 0;
	gc.sxy       = 0;
	gc.sxx       = 0;
	gc.syy       = 0;
	gc.glitches  = 0;

	gc.run = 1;

	#ifdef GPS_CALIB_PRINT
	printf("gps calib: started, trim now %d ppm\r\n", (int)rtc_calib_ppm_get());
	#endif
}

void gps_calib_stop(void)
{
	gc.run = 0;
}

void gps_calib_arm(uint8_t on)
{
	// Losing the fix invalidates the pulse train, not the samples already
	// taken - drop the baseline so the gap is not counted as elapsed time
	if((!on) && (gc.armed))
		gc.have_base = 0;

	gc.armed = on ? 1 : 0;
}

//*----------------------------------------------------------------------------
//* Function Name       : gps_calib_pps_edge
//* Object              : latch the RTC phase at the true UTC second boundary
//* Notes    			: integer only, no FPU state touched in IRQ context
//* Context    			: CONTEXT_IRQ (PPS)
//*----------------------------------------------------------------------------
void gps_calib_pps_edge(void)
{
	uint32_t	ssr, tr;
	int32_t		sec, tick, delta, elapsed, resid, reject;

	if((!gc.run) || (!gc.armed))
		return;

	if(gc.x >= (int64_t)GPS_CALIB_MAX_SPAN_S)
		return;

	// RM0433: reading SSR/TR freezes the shadow registers until DR is read
	ssr = RtcHandle.Instance->SSR;
	tr  = RtcHandle.Instance->TR;
	(void)RtcHandle.Instance->DR;

	// Seconds field is BCD, and SSR counts DOWN from PREDIV_S
	sec  = (int32_t)(((tr >> 4) & 0x07u) * 10u + (tr & 0x0Fu));
	tick = sec * (int32_t)gc.ticks_per_sec +
			((int32_t)gc.prediv_s - (int32_t)(ssr & 0xFFFFu));

	if(!gc.have_base)
	{
		gc.last_tick = tick;
		gc.have_base = 1;
		return;
	}

	// Unwrap across the minute boundary
	delta = tick - gc.last_tick;
	if(delta < 0)
		delta += gc.minute_ticks;

	// Whole seconds actually elapsed - derived from the phase, NOT assumed
	// to be one, so a missed pulse does not silently become a frequency error
	elapsed = (delta + (int32_t)gc.ticks_per_sec / 2) / (int32_t)gc.ticks_per_sec;

	reject = (int32_t)gc.ticks_per_sec / CALIB_PHASE_REJECT_DIV;
	resid  = delta - elapsed * (int32_t)gc.ticks_per_sec;

	if((elapsed < 1) || (elapsed > CALIB_MAX_GAP_S) ||
	   (resid > reject) || (resid < -reject))
	{
		// Spurious edge or a gap too long to bridge - restart the baseline
		gc.glitches++;
		gc.last_tick = tick;
		gc.have_base = 1;
		return;
	}

	gc.last_tick = tick;

	gc.x += elapsed;		// GPS seconds since baseline
	gc.y += resid;			// cumulative RTC phase error in ticks

	gc.cnt++;
	gc.sx  += gc.x;
	gc.sy  += gc.y;
	gc.sxy += gc.x * gc.y;
	gc.sxx += gc.x * gc.x;
	gc.syy += gc.y * gc.y;
}

//*----------------------------------------------------------------------------
//* Function Name       : gps_calib_get
//* Object              : least squares slope -> ppm, with its uncertainty.
//*						: 0 = a usable result is in *out
//* Notes    			: double precision on purpose - sxx reaches 1e17 over
//*						: a long run and the centred form cancels hard
//* Context    			: CONTEXT_GPS / UI
//*----------------------------------------------------------------------------
int gps_calib_get(gps_calib_stat_t *out)
{
	double	n, sx, sy, sxy, sxx, syy;
	double	sxx_c, sxy_c, syy_c, slope, ppm, se_ppm, sse, sweep;

	if(out == NULL)
		return 1;

	memset(out, 0, sizeof(*out));

	out->running      = gc.run;
	out->armed        = gc.armed;
	out->glitches     = gc.glitches;
	out->samples      = (uint32_t)gc.cnt;
	out->span_s       = (uint32_t)gc.x;
	out->trim_now_ppm = rtc_calib_ppm_get();
	out->trim_new_ppm = out->trim_now_ppm;

	if(gc.cnt < (int64_t)GPS_CALIB_MIN_SAMPLES)
		return 1;

	// Snapshot once - the ISR keeps running underneath. A torn read only
	// costs one sample's worth of consistency, which is far below the noise
	n   = (double)gc.cnt;
	sx  = (double)gc.sx;
	sy  = (double)gc.sy;
	sxy = (double)gc.sxy;
	sxx = (double)gc.sxx;
	syy = (double)gc.syy;

	sxx_c = sxx - (sx * sx) / n;
	sxy_c = sxy - (sx * sy) / n;
	syy_c = syy - (sy * sy) / n;

	if(sxx_c <= 0.0)
		return 1;

	// Slope is in RTC ticks per GPS second; one second nominally advances
	// ticks_per_sec ticks, so the fractional error is slope/ticks_per_sec
	slope = sxy_c / sxx_c;
	ppm   = (slope / (double)gc.ticks_per_sec) * 1000000.0;

	// Residual scatter about the fit gives the honest error bar - it also
	// catches jitter the quantisation model does not know about
	sse = syy_c - (slope * sxy_c);
	if(sse < 0.0)
		sse = 0.0;

	se_ppm = 0.0;
	if(gc.cnt > 2)
	{
		se_ppm = sqrt((sse / (n - 2.0)) / sxx_c);
		se_ppm = (se_ppm / (double)gc.ticks_per_sec) * 1000000.0;
	}

	// Total phase swept, in subsecond LSBs. This is the dithering quality,
	// and below GPS_CALIB_MIN_SWEEP_LSB the error bar above cannot be
	// believed - the residuals stop being independent
	sweep = fabs(slope) * (double)gc.x;
	out->sweep_lsb_x10 = (uint32_t)((sweep * 10.0) + 0.5);

	if(sweep < (double)GPS_CALIB_MIN_SWEEP_LSB)
	{
		if(gc.x < (int64_t)GPS_CALIB_LONG_SPAN_S)
		{
			// Still under-dithered and not yet old enough to fall back on
			// span alone. Say how much longer, from the drift seen so far
			if(sweep > 0.0)
			{
				double	eta = ((double)GPS_CALIB_MIN_SWEEP_LSB / sweep) * (double)gc.x;

				if(eta > (double)GPS_CALIB_LONG_SPAN_S)
					eta = (double)GPS_CALIB_LONG_SPAN_S;

				if(eta > (double)gc.x)
					out->eta_s = (uint32_t)(eta - (double)gc.x);
			}
			else
			{
				out->eta_s = (uint32_t)(GPS_CALIB_LONG_SPAN_S - gc.x);
			}

			return 1;
		}

		// Long run, poor dithering: the fit is still accurate (simulation
		// puts a 0.3 ppm clock at 0.03 ppm after 6 h) but the fitted error
		// bar is not, so replace it with a bound that never flatters
		{
			double	floor_ppm = ((0.5 / (double)gc.ticks_per_sec) / (double)gc.x) * 1000000.0;

			if(se_ppm < floor_ppm)
				se_ppm = floor_ppm;
		}
	}

	out->residual_ppm_x100 = (int32_t)((ppm    * 100.0) + ((ppm    >= 0.0) ? 0.5 : -0.5));
	out->err_ppm_x100      = (int32_t)((se_ppm * 100.0) + 0.5);

	// The measurement was made WITH the current trim applied, so what is
	// left over has to come off it. This is the step that went wrong by hand
	out->trim_new_ppm = out->trim_now_ppm -
			(int32_t)((ppm >= 0.0) ? (ppm + 0.5) : (ppm - 0.5));

	out->valid = 1;

	return 0;
}

//*----------------------------------------------------------------------------
//* Function Name       : gps_calib_accept
//* Object              : store the measured trim for this unit. 0 = done
//* Context    			: CONTEXT_GPS / UI
//*----------------------------------------------------------------------------
int gps_calib_accept(void)
{
	gps_calib_stat_t st;

	if(gps_calib_get(&st) != 0)
	{
		#ifdef GPS_CALIB_PRINT
		printf("gps calib: not enough data yet (%u samples)\r\n",
				(unsigned)st.samples);
		#endif
		return 1;
	}

	gps_calib_stop();

	return rtc_calib_ppm_save(st.trim_new_ppm);
}

//*----------------------------------------------------------------------------
//* Function Name       : gps_calib_print
//* Object              : progress line. Hand formats the ppm because the
//*						: tiny printf has no %f
//* Context    			: CONTEXT_GPS
//*----------------------------------------------------------------------------
void gps_calib_print(void)
{
	gps_calib_stat_t	st;
	int32_t				r, e;
	char				sign;

	if(gps_calib_get(&st) != 0)
	{
		#ifdef GPS_CALIB_PRINT
		printf("gps calib: %u samples, %u s, armed %d, sweep %u.%u lsb, ~%u min left\r\n",
				(unsigned)st.samples, (unsigned)st.span_s, (int)st.armed,
				(unsigned)(st.sweep_lsb_x10 / 10), (unsigned)(st.sweep_lsb_x10 % 10),
				(unsigned)((st.eta_s + 59u) / 60u));
		#endif
		return;
	}

	r    = st.residual_ppm_x100;
	sign = (r < 0) ? '-' : '+';
	if(r < 0)
		r = -r;

	e = st.err_ppm_x100;

	#ifdef GPS_CALIB_PRINT
	printf("gps calib: %u s, residual %c%d.%02d +/- %d.%02d ppm, trim %d -> %d, %u bad\r\n",
			(unsigned)st.span_s,
			sign, (int)(r / 100), (int)(r % 100),
			(int)(e / 100), (int)(e % 100),
			(int)st.trim_now_ppm, (int)st.trim_new_ppm,
			(unsigned)st.glitches);
	#endif
}

#endif // CONTEXT_GPS
