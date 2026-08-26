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
#include "mchf_pro_board.h"
#include "main.h"
#include "version.h"


#include "lcd_low.h"
#include "lcd_high.h"

#include "hw_lcd.h"
#include "hw_sdram.h"

#include "hw_sd.h"
#include "ff_gen_drv.h"
#include "sd_diskio.h"

#include "hw_flash.h"

#include "selftest_proc.h"
#include "shared_i2c.h"

extern SD_HandleTypeDef hsd_sdmmc[1];
FATFS SDFatFs;  						/* File system object for SD card logical drive */
FIL MyFile;     						/* File object */
char SDPath[4]; 						/* SD card logical drive path */

extern ulong sys_timer;

extern ulong reset_reason;
extern uchar gen_boot_reason_err;
extern uchar charge_mode;
extern uchar stay_in_boot;

int test_sd_card(void)
{
	GPIO_InitTypeDef  GPIO_InitStruct;
	int i;

	// SD_DET - PC2 after 0.8.3 mod
	GPIO_InitStruct.Pin   = SD_DET;
	GPIO_InitStruct.Mode  = GPIO_MODE_INPUT;
	GPIO_InitStruct.Pull  = GPIO_PULLUP;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
	HAL_GPIO_Init(SD_DET_PORT, &GPIO_InitStruct);

	if(HAL_GPIO_ReadPin(SD_DET_PORT, SD_DET))
	{
		printf("sd card na\r\n");
		return 1;
	}
	//printf("sd card in\r\n");

	// SD_PWR_CNTR - PB13 after 0.8.3 mod
	// Note: IO15 and IO33 on ESP32 can interfere
	//       with this line(with current mod), so
	//       make sure chip is on and those are inputs
	GPIO_InitStruct.Pin   = SD_PWR_CNTR;
	GPIO_InitStruct.Mode  = GPIO_MODE_OUTPUT_PP;
	GPIO_InitStruct.Pull  = GPIO_PULLUP;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
	HAL_GPIO_Init(SD_PWR_CNTR_PORT, &GPIO_InitStruct);

	// Power on
	#ifndef SD_PWR_SWAP_POLARITY
	HAL_GPIO_WritePin(SD_PWR_CNTR_PORT, SD_PWR_CNTR, GPIO_PIN_RESET);
	#else
	HAL_GPIO_WritePin(SD_PWR_CNTR_PORT, SD_PWR_CNTR, GPIO_PIN_SET);
	#endif

	// Wait for SD card power to stabilize
	HAL_Delay(100);

	// Init low level driver
	int sd_ret = BSP_SD_Init(0);
	if(sd_ret != 0)
	{
		printf("sd low level driver init err (%d)!\r\n", sd_ret);
		return 2;
	}

	//printf("sd block size: %d\r\n", (int)hsd_sdmmc->SdCard.BlockSize);
	//printf("sd num blocks: %d\r\n", (int)hsd_sdmmc->SdCard.BlockNbr);
	//printf("sd card size : %d\r\n", (hsd_sdmmc->SdCard.BlockNbr*hsd_sdmmc->SdCard.BlockNbr)/1024);

	uchar boot[512];

	// Read boot sector
	if(BSP_SD_ReadBlocks(0, (ulong *)boot, 0, 1) != 0)
	{
		printf("sd unable to read boot sector!\r\n");
		return 3;
	}
	//print_hex_array((uchar *)(&boot[0] + 512 - 32), 32);

	// Check signature
	if((boot[510] != 0x55) || (boot[511] != 0xAA))
	{
		printf("sd bad boot sector signature!\r\n");
		return 4;
	}

	// Manual blocks read
	for(i = 0; i < 10000; i++)
	{
		if(BSP_SD_ReadBlocks(0, (ulong *)boot, 1, 1) != 0)
			break;
	}

	printf("read blocks: %d\r\n", i);

	if(i < 10000)
		return 5;

	#if 0
	// Init FatFS
	if(FATFS_LinkDriver(&SD_Driver, SDPath) != 0)
	{
		printf("sd unable to init FS!\r\n");
		return 5;
	}

	printf("path: %s \r\n", SDPath);

	if(f_mount(&SDFatFs, (TCHAR const*)SDPath, 0) != FR_OK)
	{
		printf("sd unable to mount FS!\r\n");
		return 6;
	}

	FILINFO fno;
	FRESULT res;
	uint32_t bytesread;
	uint8_t rtext[100];

	res = f_stat("0:/system/app_proc.ini", &fno);
	if(res != FR_OK)
	{
		printf("error file stat!\r\n");
		return 7;
	}
	printf("size: %d \r\n", fno.fsize);

	/* Open the text file object with read access */
	if(f_open(&MyFile, "0:/system/app_proc.ini", FA_READ) == FR_OK)
	{
		memset(rtext, 0, sizeof(rtext));
		res = f_read(&MyFile, rtext, sizeof(rtext), (void *)&bytesread);

		if((bytesread > 0) && (res == FR_OK))
		{
			printf((char *)rtext);
		}
		else
			printf("error read file!\r\n");

		f_close(&MyFile);
	}
	else
		printf("error open file!\r\n");
	#endif

	return 0;
}

int sdram_test(void)
{
	ulong *ram_ptr = (ulong *)SDRAM_DEVICE_ADDR;
	ulong temp, i;
	ulong num_err = 0;

	// Connect SDRAM
	if(BSP_SDRAM_Init(0) != BSP_ERROR_NONE)
	{
		printf("SDRAM init error!\r\n");
		return 1;
	}

	// Write data
	for(i = 0; i < SDRAM_DEVICE_SIZE/4; i++)
	{
		temp = (i << 24)|(i << 16)|(i << 8)|i;
		*ram_ptr++ = temp;
	}

	// Reset ptr
	ram_ptr = (ulong *)SDRAM_DEVICE_ADDR;

	// Compare
	for(i = 0; i < SDRAM_DEVICE_SIZE/4; i++)
	{
		//if(i < 16)
		//	printf("%08x\r\n", *ram_ptr);

		temp = (i << 24)|(i << 16)|(i << 8)|i;
		if(*ram_ptr++ != temp)
			num_err++;
	}

	// Disconnect SDRAM
	BSP_SDRAM_DeInit(0);

	if(num_err)
	{
		printf("SDRAM test res: %d (%04x-%04x)\r\n", (int)num_err, SDRAM_DEVICE_ADDR, (int)ram_ptr);
		return 2;
	}

	return 0;
}

void fs_cleanup(void)
{
	// Clean up after file operation (close FS, de-init SD driver, card power off, etc..)
	f_mount(NULL, (TCHAR const*)SDPath, 0);
	FATFS_UnLinkDriverEx(SDPath, 0);
	BSP_SD_DeInit(0);

	#ifndef SD_PWR_SWAP_POLARITY
	HAL_GPIO_WritePin(SD_PWR_CNTR_PORT, SD_PWR_CNTR, GPIO_PIN_SET);
	#else
	HAL_GPIO_WritePin(SD_PWR_CNTR_PORT, SD_PWR_CNTR, GPIO_PIN_RESET);
	#endif
}

ulong is_firmware_valid(void)
{
	//return 5;

	#if 0
	// Is there valid jump to signature block (always const)
	if(*(ulong *)(RADIO_FIRM_ADDR + 0x004) != (RADIO_FIRM_ADDR + 0x299))
	{
		return 1;
	}

	// Signature1 valid ?
	if(*(ulong *)(RADIO_FIRM_ADDR + 0x2A0) != 0x77777777)
	{
		return 2;
	}

	// Signature2 valid ?
	if(*(ulong *)(RADIO_FIRM_ADDR + 0x2A4) != 0x88888888)
	{
		return 2;
	}
	#else
	if(*(ulong *)(RADIO_FIRM_ADDR + 0x004) == 0xFFFFFFFF)
	{
		return 1;
	}
	#endif

	// ToDo: test CRC
	// ...

	return 0;
}

// ---------------------------------------------------------------
// Extended HW tests
// ---------------------------------------------------------------

// I2C4 handle defined in shared_i2c.c
extern I2C_HandleTypeDef hbus_i2c1;

// ---------------------------------------------------------------
// Power/GPIO tests
// ---------------------------------------------------------------

static uchar vcc_5v_state = 0;

int test_5v_toggle(void)
{
	vcc_5v_state ^= 1;

#ifndef REV_0_8_4_PATCH
	HAL_GPIO_WritePin(VCC_5V_ON_PORT, VCC_5V_ON,
					  vcc_5v_state ? GPIO_PIN_SET : GPIO_PIN_RESET);
#else
	HAL_GPIO_WritePin(VCC_5V_ON_PORT, VCC_5V_ON,
					  vcc_5v_state ? GPIO_PIN_RESET : GPIO_PIN_SET);
#endif

	return vcc_5v_state;
}

static uchar fan_state = 0;

int test_fan_toggle(void)
{
	LL_GPIO_InitTypeDef gpio = {0};

	fan_state ^= 1;

	// Set output level BEFORE switching to output mode (match app code)
	if(fan_state)
		LL_GPIO_SetOutputPin(FAN_CNTR_PORT, FAN_CNTR);
	else
		LL_GPIO_ResetOutputPin(FAN_CNTR_PORT, FAN_CNTR);

	gpio.Pin       = FAN_CNTR;
	gpio.Mode      = LL_GPIO_MODE_OUTPUT;
	gpio.Pull      = LL_GPIO_PULL_DOWN;
	gpio.Speed     = LL_GPIO_SPEED_LOW;
	gpio.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
	LL_GPIO_Init(FAN_CNTR_PORT, &gpio);

	return fan_state;
}

static uchar leds_state = 0;

int test_leds_toggle(void)
{
	leds_state ^= 1;

	if(leds_state)
	{
		LL_GPIO_SetOutputPin(ON_LED_PORT, ON_LED);
		LL_GPIO_SetOutputPin(TX_LED_PORT, TX_LED);
	}
	else
	{
		LL_GPIO_ResetOutputPin(ON_LED_PORT, ON_LED);
		LL_GPIO_ResetOutputPin(TX_LED_PORT, TX_LED);
	}

	return leds_state;
}

static uchar bl_level = 4;

int test_backlight_cycle(void)
{
	static const ushort duties[] = { 0, 250, 500, 750, 999 };

	bl_level = (bl_level + 1) % 5;
	TIM1->CCR2 = duties[bl_level];

	return bl_level;
}

// ---------------------------------------------------------------
// I2C bus tests
// ---------------------------------------------------------------

int test_bq25730_ch224a(uchar *bq_ok, uchar *ch_ok)
{
	*bq_ok = 0;
	*ch_ok = 0;

	// BQ25730 at 0xD6 (0x6B << 1)
	if(shared_i2c_is_ready(0xD6, 3) == 0)
	{
		*bq_ok = 1;

		// Read ManufacturerID (reg 0x2E) and ChipID (reg 0x2F)
		uchar data[2] = {0, 0};
		shared_i2c_read_reg(0xD6, 0x2E, &data[0], 1);
		shared_i2c_read_reg(0xD6, 0x2F, &data[1], 1);
		printf("bq25730: manuf=0x%02x chip=0x%02x\r\n", data[0], data[1]);
	}

	// CH224A at 0x44 (0x22 << 1)
	if(shared_i2c_is_ready(0x44, 3) == 0)
		*ch_ok = 1;

	return (*bq_ok && *ch_ok) ? 0 : 1;
}

int test_codec_i2c(void)
{
	GPIO_InitTypeDef gpio = {0};

	// Take codec out of reset (active low reset)
	gpio.Pin   = CODEC_RESET;
	gpio.Mode  = GPIO_MODE_OUTPUT_PP;
	gpio.Pull  = GPIO_NOPULL;
	gpio.Speed = GPIO_SPEED_FREQ_LOW;
	HAL_GPIO_Init(CODEC_RESET_PORT, &gpio);
	HAL_GPIO_WritePin(CODEC_RESET_PORT, CODEC_RESET, GPIO_PIN_SET);
	HAL_Delay(50);

	// CS4245 needs a first register read before comms work reliably
	// Read chip ID register (0x01) — shared_i2c handles pin swap to codec
	uchar chip_id = 0;
	int res = shared_i2c_read_reg(0x98, 0x01, &chip_id, 1);

	if(res == 0)
		printf("cs4245: chip_id=0x%02x\r\n", chip_id);
	else
		printf("cs4245: read err %d\r\n", res);

	// Put codec back in reset
	HAL_GPIO_WritePin(CODEC_RESET_PORT, CODEC_RESET, GPIO_PIN_RESET);

	return res;
}

int test_si5351_i2c(void)
{
	I2C_HandleTypeDef hi2c2 = {0};
	GPIO_InitTypeDef gpio;

	// Enable I2C2 clock
	__HAL_RCC_I2C2_CLK_ENABLE();
	__HAL_RCC_I2C2_FORCE_RESET();
	__HAL_RCC_I2C2_RELEASE_RESET();

	// Configure PH4 (SCL) and PH5 (SDA) as I2C2
	gpio.Mode      = GPIO_MODE_AF_OD;
	gpio.Pull      = GPIO_PULLUP;
	gpio.Speed     = GPIO_SPEED_FREQ_HIGH;
	gpio.Alternate = GPIO_AF4_I2C2;

	gpio.Pin = GPIO_PIN_4;
	HAL_GPIO_Init(GPIOH, &gpio);
	gpio.Pin = GPIO_PIN_5;
	HAL_GPIO_Init(GPIOH, &gpio);

	// Init I2C2 (reuse timing from I2C4)
	hi2c2.Instance             = I2C2;
	hi2c2.Init.Timing          = hbus_i2c1.Init.Timing;
	hi2c2.Init.OwnAddress1     = 0;
	hi2c2.Init.AddressingMode  = I2C_ADDRESSINGMODE_7BIT;
	hi2c2.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
	hi2c2.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
	hi2c2.Init.NoStretchMode   = I2C_NOSTRETCH_DISABLE;

	if(HAL_I2C_Init(&hi2c2) != HAL_OK)
	{
		__HAL_RCC_I2C2_CLK_DISABLE();
		return 1;
	}

	// Check SI5351 at 0xC0 (0x60 << 1)
	int res = 0;
	if(HAL_I2C_IsDeviceReady(&hi2c2, 0xC0, 3, 100) != HAL_OK)
		res = 2;

	// Cleanup
	HAL_I2C_DeInit(&hi2c2);
	__HAL_RCC_I2C2_CLK_DISABLE();
	HAL_GPIO_DeInit(GPIOH, GPIO_PIN_4 | GPIO_PIN_5);

	return res;
}

int test_gt911_i2c(void)
{
	I2C_HandleTypeDef hi2c_ts = {0};
	GPIO_InitTypeDef gpio;

	// Touch uses I2C1 on PB6/PB7
	__HAL_RCC_I2C1_CLK_ENABLE();
	__HAL_RCC_I2C1_FORCE_RESET();
	__HAL_RCC_I2C1_RELEASE_RESET();

	// Configure PB6 (SCL) and PB7 (SDA)
	gpio.Mode      = GPIO_MODE_AF_OD;
	gpio.Pull      = GPIO_PULLUP;
	gpio.Speed     = GPIO_SPEED_FREQ_HIGH;
	gpio.Alternate = GPIO_AF4_I2C1;

	gpio.Pin = TOUCH_SCK_SCL_PIN;
	HAL_GPIO_Init(TOUCH_SCK_SCL_GPIO_PORT, &gpio);
	gpio.Pin = TOUCH_SDA_SDA_PIN;
	HAL_GPIO_Init(TOUCH_SDA_SDA_GPIO_PORT, &gpio);

	// Init I2C1 (reuse timing from I2C4)
	hi2c_ts.Instance             = I2C1;
	hi2c_ts.Init.Timing          = hbus_i2c1.Init.Timing;
	hi2c_ts.Init.OwnAddress1     = 0;
	hi2c_ts.Init.AddressingMode  = I2C_ADDRESSINGMODE_7BIT;
	hi2c_ts.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
	hi2c_ts.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
	hi2c_ts.Init.NoStretchMode   = I2C_NOSTRETCH_DISABLE;

	if(HAL_I2C_Init(&hi2c_ts) != HAL_OK)
	{
		__HAL_RCC_I2C1_CLK_DISABLE();
		return 1;
	}

	// Try primary address 0xBA, then alternate 0x28
	int res = 0;
	if(HAL_I2C_IsDeviceReady(&hi2c_ts, 0xBA, 3, 100) != HAL_OK)
	{
		if(HAL_I2C_IsDeviceReady(&hi2c_ts, 0x28, 3, 100) != HAL_OK)
			res = 2;
	}

	// Cleanup
	HAL_I2C_DeInit(&hi2c_ts);
	__HAL_RCC_I2C1_CLK_DISABLE();
	HAL_GPIO_DeInit(TOUCH_SCK_SCL_GPIO_PORT, TOUCH_SCK_SCL_PIN);
	HAL_GPIO_DeInit(TOUCH_SDA_SDA_GPIO_PORT, TOUCH_SDA_SDA_PIN);

	return res;
}

// ---------------------------------------------------------------
// Peripheral tests
// ---------------------------------------------------------------

// Ensure 5V rail is on (GPS, LoRa, etc. need it)
static void ensure_5v_on(void)
{
#ifndef REV_0_8_4_PATCH
	HAL_GPIO_WritePin(VCC_5V_ON_PORT, VCC_5V_ON, GPIO_PIN_SET);
#else
	HAL_GPIO_WritePin(VCC_5V_ON_PORT, VCC_5V_ON, GPIO_PIN_RESET);
#endif
	vcc_5v_state = 1;

	// Let rail stabilize
	HAL_Delay(50);
}

int test_gps_check(void)
{
	GPIO_InitTypeDef gpio = {0};

	printf("gps: 5v + enabling...\r\n");

	// GPS module needs the 5V rail
	ensure_5v_on();

	// Enable GPS power
	gpio.Pin   = GPS_EN_PIN;
	gpio.Mode  = GPIO_MODE_OUTPUT_PP;
	gpio.Pull  = GPIO_NOPULL;
	gpio.Speed = GPIO_SPEED_FREQ_LOW;
	HAL_GPIO_Init(GPS_EN_PORT, &gpio);
	HAL_GPIO_WritePin(GPS_EN_PORT, GPS_EN_PIN, GPIO_PIN_SET);

	// Wait for GPS to boot (M10 needs ~100ms)
	HAL_Delay(300);

	// Check GPS RX line (MCU RX <- GPS TX on PG9)
	// Idle UART = high, so if GPS is alive, its TX drives our RX high
	gpio.Pin   = GPS_RX_PIN;
	gpio.Mode  = GPIO_MODE_INPUT;
	gpio.Pull  = GPIO_PULLDOWN;
	gpio.Speed = GPIO_SPEED_FREQ_LOW;
	HAL_GPIO_Init(GPS_RX_PORT, &gpio);

	HAL_Delay(10);

	int rx_level = HAL_GPIO_ReadPin(GPS_RX_PORT, GPS_RX_PIN);
	printf("gps: rx_level=%d\r\n", rx_level);

	int res = (rx_level == GPIO_PIN_SET) ? 0 : 1;

	// Disable GPS
	HAL_GPIO_WritePin(GPS_EN_PORT, GPS_EN_PIN, GPIO_PIN_RESET);

	printf("gps: done, res=%d\r\n", res);
	return res;
}

int test_lora_check(void)
{
	LL_GPIO_InitTypeDef ll_gpio = {0};

	printf("lora: 5v + init...\r\n");

	// LoRa module needs the 5V rail
	ensure_5v_on();

	// Configure NRST (PA3) as output, drive HIGH to release from reset
	LL_GPIO_SetOutputPin(LORA_NRST_PORT, LORA_NRST);
	ll_gpio.Pin        = LORA_NRST;
	ll_gpio.Mode       = LL_GPIO_MODE_OUTPUT;
	ll_gpio.Pull       = LL_GPIO_PULL_NO;
	ll_gpio.Speed      = LL_GPIO_SPEED_LOW;
	ll_gpio.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
	LL_GPIO_Init(LORA_NRST_PORT, &ll_gpio);

	// Configure NSS (PC1) as output, drive HIGH (deselected)
	LL_GPIO_SetOutputPin(LORA_NSS_PORT, LORA_NSS);
	ll_gpio.Pin = LORA_NSS;
	LL_GPIO_Init(LORA_NSS_PORT, &ll_gpio);

	// Configure POWER (PA2) as output
	ll_gpio.Pin = LORA_POWER;
	LL_GPIO_Init(LORA_POWER_PORT, &ll_gpio);

	// Configure BUSY (PC5) as input with pull-down
	ll_gpio.Pin  = LORA_BUSY;
	ll_gpio.Mode = LL_GPIO_MODE_INPUT;
	ll_gpio.Pull = LL_GPIO_PULL_DOWN;
	LL_GPIO_Init(LORA_BUSY_PORT, &ll_gpio);

	// Power on
#ifdef LORA_POWER_INV
	LL_GPIO_ResetOutputPin(LORA_POWER_PORT, LORA_POWER);
#else
	LL_GPIO_SetOutputPin(LORA_POWER_PORT, LORA_POWER);
#endif

	printf("lora: power on, waiting...\r\n");

	// Wait for SX1262 boot, BUSY should go low after ~3ms
	HAL_Delay(50);

	int busy = LL_GPIO_IsInputPinSet(LORA_BUSY_PORT, LORA_BUSY);
	printf("lora: busy=%d\r\n", busy);

	int res = 0;
	if(busy)
	{
		HAL_Delay(100);
		busy = LL_GPIO_IsInputPinSet(LORA_BUSY_PORT, LORA_BUSY);
		printf("lora: retry busy=%d\r\n", busy);
		if(busy)
			res = 1;
	}

	// Power off
#ifdef LORA_POWER_INV
	LL_GPIO_SetOutputPin(LORA_POWER_PORT, LORA_POWER);
#else
	LL_GPIO_ResetOutputPin(LORA_POWER_PORT, LORA_POWER);
#endif

	printf("lora: done, res=%d\r\n", res);
	return res;
}

// Encoder counters — persist across calls so user sees accumulated rotation
static short enc1_count = 0;
static short enc2_count = 0;
static uchar enc1_prev  = 0xFF;
static uchar enc2_prev  = 0xFF;

// Simple quadrature direction table: prev_state(2bit) | new_state(2bit) -> +1/-1/0
static const signed char quad_table[16] = {
//  new: 00  01  10  11
	 0, -1, +1,  0,   // prev 00
	+1,  0,  0, -1,   // prev 01
	-1,  0,  0, +1,   // prev 10
	 0, +1, -1,  0    // prev 11
};

int test_encoders(uchar *enc1, uchar *enc2)
{
	GPIO_InitTypeDef gpio = {0};

	gpio.Mode  = GPIO_MODE_INPUT;
	gpio.Pull  = GPIO_PULLUP;
	gpio.Speed = GPIO_SPEED_FREQ_LOW;

	gpio.Pin = ENC1_I;
	HAL_GPIO_Init(ENC1_I_PORT, &gpio);
	gpio.Pin = ENC1_Q;
	HAL_GPIO_Init(ENC1_Q_PORT, &gpio);
	gpio.Pin = ENC2_I_PIN;
	HAL_GPIO_Init(ENC2_I_PORT, &gpio);
	gpio.Pin = ENC2_Q_PIN;
	HAL_GPIO_Init(ENC2_Q_PORT, &gpio);

	// Sample for ~1 second, counting transitions
	for(int n = 0; n < 200; n++)
	{
		HAL_Delay(5);

		uchar i1 = HAL_GPIO_ReadPin(ENC1_I_PORT, ENC1_I) ? 1 : 0;
		uchar q1 = HAL_GPIO_ReadPin(ENC1_Q_PORT, ENC1_Q) ? 1 : 0;
		uchar i2 = HAL_GPIO_ReadPin(ENC2_I_PORT, ENC2_I_PIN) ? 1 : 0;
		uchar q2 = HAL_GPIO_ReadPin(ENC2_Q_PORT, ENC2_Q_PIN) ? 1 : 0;

		uchar s1 = (i1 << 1) | q1;
		uchar s2 = (i2 << 1) | q2;

		if(enc1_prev != 0xFF)
			enc1_count += quad_table[(enc1_prev << 2) | s1];
		if(enc2_prev != 0xFF)
			enc2_count += quad_table[(enc2_prev << 2) | s2];

		enc1_prev = s1;
		enc2_prev = s2;
	}

	// Return current counts packed as signed bytes
	*enc1 = (uchar)(enc1_count & 0xFF);
	*enc2 = (uchar)(enc2_count & 0xFF);

	return 0;
}

void selftest_proc()
{
	static ulong st_timer = 0;

	if(stay_in_boot)
		return;

	// Run timer
	if(st_timer == 0)
		st_timer = sys_timer;
	else if((st_timer + 500) < sys_timer)
		st_timer = sys_timer;
	else
		return;

	if(charge_mode)
		return;

	//power_off_x(0);

	// On complete charging, execute radio firmware
	if(is_firmware_valid() == 0)
	{
		jump_to_fw(RADIO_FIRM_ADDR);
	}
}

void selftest_proc_init()
{

}
