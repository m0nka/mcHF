
#include "mchf_pro_board.h"

#ifdef USE_MT48LC4M32B2P
#include "MT48LC4M32B2P.h"
#endif

#ifdef USE_IS42S32160F
#include "IS42S32160F.h"
#endif

#include "sdram.h"

SDRAM_HandleTypeDef hsdram[SDRAM_INSTANCES_NBR];

#if (USE_HAL_SDRAM_REGISTER_CALLBACKS == 1)
static uint32_t IsMspCallbacksValid = 0;
#endif

static FMC_SDRAM_CommandTypeDef 	Command;

static void SDRAM_MspInit(SDRAM_HandleTypeDef  *hsdram)
{
	static MDMA_HandleTypeDef mdma_handle;
	GPIO_InitTypeDef gpio_init_structure;

	/* Enable FMC clock */
	__HAL_RCC_FMC_CLK_ENABLE();

	/* Enable chosen MDMAx clock */
	SDRAM_MDMAx_CLK_ENABLE();

	/* Enable GPIOs clock */
	__HAL_RCC_GPIOC_CLK_ENABLE();
	__HAL_RCC_GPIOD_CLK_ENABLE();
	__HAL_RCC_GPIOE_CLK_ENABLE();
	__HAL_RCC_GPIOF_CLK_ENABLE();
	__HAL_RCC_GPIOG_CLK_ENABLE();
	__HAL_RCC_GPIOH_CLK_ENABLE();
	__HAL_RCC_GPIOI_CLK_ENABLE();

	/* Common GPIO configuration */
	gpio_init_structure.Mode      = GPIO_MODE_AF_PP;
	gpio_init_structure.Pull      = GPIO_PULLUP;
	gpio_init_structure.Speed     = GPIO_SPEED_FREQ_VERY_HIGH;
	gpio_init_structure.Alternate = GPIO_AF12_FMC;

	// GPIOC configuration
	#ifndef PCB_V9_REV_A
	gpio_init_structure.Pin   = GPIO_PIN_0;
	HAL_GPIO_Init(GPIOC, &gpio_init_structure);
	#else
	gpio_init_structure.Pin   = FMC_SDNWE_PIN;
	HAL_GPIO_Init(FMC_SDNWE_PORT, &gpio_init_structure);
	#endif

	// GPIOD configuration
	#ifndef PCB_V9_REV_A
	gpio_init_structure.Pin   = GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_8| GPIO_PIN_9 | GPIO_PIN_10 |\
                          	  GPIO_PIN_14 | GPIO_PIN_15;
	HAL_GPIO_Init(GPIOD, &gpio_init_structure);
	#else
	// PD0
	gpio_init_structure.Pin = FMC_D2_PIN;
	HAL_GPIO_Init(FMC_D2_PORT, &gpio_init_structure);
	// PD1
	gpio_init_structure.Pin = FMC_D3_PIN;
	HAL_GPIO_Init(FMC_D3_PORT, &gpio_init_structure);
	// PD8
	gpio_init_structure.Pin = FMC_D13_PIN;
	HAL_GPIO_Init(FMC_D13_PORT, &gpio_init_structure);
	// PD9
	gpio_init_structure.Pin = FMC_D14_PIN;
	HAL_GPIO_Init(FMC_D14_PORT, &gpio_init_structure);
	// PD10
	gpio_init_structure.Pin = FMC_D15_PIN;
	HAL_GPIO_Init(FMC_D15_PORT, &gpio_init_structure);
	// PD14
	gpio_init_structure.Pin = FMC_D0_PIN;
	HAL_GPIO_Init(FMC_D0_PORT, &gpio_init_structure);
	// PD15
	gpio_init_structure.Pin = FMC_D1_PIN;
	HAL_GPIO_Init(FMC_D1_PORT, &gpio_init_structure);
	#endif

	// GPIOE configuration
	#ifndef PCB_V9_REV_A
	gpio_init_structure.Pin   = GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_7| GPIO_PIN_8 | GPIO_PIN_9 |\
                              GPIO_PIN_10 | GPIO_PIN_11 | GPIO_PIN_12 | GPIO_PIN_13 | GPIO_PIN_14 |\
                              GPIO_PIN_15;
	HAL_GPIO_Init(GPIOE, &gpio_init_structure);
	#else
	// PE0
	gpio_init_structure.Pin = FMC_NBL0_PIN;
	HAL_GPIO_Init(FMC_NBL0_PORT, &gpio_init_structure);
	// PE1
	gpio_init_structure.Pin = FMC_NBL1_PIN;
	HAL_GPIO_Init(FMC_NBL1_PORT, &gpio_init_structure);
	// PE7
	gpio_init_structure.Pin = FMC_D4_PIN;
	HAL_GPIO_Init(FMC_D4_PORT, &gpio_init_structure);
	// PE8
	gpio_init_structure.Pin = FMC_D5_PIN;
	HAL_GPIO_Init(FMC_D5_PORT, &gpio_init_structure);
	// PE9
	gpio_init_structure.Pin = FMC_D6_PIN;
	HAL_GPIO_Init(FMC_D6_PORT, &gpio_init_structure);
	// PE10
	gpio_init_structure.Pin = FMC_D7_PIN;
	HAL_GPIO_Init(FMC_D7_PORT, &gpio_init_structure);
	// PE11
	gpio_init_structure.Pin = FMC_D8_PIN;
	HAL_GPIO_Init(FMC_D8_PORT, &gpio_init_structure);
	// PE12
	gpio_init_structure.Pin = FMC_D9_PIN;
	HAL_GPIO_Init(FMC_D9_PORT, &gpio_init_structure);
	// PE13
	gpio_init_structure.Pin = FMC_D10_PIN;
	HAL_GPIO_Init(FMC_D10_PORT, &gpio_init_structure);
	// PE14
	gpio_init_structure.Pin = FMC_D11_PIN;
	HAL_GPIO_Init(FMC_D11_PORT, &gpio_init_structure);
	// PE15
	gpio_init_structure.Pin = FMC_D12_PIN;
	HAL_GPIO_Init(FMC_D12_PORT, &gpio_init_structure);
	#endif

	// GPIOF configuration
	#ifndef PCB_V9_REV_A
	gpio_init_structure.Pin   = GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_2| GPIO_PIN_3 | GPIO_PIN_4 |\
                              GPIO_PIN_5 | GPIO_PIN_11 | GPIO_PIN_12 | GPIO_PIN_13 | GPIO_PIN_14 |\
                              GPIO_PIN_15;
	HAL_GPIO_Init(GPIOF, &gpio_init_structure);
	#else
	// PF0
	gpio_init_structure.Pin = FMC_A0_PIN;
	HAL_GPIO_Init(FMC_A0_PORT, &gpio_init_structure);
	// PF1
	gpio_init_structure.Pin = FMC_A1_PIN;
	HAL_GPIO_Init(FMC_A1_PORT, &gpio_init_structure);
	// PF2
	gpio_init_structure.Pin = FMC_A2_PIN;
	HAL_GPIO_Init(FMC_A2_PORT, &gpio_init_structure);
	// PF3
	gpio_init_structure.Pin = FMC_A3_PIN;
	HAL_GPIO_Init(FMC_A3_PORT, &gpio_init_structure);
	// PF4
	gpio_init_structure.Pin = FMC_A4_PIN;
	HAL_GPIO_Init(FMC_A4_PORT, &gpio_init_structure);
	// PF5
	gpio_init_structure.Pin = FMC_A5_PIN;
	HAL_GPIO_Init(FMC_A5_PORT, &gpio_init_structure);
	// PF11
	gpio_init_structure.Pin = FMC_SDNRAS_PIN;
	HAL_GPIO_Init(FMC_SDNRAS_PORT, &gpio_init_structure);
	// PF12
	gpio_init_structure.Pin = FMC_A6_PIN;
	HAL_GPIO_Init(FMC_A6_PORT, &gpio_init_structure);
	// PF13
	gpio_init_structure.Pin = FMC_A7_PIN;
	HAL_GPIO_Init(FMC_A7_PORT, &gpio_init_structure);
	// PF14
	gpio_init_structure.Pin = FMC_A8_PIN;
	HAL_GPIO_Init(FMC_A8_PORT, &gpio_init_structure);
	// PF15
	gpio_init_structure.Pin = FMC_A9_PIN;
	HAL_GPIO_Init(FMC_A9_PORT, &gpio_init_structure);
	#endif

	// GPIOG configuration
	#ifndef PCB_V9_REV_A
	gpio_init_structure.Pin   = GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_4 | GPIO_PIN_5 | GPIO_PIN_8 | GPIO_PIN_15;
	HAL_GPIO_Init(GPIOG, &gpio_init_structure);
	#else
	// PG0
	gpio_init_structure.Pin = FMC_A10_PIN;
	HAL_GPIO_Init(FMC_A10_PORT, &gpio_init_structure);
	// PG1
	gpio_init_structure.Pin = FMC_A11_PIN;
	HAL_GPIO_Init(FMC_A11_PORT, &gpio_init_structure);
	// PG2
	gpio_init_structure.Pin = FMC_A12_PIN;
	HAL_GPIO_Init(FMC_A12_PORT, &gpio_init_structure);
	// PG4
	gpio_init_structure.Pin = FMC_BA0_PIN;
	HAL_GPIO_Init(FMC_BA0_PORT, &gpio_init_structure);
	// PG5
	gpio_init_structure.Pin = FMC_BA1_PIN;
	HAL_GPIO_Init(FMC_BA1_PORT, &gpio_init_structure);
	// PG8
	gpio_init_structure.Pin = FMC_SDCLK_PIN;
	HAL_GPIO_Init(FMC_SDCLK_PORT, &gpio_init_structure);
	// PG15
	gpio_init_structure.Pin = FMC_SDNCAS_PIN;
	HAL_GPIO_Init(FMC_SDNCAS_PORT, &gpio_init_structure);
	#endif

	// GPIOH configuration
	#ifndef PCB_V9_REV_A
	gpio_init_structure.Pin   = GPIO_PIN_2  | GPIO_PIN_3  | GPIO_PIN_8  | GPIO_PIN_9  |\
                                GPIO_PIN_10 | GPIO_PIN_11 | GPIO_PIN_12 | GPIO_PIN_13 |\
								GPIO_PIN_14 |GPIO_PIN_15;
	HAL_GPIO_Init(GPIOH, &gpio_init_structure);
	#else
	// PH2
	gpio_init_structure.Pin = FMC_SDCKE0_PIN;
	HAL_GPIO_Init(FMC_SDCKE0_PORT, &gpio_init_structure);
	// PH3
	gpio_init_structure.Pin = FMC_SDNE0_PIN;
	HAL_GPIO_Init(FMC_SDNE0_PORT, &gpio_init_structure);
	// PH8
	gpio_init_structure.Pin = FMC_D16_PIN;
	HAL_GPIO_Init(FMC_D16_PORT, &gpio_init_structure);
	// PH9
	gpio_init_structure.Pin = FMC_D17_PIN;
	HAL_GPIO_Init(FMC_D17_PORT, &gpio_init_structure);
	// PH10
	gpio_init_structure.Pin = FMC_D18_PIN;
	HAL_GPIO_Init(FMC_D18_PORT, &gpio_init_structure);
	// PH11
	gpio_init_structure.Pin = FMC_D19_PIN;
	HAL_GPIO_Init(FMC_D19_PORT, &gpio_init_structure);
	// PH12
	gpio_init_structure.Pin = FMC_D20_PIN;
	HAL_GPIO_Init(FMC_D20_PORT, &gpio_init_structure);
	// PH13
	gpio_init_structure.Pin = FMC_D21_PIN;
	HAL_GPIO_Init(FMC_D21_PORT, &gpio_init_structure);
	// PH14
	gpio_init_structure.Pin = FMC_D22_PIN;
	HAL_GPIO_Init(FMC_D22_PORT, &gpio_init_structure);
	// PH15
	gpio_init_structure.Pin = FMC_D23_PIN;
	HAL_GPIO_Init(FMC_D23_PORT, &gpio_init_structure);
	#endif

	// GPIOI configuration
	#ifndef PCB_V9_REV_A
	gpio_init_structure.Pin   = GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_3 | GPIO_PIN_4 |\
                              GPIO_PIN_5 | GPIO_PIN_6 | GPIO_PIN_7 | GPIO_PIN_9 | GPIO_PIN_10;
	HAL_GPIO_Init(GPIOI, &gpio_init_structure);
	#else
	// PI0
	gpio_init_structure.Pin = FMC_D24_PIN;
	HAL_GPIO_Init(FMC_D24_PORT, &gpio_init_structure);
	// PI1
	gpio_init_structure.Pin = FMC_D25_PIN;
	HAL_GPIO_Init(FMC_D25_PORT, &gpio_init_structure);
	// PI2
	gpio_init_structure.Pin = FMC_D26_PIN;
	HAL_GPIO_Init(FMC_D26_PORT, &gpio_init_structure);
	// PI3
	gpio_init_structure.Pin = FMC_D27_PIN;
	HAL_GPIO_Init(FMC_D27_PORT, &gpio_init_structure);
	// PI4
	gpio_init_structure.Pin = FMC_NBL2_PIN;
	HAL_GPIO_Init(FMC_NBL2_PORT, &gpio_init_structure);
	// PI5
	gpio_init_structure.Pin = FMC_NBL3_PIN;
	HAL_GPIO_Init(FMC_NBL3_PORT, &gpio_init_structure);
	// PI6
	gpio_init_structure.Pin = FMC_D28_PIN;
	HAL_GPIO_Init(FMC_D28_PORT, &gpio_init_structure);
	// PI7
	gpio_init_structure.Pin = FMC_D29_PIN;
	HAL_GPIO_Init(FMC_D29_PORT, &gpio_init_structure);
	// PI9
	gpio_init_structure.Pin = FMC_D30_PIN;
	HAL_GPIO_Init(FMC_D30_PORT, &gpio_init_structure);
	// PI10
	gpio_init_structure.Pin = FMC_D31_PIN;
	HAL_GPIO_Init(FMC_D31_PORT, &gpio_init_structure);
	#endif

	/* Configure common MDMA parameters */
	mdma_handle.Init.Request                  = MDMA_REQUEST_SW;
	mdma_handle.Init.TransferTriggerMode      = MDMA_BLOCK_TRANSFER;
	mdma_handle.Init.Priority                 = MDMA_PRIORITY_HIGH;
	mdma_handle.Init.Endianness               = MDMA_LITTLE_ENDIANNESS_PRESERVE;
	mdma_handle.Init.SourceInc                = MDMA_SRC_INC_WORD;
	mdma_handle.Init.DestinationInc           = MDMA_DEST_INC_WORD;
	mdma_handle.Init.SourceDataSize           = MDMA_SRC_DATASIZE_WORD;
	mdma_handle.Init.DestDataSize             = MDMA_DEST_DATASIZE_WORD;
	mdma_handle.Init.DataAlignment            = MDMA_DATAALIGN_PACKENABLE;
	mdma_handle.Init.SourceBurst              = MDMA_SOURCE_BURST_SINGLE;
	mdma_handle.Init.DestBurst                = MDMA_DEST_BURST_SINGLE;
	mdma_handle.Init.BufferTransferLength     = 128;
	mdma_handle.Init.SourceBlockAddressOffset = 0;
	mdma_handle.Init.DestBlockAddressOffset   = 0;
	mdma_handle.Instance                      = SDRAM_MDMAx_CHANNEL;

	/* Associate the MDMA handle */
	__HAL_LINKDMA(hsdram, hmdma, mdma_handle);

	/* Deinitialize the stream for new transfer */
	(void)HAL_MDMA_DeInit(&mdma_handle);

	/* Configure the MDMA stream */
	(void)HAL_MDMA_Init(&mdma_handle);

	/* NVIC configuration for DMA transfer complete interrupt */
	HAL_NVIC_SetPriority(SDRAM_MDMAx_IRQn, BSP_SDRAM_IT_PRIORITY, 0);
	HAL_NVIC_EnableIRQ(SDRAM_MDMAx_IRQn);
}

static void SDRAM_MspDeInit(SDRAM_HandleTypeDef  *hsdram)
{
  static MDMA_HandleTypeDef mdma_handle;

  /* Prevent unused argument(s) compilation warning */
  UNUSED(hsdram);

  /* Disable NVIC configuration for MDMA interrupt */
  HAL_NVIC_DisableIRQ(SDRAM_MDMAx_IRQn);

  /* Deinitialize the stream for new transfer */
  mdma_handle.Instance = SDRAM_MDMAx_CHANNEL;
  (void)HAL_MDMA_DeInit(&mdma_handle);
}

void BSP_SDRAM_Initialization_sequence(uint32_t RefreshCount)
{
  __IO uint32_t tmpmrd = 0;

  /* Step 1: Configure a clock configuration enable command */
  Command.CommandMode            = FMC_SDRAM_CMD_CLK_ENABLE;
  Command.CommandTarget          = FMC_SDRAM_CMD_TARGET_BANK1;
  Command.AutoRefreshNumber      = 1;
  Command.ModeRegisterDefinition = 0;

  /* Send the command */
  HAL_SDRAM_SendCommand(&hsdram[0], &Command, SDRAM_TIMEOUT);

  /* Step 2: Insert 100 us minimum delay */
  /* Inserted delay is equal to 1 ms due to systick time base unit (ms) */
  HAL_Delay(1);

  /* Step 3: Configure a PALL (precharge all) command */
  Command.CommandMode            = FMC_SDRAM_CMD_PALL;
  Command.CommandTarget          = FMC_SDRAM_CMD_TARGET_BANK1;
  Command.AutoRefreshNumber      = 1;
  Command.ModeRegisterDefinition = 0;

  /* Send the command */
  HAL_SDRAM_SendCommand(&hsdram[0], &Command, SDRAM_TIMEOUT);

  /* Step 4: Configure an Auto Refresh command */
  Command.CommandMode            = FMC_SDRAM_CMD_AUTOREFRESH_MODE;
  Command.CommandTarget          = FMC_SDRAM_CMD_TARGET_BANK1;
  Command.AutoRefreshNumber      = 8;
  Command.ModeRegisterDefinition = 0;

  /* Send the command */
  HAL_SDRAM_SendCommand(&hsdram[0], &Command, SDRAM_TIMEOUT);

  /* Step 5: Program the external memory mode register */
  tmpmrd = (uint32_t)SDRAM_MODEREG_BURST_LENGTH_1          |\
                     SDRAM_MODEREG_BURST_TYPE_SEQUENTIAL   |\
                     SDRAM_MODEREG_CAS_LATENCY_2           |\
                     SDRAM_MODEREG_OPERATING_MODE_STANDARD |\
                     SDRAM_MODEREG_WRITEBURST_MODE_SINGLE;

  Command.CommandMode            = FMC_SDRAM_CMD_LOAD_MODE;
  Command.CommandTarget          = FMC_SDRAM_CMD_TARGET_BANK1;
  Command.AutoRefreshNumber      = 1;
  Command.ModeRegisterDefinition = tmpmrd;

  /* Send the command */
  HAL_SDRAM_SendCommand(&hsdram[0], &Command, SDRAM_TIMEOUT);

  /* Step 6: Set the refresh rate counter */
  /* Set the device refresh rate */
  HAL_SDRAM_ProgramRefreshRate(&hsdram[0], RefreshCount);
}

int32_t BSP_SDRAM_Init(uint32_t Instance)
{
	int32_t ret;
	#ifdef USE_MT48LC4M32B2P
	static MT48LC4M32B2P_Context_t pRegMode;
	#endif
	#ifdef USE_IS42S32160F
	static IS42S32160F_Context_t pRegMode;
	#endif

	if(Instance >=SDRAM_INSTANCES_NBR)
	{
		ret =  BSP_ERROR_WRONG_PARAM;
	}
	else
	{

		#if (USE_HAL_SDRAM_REGISTER_CALLBACKS == 1)
		/* Register the SDRAM MSP Callbacks */
		if(IsMspCallbacksValid == 0)
		{
			if(BSP_SDRAM_RegisterDefaultMspCallbacks(Instance) != BSP_ERROR_NONE)
			{
				return BSP_ERROR_PERIPH_FAILURE;
			}
		}
		#else
		/* Msp SDRAM initialization */
		SDRAM_MspInit(&hsdram[0]);
		#endif /* USE_HAL_SDRAM_REGISTER_CALLBACKS */

		if(MX_SDRAM_Init(&hsdram[0]) != HAL_OK)
		{
			ret = BSP_ERROR_NO_INIT;
		}
		else
		{
			#if 1
			/* External memory mode register configuration */
			if(SDRAM_DEVICE_ADDR == 0xC0000000)
				pRegMode.TargetBank      = FMC_SDRAM_CMD_TARGET_BANK1;
			else
				pRegMode.TargetBank      = FMC_SDRAM_CMD_TARGET_BANK2;

			#ifdef USE_MT48LC4M32B2P
			pRegMode.RefreshMode     = MT48LC4M32B2P_AUTOREFRESH_MODE_CMD;
			pRegMode.RefreshRate     = REFRESH_COUNT;
			pRegMode.BurstLength     = MT48LC4M32B2P_BURST_LENGTH_1;
			pRegMode.BurstType       = MT48LC4M32B2P_BURST_TYPE_SEQUENTIAL;
			pRegMode.CASLatency      = MT48LC4M32B2P_CAS_LATENCY_2;
			pRegMode.OperationMode   = MT48LC4M32B2P_OPERATING_MODE_STANDARD;
			pRegMode.WriteBurstMode  = MT48LC4M32B2P_WRITEBURST_MODE_SINGLE;

			// SDRAM initialization sequence
			if(MT48LC4M32B2P_Init(&hsdram[0], &pRegMode) != MT48LC4M32B2P_OK)
			{
				ret =  BSP_ERROR_COMPONENT_FAILURE;
			}
			else
			{
				ret = BSP_ERROR_NONE;
			}
			#endif
			#ifdef USE_IS42S32160F
			pRegMode.RefreshMode     = IS42S32160F_AUTOREFRESH_MODE_CMD;
			pRegMode.RefreshRate     = REFRESH_COUNT;
			pRegMode.BurstLength     = IS42S32160F_BURST_LENGTH_1;
			pRegMode.BurstType       = IS42S32160F_BURST_TYPE_SEQUENTIAL;
			pRegMode.CASLatency      = IS42S32160F_CAS_LATENCY_2;
			pRegMode.OperationMode   = IS42S32160F_OPERATING_MODE_STANDARD;
			pRegMode.WriteBurstMode  = IS42S32160F_WRITEBURST_MODE_SINGLE;

			// SDRAM initialization sequence
			if(IS42S32160F_Init(&hsdram[0], &pRegMode) != IS42S32160F_OK)
			{
				ret =  BSP_ERROR_COMPONENT_FAILURE;
			}
			else
			{
				ret = BSP_ERROR_NONE;
			}
			#endif

		#else
		BSP_SDRAM_Initialization_sequence(REFRESH_COUNT);	// not working, fix!
		#endif
		}
	}

	return ret;
}

/**
  * @brief  DeInitializes the SDRAM device.
  * @param Instance  SDRAM Instance
  * @retval BSP status
  */
int32_t BSP_SDRAM_DeInit(uint32_t Instance)
{
  int32_t ret = BSP_ERROR_NONE;

  if(Instance >=SDRAM_INSTANCES_NBR)
  {
    ret =  BSP_ERROR_WRONG_PARAM;
  }
  else
  {
    /* SDRAM device de-initialization */
    hsdram[Instance].Instance = FMC_SDRAM_DEVICE;

    (void)HAL_SDRAM_DeInit(&hsdram[Instance]);
#if (USE_HAL_SDRAM_REGISTER_CALLBACKS == 0)
    /* SDRAM controller de-initialization */
    SDRAM_MspDeInit(&hsdram[Instance]);
#endif /* (USE_HAL_SDRAM_REGISTER_CALLBACKS == 0) */

    ret = BSP_ERROR_NONE;
  }

  return ret;
}

HAL_StatusTypeDef MX_SDRAM_Init(SDRAM_HandleTypeDef *hSdram)
{
	FMC_SDRAM_TimingTypeDef sdram_timing;

	/* SDRAM device configuration */
	hSdram->Instance = FMC_SDRAM_DEVICE;

	#ifdef USE_MT48LC4M32B2P
	// SDRAM handle configuration
	if(SDRAM_DEVICE_ADDR == 0xC0000000)
		hSdram->Init.SDBank             = FMC_SDRAM_BANK1;
	else
		hSdram->Init.SDBank             = FMC_SDRAM_BANK2;

	hSdram->Init.ColumnBitsNumber   = FMC_SDRAM_COLUMN_BITS_NUM_8;
	hSdram->Init.RowBitsNumber      = FMC_SDRAM_ROW_BITS_NUM_12;
	hSdram->Init.MemoryDataWidth    = FMC_SDRAM_MEM_BUS_WIDTH_32;
	hSdram->Init.InternalBankNumber = FMC_SDRAM_INTERN_BANKS_NUM_4;
	hSdram->Init.CASLatency         = FMC_SDRAM_CAS_LATENCY_2;
	hSdram->Init.WriteProtection    = FMC_SDRAM_WRITE_PROTECTION_DISABLE;
	hSdram->Init.SDClockPeriod      = FMC_SDRAM_CLOCK_PERIOD_3;
	hSdram->Init.ReadBurst          = FMC_SDRAM_RBURST_ENABLE;
	hsdram->Init.ReadPipeDelay      = FMC_SDRAM_RPIPE_DELAY_0;

	#ifndef SLOW_SDRAM
	/* Timing configuration for 100Mhz as SDRAM clock frequency (System clock is up to 200Mhz) */
	sdram_timing.LoadToActiveDelay    = 2;
	sdram_timing.ExitSelfRefreshDelay = 7;
	sdram_timing.SelfRefreshTime      = 4;
	sdram_timing.RowCycleDelay        = 7;
	sdram_timing.WriteRecoveryTime    = 2;
	sdram_timing.RPDelay              = 2;
	sdram_timing.RCDDelay             = 2;
	#else
	// ToDo: Reduce SDRAM speed quick hack to prevent Hard Fault on rev 0.8.4
	//       need proper fix!!!
	sdram_timing.LoadToActiveDelay    = 4;
	sdram_timing.ExitSelfRefreshDelay = 14;
	sdram_timing.SelfRefreshTime      = 8;
	sdram_timing.RowCycleDelay        = 14;
	sdram_timing.WriteRecoveryTime    = 4;
	sdram_timing.RPDelay              = 4;
	sdram_timing.RCDDelay             = 4;
	#endif
	#endif

	#ifdef USE_IS42S32160F
	// SDRAM handle configuration
	if(SDRAM_DEVICE_ADDR == 0xC0000000)
		hSdram->Init.SDBank             = FMC_SDRAM_BANK1;
	else
		hSdram->Init.SDBank             = FMC_SDRAM_BANK2;

	hSdram->Init.ColumnBitsNumber   	= FMC_SDRAM_COLUMN_BITS_NUM_8;
	hSdram->Init.RowBitsNumber      	= FMC_SDRAM_ROW_BITS_NUM_13;
	hSdram->Init.MemoryDataWidth    	= FMC_SDRAM_MEM_BUS_WIDTH_32;
	hSdram->Init.InternalBankNumber 	= FMC_SDRAM_INTERN_BANKS_NUM_4;
	hSdram->Init.CASLatency         	= FMC_SDRAM_CAS_LATENCY_2;
	hSdram->Init.WriteProtection    	= FMC_SDRAM_WRITE_PROTECTION_DISABLE;
	hSdram->Init.SDClockPeriod      	= FMC_SDRAM_CLOCK_PERIOD_3;
	hSdram->Init.ReadBurst          	= FMC_SDRAM_RBURST_ENABLE;
	hSdram->Init.ReadPipeDelay      	= FMC_SDRAM_RPIPE_DELAY_0;

	// Timing configuration(ToDo: can we run at 120Mhz?)
	sdram_timing.LoadToActiveDelay    	= 4;
	sdram_timing.ExitSelfRefreshDelay 	= 8;
	sdram_timing.SelfRefreshTime      	= 5;
	sdram_timing.RowCycleDelay        	= 8;
	sdram_timing.WriteRecoveryTime    	= 4;
	sdram_timing.RPDelay              	= 4;
	sdram_timing.RCDDelay             	= 4;
	#endif

	// SDRAM controller initialisation
	if(HAL_SDRAM_Init(hSdram, &sdram_timing) != HAL_OK)
	{
		return  HAL_ERROR;
	}

	return HAL_OK;
}

#if (USE_HAL_SDRAM_REGISTER_CALLBACKS == 1)

int32_t BSP_SDRAM_RegisterDefaultMspCallbacks (uint32_t Instance)
{
  int32_t ret = BSP_ERROR_NONE;

  /* Check if the instance is supported */
  if(Instance >=SDRAM_INSTANCES_NBR)
  {
    ret = BSP_ERROR_WRONG_PARAM;
  }
  else
  {
    /* Register MspInit/MspDeInit Callbacks */
    if(HAL_SDRAM_RegisterCallback(&hsdram[Instance],  HAL_SDRAM_MSP_INIT_CB_ID, SDRAM_MspInit) != HAL_OK)
    {
      ret = BSP_ERROR_PERIPH_FAILURE;
    }
    if(HAL_SDRAM_RegisterCallback(&hsdram[Instance], HAL_SDRAM_MSP_DEINIT_CB_ID, SDRAM_MspDeInit) != HAL_OK)
    {
      ret = BSP_ERROR_PERIPH_FAILURE;
    }
    else
    {
      IsMspCallbacksValid = 1;
    }
  }
  /* Return BSP status */
  return ret;
}

int32_t BSP_SDRAM_RegisterMspCallbacks (uint32_t Instance, BSP_SDRAM_Cb_t *CallBacks)
{
  int32_t ret = BSP_ERROR_NONE;

  /* Check if the instance is supported */
  if(Instance >=SDRAM_INSTANCES_NBR)
  {
    ret = BSP_ERROR_WRONG_PARAM;
  }
  else
  {
    /* Register MspInit/MspDeInit Callbacks */
    if(HAL_SDRAM_RegisterCallback(&hsdram[Instance], HAL_SDRAM_MSP_INIT_CB_ID, CallBacks->pMspInitCb) != HAL_OK)
    {
      ret = BSP_ERROR_PERIPH_FAILURE;
    }
    if(HAL_SDRAM_RegisterCallback(&hsdram[Instance], HAL_SDRAM_MSP_DEINIT_CB_ID, CallBacks->pMspDeInitCb) != HAL_OK)
    {
      ret = BSP_ERROR_PERIPH_FAILURE;
    }
    else
    {
      IsMspCallbacksValid = 1;
    }
  }
  /* Return BSP status */
  return ret;
}
#endif /* (USE_HAL_SDRAM_REGISTER_CALLBACKS == 1) */

int32_t BSP_SDRAM_SendCmd(uint32_t Instance, FMC_SDRAM_CommandTypeDef *SdramCmd)
{
	int32_t ret;

	if(Instance >=SDRAM_INSTANCES_NBR)
	{
		ret =  BSP_ERROR_WRONG_PARAM;
	}
	#ifdef USE_MT48LC4M32B2P
	else if(MT48LC4M32B2P_Sendcmd(&hsdram[Instance], SdramCmd) != MT48LC4M32B2P_OK)
	#endif
	#ifdef USE_IS42S32160F
	else if(IS42S32160F_Sendcmd(&hsdram[Instance], SdramCmd) != IS42S32160F_OK)
	#endif
	{
		ret = BSP_ERROR_PERIPH_FAILURE;
	}
	else
	{
		ret = BSP_ERROR_NONE;
	}

	return ret;
}

void BSP_SDRAM_IRQHandler(uint32_t Instance)
{
  /* Prevent unused argument(s) compilation warning */
  UNUSED(Instance);

  HAL_MDMA_IRQHandler(hsdram[Instance].hmdma);
}


