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

extern SD_HandleTypeDef hsd_sdmmc[1];
FATFS SDFatFs;  						/* File system object for SD card logical drive */
FIL MyFile;     						/* File object */
char SDPath[4]; 						/* SD card logical drive path */

extern ulong reset_reason;
extern uchar gen_boot_reason_err;
extern uchar charge_mode;

int test_sd_card(void)
{
	GPIO_InitTypeDef  GPIO_InitStruct;

	// SD_DET - PC2 after 0.8.3 mod
	GPIO_InitStruct.Pin   = SD_DET;
	GPIO_InitStruct.Mode  = GPIO_MODE_INPUT;
	GPIO_InitStruct.Pull  = GPIO_PULLUP;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
	HAL_GPIO_Init(SD_DET_PORT, &GPIO_InitStruct);

	if(HAL_GPIO_ReadPin(SD_DET_PORT, SD_DET))
	{
		//printf("sd card na\r\n");
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
	HAL_GPIO_WritePin(SD_PWR_CNTR_PORT, SD_PWR_CNTR, GPIO_PIN_RESET);

	// Init low level driver
	if(BSP_SD_Init(0) != 0)
	{
		//printf("sd low level driver init err!\r\n");
		return 2;
	}

	//printf("sd block size: %d\r\n", (int)hsd_sdmmc->SdCard.BlockSize);
	//printf("sd num blocks: %d\r\n", (int)hsd_sdmmc->SdCard.BlockNbr);
	//printf("sd card size : %d\r\n", (hsd_sdmmc->SdCard.BlockNbr*hsd_sdmmc->SdCard.BlockNbr)/1024);

	uchar boot[512];

	// Read boot sector
	if(BSP_SD_ReadBlocks(0, (ulong *)boot, 0, 1) != 0)
	{
		//printf("sd unable to read boot sector!\r\n");
		return 3;
	}
	//print_hex_array((uchar *)(&boot[0] + 512 - 32), 32);

	// Check signature
	if((boot[510] != 0x55) || (boot[511] != 0xAA))
	{
		//printf("sd bad boot sector signature!\r\n");
		return 4;
	}

	// Init FatFS
	if(FATFS_LinkDriver(&SD_Driver, SDPath) != 0)
	{
		//printf("sd unable to init FS!\r\n");
		return 5;
	}

	if(f_mount(&SDFatFs, (TCHAR const*)SDPath, 0) != FR_OK)
	{
		//printf("sd unable to mount FS!\r\n");
		return 6;
	}

	#if 0
	FRESULT res;
	uint32_t bytesread;
	uint8_t rtext[100];

	/* Open the text file object with read access */
	if(f_open(&MyFile, "STM32.TXT", FA_READ) == FR_OK)
	{
		memset(rtext, 0, sizeof(rtext));
		res = f_read(&MyFile, rtext, sizeof(rtext), (void *)&bytesread);

		if((bytesread > 0) && (res == FR_OK))
		{
			//printf((char *)rtext);
			f_close(&MyFile);
		}
		//else
		//	printf("error read file!\r\n");
	}
	//else
	//	printf("error open file!\r\n");
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
	HAL_GPIO_WritePin(SD_PWR_CNTR_PORT, SD_PWR_CNTR, GPIO_PIN_SET);
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

void selftest_proc()
{
	if(charge_mode)
		return;

	// On complete charging, execute radio firmware
	if(is_firmware_valid() == 0)
	{
		jump_to_fw(RADIO_FIRM_ADDR);
	}
}

void selftest_proc_init()
{

}
