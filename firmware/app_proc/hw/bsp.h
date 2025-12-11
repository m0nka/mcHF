#ifndef __BSP_H
#define __BSP_H

#include "main.h"

#define BKP_REG_PWR_CFG           (RTC->BKP28R)
#define BKP_REG_SW_CFG            (RTC->BKP27R)
#define BKP_REG_SUBDEMO_ADDRESS   (RTC->BKP26R)
#define BKP_REG_CALIB_DR0         (RTC->BKP25R)
#define BKP_REG_CALIB_DR1         (RTC->BKP24R)

void 	LCD_LL_Reset(void);
void 	bsp_gpio_clocks_on(void);

uchar 	bsp_config(void);
void 	bsp_hold_power(void);

//uint8_t BSP_SuspendCPU2( void );
//uint8_t BSP_ResumeCPU2( void );
//uint8_t BSP_TouchUpdate(void);

//void BSP_JumpToSubDemo(uint32_t SubDemoAddress);
//int BSP_ResourcesCopy(WM_HWIN hItem, FIL * pResFile, uint32_t Address);
//int BSP_FlashProgram(WM_HWIN hItem, FIL * pResFile, uint32_t Address);
//int BSP_FlashUpdate(uint32_t Address, uint8_t *pData, uint32_t Size);

#endif
