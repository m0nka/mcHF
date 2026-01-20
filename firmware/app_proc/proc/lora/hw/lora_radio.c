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

#ifdef CONTEXT_LORA

#include "sx126x.h"
#include "sx126x_commands.h"
#include "lora_spi.h"

#include "lora_radio.h"

//#define CONT_RX

// Temp as local const
#define LORA_SF		SX126X_LORA_SPREADING_FACTOR_8
#define LORA_CR		SX126X_LORA_CODING_RATE_4_8
#define LORA_BW		SX126X_LORA_BANDWIDTH_62
#define LORA_FR		869.618f

extern sx126x_handle_t radio_drv;

static uchar lora_radio_find_chip(void)
{
	char version[100];
	int i;

	// Retry loop
	for(i = 0; i < 10; i++)
	{
		// HW reset
		sx1262_reset(NULL);

		// Set standby mode
		if(sx126x_set_op_mode_standby(&radio_drv, 0) != 0)
			continue;

		// Read ID string
		if(sx126x_read_version_string(&radio_drv, version, sizeof(version)) != 0)
			continue;

		// Debug only
		#if 0
		if(version[0] > 0x80)
			print_hex_array((uchar *)version, 6);
		else
			printf("ver: %s(%d) \r\n", version, i);
		#endif

		// Detected
		if(strncmp(version,"SX1261", 6) == 0)
			return 0;

		// Retry delay
		vTaskDelay(10);
	}

	return 1;
}

static uchar lora_radio_modem_setup(void)
{
	// Set standby mode
	if(sx126x_set_op_mode_standby(&radio_drv, 1) != 0)
		return 1;

	// Setup TCXO
	if(sx126x_set_dio3_as_txco_ctrl(&radio_drv, 1.6f, 5000.0f) != 0)
	{
		printf("tcxo err\r\n");
		return 2;
	}

	// Set modem type
	if(sx126x_set_packet_type(&radio_drv, RADIOLIB_SX126X_PACKET_TYPE_LORA) != 0)
	{
		printf("pkt type err\r\n");
		return 3;
	}

	// Set initial CAD parameters
	if(sx126x_set_cad_params(&radio_drv,
							RADIOLIB_SX126X_CAD_ON_8_SYMB,
							(LORA_SF + 13),
							RADIOLIB_SX126X_CAD_PARAM_DET_MIN,
							RADIOLIB_SX126X_CAD_GOTO_STDBY,
							0) != 0)
	{
		printf("cad err\r\n");
		return 4;
	}

	// Clear IRQ
	if(sx126x_set_dio_irq_params(&radio_drv, RADIOLIB_SX126X_IRQ_NONE, RADIOLIB_SX126X_IRQ_NONE, 0, 0) != 0)
	{
		printf("irq err\r\n");
		return 5;
	}

	// Calibrate All
	radio_drv.timeout = 5000;
	if(sx126x_calibrate(&radio_drv, true, true, true, true, true, true, true) != 0)
	{
		printf("calib err\r\n");
		return 5;
	}

	// Restore timeout
	radio_drv.timeout = 1000;

	// Wait
	vTaskDelay(50);

	return 0;
}

uchar lora_radio_init(void)
{
	// Lora power on
	lora_spi_power_state(1);

	// Basic init
	if(sx126x_init(&radio_drv, 0, 0, 0, 0, 0))
		return 1;

	// Detect module
	if(lora_radio_find_chip())
		return 2;

	// Setup modem
	if(lora_radio_modem_setup() != 0)
		return 3;

	// Select LDO
	if(sx126x_set_regulator_mode(&radio_drv, 0) != 0)
		return 4;

	// Set modulation params
	if(sx126x_set_modulation_params_lora(&radio_drv, LORA_SF, LORA_BW, LORA_CR, false) != 0)
		return 5;

	// Set sync word
	if(sx126x_set_sync_word_adv(&radio_drv, RADIOLIB_SX126X_SYNC_WORD_PRIVATE, 0) != 0)
		return 6;

    uint8_t maxDetLen = RADIOLIB_MIN(RADIOLIB_SX126X_SYNC_WORD_PRIVATE, 16);
    uchar preambleDetLength = maxDetLen >= 32 ? RADIOLIB_SX126X_GFSK_PREAMBLE_DETECT_32 :
                              maxDetLen >= 24 ? RADIOLIB_SX126X_GFSK_PREAMBLE_DETECT_24 :
                              maxDetLen >= 16 ? RADIOLIB_SX126X_GFSK_PREAMBLE_DETECT_16 :
                              maxDetLen >   0 ? RADIOLIB_SX126X_GFSK_PREAMBLE_DETECT_8 :
                              RADIOLIB_SX126X_GFSK_PREAMBLE_DETECT_OFF;

    //printf("payload len: %d \r\n", preambleDetLength);

	// Set packet params
	if(sx126x_set_packet_params_lora(&radio_drv, 8, false, preambleDetLength, true, false) != 0)
		return 7;

	// Is it true ?
	if(sx126x_set_dio2_as_rf_switch_ctrl(&radio_drv, true) != 0)
		return 8;

	// Set internal RX and TX buffer addresses(256 byte space)
	if(sx126x_set_buffer_base_address(&radio_drv, 0x00, 0x00) != 0)
		return 9;

	// Set frequency
	if(sx126x_set_rf_frequency(&radio_drv, LORA_FR) != 0)
		return 10;

	// Start IRQ
	if(sx126x_set_dio_irq_params(&radio_drv, RADIOLIB_SX126X_IRQ_ALL, RADIOLIB_SX126X_IRQ_RX_DONE, 0, 0) != 0)
	{
		printf("irq err\r\n");
		return 11;
	}

	// Start RX
	#ifdef CONT_RX
	if(sx126x_set_op_mode_rx_cont(&radio_drv) != 0)
		return 12;
	#endif

	#if 0
	// Get status test
	uchar cmd_stat, chip_mode;
	//radio_drv.timeout = 10000;
	if(sx126x_get_status(&radio_drv, &cmd_stat, &chip_mode) == 0)
	{
		printf("status: %d, mode: %d \r\n", cmd_stat, chip_mode);
	}
	//radio_drv.timeout = 1000;
	#endif

	#if 0
	// Memory access test
	uchar mem[16];
	if(sx126x_read_buffer(&radio_drv, 0x80, mem, 2) == 0)
	{
		print_hex_array(mem, 2);

		mem[0] = 0x55;
		mem[1] = 0xAA;

		if(sx126x_write_buffer(&radio_drv, 0x80, mem, 2) == 0)
		{
			mem[0] = 0;
			mem[1] = 0;

			if(sx126x_read_buffer(&radio_drv, 0x80, mem, 2) == 0)
			{
				print_hex_array(mem, 2);
			}
		}
	}
	#endif

	#if 0
	// Register read/write test
	uchar regs[10];
	if(sx126x_read_register(&radio_drv, 0x038A, regs, 2) == 0)
	{
		print_hex_array(regs, 2);

		regs[0] = 1;
		regs[1] = 2;

		if(sx126x_write_register(&radio_drv, 0x038A, regs, 2) == 0)
		{
			regs[0] = 0;
			regs[1] = 0;

			if(sx126x_read_register(&radio_drv, 0x038A, regs, 2) == 0)
			{
				print_hex_array(regs, 2);
			}
		}
	}
	#endif

	return 0;
}

#ifdef CONT_RX
void lora_radio_rx_check(void)
{
	ushort a = 0;
	uchar  b = 0, c = 0;
	uchar  len, ptr;
	uchar  mem[256];

	// Check status
	if(sx126x_irq_wait(&radio_drv, 0) != 0)
		return;

	printf("-------------------- \r\n");

	// Get irq status
	if(sx126x_get_irq_status(&radio_drv, &a, &b, &c) == 0)
	{
		printf("irq stat: 0x%02x(%02x,%02x)\r\n", a, b, c);

		if((a & RADIOLIB_SX126X_IRQ_TIMEOUT) == RADIOLIB_SX126X_IRQ_TIMEOUT)
			printf("--> timeout \r\n");

		if((a & RADIOLIB_SX126X_IRQ_CRC_ERR) == RADIOLIB_SX126X_IRQ_CRC_ERR)
			printf("--> crc error \r\n");

		if((a & RADIOLIB_SX126X_IRQ_HEADER_ERR) == RADIOLIB_SX126X_IRQ_HEADER_ERR)
			printf("--> cad header error \r\n");

		if((a & RADIOLIB_SX126X_IRQ_RX_DONE) == RADIOLIB_SX126X_IRQ_RX_DONE)
			printf("--> rx done \r\n");

		if((a & RADIOLIB_SX126X_IRQ_HEADER_VALID) == RADIOLIB_SX126X_IRQ_HEADER_VALID)
			printf("--> header valid \r\n");

		if((a & RADIOLIB_SX126X_IRQ_HEADER_ERR) == RADIOLIB_SX126X_IRQ_HEADER_ERR)
			printf("--> header valid \r\n");

		if((a & RADIOLIB_SX126X_IRQ_PREAMBLE_DETECTED) == RADIOLIB_SX126X_IRQ_PREAMBLE_DETECTED)
			printf("--> preamble \r\n");

		if(a)
		{
			// Get buffer status
			if(sx126x_get_rx_buffer_status(&radio_drv, &len, &ptr) == 0)
			{
				printf("rx packet len %d, ptr %d \r\n", len, ptr);

				// Get buffer contents
				if(sx126x_read_buffer(&radio_drv, ptr, mem, len) == 0)
				{
					print_hex_array(mem, len);
					sx126x_clear_irq_status(&radio_drv, a);
				}
			}
		}
		else
			sx126x_clear_irq_status(&radio_drv, RADIOLIB_SX126X_IRQ_ALL);

		// Restart RX
		sx126x_set_op_mode_rx_cont(&radio_drv);
	}
}
#else

#define RX_TIMEOUT_MS		200
#define RX_TIMEOUT_F		(ulong)(((float)(RX_TIMEOUT_MS * 5)) * 1000.0f)/15.625f

uchar rx_state = 0;	// idle

void lora_radio_rx_check(void)
{
	ushort a = 0;
	uchar  b = 0, c = 0;
	uchar  len, ptr;
	uchar  mem[256];

	if(!rx_state)
	{
		//printf("start rx \r\n");

		if(sx126x_clear_irq_status(&radio_drv, RADIOLIB_SX126X_IRQ_ALL) != 0)
		{
			// fail 1
		}

		if(sx126x_set_buffer_base_address(&radio_drv, 0x00, 0x00) != 0)
		{
			// fail 2
		}

		if(sx126x_set_op_mode_rx(&radio_drv, RX_TIMEOUT_F) != 0)
		{
			// fail 3
		}

		if(sx126x_set_dio_irq_params(&radio_drv, RADIOLIB_SX126X_IRQ_ALL, RADIOLIB_SX126X_IRQ_RX_DONE|RADIOLIB_SX126X_IRQ_TIMEOUT, 0, 0) != 0)
		{
			// fail 4
		}

		rx_state = 1;	// wait
		return;
	}

	// Check status
	if(sx126x_irq_wait(&radio_drv, 0) != 0)
		return;

	// Get irq status
	if(sx126x_get_irq_status(&radio_drv, &a, &b, &c) == 0)
	{
		//printf("irq stat: 0x%02x(%02x,%02x)\r\n", a, b, c);

		if((a & RADIOLIB_SX126X_IRQ_TIMEOUT) == RADIOLIB_SX126X_IRQ_TIMEOUT)
		{
			//printf("--> timeout \r\n");
			sx126x_clear_irq_status(&radio_drv, RADIOLIB_SX126X_IRQ_TIMEOUT);
			return;
		}
		else if(a)
		{
			printf("-------------------- \r\n");

			if((a & RADIOLIB_SX126X_IRQ_CRC_ERR) == RADIOLIB_SX126X_IRQ_CRC_ERR)
			{
				printf("--> crc error \r\n");
				sx126x_clear_irq_status(&radio_drv, a);
				goto restart_rx;
			}

			if((a & RADIOLIB_SX126X_IRQ_HEADER_ERR) == RADIOLIB_SX126X_IRQ_HEADER_ERR)
			{
				printf("--> cad header error \r\n");
				sx126x_clear_irq_status(&radio_drv, a);
				goto restart_rx;
			}

			if((a & RADIOLIB_SX126X_IRQ_HEADER_ERR) == RADIOLIB_SX126X_IRQ_HEADER_ERR)
			{
				printf("--> header valid \r\n");
				sx126x_clear_irq_status(&radio_drv, a);
				goto restart_rx;
			}

			if((a & RADIOLIB_SX126X_IRQ_SYNC_WORD_VALID) == RADIOLIB_SX126X_IRQ_SYNC_WORD_VALID)
			{
				printf("--> sync word valid \r\n");
			}

			// Do we have fully valid data in buffer ? Then read it
			if(((a & RADIOLIB_SX126X_IRQ_RX_DONE) == RADIOLIB_SX126X_IRQ_RX_DONE) &&\
			   ((a & RADIOLIB_SX126X_IRQ_HEADER_VALID) == RADIOLIB_SX126X_IRQ_HEADER_VALID) &&\
			   ((a & RADIOLIB_SX126X_IRQ_PREAMBLE_DETECTED) == RADIOLIB_SX126X_IRQ_PREAMBLE_DETECTED))
			{
				// Set standby mode (we are already in, right ?)
				sx126x_set_op_mode_standby(&radio_drv, 0);

				// Get buffer status
				if(sx126x_get_rx_buffer_status(&radio_drv, &len, &ptr) == 0)
				{
					printf("rx packet len %d, ptr %d \r\n", len, ptr);

					// Get buffer contents
					if(sx126x_read_buffer(&radio_drv, ptr, mem, len) == 0)
					{
						print_hex_array(mem, len);
						sx126x_clear_irq_status(&radio_drv, a);
						goto restart_rx;
					}
				}
			}

			// Clear incomplete
			sx126x_clear_irq_status(&radio_drv, a);
			goto restart_rx;
		}
		else
		{
			sx126x_clear_irq_status(&radio_drv, RADIOLIB_SX126X_IRQ_ALL);
			goto restart_rx;
		}
	}

restart_rx:
	rx_state = 0;	// restart

}
#endif

#endif
