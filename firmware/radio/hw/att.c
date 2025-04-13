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

#include "att.h"

uchar att_init_done = 0;

static void att_delay_100uS(ulong delay)
{
	ulong j;

	while (delay)
	{
		j = 20000;
		while (j--)
		{
			__asm("nop");
		}
		delay--;
	}
}

static void att_shift_out(uchar val)
{
	uchar i;
	uchar value = val;

	for(i = 0; i < 6; i++)
	{
		// Set data line state
		if((value & 0x20) == 0x20)
			LL_GPIO_SetOutputPin	(ATT_DATA_PORT, ATT_DATA);

		// Clock out
		LL_GPIO_SetOutputPin		(ATT_CLK_PORT, ATT_CLK);
		att_delay_100uS(2);
		LL_GPIO_ResetOutputPin		(ATT_CLK_PORT, ATT_CLK);
		att_delay_100uS(2);

		LL_GPIO_ResetOutputPin	(ATT_DATA_PORT, ATT_DATA);
		value = value << 1;
	}

	// Activate new value
	LL_GPIO_SetOutputPin			(ATT_LE_PORT, ATT_LE);
	att_delay_100uS(2);
	LL_GPIO_ResetOutputPin			(ATT_LE_PORT, ATT_LE);
	att_delay_100uS(2);

	// Back to reset state
	//LL_GPIO_ResetOutputPin			(ATT_DATA_PORT, ATT_DATA);
}

void att_new_value(uchar db)
{
	if(!att_init_done)
		return;

	att_shift_out(db);

	//printf("ATT changed\r\n");
}

void att_hw_init(void)
{
	LL_GPIO_ResetOutputPin(ATT_LE_PORT, 	ATT_LE);
	LL_GPIO_ResetOutputPin(ATT_DATA_PORT, 	ATT_DATA);
	LL_GPIO_ResetOutputPin(ATT_CLK_PORT, 	ATT_CLK);

	// One direction only
	LL_GPIO_SetPinMode(ATT_LE_PORT, 	ATT_LE, 	LL_GPIO_MODE_OUTPUT);
	LL_GPIO_SetPinMode(ATT_DATA_PORT, 	ATT_DATA, 	LL_GPIO_MODE_OUTPUT);
	LL_GPIO_SetPinMode(ATT_CLK_PORT, 	ATT_CLK, 	LL_GPIO_MODE_OUTPUT);

	// Keep the bus inactive
	//LL_GPIO_SetPinPull(ATT_LE_PORT, 	ATT_LE, 	LL_GPIO_PULL_NO);
	//LL_GPIO_SetPinPull(ATT_DATA_PORT, 	ATT_DATA, 	LL_GPIO_PULL_NO);
	//LL_GPIO_SetPinPull(ATT_CLK_PORT, 	ATT_CLK, 	LL_GPIO_PULL_NO);

	// Minimise RF
	//LL_GPIO_SetPinSpeed(ATT_LE_PORT, 	ATT_LE, 	LL_GPIO_SPEED_FREQ_LOW);
	//LL_GPIO_SetPinSpeed(ATT_DATA_PORT, 	ATT_DATA, 	LL_GPIO_SPEED_FREQ_LOW);
	//LL_GPIO_SetPinSpeed(ATT_CLK_PORT, 	ATT_CLK, 	LL_GPIO_SPEED_FREQ_LOW);

	//printf("ATT init\r\n");

	att_init_done = 1;
}
