/************************************************************************************
**                                                                                 **
**                                 mcHF QRP Transceiver                            **
**                         Krassi Atanassov - M0NKA, 2013-2026                     **
**                                                                                 **
**---------------------------------------------------------------------------------**
**                                                                                 **
**  File name:		wspr_decoder.c                                                 **
**  Description:	WSPR decoder core                                              **
**  Last Modified:                                                                 **
**  Licence:		https://github.com/m0nka/mcHF/blob/main/LICENSE                **
************************************************************************************/
//
// Decoder pipeline:
//
//   12 kHz PCM -> complex mix @1500 Hz -> FIR decimate x8 -> FIR decimate x4
//   -> 375 Hz complex baseband (162 symbols x 256 samples per transmission)
//   -> 512 pt spectrogram, 128 sample step -> candidate peak search
//   -> coarse time/drift sync on spectrogram (sync vector correlation)
//   -> fine time/freq/drift sync by coherent 4-FSK demodulation
//   -> soft symbols -> deinterleave -> Fano decode -> message unpack
//
// Constants (sync vector, interleaver, packing) follow K9AN/KA9Q wsprd (GPLv3)
//
#include <math.h>
#include <string.h>
#include <stdio.h>

#include "wspr_decoder.h"
#include "wspr_fano.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// Large working buffers - on target they live in the dedicated SDRAM
// section (WSPR_RAM region in the linker script), on a PC build in bss
#ifdef WSPR_HOST_BUILD
#define WSPR_BIG_RAM
#else
#define WSPR_BIG_RAM __attribute__((section(".wspr_mem"))) __attribute__ ((aligned (32)))
#endif

// Front end decimator dimensioning
#define DECIM1					8							// 12000 -> 1500 Hz
#define DECIM2					4							// 1500  ->  375 Hz
#define NTAPS					64							// per stage
#define BIN_HZ					((float)WSPR_FS_BB / WSPR_FFT_SIZE)

// Candidate search
// +/- bins around center, integer constant = (110 Hz / 0.7324 Hz) = 150
#define SEARCH_BINS				((110 * WSPR_FFT_SIZE) / WSPR_FS_BB)
#define SNR_FLOOR_DB			-28.0f						// reject weaker candidates
#define SNR_SCALING_DB			26.3f						// 0.73 Hz bin -> 2500 Hz ref
#define MIN_SYNC				0.08f						// reject unsynced candidates

// Fano tuning
#define FANO_DELTA				60
#define FANO_MAXCYCLES			3000

// ------------------------------------------------------------------------
// Large buffers (SDRAM)

WSPR_BIG_RAM static float	bb_i[WSPR_MAX_BB_SAMPLES];		// baseband I, 375 Hz
WSPR_BIG_RAM static float	bb_q[WSPR_MAX_BB_SAMPLES];		// baseband Q, 375 Hz
WSPR_BIG_RAM static float	ps[WSPR_MAX_FRAMES][WSPR_FFT_SIZE];	// spectrogram power

// ------------------------------------------------------------------------
// Small state

static int		nsamps;										// baseband samples collected
static int		init_done;

// Front end state
static float	lo_c[DECIM1], lo_s[DECIM1];					// 1500 Hz LO, 8 sample period
static float	taps1[NTAPS], taps2[NTAPS];					// decimator coefficients
static float	d1_i[NTAPS], d1_q[NTAPS];					// stage 1 delay line
static float	d2_i[NTAPS], d2_q[NTAPS];					// stage 2 delay line
static int		d1_idx, d2_idx;								// ring write positions
static int		in_count, s1_count;							// decimation phase counters

// FFT
static float	fft_re[WSPR_FFT_SIZE], fft_im[WSPR_FFT_SIZE];
static float	tw_c[WSPR_FFT_SIZE / 2], tw_s[WSPR_FFT_SIZE / 2];
static float	hann[WSPR_FFT_SIZE];

// Candidate search
static float	psavg[WSPR_FFT_SIZE];
static float	smspec[2 * SEARCH_BINS + 1];

typedef struct
{
	int		bin;											// signed bin off center
	float	snr_db;
	float	freq_bb;										// refined baseband freq, Hz
	float	drift;											// Hz over transmission
	int		shift;											// symbol 0 start sample
	float	sync;											// sync quality 0..1
} wspr_cand;

static wspr_cand	cand[WSPR_MAX_CAND];
static int			ncand;

// Fano metric table
static int		mettab[2][256];

// WSPR pseudo random sync vector (one bit per channel symbol)
// Not static - shared with the encoder (wspr_encoder.c)
const uint8_t wspr_pr3[WSPR_NSYM] =
{
	1,1,0,0,0,0,0,0,1,0,0,0,1,1,1,0,0,0,1,0,
	0,1,0,1,1,1,1,0,0,0,0,0,0,0,1,0,0,1,0,1,
	0,0,0,0,0,0,1,0,1,1,0,0,1,1,0,1,0,0,0,1,
	1,0,1,0,0,0,0,1,1,0,1,0,1,0,1,0,1,0,0,1,
	0,0,1,0,1,1,0,0,0,1,1,0,1,0,1,0,0,0,1,0,
	0,0,0,0,1,0,0,1,0,0,1,1,1,0,1,1,0,0,1,1,
	0,1,0,0,0,1,1,1,0,0,0,0,0,1,0,1,0,0,1,1,
	0,0,0,0,0,0,0,1,1,0,1,0,1,1,0,0,0,1,1,0,
	0,0
};

//*----------------------------------------------------------------------------
//* Function Name       : make_lowpass
//* Object              : Hamming windowed sinc lowpass, unity DC gain
//* Context    			: CONTEXT_WSPR (init)
//*----------------------------------------------------------------------------
static void make_lowpass(float *taps, int ntaps, float cutoff_hz, float fs_hz)
{
	int		n;
	float	sum = 0.0f;
	float	fc = cutoff_hz / fs_hz;
	float	mid = (ntaps - 1) / 2.0f;

	for(n = 0; n < ntaps; n++)
	{
		float x = (float)n - mid;
		float sinc;

		if(fabsf(x) < 1e-6f)
			sinc = 2.0f * fc;
		else
			sinc = sinf(2.0f * (float)M_PI * fc * x) / ((float)M_PI * x);

		taps[n] = sinc * (0.54f - 0.46f * cosf(2.0f * (float)M_PI * n / (ntaps - 1)));
		sum += taps[n];
	}

	for(n = 0; n < ntaps; n++)
		taps[n] /= sum;
}

//*----------------------------------------------------------------------------
//* Function Name       : wspr_init_once
//* Object              : build LO/filter/FFT/metric tables on first use
//* Context    			: CONTEXT_WSPR
//*----------------------------------------------------------------------------
static void wspr_init_once(void)
{
	int i;

	if(init_done)
		return;

	// 1500 Hz LO at 12 kHz repeats every 8 samples
	for(i = 0; i < DECIM1; i++)
	{
		lo_c[i] = cosf(2.0f * (float)M_PI * WSPR_CENTER_HZ * i / WSPR_FS_IN);
		lo_s[i] = sinf(2.0f * (float)M_PI * WSPR_CENTER_HZ * i / WSPR_FS_IN);
	}

	// Decimation filters
	make_lowpass(taps1, NTAPS, 250.0f, (float)WSPR_FS_IN);
	make_lowpass(taps2, NTAPS, 150.0f, (float)WSPR_FS_IN / DECIM1);

	// FFT twiddles and window
	for(i = 0; i < WSPR_FFT_SIZE / 2; i++)
	{
		tw_c[i] = cosf(2.0f * (float)M_PI * i / WSPR_FFT_SIZE);
		tw_s[i] = sinf(2.0f * (float)M_PI * i / WSPR_FFT_SIZE);
	}
	for(i = 0; i < WSPR_FFT_SIZE; i++)
		hann[i] = 0.5f - 0.5f * cosf(2.0f * (float)M_PI * i / (WSPR_FFT_SIZE - 1));

	// Fano metric table from soft symbol byte treated as P(bit=1)
	// scaled log2 likelihood with a small positive rate bias
	for(i = 0; i < 256; i++)
	{
		float p1 = (i + 0.5f) / 256.0f;
		int m1 = (int)lroundf(10.0f * (log2f(2.0f * p1) - 0.45f));
		int m0 = (int)lroundf(10.0f * (log2f(2.0f * (1.0f - p1)) - 0.45f));

		if(m1 < -120) m1 = -120;
		if(m0 < -120) m0 = -120;

		mettab[1][i] = m1;
		mettab[0][i] = m0;
	}

	init_done = 1;
}

//*----------------------------------------------------------------------------
//* Function Name       : wspr_decoder_reset
//* Object              : prepare for a new capture
//* Context    			: CONTEXT_WSPR
//*----------------------------------------------------------------------------
void wspr_decoder_reset(void)
{
	wspr_init_once();

	nsamps   = 0;
	d1_idx   = 0;
	d2_idx   = 0;
	in_count = 0;
	s1_count = 0;

	memset(d1_i, 0, sizeof(d1_i));
	memset(d1_q, 0, sizeof(d1_q));
	memset(d2_i, 0, sizeof(d2_i));
	memset(d2_q, 0, sizeof(d2_q));
}

//*----------------------------------------------------------------------------
//* Function Name       : fir_dot
//* Object              : dot product of taps with ring buffer history
//* Notes    			: idx is the position of the newest sample
//* Context    			: CONTEXT_WSPR
//*----------------------------------------------------------------------------
static float fir_dot(const float *taps, const float *ring, int idx)
{
	float	acc = 0.0f;
	int		j;

	for(j = 0; j < NTAPS; j++)
		acc += taps[j] * ring[(idx - j) & (NTAPS - 1)];

	return acc;
}

//*----------------------------------------------------------------------------
//* Function Name       : wspr_decoder_feed
//* Object              : mix to baseband and decimate 12000 -> 375 Hz
//* Notes    			: can be called repeatedly with arbitrary chunk sizes
//* Context    			: CONTEXT_WSPR
//*----------------------------------------------------------------------------
int wspr_decoder_feed(const int16_t *pcm, int num_samples)
{
	int n, accepted = 0;

	wspr_init_once();

	for(n = 0; n < num_samples; n++)
	{
		float x = (float)pcm[n] / 32768.0f;
		int   ph = in_count & (DECIM1 - 1);

		// Complex mix down by 1500 Hz: x * e^(-j*w*n)
		d1_i[d1_idx] =  x * lo_c[ph];
		d1_q[d1_idx] = -x * lo_s[ph];

		in_count++;

		if((in_count & (DECIM1 - 1)) == 0)
		{
			// One stage 1 output (1500 Hz rate)
			d2_i[d2_idx] = fir_dot(taps1, d1_i, d1_idx);
			d2_q[d2_idx] = fir_dot(taps1, d1_q, d1_idx);

			s1_count++;

			if((s1_count & (DECIM2 - 1)) == 0)
			{
				// One stage 2 output (375 Hz rate)
				if(nsamps < WSPR_MAX_BB_SAMPLES)
				{
					bb_i[nsamps] = fir_dot(taps2, d2_i, d2_idx);
					bb_q[nsamps] = fir_dot(taps2, d2_q, d2_idx);
					nsamps++;
					accepted += DECIM1 * DECIM2;
				}
			}

			d2_idx = (d2_idx + 1) & (NTAPS - 1);
		}

		d1_idx = (d1_idx + 1) & (NTAPS - 1);
	}

	return accepted;
}

//*----------------------------------------------------------------------------
//* Function Name       : fft_512
//* Object              : in place radix 2 complex FFT of fft_re/fft_im
//* Context    			: CONTEXT_WSPR
//*----------------------------------------------------------------------------
static void fft_512(void)
{
	int i, j, k, len;

	// Bit reverse permutation
	for(i = 1, j = 0; i < WSPR_FFT_SIZE; i++)
	{
		int bit = WSPR_FFT_SIZE >> 1;
		for(; j & bit; bit >>= 1)
			j ^= bit;
		j ^= bit;

		if(i < j)
		{
			float t;
			t = fft_re[i]; fft_re[i] = fft_re[j]; fft_re[j] = t;
			t = fft_im[i]; fft_im[i] = fft_im[j]; fft_im[j] = t;
		}
	}

	// Butterflies
	for(len = 2; len <= WSPR_FFT_SIZE; len <<= 1)
	{
		int half = len >> 1;
		int step = WSPR_FFT_SIZE / len;

		for(i = 0; i < WSPR_FFT_SIZE; i += len)
		{
			for(j = 0; j < half; j++)
			{
				float wc =  tw_c[j * step];
				float ws = -tw_s[j * step];			// forward transform
				float ur, ui, vr, vi;

				k  = i + j;
				ur = fft_re[k];
				ui = fft_im[k];
				vr = fft_re[k + half] * wc - fft_im[k + half] * ws;
				vi = fft_re[k + half] * ws + fft_im[k + half] * wc;

				fft_re[k]        = ur + vr;
				fft_im[k]        = ui + vi;
				fft_re[k + half] = ur - vr;
				fft_im[k + half] = ui - vi;
			}
		}
	}
}

//*----------------------------------------------------------------------------
//* Function Name       : compute_spectrogram
//* Object              : windowed FFT power every 128 baseband samples
//* Notes    			: returns number of frames
//* Context    			: CONTEXT_WSPR
//*----------------------------------------------------------------------------
static int compute_spectrogram(void)
{
	int frame, i;
	int nframes = (nsamps - WSPR_FFT_SIZE) / WSPR_FFT_STEP + 1;

	if(nframes > WSPR_MAX_FRAMES)
		nframes = WSPR_MAX_FRAMES;

	for(i = 0; i < WSPR_FFT_SIZE; i++)
		psavg[i] = 0.0f;

	for(frame = 0; frame < nframes; frame++)
	{
		const float *pi = &bb_i[frame * WSPR_FFT_STEP];
		const float *pq = &bb_q[frame * WSPR_FFT_STEP];

		for(i = 0; i < WSPR_FFT_SIZE; i++)
		{
			fft_re[i] = pi[i] * hann[i];
			fft_im[i] = pq[i] * hann[i];
		}

		fft_512();

		for(i = 0; i < WSPR_FFT_SIZE; i++)
		{
			float p = fft_re[i] * fft_re[i] + fft_im[i] * fft_im[i];
			ps[frame][i] = p;
			psavg[i] += p;
		}
	}

	return nframes;
}

//*----------------------------------------------------------------------------
//* Function Name       : find_candidates
//* Object              : peak search over noise-normalized smoothed spectrum
//* Context    			: CONTEXT_WSPR
//*----------------------------------------------------------------------------
static void find_candidates(void)
{
	int		i, j, m;
	float	sorted[2 * SEARCH_BINS + 1];
	float	noise;
	float	min_snr = powf(10.0f, -8.0f / 10.0f);
	int		nbins = 2 * SEARCH_BINS + 1;

	// 7 bin smoothing, matched to the ~6 Hz signal bandwidth
	for(j = 0; j < nbins; j++)
	{
		int bin = j - SEARCH_BINS;					// signed bin off center
		float s = 0.0f;

		for(m = -3; m <= 3; m++)
			s += psavg[(bin + m) & (WSPR_FFT_SIZE - 1)];

		smspec[j] = s;
	}

	// Noise reference: 30th percentile (robust against several signals)
	memcpy(sorted, smspec, sizeof(sorted));
	for(i = 1; i < nbins; i++)
	{
		float v = sorted[i];
		for(j = i - 1; (j >= 0) && (sorted[j] > v); j--)
			sorted[j + 1] = sorted[j];
		sorted[j + 1] = v;
	}
	noise = sorted[(3 * nbins) / 10];
	if(noise <= 0.0f)
		noise = 1e-30f;

	for(j = 0; j < nbins; j++)
	{
		smspec[j] = smspec[j] / noise - 1.0f;
		if(smspec[j] < min_snr)
			smspec[j] = 0.1f * min_snr;
	}

	// Local maxima above the floor
	ncand = 0;
	for(j = 1; j < nbins - 1; j++)
	{
		float snr;

		if((smspec[j] <= smspec[j - 1]) || (smspec[j] < smspec[j + 1]))
			continue;

		snr = 10.0f * log10f(smspec[j]) - SNR_SCALING_DB;
		if(snr < SNR_FLOOR_DB)
			continue;

		if(ncand < WSPR_MAX_CAND)
		{
			cand[ncand].bin    = j - SEARCH_BINS;
			cand[ncand].snr_db = snr;
			ncand++;
		}
		else
		{
			// Replace the weakest kept candidate
			int weakest = 0;
			for(i = 1; i < WSPR_MAX_CAND; i++)
				if(cand[i].snr_db < cand[weakest].snr_db)
					weakest = i;
			if(snr > cand[weakest].snr_db)
			{
				cand[weakest].bin    = j - SEARCH_BINS;
				cand[weakest].snr_db = snr;
			}
		}
	}

	// Strongest first
	for(i = 1; i < ncand; i++)
	{
		wspr_cand c = cand[i];
		for(j = i - 1; (j >= 0) && (cand[j].snr_db < c.snr_db); j--)
			cand[j + 1] = cand[j];
		cand[j + 1] = c;
	}
}

//*----------------------------------------------------------------------------
//* Function Name       : coarse_sync
//* Object              : best time lag and drift from spectrogram correlation
//* Notes    			: tone spacing is exactly 2 bins, tones at -3,-1,+1,+3
//* Context    			: CONTEXT_WSPR
//*----------------------------------------------------------------------------
static void coarse_sync(wspr_cand *c, int nframes)
{
	int		lag, dstep, k;
	float	best = -1e30f;

	c->shift = 0;
	c->drift = 0.0f;
	c->sync  = 0.0f;

	for(dstep = -8; dstep <= 8; dstep++)
	{
		float drift = 0.5f * dstep;

		for(lag = -2; lag <= 17; lag++)
		{
			float ss = 0.0f, pow = 0.0f;

			for(k = 0; k < WSPR_NSYM; k++)
			{
				int fr = lag + 2 * k;
				int cb, boff;
				float p0, p1, p2, p3;

				if((fr < 0) || (fr >= nframes))
					continue;

				boff = (int)lroundf(drift * ((float)k - 80.5f) / 161.0f / BIN_HZ);
				cb   = c->bin + boff;

				p0 = ps[fr][(cb - 3) & (WSPR_FFT_SIZE - 1)];
				p1 = ps[fr][(cb - 1) & (WSPR_FFT_SIZE - 1)];
				p2 = ps[fr][(cb + 1) & (WSPR_FFT_SIZE - 1)];
				p3 = ps[fr][(cb + 3) & (WSPR_FFT_SIZE - 1)];

				ss  += (wspr_pr3[k] ? 1.0f : -1.0f) * ((p1 + p3) - (p0 + p2));
				pow += p0 + p1 + p2 + p3;
			}

			if(pow > 0.0f)
				ss /= pow;

			if(ss > best)
			{
				best     = ss;
				c->shift = lag * WSPR_FFT_STEP;
				c->drift = drift;
			}
		}
	}

	c->sync    = best;
	c->freq_bb = (float)c->bin * BIN_HZ;
}

//*----------------------------------------------------------------------------
//* Function Name       : demod_pass
//* Object              : coherent 4-FSK correlation over all 162 symbols
//* Notes    			: returns power-normalized sync correlation;
//*						: if soft != NULL also outputs soft data metrics
//* Context    			: CONTEXT_WSPR
//*----------------------------------------------------------------------------
static float demod_pass(float fbb, float drift, int shift, float *soft)
{
	int		k, t, n;
	float	ss = 0.0f, ptot = 0.0f;

	for(k = 0; k < WSPR_NSYM; k++)
	{
		int		start = shift + k * WSPR_SPS;
		float	p[4];

		if((start < 0) || (start + WSPR_SPS > nsamps))
		{
			if(soft != NULL)
				soft[k] = 0.0f;
			continue;
		}

		for(t = 0; t < 4; t++)
		{
			float f  = fbb + ((float)t - 1.5f) * WSPR_DF
			               + drift * ((float)k - 80.5f) / 161.0f;
			float w  = 2.0f * (float)M_PI * f / (float)WSPR_FS_BB;
			float dc = cosf(w), ds = sinf(w);
			float cr = 1.0f, ci = 0.0f;
			float re = 0.0f, im = 0.0f;
			const float *xi = &bb_i[start];
			const float *xq = &bb_q[start];

			for(n = 0; n < WSPR_SPS; n++)
			{
				// Correlate with e^(-j*w*n)
				float nc;

				re += xi[n] * cr + xq[n] * ci;
				im += xq[n] * cr - xi[n] * ci;

				nc = cr * dc - ci * ds;
				ci = cr * ds + ci * dc;
				cr = nc;
			}

			p[t] = re * re + im * im;
		}

		ss   += (wspr_pr3[k] ? 1.0f : -1.0f) * ((p[1] + p[3]) - (p[0] + p[2]));
		ptot += p[0] + p[1] + p[2] + p[3];

		if(soft != NULL)
			soft[k] = (p[2] + p[3]) - (p[0] + p[1]);
	}

	if(ptot > 0.0f)
		ss /= ptot;

	return ss;
}

//*----------------------------------------------------------------------------
//* Function Name       : fine_sync
//* Object              : refine shift, freq and drift around coarse estimate
//* Notes    			: returns 0 if candidate kept, 1 if rejected as unsynced
//* Context    			: CONTEXT_WSPR
//*----------------------------------------------------------------------------
static int fine_sync(wspr_cand *c)
{
	int		i;
	float	m, best;
	int		best_shift;
	float	best_f, best_d;

	// Time, coarse step (16 samples = 43 ms)
	best = -1e30f;
	best_shift = c->shift;
	for(i = -4; i <= 4; i++)
	{
		m = demod_pass(c->freq_bb, c->drift, c->shift + 16 * i, NULL);
		if(m > best) { best = m; best_shift = c->shift + 16 * i; }
	}
	c->shift = best_shift;

	// Early reject of noise candidates
	if(best < MIN_SYNC)
		return 1;

	// Frequency, 0.2 Hz step
	best = -1e30f;
	best_f = c->freq_bb;
	for(i = -3; i <= 3; i++)
	{
		float f = c->freq_bb + 0.2f * i;
		m = demod_pass(f, c->drift, c->shift, NULL);
		if(m > best) { best = m; best_f = f; }
	}
	c->freq_bb = best_f;

	// Drift, 0.25 Hz step
	best = -1e30f;
	best_d = c->drift;
	for(i = -3; i <= 3; i++)
	{
		float d = c->drift + 0.25f * i;
		m = demod_pass(c->freq_bb, d, c->shift, NULL);
		if(m > best) { best = m; best_d = d; }
	}
	c->drift = best_d;

	// Time, fine step (4 samples = 11 ms)
	best = -1e30f;
	best_shift = c->shift;
	for(i = -3; i <= 3; i++)
	{
		m = demod_pass(c->freq_bb, c->drift, c->shift + 4 * i, NULL);
		if(m > best) { best = m; best_shift = c->shift + 4 * i; }
	}
	c->shift = best_shift;

	// Frequency, fine step (0.067 Hz)
	best = -1e30f;
	best_f = c->freq_bb;
	for(i = -3; i <= 3; i++)
	{
		float f = c->freq_bb + 0.0667f * i;
		m = demod_pass(f, c->drift, c->shift, NULL);
		if(m > best) { best = m; best_f = f; }
	}
	c->freq_bb = best_f;
	c->sync    = best;

	return (best < MIN_SYNC) ? 1 : 0;
}

//*----------------------------------------------------------------------------
//* Function Name       : deinterleave
//* Object              : channel order -> coded bit order (bit reversal)
//* Context    			: CONTEXT_WSPR
//*----------------------------------------------------------------------------
static void deinterleave(uint8_t *sym)
{
	uint8_t tmp[WSPR_NSYM];
	int		p = 0, i;

	for(i = 0; p < WSPR_NSYM; i++)
	{
		// 8 bit reverse of i
		uint32_t u = (uint32_t)i;
		int j = (int)(((((u * 0x0802u) & 0x22110u) | ((u * 0x8020u) & 0x88440u)) * 0x10101u >> 16) & 0xffu);

		if(j < WSPR_NSYM)
		{
			tmp[p] = sym[j];
			p++;
		}
	}

	memcpy(sym, tmp, WSPR_NSYM);
}

//*----------------------------------------------------------------------------
//* Function Name       : unpack_message
//* Object              : 50 bit payload -> "CALL GRID dBm" (type 1 only)
//* Notes    			: returns 0 ok, 1 invalid/unsupported message type
//* Context    			: CONTEXT_WSPR
//*----------------------------------------------------------------------------
static int unpack_message(const uint8_t *dat, WSPR_DECODE *d)
{
	static const char cmap[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ ";
	int32_t	n1, n2, ngrid;
	int		ntype, i, nu;
	char	tmp[7];
	int		dlat, dlong, nlong, nlat, g1, g2;

	// Reassemble the two packed fields, 28 + 22 bits
	n1 = ((int32_t)dat[0] << 20) | ((int32_t)dat[1] << 12) |
	     ((int32_t)dat[2] << 4)  | ((int32_t)dat[3] >> 4);
	n2 = (((int32_t)dat[3] & 15) << 18) | ((int32_t)dat[4] << 10) |
	     ((int32_t)dat[5] << 2)  | ((int32_t)dat[6] >> 6);

	// Message type from the power field
	ntype = (n2 & 127) - 64;
	if((ntype < 0) || (ntype > 62))
		return 1;									// type 3 / invalid

	nu = ntype % 10;
	if((nu != 0) && (nu != 3) && (nu != 7))
		return 1;									// type 2 (compound call)

	// Callsign, 6 characters
	if(n1 >= 262177560)
		return 1;

	tmp[6] = 0;
	tmp[5] = cmap[n1 % 27 + 10]; n1 /= 27;
	tmp[4] = cmap[n1 % 27 + 10]; n1 /= 27;
	tmp[3] = cmap[n1 % 27 + 10]; n1 /= 27;
	tmp[2] = cmap[n1 % 10];      n1 /= 10;
	tmp[1] = cmap[n1 % 36];      n1 /= 36;
	if(n1 > 36)
		return 1;
	tmp[0] = cmap[n1];

	// Strip leading and trailing spaces
	for(i = 0; (i < 5) && (tmp[i] == ' '); i++)
		;
	strncpy(d->call, &tmp[i], sizeof(d->call) - 1);
	d->call[sizeof(d->call) - 1] = 0;
	for(i = (int)strlen(d->call) - 1; (i >= 0) && (d->call[i] == ' '); i--)
		d->call[i] = 0;

	if(d->call[0] == 0)
		return 1;

	// Grid locator, 4 characters
	ngrid = n2 >> 7;
	if(ngrid >= 32400)
		return 1;

	dlat  = (ngrid % 180) - 90;
	dlong = (ngrid / 180) * 2 - 180 + 2;
	if(dlong < -180) dlong += 360;
	if(dlong >  180) dlong += 360;

	nlong = (int)(60.0f * (180.0f - dlong) / 5.0f);
	g1 = nlong / 240;
	g2 = (nlong - 240 * g1) / 24;
	d->grid[0] = cmap[10 + g1];
	d->grid[2] = cmap[g2];

	nlat = (int)(60.0f * (dlat + 90) / 2.5f);
	g1 = nlat / 240;
	g2 = (nlat - 240 * g1) / 24;
	d->grid[1] = cmap[10 + g1];
	d->grid[3] = cmap[g2];
	d->grid[4] = 0;

	if((d->grid[0] > 'R') || (d->grid[1] > 'R'))
		return 1;

	d->dbm = ntype;

	snprintf(d->message, sizeof(d->message), "%s %s %d", d->call, d->grid, d->dbm);

	return 0;
}

//*----------------------------------------------------------------------------
//* Function Name       : wspr_decoder_run_raw
//* Object              : full decode pass over the fed capture, raw bits out
//* Notes    			: returns number of unique raw 50 bit payloads in out[],
//* Notes    			: no interpretation - callers unpack per personality
//* Context    			: CONTEXT_WSPR
//*----------------------------------------------------------------------------
int wspr_decoder_run_raw(WSPR_RAW_DECODE *out, int max_out)
{
	int		nframes, ic, k, i;
	int		ndecodes = 0;
	float	soft[WSPR_NSYM];
	uint8_t	symbols[WSPR_NSYM];
	uint8_t	data[(WSPR_NBITS + 7) / 8];

	wspr_init_once();

	// Need at least the full transmission (110.6 s)
	if(nsamps < WSPR_NSYM * WSPR_SPS)
		return 0;

	nframes = compute_spectrogram();
	find_candidates();

	for(ic = 0; (ic < ncand) && (ndecodes < max_out); ic++)
	{
		wspr_cand		*c = &cand[ic];
		WSPR_RAW_DECODE	raw;
		float			rms, scale;
		int				dup;

		coarse_sync(c, nframes);

		if(fine_sync(c) != 0)
			continue;

		// Final demodulation with soft symbol output
		demod_pass(c->freq_bb, c->drift, c->shift, soft);

		// Normalize soft metrics to bytes around 128
		rms = 0.0f;
		for(k = 0; k < WSPR_NSYM; k++)
			rms += soft[k] * soft[k];
		rms = sqrtf(rms / WSPR_NSYM);
		if(rms <= 0.0f)
			continue;
		scale = 128.0f / rms;

		for(k = 0; k < WSPR_NSYM; k++)
		{
			float v = 128.0f + soft[k] * scale;
			if(v < 0.0f)   v = 0.0f;
			if(v > 255.0f) v = 255.0f;
			symbols[k] = (uint8_t)v;
		}

		deinterleave(symbols);

		if(wspr_fano(symbols, data, WSPR_NBITS, mettab, FANO_DELTA, FANO_MAXCYCLES) != 0)
			continue;

		// Canonical raw payload - 50 bits, tail bits in the last byte masked
		memset(&raw, 0, sizeof(raw));
		memcpy(raw.bits, data, sizeof(raw.bits));
		raw.bits[6] &= 0xC0;

		raw.freq_hz  = WSPR_CENTER_HZ + c->freq_bb;
		raw.snr_db   = c->snr_db;
		raw.dt_sec   = (float)c->shift / WSPR_FS_BB - 1.0f;
		raw.drift_hz = c->drift;

		// Drop duplicates (same signal found via two near candidates)
		dup = 0;
		for(i = 0; i < ndecodes; i++)
		{
			if(memcmp(out[i].bits, raw.bits, sizeof(raw.bits)) == 0)
			{
				dup = 1;
				break;
			}
		}
		if(dup)
			continue;

		out[ndecodes++] = raw;
	}

	return ndecodes;
}

//*----------------------------------------------------------------------------
//* Function Name       : wspr_raw_to_type1
//* Object              : interpret one raw decode as a WSPR type 1 message
//* Notes    			: returns 0 ok, 1 not valid type 1 (the payload may
//* Notes    			: belong to another personality, e.g. MarsChat)
//* Context    			: CONTEXT_WSPR
//*----------------------------------------------------------------------------
int wspr_raw_to_type1(const WSPR_RAW_DECODE *raw, WSPR_DECODE *dec)
{
	memset(dec, 0, sizeof(*dec));

	if(unpack_message(raw->bits, dec) != 0)
		return 1;

	dec->freq_hz  = raw->freq_hz;
	dec->snr_db   = raw->snr_db;
	dec->dt_sec   = raw->dt_sec;
	dec->drift_hz = raw->drift_hz;

	return 0;
}

//*----------------------------------------------------------------------------
//* Function Name       : wspr_decoder_run
//* Object              : full decode pass over the fed capture, type 1 spots
//* Notes    			: thin wrapper - raw pass + type 1 message unpack;
//* Notes    			: raw payloads that are not valid type 1 are dropped
//* Context    			: CONTEXT_WSPR
//*----------------------------------------------------------------------------
int wspr_decoder_run(WSPR_DECODE *out, int max_out)
{
	// Single caller (wspr task) - keep the raw list off the task stack
	static WSPR_RAW_DECODE	raw[WSPR_MAX_DECODES];
	int						nraw, i;
	int						ndecodes = 0;

	nraw = wspr_decoder_run_raw(raw, WSPR_MAX_DECODES);

	for(i = 0; (i < nraw) && (ndecodes < max_out); i++)
	{
		if(wspr_raw_to_type1(&raw[i], &out[ndecodes]) == 0)
			ndecodes++;
	}

	return ndecodes;
}
