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

uchar ch224a_on_init = 0;

static uchar ch224a_i2c_write_registers(uint8_t word_addr, uint8_t *data, uint8_t len)
{
	ulong err = shared_i2c_write_reg(CH224A_I2C_ADDR, word_addr, data, len);
	if(err != 0)
	{
		printf("write block %d\r\n", (int)err);
		return 1;
	}

	return 0;
}

static uchar ch224a_i2c_read_registers(uint8_t word_addr, uint8_t *data, uint8_t len)
{
	ulong err = shared_i2c_read_reg(CH224A_I2C_ADDR, word_addr, data, len );
	if(err != 0)
	{
		printf("read block %d\r\n", (int)err);
		return 1;
	}

	return 0;
}

uchar ch224a_detect(void)
{
	uchar  resp[2], stat;
	ushort curr;

	// Do we have device on the bus ?
	if(shared_i2c_is_ready(CH224A_I2C_ADDR, 10) != 0)
	{
		if(ch224a_on_init)
			printf("== charger removed ==\r\n");

		// Charger removed
		ch224a_on_init = 0;

		return 1;
	}

	if(ch224a_on_init)
		return 0;

	// Read status register
	if(ch224a_i2c_read_registers(0x09, resp, 1))
		return 2;

	stat = resp[0];
	printf("usbpd stat: 0x%2x \r\n", stat);

	// PD flag set ?
	if((stat & 0x08) == 0x00)
		return 3;

	// Read current register
	if(ch224a_i2c_read_registers(0x50, resp, 2))
		return 4;

	// Max current from charger
	curr  = resp[1] << 8 | resp[0];
	curr *= 50;

	printf("chmax curr: %dmA \r\n", curr);

	// Current enough
	if(curr < 1600)
		return 5;

	// Switch to 15V, not working!
	//--ch224a_i2c_write_registers(0x0A, 0x03, 1);

	ch224a_on_init = 1;

	printf("== charger detected ==\r\n");
	return 0;
}

#endif
