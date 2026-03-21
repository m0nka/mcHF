/************************************************************************************
**                                                                                 **
**                                 mcHF QRP Transceiver                            **
**                         Krassi Atanassov - M0NKA, 2013-2026                     **
**                                                                                 **
**---------------------------------------------------------------------------------**
**                                                                                 **
**  File name:                                                                     **
**  Description:                                                                   **
**  Last Modified:                                                                 **
**  Licence:			https://github.com/m0nka/mcHF/blob/main/LICENSE            **
************************************************************************************/
#include "main.h"
#include "mchf_pro_board.h"

#ifdef CONTEXT_BMS

#include "shared_i2c.h"

#include "ch224a.h"

static uchar ch224a_i2c_write_registers(uint8_t word_addr, uint8_t *data, uint8_t len)
{
	ulong err = shared_i2c_write_reg(0x23, word_addr, data, len);
	if(err != 0)
	{
		printf("write block %d\r\n", (int)err);
		return 1;
	}

	return 0;
}

static uchar ch224a_i2c_read_registers(uint8_t word_addr, uint8_t *data, uint8_t len)
{
	ulong err = shared_i2c_read_reg(0x22, word_addr, data, len );
	if(err != 0)
	{
		printf("read block %d\r\n", (int)err);
		return 1;
	}

	return 0;
}

uchar ch224a_init(void)
{
	return 0;
}

#endif
