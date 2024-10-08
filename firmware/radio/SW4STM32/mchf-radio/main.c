/************************************************************************************
**                                                                                 **
**                             mcHF Pro QRP Transceiver                            **
**                         Krassi Atanassov - M0NKA, 2013-2024                     **
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

#include "version.h"
#include "radio_init.h"
#include "k_rtc.h"

#include "bsp.h"
#include "WM.h"

#include "ipc_proc.h"
#include "ui_proc.h"
#include "icc_proc.h"
#include "audio_proc.h"
#include "touch_proc.h"
#include "rotary_proc.h"
#include "bms_proc.h"
#include "vfo_proc.h"
#include "band_proc.h"
#include "trx_proc.h"

#if configAPPLICATION_ALLOCATED_HEAP == 1
__attribute__((section("heap_mem"))) uint8_t ucHeap[ configTOTAL_HEAP_SIZE ];
#endif

uint8_t BSP_Initialized = 0;
uint32_t wakeup_pressed = 0;

// UI process
extern struct	UI_DRIVER_STATE			ui_s;

// DSP core state
struct TransceiverState 				ts;

TaskHandle_t 							hIccTask	= NULL;
TaskHandle_t 							hTouchTask	= NULL;
TaskHandle_t 							hUiTask		= NULL;
TaskHandle_t 							hVfoTask	= NULL;
TaskHandle_t 							hAudioTask	= NULL;
TaskHandle_t 							hBandTask	= NULL;
TaskHandle_t 							hTrxTask	= NULL;

QueueHandle_t 							hEspMessage;

#if defined(USE_USB_FS) || defined(USE_USB_HS)
extern HCD_HandleTypeDef hhcd;
#endif /* USE_USB_FS | USE_USB_HS */

// Combined LCD/Touch reset flag
uchar lcd_touch_reset_done = 0;

void NMI_Handler(void)
{
	Error_Handler(11);
}

void HardFault_Handler(void)
{
	printf( "====================\r\n");
	printf( "=    HARD FAULT    =\r\n");
	printf( "=     %s     =\r\n", pcTaskGetName(NULL));
	printf( "====================\r\n");

	//NVIC_SystemReset();
	while(1);
}

void MemManage_Handler(void)
{
	Error_Handler(13);
}

void BusFault_Handler(void)
{
	Error_Handler(14);
}

void UsageFault_Handler(void)
{
	Error_Handler(15);
}

void DebugMon_Handler(void)
{
}

void SysTick_Handler(void)
{
	osSystickHandler();
}

void Error_Handler(int err)
{
  /* Turn LED RED on */
  //if(BSP_Initialized)
  //	BSP_LED_On(LED_RED);

  printf( " Error Handler %d\n",err);
  //configASSERT (0);
}

void BSP_ErrorHandler(void)
{
  //if(BSP_Initialized)
  //{
    //printf( "%s(): BSP Error !!!\n", __func__ );
   // BSP_LED_On(LED_RED);
  //}
}

#ifdef configUSE_MALLOC_FAILED_HOOK
/**
  * @brief  Application Malloc failure Hook
  * @param  None
  * @retval None
  */
void vApplicationMallocFailedHook(TaskHandle_t xTask, char *pcTaskName)
{
  printf( "%s(): MALLOC FAILED !!!\n", pcTaskName );

  Error_Handler(18);
}
#endif /* configUSE_MALLOC_FAILED_HOOK */

#ifdef configCHECK_FOR_STACK_OVERFLOW
void vApplicationStackOverflowHook( TaskHandle_t xTask, char *pcTaskName )
{
  printf( "%s(): STACK OVERFLOW !!!\n", pcTaskName );

  Error_Handler(19);
}
#endif /* configCHECK_FOR_STACK_OVERFLOW */

#ifdef  USE_FULL_ASSERT

/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t* file, uint32_t line)
{
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */

  /* Infinite loop */
  while (1)
  {
  }
}
#endif

//*----------------------------------------------------------------------------
//* Function Name       : start_proc
//* Object              :
//* Notes    			:
//* Notes   			:
//* Notes    			:
//* Context    			: CONTEXT_RESET
//*----------------------------------------------------------------------------
static int start_proc(void)
{
	BaseType_t res;

	hEspMessage = xQueueCreate(5, sizeof(struct ESPMessage *));

	#ifdef CONTEXT_VIDEO
	res = xTaskCreate(	(TaskFunction_t)ui_proc_task,\
						"gui_proc",\
						UI_PROC_STACK_SIZE,\
						NULL,\
						UI_PROC_PRIORITY,\
						&hUiTask);

	if(res != pdPASS)
	{
		printf("unable to create ui process\r\n");
		return 1;
	}
	#endif

	// Create TS Thread
	#ifdef CONTEXT_TOUCH
    res = xTaskCreate(	(TaskFunction_t)touch_proc_task,\
    					"tch_proc",\
						TOUCH_PROC_STACK_SIZE,\
						NULL,\
						TOUCH_PROC_PRIORITY,\
						&hTouchTask);

    if(res != pdPASS)
    {
    	printf("unable to create touch process\r\n");
    	return 2;
    }
	#endif

	#ifdef CONTEXT_ICC
    res = xTaskCreate(	(TaskFunction_t)icc_proc_task,\
    					"icc_proc",\
						UI_PROC_STACK_SIZE,\
						NULL,\
						UI_PROC_PRIORITY,\
						&hIccTask);

    if(res != pdPASS)
    {
       printf("unable to create icc process\r\n");
       return 3;
    }
	#endif

	#ifdef CONTEXT_IPC_PROC
    res = xTaskCreate(	(TaskFunction_t)ipc_proc_task,\
    					"ipc_proc",\
						UI_PROC_STACK_SIZE,\
						NULL,\
						UI_PROC_PRIORITY,\
						NULL);

    if(res != pdPASS)
    {
    	printf("unable to create ipc process\r\n");
    	return 4;
    }
	#endif

	#ifdef CONTEXT_ROTARY
    res = xTaskCreate(	(TaskFunction_t)rotary_proc_task,\
    					"rot_proc",\
						ROTARY_PROC_STACK_SIZE,\
						NULL,\
						ROTARY_PROC_PRIORITY,\
						NULL);

    if(res != pdPASS)
    {
    	printf("unable to create rotary process\r\n");
    	return 5;
    }
	#endif

	#ifdef CONTEXT_VFO
    res = xTaskCreate(	(TaskFunction_t)vfo_proc_task,\
    					"vfo_proc",\
						VFO_PROC_STACK_SIZE,\
						NULL,\
						VFO_PROC_PRIORITY,\
						&hVfoTask);

    if(res != pdPASS)
    {
    	printf("unable to create vfo process\r\n");
    	return 6;
    }
	#endif

	#ifdef CONTEXT_AUDIO
    res = xTaskCreate(	(TaskFunction_t)audio_proc_task,\
    					"aud_proc",\
						AUDIO_PROC_STACK_SIZE,\
						NULL,\
						AUDIO_PROC_PRIORITY,\
						&hAudioTask);

    if(res != pdPASS)
    {
        printf("unable to create audio process\r\n");
        return 7;
    }
    #endif

	#ifdef CONTEXT_BMS
    res = xTaskCreate(	(TaskFunction_t)bms_proc_task,\
    					"bms_proc",\
						BMS_PROC_STACK_SIZE,\
						NULL,\
						BMS_PROC_PRIORITY,\
						NULL);

    if(res != pdPASS)
    {
    	printf("unable to create bms process\r\n");
    	return 8;
    }
	#endif

    // PWM...


	#ifdef CONTEXT_BAND
    res = xTaskCreate(	(TaskFunction_t)band_proc_task,\
    					"bnd_proc",\
						BAND_PROC_STACK_SIZE,\
						NULL,\
						BAND_PROC_PRIORITY,\
						&hBandTask);

    if(res != pdPASS)
    {
    	printf("unable to create band control process\r\n");
    	return 10;
    }
	#endif

	#ifdef CONTEXT_TRX
    res = xTaskCreate(	(TaskFunction_t)trx_proc_task,\
    					"trx_proc",\
						TRX_PROC_STACK_SIZE,\
						NULL,\
						TRX_PROC_PRIORITY,\
						&hTrxTask);

    if(res != pdPASS)
    {
    	printf("unable to create trx process\r\n");
    	return 6;
    }
	#endif

    return 0;
}

//*----------------------------------------------------------------------------
//* Function Name       : main
//* Object              :
//* Notes    			:
//* Notes   			:
//* Notes    			:
//* Context    			: CONTEXT_RESET
//*----------------------------------------------------------------------------
int main(void)
{
	bsp_gpio_clocks_on();
	bsp_hold_power();

	// Disable FMC Bank1 to avoid speculative/cache accesses
	FMC_Bank1_R->BTCR[0] &= ~FMC_BCRx_MBKEN;

	// ICC driver/printf needs this
	__HAL_RCC_HSEM_CLK_ENABLE();

    // Configure the MPU attributes as Write Through
    MPU_Config();

    // Enable the CPU Cache
    CPU_CACHE_Enable();

    // HAL init
    HAL_Init();

    // Configure the system clock to 400 MHz
    SystemClock_Config();

    // RTC init
    k_CalendarBkupInit();

    // Set radio public values
    radio_init_on_reset();

    // HW init
    if(BSP_Config() != 0)
    	goto stall_radio;

    // Define running processes
    if(start_proc())
    	goto stall_radio;

    // Do we need this at all ?
    BSP_Initialized = 1;
    //printf("run os...\r\n");

    // Start scheduler
    osKernelStart();

stall_radio:
    while(1);
}
