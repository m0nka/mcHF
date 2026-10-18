/************************************************************************************
**                                                                                 **
**                                 mcHF QRP Transceiver                            **
**                         Krassi Atanassov - M0NKA, 2013-2026                     **
**                                                                                 **
**---------------------------------------------------------------------------------**
**                                                                                 **
**  File name:		icc_ft8.c                                                      **
**  Description:	FT8 waterfall front end - ft8_lib monitor.c STFT on the M4,    **
**					rows streamed to the M7 core which runs the decoder            **
**  Licence:		https://github.com/m0nka/mcHF/blob/main/LICENSE                **
************************************************************************************/
//
// A port of ft8_lib common/monitor.c monitor_process() with the reference
// parameters (12 kHz, 200..3000 Hz, time_osr = freq_osr = 2): every 960
// new samples the 3840 sample analysis frame shifts, gets a scaled Hann
// window and a real FFT, and bins 32..480 of both frequency subdivisions
// become one 898 byte row of 0.5 dB steps. The host tool
// claude/FT8/host/wf_gen.py does the same maths and is the reference this
// must match.
//
// 3840 is not a power of two, so CMSIS-DSP arm_rfft_fast_f32 is out -
// this uses kiss_fftr (mixed radix), the same FFT ft8_lib uses. The M4
// build already carries a copy for FreeDV (drivers/freedv), shared here.
//
// Two sources: the live rx tap (irq, same point and 48k -> 12k boxcar as
// the WSPR tap) and ICC_FT8_FEED, which lets the M7 core push a known
// recording through the exact same code for a byte by byte comparison.
//

// Compiled only for the STM32H747 CM4 baseband build
#ifdef H7_M4_CORE

#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <math.h>

#include "icc_ft8.h"
#include "icc_wspr.h"
#include "icc_hf_ram.h"

#include "kiss_fftr.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// Wire format constants, duplicated from common/mchf_icc_def.h to keep
// this translation unit free of the wire protocol header (same approach
// as icc_wspr.c)
#define FT8_SIG						0x9D
#define FT8_FLAG_ACTIVE				0x01
#define FT8_FLAG_OVERRUN			0x02
#define FT8_HDR_SIZE				6
#define FT8_ROW_BYTES				898
#define FT8_FEED_MAX				480
#define FT8_SRC_LIVE				0
#define FT8_SRC_INJECT				1

// STFT geometry (ft8_lib reference, must match proc/ft8/ft8_decoder.h on M7)
#define FT8_BLOCK_SIZE				1920			// 12000 * 0.160
#define FT8_TIME_OSR				2
#define FT8_FREQ_OSR				2
#define FT8_SUBBLOCK				(FT8_BLOCK_SIZE / FT8_TIME_OSR)		// 960
#define FT8_NFFT					(FT8_BLOCK_SIZE * FT8_FREQ_OSR)		// 3840
#define FT8_MIN_BIN					32
#define FT8_NUM_BINS				449

// Live tap decimation 48 kHz -> 12 kHz, as in icc_wspr.c
#define FT8_DECIM_FACTOR			4

// Input ring - 340 ms of slack for the superloop at 12 kHz, power of two
#define FT8_IN_RING					4096

// Row ring - 640 ms of slack for the M7 poll loop (one row per 80 ms)
#define FT8_ROW_RING				8

// kiss_fftr working memory for a 3840 point real FFT: 1920 point complex
// state (15.6 KB) + tmpbuf and super twiddles (23 KB). Checked at start
#define FT8_KISS_MEM				38912

// Safety stop if the M7 core never sends ICC_FT8_STOP (16 s of rows)
#define FT8_MAX_ROWS				200

// Big buffers - in the shared HF digital mode RAM
typedef struct
{
	int16_t			in[FT8_IN_RING];				// audio waiting for the STFT
	int16_t			frame[FT8_NFFT];				// analysis frame, newest last
	float			window[FT8_NFFT / 2 + 1];		// scaled Hann, symmetric half
	kiss_fft_cpx	buf[FT8_NFFT / 2 + 1];			// windowed frame in, spectrum out
	uint8_t			kiss_mem[FT8_KISS_MEM];
	uint8_t			rows[FT8_ROW_RING][FT8_ROW_BYTES];

} icc_ft8_ram_t;

_Static_assert(sizeof(icc_ft8_ram_t) <= ICC_HF_RAM_SIZE, "FT8 front end does not fit the HF RAM");

// Run state - outside the shared RAM, the audio irq checks it
typedef struct
{
	icc_ft8_ram_t		*ram;
	kiss_fftr_cfg		cfg;

	volatile uint8_t	active;
	volatile uint8_t	overrun;
	uint8_t				source;

	// Input ring, free running indices. Producer: audio irq (live) or the
	// feed handler (inject), consumer: the superloop
	volatile uint32_t	in_wr;
	volatile uint32_t	in_rd;

	// Row ring, free running. Producer: STFT, consumer: ICC_FT8_READ
	volatile uint32_t	row_wr;
	volatile uint32_t	row_rd;

	// Live tap decimator
	int32_t				acc;
	uint32_t			phase;

} icc_ft8_state_t;

static icc_ft8_state_t		ift;

//*----------------------------------------------------------------------------
//* Function Name       : icc_ft8_stft
//* Object              : one 960 sample subblock -> one waterfall row
//* Notes    			: monitor_process() for a single time_sub
//* Context    			: CONTEXT_ICC (superloop)
//*----------------------------------------------------------------------------
static void icc_ft8_stft(void)
{
	icc_ft8_ram_t	*r = ift.ram;
	float			*td = (float *)r->buf;			// in place: kiss_fftr reads the
	uint8_t			*row;							// input into its own tmpbuf first
	uint32_t		i, src, bin, fs;
	float			mag2, db;
	int				scaled;

	// Row ring full - the M7 core stopped polling, drop the subblock
	if((ift.row_wr - ift.row_rd) >= FT8_ROW_RING)
	{
		ift.in_rd  += FT8_SUBBLOCK;
		ift.overrun = 1;
		return;
	}

	// Shift the new data into the analysis frame
	memmove(r->frame, r->frame + FT8_SUBBLOCK, (FT8_NFFT - FT8_SUBBLOCK) * sizeof(int16_t));

	for(i = 0; i < FT8_SUBBLOCK; i++)
		r->frame[FT8_NFFT - FT8_SUBBLOCK + i] = r->in[(ift.in_rd + i) & (FT8_IN_RING - 1)];

	ift.in_rd += FT8_SUBBLOCK;

	// Window - same float arithmetic as monitor.c, which scales the
	// samples to +-1.0 first (load_wav divides by 32768)
	for(i = 0; i < FT8_NFFT; i++)
	{
		float w = r->window[(i <= FT8_NFFT / 2) ? i : (FT8_NFFT - i)];

		td[i] = w * ((float)r->frame[i] / 32768.0f);
	}

	kiss_fftr(ift.cfg, td, r->buf);

	// Magnitudes in 0.5 dB steps, 0..240 covers -120..0 dB
	row = r->rows[ift.row_wr % FT8_ROW_RING];

	for(fs = 0; fs < FT8_FREQ_OSR; fs++)
	{
		for(bin = FT8_MIN_BIN; bin < (FT8_MIN_BIN + FT8_NUM_BINS); bin++)
		{
			src    = (bin * FT8_FREQ_OSR) + fs;
			mag2   = (r->buf[src].i * r->buf[src].i) + (r->buf[src].r * r->buf[src].r);
			db     = 10.0f * log10f(1E-12f + mag2);
			scaled = (int)(2 * db + 240);

			*row++ = (scaled < 0) ? 0 : ((scaled > 255) ? 255 : (uint8_t)scaled);
		}
	}

	ift.row_wr++;

	// Safety stop - never stream forever on a lost stop command
	if(ift.row_wr >= FT8_MAX_ROWS)
		ift.active = 0;
}

//*----------------------------------------------------------------------------
//* Function Name       : icc_ft8_process
//* Object              : run the STFT over every complete subblock
//* Context    			: CONTEXT_ICC (superloop)
//*----------------------------------------------------------------------------
static void icc_ft8_process(void)
{
	while(ift.active && ((ift.in_wr - ift.in_rd) >= FT8_SUBBLOCK))
		icc_ft8_stft();
}

//*----------------------------------------------------------------------------
//* Function Name       : icc_ft8_start
//* Object              : begin a new slot - clean frame, empty rings
//* Notes    			: returns 0 on success
//* Context    			: CONTEXT_ICC (superloop)
//*----------------------------------------------------------------------------
uint8_t icc_ft8_start(uint8_t source)
{
	icc_ft8_ram_t	*r = (icc_ft8_ram_t *)icc_hf_ram;
	size_t			len = FT8_KISS_MEM;
	uint32_t		i;

	// Stop the irq tap before anything below moves
	ift.active = 0;

	// The two HF digital mode streams share their RAM - the M7 core never
	// runs both, this is only belt and braces
	icc_wspr_release();

	memset(&ift, 0, sizeof(ift));
	ift.ram    = r;
	ift.source = source;

	memset(r->frame, 0, sizeof(r->frame));

	// monitor.c: window[i] = (2 / nfft) * hann(i, nfft), hann = sin^2(pi i / N)
	for(i = 0; i <= FT8_NFFT / 2; i++)
	{
		float x = sinf((float)M_PI * i / FT8_NFFT);

		r->window[i] = (2.0f / FT8_NFFT) * (x * x);
	}

	ift.cfg = kiss_fftr_alloc(FT8_NFFT, 0, r->kiss_mem, &len);
	if(ift.cfg == NULL)
	{
		printf("ft8: kiss needs %u bytes\r\n", (unsigned int)len);
		return 1;
	}

	ift.active = 1;

	printf("ft8 start, source %d\r\n", source);
	return 0;
}

//*----------------------------------------------------------------------------
//* Function Name       : icc_ft8_stop
//* Object              : end the stream, buffered rows stay readable
//* Context    			: CONTEXT_ICC (superloop)
//*----------------------------------------------------------------------------
void icc_ft8_stop(void)
{
	if(!ift.active)
		return;

	ift.active = 0;

	printf("ft8 stop, %u rows\r\n", (unsigned int)ift.row_wr);
}

//*----------------------------------------------------------------------------
//* Function Name       : icc_ft8_release
//* Object              : stop and drop buffered rows
//* Context    			: CONTEXT_ICC (superloop)
//*----------------------------------------------------------------------------
void icc_ft8_release(void)
{
	icc_ft8_stop();

	ift.ram    = NULL;
	ift.row_rd = ift.row_wr;
}

//*----------------------------------------------------------------------------
//* Function Name       : icc_ft8_feed
//* Object              : bench input - PCM from the M7 core instead of the
//*						: rx tap, processed right away so the reply already
//*						: counts the rows it produced
//* Notes    			: returns the number of rows waiting
//* Context    			: CONTEXT_ICC (superloop)
//*----------------------------------------------------------------------------
uint8_t icc_ft8_feed(const uint8_t *payload)
{
	uint32_t	n, i;

	if((!ift.active) || (ift.source != FT8_SRC_INJECT))
		return 0;

	n = (uint32_t)(payload[0] | (payload[1] << 8));
	if(n > FT8_FEED_MAX)
		n = FT8_FEED_MAX;

	payload += 2;

	for(i = 0; i < n; i++, payload += 2)
	{
		if((ift.in_wr - ift.in_rd) >= FT8_IN_RING)
		{
			ift.overrun = 1;
			break;
		}

		ift.ram->in[ift.in_wr & (FT8_IN_RING - 1)] = (int16_t)(payload[0] | (payload[1] << 8));
		ift.in_wr++;
	}

	icc_ft8_process();

	return (uint8_t)(ift.row_wr - ift.row_rd);
}

//*----------------------------------------------------------------------------
//* Function Name       : icc_ft8_get_row
//* Object              : fill the ICC_FT8_READ response, one row per call
//* Context    			: CONTEXT_ICC (superloop)
//*----------------------------------------------------------------------------
uint16_t icc_ft8_get_row(uint8_t *buffer)
{
	uint16_t	len = 0;
	uint16_t	seq = 0;
	uint8_t		flags = 0;

	if(buffer == NULL)
		return 0;

	// Catch up first, so a poll never misses a subblock that is ready
	icc_ft8_process();

	if(ift.active)
		flags |= FT8_FLAG_ACTIVE;

	if(ift.overrun)
		flags |= FT8_FLAG_OVERRUN;

	if((ift.ram != NULL) && (ift.row_rd != ift.row_wr))
	{
		len = FT8_ROW_BYTES;
		seq = (uint16_t)ift.row_rd;
		memcpy(buffer + FT8_HDR_SIZE, ift.ram->rows[ift.row_rd % FT8_ROW_RING], len);
		ift.row_rd++;
	}

	buffer[0] = FT8_SIG;
	buffer[1] = flags;
	buffer[2] = (uint8_t)(seq >> 0);
	buffer[3] = (uint8_t)(seq >> 8);
	buffer[4] = (uint8_t)(len >> 0);
	buffer[5] = (uint8_t)(len >> 8);

	return (FT8_HDR_SIZE + len);
}

//*----------------------------------------------------------------------------
//* Function Name       : icc_ft8_thread
//* Object              : superloop worker for the live source
//* Context    			: CONTEXT_ICC (superloop)
//*----------------------------------------------------------------------------
void icc_ft8_thread(void)
{
	if(ift.active && (ift.source == FT8_SRC_LIVE))
		icc_ft8_process();
}

//*----------------------------------------------------------------------------
//* Function Name       : icc_ft8_collect
//* Object              : decimate one rx audio block into the input ring
//* Context    			: CONTEXT_IRQ (audio DMA block handler)
//*----------------------------------------------------------------------------
void icc_ft8_collect(volatile int16_t *src, uint32_t num_frames, uint32_t tx_mode)
{
	uint32_t	i;

	if((!ift.active) || (ift.source != FT8_SRC_LIVE))
		return;

	for(i = 0; i < num_frames; i++)
	{
		// RIGHT channel = fixed LINE OUT scaling, see icc_wspr_collect()
		ift.acc += tx_mode ? 0 : (int32_t)(src[1]);
		src     += 2;

		if(++ift.phase < FT8_DECIM_FACTOR)
			continue;

		// Superloop fell behind by more than the ring - drop, flag it
		if((ift.in_wr - ift.in_rd) >= FT8_IN_RING)
			ift.overrun = 1;
		else
		{
			ift.ram->in[ift.in_wr & (FT8_IN_RING - 1)] = (int16_t)(ift.acc / FT8_DECIM_FACTOR);
			ift.in_wr++;
		}

		ift.acc   = 0;
		ift.phase = 0;
	}
}

#endif // H7_M4_CORE
