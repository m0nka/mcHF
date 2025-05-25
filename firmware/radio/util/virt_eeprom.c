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
**  Licence:               GNU GPLv3                                               **
************************************************************************************/

#include "main.h"
#include "mchf_pro_board.h"

#include "virt_eeprom.h"

// Public radio state
extern struct	TRANSCEIVER_STATE_UI	tsu;

#if 0
//
// Write to backup RAM with cache flush(32bit values only)
//
// https://community.st.com/t5/stm32-mcus-products/stm32h743-backup-ram-not-saved/td-p/273468
//
static void virt_eeprom_write_with_flush(uint64_t *padd, uint64_t data)
{
	*padd = data;
	padd  = (uint64_t *)((uint32_t)padd & 0xFFFFFFE0);
	SCB_CleanDCache_by_Addr((uint32_t *)padd, 32);
}
#else
// 8 bit version
static void virt_eeprom_write_with_flush(uchar *padd, uchar data)
{
	*padd = data;
	SCB_CleanDCache_by_Addr((ulong *)padd, 32);
}
#endif

//*----------------------------------------------------------------------------
//* Function Name       : virt_eeprom_write
//* Object              :
//* Notes    			:
//* Notes   			:
//* Notes    			:
//* Context    			: CONTEXT_RESET_VECTOR
//*----------------------------------------------------------------------------
void virt_eeprom_write(ushort addr,uchar value)
{
	if(!tsu.eeprom_init_done)
		return;

	if(addr > 0xFFF)
		return;

	// Write with flush
	virt_eeprom_write_with_flush((uchar *)(EEP_BASE + addr), value);

	// Verify
	#if 0
	uchar read = virt_eeprom_read(addr);
	if(read != value)
		printf("verify fail \r\n");
	#endif
}

//*----------------------------------------------------------------------------
//* Function Name       : virt_eeprom_read
//* Object              :
//* Notes    			:
//* Notes   			:
//* Notes    			:
//* Context    			: CONTEXT_RESET_VECTOR
//*----------------------------------------------------------------------------
ulong virt_eeprom_read(ushort addr)
{
	if(!tsu.eeprom_init_done)
		return 0xFF;

	if(addr > 0xFFF)
		return 0xFF;

	return *(uchar *)(EEP_BASE + addr);
}

//*----------------------------------------------------------------------------
//* Function Name       : virt_eeprom_init
//* Object              :
//* Notes    			:
//* Notes   			:
//* Notes    			:
//* Context    			: CONTEXT_RESET_VECTOR
//*----------------------------------------------------------------------------
ulong virt_eeprom_init(void)
{
	#ifndef USE_LSE
	printf("WARNING: LSE off!! \r\n");
	#endif

	// SRAM clock on
	__HAL_RCC_BKPRAM_CLK_ENABLE();

	// Enables access to the backup domain
	PWR->CR1 |= PWR_CR1_DBP;
  	while((PWR->CR1 & PWR_CR1_DBP) == RESET)
    {
  		__asm("nop");
  	}

  	// Backup power regulator on
 	if(HAL_PWREx_EnableBkUpReg() != HAL_OK)
  	{
 		printf("backup power reg fail\r\n");
  		return 1;
  	}

	// Set to enabled
	tsu.eeprom_init_done = 1;

	#if 0
	uchar x = k_BkupRestoreParameter(6); 	printf("bkup before: 0x%x \r\n", x++);
 	k_BkupSaveParameter(6, x);
 	printf("bkup after: 0x%x \r\n", k_BkupRestoreParameter(6));

	uchar y = *(uchar *)(EEP_BASE + 4);
 	printf("sram before: 0x%x \r\n", y++);
 	write_with_flush((uint64_t *)(EEP_BASE + 4), y);
 	printf("sram after: 0x%x \r\n", *(uchar *)(EEP_BASE + 4));
	#endif

	return 0;
}
