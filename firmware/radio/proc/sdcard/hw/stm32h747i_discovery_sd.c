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

#include "stm32h747i_discovery_sd.h"
//#include "stm32h747i_discovery_bus.h"

#ifdef CONTEXT_SD

#if (USE_HAL_SD_REGISTER_CALLBACKS == 1)
/* Is Msp Callbacks registered */
static uint32_t   IsMspCallbacksValid[SD_INSTANCES_NBR] = {0};
#endif
typedef void (* BSP_EXTI_LineCallback) (void);

SD_HandleTypeDef hsd_sdmmc[SD_INSTANCES_NBR];
EXTI_HandleTypeDef hsd_exti[SD_INSTANCES_NBR];

//static uint32_t PinDetect[SD_INSTANCES_NBR]  = {SD_DETECT_PIN};

#if (USE_HAL_SD_REGISTER_CALLBACKS == 1)
// Is Msp Callbacks registered
static uint32_t   IsMspCallbacksValid[SD_INSTANCES_NBR] = {0};
#endif

static void SD_MspInit(SD_HandleTypeDef *hsd);
static void SD_MspDeInit(SD_HandleTypeDef *hsd);
#if (USE_HAL_SD_REGISTER_CALLBACKS == 1)
static void SD_AbortCallback(SD_HandleTypeDef *hsd);
static void SD_TxCpltCallback(SD_HandleTypeDef *hsd);
static void SD_RxCpltCallback(SD_HandleTypeDef *hsd);
#if (USE_SD_TRANSCEIVER > 0U)
static void SD_DriveTransceiver_1_8V_Callback(FlagStatus status);
#endif
#endif // (USE_HAL_SD_REGISTER_CALLBACKS == 1)
static void SD_EXTI_Callback(void);

int32_t BSP_SD_Init(uint32_t Instance)
{
	int32_t ret = BSP_ERROR_NONE;

	if(Instance >= SD_INSTANCES_NBR)
	{
		ret = BSP_ERROR_WRONG_PARAM;
	}
	else
	{
		if(BSP_SD_IsDetected(Instance) != SD_PRESENT)
		{
			ret = BSP_ERROR_UNKNOWN_COMPONENT;
		}
		else
		{
			#if (USE_HAL_SD_REGISTER_CALLBACKS == 1)
			// Register the SD MSP Callbacks
			if(IsMspCallbacksValid[Instance] == 0UL)
			{
				if(BSP_SD_RegisterDefaultMspCallbacks(Instance) != BSP_ERROR_NONE)
				{
					ret = BSP_ERROR_PERIPH_FAILURE;
				}
			}
			#else
			// Msp SD initialization
			SD_MspInit(&hsd_sdmmc[Instance]);
			#endif // USE_HAL_SD_REGISTER_CALLBACKS

			if(ret == BSP_ERROR_NONE)
			{
				//  HAL SD initialization and Enable wide operation
				if(MX_SDMMC1_SD_Init(&hsd_sdmmc[Instance]) != HAL_OK)
				{
					ret = BSP_ERROR_PERIPH_FAILURE;
				}
				else if(HAL_SD_ConfigWideBusOperation(&hsd_sdmmc[Instance], SDMMC_BUS_WIDE_4B) != HAL_OK)
				{
					ret = BSP_ERROR_PERIPH_FAILURE;
				}
				else
				{
					// Switch to High Speed mode if the card support this mode
					(void)HAL_SD_ConfigSpeedBusOperation(&hsd_sdmmc[Instance], SDMMC_SPEED_MODE_HIGH);

					#if (USE_HAL_SD_REGISTER_CALLBACKS == 1)
					// Register SD TC, HT and Abort callbacks
					if(HAL_SD_RegisterCallback(&hsd_sdmmc[Instance], HAL_SD_TX_CPLT_CB_ID, SD_TxCpltCallback) != HAL_OK)
					{
						ret = BSP_ERROR_PERIPH_FAILURE;
					}
					else if(HAL_SD_RegisterCallback(&hsd_sdmmc[Instance], HAL_SD_RX_CPLT_CB_ID, SD_RxCpltCallback) != HAL_OK)
					{
						ret = BSP_ERROR_PERIPH_FAILURE;
					}
					else if(HAL_SD_RegisterCallback(&hsd_sdmmc[Instance], HAL_SD_ABORT_CB_ID, SD_AbortCallback) != HAL_OK)
					{
						ret = BSP_ERROR_PERIPH_FAILURE;
					}
					else
					{
						#if (USE_SD_TRANSCEIVER != 0U)
						if(HAL_SD_RegisterTransceiverCallback(&hsd_sdmmc[Instance], SD_DriveTransceiver_1_8V_Callback) != HAL_OK)
						{
							ret = BSP_ERROR_PERIPH_FAILURE;
						}
						#endif
					}
					#endif // USE_HAL_SD_REGISTER_CALLBACKS
				}
			}
		}
	}

	return ret;
}

int32_t BSP_SD_DeInit(uint32_t Instance)
{
	int32_t ret = BSP_ERROR_NONE;

	if(Instance >= SD_INSTANCES_NBR)
	{
		ret = BSP_ERROR_WRONG_PARAM;
	}
	else
	{
		if(HAL_SD_DeInit(&hsd_sdmmc[Instance]) != HAL_OK)// HAL SD de-initialization
		{
			ret = BSP_ERROR_PERIPH_FAILURE;
		}
		else
		{
			// Msp SD de-initialization
			#if (USE_HAL_SD_REGISTER_CALLBACKS == 0)
			SD_MspDeInit(&hsd_sdmmc[Instance]);
			#endif // (USE_HAL_SD_REGISTER_CALLBACKS == 0)
		}
	}

	return ret;
}

__weak HAL_StatusTypeDef MX_SDMMC1_SD_Init(SD_HandleTypeDef *hsd)
{
	HAL_StatusTypeDef ret = HAL_OK;

	// uSD device interface configuration
	hsd->Instance                 = SDMMC1;
	hsd->Init.ClockEdge           = SDMMC_CLOCK_EDGE_RISING;
	hsd->Init.ClockPowerSave      = SDMMC_CLOCK_POWER_SAVE_DISABLE;
	hsd->Init.BusWide             = SDMMC_BUS_WIDE_4B;
	hsd->Init.HardwareFlowControl = SDMMC_HARDWARE_FLOW_CONTROL_DISABLE;
	#if (USE_SD_TRANSCEIVER >0)
	hsd->Init.TranceiverPresent   = SDMMC_TRANSCEIVER_PRESENT;
	#endif //USE_SD_TRANSCEIVER
	#if ( USE_SD_HIGH_PERFORMANCE > 0 )
	hsd->Init.ClockDiv            = SDMMC_HSpeed_CLK_DIV;
	#else
	hsd->Init.ClockDiv            = SDMMC_NSpeed_CLK_DIV;
	#endif // USE_SD_HIGH_PERFORMANCE

	// HAL SD initialization
	if(HAL_SD_Init(hsd) != HAL_OK)
	{
		ret = HAL_ERROR;
	}

	return ret;
}

#if (USE_HAL_SD_REGISTER_CALLBACKS == 1)
int32_t BSP_SD_RegisterDefaultMspCallbacks(uint32_t Instance)
{
  int32_t ret = BSP_ERROR_NONE;

  if(Instance >= SD_INSTANCES_NBR)
  {
    ret = BSP_ERROR_WRONG_PARAM;
  }
  else
  {
    // Register MspInit/MspDeInit Callbacks
    if(HAL_SD_RegisterCallback(&hsd_sdmmc[Instance], HAL_SD_MSP_INIT_CB_ID, SD_MspInit) != HAL_OK)
    {
      ret = BSP_ERROR_PERIPH_FAILURE;
    }
    else if(HAL_SD_RegisterCallback(&hsd_sdmmc[Instance], HAL_SD_MSP_DEINIT_CB_ID, SD_MspDeInit) != HAL_OK)
    {
      ret = BSP_ERROR_PERIPH_FAILURE;
    }
    else
    {
      IsMspCallbacksValid[Instance] = 1U;
    }
  }

  // Return BSP status
  return ret;
}

int32_t BSP_SD_RegisterMspCallbacks(uint32_t Instance, BSP_SD_Cb_t *CallBacks)
{
  int32_t ret = BSP_ERROR_NONE;

  if(Instance >= SD_INSTANCES_NBR)
  {
    ret = BSP_ERROR_WRONG_PARAM;
  }
  else
  {
    // Register MspInit/MspDeInit Callbacks
    if(HAL_SD_RegisterCallback(&hsd_sdmmc[Instance], HAL_SD_MSP_INIT_CB_ID, CallBacks->pMspInitCb) != HAL_OK)
    {
      ret = BSP_ERROR_PERIPH_FAILURE;
    }
    else if(HAL_SD_RegisterCallback(&hsd_sdmmc[Instance], HAL_SD_MSP_DEINIT_CB_ID, CallBacks->pMspDeInitCb) != HAL_OK)
    {
      ret = BSP_ERROR_PERIPH_FAILURE;
    }
    else
    {
      IsMspCallbacksValid[Instance] = 1U;
    }
  }

  // Return BSP status
  return ret;
}
#endif // (USE_HAL_SD_REGISTER_CALLBACKS == 1)

int32_t BSP_SD_DetectITConfig(uint32_t Instance)
{
	int32_t ret;
	GPIO_InitTypeDef gpio_init_structure;
	const uint32_t SD_EXTI_LINE[SD_INSTANCES_NBR]   = {SD_DETECT_EXTI_LINE};
	static BSP_EXTI_LineCallback SdCallback[SD_INSTANCES_NBR] = {SD_EXTI_Callback};

	if(Instance> SD_INSTANCES_NBR)
	{
		ret = BSP_ERROR_WRONG_PARAM;
	}
	else
	{
		gpio_init_structure.Pin 	= SD_DET;
		gpio_init_structure.Pull 	= GPIO_PULLUP;
		gpio_init_structure.Speed 	= GPIO_SPEED_FREQ_HIGH;
		gpio_init_structure.Mode 	= GPIO_MODE_IT_RISING_FALLING;
		HAL_GPIO_Init(SD_DET_PORT, &gpio_init_structure);

		// Enable and set SD detect EXTI Interrupt to the lowest priority
		HAL_NVIC_SetPriority((IRQn_Type)(SD_DETECT_EXTI_IRQn), 0x0F, 0x00);
		HAL_NVIC_EnableIRQ((IRQn_Type)(SD_DETECT_EXTI_IRQn));
		HAL_EXTI_GetHandle(&hsd_exti[Instance], SD_EXTI_LINE[Instance]);

		if(HAL_EXTI_RegisterCallback(&hsd_exti[Instance],  HAL_EXTI_COMMON_CB_ID, SdCallback[Instance]) != HAL_OK)
		{
			ret = BSP_ERROR_PERIPH_FAILURE;
		}
		else
		{
			ret = BSP_ERROR_NONE;
		}
	}

	// Return BSP status
	return ret;
}

__weak void BSP_SD_DetectCallback(uint32_t Instance, uint32_t Status)
{
	// Prevent unused argument(s) compilation warning
	UNUSED(Instance);
	UNUSED(Status);
}

int32_t BSP_SD_IsDetected(uint32_t Instance)
{
	int32_t ret = BSP_ERROR_UNKNOWN_FAILURE;

	if(Instance >= SD_INSTANCES_NBR)
	{
		return BSP_ERROR_WRONG_PARAM;
	}
	else
	{
		ret = (uint32_t)HAL_GPIO_ReadPin(SD_DET_PORT, SD_DET);

		// Check SD card detect pin
		if(ret != GPIO_PIN_RESET)
		{
			// Power off
			#ifdef SD_PWR_SWAP_POLARITY
			HAL_GPIO_WritePin(SD_PWR_CNTR_PORT, SD_PWR_CNTR, GPIO_PIN_RESET);
			#else
			HAL_GPIO_WritePin(SD_PWR_CNTR_PORT, SD_PWR_CNTR, GPIO_PIN_SET);
			#endif

			ret = (int32_t)SD_NOT_PRESENT;
		}
		else
		{
			// Power on
			#ifndef SD_PWR_SWAP_POLARITY
			HAL_GPIO_WritePin(SD_PWR_CNTR_PORT, SD_PWR_CNTR, GPIO_PIN_RESET);
			#else
			HAL_GPIO_WritePin(SD_PWR_CNTR_PORT, SD_PWR_CNTR, GPIO_PIN_SET);
			#endif

			ret = (int32_t)SD_PRESENT;
		}
	}

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
		if(HAL_SD_ReadBlocks(&hsd_sdmmc[Instance], (uint8_t *)pData, BlockIdx, BlocksNbr, timeout) != HAL_OK)
		{
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
		if(HAL_SD_ReadBlocks_DMA(&hsd_sdmmc[Instance], (uint8_t *)pData, BlockIdx, BlocksNbr) != HAL_OK)
		{
			ret = BSP_ERROR_PERIPH_FAILURE;
		}
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

#if !defined (USE_HAL_SD_REGISTER_CALLBACKS) || (USE_HAL_SD_REGISTER_CALLBACKS == 0)
void HAL_SD_AbortCallback(SD_HandleTypeDef *hsd)
{
	BSP_SD_AbortCallback((hsd == &hsd_sdmmc[0]) ? 0UL : 1UL);
}

void HAL_SD_TxCpltCallback(SD_HandleTypeDef *hsd)
{
	BSP_SD_WriteCpltCallback((hsd == &hsd_sdmmc[0]) ? 0UL : 1UL);
}

void HAL_SD_RxCpltCallback(SD_HandleTypeDef *hsd)
{
	BSP_SD_ReadCpltCallback((hsd == &hsd_sdmmc[0]) ? 0UL : 1UL);
}

#if (USE_SD_TRANSCEIVER != 0U)
/**
  * @brief  Enable the SD Transceiver 1.8V Mode Callback.
  */
void HAL_SD_DriveTransciver_1_8V_Callback(FlagStatus status)
{
#if (USE_BSP_IO_CLASS > 0U)
  if(status == SET)
  {
    BSP_IO_WritePin(0, SD_LDO_SEL_PIN, IO_PIN_SET);
  }
  else
  {
    BSP_IO_WritePin(0, SD_LDO_SEL_PIN, IO_PIN_RESET);
  }
#endif
}
#endif
#endif /* !defined (USE_HAL_SD_REGISTER_CALLBACKS) || (USE_HAL_SD_REGISTER_CALLBACKS == 0)   */

void BSP_SD_DETECT_IRQHandler(uint32_t Instance)
{
	HAL_EXTI_IRQHandler(&hsd_exti[Instance]);
}

void BSP_SD_IRQHandler(uint32_t Instance)
{
	HAL_SD_IRQHandler(&hsd_sdmmc[Instance]);
}

__weak void BSP_SD_AbortCallback(uint32_t Instance)
{
	/* Prevent unused argument(s) compilation warning */
	UNUSED(Instance);
}

__weak void BSP_SD_WriteCpltCallback(uint32_t Instance)
{
	/* Prevent unused argument(s) compilation warning */
	UNUSED(Instance);
}

__weak void BSP_SD_ReadCpltCallback(uint32_t Instance)
{
	/* Prevent unused argument(s) compilation warning */
	UNUSED(Instance);
}

#if (USE_HAL_SD_REGISTER_CALLBACKS == 1)
/**
  * @brief SD Abort callbacks
  * @param hsd  SD handle
  * @retval None
  */
static void SD_AbortCallback(SD_HandleTypeDef *hsd)
{
  BSP_SD_AbortCallback((hsd == &hsd_sdmmc[0]) ? 0UL : 1UL);
}

/**
  * @brief Tx Transfer completed callbacks
  * @param hsd  SD handle
  * @retval None
  */
static void SD_TxCpltCallback(SD_HandleTypeDef *hsd)
{
  BSP_SD_WriteCpltCallback((hsd == &hsd_sdmmc[0]) ? 0UL : 1UL);
}

/**
  * @brief Rx Transfer completed callbacks
  * @param hsd  SD handle
  * @retval None
  */
static void SD_RxCpltCallback(SD_HandleTypeDef *hsd)
{
  BSP_SD_ReadCpltCallback((hsd == &hsd_sdmmc[0]) ? 0UL : 1UL);
}
#endif

static void SD_EXTI_Callback(void)
{
	uint32_t sd_status = SD_PRESENT;

	BSP_SD_DetectCallback(0,sd_status);
}

static void SD_MspInit(SD_HandleTypeDef *hsd)
{
	GPIO_InitTypeDef gpio_init_structure;

	if(hsd == &hsd_sdmmc[0])
	{
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

		// NVIC configuration for SDIO interrupts
		HAL_NVIC_SetPriority(SDMMC1_IRQn, 14, 0);
		HAL_NVIC_EnableIRQ	(SDMMC1_IRQn);
	}
}

static void SD_MspDeInit(SD_HandleTypeDef *hsd)
{
	GPIO_InitTypeDef gpio_init_structure;

	if(hsd == &hsd_sdmmc[0])
	{
		HAL_NVIC_DisableIRQ(SDMMC1_IRQn);

		/* DeInit GPIO pins can be done in the application
    	(by surcharging this __weak function) */

		/* Disable SDMMC1 clock */
		__HAL_RCC_SDMMC1_CLK_DISABLE();

		/* GPIOJ configuration */
		//gpio_init_structure.Pin       = GPIO_PIN_14;
		//HAL_GPIO_DeInit(GPIOJ, gpio_init_structure.Pin);

		/* GPIOC configuration */
		gpio_init_structure.Pin = SDMMC1_D0 | SDMMC1_D1 | SDMMC1_D2 | SDMMC1_D3 | SDMMC1_CLK;
		HAL_GPIO_DeInit(SDMMC1_SDIO_PORTC, gpio_init_structure.Pin);

		/* GPIOD configuration */
		gpio_init_structure.Pin = SDMMC1_CMD;
		HAL_GPIO_DeInit(SDMMC1_SDIO_PORTD, gpio_init_structure.Pin);
	}
}

#endif
