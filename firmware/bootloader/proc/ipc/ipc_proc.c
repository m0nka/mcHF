/************************************************************************************
**                                                                                 **
**                             mcHF Pro QRP Transceiver                            **
**                         Krassi Atanassov - M0NKA, 2013-2024                     **
**                                                                                 **
**---------------------------------------------------------------------------------**
**                                                                                 **
**  File name:                                                                     **
**  Description:                                                                   **
**  Last Modified:                                                                 **
**  Licence:       The mcHF project is released for radio amateurs experimentation **
**               and non-commercial use only.Check 3rd party drivers for licensing **
************************************************************************************/
#include "mchf_pro_board.h"
#include "main.h"

#include "ipc_proc.h"

//! #include "wifi.h"
#include "mchf_ipc_def.h"

#ifdef CONTEXT_IPC_PROC

#include "ESP32/esp32_ipc_uart.h"
#include "ESP32/esp32_prog_uart.h"
#include "lwrb.h"

//
// As the programmer board is connected in parallel to
// the M7 core, need to un-comment in order to update
// ESP32 firmware
//
#define IPC_ALLOW_PROGRAMMING

// Public radio state
//extern struct	TRANSCEIVER_STATE_UI	tsu;

//extern QueueHandle_t 	hEspMessage;

uchar ipc_init_done = 0;
uchar ipc_seq_id = 0;

//*----------------------------------------------------------------------------
//* Function Name       : ipc_pro_checksum
//* Object              :
//* Notes    			:
//* Notes    			:
//* Context    			: CONTEXT_IPC_PROC
//*----------------------------------------------------------------------------
unsigned char ipc_pro_checksum(unsigned char *data, unsigned int len)
{
	unsigned char xorCS = 0x00;

	for (int i = 0; i < len; i++)
	{
		unsigned char extract = *data++;
		xorCS ^= extract;
	}
	return xorCS;
}

//*----------------------------------------------------------------------------
//* Function Name       : ipc_proc_exp_size
//* Object              :
//* Notes    			: return payload size per command (from .h file)
//* Notes    			:
//* Context    			: CONTEXT_IPC_PROC
//*----------------------------------------------------------------------------
ushort ipc_proc_exp_size(uchar cmd)
{
	ushort expected	= 10;	// header size

	switch(cmd)
	{
		case MENU_0X01:
			expected += MENU_0X01_SZ;
			break;
		case MENU_READ_ESP_32_VERSION:
			expected += MENU_READ_ESP_32_VERSION_SZ;
			break;
		case MENU_WIFI_GET_DETAILS:
			expected += MENU_WIFI_GET_DETAILS_SZ;
			break;
		case MENU_WIFI_CONNECT_TO_NETWORK:
			expected += MENU_WIFI_CONNECT_TO_NETWORK_SZ;
			break;
		case MENU_WIFI_GET_NETWORK_RSSI:
			expected += MENU_WIFI_GET_NETWORK_RSSI_SZ;
			break;
		case MENU_WRITE_SQL_VALUE:
			expected += MENU_WRITE_SQL_VALUE_SZ;
			break;
		case MENU_READ_SQL_VALUE:
			expected += MENU_READ_SQL_VALUE_SZ;
			break;
		case MENU_GET_UTC:
			expected += MENU_GET_UTC_SZ;
			break;
		case MENU_SET_ATTEN:
			expected += MENU_SET_ATTEN_SZ;
			break;

		case MENU_POST_INIT:
			expected = (CMD_NO_RESPONSE - 1);
			break;

		// ---------------------------------------------------
		case MENU_ESP32_REBOOT:
			expected = (CMD_NO_RESPONSE - 1);
			break;
		default:
			return CMD_NOT_SUPPORTED;
	}
	expected++;			// Checksum

	//printf("expected %d\r\n", expected);
	return expected;
}

//*----------------------------------------------------------------------------
//* Function Name       : ipc_proc_exchange
//* Object              :
//* Notes    			: send and receive
//* Notes    			:
//* Context    			: CONTEXT_IPC_PROC
//*----------------------------------------------------------------------------
uchar ipc_proc_exchange(uchar cmd, uchar *payload, uchar p_size, uchar *buffer, uchar *ret_size, ulong timeout)
{
	uint8_t TxBuffer[128];
	uint8_t RxBuffer[128];

	ushort 	expected, out_size = 11; // header + checksum
	uchar 	chksum;

	//printf("ipc cmd: %02x\r\n", cmd);

	ipc_uart_flush();

	// Clear buffers
	memset(RxBuffer, 0, sizeof(RxBuffer));
	memset(TxBuffer, 0, sizeof(TxBuffer));

	// Check command validity, get payload size from definition file
	expected = ipc_proc_exp_size(cmd);
	if(expected == CMD_NOT_SUPPORTED)
		return 1;			// not supported command

	if((expected & 1024) > sizeof(RxBuffer))
		return 2;			// return buffer too small

	// Init command
	TxBuffer[0] = cmd;				// command
	TxBuffer[1] = (0xFF -cmd);		// inverted cmd
	TxBuffer[2] = ipc_seq_id;		// sequence ID

	if((payload != NULL) && (p_size))
	{
		memcpy(TxBuffer + 10, payload, p_size);
		out_size += p_size;

		TxBuffer[8] = (uchar)(p_size >> 0);
		TxBuffer[9] = (uchar)(p_size >> 8);
	}

	// Insert expected size
	TxBuffer[6] = (uchar)(expected >> 0);
	TxBuffer[7] = (uchar)(expected >> 8);

	// Checksum on outgoing transfer
	TxBuffer[out_size - 1] = ipc_pro_checksum(TxBuffer, (out_size - 1));

	//print_hex_array(TxBuffer, out_size);

	// Send command
	if(ipc_uart_send((uint8_t*)TxBuffer, out_size) != 0)
		return 3;

	ipc_seq_id++;

	// Do we need response ? Emulate success
	if(expected == CMD_NO_RESPONSE)
		return 0;

	// Start read
	if(ipc_uart_read((uint8_t *)RxBuffer, expected, timeout) != 0)
		return 4;

	// Calculate checksum
	chksum = ipc_pro_checksum(RxBuffer, (expected - 1));
	//printf("chk sum %02x\r\n", chksum);

	if(chksum != RxBuffer[expected - 1])
		return 5;

	//print_hex_array(RxBuffer, 10);	// header

	// Copy to caller task
	if((buffer != NULL) && (ret_size != NULL))
	{
		//print_hex_array(RxBuffer + 10, expected - 11);
		memcpy(buffer, RxBuffer + 10, expected - 11); // no header, no checksum
		*ret_size = (expected - 11);
	}

	return 0;
}

#if 0
//*----------------------------------------------------------------------------
//* Function Name       : ipc_proc_poll_rssi
//* Object              :
//* Notes    			:
//* Notes    			:
//* Context    			: CONTEXT_IPC_PROC
//*----------------------------------------------------------------------------
static void ipc_proc_poll_rssi(void)
{
	uchar rssi_buf[20];
	uchar ret_size = 0, err;

	static ulong rssi_read_skip = 0;

	// Do not call if init is bad
	if(!ipc_init_done)
		return;

	rssi_read_skip++;
	if(rssi_read_skip < 400)	// every 20s
		return;

	rssi_read_skip = 0;

	//printf("-------------------------\r\n");

	err = ipc_proc_exchange(MENU_WIFI_GET_NETWORK_RSSI, NULL, 0, rssi_buf, &ret_size, 50);
	if((err == 0) && (rssi_buf[9] == 4))
	{
		tsu.wifi_rssi = ((rssi_buf[10] << 24)|(rssi_buf[11] << 16)|(rssi_buf[12] << 8)|(rssi_buf[13]));
		//printf("RSSI=%d dBm\r\n", tsu.wifi_rssi);
	}
	//else
	//	printf("err %d\r\n", err);
}
#endif

//*----------------------------------------------------------------------------
//* Function Name       : icc_proc_check_msg
//* Object              :
//* Notes    			:
//* Notes    			:
//* Context    			: CONTEXT_IPC_PROC
//*----------------------------------------------------------------------------
static void icc_proc_check_msg(void)
{
#if 0
	osEvent event;

	// Wait for a short time for pending messages
	event = osMessageGet(hEspMessage, IPC_PROC_SLEEP_TIME);
	if(event.status != osEventMessage)
		return;

	// Check status type
	if(event.status != osEventMessage)
		return;

	struct ESPMessage *esp_msg = (struct ESPMessage *)event.value.p;

	//printf("-------------------------\r\n");
	//printf("ipc msg id: %d \r\n", esp_msg->ucMessageID);

	// Dump messages, but do not process
	if(!ipc_init_done)
		return;

	switch(esp_msg->ucMessageID)
	{
		// Read firmware version
		case 1:
			esp_msg->ucExecResult = ipc_proc_exchange(MENU_READ_ESP_32_VERSION, NULL, 0, esp_msg->ucData,&esp_msg->ucDataReady, 500);
			break;

		// Read WiFi details
		case 2:
			esp_msg->ucExecResult = ipc_proc_exchange(MENU_WIFI_GET_DETAILS, NULL, 0, esp_msg->ucData,&esp_msg->ucDataReady, 500);
			break;

		// Connect to WiFi network
		case 3:
		{
			esp_msg->ucExecResult = ipc_proc_exchange(MENU_WIFI_CONNECT_TO_NETWORK, (uchar *)vc, sizeof(vc), esp_msg->ucData,&esp_msg->ucDataReady, 2000);
			if(esp_msg->ucExecResult == 0)
			{
				// Reboot it
				ipc_proc_exchange(MENU_ESP32_REBOOT, NULL, 0, NULL, NULL, 200);

				// Give it time to reboot and connect to wifi network
				vTaskDelay(3000);
			}
			break;
		}

		// Write virtual eeprom (SQLite call)
		case 4:
		{
			esp_msg->ucExecResult = ipc_proc_exchange(	MENU_WRITE_SQL_VALUE,
														esp_msg->ucData,
														esp_msg->usPayloadSize,
														esp_msg->ucData,
														&esp_msg->ucDataReady,
														3000);
			break;
		}

		// Read virtual eeprom
		case 5:
		{
			esp_msg->ucExecResult = ipc_proc_exchange(	MENU_READ_SQL_VALUE,
														esp_msg->ucData,
														esp_msg->usPayloadSize,
														esp_msg->ucData,
														&esp_msg->ucDataReady,
														500);
			break;
		}

		// Read NTP time
		case 6:
		{
			esp_msg->ucExecResult = ipc_proc_exchange(	MENU_GET_UTC,
														esp_msg->ucData,
														esp_msg->usPayloadSize,
														esp_msg->ucData,
														&esp_msg->ucDataReady,
														5000);
			break;
		}

		// Update attenuator
		case 7:
		{
			uchar msg[2];

			msg[0] = tsu.band[tsu.curr_band].atten;
			//printf("atten val=%d\r\n", msg[0]);
			esp_msg->ucExecResult = ipc_proc_exchange(	MENU_SET_ATTEN,
														msg,
														1,
														msg,
														(msg + 1),
														200);
			break;
		}

		default:
			printf("no handler for msg: %d\r\n",esp_msg->ucMessageID);
			goto complete;
	}

	if(esp_msg->ucExecResult)
		printf("uart comm err: %d (cmd %d)\r\n",esp_msg->ucExecResult, esp_msg->ucMessageID);

complete:
	// Mark as complete
	esp_msg->ucProcStatus = TASK_PROC_DONE;
#endif
}

static void ipc_proc_esp_reset(void)
{
	GPIO_InitTypeDef  gpio_init_structure;

	gpio_init_structure.Pull  = GPIO_NOPULL;
	gpio_init_structure.Speed = GPIO_SPEED_FREQ_LOW;
	gpio_init_structure.Mode  = GPIO_MODE_OUTPUT_PP;
	gpio_init_structure.Pin   = ESP_RESET;
	HAL_GPIO_Init(ESP_RESET_PORT, &gpio_init_structure);

	// Power on
	HAL_GPIO_WritePin(ESP_POWER_PORT, ESP_POWER, GPIO_PIN_RESET);

	// Reset
	HAL_GPIO_WritePin(ESP_RESET_PORT, ESP_RESET, GPIO_PIN_RESET);
	HAL_Delay(100);

	#ifndef IPC_ALLOW_PROGRAMMING
	HAL_GPIO_WritePin(ESP_RESET_PORT, ESP_RESET, GPIO_PIN_SET);
	#else
	// Set back as Input, to allow for internal pull up to keep
	// CPU running, and allow programming
	gpio_init_structure.Mode  = GPIO_MODE_INPUT;
	HAL_GPIO_Init(ESP_RESET_PORT, &gpio_init_structure);
	#endif
}

static void ipc_proc_hw_signals_init(void)
{
	GPIO_InitTypeDef  gpio_init_structure;

	// Allow programming by keeping all lines high impedance
	// on reset(in case the bootloader set then as output)
	gpio_init_structure.Pull  = GPIO_NOPULL;
	gpio_init_structure.Speed = GPIO_SPEED_FREQ_LOW;
	gpio_init_structure.Mode  = GPIO_MODE_INPUT;

	gpio_init_structure.Pin   = ESP_RESET;
	HAL_GPIO_Init(ESP_RESET_PORT, &gpio_init_structure);

	gpio_init_structure.Pin   = ESP_POWER;
	HAL_GPIO_Init(ESP_POWER_PORT, &gpio_init_structure);

	gpio_init_structure.Pin   = ESP_GPIO0;
	HAL_GPIO_Init(ESP_GPIO0_PORT, &gpio_init_structure);

	#ifndef IPC_ALLOW_PROGRAMMING
	// IO0 is PC15
	gpio_init_structure.Pin   = ESP_GPIO0;
	gpio_init_structure.Mode  = GPIO_MODE_OUTPUT_PP;
	HAL_GPIO_Init(ESP_GPIO0_PORT, &gpio_init_structure);

	// IO0 state
	HAL_GPIO_WritePin(ESP_GPIO0_PORT, ESP_GPIO0, GPIO_PIN_SET);
	#endif

	// Power is PI11
	gpio_init_structure.Pin   = ESP_POWER;
	gpio_init_structure.Mode  = GPIO_MODE_OUTPUT_PP;
	HAL_GPIO_Init(ESP_POWER_PORT, &gpio_init_structure);

	// Power off
	HAL_GPIO_WritePin(ESP_POWER_PORT, ESP_POWER, GPIO_PIN_SET);
}

//
// How to safely (and with lowest current draw) turn off the
// ESP32 ?
//
static void ipc_proc_hw_signals_cleanup(void)
{
	GPIO_InitTypeDef  gpio_init_structure;

	// Power off
	HAL_GPIO_WritePin(ESP_POWER_PORT, ESP_POWER, GPIO_PIN_SET);

	gpio_init_structure.Pull  = GPIO_NOPULL;
	gpio_init_structure.Speed = GPIO_SPEED_FREQ_LOW;
	gpio_init_structure.Mode  = GPIO_MODE_INPUT;

	gpio_init_structure.Pin   = ESP_RESET;
	HAL_GPIO_Init(ESP_RESET_PORT, &gpio_init_structure);

	gpio_init_structure.Pin   = ESP_GPIO0;
	HAL_GPIO_Init(ESP_GPIO0_PORT, &gpio_init_structure);
}

int ipc_proc_establish_link(char *fw_ver)
{
	uchar buff[100];
	uchar err, ret_size = 0;
	ulong i, timeout = 200;

	// Reset ESP32
	ipc_proc_esp_reset();

	HAL_Delay(200);

	// Wait proper boot-up/wifi connection
	for(i = 0; i < timeout; i++)
	{
		// Dummy command, to find out if UART handler is responding
		err = ipc_proc_exchange(MENU_0X01, NULL, 0, NULL, NULL, 50);
		if(err == 0)
			break;

		// Sending those commands too often prevents the IDLE LINE trigger
		// in the co-processor and transfer arrive grouped together
		HAL_Delay(1);
	}

	if(i == timeout)
	{
		printf("esp32 failed to boot(%d)!\r\n", i);
		return 1;
	}
	//printf("esp32 link up(%d)\r\n", i);

	// Read version
	err = ipc_proc_exchange(MENU_READ_ESP_32_VERSION, NULL, 0, buff, &ret_size, 500);
	if(err == 0)
	{
		//printf("ESP32 firmware: %s\r\n", buff);

		if(fw_ver != NULL)
			strcpy(fw_ver, buff);	// ToDo: unsafe, fix!!!
	}
	else
	{
		printf("err read fw ver %d\r\n", err);
		return 2;
	}

	// Extra init, no response, allow to take time
	//ipc_proc_exchange(MENU_POST_INIT, NULL, 0, NULL, 0, 0);

	//HAL_Delay(2000);

	#if 0
	// Read WiFi details
	err = ipc_proc_exchange(MENU_WIFI_GET_DETAILS, NULL, 0, buff, &ret_size, 500);
	if(err == 0)
	{
		//printf("wifi data ret size %d\r\n", ret_size);
		if(buff[9])
		{
			printf("MAC: %02x:%02x:%02x:%02x:%02x:%02x, IP: %d.%d.%d.%d\r\n",	buff[10],
																				buff[11],
																				buff[12],
																				buff[13],
																				buff[14],
																				buff[15],
																				buff[16],
																				buff[17],
																				buff[18],
																				buff[19]);
			printf("SSID: %s\r\n", (char *)(&buff[21]));
		}
	}
	else
	{
		printf("err read fw ver %d\r\n", err);
		return 3;
	}
	#endif

	return 0;
}

//*----------------------------------------------------------------------------
//* Function Name       : esp32_uart_init
//* Object              :
//* Notes    			: called from main(), not task!
//* Notes    			:
//* Context    			: CONTEXT_RESET
//*----------------------------------------------------------------------------
void ipc_proc_init(void)
{
	//printf("ipc_proc_init\r\n");

	// HW control lines
	ipc_proc_hw_signals_init();

	// Low level driver
	ipc_uart_init();
	//prog_uart_init();

	// Allow calls
	ipc_init_done = 1;

	//printf("ipc_proc_init ok\r\n");
}

//extern struct	UI_DRIVER_STATE			ui_s;
//extern 			TaskHandle_t 			hUiTask;

void test(void)
{
	uchar buff[100];

	if(prog_uart_read(buff, 5, 50) == 0)
	{
		printf("got bytes\r\n");
	}
}

//*----------------------------------------------------------------------------
//* Function Name       : ipc_proc_task
//* Object              :
//* Notes    			:
//* Notes    			:
//* Context    			: CONTEXT_IPC_PROC
//*----------------------------------------------------------------------------
void ipc_proc_task(void const *arg)
{
#if 0
	vTaskDelay(IPC_PROC_START_DELAY);
	printf("ipc proc start\r\n");

	// Connect to co-processor
	if(ipc_proc_establish_link())
		goto ipc_proc_suspend;

	// Test
	#if 0
	// Test
	vTaskDelay(2000);
	ui_s.req_state = MODE_MENU;
	if(hUiTask != NULL)
		xTaskNotify(hUiTask, 0x01, eSetValueWithOverwrite);
	#endif

ipc_proc_loop:
	//test();
	//vTaskDelay(200);
	icc_proc_check_msg();
	//ipc_proc_poll_rssi();	// ToDo: maybe polling message from UI, instead of looping here ?
	goto ipc_proc_loop;

ipc_proc_suspend:

	// Clean up, HW de-init, power off
	#ifndef IPC_ALLOW_PROGRAMMING
	ipc_proc_hw_signals_cleanup();
	ipc_uart_stop();
	#endif

	printf("ipc proc suspended\r\n");
	vTaskSuspend(NULL);
#endif
}
#endif
