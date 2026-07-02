/************************************************************************************
**                                                                                 **
**                                 mcHF QRP Transceiver                            **
**                         Krassi Atanassov - M0NKA, 2013-2026                     **
**                                                                                 **
**---------------------------------------------------------------------------------**
**                                                                                 **
**  File name:		uhsdr_hw_sai_m4.c                                              **
**  Description:	SAI1 audio streaming hw driver for the H747 CM4 core,          **
**					direct port of the CLINT baseband audio_sai driver.            **
**					SAI1 Block A = CPU RX (master), Block B = CPU TX,              **
**					16 bit I2S protocol, circular DMA on DMA2 streams 1/4          **
**  Licence:		https://github.com/m0nka/mcHF/blob/main/LICENSE                **
************************************************************************************/

// Compiled only for the STM32H747 CM4 baseband build
#ifdef H7_M4_CORE

#include <stdio.h>

#include "uhsdr_hw_sai_m4.h"

#define SAI_M4_DATA_SIZE		SAI_PROTOCOL_DATASIZE_16BIT

// SAI audio frequency helpers (subset used here)
#define SAI_M4_FREQUENCY_48K	48000U
#define SAI_M4_FREQUENCY_96K	96000U

SAI_HandleTypeDef	haudio_out_sai;
SAI_HandleTypeDef	haudio_in_sai;

void AUDIO_OUT_SAIx_DMAx_IRQHandler(void)
{
	HAL_DMA_IRQHandler(haudio_out_sai.hdmatx);
}

void AUDIO_IN_SAIx_DMAx_IRQHandler(void)
{
	HAL_DMA_IRQHandler(haudio_in_sai.hdmarx);
}

static void sai_msp_init(SAI_HandleTypeDef *hsai)
{
	GPIO_InitTypeDef  gpio_init_structure;
	static DMA_HandleTypeDef hdma_sai_tx, hdma_sai_rx;

	// Enable SAI clock
	AUDIO_OUT_SAIx_CLK_ENABLE();

	gpio_init_structure.Mode = GPIO_MODE_AF_PP;
	gpio_init_structure.Pull = GPIO_NOPULL;
	gpio_init_structure.Speed = GPIO_SPEED_FREQ_VERY_HIGH;

	if(hsai->Instance == AUDIO_OUT_SAIx)
	{
		__HAL_RCC_SAI1_CLK_ENABLE();

		gpio_init_structure.Pin = AUDIO_OUT_SAIx_FS_PIN;
		gpio_init_structure.Alternate = AUDIO_OUT_SAIx_FS_AF;
		HAL_GPIO_Init(AUDIO_OUT_SAIx_FS_GPIO_PORT, &gpio_init_structure);

		gpio_init_structure.Pin = AUDIO_OUT_SAIx_SCK_PIN;
		gpio_init_structure.Alternate = AUDIO_OUT_SAIx_SCK_AF;
		HAL_GPIO_Init(AUDIO_OUT_SAIx_SCK_GPIO_PORT, &gpio_init_structure);

		gpio_init_structure.Pin =  AUDIO_OUT_SAIx_SD_PIN;
		gpio_init_structure.Alternate = AUDIO_OUT_SAIx_SD_AF;
		HAL_GPIO_Init(AUDIO_OUT_SAIx_SD_GPIO_PORT, &gpio_init_structure);

		AUDIO_OUT_SAIx_DMAx_CLK_ENABLE();

		hdma_sai_tx.Init.PeriphDataAlignment = DMA_PDATAALIGN_HALFWORD;
		hdma_sai_tx.Init.MemDataAlignment    = DMA_MDATAALIGN_HALFWORD;

		hdma_sai_tx.Init.Request             = AUDIO_OUT_SAIx_DMAx_REQUEST;
		hdma_sai_tx.Init.PeriphInc           = DMA_PINC_DISABLE;
		hdma_sai_tx.Init.MemInc              = DMA_MINC_ENABLE;
		hdma_sai_tx.Init.Mode                = DMA_CIRCULAR;
		hdma_sai_tx.Init.Priority            = DMA_PRIORITY_HIGH;
		hdma_sai_tx.Init.FIFOMode            = DMA_FIFOMODE_ENABLE;
		hdma_sai_tx.Init.FIFOThreshold       = DMA_FIFO_THRESHOLD_FULL;
		hdma_sai_tx.Init.Direction           = DMA_MEMORY_TO_PERIPH;
		hdma_sai_tx.Instance                 = AUDIO_OUT_SAIx_DMAx_STREAM;
		hdma_sai_tx.Init.MemBurst            = DMA_MBURST_SINGLE;
		hdma_sai_tx.Init.PeriphBurst         = DMA_PBURST_SINGLE;

		// Associate the DMA handle
		__HAL_LINKDMA(hsai, hdmatx, hdma_sai_tx);

		// Deinitialize the Stream for new transfer
		(void)HAL_DMA_DeInit(&hdma_sai_tx);

		// Configure the DMA Stream
		(void)HAL_DMA_Init(&hdma_sai_tx);

		// SAI DMA IRQ Channel configuration
		HAL_NVIC_SetPriority(AUDIO_OUT_SAIx_DMAx_IRQ, 5, 0);
		HAL_NVIC_EnableIRQ(AUDIO_OUT_SAIx_DMAx_IRQ);
	}

	// Audio In Msp initialization
	if(hsai->Instance == AUDIO_IN_SAIx)
	{
		AUDIO_IN_SAIx_CLK_ENABLE();

		gpio_init_structure.Pin = AUDIO_IN_SAIx_FS_PIN;
		gpio_init_structure.Alternate = AUDIO_IN_SAIx_FS_AF;
		HAL_GPIO_Init(AUDIO_IN_SAIx_FS_GPIO_PORT, &gpio_init_structure);

		gpio_init_structure.Pin = AUDIO_IN_SAIx_SCK_PIN;
		gpio_init_structure.Alternate = AUDIO_IN_SAIx_SCK_AF;
		HAL_GPIO_Init(AUDIO_IN_SAIx_SCK_GPIO_PORT, &gpio_init_structure);

		gpio_init_structure.Pin =  AUDIO_IN_SAIx_SD_PIN;
		gpio_init_structure.Alternate = AUDIO_IN_SAIx_SD_AF;
		HAL_GPIO_Init(AUDIO_IN_SAIx_SD_GPIO_PORT, &gpio_init_structure);

		gpio_init_structure.Pin = AUDIO_IN_SAIx_MCLK_PIN;
		gpio_init_structure.Alternate = AUDIO_IN_SAIx_MCLK_AF;
		HAL_GPIO_Init(AUDIO_IN_SAIx_MCLK_GPIO_PORT, &gpio_init_structure);

		AUDIO_IN_SAIx_DMAx_CLK_ENABLE();

		hdma_sai_rx.Init.PeriphDataAlignment = DMA_PDATAALIGN_HALFWORD;
		hdma_sai_rx.Init.MemDataAlignment    = DMA_MDATAALIGN_HALFWORD;

		hdma_sai_rx.Init.Request             = AUDIO_IN_SAIx_DMAx_REQUEST;
		hdma_sai_rx.Init.Direction           = DMA_PERIPH_TO_MEMORY;
		hdma_sai_rx.Init.PeriphInc           = DMA_PINC_DISABLE;
		hdma_sai_rx.Init.MemInc              = DMA_MINC_ENABLE;
		hdma_sai_rx.Init.Mode                = DMA_CIRCULAR;
		hdma_sai_rx.Init.Priority            = DMA_PRIORITY_HIGH;
		hdma_sai_rx.Init.FIFOMode            = DMA_FIFOMODE_DISABLE;
		hdma_sai_rx.Init.FIFOThreshold       = DMA_FIFO_THRESHOLD_FULL;
		hdma_sai_rx.Init.MemBurst            = DMA_MBURST_SINGLE;
		hdma_sai_rx.Init.PeriphBurst         = DMA_MBURST_SINGLE;

		hdma_sai_rx.Instance = AUDIO_IN_SAIx_DMAx_STREAM;

		// Associate the DMA handle
		__HAL_LINKDMA(hsai, hdmarx, hdma_sai_rx);

		// Deinitialize the Stream for new transfer
		HAL_DMA_DeInit(&hdma_sai_rx);

		// Configure the DMA Stream
		HAL_DMA_Init(&hdma_sai_rx);

		// SAI DMA IRQ Channel configuration
		HAL_NVIC_SetPriority(AUDIO_IN_SAIx_DMAx_IRQ, 5, 0);
		HAL_NVIC_EnableIRQ(AUDIO_IN_SAIx_DMAx_IRQ);
	}
}

static HAL_StatusTypeDef sai_clock_config(uint32_t sample_rate)
{
	HAL_StatusTypeDef ret = HAL_OK;
	RCC_PeriphCLKInitTypeDef rcc_ex_clk_init_struct;

	HAL_RCCEx_GetPeriphCLKConfig(&rcc_ex_clk_init_struct);

	// Set the PLL configuration according to the audio frequency,
	// 8/16/32/48/96 kHz family
	rcc_ex_clk_init_struct.PLL2.PLL2P = 7;
	rcc_ex_clk_init_struct.PLL2.PLL2N = 344;

	rcc_ex_clk_init_struct.PeriphClockSelection = RCC_PERIPHCLK_SAI1;
	rcc_ex_clk_init_struct.Sai1ClockSelection = RCC_SAI1CLKSOURCE_PLL2;
	rcc_ex_clk_init_struct.PLL2.PLL2Q = 1;
	rcc_ex_clk_init_struct.PLL2.PLL2R = 1;
	rcc_ex_clk_init_struct.PLL2.PLL2M = 25;

	if(HAL_RCCEx_PeriphCLKConfig(&rcc_ex_clk_init_struct) != HAL_OK)
	{
		ret = HAL_ERROR;
	}

	(void)sample_rate;

	return ret;
}

//*----------------------------------------------------------------------------
//* Function Name       : sai_block_a_init
//* Object              : SAI BLOCK A - CPU RX (master)
//* Context    			: CONTEXT_AUDIO
//*----------------------------------------------------------------------------
static HAL_StatusTypeDef sai_block_a_init(SAI_HandleTypeDef* hsai, uint32_t sample_rate)
{
	HAL_StatusTypeDef ret = HAL_OK;

	__HAL_SAI_DISABLE(hsai);

	hsai->Init.AudioFrequency         = sample_rate;
	hsai->Init.MonoStereoMode         = SAI_STEREOMODE;
	hsai->Init.AudioMode              = SAI_MODEMASTER_RX;
	hsai->Init.NoDivider              = SAI_MASTERDIVIDER_ENABLE;

	hsai->Init.Synchro                = SAI_ASYNCHRONOUS;
	hsai->Init.SynchroExt             = SAI_SYNCEXT_DISABLE;

	hsai->Init.OutputDrive            = SAI_OUTPUTDRIVE_DISABLE;
	hsai->Init.FIFOThreshold          = SAI_FIFOTHRESHOLD_1QF;
	hsai->Init.CompandingMode         = SAI_NOCOMPANDING;
	hsai->Init.TriState               = SAI_OUTPUT_RELEASED;

	if(HAL_SAI_InitProtocol(hsai, SAI_I2S_STANDARD, SAI_M4_DATA_SIZE, 2) != HAL_OK)
	{
		printf("sai A protocol err\r\n");
		ret = HAL_ERROR;
	}

	if(HAL_SAI_Init(hsai) != HAL_OK)
	{
		printf("sai A init err\r\n");
		ret = HAL_ERROR;
	}

	__HAL_SAI_ENABLE(hsai);

	return ret;
}

//*----------------------------------------------------------------------------
//* Function Name       : sai_block_b_init
//* Object              : SAI BLOCK B - CPU TX
//* Context    			: CONTEXT_AUDIO
//*----------------------------------------------------------------------------
static HAL_StatusTypeDef sai_block_b_init(SAI_HandleTypeDef* hsai, uint32_t sample_rate)
{
	HAL_StatusTypeDef ret = HAL_OK;

	__HAL_SAI_DISABLE(hsai);

	hsai->Init.AudioFrequency       = sample_rate;
	hsai->Init.MonoStereoMode       = SAI_STEREOMODE;
	hsai->Init.AudioMode            = SAI_MODEMASTER_TX;
	hsai->Init.NoDivider            = SAI_MASTERDIVIDER_ENABLE;

	hsai->Init.Synchro              = SAI_ASYNCHRONOUS;
	hsai->Init.OutputDrive          = SAI_OUTPUTDRIVE_ENABLE;
	hsai->Init.FIFOThreshold        = SAI_FIFOTHRESHOLD_1QF;
	hsai->Init.SynchroExt           = SAI_SYNCEXT_DISABLE;
	hsai->Init.CompandingMode       = SAI_NOCOMPANDING;
	hsai->Init.TriState             = SAI_OUTPUT_NOTRELEASED;

	if(HAL_SAI_InitProtocol(hsai, SAI_I2S_STANDARD, SAI_M4_DATA_SIZE, 2) != HAL_OK)
	{
		printf("sai B protocol err\r\n");
		ret = HAL_ERROR;
	}

	if(HAL_SAI_Init(hsai) != HAL_OK)
	{
		printf("sai B init err\r\n");
		ret = HAL_ERROR;
	}

	__HAL_SAI_ENABLE(hsai);

	return ret;
}

//*----------------------------------------------------------------------------
//* Function Name       : uhsdr_sai_m4_start
//* Object              : init SAI1 + DMA and start circular streaming
//* Context    			: CONTEXT_AUDIO
//*----------------------------------------------------------------------------
uint8_t uhsdr_sai_m4_start(int16_t *rx_buf, int16_t *tx_buf, uint32_t buf_len_halfwords)
{
	uint32_t sample_rate = SAI_M4_FREQUENCY_48K;

	// PLL clock
	if(sai_clock_config(sample_rate) != HAL_OK)
	{
		printf("err sai clock\r\n");
		return 1;
	}

	haudio_in_sai.Instance = AUDIO_IN_SAIx;
	sai_msp_init(&haudio_in_sai);

	haudio_out_sai.Instance = AUDIO_OUT_SAIx;
	sai_msp_init(&haudio_out_sai);

	if(sai_block_a_init(&haudio_in_sai, sample_rate) != HAL_OK)
		return 2;

	if(sai_block_b_init(&haudio_out_sai, sample_rate) != HAL_OK)
		return 3;

	if(HAL_SAI_Receive_DMA(&haudio_in_sai, (uint8_t *)rx_buf, buf_len_halfwords) != HAL_OK)
	{
		printf("error rx dma %d\r\n", (int)haudio_in_sai.ErrorCode);
		return 4;
	}

	if(HAL_SAI_Transmit_DMA(&haudio_out_sai, (uint8_t *)tx_buf, buf_len_halfwords) != HAL_OK)
	{
		printf("error tx dma %d\r\n", (int)haudio_out_sai.ErrorCode);
		return 5;
	}

	return 0;
}

//*----------------------------------------------------------------------------
//* Function Name       : uhsdr_sai_m4_stop
//* Object              : stop SAI streaming
//* Context    			: CONTEXT_AUDIO
//*----------------------------------------------------------------------------
void uhsdr_sai_m4_stop(void)
{
	HAL_SAI_DMAStop(&haudio_in_sai);
	HAL_SAI_DMAStop(&haudio_out_sai);
}

#endif // H7_M4_CORE
