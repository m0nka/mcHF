//************************************************************************************
//**                                                                                 **
//**                                 mcHF QRP Transceiver                            **
//**                         Krassi Atanassov - M0NKA, 2013-2026                     **
//**                                                                                 **
//**---------------------------------------------------------------------------------**
//**                                                                                 **
//**  File name:		cpu_trace.c                                                  **
//**  Description:  	Per task CPU load diagnostics(DWT cycle counter based)       **
//**  Licence:			https://github.com/m0nka/mcHF/blob/main/LICENSE            **
//************************************************************************************
//
// Purpose: catch the "CPU jumps to 100% after a while" condition red-handed.
//
// The on-screen CPU meter(cpu_utils.c) only says the idle task is starved,
// it can not say by whom. This module gives FreeRTOS a high resolution
// run time stats clock(DWT->CYCCNT / 1024) and periodically prints the
// per task CPU share over the last interval, plus stack headroom, heap
// state and the rate of the two interrupt sources that can generate
// load outside of any task context(M4 FFT broadcast HSEM, touch EXTI).
//
// Wiring:
//   - FreeRTOSConfig.h: configGENERATE_RUN_TIME_STATS = 1,
//     portCONFIGURE_TIMER_FOR_RUN_TIME_STATS() -> cpu_trace_init(),
//     portGET_RUN_TIME_COUNTER_VALUE()         -> cpu_trace_counter()
//   - ui_proc.c main loop calls cpu_trace_poll()
//   - icc_proc.c HSEM_ID_4 handler increments cpu_trace_fft_irq_cnt
//   - touch_proc.c EXTI handler increments cpu_trace_touch_irq_cnt
//
#include "mchf_pro_board.h"
#include "main.h"

#include "cpu_trace.h"
#include "cpu_utils.h"

#ifdef CPU_TRACE_ENABLE

// Upper bound on tasks reported in one dump
#define CPU_TRACE_MAX_TASKS			24

// prev counter storage, indexed by xTaskNumber(1 based, grows per created task)
#define CPU_TRACE_TASK_SLOTS		32

// FreeRTOS process state(for the ps.epoch mS counter)
extern struct PROC_STATE 			ps;

// IRQ storm detectors
volatile ulong 						cpu_trace_fft_irq_cnt 	= 0;
volatile ulong 						cpu_trace_touch_irq_cnt = 0;

//*----------------------------------------------------------------------------
//* Function Name       : cpu_trace_init
//* Object              : enable the DWT cycle counter
//* Notes    			: called by the kernel via
//* Notes   			: portCONFIGURE_TIMER_FOR_RUN_TIME_STATS()
//* Context    			: CONTEXT_RESET
//*----------------------------------------------------------------------------
void cpu_trace_init(void)
{
	// Trace subsystem on
	CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;

	// Unlock DWT registers(needed on CM7)
	DWT->LAR = 0xC5ACCE55;

	// Reset and start the cycle counter
	DWT->CYCCNT = 0;
	DWT->CTRL  |= DWT_CTRL_CYCCNTENA_Msk;
}

//*----------------------------------------------------------------------------
//* Function Name       : cpu_trace_counter
//* Object              : run time stats clock, ~468.75 kHz at 480 MHz core
//* Notes    			: CYCCNT wraps every ~8.9S at 480 MHz, so the raw
//* Notes   			: counter is software extended here. Safe as long as
//* Notes    			: context switches happen more often than the wrap,
//* Notes    			: which the 1 kHz tick guarantees in practice
//* Context    			: kernel(context switch) and task(stats dump)
//*----------------------------------------------------------------------------
ulong cpu_trace_counter(void)
{
	static ulong 	last_cyccnt  = 0;
	static uint64_t total_cycles = 0;

	ulong now, ret, primask;

	// Short critical section, callers mix task and handler context
	primask = __get_PRIMASK();
	__disable_irq();

	now 		  = DWT->CYCCNT;
	total_cycles += (ulong)(now - last_cyccnt);
	last_cyccnt   = now;

	ret = (ulong)(total_cycles >> 10);

	__set_PRIMASK(primask);

	return ret;
}

//*----------------------------------------------------------------------------
//* Function Name       : cpu_trace_task_state_char
//* Object              : one letter task state for compact print
//*----------------------------------------------------------------------------
static char cpu_trace_task_state_char(eTaskState state)
{
	switch(state)
	{
		case eRunning:		return 'X';
		case eReady:		return 'R';
		case eBlocked:		return 'B';
		case eSuspended:	return 'S';
		case eDeleted:		return 'D';
		default:			return '?';
	}
}

//*----------------------------------------------------------------------------
//* Function Name       : cpu_trace_poll
//* Object              : periodic per task CPU share dump over the debug UART
//* Notes    			: prints share of the LAST interval, not since boot,
//* Notes   			: so the culprit task is obvious the moment the
//* Notes    			: overload condition kicks in
//* Context    			: CONTEXT_VIDEO(UI task main loop)
//*----------------------------------------------------------------------------
void cpu_trace_poll(void)
{
	static ulong 		last_dump_ms	= 0;
	static ulong 		last_total_time = 0;
	static ulong 		last_fft_cnt 	= 0;
	static ulong 		last_tch_cnt 	= 0;
	static ulong 		prev_task_time[CPU_TRACE_TASK_SLOTS] = { 0 };

	// Static - called from single(UI) task only, keep off the task stack
	static TaskStatus_t snap[CPU_TRACE_MAX_TASKS];

	ulong 				num_tasks, total_time, total_delta, interval_ms;
	ulong 				fft_now, tch_now;
	ulong 				i;

	interval_ms = ps.epoch - last_dump_ms;
	if(interval_ms < CPU_TRACE_DUMP_PERIOD_MS)
		return;

	last_dump_ms = ps.epoch;

	// Snapshot IRQ counters
	fft_now = cpu_trace_fft_irq_cnt;
	tch_now = cpu_trace_touch_irq_cnt;

	// Snapshot all tasks
	num_tasks = uxTaskGetSystemState(snap, CPU_TRACE_MAX_TASKS, &total_time);
	if(num_tasks == 0)
		return;		// snap array too small - increase CPU_TRACE_MAX_TASKS

	total_delta 	= total_time - last_total_time;
	last_total_time = total_time;
	if(total_delta == 0)
		return;

	printf("--- cpu %d%%, fft %d/s, tch %d/s, heap %d(min %d) ---\r\n",
			(int)osGetCPUUsage(),
			(int)(((fft_now - last_fft_cnt) * 1000) / interval_ms),
			(int)(((tch_now - last_tch_cnt) * 1000) / interval_ms),
			(int)xPortGetFreeHeapSize(),
			(int)xPortGetMinimumEverFreeHeapSize());

	last_fft_cnt = fft_now;
	last_tch_cnt = tch_now;

	for(i = 0; i < num_tasks; i++)
	{
		TaskStatus_t *t = &snap[i];
		ulong 		 task_delta = 0;
		ulong 		 permille;
		char 		 name_pad[17];
		const char 	 *nm = t->pcTaskName;
		ulong 		 j;

		// Delta over the last interval
		if(t->xTaskNumber < CPU_TRACE_TASK_SLOTS)
		{
			task_delta = t->ulRunTimeCounter - prev_task_time[t->xTaskNumber];
			prev_task_time[t->xTaskNumber] = t->ulRunTimeCounter;
		}

		// 64 bit intermediate, task_delta*1000 can exceed 32 bits
		permille = (ulong)(((uint64_t)task_delta * 1000) / total_delta);

		// Manual left-justify - the tiny printf in common/print_f.c only
		// knows %d/i/u/x/X/s/c with optional '0' fill and width, and it
		// aborts the whole line on anything else(like the '-' flag)
		for(j = 0; j < (sizeof(name_pad) - 1); j++)
		{
			if(*nm)
				name_pad[j] = *nm++;
			else
				name_pad[j] = ' ';
		}
		name_pad[sizeof(name_pad) - 1] = 0;

		printf("%s %c p%d %3d.%d%% stk %4d\r\n",
				name_pad,
				cpu_trace_task_state_char(t->eCurrentState),
				(int)t->uxCurrentPriority,
				(int)(permille / 10), (int)(permille % 10),
				(int)t->usStackHighWaterMark);
	}
}

#endif
