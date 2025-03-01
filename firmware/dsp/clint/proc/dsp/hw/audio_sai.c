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
#include "mchf_board.h"

#include <stdio.h>
#include <string.h>

#include "audio_sai.h"

#include "stm32h7xx_hal.h"

#include "dsp_proc.h"

#if (DATA_SIZE == SAI_PROTOCOL_DATASIZE_16BIT)
ALIGN_32BYTES (uint16_t TxBuff[DMA_BUFF_SIZE]);
ALIGN_32BYTES (uint16_t RxBuff[DMA_BUFF_SIZE]);
#else
ALIGN_32BYTES (uint32_t TxBuff[DMA_BUFF_SIZE]);
ALIGN_32BYTES (uint32_t RxBuff[DMA_BUFF_SIZE]);
#endif

extern __IO TransceiverState 	ts;

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

void HAL_SAI_RxHalfCpltCallback(SAI_HandleTypeDef *hsai)
{
	#if 0
	for(int i = 0; i < DMA_BUFF_SIZE/2; i++)
		TxBuff[i] = RxBuff[i];
	#else
	I2S_RX_CallBack(RxBuff + 0, TxBuff + 0, DMA_BUFF_SIZE/2, 1);
	#endif
}

void HAL_SAI_RxCpltCallback(SAI_HandleTypeDef *hsai)
{
	#if 0
	for(int i = 0; i < DMA_BUFF_SIZE/2; i++)
		TxBuff[i + DMA_BUFF_SIZE/2] = RxBuff[i + DMA_BUFF_SIZE/2];
	#else
    I2S_RX_CallBack(RxBuff + DMA_BUFF_SIZE/2, TxBuff + DMA_BUFF_SIZE/2, DMA_BUFF_SIZE/2, 0);
	#endif
}

static void SAI_MspInit(SAI_HandleTypeDef *hsai)
{
	GPIO_InitTypeDef  gpio_init_structure;
	static DMA_HandleTypeDef hdma_sai_tx, hdma_sai_rx;

    /* Enable SAI clock */
    AUDIO_OUT_SAIx_CLK_ENABLE();

  	gpio_init_structure.Mode = GPIO_MODE_AF_PP;
    gpio_init_structure.Pull = GPIO_NOPULL;
    gpio_init_structure.Speed = GPIO_SPEED_FREQ_VERY_HIGH;

    if(hsai->Instance == AUDIO_OUT_SAIx)
    {
    	/* Enable SAI clock */
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

		#if(DATA_SIZE == SAI_PROTOCOL_DATASIZE_16BIT)
    	hdma_sai_tx.Init.PeriphDataAlignment = DMA_PDATAALIGN_HALFWORD;
    	hdma_sai_tx.Init.MemDataAlignment    = DMA_MDATAALIGN_HALFWORD;
		#else
   		hdma_sai_tx.Init.PeriphDataAlignment = DMA_PDATAALIGN_WORD;
   		hdma_sai_tx.Init.MemDataAlignment    = DMA_MDATAALIGN_WORD;
		#endif

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

    	/* Associate the DMA handle */
    	__HAL_LINKDMA(hsai, hdmatx, hdma_sai_tx);

    	/* Deinitialize the Stream for new transfer */
    	(void)HAL_DMA_DeInit(&hdma_sai_tx);

    	/* Configure the DMA Stream */
    	(void)HAL_DMA_Init(&hdma_sai_tx);

    	/* SAI DMA IRQ Channel configuration */
    	HAL_NVIC_SetPriority(AUDIO_OUT_SAIx_DMAx_IRQ, BSP_AUDIO_OUT_IT_PRIORITY, 0);
    	HAL_NVIC_EnableIRQ(AUDIO_OUT_SAIx_DMAx_IRQ);
    }

    /* Audio In Msp initialization */
    if(hsai->Instance == AUDIO_IN_SAIx)
    {
    	/* Enable SAI clock */
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

		#if(DATA_SIZE == SAI_PROTOCOL_DATASIZE_16BIT)
    	hdma_sai_rx.Init.PeriphDataAlignment = DMA_PDATAALIGN_HALFWORD;
    	hdma_sai_rx.Init.MemDataAlignment    = DMA_MDATAALIGN_HALFWORD;
		#else
    	hdma_sai_rx.Init.PeriphDataAlignment = DMA_PDATAALIGN_WORD;
    	hdma_sai_rx.Init.MemDataAlignment    = DMA_MDATAALIGN_WORD;
		#endif

    	/* Configure the hdma_sai_rx handle parameters */
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

    	/* Associate the DMA handle */
    	__HAL_LINKDMA(hsai, hdmarx, hdma_sai_rx);

    	/* Deinitialize the Stream for new transfer */
    	HAL_DMA_DeInit(&hdma_sai_rx);

    	/* Configure the DMA Stream */
    	HAL_DMA_Init(&hdma_sai_rx);

    	/* SAI DMA IRQ Channel configuration */
    	HAL_NVIC_SetPriority(AUDIO_IN_SAIx_DMAx_IRQ, BSP_AUDIO_IN_IT_PRIORITY, 0);
    	HAL_NVIC_EnableIRQ(AUDIO_IN_SAIx_DMAx_IRQ);
    }
}

static void SAI_MspDeInit(SAI_HandleTypeDef *hsai)
{
  GPIO_InitTypeDef  gpio_init_structure;
  if(hsai->Instance == AUDIO_OUT_SAIx)
  {
    /* SAI DMA IRQ Channel deactivation */
    HAL_NVIC_DisableIRQ(AUDIO_OUT_SAIx_DMAx_IRQ);

    /* Deinitialize the DMA stream */
    (void)HAL_DMA_DeInit(hsai->hdmatx);

    /* Disable SAI peripheral */
    __HAL_SAI_DISABLE(hsai);

    // ToDo: De-init correct pins
	#if 0
    /* Deactivates CODEC_SAI pins FS, SCK, MCK and SD by putting them in input mode */
    gpio_init_structure.Pin = AUDIO_OUT_SAIx_FS_PIN;
    HAL_GPIO_DeInit(AUDIO_OUT_SAIx_FS_GPIO_PORT, gpio_init_structure.Pin);

    gpio_init_structure.Pin = AUDIO_OUT_SAIx_SCK_PIN;
    HAL_GPIO_DeInit(AUDIO_OUT_SAIx_SCK_GPIO_PORT, gpio_init_structure.Pin);

    gpio_init_structure.Pin =  AUDIO_OUT_SAIx_SD_PIN;
    HAL_GPIO_DeInit(AUDIO_OUT_SAIx_SD_GPIO_PORT, gpio_init_structure.Pin);

    //gpio_init_structure.Pin = AUDIO_OUT_SAIx_MCLK_PIN;
    //HAL_GPIO_DeInit(AUDIO_OUT_SAIx_MCLK_GPIO_PORT, gpio_init_structure.Pin);
	#endif

    /* Disable SAI clock */
    AUDIO_OUT_SAIx_CLK_DISABLE();
  }
  if(hsai->Instance == AUDIO_IN_SAIx)
  {
    /* SAI DMA IRQ Channel deactivation */
    HAL_NVIC_DisableIRQ(AUDIO_IN_SAIx_DMAx_IRQ);

    /* Deinitialize the DMA stream */
    (void)HAL_DMA_DeInit(hsai->hdmarx);

    /* Disable SAI peripheral */
    __HAL_SAI_DISABLE(hsai);

    // ToDo: De-init correct pins
	#if 0
    /* Deactivates CODEC_SAI pin SD by putting them in input mode */
    gpio_init_structure.Pin = AUDIO_IN_SAIx_SD_PIN;
    HAL_GPIO_DeInit(AUDIO_IN_SAIx_SD_GPIO_PORT, gpio_init_structure.Pin);
	#endif

    /* Disable SAI clock */
    AUDIO_IN_SAIx_CLK_DISABLE();
  }
}

HAL_StatusTypeDef MX_SAI1_ClockConfig(SAI_HandleTypeDef *hsai, uint32_t SampleRate)
{
	/* Prevent unused argument(s) compilation warning */
	UNUSED(hsai);

	HAL_StatusTypeDef ret = HAL_OK;
	RCC_PeriphCLKInitTypeDef rcc_ex_clk_init_struct;

	HAL_RCCEx_GetPeriphCLKConfig(&rcc_ex_clk_init_struct);

	/* Set the PLL configuration according to the audio frequency */
	if((SampleRate == AUDIO_FREQUENCY_11K) || (SampleRate == AUDIO_FREQUENCY_22K) || (SampleRate == AUDIO_FREQUENCY_44K))
	{
		rcc_ex_clk_init_struct.PLL2.PLL2P = 38;
		rcc_ex_clk_init_struct.PLL2.PLL2N = 429;
	}
	else /* AUDIO_FREQUENCY_8K, AUDIO_FREQUENCY_16K, AUDIO_FREQUENCY_32K, AUDIO_FREQUENCY_48K, AUDIO_FREQUENCY_96K */
	{
		rcc_ex_clk_init_struct.PLL2.PLL2P = 7;
		rcc_ex_clk_init_struct.PLL2.PLL2N = 344;
	}

	rcc_ex_clk_init_struct.PeriphClockSelection = RCC_PERIPHCLK_SAI1;
	rcc_ex_clk_init_struct.Sai1ClockSelection = RCC_SAI1CLKSOURCE_PLL2;
	rcc_ex_clk_init_struct.PLL2.PLL2Q = 1;
	rcc_ex_clk_init_struct.PLL2.PLL2R = 1;
	rcc_ex_clk_init_struct.PLL2.PLL2M = 25;

	if(HAL_RCCEx_PeriphCLKConfig(&rcc_ex_clk_init_struct) != HAL_OK)
	{
		ret = HAL_ERROR;
	}

	return ret;
}

//*----------------------------------------------------------------------------
//* Function Name       : MX_SAI1_Block_A_Init
//* Object              :
//* Notes    			: SAI BLOCK A - CPU RX
//* Notes   			:
//* Notes    			:
//* Context    			: CONTEXT_AUDIO
//*----------------------------------------------------------------------------
HAL_StatusTypeDef MX_SAI1_Block_A_Init(SAI_HandleTypeDef* hsai)
{
	HAL_StatusTypeDef ret = HAL_OK;

	__HAL_SAI_DISABLE(hsai);

	// Configure SAI1_Block
	hsai->Init.AudioFrequency         = ts.samp_rate;
	hsai->Init.MonoStereoMode         = SAI_STEREOMODE;
	hsai->Init.AudioMode              = SAI_MODEMASTER_RX;
	hsai->Init.NoDivider              = SAI_MASTERDIVIDER_ENABLE;

	hsai->Init.Synchro                = SAI_ASYNCHRONOUS;
	hsai->Init.SynchroExt             = SAI_SYNCEXT_DISABLE;

	hsai->Init.OutputDrive            = SAI_OUTPUTDRIVE_DISABLE;
	hsai->Init.FIFOThreshold          = SAI_FIFOTHRESHOLD_1QF;
	hsai->Init.CompandingMode         = SAI_NOCOMPANDING;
	hsai->Init.TriState               = SAI_OUTPUT_RELEASED;

	if(HAL_SAI_InitProtocol(hsai, SAI_I2S_STANDARD, DATA_SIZE, 2) != HAL_OK)
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
//* Function Name       : MX_SAI1_Block_B_Init
//* Object              :
//* Notes    			: SAI BLOCK B - CPU TX
//* Notes   			:
//* Notes    			:
//* Context    			: CONTEXT_AUDIO
//*----------------------------------------------------------------------------
HAL_StatusTypeDef MX_SAI1_Block_B_Init(SAI_HandleTypeDef* hsai)
{
	HAL_StatusTypeDef ret = HAL_OK;

	__HAL_SAI_DISABLE(hsai);

	// Configure SAI1_Block
	hsai->Init.AudioFrequency       = ts.samp_rate;
	hsai->Init.MonoStereoMode       = SAI_STEREOMODE;
	hsai->Init.AudioMode            = SAI_MODEMASTER_TX;
	hsai->Init.NoDivider            = SAI_MASTERDIVIDER_ENABLE;

	hsai->Init.Synchro              = SAI_ASYNCHRONOUS;
	hsai->Init.OutputDrive          = SAI_OUTPUTDRIVE_ENABLE;
	hsai->Init.FIFOThreshold        = SAI_FIFOTHRESHOLD_1QF;
	hsai->Init.SynchroExt           = SAI_SYNCEXT_DISABLE;
	hsai->Init.CompandingMode       = SAI_NOCOMPANDING;
	hsai->Init.TriState             = SAI_OUTPUT_NOTRELEASED;

	if(HAL_SAI_InitProtocol(hsai, SAI_I2S_STANDARD, DATA_SIZE, 2) != HAL_OK)
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

static int32_t BSP_AUDIO_INOUT_Init(void)
{
	// PLL clock
	if(MX_SAI1_ClockConfig(NULL, ts.samp_rate) != HAL_OK)
		return BSP_ERROR_CLOCK_FAILURE;

	haudio_in_sai.Instance = AUDIO_IN_SAIx;
	SAI_MspInit(&haudio_in_sai);

	haudio_out_sai.Instance = AUDIO_OUT_SAIx;
	SAI_MspInit(&haudio_out_sai);

	if(MX_SAI1_Block_A_Init(&haudio_in_sai) != HAL_OK)
		return BSP_ERROR_PERIPH_FAILURE;

	if(MX_SAI1_Block_B_Init(&haudio_out_sai) != HAL_OK)
		return BSP_ERROR_PERIPH_FAILURE;

	return BSP_ERROR_NONE;
}

//*----------------------------------------------------------------------------
//* Function Name       : audio_sai_hw_init
//* Object              :
//* Notes    			:
//* Notes   			:
//* Notes    			:
//* Context    			: CONTEXT_AUDIO
//*----------------------------------------------------------------------------
uchar audio_sai_hw_init(void)
{
	//printf("sai init...\r\n");

	if(BSP_AUDIO_INOUT_Init() != BSP_ERROR_NONE)
	{
		printf("err sai init\r\n");
		return 1;
	}

	if(HAL_SAI_Receive_DMA(&haudio_in_sai, (uint8_t *)RxBuff, DMA_BUFF_SIZE) != HAL_OK)
	{
		printf("error rx dma %d\r\n", haudio_in_sai.ErrorCode);
		return 2;
	}

	if(HAL_SAI_Transmit_DMA(&haudio_out_sai, (uint8_t *)TxBuff, DMA_BUFF_SIZE) != HAL_OK)
	{
		printf("error tx dma %d\r\n", haudio_out_sai.ErrorCode);
		return 3;
	}

	//printf("sai started\r\n");
	return 0;
}

#if 0
//*----------------------------------------------------------------------------
//* Function Name       : audio_sai_get_buffer
//* Object              :
//* Notes    			: temp here!!!!
//* Notes   			:
//* Notes    			:
//* Context    			: CONTEXT_AUDIO
//*----------------------------------------------------------------------------
void audio_sai_get_buffer(uchar *buffer)
{
	memcpy(buffer, RxBuff, DMA_BUFF_SIZE);
}
#endif


