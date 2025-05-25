/**
  ******************************************************************************
  * @file    bsp.c
  * @author  MCD Application Team
  * @brief   This file provides the kernel bsp functions
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2019 STMicroelectronics.
  * All rights reserved.</center></h2>
  *
  * This software component is licensed by ST under Ultimate Liberty license
  * SLA0044, the "License"; You may not use this file except in compliance with
  * the License. You may obtain a copy of the License at:
  *                             www.st.com/SLA0044
  *
  ******************************************************************************
  */

#include "main.h"
#include "mchf_pro_board.h"

/* Includes ------------------------------------------------------------------*/
#include "bsp.h"

#include "ipc_proc.h"
#include "audio_proc.h"
#include "touch_proc.h"
#include "rotary_proc.h"
#include "vfo_proc.h"
#include "band_proc.h"
#include "trx_proc.h"
#include "touch_proc.h"
#include "ui_proc.h"
#include "bms_proc.h"
#include "keypad_proc.h"
#include "radio_init.h"

#include "version.h"

//#define HSEM_ID_0                       (0U) /* HW semaphore 0*/

#define MAX_FLASH_WRITE_FAILURE         10

#define USARTx                          USART2

#ifndef EXT_FLASH_SIZE
#define EXT_FLASH_SIZE                  MT25TL01G_FLASH_SIZE
#endif /* EXT_FLASH_SIZE */
#ifndef EXT_FLASH_SECTOR_SIZE
#define EXT_FLASH_SECTOR_SIZE           MT25TL01G_SECTOR_SIZE
#endif /* EXT_FLASH_SECTOR_SIZE */
#ifndef EXT_FLASH_SUBSECTOR_SIZE
#define EXT_FLASH_SUBSECTOR_SIZE        (2 * MT25TL01G_SUBSECTOR_SIZE)
#endif /* EXT_FLASH_SUBSECTOR_SIZE */
#ifndef EXT_FLASH_PAGE_SIZE
#define EXT_FLASH_PAGE_SIZE             MT25TL01G_PAGE_SIZE
#endif /* EXT_FLASH_PAGE_SIZE */

#define FLASH_BURST_WIDTH               256 /* in bits */

// #define __RAM_CODE_SECTION __attribute__ ((section ("flasher_code_section"))) __RAM_FUNC
#define __RAM_CODE_SECTION __attribute__ ((section ("flasher_code_section")))
// #define __RAM_CODE_SECTION __RAM_FUNC
// #define __RAM_CODE_SECTION

/* Private variables ---------------------------------------------------------*/
/* Private function prototypes -----------------------------------------------*/
__RAM_CODE_SECTION
HAL_StatusTypeDef HAL_FLASH_Unlock(void);
__RAM_CODE_SECTION
uint32_t HAL_FLASH_GetError(void);
__RAM_CODE_SECTION
HAL_StatusTypeDef HAL_FLASH_Program(uint32_t TypeProgram, uint32_t Address, uint32_t DataAddress);
__RAM_CODE_SECTION
HAL_StatusTypeDef HAL_FLASHEx_Erase(FLASH_EraseInitTypeDef *pEraseInit, uint32_t *SectorError);
__RAM_CODE_SECTION
HAL_StatusTypeDef HAL_FLASHEx_Unlock_Bank1(void);
__RAM_CODE_SECTION
HAL_StatusTypeDef HAL_FLASHEx_Unlock_Bank2(void);

#if 0
/* Private functions ---------------------------------------------------------*/
__STATIC_INLINE void my_data_copy(void *dst, void *src, size_t size)
{
  printf("Copying data from 0x%08X to 0x%08X - size is %u\n", (unsigned int )src, (unsigned int )dst, size);
  memcpy(dst, src, size);
}
#endif

#if 0
/* copy all bytes between s (inclusive) and e (exclusive) to d */
/* copy the bytes from ITCMROM_region to ITCMRAM_region */
static void Copy_EXT_RAM_data(void)
{
#if defined ( __GNUC__ )
	extern char EXT_RAM_START asm("EXT_RAM_START");
	extern char EXT_RAM_END asm("EXT_RAM_END");
	extern char EXT_ROM_START asm("EXT_ROM_START");
	size_t size = (size_t) (&EXT_RAM_END - &EXT_RAM_START);
  my_data_copy(&EXT_RAM_START, &EXT_ROM_START, size);
#elif defined ( __CC_ARM )
	extern uint32_t Load$$RO_EXT_RAM$$RO$$Base[];
	extern uint32_t Load$$RO_EXT_RAM$$RO$$Length[];
	extern uint32_t Image$$RO_EXT_RAM$$RO$$Base[];
	my_data_copy((void *) Image$$RO_EXT_RAM$$RO$$Base, Load$$RO_EXT_RAM$$RO$$Base, (unsigned long) Load$$RO_EXT_RAM$$RO$$Length);
#elif defined (__ICCARM__)
	#pragma segment="EXT_RAM"
	#pragma segment="EXT_ROM"
	size_t size = (size_t)((uint32_t)__sfe("EXT_ROM") - (uint32_t)__sfb("EXT_ROM"));
	my_data_copy(__sfb("EXT_RAM"), __sfb("EXT_ROM"), size);
#endif
}
#endif

/**
  * @brief  Gets the page of a given address
  * @param  Addr: Address of the FLASH Memory
  * @retval The page of a given address
  */
static inline uint32_t GetPage(uint32_t Addr)
{
  if IS_FLASH_PROGRAM_ADDRESS_BANK1(Addr)
  {
    /* Bank 1 */
    return (Addr - FLASH_BANK1_BASE) / FLASH_SECTOR_SIZE;
  }
  else
  {
    /* Bank 2 */
    return (Addr - FLASH_BANK2_BASE) / FLASH_SECTOR_SIZE;
  }
}

/**
  * @brief  Gets the bank of a given address
  * @param  Addr: Address of the FLASH Memory
  * @retval The bank of a given address
  */
static inline uint32_t GetBank(uint32_t Addr)
{
  uint32_t bank = 0;

  if (READ_BIT(SYSCFG->UR0, SYSCFG_UR0_BKS) == 0)
  {
  	/* No Bank swap */
    if IS_FLASH_PROGRAM_ADDRESS_BANK1(Addr)
    {
      bank = FLASH_BANK_1;
    }
    else
    {
      bank = FLASH_BANK_2;
    }
  }
  else
  {
  	/* Bank swap */
    if IS_FLASH_PROGRAM_ADDRESS_BANK1(Addr)
    {
      bank = FLASH_BANK_2;
    }
    else
    {
      bank = FLASH_BANK_1;
    }
  }

  return bank;
}

// ToDo: measure this delay and adjust !!
void bsp_delay_100uS(ulong delay)
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

static void bsp_backlight_init(void)
{
	  GPIO_InitTypeDef  gpio_init_structure;

	  /* LCD_BL_CTRL GPIO configuration */
	  //LCD_BL_CTRL_GPIO_CLK_ENABLE();

	  gpio_init_structure.Pin       = LCD_BL_CTRL_PIN;
	  gpio_init_structure.Mode      = GPIO_MODE_OUTPUT_PP;
	  gpio_init_structure.Speed     = GPIO_SPEED_FREQ_HIGH;

	  HAL_GPIO_Init(LCD_BL_CTRL_GPIO_PORT, &gpio_init_structure);

	  /* Assert back-light LCD_BL_CTRL pin */
	  //HAL_GPIO_WritePin(LCD_BL_CTRL_GPIO_PORT, LCD_BL_CTRL_PIN, GPIO_PIN_SET);
}

//*----------------------------------------------------------------------------
//* Function Name       : tasks_initial_init
//* Object              :
//* Notes    			: All hardware init that needs to be done before the
//* Notes   			: OS start here
//* Notes    			:
//* Context    			: CONTEXT_RESET
//*----------------------------------------------------------------------------
void tasks_pre_os_init(void)
{
	#ifdef CONTEXT_BMS
	bms_proc_hw_init();
	#endif

	#ifdef CONTEXT_ROTARY
	rotary_proc_hw_init();
	#endif

  	#ifdef CONTEXT_IPC_PROC
	ipc_proc_init();
  	#endif

  	#ifdef CONTEXT_AUDIO
	audio_proc_hw_init();
  	#endif

	#ifdef CONTEXT_TOUCH
	touch_proc_hw_init();
	#endif

	#ifdef CONTEXT_VFO
	vfo_proc_hw_init();
	#endif

	#ifdef CONTEXT_BAND
	band_proc_hw_init();
	#endif

	#ifdef CONTEXT_TRX
	trx_proc_hw_init();
	#endif

	#ifdef CONTEXT_KEYPAD
	keypad_proc_init();
	#endif
}

#ifdef CONTEXT_ICC
// M4 core Keyer IRQ setup
static void EXTI23_IRQHandler_Config(void)
{
	GPIO_InitTypeDef   GPIO_InitStructure;

	// Configure PC.13 pin as the EXTI input event line in interrupt mode for both CPU1 and CPU2
	GPIO_InitStructure.Mode 	= GPIO_MODE_IT_FALLING;
	GPIO_InitStructure.Pull 	= GPIO_PULLUP;
	GPIO_InitStructure.Speed 	= GPIO_SPEED_FREQ_VERY_HIGH;

	GPIO_InitStructure.Pin 		= PADDLE_DIT;
	HAL_GPIO_Init(PADDLE_DIT_PIO, &GPIO_InitStructure);

	GPIO_InitStructure.Pin 		= PADDLE_DAH;
	HAL_GPIO_Init(PADDLE_DAH_PIO, &GPIO_InitStructure);

	// Configure the second CPU (CM4) EXTI line for IT
	HAL_EXTI_D2_EventInputConfig(EXTI_LINE2 , EXTI_MODE_IT,  ENABLE);
	HAL_EXTI_D2_EventInputConfig(EXTI_LINE3 , EXTI_MODE_IT,  ENABLE);
}
#endif

// 5V, 8V
static void power_cntr_init(void)
{
	GPIO_InitTypeDef  gpio_init_structure;

	gpio_init_structure.Mode  = GPIO_MODE_OUTPUT_PP;
	gpio_init_structure.Pull  = GPIO_PULLDOWN;
	gpio_init_structure.Speed = GPIO_SPEED_FREQ_LOW;

	// Not needed because of RF board power mod!
	#if 0
	// -----------------------------------------------------
	// -----------------------------------------------------
	// When no batteries installed, there will be missing
	// LOAD_16V rail. We need to test and enable the charging
	// regulator (BMS should do that!!!)
	//
	// Temporary put it on, for testing, but with batteries
	// has to be removed !!!
	//
	// CHGR_ON is PD4, active low
	gpio_init_structure.Pin   = GPIO_PIN_4;
	HAL_GPIO_Init(GPIOD, &gpio_init_structure);
	// ON
	HAL_GPIO_WritePin(GPIOD, GPIO_PIN_4, GPIO_PIN_RESET);
	printf("######## CHARGER IS ON !!! ########\r\n");
	// -----------------------------------------------------
	// -----------------------------------------------------
	#endif

	// 5V on is PG10
	gpio_init_structure.Pin   = VCC_5V_ON;
	HAL_GPIO_Init(VCC_5V_ON_PORT, &gpio_init_structure);

	// 5V ON on start
	#if 0
	HAL_GPIO_WritePin(VCC_5V_ON_PORT, VCC_5V_ON, GPIO_PIN_RESET);
	#else
	HAL_GPIO_WritePin(VCC_5V_ON_PORT, VCC_5V_ON, GPIO_PIN_SET);
	#endif

	#if 0
	// 8V on is PG9
	gpio_init_structure.Pin   = GPIO_PIN_9;
	HAL_GPIO_Init(GPIOG, &gpio_init_structure);
	// 8V ON on start (actually 6V after mod)
	HAL_GPIO_WritePin(GPIOG, GPIO_PIN_9, GPIO_PIN_RESET);
	#endif
}

#if 0
// via main 3V regulator
void power_off(void)
{
	printf("...power off\r\n");

	#ifndef REV_8_2
	GPIO_InitTypeDef  GPIO_InitStruct;
	vTaskDelay(300);

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

	// Charger off
	HAL_GPIO_WritePin(CHGR_ON_PORT, CHGR_ON, 0);
	HAL_GPIO_WritePin(CC_CV_PORT,   CC_CV,   0);

	// Balancer off
	HAL_GPIO_WritePin(BAL13_PORT, BAL1_ON|BAL2_ON|BAL3_ON, 	GPIO_PIN_RESET);
	HAL_GPIO_WritePin(BAL4_PORT,  BAL4_ON, 					GPIO_PIN_RESET);

	// Give time for C71 to discharge
	vTaskDelay(1000);

	// Enter reason for reset
	WRITE_REG(BKP_REG_RESET_REASON, RESET_POWER_OFF);
	HAL_PWR_DisableBkUpAccess();

	// Power off, so CPU when enter bootloader after restart
	// will be running on left over juice in power caps
	HAL_GPIO_WritePin(POWER_HOLD_PORT,POWER_HOLD, 0);

	// Restart to bootloader
	NVIC_SystemReset();
	#endif
}
#endif

// Via stop mode
void bsp_power_off(void)
{
	//printf("power off in\r\n");

	// Stop all repaints
	#ifdef CONTEXT_VIDEO
	ui_proc_power_cleanup();
	#endif

	// Safely stop OS
	portDISABLE_INTERRUPTS();

	// Tasks hw cleanup
	audio_proc_power_cleanup();
	band_proc_power_cleanup();
	rotary_proc_power_cleanup();
	touch_proc_power_cleanup();
	trx_proc_power_clean_up();
	vfo_proc_power_cleanup();
	radio_init_save_before_off();
	#ifdef CONTEXT_BMS
	bms_proc_power_cleanup();
	#endif

	HAL_Delay(3000);

	#if 0
	// Enter reason for reset, so the bootloader doesn't power back on the radio
	WRITE_REG(BKP_REG_RESET_REASON, RESET_POWER_OFF);
	HAL_PWR_DisableBkUpAccess();
	// Restart to bootloader
	NVIC_SystemReset();
	#else
	HAL_GPIO_WritePin(LCD_BL_CTRL_GPIO_PORT, LCD_BL_CTRL_PIN, GPIO_PIN_RESET);
	LL_GPIO_ResetOutputPin(POWER_HOLD_PORT, POWER_HOLD);
	#endif
}

#if 0
void bal_control_init(void)
{
	GPIO_InitTypeDef  gpio_init_structure;

	gpio_init_structure.Mode  = GPIO_MODE_OUTPUT_PP;
	gpio_init_structure.Pull  = GPIO_NOPULL;
	gpio_init_structure.Speed = GPIO_SPEED_FREQ_LOW;

	// BAL1 is PG6, BAL2 is PG3, BAL3 is PG2
	gpio_init_structure.Pin   = BAL1_ON|BAL2_ON|BAL3_ON;
	HAL_GPIO_Init(BAL13_PORT, &gpio_init_structure);

	// BAL4 is PD7
	gpio_init_structure.Pin   = BAL4_ON;
	HAL_GPIO_Init(BAL4_PORT, &gpio_init_structure);

	// Force OFF
	HAL_GPIO_WritePin(BAL13_PORT, BAL1_ON|BAL2_ON|BAL3_ON, 	GPIO_PIN_RESET);
	HAL_GPIO_WritePin(BAL4_PORT,  BAL4_ON, 					GPIO_PIN_RESET);
}
#endif

static void ptt_init(void)
{
	GPIO_InitTypeDef  gpio_init_structure;

	gpio_init_structure.Mode  = GPIO_MODE_OUTPUT_PP;
	gpio_init_structure.Pull  = GPIO_NOPULL;
	gpio_init_structure.Speed = GPIO_SPEED_FREQ_HIGH;

	// PTT line
	gpio_init_structure.Pin   = PTT_PIN;
	HAL_GPIO_Init(PTT_PIN_PORT, &gpio_init_structure);

	// RX on start
	HAL_GPIO_WritePin(PTT_PIN_PORT, PTT_PIN, GPIO_PIN_RESET);
}

void bsp_hold_power(void)
{
#if 0
	GPIO_InitTypeDef  GPIO_InitStruct;

	__HAL_RCC_GPIOC_CLK_ENABLE();

	HAL_GPIO_WritePin(POWER_HOLD_PORT,POWER_HOLD, 1);	// hold power

	GPIO_InitStruct.Pin   = POWER_HOLD;
	GPIO_InitStruct.Mode  = GPIO_MODE_OUTPUT_PP;
	GPIO_InitStruct.Pull  = GPIO_PULLDOWN;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;

	HAL_GPIO_Init(POWER_HOLD_PORT, &GPIO_InitStruct);
#else
	LL_GPIO_InitTypeDef 		GPIO_InitStruct = {0};

	// This is first ever call, so enable gpio clock
	LL_AHB4_GRP1_EnableClock(LL_AHB4_GRP1_PERIPH_GPIOC);

	// Hold the regulator line
	LL_GPIO_SetOutputPin(POWER_HOLD_PORT, POWER_HOLD);

	GPIO_InitStruct.Pin 	= POWER_HOLD;
	GPIO_InitStruct.Mode 	= LL_GPIO_MODE_OUTPUT;
	GPIO_InitStruct.Pull 	= LL_GPIO_PULL_DOWN;
	LL_GPIO_Init(POWER_HOLD_PORT, &GPIO_InitStruct);


#endif
}

void bsp_gpio_clocks_on(void)
{
	// All GPIO clocks on
	__HAL_RCC_GPIOA_CLK_ENABLE();
	__HAL_RCC_GPIOB_CLK_ENABLE();
	__HAL_RCC_GPIOC_CLK_ENABLE();
	__HAL_RCC_GPIOD_CLK_ENABLE();
	__HAL_RCC_GPIOE_CLK_ENABLE();
	__HAL_RCC_GPIOF_CLK_ENABLE();
	__HAL_RCC_GPIOG_CLK_ENABLE();
	__HAL_RCC_GPIOH_CLK_ENABLE();
	__HAL_RCC_GPIOI_CLK_ENABLE();
}

#if 0
static void MX_CRC_Init(void)
{
	static CRC_HandleTypeDef hcrc;

	hcrc.Instance = CRC;
	hcrc.Init.DefaultPolynomialUse = DEFAULT_POLYNOMIAL_ENABLE;
	hcrc.Init.DefaultInitValueUse = DEFAULT_INIT_VALUE_ENABLE;
	hcrc.Init.InputDataInversionMode = CRC_INPUTDATA_INVERSION_NONE;
	hcrc.Init.OutputDataInversionMode = CRC_OUTPUTDATA_INVERSION_DISABLE;
	hcrc.InputDataFormat = CRC_INPUTDATA_FORMAT_BYTES;

	if(HAL_CRC_Init(&hcrc) != HAL_OK)
	{
		Error_Handler(342);
	}
}
#endif

uint8_t bsp_config(void)
{
	//LL_GPIO_InitTypeDef 		GPIO_InitStruct = {0};

	// Enable CRC to Unlock GUI
	__HAL_RCC_CRC_CLK_ENABLE();
	//--MX_CRC_Init();

	printf_init(1);
	printf("----------------------------------------------  \r\n");
	printf("-->%s v: %d.%d.%d\r\n", DEVICE_STRING, MCHF_R_VER_MINOR, MCHF_R_VER_RELEASE, MCHF_R_VER_BUILD);

	power_cntr_init();

	ptt_init();

	bsp_backlight_init();

	// DSP core Keyer IRQ
	#ifdef CONTEXT_ICC
	EXTI23_IRQHandler_Config();
	#endif

	// Task hw basic init (after LCD Reset!)
	//--tasks_pre_os_init();

	/* Print Clock configuration */
	//printf( "== CPU running at %dMHz, Peripherals at %dMHz/%dMHz  ==\r\n" , (HAL_RCCEx_GetD1SysClockFreq()/1000000U)
	//                                                                  	  , (HAL_RCC_GetPCLK1Freq()/1000000U)
	//																	  , (HAL_RCC_GetPCLK2Freq()/1000000U) );

	return 0;
}

static int BSP_VerifyData(const uint64_t *pData, const uint64_t *pFlash, uint32_t DataSize)
{
  uint32_t i;
  for(i = 0; i < DataSize; i+=8)
  {
    if (pData[i] != pFlash[i] )
      return 1;
  }

  return 0;
}

#if 0
/**
  * @brief  Copy resource file into the OctoSPI memory
  * @param  hItem    : Progress bar used to indicate the transfer progression
  * @param  pResFile : Resource file to be copied in the OctoSPI memory
  * @param  Address  : Address where the resource will be copied
  * @retval ErrorCode : 0 if success -1 otherwise
  */
int BSP_ResourcesCopy(WM_HWIN hItem, FIL * pResFile, uint32_t Address)
{
  int RetErr = 0;
  BSP_QSPI_Info_t QSPIInfo;
  uint32_t file_size = 0;
  uint32_t numOfReadBytes = 0, FlashAddr = 0, nbTotalBytes = 0;
  int32_t ospiStatus = BSP_ERROR_NONE;
  uint8_t *pSdData = 0;
  int sector = 0, nb_sectors = 0;

  /* Sop Memory Mapping mode */
  BSP_QSPI_DeInit(0);

  /* Re-initialize NOR OctoSPI flash to exit memory-mapped mode */
  BSP_QSPI_Init_t init ;
  init.InterfaceMode=MT25TL01G_QPI_MODE;
  init.TransferRate= MT25TL01G_DTR_TRANSFER ;
  init.DualFlashMode= MT25TL01G_DUALFLASH_ENABLE;
  if (BSP_QSPI_Init(0,&init) != BSP_ERROR_NONE)
  {
    /* Initialization Error */
    return -1;
  }

  /* Initialize the structure */
  QSPIInfo.FlashSize          = (uint32_t)0x00;
  QSPIInfo.EraseSectorSize    = (uint32_t)0x00;
  QSPIInfo.EraseSectorsNumber = (uint32_t)0x00;
  QSPIInfo.ProgPageSize       = (uint32_t)0x00;
  QSPIInfo.ProgPagesNumber    = (uint32_t)0x00;

  ospiStatus = BSP_QSPI_GetInfo(0,&QSPIInfo);
  if (ospiStatus != BSP_ERROR_NONE)
  {
    RetErr = -1;
    goto exit;
  }

  /* Test the correctness */
  if((QSPIInfo.FlashSize          != EXT_FLASH_SIZE) ||
     (QSPIInfo.EraseSectorSize    != EXT_FLASH_SUBSECTOR_SIZE) ||
     (QSPIInfo.EraseSectorsNumber != (EXT_FLASH_SIZE/EXT_FLASH_SUBSECTOR_SIZE)) ||
     (QSPIInfo.ProgPageSize       != EXT_FLASH_PAGE_SIZE)  ||
     (QSPIInfo.ProgPagesNumber    != (EXT_FLASH_SIZE/EXT_FLASH_PAGE_SIZE)))
  {
    RetErr = -1;
    goto exit;
  }

   pSdData = ff_malloc(QSPIInfo.EraseSectorSize);
   if (pSdData == 0)
   {
     RetErr = -2;
     goto exit;
   }

  if (f_lseek(pResFile, 0) != FR_OK)
  {
    RetErr = -3;
    goto exit;
  }

  file_size = f_size(pResFile);

  nb_sectors = (file_size / QSPIInfo.EraseSectorSize);
  if(file_size % QSPIInfo.EraseSectorSize)
    nb_sectors++;

  PROGBAR_SetMinMax(hItem, 0, nb_sectors);

  FlashAddr = (Address - QSPI_BASE_ADDRESS);
  do
  {
    memset(pSdData, 0xFF, QSPIInfo.EraseSectorSize);

    /* Read and Program data */
    if(f_read(pResFile, pSdData, QSPIInfo.EraseSectorSize, (void *)&numOfReadBytes) != FR_OK)
    {
      RetErr = -4;
      goto exit;
    }

    ospiStatus = BSP_QSPI_EraseBlock(0,FlashAddr,MT25TL01G_ERASE_4K);
    if (ospiStatus != BSP_ERROR_NONE)
    {
      RetErr = -1;
      goto exit;
    }

    ospiStatus = BSP_QSPI_Write(0,(uint8_t *)pSdData, FlashAddr, numOfReadBytes);
    if (ospiStatus != BSP_ERROR_NONE)
    {
      RetErr = -1;
      goto exit;
    }

    /* Wait the end of write operation */
    do
    {
      ospiStatus = BSP_QSPI_GetStatus(0);
    } while (ospiStatus == BSP_ERROR_BUSY);

    /* Check the write operation correctness */
    if (ospiStatus != BSP_ERROR_NONE)
    {
      RetErr = -1;
      goto exit;
    }

    FlashAddr     += numOfReadBytes;
    nbTotalBytes  += numOfReadBytes;

    PROGBAR_SetValue(hItem, ++sector);
    GUI_Exec();

  } while((numOfReadBytes == QSPIInfo.EraseSectorSize) && (nbTotalBytes < file_size));

exit:
  if(pSdData)
  {
    ff_free(pSdData);
  }
  
  /* Reconfigure memory mapped mode */
  if(BSP_QSPI_EnableMemoryMappedMode(0) != BSP_ERROR_NONE)
  {
    RetErr = -1;
  }

  if(RetErr)
    printf("\nFailed : Error=%d, ospiStatus=%d !!\n", RetErr, (int)ospiStatus);

  GUI_Exec();

  return RetErr;
}
#endif

uint8_t BSP_SuspendCPU2 ( void )
{
  uint8_t dual_core = 0;
  int32_t timeout   = 0xFFFF;

  printf("Suspending CPU2 : ");

  /* Check if the CPU 2 is up and running */
  while((__HAL_RCC_GET_FLAG(RCC_FLAG_D2CKRDY) == RESET) && (timeout-- > 0));
  if ( timeout > 0 )
  {
    /*Take HSEM */
    HAL_HSEM_FastTake(HSEM_ID_0);
    /*Release HSEM in order to notify the CPU2(CM4)*/
    HAL_HSEM_Release(HSEM_ID_0, 0);

    /* Wait until CPU2 enter in STOP mode */
    timeout   = 0x1FFFF;
    while((__HAL_RCC_GET_FLAG(RCC_FLAG_D2CKRDY) != RESET) && (timeout-- > 0));
    if ( timeout > 0 )
    {
      dual_core = 1;
    }
  }

  if ( dual_core )
    printf("OK\n");
  else
    printf("OK\n");

  return dual_core;
}

uint8_t BSP_ResumeCPU2 ( void )
{
  uint8_t dual_core = 0;
  int32_t timeout   = 0xFFFF;

  printf("Resuming CPU2 : ");

  /* Check if the CPU 2 is already suspended */
  while((__HAL_RCC_GET_FLAG(RCC_FLAG_D2CKRDY) != RESET) && (timeout-- > 0));
  if ( timeout > 0 )
  {
    /*Take HSEM */
    HAL_HSEM_FastTake(HSEM_ID_0);
    /*Release HSEM in order to notify the CPU2(CM4)*/
    HAL_HSEM_Release(HSEM_ID_0, 1);

    /* Wait until CPU2 boots and enters in stop mode or timeout*/
    timeout = 0xFFFF;
    while((__HAL_RCC_GET_FLAG(RCC_FLAG_D2CKRDY) == RESET) && (timeout-- > 0));
    if ( timeout > 0 )
    {
      dual_core = 1;
    }
  }

  if ( dual_core )
    printf("OK\n");
  else
    printf("OK\n");

  return dual_core;
}

/**
  * @brief  Copy program code file into the Internal Flash memory
  * @param  hItem    : Progress bar used to indicate the transfer progression
  * @param  pResFile : Program code file to be copied in the Internal Flash memory
  * @param  Address  : Address where the code will be copied
  * @retval ErrorCode : 0 if success -1 otherwise
  */
int BSP_FlashProgram(WM_HWIN hItem, FIL * pResFile, uint32_t Address)
{
  FLASH_EraseInitTypeDef EraseInitStruct;

  int Ret = 0;
  uint8_t dual_core = 0;
  uint32_t EraseError = 0;
  uint32_t numOfReadBytes = 0, nbTotalBytes = 0;
  uint32_t offset = 0;
  uint8_t *pSdData;
  uint64_t *pData256, *pFlash256;
  int sector = 0;

  pSdData = (uint8_t *)ff_malloc(FLASH_SECTOR_SIZE);
  if (pSdData == NULL)
  {
    return -1;
  }

  if (f_lseek(pResFile, 0) != FR_OK)
  {
    ff_free(pSdData);
    return -2;
  }

  /* Get the 1st page to erase */
  const uint32_t StartSector = GetPage(Address);
  /* Get the number of pages to erase from 1st page */
  const uint32_t NbOfSectors = GetPage(Address + f_size(pResFile) -1) - StartSector + 1;
  /* Get the bank */
  const uint32_t BankNumber = GetBank(Address);

  /* Clear pending flags (if any) */
  __HAL_FLASH_CLEAR_FLAG(FLASH_FLAG_ALL_BANK1);
  __HAL_FLASH_CLEAR_FLAG(FLASH_FLAG_ALL_BANK2);

  /* Unlock the Flash to enable the flash control register access *************/
  HAL_FLASH_Unlock();

  /* Clear all error flags */
  if(BankNumber == FLASH_BANK_1)
    __HAL_FLASH_CLEAR_FLAG(FLASH_FLAG_ALL_ERRORS_BANK1);
  else
    __HAL_FLASH_CLEAR_FLAG(FLASH_FLAG_ALL_ERRORS_BANK2);

  /* Fill EraseInit structure*/
  EraseInitStruct.TypeErase     = FLASH_TYPEERASE_SECTORS;
  EraseInitStruct.VoltageRange  = FLASH_VOLTAGE_RANGE_3;
  EraseInitStruct.Banks         = BankNumber;
  EraseInitStruct.Sector        = StartSector;
  EraseInitStruct.NbSectors     = NbOfSectors;

  /* Note: If an erase operation in Flash memory also concerns data in the data or instruction cache,
     you have to make sure that these data are rewritten before they are accessed during code
     execution. If this cannot be done safely, it is recommended to flush the caches by setting the
     DCRST and ICRST bits in the FLASH_CR register. */
  if (HAL_FLASHEx_Erase(&EraseInitStruct, &EraseError) != HAL_OK)
  {
    Ret = HAL_FLASH_GetError();
    goto unlock_and_exit;
  }

  PROGBAR_SetMinMax(hItem, 0, NbOfSectors);
  pFlash256 = (uint64_t *)Address;

  do
  {
    memset(pSdData, 0xFF, FLASH_SECTOR_SIZE);

    Ret = f_read(pResFile, pSdData, FLASH_SECTOR_SIZE, (void *)&numOfReadBytes);
    if(Ret == FR_OK)
    {
      uint8_t failure = 0;
      offset          = 0;
      pData256       = (uint64_t *)pSdData;

      /* Disable, Clean and Invalidate D-Cache */
      SCB_DisableDCache();
      SCB_CleanInvalidateDCache();

      /* Program the user Flash area word by word */
      while (offset < numOfReadBytes)
      {
        if (HAL_FLASH_Program(FLASH_TYPEPROGRAM_FLASHWORD, (uint32_t )pFlash256, (uint32_t )pData256) == HAL_OK)
        {
          if( BSP_VerifyData(pData256, pFlash256, (FLASH_BURST_WIDTH / (sizeof(uint64_t) * sizeof(uint64_t)))) )
          {
            Ret = -1;
            /* Enable D-Cache */
            SCB_EnableDCache();
            goto unlock_and_exit;
          }
          pData256  += (FLASH_BURST_WIDTH / (sizeof(uint64_t) * sizeof(uint64_t)));
          pFlash256 += (FLASH_BURST_WIDTH / (sizeof(uint64_t) * sizeof(uint64_t)));
          offset    += (FLASH_BURST_WIDTH / (sizeof(uint64_t) * sizeof(uint8_t)));
          failure    = 0;
        }
        else
        {
          if (failure++ > MAX_FLASH_WRITE_FAILURE)
          {
            Ret = HAL_FLASH_GetError();
            /* Enable D-Cache */
            SCB_EnableDCache();
            goto unlock_and_exit;
          }
        }
      }

      /* Enable D-Cache */
      SCB_EnableDCache();

      nbTotalBytes += numOfReadBytes;
    }
    else
    {
      goto unlock_and_exit;
    }

    PROGBAR_SetValue(hItem, ++sector);
    GUI_Exec();

  } while(numOfReadBytes == FLASH_SECTOR_SIZE);

unlock_and_exit:
  if(BankNumber == FLASH_BANK_1)
    HAL_FLASHEx_Unlock_Bank1();
  else
    HAL_FLASHEx_Unlock_Bank2();

  ff_free(pSdData);
  GUI_Exec();

  if(dual_core)
  {
    BSP_ResumeCPU2();
  }

  return Ret;
}

__RAM_CODE_SECTION
static int __update_flash(FLASH_EraseInitTypeDef *EraseInitStruct, const uint64_t *pFlash256, const uint64_t *pData256, uint32_t Size)
{
  int       Ret = 0;
  uint32_t  EraseError = 0;
  uint32_t  offset = 0;
  uint8_t   failure = 0;

  /* Clear pending flags (if any) */
  __HAL_FLASH_CLEAR_FLAG(FLASH_FLAG_ALL_BANK1);
  __HAL_FLASH_CLEAR_FLAG(FLASH_FLAG_ALL_BANK2);

  /* Unlock the Flash to enable the flash control register access *************/
  HAL_FLASH_Unlock();

  /* Clear all error flags */
  if(EraseInitStruct->Banks == FLASH_BANK_1)
    __HAL_FLASH_CLEAR_FLAG(FLASH_FLAG_ALL_ERRORS_BANK1);
  else
    __HAL_FLASH_CLEAR_FLAG(FLASH_FLAG_ALL_ERRORS_BANK2);

  __disable_irq();

  /* Note: If an erase operation in Flash memory also concerns data in the data or instruction cache,
     you have to make sure that these data are rewritten before they are accessed during code
     execution. If this cannot be done safely, it is recommended to flush the caches by setting the
     DCRST and ICRST bits in the FLASH_CR register. */
  if (HAL_FLASHEx_Erase(EraseInitStruct, &EraseError) != HAL_OK)
  {
    Ret = HAL_FLASH_GetError();
    goto unlock_and_exit;
  }

  /* Program the user Flash area word by word */
  offset    = 0;
  while (offset < FLASH_SECTOR_SIZE)
  {
    if (HAL_FLASH_Program(FLASH_TYPEPROGRAM_FLASHWORD, (uint32_t )pFlash256, (uint32_t )pData256) == HAL_OK)
    {
      if( BSP_VerifyData(pData256, pFlash256, (FLASH_BURST_WIDTH / (sizeof(uint64_t) * sizeof(uint64_t)))) )
      {
        Ret = -1;
        goto unlock_and_exit;
      }
      pData256  += (FLASH_BURST_WIDTH / (sizeof(uint64_t) * sizeof(uint64_t)));
      pFlash256 += (FLASH_BURST_WIDTH / (sizeof(uint64_t) * sizeof(uint64_t)));
      offset    += (FLASH_BURST_WIDTH / (sizeof(uint64_t) * sizeof(uint8_t)));
      failure    = 0;
    }
    else
    {
      if (failure++ > MAX_FLASH_WRITE_FAILURE)
      {
        Ret = HAL_FLASH_GetError();
        goto unlock_and_exit;
      }
    }
  }

unlock_and_exit:
  if(EraseInitStruct->Banks == FLASH_BANK_1)
    HAL_FLASHEx_Unlock_Bank1();
  else
    HAL_FLASHEx_Unlock_Bank2();

  __enable_irq();
  return Ret;
}

/**
  * @brief  Copy program code file into the Internal Flash memory
  * @param  Address   : Address where the code will be copied
  * @param  pData     : Program code file to be copied in the Internal Flash memory
  * @param  Size      : Size of the data to be programmed
  * @retval ErrorCode : 0 if success -1 otherwise
  */
int BSP_FlashUpdate(uint32_t Address, uint8_t *pData, uint32_t Size)
{
  int             Ret = 0;
  uint8_t         dual_core = 0;
  const uint64_t *pFlash256;
  uint64_t *pData256;
  uint64_t *pDst256;
  uint64_t *SectorData;
  uint32_t  offset = 0;
  FLASH_EraseInitTypeDef EraseInitStruct;

  if (pData == NULL)
  {
    return -1;
  }

  SectorData = (uint64_t *)ff_malloc(FLASH_SECTOR_SIZE);
  if (SectorData == NULL)
  {
    return -2;
  }
  memset(SectorData, 0xFF, FLASH_SECTOR_SIZE);

  /* Get the 1st page to erase */
  const uint32_t StartSector = GetPage(Address);
  /* Get the number of pages to erase from 1st page */
  const uint32_t NbOfSectors = GetPage(Address + Size - 1) - StartSector + 1;
  /* Get the bank */
  const uint32_t BankNumber = GetBank(Address);

  pFlash256 = (uint64_t *)(FLASH_BASE + (StartSector * FLASH_SECTOR_SIZE));
  /* Read current data */
  for(offset = 0; offset < (FLASH_SECTOR_SIZE / 8); offset++ )
  {
    SectorData[offset] = pFlash256[offset];
  }

  pData256  = (uint64_t *)pData;
  pDst256   = (uint64_t *)((uint32_t )SectorData + (uint32_t )(Address & (FLASH_SECTOR_SIZE -1)));
  /* Update DC */
  for(offset = 0; offset < (Size/8); offset++ )
  {
    *pDst256++ = *pData256++;
  }

  /* Fill EraseInit structure*/
  EraseInitStruct.TypeErase     = FLASH_TYPEERASE_SECTORS;
  EraseInitStruct.VoltageRange  = FLASH_VOLTAGE_RANGE_3;
  EraseInitStruct.Banks         = BankNumber;
  EraseInitStruct.Sector        = StartSector;
  EraseInitStruct.NbSectors     = NbOfSectors;

  /* Disable, Clean and Invalidate D-Cache */
  SCB_DisableDCache();
  SCB_CleanInvalidateDCache();

  Ret = __update_flash(&EraseInitStruct, pFlash256, SectorData, Size );

  /* Enable D-Cache */
  SCB_EnableDCache();

  if(dual_core)
  {
    BSP_ResumeCPU2();
  }

  ff_free(SectorData);
  return Ret;
}
