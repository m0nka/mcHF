/************************************************************************************
**                                                                                 **
**                                 mcHF QRP Transceiver                            **
**                         Krassi Atanassov - M0NKA, 2013-2026                     **
**                                                                                 **
**---------------------------------------------------------------------------------**
**                                                                                 **
**  File name:		ft8_proc.c                                                     **
**  Description:	FT8 decoder process                                            **
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

#include "mchf_icc_def.h"

#include "ft8_decoder.h"
#include "ft8_proc.h"

#include "hf_app.h"

// FreeRTOS process state
extern struct PROC_STATE	ps;

// Public radio state - dial frequency for the decode log
extern struct TRANSCEIVER_STATE_UI	tsu;

// Bench layout of the HF app arena (proc/hf_app), overlaid on the WSPR
// buffers - all NULL while the arena is not held:
//
//	ft8_mag		reference waterfall of the slot, from the .wf file (~167 KB)
//	ft8_m4wf	the same slot as the M4 front end made it from the .pcm
//	ft8_pcm		the slot audio, 16 bit LE mono @ 12 kHz (up to 15 s)
#define FT8_WF_AREA					((FT8_WF_BYTES + 31) & ~31UL)
#define FT8_PCM_MAX					(15UL * FT8_SAMPLE_RATE * 2UL)
#define FT8_ARENA_NEED				(2UL * FT8_WF_AREA + FT8_PCM_MAX)

static uint8_t				*ft8_mag  = NULL;
static uint8_t				*ft8_m4wf = NULL;
static uint8_t				*ft8_pcm  = NULL;

// M4 waterfall stream - shared with the icc task (ft8_stream_xxx below)
static volatile uchar		ft8_st_want   = 0;		// ft8 task: stream wanted
static volatile uchar		ft8_st_source = ICC_FT8_SRC_INJECT;
static volatile uchar		ft8_st_on     = 0;		// icc task: M4 acknowledged
static volatile uchar		ft8_st_ovr    = 0;		// M4 reported lost data
static volatile ushort		ft8_st_rows   = 0;		// rows landed
static volatile ushort		ft8_st_order  = 0;		// rows out of sequence
static uint8_t * volatile	ft8_st_dst    = NULL;	// where rows land
static const uint8_t		*ft8_inj_pcm  = NULL;	// bench source
static volatile ulong		ft8_inj_len   = 0;		// samples
static volatile ulong		ft8_inj_pos   = 0;

// SD card read chunk in AXI ram, copied into SDRAM by the CPU. Same reason
// as the WSPR bounce buffer: no SDMMC DMA to or from the FMC bus
__attribute__((section(".axi_mem"))) __attribute__ ((aligned (32))) \
static uint8_t				ft8_sd_bounce[4096];

static FT8_DECODE_RESULT	ft8_res[FT8_MAX_DECODES];
static ftx_waterfall_t		ft8_wf;

// Software extended DWT cycle counter. CYCCNT wraps every ~8.9 s at 480 MHz,
// the decoder samples it far more often than that
static uint32_t				ft8_cyc_last;
static uint32_t				ft8_cyc_rem;
static uint32_t				ft8_us;

// Bench line buffer - target snprintf is common/print_f.c, no floats
static char					ft8_line[128];

//*----------------------------------------------------------------------------
//* Function Name       : ft8_clock_us
//* Object              : microsecond clock for the decoder stage timing
//* Context    			: CONTEXT_FT8
//*----------------------------------------------------------------------------
static uint32_t ft8_clock_us(void)
{
	uint32_t	now = DWT->CYCCNT;
	uint32_t	mhz = SystemCoreClock / 1000000;

	ft8_cyc_rem  += now - ft8_cyc_last;
	ft8_cyc_last  = now;

	ft8_us       += ft8_cyc_rem / mhz;
	ft8_cyc_rem  %= mhz;

	return ft8_us;
}

//*----------------------------------------------------------------------------
//* Function Name       : ft8_clock_init
//* Object              : make sure the DWT cycle counter runs
//* Notes    			: does not reset CYCCNT, cpu_trace may own it too
//* Context    			: CONTEXT_FT8
//*----------------------------------------------------------------------------
static void ft8_clock_init(void)
{
	CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
	DWT->LAR = 0xC5ACCE55;
	DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;

	ft8_cyc_last = DWT->CYCCNT;
	ft8_cyc_rem  = 0;
	ft8_us       = 0;
}

//*----------------------------------------------------------------------------
//* Function Name       : ft8_log
//* Object              : bench output to the debug UART and the log file
//* Context    			: CONTEXT_FT8
//*----------------------------------------------------------------------------
static void ft8_log(FIL *log, uchar log_ok, const char *line)
{
	UINT	bw;

	printf("%s", line);

	if(log_ok)
		f_write(log, line, strlen(line), &bw);
}

//*----------------------------------------------------------------------------
//* Function Name       : ft8_load_wf
//* Object              : read a .wf file into the slot waterfall
//* Notes    			: returns the block count, negative on error
//* Context    			: CONTEXT_FT8
//*----------------------------------------------------------------------------
static int ft8_load_wf(const char *path)
{
	FIL				file;
	FT8_WF_HEADER	hdr;
	UINT			br;
	ulong			len, pos = 0, chunk;
	int				ret = -1;

	if(f_open(&file, path, FA_READ) != FR_OK)
		return -1;

	if((f_read(&file, ft8_sd_bounce, sizeof(hdr), &br) != FR_OK) || (br != sizeof(hdr)))
		goto done;

	memcpy(&hdr, ft8_sd_bounce, sizeof(hdr));

	// Geometry must match what the decoder is built for
	ret = -2;
	if((hdr.magic != FT8_WF_MAGIC) || (hdr.num_bins != FT8_NUM_BINS) || (hdr.min_bin != FT8_MIN_BIN) ||
	   (hdr.time_osr != FT8_TIME_OSR) || (hdr.freq_osr != FT8_FREQ_OSR) ||
	   (hdr.num_blocks > FT8_MAX_BLOCKS) || (hdr.hdr_size != sizeof(hdr)))
		goto done;

	ret = -3;
	len = (ulong)hdr.num_blocks * FT8_TIME_OSR * FT8_FREQ_OSR * FT8_NUM_BINS;
	while(pos < len)
	{
		chunk = len - pos;
		if(chunk > sizeof(ft8_sd_bounce))
			chunk = sizeof(ft8_sd_bounce);

		if((f_read(&file, ft8_sd_bounce, chunk, &br) != FR_OK) || (br != chunk))
			goto done;

		memcpy(ft8_mag + pos, ft8_sd_bounce, chunk);
		pos += chunk;
	}

	ret = hdr.num_blocks;

done:
	f_close(&file);
	return ret;
}

//*----------------------------------------------------------------------------
//* Function Name       : ft8_score_ref
//* Object              : count WSJT-X reference decodes we also found
//* Notes    			: reference .txt lines look like
//*						: "110130  -6  0.7  683 ~  CQ TA6CQ KN70      AS Turkey"
//*						: - message after '~', annotations after 2+ spaces.
//*						: FatFS is built without f_gets, the whole file
//*						: (~1.5 KB) is read into the bounce buffer instead
//* Context    			: CONTEXT_FT8
//*----------------------------------------------------------------------------
static void ft8_score_ref(const char *wf_path, int n_res, int *refs, int *hits)
{
	FIL		file;
	UINT	br;
	char	path[64], ref[FTX_MAX_MESSAGE_LENGTH];
	char	*p, *txt = (char *)ft8_sd_bounce;
	int		i, n;

	*refs = -1;
	*hits = 0;

	n = strlen(wf_path);
	if((n < 3) || (n >= (int)sizeof(path)))
		return;

	strcpy(path, wf_path);
	strcpy(path + n - 3, ".txt");

	if(f_open(&file, path, FA_READ) != FR_OK)
		return;

	br = 0;
	f_read(&file, txt, sizeof(ft8_sd_bounce) - 1, &br);
	f_close(&file);
	txt[br] = 0;

	*refs = 0;
	for(p = txt; (p = strchr(p, '~')) != NULL; )
	{
		p++;
		while(*p == ' ')
			p++;

		n = 0;
		while(*p && (*p != '\r') && (*p != '\n') && (n < (int)sizeof(ref) - 1))
		{
			if((p[0] == ' ') && (p[1] == ' '))
				break;
			ref[n++] = *p++;
		}
		ref[n] = 0;

		if(n == 0)
			continue;

		(*refs)++;
		for(i = 0; i < n_res; i++)
		{
			if(strcmp(ft8_res[i].text, ref) == 0)
			{
				(*hits)++;
				break;
			}
		}
	}
}

//*----------------------------------------------------------------------------
//* Function Name       : ft8_proc_ram_put
//* Object              : hand the HF app arena back
//* Context    			: CONTEXT_FT8
//*----------------------------------------------------------------------------
static void ft8_proc_ram_put(void)
{
	ft8_st_dst = NULL;
	ft8_mag    = NULL;
	ft8_m4wf   = NULL;
	ft8_pcm    = NULL;

	hf_app_ram_release(HF_APP_FT8);
}

//*----------------------------------------------------------------------------
//* Function Name       : ft8_stream_want/mark/inject_next/row
//* Object              : M4 waterfall stream, icc task side - see ft8_proc.h
//* Context    			: CONTEXT_ICC
//*----------------------------------------------------------------------------
uchar ft8_stream_want(uchar *source)
{
	*source = ft8_st_source;
	return ft8_st_want;
}

void ft8_stream_mark(uchar on)
{
	ft8_st_on = on;
}

ushort ft8_stream_inject_next(uchar *dst, ushort max_samples)
{
	ulong	n = ft8_inj_len - ft8_inj_pos;

	if((ft8_inj_pcm == NULL) || (n == 0))
		return 0;

	if(n > max_samples)
		n = max_samples;

	memcpy(dst, ft8_inj_pcm + ft8_inj_pos * 2, n * 2);
	ft8_inj_pos += n;

	return (ushort)n;
}

void ft8_stream_row(ushort seq, const uchar *row, ushort len, uchar flags)
{
	uint8_t	*dst = ft8_st_dst;

	if(flags & ICC_FT8_FLAG_OVERRUN)
		ft8_st_ovr = 1;

	if((dst == NULL) || (len != ICC_FT8_ROW_BYTES))
		return;

	// Row n is waterfall bytes [n * 898 ..], a slot holds 2 rows per block
	if(seq >= (FT8_MAX_BLOCKS * FT8_TIME_OSR))
		return;

	if(seq != ft8_st_rows)
		ft8_st_order++;

	memcpy(dst + (ulong)seq * ICC_FT8_ROW_BYTES, row, len);
	ft8_st_rows++;
}

//*----------------------------------------------------------------------------
//* Function Name       : ft8_stream_kick
//* Object              : wake the icc task to service the stream
//* Context    			: CONTEXT_FT8
//*----------------------------------------------------------------------------
static void ft8_stream_kick(void)
{
	if(ps.hIccTask != NULL)
		xTaskNotify(ps.hIccTask, UI_ICC_FT8, eSetValueWithOverwrite);
}

//*----------------------------------------------------------------------------
//* Function Name       : ft8_stream_begin
//* Object              : ask for an M4 stream, rows land in 'dst'
//* Context    			: CONTEXT_FT8
//*----------------------------------------------------------------------------
static void ft8_stream_begin(uchar source, uint8_t *dst)
{
	ft8_st_dst    = dst;
	ft8_st_rows   = 0;
	ft8_st_order  = 0;
	ft8_st_ovr    = 0;
	ft8_st_source = source;
	ft8_st_want   = 1;

	ft8_stream_kick();
}

//*----------------------------------------------------------------------------
//* Function Name       : ft8_stream_end
//* Object              : stop the stream, wait for the icc task to confirm
//*						: the M4 side is down (a restart must not find it
//*						: still running with the previous slot's frame)
//* Context    			: CONTEXT_FT8
//*----------------------------------------------------------------------------
static void ft8_stream_end(void)
{
	TickType_t	t0 = xTaskGetTickCount();

	ft8_st_want = 0;

	while(ft8_st_on && ((xTaskGetTickCount() - t0) < 2000))
	{
		ft8_stream_kick();
		vTaskDelay(5);
	}

	ft8_st_dst = NULL;
}

//*----------------------------------------------------------------------------
//* Function Name       : ft8_stream_run
//* Object              : drive one M4 stream and wait until 'rows' rows
//*						: landed in 'dst' (or timeout), then stop it
//* Notes    			: returns the rows received
//* Context    			: CONTEXT_FT8
//*----------------------------------------------------------------------------
static int ft8_stream_run(uchar source, uint8_t *dst, int rows, ulong timeout_ms)
{
	TickType_t	t0 = xTaskGetTickCount(), kick;

	ft8_stream_begin(source, dst);
	kick = xTaskGetTickCount();

	while((ft8_st_rows < rows) && ((xTaskGetTickCount() - t0) < timeout_ms))
	{
		// Again every 200 ms - the notification value can be overwritten
		// by another icc command
		if((xTaskGetTickCount() - kick) >= 200)
		{
			kick = xTaskGetTickCount();
			ft8_stream_kick();
		}

		vTaskDelay(10);
	}

	ft8_stream_end();

	return ft8_st_rows;
}

//*----------------------------------------------------------------------------
//* Function Name       : ft8_load_pcm
//* Object              : read NAME.pcm next to a .wf file into ft8_pcm
//* Notes    			: returns the sample count, 0 if there is none
//* Context    			: CONTEXT_FT8
//*----------------------------------------------------------------------------
static ulong ft8_load_pcm(const char *wf_path)
{
	FIL		file;
	UINT	br;
	char	path[64];
	ulong	len, pos = 0, chunk;
	int		n = strlen(wf_path);

	if((n < 3) || (n + 2 >= (int)sizeof(path)))
		return 0;

	strcpy(path, wf_path);
	strcpy(path + n - 3, ".pcm");

	if(f_open(&file, path, FA_READ) != FR_OK)
		return 0;

	len = f_size(&file);
	if(len > FT8_PCM_MAX)
		len = FT8_PCM_MAX;

	while(pos < len)
	{
		chunk = len - pos;
		if(chunk > sizeof(ft8_sd_bounce))
			chunk = sizeof(ft8_sd_bounce);

		if((f_read(&file, ft8_sd_bounce, chunk, &br) != FR_OK) || (br != chunk))
		{
			len = 0;
			break;
		}

		memcpy(ft8_pcm + pos, ft8_sd_bounce, chunk);
		pos += chunk;
	}

	f_close(&file);
	return len / 2;
}

//*----------------------------------------------------------------------------
//* Function Name       : ft8_bench_m4
//* Object              : WP3 - push the slot audio through the M4 front end,
//*						: compare its waterfall with the reference one and
//*						: decode it
//* Notes    			: ft8_mag holds the reference waterfall ('blocks')
//* Context    			: CONTEXT_FT8
//*----------------------------------------------------------------------------
static void ft8_bench_m4(const char *path, int blocks, FIL *log, uchar log_ok,
						 int *m4_dec, int *m4_hits, ulong *m4_diff)
{
	ulong				samples, i, same = 0, near = 0, bytes;
	int					rows, want, d, dmax = 0, n, refs, hits;
	uint32_t			us;
	FT8_DECODE_STATS	st;

	*m4_dec  = -1;
	*m4_hits = 0;
	*m4_diff = 0;

	samples = ft8_load_pcm(path);
	if(samples == 0)
		return;

	// Whole rows the audio makes, capped at what the reference holds
	want = (int)(samples / (FT8_SAMPLE_RATE * FT8_SYMBOL_PERIOD / FT8_TIME_OSR));
	if(want > blocks * FT8_TIME_OSR)
		want = blocks * FT8_TIME_OSR;

	memset(ft8_m4wf, 0, FT8_WF_BYTES);

	ft8_inj_pcm = ft8_pcm;
	ft8_inj_len = samples;
	ft8_inj_pos = 0;

	us   = ft8_clock_us();
	rows = ft8_stream_run(ICC_FT8_SRC_INJECT, ft8_m4wf, want, 20000);
	us   = ft8_clock_us() - us;

	ft8_inj_pcm = NULL;

	// Byte by byte against the reference - the host builds it in double
	// precision, the M4 in single, so +-1 (0.5 dB) at a rounding edge is
	// expected, anything more is a real difference
	bytes = (ulong)(rows / FT8_TIME_OSR) * FT8_TIME_OSR * ICC_FT8_ROW_BYTES;
	for(i = 0; i < bytes; i++)
	{
		d = abs((int)ft8_m4wf[i] - (int)ft8_mag[i]);

		if(d == 0)
			same++;
		else if(d == 1)
			near++;

		if(d > dmax)
			dmax = d;
	}

	*m4_diff = bytes - same - near;

	// Decode the M4 waterfall (the hash table is shared with the reference
	// decode just before, as it would be slot to slot)
	ft8_decoder_wf_init(&ft8_wf, ft8_m4wf, rows / FT8_TIME_OSR);
	n = ft8_decoder_run(&ft8_wf, ft8_res, FT8_MAX_DECODES, &st);

	ft8_score_ref(path, n, &refs, &hits);

	*m4_dec  = st.decodes;
	*m4_hits = hits;

	// Percentages in tenths - no float in the target printf
	snprintf(ft8_line, sizeof(ft8_line), "   m4: %d/%d rows%s%s, same %d.%d%% +-1 %d.%d%% worse %u max %d \r\n",
			rows, want, ft8_st_ovr ? " OVR" : "", ft8_st_order ? " ORD" : "",
			bytes ? (int)(same * 1000 / bytes) / 10 : 0, bytes ? (int)(same * 1000 / bytes) % 10 : 0,
			bytes ? (int)(near * 1000 / bytes) / 10 : 0, bytes ? (int)(near * 1000 / bytes) % 10 : 0,
			(uint)*m4_diff, dmax);
	ft8_log(log, log_ok, ft8_line);

	snprintf(ft8_line, sizeof(ft8_line), "   m4: %d dec, %d/%d ref (stream %d ms) \r\n",
			st.decodes, hits, refs, (int)(us / 1000));
	ft8_log(log, log_ok, ft8_line);
}

//*----------------------------------------------------------------------------
//* Function Name       : ft8_proc_bench
//* Object              : decode every .wf file in FT8_BENCH_DIR, report
//*						: per stage timing and the score against WSJT-X
//* Context    			: CONTEXT_FT8
//*----------------------------------------------------------------------------
static void ft8_proc_bench(void)
{
	DIR					dir;
	FILINFO				fno;
	FIL					log;
	uchar				log_ok;
	char				path[64];
	int					i, n, blocks, refs, hits;
	int					files = 0, tot_dec = 0, tot_refs = 0, tot_hits = 0;
	int					m4_files = 0, m4_dec, m4_hits, m4_tot_dec = 0, m4_tot_hits = 0, m4_tot_refs = 0;
	ulong				m4_diff, m4_tot_diff = 0;
	uint32_t			us_sum = 0, us_worst = 0, us_load;
	FT8_DECODE_STATS	st;
	uint8_t				*arena;

	// Waits out a WSPR cycle still winding down after the mode switch
	arena = (uint8_t *)hf_app_ram_acquire(HF_APP_FT8, FT8_ARENA_NEED, 5000);
	if(arena == NULL)
	{
		printf("ft8: hf arena not available, no bench \r\n");
		return;
	}

	ft8_mag  = arena;
	ft8_m4wf = arena + FT8_WF_AREA;
	ft8_pcm  = arena + 2 * FT8_WF_AREA;

	if(f_opendir(&dir, FT8_BENCH_DIR) != FR_OK)
	{
		printf("ft8: no %s on the sd card \r\n", FT8_BENCH_DIR);
		ft8_proc_ram_put();
		return;
	}

	log_ok = (f_open(&log, FT8_BENCH_LOG, FA_WRITE | FA_CREATE_ALWAYS) == FR_OK);

	snprintf(ft8_line, sizeof(ft8_line), "ft8: bench start, core %d MHz \r\n", (int)(SystemCoreClock / 1000000));
	ft8_log(&log, log_ok, ft8_line);

	for(;;)
	{
		if((f_readdir(&dir, &fno) != FR_OK) || (fno.fname[0] == 0))
			break;

		// Left the FT8 screen - hand HF back to the new owner
		if(!hf_app_allowed(HF_APP_FT8))
		{
			ft8_log(&log, log_ok, "ft8: hf owner changed, bench stopped \r\n");
			break;
		}

		n = strlen(fno.fname);
		if((n < 4) || (strcmp(fno.fname + n - 3, ".wf") != 0))
			continue;

		if((strlen(FT8_BENCH_DIR) + 1 + n) >= sizeof(path))
			continue;

		strcpy(path, FT8_BENCH_DIR);
		strcat(path, "/");
		strcat(path, fno.fname);

		us_load = ft8_clock_us();
		blocks  = ft8_load_wf(path);
		us_load = ft8_clock_us() - us_load;

		if(blocks < 0)
		{
			snprintf(ft8_line, sizeof(ft8_line), "ft8: %s load err %d \r\n", fno.fname, blocks);
			ft8_log(&log, log_ok, ft8_line);
			continue;
		}

		ft8_decoder_wf_init(&ft8_wf, ft8_mag, blocks);
		n = ft8_decoder_run(&ft8_wf, ft8_res, FT8_MAX_DECODES, &st);

		ft8_score_ref(path, n, &refs, &hits);

		snprintf(ft8_line, sizeof(ft8_line), "== %s: %d dec, %d/%d ref, %d cand %d fail %d dup \r\n",
				fno.fname, st.decodes, hits, refs, st.candidates, st.ldpc_fail, st.duplicates);
		ft8_log(&log, log_ok, ft8_line);

		snprintf(ft8_line, sizeof(ft8_line), "   ms: sync %d ldpc %d unpack %d total %d (sd load %d) \r\n",
				(int)(st.us_sync / 1000), (int)(st.us_ldpc / 1000), (int)(st.us_unpack / 1000),
				(int)(st.us_total / 1000), (int)(us_load / 1000));
		ft8_log(&log, log_ok, ft8_line);

		for(i = 0; i < n; i++)
		{
			// No '-' flag and no '+' in the target printf - sign by hand
			snprintf(ft8_line, sizeof(ft8_line), "   %c%d %c%d.%d %4u  %s \r\n",
					(ft8_res[i].snr < 0) ? '-' : '+', abs(ft8_res[i].snr),
					(ft8_res[i].dt_10 < 0) ? '-' : '+', abs(ft8_res[i].dt_10) / 10, abs(ft8_res[i].dt_10) % 10,
					ft8_res[i].freq_hz, ft8_res[i].text);
			ft8_log(&log, log_ok, ft8_line);
		}

		files++;
		tot_dec += st.decodes;
		us_sum  += st.us_total;
		if(st.us_total > us_worst)
			us_worst = st.us_total;

		if(refs > 0)
		{
			tot_refs += refs;
			tot_hits += hits;
		}

		// WP3 - the same slot through the M4 front end, when NAME.pcm exists
		ft8_bench_m4(path, blocks, &log, log_ok, &m4_dec, &m4_hits, &m4_diff);
		if(m4_dec >= 0)
		{
			m4_files++;
			m4_tot_dec  += m4_dec;
			m4_tot_diff += m4_diff;

			if(refs > 0)
			{
				m4_tot_refs += refs;
				m4_tot_hits += m4_hits;
			}
		}

		// Let the rest of the system breathe between slots
		vTaskDelay(10);
	}

	f_closedir(&dir);

	if(files)
	{
		snprintf(ft8_line, sizeof(ft8_line), "ft8: %d files, %d decodes, %d of %d refs, avg %d ms, worst %d ms \r\n",
				files, tot_dec, tot_hits, tot_refs, (int)(us_sum / files / 1000), (int)(us_worst / 1000));
		ft8_log(&log, log_ok, ft8_line);
	}
	else
		ft8_log(&log, log_ok, "ft8: no .wf files \r\n");

	if(m4_files)
	{
		snprintf(ft8_line, sizeof(ft8_line), "ft8: m4 %d files, %d decodes, %d of %d refs, %u bytes off by more than 1 \r\n",
				m4_files, m4_tot_dec, m4_tot_hits, m4_tot_refs, (uint)m4_tot_diff);
		ft8_log(&log, log_ok, ft8_line);
	}

	if(log_ok)
		f_close(&log);

	ft8_proc_ram_put();
}

// ------------------------------------------------------------------------
// Live receiver (WP5)
//
// While the FT8 screen is up the task holds the HF app arena and runs one
// capture per 15 s UTC slot: the M4 streams rows from the live rx tap from
// the slot boundary on, straight into the waterfall; at FT8_LIVE_ROWS the
// stream stops and the slot is decoded, well before the next boundary.

// 170 rows = 85 blocks = 13.6 s: a 12.64 s signal starting up to +1.0 s
// late is complete (WSJT-X transmits from +0.5 s)
#define FT8_LIVE_ROWS				170

// A slot is only started this close to its boundary - later than that
// every DT would be off, better to skip it
#define FT8_LIVE_LATE_MS			400

// Poll interval while live (slot boundary resolution)
#define FT8_LIVE_POLL_MS			20

// Slot clock correction from the decodes themselves (the WSJT-X operator's
// "watch the DT column", automated). The RTC is left alone - GPS, WSPR and
// the clock display keep reading it as it is - only FT8's idea of where a
// slot starts moves. Soaks up the NMEA latency of the GPS time set (the
// RTC second lands when the sentence is processed, not on the true UTC
// second), drift between GPS syncs, and is the only time keeping a unit
// without GPS has once its clock is roughly right
#define FT8_DT_MIN_DECODES			2		// slots with fewer teach nothing
#define FT8_DT_DEADBAND_MS			80		// median |DT| below this is left alone
#define FT8_DT_GAIN_PCT				70		// damping, one odd slot can not throw it
#define FT8_DT_MAX_OFS_MS			7500	// half a slot - beyond, the sign is ambiguous

static int					ft8_clock_ofs_ms = 0;	// added to the RTC for slot timing

// Transmitter. The frame goes to the M4 this far into our slot, so that
// after the exciter keys the tones start close to the nominal +0.5 s
// (what WSJT-X calls DT 0). Frames sent before the watchdog disarms
#define FT8_TX_START_MS				350
#define FT8_TX_WATCHDOG				6

static uint8_t				ft8_tx_tones[FT8_TONES];
static volatile uchar		ft8_tx_pending = 0;		// frame staged for the icc task
static volatile uchar		ft8_tx_abort   = 0;		// cut the frame on the air
static uchar				ft8_tx_sent    = 0;		// frame of this slot handed over

static void ft8_live_tx_slot(void);

// Waterfall display line ring (ft8_proc.h), rendered here and painted by
// the screen: one line per FT8 symbol (160 ms). In the arena above
// everything the bench uses, so a bench run leaves it be
#define FT8_DISP_OFS				(704UL * 1024UL)
#define FT8_DISP_BYTES				(FT8_DISP_W * FT8_DISP_RING)
#define FT8_LIVE_NEED				(FT8_DISP_OFS + FT8_DISP_BYTES)

static uint8_t				*ft8_disp       = NULL;
static volatile ulong		ft8_disp_gen    = 0;		// lines written
static int					ft8_disp_block  = 0;	// next block of the slot to render

static volatile uchar		ft8_live_req  = 0;		// screen wants live rx
static uchar				ft8_live_held = 0;		// task holds the arena
static FT8_LIVE_STATUS		ft8_live;
static long					ft8_live_slot = -1;		// slot number last started
static TickType_t			ft8_live_t0;
static char					ft8_live_time[8];		// "hhmmss" of that slot
static uchar				ft8_live_noarena_said = 0;

// Decode history for the screen - the task writes, the gui task copies
static FT8_DECODE			ft8_hist[FT8_HISTORY];
static int					ft8_hist_n = 0;
static volatile ulong		ft8_hist_gen = 0;

//*----------------------------------------------------------------------------
//* Function Name       : ft8_live_slot_pos
//* Object              : where we are in the 15 s slot, from the RTC
//* Notes    			: returns ms into the slot, *slot = slot number in
//*						: the hour, *hhmmss = slot start time text
//* Context    			: CONTEXT_FT8
//*----------------------------------------------------------------------------
static int ft8_live_slot_pos(long *slot, char *hhmmss)
{
	RTC_TimeTypeDef	tm = {0};
	RTC_DateTypeDef	dt = {0};
	long			day_ms, start;
	int				ms = 0;

	// Time then date - the shadow registers unlock on the date read
	k_GetTime(&tm);
	k_GetDate(&dt);

	if(tm.SecondFraction)
		ms = (int)(((tm.SecondFraction - tm.SubSeconds) * 1000UL) / (tm.SecondFraction + 1));

	// Milliseconds into the UTC day, as FT8 sees it (RTC + correction)
	day_ms  = ((((long)tm.Hours * 60) + tm.Minutes) * 60 + tm.Seconds) * 1000L + ms;
	day_ms += ft8_clock_ofs_ms;
	day_ms  = (day_ms + 86400000L) % 86400000L;

	*slot = day_ms / 15000L;
	start = (*slot * 15000L) / 1000L;			// slot start, seconds into the day

	if(hhmmss != NULL)
		snprintf(hhmmss, 8, "%02d%02d%02d", (int)(start / 3600), (int)((start / 60) % 60), (int)(start % 60));

	return (int)(day_ms % 15000L);
}

//*----------------------------------------------------------------------------
//* Function Name       : ft8_disp_next
//* Object              : ring slot for the next line - published by
//*						: ft8_disp_done() once it is filled
//* Context    			: CONTEXT_FT8
//*----------------------------------------------------------------------------
static uint8_t *ft8_disp_next(void)
{
	return ft8_disp + (ulong)FT8_DISP_SLOT(ft8_disp_gen) * FT8_DISP_W;
}

static void ft8_disp_done(void)
{
	ft8_disp_gen++;
}

//*----------------------------------------------------------------------------
//* Function Name       : ft8_disp_block_line
//* Object              : one waterfall block (both time halves) -> one
//*						: display line, 200..3000 Hz across FT8_DISP_W
//* Notes    			: contrast follows the line's own mean level - 0.5 dB
//*						: per palette step from 2 dB under the mean, so the
//*						: 64 colours span 32 dB of signal above the noise
//* Context    			: CONTEXT_FT8
//*----------------------------------------------------------------------------
static void ft8_disp_block_line(int block)
{
	const uint8_t	*r0 = ft8_mag + (ulong)block * FT8_TIME_OSR * ICC_FT8_ROW_BYTES;
	const uint8_t	*r1 = r0 + ICC_FT8_ROW_BYTES;
	uint8_t			*line;
	ulong			sum = 0;
	int				x, j, v, floor;

	for(j = 0; j < ICC_FT8_ROW_BYTES; j++)
		sum += r0[j];

	floor = (int)(sum / ICC_FT8_ROW_BYTES) - 4;

	line = ft8_disp_next();

	for(x = 0; x < FT8_DISP_W; x++)
	{
		// Fine bin (3.125 Hz) under this pixel: bin j/2 of freq_sub j%2
		j = (x * (FT8_NUM_BINS * FT8_FREQ_OSR)) / FT8_DISP_W;
		j = ((j & 1) * FT8_NUM_BINS) + (j >> 1);

		// Loudest of the two time halves, so short tones do not flicker
		v = (r0[j] > r1[j]) ? r0[j] : r1[j];
		v = v - floor;

		line[x] = (uint8_t)((v < 0) ? 0 : ((v > 63) ? 63 : v));
	}

	ft8_disp_done();
}

//*----------------------------------------------------------------------------
//* Function Name       : ft8_disp_catch_up
//* Object              : render every block of the slot that is complete
//* Context    			: CONTEXT_FT8
//*----------------------------------------------------------------------------
static void ft8_disp_catch_up(void)
{
	if(ft8_disp == NULL)
		return;

	while(((ft8_disp_block + 1) * FT8_TIME_OSR) <= ft8_st_rows)
		ft8_disp_block_line(ft8_disp_block++);
}

//*----------------------------------------------------------------------------
//* Function Name       : ft8_disp_slot_mark
//* Object              : dashed line where a new slot starts
//* Context    			: CONTEXT_FT8
//*----------------------------------------------------------------------------
static void ft8_disp_slot_mark(void)
{
	uint8_t	*line;
	int		x;

	ft8_disp_block = 0;

	if(ft8_disp == NULL)
		return;

	line = ft8_disp_next();

	for(x = 0; x < FT8_DISP_W; x++)
		line[x] = (x & 4) ? 40 : 0;

	ft8_disp_done();
}

//*----------------------------------------------------------------------------
//* Function Name       : ft8_live_publish
//* Object              : slot decodes -> screen history and the SD log
//* Context    			: CONTEXT_FT8
//*----------------------------------------------------------------------------
static void ft8_live_publish(int n)
{
	FT8_DECODE		d;
	FIL				file;
	UINT			bw;
	uchar			log_ok;
	ulong			dial = tsu.band[tsu.curr_band].active_vfo == VFO_A ?
						   tsu.band[tsu.curr_band].vfo_a : tsu.band[tsu.curr_band].vfo_b;
	int				i, dt;

	log_ok = (f_open(&file, FT8_DECODES_FILE, FA_WRITE | FA_OPEN_APPEND) == FR_OK);

	for(i = 0; i < n; i++)
	{
		memset(&d, 0, sizeof(d));
		memcpy(d.time, ft8_live_time, sizeof(d.time));

		// ft8_lib gives the start time from the slot start, WSJT-X shows
		// it relative to the nominal +0.5 s
		dt     = ft8_res[i].dt_10 * 10 - 50;
		d.dt   = (int16_t)dt;
		d.snr  = (int8_t)((ft8_res[i].snr > 99) ? 99 : ((ft8_res[i].snr < -99) ? -99 : ft8_res[i].snr));
		d.freq = ft8_res[i].freq_hz;
		strncpy(d.msg, ft8_res[i].text, FT8_MSG_LEN - 1);

		// History, oldest dropped
		vTaskSuspendAll();
		if(ft8_hist_n == FT8_HISTORY)
		{
			memmove(ft8_hist, ft8_hist + 1, (FT8_HISTORY - 1) * sizeof(FT8_DECODE));
			ft8_hist_n--;
		}
		ft8_hist[ft8_hist_n++] = d;
		xTaskResumeAll();

		// WSJT-X ALL.TXT-ish: time, dial, snr, dt, audio, message
		if(log_ok)
		{
			snprintf(ft8_line, sizeof(ft8_line), "%s %u %c%d %c%d.%d %4u ~ %s\r\n",
					d.time, (uint)dial,
					(d.snr < 0) ? '-' : '+', abs(d.snr),
					(dt < 0) ? '-' : '+', abs(dt) / 100, (abs(dt) / 10) % 10,
					d.freq, d.msg);
			f_write(&file, ft8_line, strlen(ft8_line), &bw);
		}
	}

	if(log_ok)
		f_close(&file);

	ft8_hist_gen++;
}

//*----------------------------------------------------------------------------
//* Function Name       : ft8_live_track_dt
//* Object              : nudge the slot clock correction by the median DT
//*						: of this slot's decodes
//* Notes    			: DT here is WSJT-X style (0 = nominal +0.5 s start).
//*						: Positive DT = signals late in our window = we
//*						: started early = FT8's clock runs ahead of UTC, so
//*						: the correction moves the other way
//* Context    			: CONTEXT_FT8
//*----------------------------------------------------------------------------
static void ft8_live_track_dt(int n)
{
	int		dt[FT8_MAX_DECODES];
	int		i, j, t, med, step;

	if(n < FT8_DT_MIN_DECODES)
		return;

	for(i = 0; i < n; i++)
		dt[i] = ft8_res[i].dt_10 * 100 - 500;		// ms, WSJT-X style

	// Insertion sort, n is small
	for(i = 1; i < n; i++)
	{
		t = dt[i];
		for(j = i; (j > 0) && (dt[j - 1] > t); j--)
			dt[j] = dt[j - 1];
		dt[j] = t;
	}

	med = (n & 1) ? dt[n / 2] : (dt[n / 2 - 1] + dt[n / 2]) / 2;

	if(abs(med) < FT8_DT_DEADBAND_MS)
		return;

	step = (med * FT8_DT_GAIN_PCT) / 100;

	ft8_clock_ofs_ms -= step;

	if(ft8_clock_ofs_ms > FT8_DT_MAX_OFS_MS)
		ft8_clock_ofs_ms = FT8_DT_MAX_OFS_MS;
	else if(ft8_clock_ofs_ms < -FT8_DT_MAX_OFS_MS)
		ft8_clock_ofs_ms = -FT8_DT_MAX_OFS_MS;
}

//*----------------------------------------------------------------------------
//* Function Name       : ft8_live_decode
//* Object              : end of the capture - stop the stream, decode
//* Context    			: CONTEXT_FT8
//*----------------------------------------------------------------------------
static void ft8_live_decode(void)
{
	FT8_DECODE_STATS	st;
	int					rows, n;

	ft8_stream_end();

	rows = ft8_st_rows;
	if(rows > FT8_LIVE_ROWS)
		rows = FT8_LIVE_ROWS;

	ft8_disp_catch_up();

	ft8_live.state     = FT8_LIVE_DECODE;
	ft8_live.rows_lost = (ushort)(FT8_LIVE_ROWS - rows + ft8_st_order);
	memcpy(ft8_live.last_time, ft8_live_time, sizeof(ft8_live.last_time));

	ft8_decoder_wf_init(&ft8_wf, ft8_mag, rows / FT8_TIME_OSR);
	n = ft8_decoder_run(&ft8_wf, ft8_res, FT8_MAX_DECODES, &st);

	ft8_live.last_count = (uchar)n;
	ft8_live.last_ms    = (ushort)(st.us_total / 1000);
	ft8_live.slots++;

	ft8_live_track_dt(n);
	ft8_live.clock_ofs_ms = (short)ft8_clock_ofs_ms;

	printf("ft8: %s %d dec, %d rows%s, cand %d, %d ms, clk %d ms \r\n",
			ft8_live_time, n, rows, ft8_st_ovr ? " OVR" : "", st.candidates, ft8_live.last_ms,
			ft8_clock_ofs_ms);

	ft8_live_publish(n);

	ft8_live.state = FT8_LIVE_WAIT;
}

//*----------------------------------------------------------------------------
//* Function Name       : ft8_live_release
//* Object              : stop whatever runs and give the arena back
//* Context    			: CONTEXT_FT8
//*----------------------------------------------------------------------------
static void ft8_live_release(void)
{
	if(!ft8_live_held)
		return;

	ft8_stream_end();

	// Leaving FT8 - a frame on the air is cut off, nothing stays armed
	if(ft8_live.tx_armed || (ft8_live.state == FT8_LIVE_TX))
		ft8_proc_tx_halt();

	ft8_disp = NULL;
	ft8_proc_ram_put();

	ft8_live_held  = 0;
	ft8_live_slot  = -1;
	ft8_live.state = FT8_LIVE_OFF;
}

//*----------------------------------------------------------------------------
//* Function Name       : ft8_live_service
//* Object              : live receiver state machine, every poll
//* Context    			: CONTEXT_FT8
//*----------------------------------------------------------------------------
static void ft8_live_service(void)
{
	long	slot;
	int		pos;
	uint8_t	*arena;

	if(!ft8_live_req)
	{
		ft8_live_release();
		return;
	}

	// Take the arena for the whole FT8 session - a WSPR cycle may still be
	// winding down right after the mode switch, try again next poll
	if(!ft8_live_held)
	{
		arena = (uint8_t *)hf_app_ram_acquire(HF_APP_FT8, FT8_LIVE_NEED, 0);
		if(arena == NULL)
		{
			ft8_live.state = FT8_LIVE_NO_ARENA;

			if(!ft8_live_noarena_said)
				printf("ft8: live waits for the hf arena \r\n");

			ft8_live_noarena_said = 1;
			return;
		}

		ft8_mag               = arena;
		ft8_disp              = arena + FT8_DISP_OFS;
		ft8_live_held         = 1;

		// Fresh image each session - the arena held WSPR data before
		memset(ft8_disp, 0, FT8_DISP_BYTES);
		ft8_disp_gen = 0;
		ft8_live_noarena_said = 0;
		ft8_live.state        = FT8_LIVE_WAIT;

		printf("ft8: live rx on \r\n");
	}

	switch(ft8_live.state)
	{
		case FT8_LIVE_WAIT:
		{
			pos = ft8_live_slot_pos(&slot, ft8_live_time);

			// A fresh slot, and we are early enough into it
			if((slot != ft8_live_slot) && (pos < FT8_LIVE_LATE_MS))
			{
				ft8_live_slot = slot;
				ft8_live_t0   = xTaskGetTickCount();

				// Ours to transmit in - no capture, we would only hear
				// ourselves
				if(ft8_live.tx_armed && ((slot & 1) == ft8_live.tx_parity))
				{
					ft8_tx_sent    = 0;
					ft8_live.state = FT8_LIVE_TX;

					ft8_disp_slot_mark();
					break;
				}

				// Rows missing from this slot must read as silence, not as
				// the previous slot
				memset(ft8_mag, 0, FT8_WF_BYTES);

				ft8_stream_begin(ICC_FT8_SRC_LIVE, ft8_mag);
				ft8_live.state = FT8_LIVE_CAPTURE;

				ft8_disp_slot_mark();
			}
			break;
		}

		case FT8_LIVE_CAPTURE:
		{
			// Enough rows, or the stream stalled - decode what is there
			if((ft8_st_rows >= FT8_LIVE_ROWS) ||
			   ((xTaskGetTickCount() - ft8_live_t0) > 14000))
				ft8_live_decode();
			else
			{
				ft8_disp_catch_up();

				if(!ft8_st_on)
					ft8_stream_kick();
			}

			break;
		}

		case FT8_LIVE_TX:
			ft8_live_tx_slot();
			break;

		default:
			ft8_live.state = FT8_LIVE_WAIT;
			break;
	}
}

//*----------------------------------------------------------------------------
//* Function Name       : ft8_proc_tx_set / arm / halt
//* Object              : transmitter control from the screen
//* Context    			: any task
//*----------------------------------------------------------------------------
int ft8_proc_tx_set(const char *msg, uchar parity)
{
	uint8_t	tones[FT8_TONES];
	int		rc;

	rc = ft8_decoder_encode(msg, tones);
	if(rc != 0)
	{
		printf("ft8: tx message '%s' does not encode (%d) \r\n", msg, rc);
		return rc;
	}

	// The frame on the air (if any) keeps its own copy on the M4
	memcpy(ft8_tx_tones, tones, sizeof(ft8_tx_tones));
	strncpy(ft8_live.tx_msg, msg, FT8_MSG_LEN - 1);
	ft8_live.tx_msg[FT8_MSG_LEN - 1] = 0;

	ft8_live.tx_parity = parity & 1;
	ft8_live.tx_count  = 0;
	ft8_live.tx_armed  = 1;

	printf("ft8: tx armed, '%s' in %s slots \r\n", ft8_live.tx_msg, ft8_live.tx_parity ? "odd" : "even");
	return 0;
}

void ft8_proc_tx_arm(uchar on)
{
	// Nothing to send yet - arming is CALL CQ's job
	if(on && (ft8_live.tx_msg[0] == 0))
		return;

	if(on)
		ft8_live.tx_count = 0;

	ft8_live.tx_armed = on;
}

void ft8_proc_tx_halt(void)
{
	ft8_live.tx_armed = 0;
	ft8_tx_pending    = 0;
	ft8_tx_abort      = 1;

	ft8_stream_kick();
}

//*----------------------------------------------------------------------------
//* Function Name       : ft8_stream_tx_take / result / abort_take
//* Object              : transmitter hand-over to the icc task
//* Context    			: CONTEXT_ICC
//*----------------------------------------------------------------------------
ushort ft8_stream_tx_take(uchar *buf)
{
	if(!ft8_tx_pending)
		return 0;

	ft8_tx_pending = 0;

	buf[0] = (uchar)(FT8_TX_AUDIO_HZ >> 0);
	buf[1] = (uchar)(FT8_TX_AUDIO_HZ >> 8);
	buf[2] = FT8_TONES;
	memcpy(buf + 3, ft8_tx_tones, FT8_TONES);

	return 3 + FT8_TONES;
}

void ft8_stream_tx_result(uchar rc)
{
	ft8_live.tx_rc = rc;

	if(rc != 0)
		printf("ft8: m4 refused the tx frame (%d) \r\n", rc);
}

uchar ft8_stream_tx_abort_take(void)
{
	if(!ft8_tx_abort)
		return 0;

	ft8_tx_abort = 0;
	return 1;
}

//*----------------------------------------------------------------------------
//* Function Name       : ft8_live_tx_slot
//* Object              : our transmit slot - hand the frame over at
//*						: FT8_TX_START_MS, then wait for the slot to end
//*						: (the M4 unkeys itself after the last symbol)
//* Context    			: CONTEXT_FT8
//*----------------------------------------------------------------------------
static void ft8_live_tx_slot(void)
{
	long	slot;
	int		pos = ft8_live_slot_pos(&slot, NULL);

	// Slot over - back to listening
	if(slot != ft8_live_slot)
	{
		ft8_live.state = FT8_LIVE_WAIT;
		return;
	}

	if((!ft8_tx_sent) && (pos >= FT8_TX_START_MS))
	{
		ft8_tx_sent = 1;

		// Disarmed or halted since the slot began - stay quiet
		if(!ft8_live.tx_armed)
			return;

		ft8_tx_pending = 1;
		ft8_stream_kick();

		ft8_live.tx_count++;
		printf("ft8: %s tx '%s' (%d) \r\n", ft8_live_time, ft8_live.tx_msg, ft8_live.tx_count);

		// Watchdog - nobody answered, stop calling
		if(ft8_live.tx_count >= FT8_TX_WATCHDOG)
		{
			ft8_live.tx_armed = 0;
			printf("ft8: tx watchdog, disarmed \r\n");
		}
	}
}

//*----------------------------------------------------------------------------
//* Function Name       : ft8_proc_live
//* Object              : live reception on/off (FT8 screen entry/exit)
//* Context    			: any task
//*----------------------------------------------------------------------------
void ft8_proc_live(uchar on)
{
	ft8_live_req = on;

	if(ps.hFt8Task != NULL)
		xTaskNotify(ps.hFt8Task, FT8_NOTIFY_LIVE, eSetBits);
}

//*----------------------------------------------------------------------------
//* Function Name       : ft8_proc_get_history
//* Object              : decode history snapshot for the screen
//* Context    			: CONTEXT_VIDEO (gui task)
//*----------------------------------------------------------------------------
int ft8_proc_get_history(FT8_DECODE *out, int max, ulong *gen)
{
	int	n, first;

	vTaskSuspendAll();

	n     = (ft8_hist_n < max) ? ft8_hist_n : max;
	first = ft8_hist_n - n;

	memcpy(out, ft8_hist + first, n * sizeof(FT8_DECODE));

	if(gen != NULL)
		*gen = ft8_hist_gen;

	xTaskResumeAll();

	return n;
}

//*----------------------------------------------------------------------------
//* Function Name       : ft8_proc_display
//* Object              : waterfall display image for the screen, NULL while
//*						: live rx does not hold the arena (off, bench)
//* Context    			: CONTEXT_VIDEO (gui task)
//*----------------------------------------------------------------------------
const uint8_t *ft8_proc_display(ulong *count)
{
	if(count != NULL)
		*count = ft8_disp_gen;

	return ft8_live_held ? ft8_disp : NULL;
}

//*----------------------------------------------------------------------------
//* Function Name       : ft8_proc_get_status
//* Context    			: any task
//*----------------------------------------------------------------------------
void ft8_proc_get_status(FT8_LIVE_STATUS *st)
{
	*st = ft8_live;

	if(!ft8_live_req)
		st->state = FT8_LIVE_OFF;
}

//*----------------------------------------------------------------------------
//* Function Name       : ft8_proc_request_bench
//* Object              : post a bench request to the ft8 task
//* Context    			: any task
//*----------------------------------------------------------------------------
uchar ft8_proc_request_bench(void)
{
	if(ps.hFt8Task == NULL)
		return 1;

	xTaskNotify(ps.hFt8Task, FT8_NOTIFY_BENCH, eSetBits);
	return 0;
}

//*----------------------------------------------------------------------------
//* Function Name       : ft8_proc_task
//* Object              : FT8 decoder process
//* Context    			: CONTEXT_FT8
//*----------------------------------------------------------------------------
void ft8_proc_task(void const *arg)
{
	ulong	ulNotificationValue = 0;

	vTaskDelay(FT8_PROC_START_DELAY);

	ft8_clock_init();
	ft8_decoder_init(ft8_clock_us);

	for(;;)
	{
		// Forever asleep unless live rx wants the slot boundary watched
		TickType_t sleep = ft8_live_req ? pdMS_TO_TICKS(FT8_LIVE_POLL_MS) : FT8_PROC_SLEEP_TIME;

		if(xTaskNotifyWait(0x00, ULONG_MAX, &ulNotificationValue, sleep) == pdTRUE)
		{
			// The bench needs the whole arena - live rx pauses for it and
			// picks up again at the next slot boundary
			if(ulNotificationValue & FT8_NOTIFY_BENCH)
			{
				ft8_live_release();
				ft8_proc_bench();
			}
		}

		ft8_live_service();
	}
}

#endif
