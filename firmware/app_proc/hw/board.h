#ifndef __BOARD_H
#define __BOARD_H

void 	mchf_pro_board_debug_led_init(void);
void 	mchf_pro_board_blink_if_alive(uchar flags);

void 	mchf_pro_board_read_cpu_details(void);
void 	mchf_pro_board_start_gpio_clocks(void);

void 	mchf_pro_board_mpu_config(void);
void 	mchf_pro_board_cpu_cache_enable(void);

uchar 	mchf_pro_board_system_clock_config(uchar clk_src);
uchar 	mchf_pro_board_rtc_clock_config(uchar clk_src);
void 	mchf_pro_board_rtc_clock_disable(void);

void 	mchf_pro_board_swo_init(void);
void 	mchf_pro_board_mco2_on(void);

void 	mchf_pro_board_sensitive_hw_init(void);

void 	bsp_gpio_clocks_on(void);

uchar 	bsp_config(void);
void 	bsp_hold_power(void);

void 	SystemClockChange_Handler(void);
void 	SystemClock_Config(void);
void 	PeriphCommonClock_Config(void);
void 	MPU_Config(void);
void 	CPU_CACHE_Enable(void);

#endif
