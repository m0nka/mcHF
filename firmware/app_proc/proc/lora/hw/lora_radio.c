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

uchar rx_state = 0;	// idle

extern sx126x_handle_t radio_drv;

uchar  lora_mem[256];

// Replay attack data
#if 1
unsigned char tx_data[37] = {
	0x15, 0x00, 0x11, 0x4F, 0xB5, 0x74, 0x53, 0x49, 0x18, 0x80, 0x0F, 0xF1, 0xB5, 0x1E, 0x75, 0x3C,
	0x57, 0x3B, 0xC0, 0xBD, 0x80, 0x30, 0x63, 0xAD, 0x08, 0x74, 0xAD, 0xBF, 0xBE, 0x8A, 0x73, 0x3B,
	0x83, 0xC5, 0x2C, 0xF3, 0x06
};
#else
unsigned char tx_data[38] = {
	0x15, 0x01, 0x25, 0x90, 0xCC, 0xB6, 0x18, 0x09, 0x3B, 0x66, 0xC5, 0xC4, 0x74, 0x95, 0xF5, 0x99,
	0x7F, 0x41, 0x4E, 0xFE, 0xE5, 0xE9, 0xAD, 0xAA, 0xCF, 0xB3, 0x56, 0xFA, 0x9C, 0x05, 0x52, 0x8A,
	0xD3, 0x5A, 0x6A, 0x91, 0xB9, 0xB8
};
#endif

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
	if(sx126x_set_dio3_as_txco_ctrl(&radio_drv, 1.8f, 5000.0f) != 0)
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
	#ifdef MESHCORE
	if(sx126x_set_sync_word(&radio_drv, RADIOLIB_SX126X_SYNC_WORD_PRIVATE) != 0)
		return 6;
	#else
	if(sx126x_set_sync_word_adv(&radio_drv, 0x2B, 0x00) != 0)
		return 6;
	#endif

	#if 0
    uint8_t maxDetLen = RADIOLIB_MIN(RADIOLIB_SX126X_SYNC_WORD_PRIVATE, 16);
    uchar preambleDetLength = maxDetLen >= 32 ? RADIOLIB_SX126X_GFSK_PREAMBLE_DETECT_32 :
                              maxDetLen >= 24 ? RADIOLIB_SX126X_GFSK_PREAMBLE_DETECT_24 :
                              maxDetLen >= 16 ? RADIOLIB_SX126X_GFSK_PREAMBLE_DETECT_16 :
                              maxDetLen >   0 ? RADIOLIB_SX126X_GFSK_PREAMBLE_DETECT_8 :
                              RADIOLIB_SX126X_GFSK_PREAMBLE_DETECT_OFF;
    //printf("payload len: %d \r\n", preambleDetLength);
	#endif

	// Set packet params
	if(sx126x_set_packet_params_lora(&radio_drv, LORA_PL, false, 0, true, false) != 0)
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

	#ifdef CONT_RX
	// Start IRQ
	if(sx126x_set_dio_irq_params(&radio_drv, RADIOLIB_SX126X_IRQ_ALL, RADIOLIB_SX126X_IRQ_RX_DONE, 0, 0) != 0)
	{
		printf("irq err\r\n");
		return 11;
	}

	// Start RX
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
	//uchar mem[16];
	if(sx126x_read_buffer(&radio_drv, 0x00, lora_mem, 2) == 0)
	{
		print_hex_array(lora_mem, 2);

		lora_mem[0] = 0x55;
		lora_mem[1] = 0xAA;

		if(sx126x_write_buffer(&radio_drv, 0x00, lora_mem, 2) == 0)
		{
			lora_mem[0] = 0;
			lora_mem[1] = 0;

			if(sx126x_read_buffer(&radio_drv, 0x00, lora_mem, 2) == 0)
			{
				print_hex_array(lora_mem, 2);
			}
		}
	}
	return 6;
	#endif

	#if 0
	// Memory access test
	int i;
	ushort max_mem = 256;

	// Clear FIFO
	memset(lora_mem, 0, max_mem);
	sx126x_write_buffer(&radio_drv, 0x00, lora_mem, max_mem);

	// Read back
	if(sx126x_read_buffer(&radio_drv, 0x00, lora_mem, max_mem) == 0)
	{
		print_hex_array(lora_mem, max_mem);
	}
	printf(" \r\n");

	// Load with rnd
	for(i = 0; i < max_mem; i++)
		lora_mem[i] = i;
	sx126x_write_buffer(&radio_drv, 0x00, lora_mem, max_mem);

	// Clear local
	memset(lora_mem, 0, max_mem);
	printf(" \r\n");

	// Read back
	if(sx126x_read_buffer(&radio_drv, 0x00, lora_mem, max_mem) == 0)
	{
		print_hex_array(lora_mem, max_mem);
	}

	return 6;
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

void lora_radio_signal_stats(void)
{
	uchar 	x, y, z;
	float 	sig, rssi, snr;
	char 	fbuf1[16], fbuf2[16], fbuf3[16];

	if(sx126x_get_packet_status_lora(&radio_drv, &x, &y, &z) != 0)
		return;

	//printf("packet stat: 0x%02x 0x%02x 0x%02x \r\n", x, y, z);

	// RssiPkt Lora
	sig  = (float)(-1.0 * x/2.0);

	// SignalRssiPkt Lora
	if(y < 128)
		snr = (float)(y/4.0);
	else
		snr = (float)((y - 256)/4.0);

	// SnrPkt Lora
	rssi = (float)(-1.0 * z/2.0);

	ftoa(sig,  fbuf1, sizeof(fbuf1));
	ftoa(rssi, fbuf2, sizeof(fbuf2));
	ftoa(snr,  fbuf3, sizeof(fbuf3));

	printf("PWR: %sdBm SNR: %sdB RSSI: %sdB \r\n", fbuf1, fbuf2, fbuf3);

	#if 0
	if(sx126x_get_rssi_inst(&radio_drv, &rssi) == 0)
	{
		char fbuf[16];
		ftoa(rssi, fbuf, sizeof(fbuf));
		printf("packet rssi %sdBm \r\n", fbuf);
	}
	#endif
}

void lora_radio_rx_check(void)
{
	ushort 	a = 0;
	uchar  	b = 0, c = 0;
	uchar  	len, ptr;

	if(!rx_state)
	{
		//printf("start rx \r\n");

		sx126x_clear_irq_status	 (&radio_drv, RADIOLIB_SX126X_IRQ_ALL);
		sx126x_set_op_mode_rx	 (&radio_drv, RX_TIMEOUT_F);
		sx126x_set_dio_irq_params(&radio_drv, RADIOLIB_SX126X_IRQ_ALL, RADIOLIB_SX126X_IRQ_RX_DONE|RADIOLIB_SX126X_IRQ_TIMEOUT, 0, 0);

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
			printf("------------------------------------------ \r\n");
			//printf("irq stat: 0x%02x(%02x,%02x)\r\n", a, b, c);

			// Get RSSI of packet
			lora_radio_signal_stats();

			#if 0
			if((a & RADIOLIB_SX126X_IRQ_RX_DONE) == RADIOLIB_SX126X_IRQ_RX_DONE)
				printf("--> rx done \r\n");

			if((a & RADIOLIB_SX126X_IRQ_PREAMBLE_DETECTED) == RADIOLIB_SX126X_IRQ_PREAMBLE_DETECTED)
				printf("--> preamble \r\n");

			if((a & RADIOLIB_SX126X_IRQ_HEADER_VALID) == RADIOLIB_SX126X_IRQ_HEADER_VALID)
				printf("--> header valid \r\n");

			if((a & RADIOLIB_SX126X_IRQ_SYNC_WORD_VALID) == RADIOLIB_SX126X_IRQ_SYNC_WORD_VALID)
				printf("--> sync word valid \r\n");
			#endif

			if((a & RADIOLIB_SX126X_IRQ_CRC_ERR) == RADIOLIB_SX126X_IRQ_CRC_ERR)
			{
				printf("--> crc error \r\n");
				sx126x_clear_irq_status(&radio_drv, a);
				goto restart_rx;
			}

			if((a & RADIOLIB_SX126X_IRQ_HEADER_ERR) == RADIOLIB_SX126X_IRQ_HEADER_ERR)
			{
				printf("--> header error \r\n");
				sx126x_clear_irq_status(&radio_drv, a);
				goto restart_rx;
			}

			// Do we have fully valid data in buffer ? Then read it
			if(((a & RADIOLIB_SX126X_IRQ_RX_DONE) == RADIOLIB_SX126X_IRQ_RX_DONE) &&
			   ((a & RADIOLIB_SX126X_IRQ_HEADER_VALID) == RADIOLIB_SX126X_IRQ_HEADER_VALID))
			{
				// Get buffer status
				if(sx126x_get_rx_buffer_status(&radio_drv, &len, &ptr) == 0)
				{
					// Get buffer contents
					if(sx126x_read_buffer(&radio_drv, ptr, lora_mem, len) == 0)
					{
						printf("size: %d \r\n", len);
						print_hex_array(lora_mem, len);
						sx126x_clear_irq_status(&radio_drv, a);
						goto restart_rx;
					}
				}
			}
			else if((a & RADIOLIB_SX126X_IRQ_PREAMBLE_DETECTED) == RADIOLIB_SX126X_IRQ_PREAMBLE_DETECTED)
			{
				printf("--> preamble only \r\n");
			}

			// Just in case
			sx126x_clear_irq_status(&radio_drv, a);
		}
	}

restart_rx:
	rx_state = 0;	// restart
}

void lora_radio_schedule_tx(void)
{
	ushort 	a = 0;
	uchar  	b = 0, c = 0;

	printf("tx ...\r\n");

	sx126x_set_pa_config(&radio_drv, 0x04, 0x07, false);
	sx126x_set_tx_params(&radio_drv, 22, true, 200);
	sx126x_set_packet_params_lora(&radio_drv, LORA_PL, false, sizeof(tx_data), true, false);
	sx126x_write_buffer(&radio_drv, 0x00, tx_data, sizeof(tx_data));
	sx126x_set_dio_irq_params(&radio_drv, RADIOLIB_SX126X_IRQ_ALL, RADIOLIB_SX126X_IRQ_TX_DONE|RADIOLIB_SX126X_IRQ_TIMEOUT, 0, 0);
	//sx126x_set_op_mode_tx(&radio_drv);
	sx126x_set_op_mode_tx_t(&radio_drv, TX_TIMEOUT_F);

	// Wait complete
	if(sx126x_irq_wait(&radio_drv, 5000) != 0)
	{
		printf("irq timeout\r\n");
		return;
	}

	// Get irq status
	if(sx126x_get_irq_status(&radio_drv, &a, &b, &c) == 0)
	{
		printf("irq stat: 0x%02x(%02x,%02x)\r\n", a, b, c);

		sx126x_clear_irq_status(&radio_drv, a);

		if((a & RADIOLIB_SX126X_IRQ_TIMEOUT) == RADIOLIB_SX126X_IRQ_TIMEOUT)
			printf("--> timeout \r\n");

		if((a & RADIOLIB_SX126X_IRQ_TX_DONE) == RADIOLIB_SX126X_IRQ_TX_DONE)
			printf("--> tx done \r\n");
	}

	printf("tx finished\r\n");
}

#endif
