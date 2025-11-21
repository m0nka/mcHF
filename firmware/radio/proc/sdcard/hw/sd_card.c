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

#include "storage_proc.h"
#include "sd_card.h"

#ifdef CONTEXT_SD

SD_HandleTypeDef hsd_sdmmc[SD_INSTANCES_NBR];
EXTI_HandleTypeDef hsd_exti[SD_INSTANCES_NBR];

static void SD_MspInit(SD_HandleTypeDef *hsd);
static void SD_MspDeInit(SD_HandleTypeDef *hsd);

//void BSP_SD_DETECT_IRQHandler(uint32_t Instance)
//{
//	HAL_EXTI_IRQHandler(&hsd_exti[Instance]);
//}

void BSP_SD_IRQHandler(uint32_t Instance)
{
	HAL_SD_IRQHandler(&hsd_sdmmc[Instance]);
}

void EXTI0_IRQHandler(void)
{
	if(__HAL_GPIO_EXTI_GET_IT(GPIO_PIN_0) != 0x00U)
	{
		//printf("sd irq\r\n");

		Storage_DetectSDCard();

		__HAL_GPIO_EXTI_CLEAR_IT(GPIO_PIN_0);
	}
}

int32_t BSP_SD_Init(uint32_t Instance)
{
	int32_t ret = BSP_ERROR_NONE;

	printf("BSP_SD_Init  \r\n");

	if(Instance >= SD_INSTANCES_NBR)
	{
		printf("BSP_SD_Init1  \r\n");
		ret = BSP_ERROR_WRONG_PARAM;
	}
	else
	{
		if(BSP_SD_IsDetected(Instance) != SD_PRESENT)
		{
			printf("BSP_SD_Init2  \r\n");
			ret = BSP_ERROR_UNKNOWN_COMPONENT;
		}
		else
		{
			// Msp SD initialization
			SD_MspInit(&hsd_sdmmc[Instance]);
			if(ret == BSP_ERROR_NONE)
			{
				//  HAL SD initialization and Enable wide operation
				if(MX_SDMMC1_SD_Init(&hsd_sdmmc[Instance]) != HAL_OK)
				{
					printf("BSP_SD_Init3  \r\n");
					ret = BSP_ERROR_PERIPH_FAILURE;
				}
				else if(HAL_SD_ConfigWideBusOperation(&hsd_sdmmc[Instance], SDMMC_BUS_WIDE_4B) != HAL_OK)
				{
					printf("BSP_SD_Init4  \r\n");
					ret = BSP_ERROR_PERIPH_FAILURE;
				}
				else
				{
					// Switch to High Speed mode if the card support this mode
					(void)HAL_SD_ConfigSpeedBusOperation(&hsd_sdmmc[Instance], SDMMC_SPEED_MODE_HIGH);
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

int32_t BSP_SD_DetectITConfig(uint32_t Instance)
{
	int32_t ret;
	GPIO_InitTypeDef gpio_init_structure;
	//const uint32_t SD_EXTI_LINE[SD_INSTANCES_NBR]   = {SD_DETECT_EXTI_LINE};
	//static BSP_EXTI_LineCallback SdCallback[SD_INSTANCES_NBR] = {SD_EXTI_Callback};

	printf("set irq  \r\n");

	if(Instance> SD_INSTANCES_NBR)
	{
		ret = BSP_ERROR_WRONG_PARAM;
	}
	else
	{

		printf("set irq  \r\n");

		gpio_init_structure.Pin 	= SD_DET;
		gpio_init_structure.Pull 	= GPIO_PULLUP;
		gpio_init_structure.Speed 	= GPIO_SPEED_FREQ_HIGH;
		gpio_init_structure.Mode 	= GPIO_MODE_IT_RISING_FALLING;
		HAL_GPIO_Init(SD_DET_PORT, &gpio_init_structure);

#if 0
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
#endif

		HAL_EXTI_GetHandle(&hsd_exti[Instance], EXTI_LINE_0);

		HAL_NVIC_SetPriority(EXTI0_IRQn, 15U, 0x00);
		HAL_NVIC_EnableIRQ  (EXTI0_IRQn);

		//HAL_EXTI_D1_EventInputConfig(EXTI_LINE0 , EXTI_MODE_IT,  ENABLE);
	}

	// Return BSP status
	return ret;
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
			#if 1
			printf("power off  \r\n");

			// Power off
			#ifdef SD_PWR_SWAP_POLARITY
			HAL_GPIO_WritePin(SD_PWR_CNTR_PORT, SD_PWR_CNTR, GPIO_PIN_RESET);
			#else
			HAL_GPIO_WritePin(SD_PWR_CNTR_PORT, SD_PWR_CNTR, GPIO_PIN_SET);
			#endif
			#endif

			ret = (int32_t)SD_NOT_PRESENT;
		}
		else
		{
			printf("power on  \r\n");

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
		// Power off
		#ifdef SD_PWR_SWAP_POLARITY
		HAL_GPIO_WritePin(SD_PWR_CNTR_PORT, SD_PWR_CNTR, GPIO_PIN_RESET);
		#else
		HAL_GPIO_WritePin(SD_PWR_CNTR_PORT, SD_PWR_CNTR, GPIO_PIN_SET);
		#endif

		HAL_NVIC_DisableIRQ(SDMMC1_IRQn);

		/* DeInit GPIO pins can be done in the application
    	(by surcharging this __weak function) */

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
