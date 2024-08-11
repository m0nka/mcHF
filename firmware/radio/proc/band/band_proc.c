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

#ifdef CONTEXT_BAND

#include "band_proc.h"

#include "stm32h7xx_hal.h"
#include "stm32h7xx_hal_gpio.h"

// Public radio state
extern struct	TRANSCEIVER_STATE_UI	tsu;

//*----------------------------------------------------------------------------
//* Function Name       : band_proc_hw_init
//* Object              :
//* Notes    			:
//* Notes   			:
//* Notes    			:
//* Context    			: CONTEXT_RESET
//*----------------------------------------------------------------------------
void band_proc_hw_init(void)
{
	GPIO_InitTypeDef  	GPIO_InitStruct;

	//printf("band_proc_hw_init\r\n");

	GPIO_InitStruct.Mode 		= GPIO_MODE_OUTPUT_PP;
	GPIO_InitStruct.Pull 		= GPIO_NOPULL;
	GPIO_InitStruct.Speed 		= GPIO_SPEED_FREQ_LOW;

	GPIO_InitStruct.Pin 		= BAND0_PIN;
	HAL_GPIO_Init(BAND0_PORT, &GPIO_InitStruct);

	GPIO_InitStruct.Pin 		= BAND1_PIN;
	HAL_GPIO_Init(BAND1_PORT, &GPIO_InitStruct);

	GPIO_InitStruct.Pin 		= BAND2_PIN;
	HAL_GPIO_Init(BAND2_PORT, &GPIO_InitStruct);

	GPIO_InitStruct.Pin 		= BAND3_PIN;
	HAL_GPIO_Init(BAND3_PORT, &GPIO_InitStruct);

	// LS145 disabled by default
	HAL_GPIO_WritePin(BAND3_PORT, BAND3_PIN, GPIO_PIN_SET);

	//printf("band_proc_hw_init ok\r\n");
}

void band_proc_power_cleanup(void)
{
	GPIO_InitTypeDef  	GPIO_InitStruct;

	GPIO_InitStruct.Mode 		= GPIO_MODE_OUTPUT_PP;
	GPIO_InitStruct.Pull 		= GPIO_NOPULL;
	GPIO_InitStruct.Speed 		= GPIO_SPEED_FREQ_LOW;

	GPIO_InitStruct.Pin 		= BAND0_PIN;
	HAL_GPIO_Init(BAND0_PORT, &GPIO_InitStruct);

	GPIO_InitStruct.Pin 		= BAND1_PIN;
	HAL_GPIO_Init(BAND1_PORT, &GPIO_InitStruct);

	GPIO_InitStruct.Pin 		= BAND2_PIN;
	HAL_GPIO_Init(BAND2_PORT, &GPIO_InitStruct);

	GPIO_InitStruct.Pin 		= BAND3_PIN;
	HAL_GPIO_Init(BAND3_PORT, &GPIO_InitStruct);

	// LS145 disabled by default
	HAL_GPIO_WritePin(BAND0_PORT, BAND0_PIN, GPIO_PIN_RESET);
	HAL_GPIO_WritePin(BAND1_PORT, BAND1_PIN, GPIO_PIN_RESET);
	HAL_GPIO_WritePin(BAND2_PORT, BAND2_PIN, GPIO_PIN_RESET);
	HAL_GPIO_WritePin(BAND3_PORT, BAND3_PIN, GPIO_PIN_SET);
}

//*----------------------------------------------------------------------------
//* Function Name       : band_proc_pulse_relays
//* Object              :
//* Notes    			: Pulse via A3 line
//* Notes   			: Minimum set/reset signal width is 10mS
//* Notes    			:
//* Context    			: CONTEXT_BAND
//*----------------------------------------------------------------------------
static void band_proc_pulse_relays(void)
{
	for(int i = 0; i < 4; i++)
	{
		HAL_GPIO_WritePin(BAND3_PORT, BAND3_PIN, GPIO_PIN_RESET);
		vTaskDelay(20);
		HAL_GPIO_WritePin(BAND3_PORT, BAND3_PIN, GPIO_PIN_SET);
		vTaskDelay(20);
	}
}

//*----------------------------------------------------------------------------
//* Function Name       : band_proc_k1_reset
//* Object              :
//* Notes    			: connect pins 2/3 and 7/6
//* Notes   			:
//* Notes    			:
//* Context    			: CONTEXT_BAND
//*----------------------------------------------------------------------------
static void band_proc_k1_reset(void)
{
	HAL_GPIO_WritePin(BAND3_PORT, BAND3_PIN, GPIO_PIN_SET);

	HAL_GPIO_WritePin(BAND0_PORT, BAND0_PIN, GPIO_PIN_RESET);
	HAL_GPIO_WritePin(BAND1_PORT, BAND1_PIN, GPIO_PIN_RESET);
	HAL_GPIO_WritePin(BAND2_PORT, BAND2_PIN, GPIO_PIN_RESET);
	band_proc_pulse_relays();

	//printf("K1 reset\r\n");
}

//*----------------------------------------------------------------------------
//* Function Name       : band_proc_k1_set
//* Object              :
//* Notes    			: connect pins 4/3 and 5/6
//* Notes   			:
//* Notes    			:
//* Context    			: CONTEXT_BAND
//*----------------------------------------------------------------------------
static void band_proc_k1_set(void)
{
	HAL_GPIO_WritePin(BAND3_PORT, BAND3_PIN, GPIO_PIN_SET);

	HAL_GPIO_WritePin(BAND0_PORT, BAND0_PIN, GPIO_PIN_SET);
	HAL_GPIO_WritePin(BAND1_PORT, BAND1_PIN, GPIO_PIN_RESET);
	HAL_GPIO_WritePin(BAND2_PORT, BAND2_PIN, GPIO_PIN_RESET);
	band_proc_pulse_relays();

	//printf("K1 set\r\n");
}

//*----------------------------------------------------------------------------
//* Function Name       : band_proc_k2_reset
//* Object              :
//* Notes    			: connect pins 2/3 and 7/6
//* Notes   			:
//* Notes    			:
//* Context    			: CONTEXT_BAND
//*----------------------------------------------------------------------------
static void band_proc_k2_reset(void)
{
	HAL_GPIO_WritePin(BAND3_PORT, BAND3_PIN, GPIO_PIN_SET);

	HAL_GPIO_WritePin(BAND0_PORT, BAND0_PIN, GPIO_PIN_RESET);
	HAL_GPIO_WritePin(BAND1_PORT, BAND1_PIN, GPIO_PIN_SET);
	HAL_GPIO_WritePin(BAND2_PORT, BAND2_PIN, GPIO_PIN_RESET);
	band_proc_pulse_relays();

	//printf("K2 reset\r\n");
}

//*----------------------------------------------------------------------------
//* Function Name       : band_proc_k2_set
//* Object              :
//* Notes    			: connect pins 4/3 and 5/6
//* Notes   			:
//* Notes    			:
//* Context    			: CONTEXT_BAND
//*----------------------------------------------------------------------------
static void band_proc_k2_set(void)
{
	HAL_GPIO_WritePin(BAND3_PORT, BAND3_PIN, GPIO_PIN_SET);

	HAL_GPIO_WritePin(BAND0_PORT, BAND0_PIN, GPIO_PIN_SET);
	HAL_GPIO_WritePin(BAND1_PORT, BAND1_PIN, GPIO_PIN_SET);
	HAL_GPIO_WritePin(BAND2_PORT, BAND2_PIN, GPIO_PIN_RESET);
	band_proc_pulse_relays();

	//printf("K2 set\r\n");
}

//*----------------------------------------------------------------------------
//* Function Name       : band_proc_k3_reset
//* Object              :
//* Notes    			: connect pins 2/3 and 7/6
//* Notes   			:
//* Notes    			:
//* Context    			: CONTEXT_BAND
//*----------------------------------------------------------------------------
static void band_proc_k3_reset(void)
{
	HAL_GPIO_WritePin(BAND3_PORT, BAND3_PIN, GPIO_PIN_SET);

	HAL_GPIO_WritePin(BAND0_PORT, BAND0_PIN, GPIO_PIN_RESET);
	HAL_GPIO_WritePin(BAND1_PORT, BAND1_PIN, GPIO_PIN_RESET);
	HAL_GPIO_WritePin(BAND2_PORT, BAND2_PIN, GPIO_PIN_SET);
	band_proc_pulse_relays();

	//printf("K3 reset\r\n");
}

//*----------------------------------------------------------------------------
//* Function Name       : band_proc_k3_set
//* Object              :
//* Notes    			: connect pins 4/3 and 5/6
//* Notes   			:
//* Notes    			:
//* Context    			: CONTEXT_BAND
//*----------------------------------------------------------------------------
static void band_proc_k3_set(void)
{
	HAL_GPIO_WritePin(BAND3_PORT, BAND3_PIN, GPIO_PIN_SET);

	HAL_GPIO_WritePin(BAND0_PORT, BAND0_PIN, GPIO_PIN_SET);
	HAL_GPIO_WritePin(BAND1_PORT, BAND1_PIN, GPIO_PIN_RESET);
	HAL_GPIO_WritePin(BAND2_PORT, BAND2_PIN, GPIO_PIN_SET);
	band_proc_pulse_relays();

	//printf("K3 set\r\n");
}

//*----------------------------------------------------------------------------
//* Function Name       : band_proc_k4_reset
//* Object              :
//* Notes    			: connect pins 2/3 and 7/6
//* Notes   			:
//* Notes    			:
//* Context    			: CONTEXT_BAND
//*----------------------------------------------------------------------------
static void band_proc_k4_reset(void)
{
	HAL_GPIO_WritePin(BAND3_PORT, BAND3_PIN, GPIO_PIN_SET);

	HAL_GPIO_WritePin(BAND0_PORT, BAND0_PIN, GPIO_PIN_RESET);
	HAL_GPIO_WritePin(BAND1_PORT, BAND1_PIN, GPIO_PIN_SET);
	HAL_GPIO_WritePin(BAND2_PORT, BAND2_PIN, GPIO_PIN_SET);
	band_proc_pulse_relays();

	//printf("K4 reset\r\n");
}

//*----------------------------------------------------------------------------
//* Function Name       : band_proc_k4_set
//* Object              :
//* Notes    			: connect pins 4/3 and 5/6
//* Notes   			:
//* Notes    			:
//* Context    			: CONTEXT_BAND
//*----------------------------------------------------------------------------
static void band_proc_k4_set(void)
{
	HAL_GPIO_WritePin(BAND3_PORT, BAND3_PIN, GPIO_PIN_SET);

	HAL_GPIO_WritePin(BAND0_PORT, BAND0_PIN, GPIO_PIN_SET);
	HAL_GPIO_WritePin(BAND1_PORT, BAND1_PIN, GPIO_PIN_SET);
	HAL_GPIO_WritePin(BAND2_PORT, BAND2_PIN, GPIO_PIN_SET);
	band_proc_pulse_relays();

	//printf("K4 set\r\n");
}

// -------------------------------------------------------
//	BAND		K1			K2			K3			K4
// -------------------------------------------------------
//    160m		R			S			S			R
//
//	   80m		R			R			R			R
//
//	60/40m		S			R			R			S
//
//	20/30m		S			S			S			S
//
//	17-10m		R			R			S			S
// -------------------------------------------------------
void band_proc_change_filter(uchar band, uchar bpf_only, uchar lpf_only)
{
	if(bpf_only)
		goto do_bpf;

	// Set LPFs
	switch(band)
	{
		case BAND_MODE_2200:
		case BAND_MODE_630:
		case BAND_MODE_160:
		{
			//printf("set 160m...\r\n");
			band_proc_k1_reset();
			band_proc_k2_set();
			band_proc_k3_set();
			band_proc_k4_reset();
			break;
		}

		case BAND_MODE_80:
		{
			//printf("set 80m...\r\n");
			band_proc_k1_reset();
			band_proc_k2_reset();
			band_proc_k3_reset();
			band_proc_k4_reset();
			break;
		}

		case BAND_MODE_60:
		case BAND_MODE_40:
		{
			//printf("set 60/40m...\r\n");
			band_proc_k1_set();
			band_proc_k2_reset();
			band_proc_k3_reset();
			band_proc_k4_set();
			break;
		}

		case BAND_MODE_30:
		case BAND_MODE_20:
		{
			//printf("set 30/20m...\r\n");
			band_proc_k1_set();
			band_proc_k2_set();
			band_proc_k3_set();
			band_proc_k4_set();
			break;
		}

		case BAND_MODE_17:
		case BAND_MODE_15:
		case BAND_MODE_12:
		case BAND_MODE_10:
		{
			//printf("set 17/10m...\r\n");
			band_proc_k1_reset();
			band_proc_k2_reset();
			band_proc_k3_set();
			band_proc_k4_set();
			break;
		}

		default:
			printf("not supported filter!\r\n");
			break;
	}

	if(lpf_only)
		return;

do_bpf:

	// Set BPFs
	// Constant line states for the BPF filter,
	// always last - after LPF change
	switch(band)
	{
		case BAND_MODE_2200:	// do we need to bypass those ?
		case BAND_MODE_630:		// --
		case BAND_MODE_160:
		case BAND_MODE_80:
		{
			//BAND0_PIO->BSRRL = BAND0;
			//BAND1_PIO->BSRRL = BAND1;
			HAL_GPIO_WritePin(BAND0_PORT, BAND0_PIN, GPIO_PIN_SET);
			HAL_GPIO_WritePin(BAND1_PORT, BAND1_PIN, GPIO_PIN_SET);
			break;
		}

		case BAND_MODE_60:
		case BAND_MODE_40:
		{
			//BAND0_PIO->BSRRL = BAND0;
			//BAND1_PIO->BSRRH = BAND1;
			HAL_GPIO_WritePin(BAND0_PORT, BAND0_PIN, GPIO_PIN_SET);
			HAL_GPIO_WritePin(BAND1_PORT, BAND1_PIN, GPIO_PIN_RESET);
			break;
		}

		case BAND_MODE_30:
		case BAND_MODE_20:
		{
			//BAND0_PIO->BSRRH = BAND0;
			//BAND1_PIO->BSRRH = BAND1;
			HAL_GPIO_WritePin(BAND0_PORT, BAND0_PIN, GPIO_PIN_RESET);
			HAL_GPIO_WritePin(BAND1_PORT, BAND1_PIN, GPIO_PIN_RESET);
			break;
		}

		case BAND_MODE_17:
		case BAND_MODE_15:
		case BAND_MODE_12:
		case BAND_MODE_10:
		{
			//BAND0_PIO->BSRRH = BAND0;
			//BAND1_PIO->BSRRL = BAND1;
			HAL_GPIO_WritePin(BAND0_PORT, BAND0_PIN, GPIO_PIN_RESET);
			HAL_GPIO_WritePin(BAND1_PORT, BAND1_PIN, GPIO_PIN_SET);
			break;
		}

		default:
			break;
	}
	//printf("--------------\r\n");
}

//*----------------------------------------------------------------------------
//* Function Name       : band_proc_task
//* Object              :
//* Notes    			:
//* Notes   			:
//* Notes    			:
//* Context    			: CONTEXT_BAND
//*----------------------------------------------------------------------------
void band_proc_task(void const * argument)
{
	ulong 	ulNotificationValue = 0, ulNotif;

	vTaskDelay(BAND_PROC_START_DELAY);
	//printf("band proc start\r\n");

	//band_proc_change_filter(BAND_MODE_10, 0, 0);
	//vTaskDelay(50);
	//band_proc_change_filter(BAND_MODE_20, 0, 0);
	//vTaskDelay(50);
	//band_proc_change_filter(BAND_MODE_40, 0, 0);
	//vTaskDelay(50);
	//band_proc_change_filter(BAND_MODE_160, 0, 0);
	//vTaskDelay(50);
	band_proc_change_filter(tsu.curr_band, 0, 0);

band_proc_loop:

	ulNotif = xTaskNotifyWait(0x00, ULONG_MAX, &ulNotificationValue, BAND_PROC_SLEEP_TIME);
	if((ulNotif)&&(ulNotificationValue))
	{
		band_proc_change_filter(tsu.curr_band, 0, 0);	// use notification value or public ?
	}

	vTaskDelay(500);

	goto band_proc_loop;
}

#endif
