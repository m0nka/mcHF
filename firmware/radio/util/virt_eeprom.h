/************************************************************************************
**                                                                                 **
**                             mcHF Pro QRP Transceiver                            **
**                         Krassi Atanassov - M0NKA, 2013-2025                     **
**                                                                                 **
**---------------------------------------------------------------------------------**
**                                                                                 **
**  File name:                                                                     **
**  Description:                                                                   **
**  Last Modified:                                                                 **
**  Licence:                                                                       **
************************************************************************************/
#ifndef __VIRT_EEPROM_H
#define __VIRT_EEPROM_H

// Virtual eeprom locations
#define	EEP_BASE_ADDR				0x000
// ---------------------------
// Checksum - 3 bytes reserved
// ---------------------------
#define	EEP_CURR_BAND				0x004
#define	EEP_SW_SMOOTH				0x005
#define	EEP_DEMO_MODE				0x006
#define	EEP_BRIGHTNESS				0x007
#define	EEP_SMET_TYPE				0x008
#define	EEP_AGC_MODE				0x009
#define	EEP_RF_GAIN					0x00A
#define	EEP_BT_ON					0x00B
//
#define	EEP_BANDS					0xE10		// pos 3600, band info, 400 bytes

// -----------------------------------------------------------------------------
// Virtual Eeprom in BackUp SRAM access macros
//
// assumes: 1. Battery connected to VBAT pin
//			2. External 32 kHz LSE, all clocks enabled(RCC_OSCILLATORTYPE_LSE,RCC_LSE_ON)
//			3. Write access to Backup domain enabled (PWR->CR1 |= PWR_CR1_DBP)
//			4. Enabled BKPRAM clock (__HAL_RCC_BKPRAM_CLK_ENABLE())
//			5. Enabled Backup SRAM low power Regulator (HAL_PWREx_EnableBkUpReg())
//
#define EEP_BASE					0x38800000
//

void 	virt_eeprom_write(ushort addr,uchar value);
ulong 	virt_eeprom_read(ushort addr);
ulong 	virt_eeprom_init(void);

#endif
