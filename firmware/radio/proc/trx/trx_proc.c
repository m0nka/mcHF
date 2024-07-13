/************************************************************************************
**                                                                                 **
**                             mcHF Pro QRP Transceiver                            **
**                         Krassi Atanassov - M0NKA, 2013-2021                     **
**                                                                                 **
**---------------------------------------------------------------------------------**
**                                                                                 **
**  File name:                                                                     **
**  Description:                                                                   **
**  Last Modified:                                                                 **
**  Licence:       The mcHF project is released for radio amateurs experimentation **
**               and non-commercial use only.Check 3rd party drivers for licensing **
************************************************************************************/
#include "mchf_pro_board.h"
#include "main.h"

#ifdef CONTEXT_TRX

#include "trx_proc.h"

// Public radio state
extern struct	TRANSCEIVER_STATE_UI	tsu;

#define DACx                            DAC1
//#define DACx_CHANNEL_GPIO_CLK_ENABLE()  __HAL_RCC_GPIOA_CLK_ENABLE()

#define DACx_CLK_ENABLE()               __HAL_RCC_DAC12_CLK_ENABLE()
#define DACx_FORCE_RESET()              __HAL_RCC_DAC12_FORCE_RESET()
#define DACx_RELEASE_RESET()            __HAL_RCC_DAC12_RELEASE_RESET()

/* Definition for DACx Channel Pin */
//#define DACx_CHANNEL_PIN                GPIO_PIN_4
//#define DACx_CHANNEL_GPIO_PORT          GPIOA

/* Definition for DACx's Channel */
//#define DACx_CHANNEL                    DAC_CHANNEL_1

DAC_HandleTypeDef    DacHandle;
static DAC_ChannelConfTypeDef sConfig;


void HAL_DAC_MspInit(DAC_HandleTypeDef *hdac)
{
  GPIO_InitTypeDef          GPIO_InitStruct;

  //printf("msp dac init\r\n");

  /*##-1- Enable peripherals and GPIO Clocks #################################*/
  /* Enable GPIO clock ****************************************/
  //DACx_CHANNEL_GPIO_CLK_ENABLE();

  /* DAC Periph clock enable */
  DACx_CLK_ENABLE();

  /*##-2- Configure peripheral GPIO ##########################################*/
  /* DAC Channel1 GPIO pin configuration */
  GPIO_InitStruct.Pin  = DAC1_OUT1|DAC1_OUT2;
  GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(DAC1_OUTX_PORT, &GPIO_InitStruct);
}

//#define ENABLE_TX_PA

/* Definition for DACx's Channel */
static void dacs_init(void)
{
	//printf("dac init\r\n");

	// ---------------------------------------------------
	// 5V ON for VREF chip
	//HAL_GPIO_WritePin(GPIOG, GPIO_PIN_10, GPIO_PIN_RESET);

	#ifdef ENABLE_TX_PA
	// 8V ON for PA pre-amp
	HAL_GPIO_WritePin(GPIOG, GPIO_PIN_9, GPIO_PIN_RESET);
	#endif

	DacHandle.Instance = DACx;

	/*##-0- DeInit the DAC peripheral ##########################################*/
	if (HAL_DAC_DeInit(&DacHandle) != HAL_OK)
	  {
	    /* DeInitialization Error */
	    //Error_Handler();
		printf("dac err 1\r\n");
	  }

	  /*##-1- Configure the DAC peripheral #######################################*/

	  if (HAL_DAC_Init(&DacHandle) != HAL_OK)
	  {
	    /* Initialization Error */
	    //Error_Handler();
		  printf("dac err 2\r\n");
	  }

	  /*##-2- Configure DAC channel1 #############################################*/
	  sConfig.DAC_Trigger = DAC_TRIGGER_NONE;
	  sConfig.DAC_OutputBuffer = DAC_OUTPUTBUFFER_ENABLE;

	  if (HAL_DAC_ConfigChannel(&DacHandle, &sConfig, DAC_CHANNEL_1) != HAL_OK)
	  {
	    /* Channel configuration Error */
	    //Error_Handler();

		  printf("dac err 3\r\n");
	  }

	  if (HAL_DAC_ConfigChannel(&DacHandle, &sConfig, DAC_CHANNEL_2) != HAL_OK)
	  {
	    /* Channel configuration Error */
	    //Error_Handler();

		  printf("dac err 3\r\n");
	  }

	#ifdef ENABLE_TX_PA
	  /*##-3- Set DAC Channel1 DHR register ######################################*/
	  if (HAL_DAC_SetValue(&DacHandle, DAC_CHANNEL_1, DAC_ALIGN_12B_R, 3500) != HAL_OK)
	  {
	    /* Setting value Error */
	    //Error_Handler();

		  printf("dac err 4\r\n");
	  }
	  if (HAL_DAC_SetValue(&DacHandle, DAC_CHANNEL_2, DAC_ALIGN_12B_R, 3500) != HAL_OK)
	  {
	    /* Setting value Error */
	    //Error_Handler();

		  printf("dac err 4\r\n");
	  }

	  /*##-4- Enable DAC Channel1 ################################################*/
	  if (HAL_DAC_Start(&DacHandle, DAC_CHANNEL_1) != HAL_OK)
	  {
	    /* Start Error */
	    //Error_Handler();

		  printf("dac err 5\r\n");
	  }
	  if (HAL_DAC_Start(&DacHandle, DAC_CHANNEL_2) != HAL_OK)
	  {
	    /* Start Error */
	    //Error_Handler();

		  printf("dac err 5\r\n");
	  }
	#endif

	  //printf("dac ok\r\n");
}

//*----------------------------------------------------------------------------
//* Function Name       : trx_proc_hw_init
//* Object              :
//* Notes    			:
//* Notes   			:
//* Notes    			:
//* Context    			: CONTEXT_RESET
//*----------------------------------------------------------------------------
void trx_proc_hw_init(void)
{
	GPIO_InitTypeDef  gpio_init_structure;

	//printf("trx_proc_hw_init\r\n");

	gpio_init_structure.Pin   = FAN_CNTR;
	gpio_init_structure.Mode  = GPIO_MODE_OUTPUT_PP;
	gpio_init_structure.Pull  = GPIO_PULLDOWN;
	gpio_init_structure.Speed = GPIO_SPEED_FREQ_LOW;
	HAL_GPIO_Init(FAN_CNTR_PORT, &gpio_init_structure);

	// Fan On
	HAL_GPIO_WritePin(FAN_CNTR_PORT, FAN_CNTR, GPIO_PIN_SET);

	dacs_init();

	//printf("trx_proc_hw_init ok\r\n");
}

void trx_proc_power_clean_up(void)
{
	HAL_DAC_DeInit(&DacHandle);
}

//*----------------------------------------------------------------------------
//* Function Name       : trx_proc_worker
//* Object              :
//* Notes    			:
//* Notes   			:
//* Notes    			:
//* Context    			: CONTEXT_TRX
//*----------------------------------------------------------------------------
static void trx_proc_worker(ulong val)
{
	uchar mode = val & 0xFF;
	uchar dat0 = (val >>  8) & 0xFF;
	uchar dat1 = (val >> 16) & 0xFF;
	uchar dat2 = (val >> 24) & 0xFF;

	// Now on all the time, need RF board mod!
	#if 0
	// ----------------------------------------------------------
	// 8V ON/OFF for PA pre-amp
	if((mode & 2) == 2)
	{
		if(dat0)
		{
			printf("PA pre-amp power on\r\n");
			HAL_GPIO_WritePin(GPIOG, GPIO_PIN_9, GPIO_PIN_RESET);
		}
		else
		{
			printf("PA pre-amp power off\r\n");
			HAL_GPIO_WritePin(GPIOG, GPIO_PIN_9, GPIO_PIN_SET);
		}
	}
	#endif

	// ----------------------------------------------------------
	// Bias0 ON/OFF
	if((mode & 4) == 4)
	{
		if(dat1)
		{
			//printf("bias0 on, %d\r\n", tsu.bias0);
			if(HAL_DAC_SetValue(&DacHandle, DAC_CHANNEL_1, DAC_ALIGN_12B_R, tsu.bias0) != HAL_OK)
			{
				printf("dac err 4\r\n");
			}

			if (HAL_DAC_Start(&DacHandle, DAC_CHANNEL_1) != HAL_OK)
			{
				printf("dac err 5\r\n");
			}
		}
		else
		{
			printf("bias0 off\r\n");
			HAL_DAC_Stop(&DacHandle, DAC_CHANNEL_1);
		}
	}

	// ----------------------------------------------------------
	// Bias1 ON/OFF
	if((mode & 8) == 8)
	{
		if(dat2)
		{
			//printf("bias1 on, %d\r\n", tsu.bias1);
			if(HAL_DAC_SetValue(&DacHandle, DAC_CHANNEL_2, DAC_ALIGN_12B_R, tsu.bias1) != HAL_OK)
			{
				printf("dac err 4\r\n");
			}

			if (HAL_DAC_Start(&DacHandle, DAC_CHANNEL_2) != HAL_OK)
			{
				printf("dac err 5\r\n");
			}
		}
		else
		{
			printf("bias1 off\r\n");
			HAL_DAC_Stop(&DacHandle, DAC_CHANNEL_2);
		}
	}
}

//*----------------------------------------------------------------------------
//* Function Name       : trx_proc_task
//* Object              :
//* Notes    			:
//* Notes   			:
//* Notes    			:
//* Context    			: CONTEXT_TRX
//*----------------------------------------------------------------------------
void trx_proc_task(void const *arg)
{
	ulong 	ulNotificationValue = 0, ulNotif;

	vTaskDelay(TRX_PROC_START_DELAY);
	//printf("trx proc start\r\n");

	// Test only
	tsu.bias0 = 3400;
	tsu.bias1 = 3400;
	trx_proc_worker(0xFFFFFFFF);

	// Fan Off
	HAL_GPIO_WritePin(FAN_CNTR_PORT, FAN_CNTR, GPIO_PIN_RESET);

trx_proc_loop:

	ulNotif = xTaskNotifyWait(0x00, ULONG_MAX, &ulNotificationValue, TRX_PROC_SLEEP_TIME);
	if((ulNotif)&&(ulNotificationValue))
	{
		trx_proc_worker(ulNotificationValue);
	}

	goto trx_proc_loop;
}

#endif
