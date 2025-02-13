#ifndef __ADC_H
#define __ADC_H

// Timeout to wait for current conversion on going to be completed.
// Timeout fixed to worst case, for 1 channel.
//   - maximum sampling time (830.5 adc_clk)
//   - ADC resolution (Tsar 16 bits= 16.5 adc_clk)
//   - ADC clock with prescaler 256
//     823 * 256 = 210688 clock cycles max
// Unit: cycles of CPU clock.
#define ADC_CALIBRATION_TIMEOUT_MS      (1320UL)
#define ADC_ENABLE_TIMEOUT_MS           (2UL)
#define ADC_DISABLE_TIMEOUT_MS          (2UL)
#define ADC_STOP_CONVERSION_TIMEOUT_MS  (2UL)
#define ADC_CONVERSION_TIMEOUT_MS       (50UL)

// Delay between ADC end of calibration and ADC enable.
// Delay estimation in CPU cycles: Case of ADC enable done
// immediately after ADC calibration, ADC clock setting slow
// (LL_ADC_CLOCK_ASYNC_DIV32). Use a higher delay if ratio
// (CPU clock / ADC clock) is above 32.
#define ADC_DELAY_CALIB_ENABLE_CPU_CYCLES  (LL_ADC_DELAY_CALIB_ENABLE_ADC_CYCLES * 32)

#define VDDA_APPLI                       (3300U)

void adc_callback(void);
void adc_init(void);

#endif
