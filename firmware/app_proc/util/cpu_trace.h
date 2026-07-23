//************************************************************************************
//**                                                                                 **
//**                                 mcHF QRP Transceiver                            **
//**                         Krassi Atanassov - M0NKA, 2013-2026                     **
//**                                                                                 **
//**---------------------------------------------------------------------------------**
//**                                                                                 **
//**  File name:		cpu_trace.h                                                  **
//**  Description:  	Per task CPU load diagnostics(DWT cycle counter based)       **
//**  Licence:			https://github.com/m0nka/mcHF/blob/main/LICENSE            **
//************************************************************************************
#ifndef __CPU_TRACE_H
#define __CPU_TRACE_H

// ----------------------------------------------------------------------
// Master switch - uncomment to compile the task monitor in
//
// Enables FreeRTOS run time stats(hooked in FreeRTOSConfig.h) and the
// periodic per task load dump on the debug UART. Everything compiles
// away to nothing when commented out
//
//#define CPU_TRACE_ENABLE
// ----------------------------------------------------------------------

// Periodic dump interval in mS, keep at 5000 or below, the per-mille
// maths in the dump routine will overflow 64 bit intermediate on
// silly big values(also more UART traffic on smaller values)
#define CPU_TRACE_DUMP_PERIOD_MS		5000

#ifdef CPU_TRACE_ENABLE

// Run time stats clock for FreeRTOS(referenced from FreeRTOSConfig.h)
void 			cpu_trace_init(void);
unsigned long 	cpu_trace_counter(void);

// Periodic per task load dump, call from a task that survives
// the overload condition(currently the UI task main loop)
void 			cpu_trace_poll(void);

// IRQ storm detectors, incremented from ISR context
extern volatile unsigned long 	cpu_trace_fft_irq_cnt;
extern volatile unsigned long 	cpu_trace_touch_irq_cnt;

#define cpu_trace_fft_irq_hit()			(cpu_trace_fft_irq_cnt++)
#define cpu_trace_touch_irq_hit()		(cpu_trace_touch_irq_cnt++)

#else

// Monitor disabled - all call sites compile to nothing
#define cpu_trace_poll()
#define cpu_trace_fft_irq_hit()
#define cpu_trace_touch_irq_hit()

#endif

#endif
