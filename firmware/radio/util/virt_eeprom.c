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

// Public radio state
extern struct	TRANSCEIVER_STATE_UI	tsu;

//*----------------------------------------------------------------------------
//* Function Name       : WRITE_EEPROM
//* Object              :
//* Notes    			:
//* Notes   			:
//* Notes    			:
//* Context    			: anyone!
//*----------------------------------------------------------------------------
void WRITE_EEPROM(ushort addr,uchar value)
{
	uchar *bkp = (uchar *)EEP_BASE;

	if(!tsu.eeprom_init_done)
	{
		//printf("eeprom init not done!\r\n");
		return;
	}

	if(addr > 0xFFF)
		return;

	// Write to BackUp SRAM
	*(bkp + addr) = value;
}

//*----------------------------------------------------------------------------
//* Function Name       : READ_EEPROM
//* Object              :
//* Notes    			:
//* Notes   			:
//* Notes    			:
//* Context    			: anyone!
//*----------------------------------------------------------------------------
ulong READ_EEPROM(ushort addr)
{
	ulong ret;
	uchar *bkp = (uchar *)EEP_BASE;

	if(!tsu.eeprom_init_done)
	{
		//printf("eeprom init not done!\r\n");
		return 0xFF;
	}

	if(addr > 0xFFF)
		return 0xFF;

	// Read BackUp SRAM
	return *(bkp + addr);
}

//*----------------------------------------------------------------------------
//* Function Name       : INIT_EEPROM
//* Object              :
//* Notes    			:
//* Notes   			:
//* Notes    			:
//* Context    			: anyone!
//*----------------------------------------------------------------------------
ulong INIT_EEPROM(void)
{
	#if 0
	if(xTaskGetSchedulerState() != taskSCHEDULER_RUNNING)
	{
		printf("os not started yet!\r\n");
		return 1;
	}
	#endif

	// Set to enabled
	tsu.eeprom_init_done = 1;

	//printf("== eeprom init done ==\r\n");
	return 0;
}
