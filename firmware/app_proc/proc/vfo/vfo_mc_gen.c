/************************************************************************************
**                                                                                 **
**                                 mcHF QRP Transceiver                            **
**                         Krassi Atanassov - M0NKA, 2013-2026                     **
**                                                                                 **
**---------------------------------------------------------------------------------**
**                                                                                 **
**  File name:		vfo_mc_gen.c                                                   **
**  Description:	MarsChat 4-FSK generator on the Si5351 CLK1 test injector      **
**  Last Modified:                                                                 **
**  Licence:		https://github.com/m0nka/mcHF/blob/main/LICENSE                **
************************************************************************************/

#include "mchf_pro_board.h"
#include "main.h"

#ifdef CONTEXT_VFO

#include "si5351.h"
#include "vfo_mc_gen.h"

// FreeRTOS process state
extern struct PROC_STATE	ps;

// Tone offsets from the group center in 0.01 Hz units:
// (s - 1.5) * 1.46484375 Hz, rounded (max error 0.3 cHz, no accumulation)
static const long mc_tone_offset_c[4] = { -220, -73, 73, 220 };

// Max symbols per stream (one WSPR transmission)
#define MC_GEN_MAX_SYMS			162

// Generator state, single instance
static struct
{
	uchar		on;								// stream running (vfo task only)
	uchar		pending;						// armed, first key-up on next proc
	uchar		syms[MC_GEN_MAX_SYMS];
	ushort		nsym;
	ushort		idx;
	ulong		start_tick;
	uint64_t	center_c;						// tone group center, 0.01 Hz units

} mc_gen;

//*----------------------------------------------------------------------------
//* Function Name       : mc_gen_deadline
//* Object              : ms offset of symbol boundary n from stream start
//* Notes    			: n * 8192/12 ms = n * 2048/3 ms, exact every 3 symbols
//* Context    			: CONTEXT_VFO
//*----------------------------------------------------------------------------
static ulong mc_gen_deadline(ulong n)
{
	return ((n * 2048UL) + 1UL) / 3UL;
}

//*----------------------------------------------------------------------------
//* Function Name       : mc_gen_set_tone
//* Object              : program CLK1 for one 4-FSK symbol
//* Context    			: CONTEXT_VFO
//*----------------------------------------------------------------------------
static void mc_gen_set_tone(uchar sym)
{
	Si5351_set_freq(mc_gen.center_c + mc_tone_offset_c[sym & 3], SI5351_CLK1);
}

//*----------------------------------------------------------------------------
//* Function Name       : vfo_mc_gen_init
//* Object              :
//* Context    			: CONTEXT_VFO
//*----------------------------------------------------------------------------
void vfo_mc_gen_init(void)
{
	mc_gen.on		= 0;
	mc_gen.pending	= 0;
	mc_gen.nsym		= 0;
	mc_gen.idx		= 0;
}

//*----------------------------------------------------------------------------
//* Function Name       : vfo_mc_gen_start
//* Object              : arm a stream - the vfo task keys up within ms
//* Notes    			: caller owns slot timing (start at even minute +1s).
//* Notes    			: All Si5351 i2c traffic stays in the vfo task, so
//* Notes    			: this only stages the request and wakes the task
//* Context    			: any task
//*----------------------------------------------------------------------------
uchar vfo_mc_gen_start(ulong center_hz, const uchar *syms, ushort nsym)
{
	ushort i;

	if((mc_gen.on) || (mc_gen.pending) ||
	   (syms == NULL) || (nsym == 0) || (nsym > MC_GEN_MAX_SYMS))
		return 1;

	for(i = 0; i < nsym; i++)
		mc_gen.syms[i] = syms[i] & 3;

	mc_gen.nsym		= nsym;
	mc_gen.idx		= 0;
	mc_gen.center_c	= (uint64_t)center_hz * 100ULL;
	mc_gen.pending	= 1;

	// Kick the vfo task out of its long sleep - it keys the first tone
	if(ps.hVfoTask != NULL)
		xTaskNotify(ps.hVfoTask, 0, eNoAction);

	return 0;
}

//*----------------------------------------------------------------------------
//* Function Name       : vfo_mc_gen_stop
//* Object              : carrier off, stream aborted/complete
//* Context    			: CONTEXT_VFO
//*----------------------------------------------------------------------------
void vfo_mc_gen_stop(void)
{
	Si5351_output_enable(SI5351_CLK1, 0);
	mc_gen.on		= 0;
	mc_gen.pending	= 0;
}

//*----------------------------------------------------------------------------
//* Function Name       : vfo_mc_gen_active
//* Object              :
//* Context    			: any task
//*----------------------------------------------------------------------------
uchar vfo_mc_gen_active(void)
{
	return (uchar)(mc_gen.on || mc_gen.pending);
}

//*----------------------------------------------------------------------------
//* Function Name       : vfo_mc_gen_next_delay
//* Object              : ticks until the next symbol boundary (>= 1)
//* Context    			: CONTEXT_VFO
//*----------------------------------------------------------------------------
ulong vfo_mc_gen_next_delay(void)
{
	ulong elapsed, next;

	if(mc_gen.pending)
		return 1;

	if(!mc_gen.on)
		return portMAX_DELAY;

	elapsed	= xTaskGetTickCount() - mc_gen.start_tick;
	next	= mc_gen_deadline((ulong)mc_gen.idx + 1);

	if(next <= elapsed)
		return 1;

	return next - elapsed;
}

//*----------------------------------------------------------------------------
//* Function Name       : vfo_mc_gen_proc
//* Object              : advance the symbol stream on deadline
//* Notes    			: deadlines are absolute offsets from start_tick, so
//* Notes    			: early wake-ups or a late call can never accumulate
//* Notes    			: timing error
//* Context    			: CONTEXT_VFO
//*----------------------------------------------------------------------------
void vfo_mc_gen_proc(void)
{
	ulong elapsed;

	// Armed by another task - key the first tone from the vfo context
	if(mc_gen.pending)
	{
		mc_gen.pending = 0;

		mc_gen_set_tone(mc_gen.syms[0]);
		Si5351_output_enable(SI5351_CLK1, 1);

		mc_gen.start_tick	= xTaskGetTickCount();
		mc_gen.on			= 1;
	}

	if(!mc_gen.on)
		return;

	elapsed = xTaskGetTickCount() - mc_gen.start_tick;

	while(mc_gen.on && (elapsed >= mc_gen_deadline((ulong)mc_gen.idx + 1)))
	{
		mc_gen.idx++;

		if(mc_gen.idx >= mc_gen.nsym)
		{
			vfo_mc_gen_stop();
			printf("mc gen: stream done, %u symbols in %u ms \r\n",
					(uint)mc_gen.nsym, (uint)elapsed);
			break;
		}

		mc_gen_set_tone(mc_gen.syms[mc_gen.idx]);
	}
}

#endif
