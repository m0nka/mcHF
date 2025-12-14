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

#include "storage_proc.h"
#include "sd_card.h"

#ifdef CONTEXT_SD

SD_HandleTypeDef hsd_sdmmc[SD_INSTANCES_NBR];
//EXTI_HandleTypeDef hsd_exti[SD_INSTANCES_NBR];

//*----------------------------------------------------------------------------
//* Function Name       : SDMMC1_IRQHandler
//* Object              :
//* Notes    			:
//* Notes   			:
//* Notes    			:
//* Context    			: CONTEXT_IRQ
//*----------------------------------------------------------------------------
void SDMMC1_IRQHandler(uint32_t Instance)
{
	HAL_SD_IRQHandler(&hsd_sdmmc[Instance]);
}

//*----------------------------------------------------------------------------
//* Function Name       : HAL_SD_TxCpltCallback
//* Object              :
//* Notes    			:
//* Notes   			:
//* Notes    			:
//* Context    			: CONTEXT_IRQ
//*----------------------------------------------------------------------------
void HAL_SD_TxCpltCallback(SD_HandleTypeDef *hsd)
{
	#ifdef SD_USE_DMA
	BSP_SD_WriteCpltCallback(0);
	#endif
}

//*----------------------------------------------------------------------------
//* Function Name       : HAL_SD_RxCpltCallback
//* Object              :
//* Notes    			:
//* Notes   			:
//* Notes    			:
//* Context    			: CONTEXT_IRQ
//*----------------------------------------------------------------------------
void HAL_SD_RxCpltCallback(SD_HandleTypeDef *hsd)
{
	#ifdef SD_USE_DMA
	BSP_SD_ReadCpltCallback(0);
	#endif
}

//*----------------------------------------------------------------------------
//* Function Name       : HAL_SD_ErrorCallback
//* Object              :
//* Notes    			:
//* Notes   			:
//* Notes    			:
//* Context    			: CONTEXT_IRQ
//*----------------------------------------------------------------------------
void HAL_SD_ErrorCallback(SD_HandleTypeDef *hsd)
{
	#ifdef SD_USE_DMA
	BSP_SD_ErrorCallback();
	#else
	printf("== sd error == \r\n");
	#endif
}

//*----------------------------------------------------------------------------
//* Function Name       : HAL_SD_AbortCallback
//* Object              :
//* Notes    			:
//* Notes   			:
//* Notes    			:
//* Context    			: CONTEXT_IRQ
//*----------------------------------------------------------------------------
void HAL_SD_AbortCallback(SD_HandleTypeDef *hsd)
{
	#ifdef SD_USE_DMA
	BSP_SD_AbortCallback(0);
	#endif
}

static void SD_MspInit(SD_HandleTypeDef *hsd)
{
	#ifdef SD_USE_PLL2
	RCC_PeriphCLKInitTypeDef 	PeriphClkInitStruct = {0};
	#endif
	GPIO_InitTypeDef 			gpio_init_structure;

	if(hsd == &hsd_sdmmc[0])
	{
		//printf("SD_MspInit  \r\n");

		// SD Card clock - 18.75Mhz
		#ifdef SD_USE_PLL2
		PeriphClkInitStruct.PeriphClockSelection	= RCC_PERIPHCLK_SDMMC;
		PeriphClkInitStruct.SdmmcClockSelection		= RCC_SDMMCCLKSOURCE_PLL2;
		PeriphClkInitStruct.PLL2.PLL2R           	= 8;
		HAL_RCCEx_PeriphCLKConfig(&PeriphClkInitStruct);
		#endif

		// Enable SDIO clock
		__HAL_RCC_SDMMC1_CLK_ENABLE();

		// Common GPIO configuration
		gpio_init_structure.Mode      = GPIO_MODE_AF_PP;
		gpio_init_structure.Pull      = GPIO_PULLUP;
		gpio_init_structure.Speed     = GPIO_SPEED_FREQ_VERY_HIGH;
		gpio_init_structure.Alternate = GPIO_AF12_SDIO1;

		gpio_init_structure.Pin = SDMMC1_D0;
		HAL_GPIO_Init(SDMMC1_SDIO_PORTC, &gpio_init_structure);

		gpio_init_structure.Pin = SDMMC1_D1;
		HAL_GPIO_Init(SDMMC1_SDIO_PORTC, &gpio_init_structure);

		gpio_init_structure.Pin = SDMMC1_D2;
		HAL_GPIO_Init(SDMMC1_SDIO_PORTC, &gpio_init_structure);

		gpio_init_structure.Pin = SDMMC1_D3;
		HAL_GPIO_Init(SDMMC1_SDIO_PORTC, &gpio_init_structure);

		gpio_init_structure.Pin = SDMMC1_CLK;
		HAL_GPIO_Init(SDMMC1_SDIO_PORTC, &gpio_init_structure);

		gpio_init_structure.Pin = SDMMC1_CMD;
		HAL_GPIO_Init(SDMMC1_SDIO_PORTD, &gpio_init_structure);

		// Configure Input mode for SD detection pin
		gpio_init_structure.Pin 	= SD_DET;
		gpio_init_structure.Pull 	= GPIO_PULLUP;
		gpio_init_structure.Speed 	= GPIO_SPEED_FREQ_HIGH;
		gpio_init_structure.Mode 	= GPIO_MODE_INPUT;
		HAL_GPIO_Init(SD_DET_PORT, &gpio_init_structure);

		gpio_init_structure.Pin   	= SD_PWR_CNTR;
		gpio_init_structure.Mode  	= GPIO_MODE_OUTPUT_PP;
		gpio_init_structure.Pull  	= GPIO_PULLUP;
		gpio_init_structure.Speed 	= GPIO_SPEED_FREQ_LOW;
		HAL_GPIO_Init(SD_PWR_CNTR_PORT, &gpio_init_structure);

		// Card power state on reset
		if(HAL_GPIO_ReadPin(SD_DET_PORT, SD_DET) != GPIO_PIN_RESET)
		{
			// Power off
			#ifdef SD_PWR_SWAP_POLARITY
			HAL_GPIO_WritePin(SD_PWR_CNTR_PORT, SD_PWR_CNTR, GPIO_PIN_RESET);
			#else
			HAL_GPIO_WritePin(SD_PWR_CNTR_PORT, SD_PWR_CNTR, GPIO_PIN_SET);
			#endif
		}
		else
		{
			// Power on
			#ifndef SD_PWR_SWAP_POLARITY
			HAL_GPIO_WritePin(SD_PWR_CNTR_PORT, SD_PWR_CNTR, GPIO_PIN_RESET);
			#else
			HAL_GPIO_WritePin(SD_PWR_CNTR_PORT, SD_PWR_CNTR, GPIO_PIN_SET);
			#endif
		}

		__HAL_RCC_SDMMC1_FORCE_RESET();
		__HAL_RCC_SDMMC1_RELEASE_RESET();

		// NVIC configuration for SDIO interrupts
		HAL_NVIC_SetPriority(SDMMC1_IRQn, 14, 0);
		HAL_NVIC_EnableIRQ	(SDMMC1_IRQn);
	}
}

#if 0
static void SD_MspDeInit(SD_HandleTypeDef *hsd)
{
	GPIO_InitTypeDef gpio_init_structure;

	if(hsd == &hsd_sdmmc[0])
	{
		// Power off
		#ifdef SD_PWR_SWAP_POLARITY
		HAL_GPIO_WritePin(SD_PWR_CNTR_PORT, SD_PWR_CNTR, GPIO_PIN_RESET);
		#else
		HAL_GPIO_WritePin(SD_PWR_CNTR_PORT, SD_PWR_CNTR, GPIO_PIN_SET);
		#endif

		HAL_NVIC_DisableIRQ(SDMMC1_IRQn);

		/* Disable SDMMC1 clock */
		__HAL_RCC_SDMMC1_CLK_DISABLE();

		/* GPIOC configuration */
		gpio_init_structure.Pin = SDMMC1_D0 | SDMMC1_D1 | SDMMC1_D2 | SDMMC1_D3 | SDMMC1_CLK;
		HAL_GPIO_DeInit(SDMMC1_SDIO_PORTC, gpio_init_structure.Pin);

		/* GPIOD configuration */
		gpio_init_structure.Pin = SDMMC1_CMD;
		HAL_GPIO_DeInit(SDMMC1_SDIO_PORTD, gpio_init_structure.Pin);
	}
}
#endif

static uint32_t SD_SendSDStatusA(SD_HandleTypeDef *hsd, uint32_t *pSDstatus)
{
  SDMMC_DataInitTypeDef config;
  uint32_t errorstate;
  uint32_t tickstart = HAL_GetTick();
  uint32_t count;
  uint32_t *pData = pSDstatus;

  /* Check SD response */
  if ((SDMMC_GetResponse(hsd->Instance, SDMMC_RESP1) & SDMMC_CARD_LOCKED) == SDMMC_CARD_LOCKED)
  {
    return HAL_SD_ERROR_LOCK_UNLOCK_FAILED;
  }

  /* Set block size for card if it is not equal to current block size for card */
  errorstate = SDMMC_CmdBlockLength(hsd->Instance, 64U);
  if (errorstate != HAL_SD_ERROR_NONE)
  {
    hsd->ErrorCode |= HAL_SD_ERROR_NONE;
    return errorstate;
  }

  /* Send CMD55 */
  errorstate = SDMMC_CmdAppCommand(hsd->Instance, (uint32_t)(hsd->SdCard.RelCardAdd << 16U));
  if (errorstate != HAL_SD_ERROR_NONE)
  {
    hsd->ErrorCode |= HAL_SD_ERROR_NONE;
    return errorstate;
  }

  /* Configure the SD DPSM (Data Path State Machine) */
  config.DataTimeOut   = SDMMC_DATATIMEOUT;
  config.DataLength    = 64U;
  config.DataBlockSize = SDMMC_DATABLOCK_SIZE_64B;
  config.TransferDir   = SDMMC_TRANSFER_DIR_TO_SDMMC;
  config.TransferMode  = SDMMC_TRANSFER_MODE_BLOCK;
  config.DPSM          = SDMMC_DPSM_ENABLE;
  (void)SDMMC_ConfigData(hsd->Instance, &config);

  /* Send ACMD13 (SD_APP_STAUS)  with argument as card's RCA */
  errorstate = SDMMC_CmdStatusRegister(hsd->Instance);
  if (errorstate != HAL_SD_ERROR_NONE)
  {
    hsd->ErrorCode |= HAL_SD_ERROR_NONE;
    return errorstate;
  }

  /* Get status data */
  while (!__HAL_SD_GET_FLAG(hsd, SDMMC_FLAG_RXOVERR | SDMMC_FLAG_DCRCFAIL | SDMMC_FLAG_DTIMEOUT | SDMMC_FLAG_DATAEND))
  {
    if (__HAL_SD_GET_FLAG(hsd, SDMMC_FLAG_RXFIFOHF))
    {
      for (count = 0U; count < 8U; count++)
      {
        *pData = SDMMC_ReadFIFO(hsd->Instance);
        pData++;
      }
    }

    if ((HAL_GetTick() - tickstart) >=  SDMMC_SWDATATIMEOUT)
    {
      return HAL_SD_ERROR_TIMEOUT;
    }
  }

  if (__HAL_SD_GET_FLAG(hsd, SDMMC_FLAG_DTIMEOUT))
  {
    return HAL_SD_ERROR_DATA_TIMEOUT;
  }
  else if (__HAL_SD_GET_FLAG(hsd, SDMMC_FLAG_DCRCFAIL))
  {
    return HAL_SD_ERROR_DATA_CRC_FAIL;
  }
  else if (__HAL_SD_GET_FLAG(hsd, SDMMC_FLAG_RXOVERR))
  {
    return HAL_SD_ERROR_RX_OVERRUN;
  }
  else
  {
    /* Nothing to do */
  }

  while ((__HAL_SD_GET_FLAG(hsd, SDMMC_FLAG_DPSMACT)))
  {
    *pData = SDMMC_ReadFIFO(hsd->Instance);
    pData++;

    if ((HAL_GetTick() - tickstart) >=  SDMMC_SWDATATIMEOUT)
    {
      return HAL_SD_ERROR_TIMEOUT;
    }
  }

  /* Clear all the static status flags*/
  __HAL_SD_CLEAR_FLAG(hsd, SDMMC_STATIC_DATA_FLAGS);

  return HAL_SD_ERROR_NONE;
}

HAL_StatusTypeDef HAL_SD_GetCardStatusA(SD_HandleTypeDef *hsd, HAL_SD_CardStatusTypeDef *pStatus)
{
  uint32_t sd_status[16];
  uint32_t errorstate;
  HAL_StatusTypeDef status = HAL_OK;

  if (hsd->State == HAL_SD_STATE_BUSY)
  {
    return HAL_ERROR;
  }

  errorstate = SD_SendSDStatusA(hsd, sd_status);
  if (errorstate != HAL_SD_ERROR_NONE)
  {
    /* Clear all the static flags */
    __HAL_SD_CLEAR_FLAG(hsd, SDMMC_STATIC_FLAGS);
    hsd->ErrorCode |= errorstate;
    hsd->State = HAL_SD_STATE_READY;
    status = HAL_ERROR;
  }
  else
  {
    pStatus->DataBusWidth = (uint8_t)((sd_status[0] & 0xC0U) >> 6U);

    pStatus->SecuredMode = (uint8_t)((sd_status[0] & 0x20U) >> 5U);

    pStatus->CardType = (uint16_t)(((sd_status[0] & 0x00FF0000U) >> 8U) | ((sd_status[0] & 0xFF000000U) >> 24U));

    pStatus->ProtectedAreaSize = (((sd_status[1] & 0xFFU) << 24U)    | ((sd_status[1] & 0xFF00U) << 8U) |
                                  ((sd_status[1] & 0xFF0000U) >> 8U) | ((sd_status[1] & 0xFF000000U) >> 24U));

    pStatus->SpeedClass = (uint8_t)(sd_status[2] & 0xFFU);

    pStatus->PerformanceMove = (uint8_t)((sd_status[2] & 0xFF00U) >> 8U);

    pStatus->AllocationUnitSize = (uint8_t)((sd_status[2] & 0xF00000U) >> 20U);

    pStatus->EraseSize = (uint16_t)(((sd_status[2] & 0xFF000000U) >> 16U) | (sd_status[3] & 0xFFU));

    pStatus->EraseTimeout = (uint8_t)((sd_status[3] & 0xFC00U) >> 10U);

    pStatus->EraseOffset = (uint8_t)((sd_status[3] & 0x0300U) >> 8U);

    pStatus->UhsSpeedGrade = (uint8_t)((sd_status[3] & 0x00F0U) >> 4U);
    pStatus->UhsAllocationUnitSize = (uint8_t)(sd_status[3] & 0x000FU) ;
    pStatus->VideoSpeedClass = (uint8_t)((sd_status[4] & 0xFF000000U) >> 24U);
  }

  /* Set Block Size for Card */
  errorstate = SDMMC_CmdBlockLength(hsd->Instance, BLOCKSIZE);
  if (errorstate != HAL_SD_ERROR_NONE)
  {
    /* Clear all the static flags */
    __HAL_SD_CLEAR_FLAG(hsd, SDMMC_STATIC_FLAGS);
    hsd->ErrorCode = errorstate;
    hsd->State = HAL_SD_STATE_READY;
    status = HAL_ERROR;
  }

  return status;
}

//
// RTOS safe re-implementation of the lib call
//
static HAL_StatusTypeDef HAL_SD_InitA(SD_HandleTypeDef *hsd)
{
	HAL_SD_CardStatusTypeDef CardStatus;
	uint32_t speedgrade;
	uint32_t unitsize;
	uint32_t tickstart;

	/* Check the SD handle allocation */
	if (hsd == NULL)
	{
		return HAL_ERROR;
	}

	if(hsd->State == HAL_SD_STATE_RESET)
	{
		/* Allocate lock resource and initialize it */
		hsd->Lock = HAL_UNLOCKED;

		/* Init the low level hardware : GPIO, CLOCK, CORTEX...etc */
		HAL_SD_MspInit(hsd);
	}

	hsd->State = HAL_SD_STATE_PROGRAMMING;

	/* Initialize the Card parameters */
	if (HAL_SD_InitCard(hsd) != HAL_OK)
	{
		return HAL_ERROR;
	}

	printf("stall in..  \r\n");

	if(HAL_SD_GetCardStatusA(hsd, &CardStatus) != HAL_OK)
	{
		return HAL_ERROR;
	}

	printf("stall out  \r\n");

	/* Get Initial Card Speed from Card Status*/
	speedgrade = CardStatus.UhsSpeedGrade;
	unitsize = CardStatus.UhsAllocationUnitSize;
	if ((hsd->SdCard.CardType == CARD_SDHC_SDXC) && ((speedgrade != 0U) || (unitsize != 0U)))
	{
		hsd->SdCard.CardSpeed = CARD_ULTRA_HIGH_SPEED;
	}
	else
	{
		if (hsd->SdCard.CardType == CARD_SDHC_SDXC)
		{
			hsd->SdCard.CardSpeed  = CARD_HIGH_SPEED;
		}
		else
		{
			hsd->SdCard.CardSpeed  = CARD_NORMAL_SPEED;
		}
	}

	/* Configure the bus wide */
	if (HAL_SD_ConfigWideBusOperation(hsd, hsd->Init.BusWide) != HAL_OK)
	{
		return HAL_ERROR;
	}

	/* Verify that SD card is ready to use after Initialization */
	tickstart = HAL_GetTick();
	while ((HAL_SD_GetCardState(hsd) != HAL_SD_CARD_TRANSFER))
	{
		if ((HAL_GetTick() - tickstart) >=  SDMMC_SWDATATIMEOUT)
		{
			hsd->ErrorCode = HAL_SD_ERROR_TIMEOUT;
			hsd->State = HAL_SD_STATE_READY;
			return HAL_TIMEOUT;
		}

		vTaskDelay(5);
	}

	/* Initialize the error code */
	hsd->ErrorCode = HAL_SD_ERROR_NONE;

	/* Initialize the SD operation */
	hsd->Context = SD_CONTEXT_NONE;

	/* Initialize the SD state */
	hsd->State = HAL_SD_STATE_READY;

	return HAL_OK;
}

static HAL_StatusTypeDef sdmmc1_init(SD_HandleTypeDef *hsd)
{
	HAL_StatusTypeDef ret = HAL_OK;

	// uSD device interface configuration
	hsd->Instance                 = SDMMC1;
	hsd->Init.ClockEdge           = SDMMC_CLOCK_EDGE_RISING;
	hsd->Init.ClockPowerSave      = SDMMC_CLOCK_POWER_SAVE_DISABLE;
	hsd->Init.BusWide             = SDMMC_BUS_WIDE_4B;
	hsd->Init.HardwareFlowControl = SDMMC_HARDWARE_FLOW_CONTROL_DISABLE;
	hsd->Init.ClockDiv            = SDMMC_NSpeed_CLK_DIV;

	// HAL SD initialization
	if(HAL_SD_InitA(hsd) != HAL_OK)
	{
		ret = HAL_ERROR;
	}
	//printf("card speed: %d \r\n", (int)hsd->SdCard.CardSpeed);

	return ret;
}

//
// ToDo: Fix it, as it doesn't work(needs CPU restart)
//
void sd_card_reset_driver(void)
{
	SDMMC_PowerState_OFF(hsd_sdmmc[0].Instance);

	hsd_sdmmc[0].ErrorCode = HAL_SD_ERROR_NONE;
	hsd_sdmmc[0].State = HAL_SD_STATE_RESET;
}

void sd_card_low_level_init(uint32_t Instance)
{
	// Msp SD initialization
	SD_MspInit(&hsd_sdmmc[Instance]);
}

void sd_card_power(uchar state)
{
	if(state)
	{
		// Power on
		#ifndef SD_PWR_SWAP_POLARITY
		HAL_GPIO_WritePin(SD_PWR_CNTR_PORT, SD_PWR_CNTR, GPIO_PIN_RESET);
		#else
		HAL_GPIO_WritePin(SD_PWR_CNTR_PORT, SD_PWR_CNTR, GPIO_PIN_SET);
		#endif
	}
	else
	{
		// Power off
		#ifdef SD_PWR_SWAP_POLARITY
		HAL_GPIO_WritePin(SD_PWR_CNTR_PORT, SD_PWR_CNTR, GPIO_PIN_RESET);
		#else
		HAL_GPIO_WritePin(SD_PWR_CNTR_PORT, SD_PWR_CNTR, GPIO_PIN_SET);
		#endif
	}
}

int32_t sd_card_init(uint32_t Instance)
{
	if(BSP_SD_IsDetected() != SD_PRESENT)
	{
		//printf("no card  \r\n");
		return BSP_ERROR_UNKNOWN_COMPONENT;
	}

	//  HAL SD initialization and Enable wide operation
	if(sdmmc1_init(&hsd_sdmmc[Instance]) != HAL_OK)
	{
		//printf("sd_card_init err3  \r\n");
		return BSP_ERROR_PERIPH_FAILURE;
	}

	//printf("sd_card_init.. ok \r\n");
	return BSP_ERROR_NONE;
}

int32_t sd_card_set_exti_irq(uint32_t Instance)
{
	GPIO_InitTypeDef gpio_init_structure;

	gpio_init_structure.Pin 	= SD_DET;
	gpio_init_structure.Pull 	= GPIO_PULLUP;
	gpio_init_structure.Speed 	= GPIO_SPEED_FREQ_LOW;
	gpio_init_structure.Mode 	= GPIO_MODE_IT_RISING_FALLING;
	HAL_GPIO_Init(SD_DET_PORT, &gpio_init_structure);

	HAL_NVIC_SetPriority(EXTI0_IRQn, 15U, 0x00);
	HAL_NVIC_EnableIRQ  (EXTI0_IRQn);

	return BSP_ERROR_NONE;
}

int32_t BSP_SD_IsDetected(void)
{
	int32_t ret = BSP_ERROR_UNKNOWN_FAILURE;

	ret = (uint32_t)HAL_GPIO_ReadPin(SD_DET_PORT, SD_DET);

	// Check SD card detect pin
	if(ret != GPIO_PIN_RESET)
		ret = (int32_t)SD_NOT_PRESENT;
	else
		ret = (int32_t)SD_PRESENT;

	return ret;
}

int32_t BSP_SD_ReadBlocks(uint32_t Instance, uint32_t *pData, uint32_t BlockIdx, uint32_t BlocksNbr)
{
	int32_t  ret = BSP_ERROR_NONE;
	uint32_t timeout = SD_READ_TIMEOUT*BlocksNbr;

	if(Instance >= SD_INSTANCES_NBR)
	{
		ret = BSP_ERROR_WRONG_PARAM;
	}
	else
	{
    	//
    	// ToDo: Fix this!!!
    	//
    	vTaskDelay(15);

		if(HAL_SD_ReadBlocks(&hsd_sdmmc[Instance], (uint8_t *)pData, BlockIdx, BlocksNbr, timeout) != HAL_OK)
		{
			//sd_card_reset_driver();

			//printf("rd err: 0x%x \r\n", hsd_sdmmc[Instance].ErrorCode);
			ret = BSP_ERROR_PERIPH_FAILURE;
		}
	}

	// Return BSP status
	return ret;
}

int32_t BSP_SD_WriteBlocks(uint32_t Instance, uint32_t *pData, uint32_t BlockIdx, uint32_t BlocksNbr)
{
	int32_t ret = BSP_ERROR_NONE;
	uint32_t timeout = SD_READ_TIMEOUT*BlocksNbr;

	if(Instance >= SD_INSTANCES_NBR)
	{
		ret = BSP_ERROR_WRONG_PARAM;
	}
	else
	{
		if(HAL_SD_WriteBlocks(&hsd_sdmmc[Instance], (uint8_t *)pData, BlockIdx, BlocksNbr, timeout) != HAL_OK)
		{
			ret = BSP_ERROR_PERIPH_FAILURE;
		}
	}

	// Return BSP status
	return ret;
}

int32_t BSP_SD_ReadBlocks_DMA(uint32_t Instance, uint32_t *pData, uint32_t BlockIdx, uint32_t BlocksNbr)
{
	int32_t ret = BSP_ERROR_NONE;

	if(Instance >= SD_INSTANCES_NBR)
	{
		ret = BSP_ERROR_WRONG_PARAM;
	}
	else
	{
		#if 1
		//printf("read %d %d  \r\n", BlockIdx, BlocksNbr);
		if(HAL_SD_ReadBlocks_DMA(&hsd_sdmmc[Instance], (uint8_t *)pData, BlockIdx, BlocksNbr) != HAL_OK)
		{
			ret = BSP_ERROR_PERIPH_FAILURE;
		}
		#else
		ret = 0;
		#endif
	}

	// Return BSP status
	return ret;
}

int32_t BSP_SD_WriteBlocks_DMA(uint32_t Instance, uint32_t *pData, uint32_t BlockIdx, uint32_t BlocksNbr)
{
	int32_t ret = BSP_ERROR_NONE;

	if(Instance >= SD_INSTANCES_NBR)
	{
		ret = BSP_ERROR_WRONG_PARAM;
	}
	else
	{
		if(HAL_SD_WriteBlocks_DMA(&hsd_sdmmc[Instance], (uint8_t *)pData, BlockIdx, BlocksNbr) != HAL_OK)
		{
			ret = BSP_ERROR_PERIPH_FAILURE;
		}
	}

	// Return BSP status
	return ret;
}

#if 0
int32_t BSP_SD_ReadBlocks_IT(uint32_t Instance, uint32_t *pData, uint32_t BlockIdx, uint32_t BlocksNbr)
{
	int32_t ret = BSP_ERROR_NONE;

	if(Instance >= SD_INSTANCES_NBR)
	{
		ret = BSP_ERROR_WRONG_PARAM;
	}
	else
	{
		if(HAL_SD_ReadBlocks_IT(&hsd_sdmmc[Instance], (uint8_t *)pData, BlockIdx, BlocksNbr) != HAL_OK)
		{
			ret = BSP_ERROR_PERIPH_FAILURE;
		}
	}

	// Return BSP status
	return ret;
}
#endif

#if 0
int32_t BSP_SD_WriteBlocks_IT(uint32_t Instance, uint32_t *pData, uint32_t BlockIdx, uint32_t BlocksNbr)
{
	int32_t ret = BSP_ERROR_NONE;

	if(Instance >= SD_INSTANCES_NBR)
	{
		ret = BSP_ERROR_WRONG_PARAM;
	}
	else
	{
		if(HAL_SD_WriteBlocks_IT(&hsd_sdmmc[Instance], (uint8_t *)pData, BlockIdx, BlocksNbr) != HAL_OK)
		{
			ret = BSP_ERROR_PERIPH_FAILURE;
		}
	}

	// Return BSP status
	return ret;
}
#endif

#if 0
int32_t BSP_SD_Erase(uint32_t Instance, uint32_t BlockIdx, uint32_t BlocksNbr)
{
	int32_t ret = BSP_ERROR_NONE;

	if(Instance >= SD_INSTANCES_NBR)
	{
		ret = BSP_ERROR_WRONG_PARAM;
	}
	else
	{
		if(HAL_SD_Erase(&hsd_sdmmc[Instance], BlockIdx, BlockIdx + BlocksNbr) != HAL_OK)
		{
			ret = BSP_ERROR_PERIPH_FAILURE;
		}
	}

	// Return BSP status
	return ret;
}
#endif

int32_t BSP_SD_GetCardState(uint32_t Instance)
{
	return (int32_t)((HAL_SD_GetCardState(&hsd_sdmmc[Instance]) == HAL_SD_CARD_TRANSFER ) ? SD_TRANSFER_OK : SD_TRANSFER_BUSY);
}

int32_t BSP_SD_GetCardInfo(uint32_t Instance, BSP_SD_CardInfo *CardInfo)
{
	int32_t ret = BSP_ERROR_NONE;

	if(Instance >= SD_INSTANCES_NBR)
	{
		ret = BSP_ERROR_WRONG_PARAM;
	}
	else
	{
		if(HAL_SD_GetCardInfo(&hsd_sdmmc[Instance], CardInfo) != HAL_OK)
		{
			ret = BSP_ERROR_PERIPH_FAILURE;
		}
	}

	// Return BSP status
	return ret;
}

#endif
