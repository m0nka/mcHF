/************************************************************************************
**                                                                                 **
**                                 mcHF QRP Transceiver                            **
**                         Krassi Atanassov - M0NKA, 2013-2026                     **
**                                                                                 **
**---------------------------------------------------------------------------------**
**                                                                                 **
**  File name:		ft8_proc.h                                                     **
**  Description:	FT8 decoder process                                            **
**  Last Modified:                                                                 **
**  Licence:		https://github.com/m0nka/mcHF/blob/main/LICENSE                **
************************************************************************************/
//
// Decode bench (claude/FT8/PROJECT.md WP2/WP3). On request the task walks
// every waterfall file in FT8_BENCH_DIR and reports on the debug UART and
// to FT8_BENCH_LOG:
//
//	- WP2: decode of the reference waterfall (.wf), per stage timing
//	- WP3: when NAME.pcm sits next to it, the same audio pushed through the
//		   M4 waterfall front end (ICC_FT8_FEED), the result compared byte
//		   by byte against the .wf and decoded
//
// Waterfall and PCM files are made on the PC by claude/FT8/host/wf_gen.py,
// with the WSJT-X reference decode list (.txt) next to each one for scoring.
//
#ifndef __FT8_PROC_H
#define __FT8_PROC_H

#include <stdint.h>

// Task notification bits
#define FT8_NOTIFY_BENCH			0x01
#define FT8_NOTIFY_LIVE				0x02

// SD card locations
#define FT8_BENCH_DIR				"0://ft8"
#define FT8_BENCH_LOG				"0://ft8/bench.log"
#define FT8_DECODES_FILE			"0://ft8/decodes.txt"

// One decode as the screen shows it
#define FT8_MSG_LEN					36

typedef struct
{
	char		time[8];				// slot start, "hhmmss" UTC
	int8_t		snr;					// dB
	int16_t		dt;						// hundredths of a second, WSJT-X style
	uint16_t	freq;					// audio Hz
	char		msg[FT8_MSG_LEN];

} FT8_DECODE;

// Decode history kept for the screen, oldest first
#define FT8_HISTORY					48

// Live receiver state, for the slot bar
#define FT8_LIVE_OFF				0
#define FT8_LIVE_WAIT				1		// armed, waiting for a slot boundary
#define FT8_LIVE_CAPTURE			2		// M4 streaming this slot
#define FT8_LIVE_DECODE				3
#define FT8_LIVE_NO_ARENA			4		// HF RAM held by someone else
#define FT8_LIVE_TX					5		// our transmit slot

typedef struct
{
	uchar		state;					// FT8_LIVE_xxx
	uchar		last_count;				// decodes in the last slot
	ushort		last_ms;				// its decode time
	ushort		rows_lost;				// rows missing in the last slot
	ulong		slots;					// slots decoded since entry
	char		last_time[8];			// "hhmmss" of the last decoded slot
	short		clock_ofs_ms;			// FT8 slot timing correction, see ft8_proc.c

	// Transmitter
	uchar		tx_armed;
	uchar		tx_parity;				// 0 = even slots (:00 :30), 1 = odd
	uchar		tx_count;				// frames sent since armed
	uchar		tx_rc;					// last M4 start result, 0 = ok
	char		tx_msg[FT8_MSG_LEN];

} FT8_LIVE_STATUS;

void	ft8_proc_task(void const *arg);

// Queue a decode bench run over FT8_BENCH_DIR. Callable from any task.
// Live reception pauses for it. Returns 0 if the request was posted
uchar	ft8_proc_request_bench(void);

// Live reception on/off - the FT8 screen on entry/exit. Any task
void	ft8_proc_live(uchar on);

// Snapshot of the decode history (oldest first) - returns the count and
// the history generation, which changes whenever a slot adds decodes
int		ft8_proc_get_history(FT8_DECODE *out, int max, ulong *gen);

void	ft8_proc_get_status(FT8_LIVE_STATUS *st);

// ------------------------------------------------------------------------
// Transmitter. Any task (the FT8 screen)

// Operator identity - fixed until there is a settings home for it
#define FT8_MY_CALL					"M0NKA"
#define FT8_MY_GRID					"IO91"

// Audio offset of our transmission (tone 0), Hz
#define FT8_TX_AUDIO_HZ				1500

// Queue a message for every slot of the given parity and arm. Returns 0
// on success, else the message did not encode and nothing changed
int		ft8_proc_tx_set(const char *msg, uchar parity);

// Arm / disarm without changing the message; disarm lets a frame on the
// air finish, halt cuts it off
void	ft8_proc_tx_arm(uchar on);
void	ft8_proc_tx_halt(void);

// icc task side: a staged ICC_FT8_TX_START payload (returns its length,
// 0 = none) and the M4 answer; abort request for ICC_MC_TX_STOP
ushort	ft8_stream_tx_take(uchar *buf);
void	ft8_stream_tx_result(uchar rc);
uchar	ft8_stream_tx_abort_take(void);

// Waterfall display lines: a ring of FT8_DISP_RING lines of FT8_DISP_W
// pixels, 6 bit palette indices (desktop waterfall_blue[]), 200 Hz on the
// left to 3000 Hz on the right. Line n (n = 0, 1, 2 ... since live rx
// started) sits in ring slot FT8_DISP_SLOT(n) - newer lines at LOWER
// slots, so newest-first runs are contiguous up to the wrap. The ring is
// FT8_DISP_H lines on screen plus slack, so the screen can pace its own
// scrolling a few lines behind the writer. NULL while live rx is off,
// *count = lines written so far
#define FT8_DISP_W					780
#define FT8_DISP_H					96
#define FT8_DISP_RING				(FT8_DISP_H + 16)
#define FT8_DISP_SLOT(n)			((FT8_DISP_RING - 1) - ((n) % FT8_DISP_RING))

const uint8_t	*ft8_proc_display(ulong *count);

// ------------------------------------------------------------------------
// M4 waterfall stream, serviced by the icc task (the only task that talks
// to the M4 core). The ft8 task says what it wants, the icc task drives
// ICC_FT8_START/STOP/FEED/READ to match on every pass, so a lost wake-up
// notification delays the stream but can not strand it

// What the ft8 task wants: 1 = stream running, *source = ICC_FT8_SRC_xxx
uchar	ft8_stream_want(uchar *source);

// The icc task reports the M4 side state (1 = start acknowledged)
void	ft8_stream_mark(uchar on);

// Bench source: next PCM samples to feed, 16 bit LE into dst, returns the
// sample count (0 = all fed)
ushort	ft8_stream_inject_next(uchar *dst, ushort max_samples);

// One waterfall row from the M4 core (ICC_FT8_READ payload)
void	ft8_stream_row(ushort seq, const uchar *row, ushort len, uchar flags);

#endif
