#ifndef __BOARD_H
#define __BOARD_H

typedef struct pwr_db
{
  __IO uint32_t t[0x30/4];
  __IO uint32_t PDR1;

}PWDDBG_TypeDef;

/* Private macro -------------------------------------------------------------*/
#define PWDDBG                          ((PWDDBG_TypeDef*)PWR)
#define DEVICE_IS_CUT_2_1()             (HAL_GetREVID() & 0x21ff) ? 1 : 0

#define PWR_CFG_SMPS    				0xCAFECAFE
#define PWR_CFG_LDO     				0x5ACAFE5A

#if 0
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
#endif

void 	board_gpio_clocks_on(void);
uchar 	board_config(void);
void 	board_hold_power(void);
void 	board_power_off(void);
void 	board_toggle_rx_tx(void);

void 	SystemClockChange_Handler(void);
void 	SystemClock_Config(void);
void 	PeriphCommonClock_Config(void);
void 	MPU_Config(void);
void 	CPU_CACHE_Enable(void);

#endif
