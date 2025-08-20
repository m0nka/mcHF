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

#include "keypad_proc.h"

#include "hw_lcd.h"

CRC_HandleTypeDef   CrcHandle;
RTC_HandleTypeDef 	RtcHandle;

extern const unsigned char dsp_idle[816];

extern ulong reset_reason;
extern uchar gen_boot_reason_err;
extern uchar stay_in_boot;

#if 0
#define USE_RTC
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
  if (GPIO_Pin == GPIO_PIN_13)
  {
	  printf("yeah\r\n");
  }
}
void EXTI15_10_IRQHandler(void)
{
  HAL_GPIO_EXTI_IRQHandler(BUTTON_WAKEUP_PIN);
}
static void TS_EXTI_Callback(void)
{
	//BSP_TS_Callback(0);
	printf("hehe\r\n");
}
//EXTI_HandleTypeDef hts_exti;
EXTI_HandleTypeDef hts_exti[1] = {0};
#ifdef USE_RTC
RTC_HandleTypeDef RtcHandle;
#define RTC_ASYNCH_PREDIV  0x7F   /* LSE as RTC clock */
#define RTC_SYNCH_PREDIV   0x00FF /* LSE as RTC clock */
#endif
static void EXTI15_10_IRQHandler_Config(void)
{
	GPIO_InitTypeDef   GPIO_InitStructure;

	#ifdef USE_RTC
	RtcHandle.Instance 				= RTC;
	RtcHandle.Init.HourFormat 		= RTC_HOURFORMAT_24;
	RtcHandle.Init.AsynchPrediv 	= RTC_ASYNCH_PREDIV;
	RtcHandle.Init.SynchPrediv 		= RTC_SYNCH_PREDIV;
	RtcHandle.Init.OutPut 			= RTC_OUTPUT_DISABLE;
	RtcHandle.Init.OutPutPolarity 	= RTC_OUTPUT_POLARITY_HIGH;
	RtcHandle.Init.OutPutType 		= RTC_OUTPUT_TYPE_OPENDRAIN;

	// Disable the write protection for RTC registers
	__HAL_RTC_WRITEPROTECTION_DISABLE	(&RtcHandle);

	// Reference manual, page 2081
	RtcHandle.Instance->CR		&= ~RTC_CR_OSEL_0;
	RtcHandle.Instance->CR 		&= ~RTC_CR_OSEL_1;
	RtcHandle.Instance->CR 		&= ~(RTC_CR_COE);
	RtcHandle.Instance->TAMPCR  |= (RTC_TAMPCR_TAMP1E);
	RtcHandle.Instance->CR 	    &= ~(RTC_CR_TSE);

	// Enable the write protection for RTC registers
	__HAL_RTC_WRITEPROTECTION_ENABLE(&RtcHandle);

	if(HAL_RTC_Init(&RtcHandle) != HAL_OK)
	{
	}
	#endif

	/* Configure PC.13 pin as the EXTI input event line in interrupt mode for both CPU1 and CPU2*/
	GPIO_InitStructure.Pin 		= GPIO_PIN_13;
	GPIO_InitStructure.Mode 	= GPIO_MODE_IT_RISING;
	GPIO_InitStructure.Pull 	= GPIO_PULLDOWN;
	//GPIO_InitStructure.Speed	= GPIO_SPEED_FREQ_HIGH;
	HAL_GPIO_Init(GPIOC, &GPIO_InitStructure);

	HAL_EXTI_D1_EventInputConfig	(EXTI_LINE13 , EXTI_MODE_IT,  ENABLE);
	(void)HAL_EXTI_GetHandle		(&hts_exti[0], EXTI_LINE_13);
	(void)HAL_EXTI_RegisterCallback	(&hts_exti[0], HAL_EXTI_COMMON_CB_ID, TS_EXTI_Callback);

	/* Enable and set EXTI lines 15 to 10 Interrupt to the lowest priority */
	HAL_NVIC_SetPriority(EXTI15_10_IRQn, 15, 0);
	HAL_NVIC_EnableIRQ(EXTI15_10_IRQn);
}
#endif

#if 0
// Via standby mode
static void power_off_x(uchar reset_reason)
{
	PWREx_WakeupPinTypeDef sPinParams;

	/* Disable used wakeup source: PWR_WAKEUP_PIN4 */
	HAL_PWR_DisableWakeUpPin(PWR_WAKEUP_PIN2);

	/* Clear all related wakeup flags */
	HAL_PWREx_ClearWakeupFlag(PWR_WAKEUP_PIN_FLAGS);

	/* Enable WakeUp Pin PWR_WAKEUP_PIN4 connected to PC.13 User Button */
	sPinParams.WakeUpPin    = PWR_WAKEUP_PIN2;
	sPinParams.PinPolarity  = PWR_PIN_POLARITY_LOW;
	sPinParams.PinPull      = PWR_PIN_NO_PULL;
	HAL_PWREx_EnableWakeUpPin(&sPinParams);

	if(reset_reason != RESET_POWER_OFF)
	{
		HAL_GPIO_WritePin(POWER_LED_PORT,POWER_LED, 0);									// LED Off
		HAL_GPIO_WritePin(LCD_BL_CTRL_GPIO_PORT, LCD_BL_CTRL_PIN, GPIO_PIN_RESET);		// backlight on
	}

	/* Enter the Standby mode */
	HAL_PWR_EnterSTANDBYMode();
}
#else
void power_off_x(uchar reset_reason)
{
	// LED off
	HAL_GPIO_WritePin(POWER_LED_PORT, POWER_LED, 0);

	// Backlight off
	HAL_GPIO_WritePin(LCD_BL_CTRL_GPIO_PORT, LCD_BL_CTRL_PIN, GPIO_PIN_RESET);

	// Regulator off
	HAL_GPIO_WritePin(POWER_HOLD_PORT, POWER_HOLD, 0);

	// Stall
	while(1);
}
#endif

#if 0
void power_off(void)
{
	GPIO_InitTypeDef  GPIO_InitStruct;

	printf("...power off\r\n");
	HAL_Delay(300);

	#ifndef REV_8_2
	// 5V OFF
	HAL_GPIO_WritePin(GPIOG, GPIO_PIN_10, GPIO_PIN_SET);

	// 8V OFF
	HAL_GPIO_WritePin(GPIOG, GPIO_PIN_9, GPIO_PIN_SET);

	// PG11 is power hold
	GPIO_InitStruct.Pin   = GPIO_PIN_11;
	GPIO_InitStruct.Mode  = GPIO_MODE_INPUT;	//GPIO_MODE_OUTPUT_PP;
	//GPIO_InitStruct.Pull  = GPIO_PULLUP;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
	HAL_GPIO_Init(GPIOG, &GPIO_InitStruct);
	//HAL_GPIO_WritePin(GPIOG, GPIO_PIN_11, 1);	// drop power
	#else
	// 5V OFF
	HAL_GPIO_WritePin(VCC_5V_ON_PORT, VCC_5V_ON, GPIO_PIN_RESET);

	// Drop power
	HAL_GPIO_WritePin(POWER_HOLD_PORT, POWER_HOLD, 0);
	#endif

	// Shouldn't be here!
	while(1);
}
#endif

void jump_to_fw(uint32_t SubDemoAddress)
{
	//printf("jump\r\n");

	/* Store the address of the Sub Demo binary */
	HAL_PWR_EnableBkUpAccess();
	WRITE_REG(BKP_REG_RESET_REASON, RESET_JUMP_TO_FW);
	HAL_PWR_DisableBkUpAccess();

	/* Disable LCD */
	#ifdef USE_EM_WIN
	LCD_Off();
	#endif

	//GUI_Delay(200);		// causes re-entrance
	//HAL_Delay(200);

	/* Disable the MPU */
	HAL_MPU_Disable();

	/* DeInit Storage */
	//Storage_DeInit();

	// ToDo: stop uart driver..
	//       other hw ?

	//printf("Jump to radio(0x%08x)...\r\n", (int)SubDemoAddress);

	/* Disable and Invalidate I-Cache */
	SCB_DisableICache();
	SCB_InvalidateICache();

	/* Disable, Clean and Invalidate D-Cache */
	SCB_DisableDCache();
	SCB_CleanInvalidateDCache();

	//HAL_Delay(50);

	//printf("reset\r\n");
	HAL_NVIC_SystemReset();
}

#if 0
uchar update_radio(void)
{
	uchar res = 0;

	__HAL_RCC_CRC_CLK_ENABLE();

	CrcHandle.Instance = CRC;
	CrcHandle.Init.DefaultPolynomialUse    = DEFAULT_POLYNOMIAL_DISABLE;
	CrcHandle.Init.DefaultInitValueUse     = DEFAULT_INIT_VALUE_ENABLE;
	CrcHandle.Init.InputDataInversionMode  = CRC_INPUTDATA_INVERSION_NONE;
	CrcHandle.Init.OutputDataInversionMode = CRC_OUTPUTDATA_INVERSION_DISABLE;
	CrcHandle.InputDataFormat              = CRC_INPUTDATA_FORMAT_WORDS;

	// Init CRC unit
	if(HAL_CRC_Init(&CrcHandle) != HAL_OK)
	{
		//printf("error crc unit!\r\n");
		return 1;
	}

	// Open flash file from SD card
	if(f_open(&MyFile, "radio.bin", FA_READ) != FR_OK)
	{
		//printf("error open file!\r\n");
		return 2;
	}

	ulong fs = f_size(&MyFile);
	//printf("file size: %d bytes\r\n", (int)fs);

	// Remove checksum
	fs -= 4;

	if(f_lseek(&MyFile, fs) != FR_OK)
	{
		//printf("error chksum location!\r\n");
		res = 3;
		goto fw_upd_clean_up;
	}

	ulong chk  = 0;
	ulong read = 0;

	if(f_read(&MyFile, &chk, 4, (void *)&read) != FR_OK)
	{
		//printf("error chksum read!\r\n");
		res = 4;
		goto fw_upd_clean_up;
	}

	if(read != 4)
	{
		//printf("error chksum size!\r\n");
		res = 5;
		goto fw_upd_clean_up;
	}
	//printf("file crc: 0x%x\r\n", chk);

	// Roll back
	if(f_lseek(&MyFile, 0) != FR_OK)
	{
		//printf("error rollback!\r\n");
		res = 6;
		goto fw_upd_clean_up;
	}

	ulong blocks = fs/512;
	ulong leftov = fs%512;
	ulong calc_crc = 0;

	//printf("chunks count: %d\r\n", blocks);
	//printf("extra bytes: %d\r\n", leftov);

	uchar *temp = malloc(512);
	if(temp == NULL)
	{
		//printf("error alloc temp block!\r\n");
		res = 7;
		goto fw_upd_clean_up;
	}

	// First
	if(f_read(&MyFile, temp, 512, (void *)&read) != FR_OK)
	{
		//printf("error chunk read!\r\n");
		free(temp);
		res = 8;
		goto fw_upd_clean_up;
	}

	if(read != 512)
	{
		//printf("error first chunk size!\r\n");
		free(temp);
		res = 9;
		goto fw_upd_clean_up;
	}

	calc_crc = HAL_CRC_Calculate(&CrcHandle, (uint32_t*)temp, 512/4);
	blocks--;

	// All chunks
	for(ulong i = 0; i < blocks; i++)
	{
		if(f_read(&MyFile, temp, 512, (void *)&read) != FR_OK)
		{
			//printf("error chunk read!\r\n");
			free(temp);
			res = 10;
			goto fw_upd_clean_up;
		}

		if(read != 512)
		{
			//printf("error next(%d) chunk size!\r\n", i);
			free(temp);
			res = 11;
			goto fw_upd_clean_up;
		}

		calc_crc = HAL_CRC_Accumulate(&CrcHandle, (uint32_t *)temp, 512/4);
	}

	// Leftovers
	if(leftov)
	{
		if(f_read(&MyFile, temp, leftov, (void *)&read) != FR_OK)
		{
			//printf("error leftover read!\r\n");
			free(temp);
			res = 12;
			goto fw_upd_clean_up;
		}

		if(read != leftov)
		{
			//printf("error last chunk size!\r\n");
			free(temp);
			res = 13;
			goto fw_upd_clean_up;
		}

		calc_crc = HAL_CRC_Accumulate(&CrcHandle, (uint32_t *)temp, leftov/4);
	}
	free(temp);
	//printf("calc checksum: 0x%x\r\n", calc_crc);

	// Test CRC
	if(chk != calc_crc)
	{
		//printf("crc mismatch!\r\n");
		res = 14;
		goto fw_upd_clean_up;
	}

	// Flash the file
	if(hw_flash_program_file(&MyFile, RADIO_FIRM_ADDR) != 0)
	{
		//printf("error writing file!\r\n");
		res = 15;
	}

fw_upd_clean_up:
	f_close(&MyFile);
	return res;
}
#endif

#if 0
static int boot_dsp_core(ulong *checksum)
{
	int32_t timeout = 0xFFFF;
	int 	i, res = 0;
	ulong  chk;

	// Checksum of header
	for(i = 0, chk = 0; i < sizeof(dsp_idle); i++)
	{
		chk += dsp_idle[i];
	}

	if(checksum)
		*checksum = chk;

	// Copy CM4 code to D2_SRAM memory
	memcpy((void *)D2_AXISRAM_BASE, dsp_idle, sizeof(dsp_idle));

	// Remap M4 core boot address (overwrites fuses)
	HAL_RCCEx_EnableBootCore(RCC_BOOT_C2);

	// Wait until CPU2 boots and enters in stop mode or timeout
	while((__HAL_RCC_GET_FLAG(RCC_FLAG_D2CKRDY) != RESET) && (timeout-- > 0))
	{
		__asm("nop");
	}

	if(timeout < 0)
		res = 1;

	// Markup in memory blank DSP Firmware
	WRITE_REG(BKP_REG_DSP_ID, DSP_IDLE);

	return res;
}
#endif

#if 0
static uchar load_default_dsp_core(uchar is_default)
{
	FRESULT 	res  = 0;
	uint32_t	read = 0;
	ulong 		curr_addr = 0;
	char		dsp_name[32];
	ulong 		curr_id;

	if(is_default)
		curr_id = DSP_LOADED_CLINT;
	else
		curr_id = READ_REG(BKP_REG_DSP_ID);

	switch(curr_id)
	{
		case DSP_LOADED_CLINT:
			strcpy(dsp_name, "clint.bin");
			//curr_id = 0;
			break;

		case DSP_LOADED_UHSDR:
			strcpy(dsp_name, "uhsdr.bin");
			//curr_id = 0;
			break;

		default:
			strcpy(dsp_name, "clint.bin");
			break;
	}
	printf("will use file: %s\r\n", dsp_name);

	// Open the file
	if(f_open(&MyFile, dsp_name, FA_READ) != FR_OK)
	{
		printf("error open file!\r\n");
		return 1;
	}

#if 1
	uchar *temp = malloc(512);
	if(temp == NULL)
	{
		printf("error alloc temp block!\r\n");
		res = 2;
		goto dsp_upd_clean_up;
	}

	// Get size
	ulong fs = f_size(&MyFile);

	fs -= 512;
	ulong cnt = fs/512;
	ulong lef = fs%512;

	if((cnt == 0) && (lef > 0))
		goto last_chunk;

	while(cnt)
	{
		// Next
		if(f_read(&MyFile, temp, 512, (void *)&read) != FR_OK)
		{
			printf("error chunk read!\r\n");
			res = 3;
			goto dsp_upd_clean_up;
		}

		if(read != 512)
		{
			printf("error first chunk size!\r\n");
			res = 4;
			goto dsp_upd_clean_up;
		}

		// Copy first chunk
		memcpy((void *)(D2_AXISRAM_BASE + curr_addr), (void *)temp, 512);
		curr_addr += 512;
		cnt--;
	}

last_chunk:

	if(lef == 0)
		goto dsp_upd_clean_up;

	// Last
	if(f_read(&MyFile, temp, lef, (void *)&read) != FR_OK)
	{
		printf("error chunk read!\r\n");
		res = 5;
		goto dsp_upd_clean_up;
	}

	if(lef != read)
	{
		printf("error first chunk size!\r\n");
		res = 6;
		goto dsp_upd_clean_up;
	}

	// Copy first chunk
	memcpy((void *)(D2_AXISRAM_BASE + curr_addr), (void *)temp, lef);

	// ToDo: Checksum test
	//..

    #else
	if(curr_id == 0)
	{
		// 0x081D0000, 0x30000
		if(hw_flash_program_file(&MyFile, 0x081D0000) != 0)
		{
			printf("error flashing dsp file!\r\n");
		}
	}
	else
	{
		// 0x081A0000, 0x30000
		if(hw_flash_program_file(&MyFile, 0x081A0000) != 0)
		{
			printf("error flashing dsp file!\r\n");
		}
	}
    #endif

	printf("write dsp id: %d\r\n", (int)curr_id);

	// Markup in memory that is valid
	WRITE_REG(BKP_REG_DSP_ID, curr_id);

dsp_upd_clean_up:
	free(temp);
	f_close(&MyFile);
	return res;
}
#endif



void early_backup_domain_init(void)
{
	/* Enable Back up SRAM */
	/* Enable write access to Backup domain */
	PWR->CR1 |= PWR_CR1_DBP;
	while((PWR->CR1 & PWR_CR1_DBP) == RESET)
	{
		__asm("nop");
	}

	// Enable BKPRAM clock
	__HAL_RCC_BKPRAM_CLK_ENABLE();
}

void bt_hw_power(void)
{
	GPIO_InitTypeDef  gpio_init_structure;

	gpio_init_structure.Pull  = GPIO_NOPULL;

	// BT Power Control
	gpio_init_structure.Pin   = RFM_DIO2;
	gpio_init_structure.Mode  = GPIO_MODE_OUTPUT_PP;
	HAL_GPIO_Init(RFM_DIO2_PORT, &gpio_init_structure);

	// Power off
	HAL_GPIO_WritePin(RFM_DIO2_PORT, RFM_DIO2, GPIO_PIN_SET);
}

void lora_hw_power(void)
{
	// ToDo: Make sure LORA module is off
}

// Make sure TX is disabled
//
void mchf_pro_board_tx_disable(void)
{
	GPIO_InitTypeDef  gpio_init_structure;

	HAL_GPIO_WritePin(DAC1_OUTX_PORT,  	DAC1_OUT1,  GPIO_PIN_RESET);
	HAL_GPIO_WritePin(DAC1_OUTX_PORT,  	DAC1_OUT2,  GPIO_PIN_RESET);
	HAL_GPIO_WritePin(PTT_PIN_PORT, 	PTT_PIN, 	GPIO_PIN_RESET);

	gpio_init_structure.Mode  = GPIO_MODE_OUTPUT_PP;
	gpio_init_structure.Pull  = GPIO_NOPULL;

	gpio_init_structure.Pin   = PTT_PIN;
	HAL_GPIO_Init(PTT_PIN_PORT, &gpio_init_structure);

	gpio_init_structure.Pin   = DAC1_OUT1|DAC1_OUT2;
	HAL_GPIO_Init(DAC1_OUTX_PORT, &gpio_init_structure);
}

void mchf_pro_board_init(void)
{
	// Use sharing, as DSP core might be running after reset
	printf_init(1);
	printf("..........................................................\r\n");
	printf("..........................................................\r\n");
	printf("-->%s v: %d.%d  \r\n", DEVICE_STRING, MCHF_L_VER_RELEASE, MCHF_L_VER_BUILD);

	// Initialise the screen
	hw_lcd_gpio_init();
	hw_lcd_reset();

	// BT module off
	bt_hw_power();

	// LORA module off
	lora_hw_power();

	// Transmitter off
	mchf_pro_board_tx_disable();

	#ifdef CONTEXT_IPC_PROC
	ipc_proc_init();
	#endif

	// Seems to be important
	//--early_backup_domain_init();
}

// Critical HW init on start
//
// - missing pullups or pulldowns ? Well, deal with it here
//
void critical_hw_init_and_run_fw(void)
{
	GPIO_InitTypeDef  GPIO_InitStruct;

	// Enable RTC back-up registers access
	//__HAL_RCC_RTC_ENABLE();
	//__HAL_RCC_RTC_CLK_ENABLE();
	//HAL_PWR_EnableBkUpAccess();

    //reset_reason = READ_REG(BKP_REG_RESET_REASON);
	//WRITE_REG(BKP_REG_RESET_REASON, RESET_CLEAR);

	// PG11 is power hold
	GPIO_InitStruct.Pin   = POWER_HOLD;
	GPIO_InitStruct.Mode  = GPIO_MODE_OUTPUT_PP;
	GPIO_InitStruct.Pull  = GPIO_PULLDOWN;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
	HAL_GPIO_Init(POWER_HOLD_PORT, &GPIO_InitStruct);

	// Power off request from firmware
	//if(reset_reason == RESET_POWER_OFF)
	//{
		//printf("power off request from radio...\r\n");
		//HAL_Delay(300);
		//power_off_x(RESET_POWER_OFF);
	//}

	// PF6 is LED - ack boot up, in firmware should be ambient sensor, not GPIO!
	GPIO_InitStruct.Pin   = POWER_LED;
	HAL_GPIO_Init(POWER_LED_PORT, &GPIO_InitStruct);

	// Hold power
	HAL_GPIO_WritePin(POWER_LED_PORT,POWER_LED, 1);		// led on
	HAL_GPIO_WritePin(POWER_HOLD_PORT,POWER_HOLD, 1);	// hold power, high

	// Keep 5V and 8V rails off
	GPIO_InitStruct.Pin   = VCC_5V_ON;
	HAL_GPIO_Init(VCC_5V_ON_PORT, &GPIO_InitStruct);

	#ifndef REV_0_8_4_PATCH
	HAL_GPIO_WritePin(VCC_5V_ON_PORT, VCC_5V_ON, 0);
	#else
	HAL_GPIO_WritePin(VCC_5V_ON_PORT, VCC_5V_ON, 1);
	#endif

	//HAL_PWR_DisableBkUpAccess();

	if(keypad_proc_is_held_on_start())
		return;

	//if(reset_reason == RESET_JUMP_TO_FW)
	//{
		// Reinitialize the Stack pointer
		__set_MSP(*(__IO uint32_t*) RADIO_FIRM_ADDR);

		// Jump to application address
		((pFunc) (*(__IO uint32_t*) (RADIO_FIRM_ADDR + 4)))();
	//}
}

