/************************************************************************************
**                                                                                 **
**                                 mcHF QRP Transceiver                            **
**                         Krassi Atanassov - M0NKA, 2013-2026                     **
**                                                                                 **
**---------------------------------------------------------------------------------**
**                                                                                 **
**  File name:                                                                     **
**  Description:                                                                   **
**  Last Modified:                                                                 **
**  Licence:			https://github.com/m0nka/mcHF/blob/main/LICENSE            **
************************************************************************************/
#include "mchf_pro_board.h"
#include "main.h"

// Reserved FreeRTOS heap memory
#if configAPPLICATION_ALLOCATED_HEAP == 1
__attribute__((section(".axi_mem"))) uint8_t ucHeap[configTOTAL_HEAP_SIZE];
#endif

// UI process
extern struct	UI_DRIVER_STATE			ui_s;

// DSP core state
struct TransceiverState 				ts;

// FreeRTOS process state
struct PROC_STATE 						ps;

// App Loader messaging(inherited from Genie code)
APPLOADER_QUEUE_PARAMETERS 				pxAppLoaderParameters;

//*----------------------------------------------------------------------------
//* Function Name       : NMI_Handler
//* Object              :
//* Notes    			:
//* Notes   			:
//* Notes    			:
//* Context    			: CONTEXT_RESET
//*----------------------------------------------------------------------------
void NMI_Handler(void)
{
	Error_Handler(11);
}

//*----------------------------------------------------------------------------
//* Function Name       : HardFault_Handler
//* Object              :
//* Notes    			:
//* Notes   			:
//* Notes    			:
//* Context    			: CONTEXT_RESET
//*----------------------------------------------------------------------------
void HardFault_Handler(void)
{
	printf( "====================\r\n");
	printf( "=    HARD FAULT    =\r\n");
	printf( "=       [%s]      =\r\n", pcTaskGetName(NULL));
	printf( "====================\r\n");

	#if 0
	NVIC_SystemReset();
	#else
	//HAL_GPIO_WritePin(LCD_BL_CTRL_GPIO_PORT, LCD_BL_CTRL_PIN, GPIO_PIN_RESET);
	LL_GPIO_ResetOutputPin(POWER_HOLD_PORT, POWER_HOLD);
	#endif

	while(1)
	{
		__asm("nop");
	}
}

//*----------------------------------------------------------------------------
//* Function Name       : MemManage_Handler
//* Object              :
//* Notes    			:
//* Notes   			:
//* Notes    			:
//* Context    			: CONTEXT_RESET
//*----------------------------------------------------------------------------
void MemManage_Handler(void)
{
	Error_Handler(13);
}

//*----------------------------------------------------------------------------
//* Function Name       : BusFault_Handler
//* Object              :
//* Notes    			:
//* Notes   			:
//* Notes    			:
//* Context    			: CONTEXT_RESET
//*----------------------------------------------------------------------------
void BusFault_Handler(void)
{
	Error_Handler(14);
}

//*----------------------------------------------------------------------------
//* Function Name       : UsageFault_Handler
//* Object              :
//* Notes    			:
//* Notes   			:
//* Notes    			:
//* Context    			: CONTEXT_RESET
//*----------------------------------------------------------------------------
void UsageFault_Handler(void)
{
	Error_Handler(15);
}

//*----------------------------------------------------------------------------
//* Function Name       : DebugMon_Handler
//* Object              :
//* Notes    			:
//* Notes   			:
//* Notes    			:
//* Context    			: CONTEXT_RESET
//*----------------------------------------------------------------------------
void DebugMon_Handler(void)
{
}

//*----------------------------------------------------------------------------
//* Function Name       : SysTick_Handler
//* Object              :
//* Notes    			:
//* Notes   			:
//* Notes    			:
//* Context    			: CONTEXT_RESET
//*----------------------------------------------------------------------------
void SysTick_Handler(void)
{
	// Local timers
	(ps.epoch)++;

	// Os Tick
	osSystickHandler();

	// Hal tick
	#ifndef USE_SEPARATE_TIMER_FOR_HAL
	HAL_IncTick();
	#endif
}

//
// ToDo: There is something wrong with EXTI routing! Check before PCB rev B!
//

//*----------------------------------------------------------------------------
//* Function Name       : EXTI0_IRQHandler
//* Object              :
//* Notes    			: exti trap, line0
//* Notes   			:
//* Notes    			:
//* Context    			: CONTEXT_IRQ
//*----------------------------------------------------------------------------
void EXTI0_IRQHandler(void)
{
	#if 0
	if(__HAL_GPIO_EXTI_GET_IT(GPIO_PIN_0) != 0x00U)
	{
		BaseType_t xHigherPriorityTaskWoken;

		xHigherPriorityTaskWoken = pdFALSE;
		xTaskNotifyFromISR(ps.hSdcTask, 0x45, eSetBits, &xHigherPriorityTaskWoken);
		portYIELD_FROM_ISR(xHigherPriorityTaskWoken );

		__HAL_GPIO_EXTI_CLEAR_IT(GPIO_PIN_0);
	}
	#else
	// Line 0
	if(LL_EXTI_IsActiveFlag_0_31(LL_EXTI_LINE_0) != RESET)
	{
		BaseType_t xHigherPriorityTaskWoken;

		LL_EXTI_ClearFlag_0_31(LL_EXTI_LINE_0);

		xHigherPriorityTaskWoken = pdFALSE;
		xTaskNotifyFromISR(ps.hSdcTask, 0x45, eSetBits, &xHigherPriorityTaskWoken);
		portYIELD_FROM_ISR(xHigherPriorityTaskWoken );
	}
	#endif
}

//*----------------------------------------------------------------------------
//* Function Name       : EXTI4_IRQHandler
//* Object              :
//* Notes    			: Lora driver
//* Notes   			:
//* Notes    			:
//* Context    			: CONTEXT_IRQ
//*----------------------------------------------------------------------------
void EXTI4_IRQHandler(void)
{
	#if 0
	if (__HAL_GPIO_EXTI_GET_IT(LORA_DIO1) != 0x00U)
	{
		lora_proc_dio1_irq();
	    __HAL_GPIO_EXTI_CLEAR_IT(LORA_DIO1);
	}
	#else
	// Line 4
	if(LL_EXTI_IsActiveFlag_0_31(LL_EXTI_LINE_4) != RESET)
	{
		LL_EXTI_ClearFlag_0_31(LL_EXTI_LINE_4);

		#ifdef CONTEXT_LORA
		lora_proc_dio1_irq();
		#endif
	}
	#endif
}

//*----------------------------------------------------------------------------
//* Function Name       : EXTI9_5_IRQHandler
//* Object              :
//* Notes    			: Shared between Touch and Lora drivers
//* Notes   			:
//* Notes    			:
//* Context    			: CONTEXT_IRQ
//*----------------------------------------------------------------------------
void EXTI9_5_IRQHandler(void)
{
	// Line 5
	if(LL_EXTI_IsActiveFlag_0_31(LL_EXTI_LINE_5) != RESET)
	{
		LL_EXTI_ClearFlag_0_31(LL_EXTI_LINE_5);

		#ifdef CONTEXT_LORA
		lora_proc_busy_irq();
		#endif
	}
	else if(LL_EXTI_IsActiveFlag_0_31(LL_EXTI_LINE_6) != RESET)
	{
		LL_EXTI_ClearFlag_0_31(LL_EXTI_LINE_6);

		#ifdef CONTEXT_TOUCH
		touch_proc_irq();
		#endif
	}
	else if(LL_EXTI_IsActiveFlag_0_31(LL_EXTI_LINE_8) != RESET)
	{
		LL_EXTI_ClearFlag_0_31(LL_EXTI_LINE_8);

		#ifdef CONTEXT_KEYPAD
		keypad_proc_irq(8);
		#else
		GPS_PPS_IRQHandler();
		#endif
	}
}

//*----------------------------------------------------------------------------
//* Function Name       : EXTI15_10_IRQHandler
//* Object              :
//* Notes    			: Handle keyboard events
//* Notes   			:
//* Notes    			:
//* Context    			: CONTEXT_IRQ
//*----------------------------------------------------------------------------
void EXTI15_10_IRQHandler(void)
{
	if(LL_EXTI_IsActiveFlag_0_31(LL_EXTI_LINE_11) != RESET)
	{
		LL_EXTI_ClearFlag_0_31(LL_EXTI_LINE_11);

		#ifdef CONTEXT_KEYPAD
		keypad_proc_irq(11);
		#endif
	}
	else if(LL_EXTI_IsActiveFlag_0_31(LL_EXTI_LINE_12) != RESET)
	{
		LL_EXTI_ClearFlag_0_31(LL_EXTI_LINE_12);

		#ifdef CONTEXT_KEYPAD
		keypad_proc_irq(12);
		#endif
	}
}

//*----------------------------------------------------------------------------
//* Function Name       : Error_Handler
//* Object              :
//* Notes    			:
//* Notes   			:
//* Notes    			:
//* Context    			: CONTEXT_RESET
//*----------------------------------------------------------------------------
void Error_Handler(int err)
{
	__disable_irq();
	printf(" Error Handler %d\n", err);

	//NVIC_SystemReset();
	while(1);
}

#ifdef configUSE_MALLOC_FAILED_HOOK
//*----------------------------------------------------------------------------
//* Function Name       : vApplicationMallocFailedHook
//* Object              :
//* Notes    			:
//* Notes   			:
//* Notes    			:
//* Context    			: CONTEXT_RESET
//*----------------------------------------------------------------------------
void vApplicationMallocFailedHook(TaskHandle_t xTask, char *pcTaskName)
{
  printf( "%s(): MALLOC FAILED !!!\n", pcTaskName );

  Error_Handler(18);
}
#endif

#ifdef configCHECK_FOR_STACK_OVERFLOW
//*----------------------------------------------------------------------------
//* Function Name       : vApplicationStackOverflowHook
//* Object              :
//* Notes    			:
//* Notes   			:
//* Notes    			:
//* Context    			: CONTEXT_RESET
//*----------------------------------------------------------------------------
void vApplicationStackOverflowHook( TaskHandle_t xTask, char *pcTaskName )
{
  printf( "%s(): STACK OVERFLOW !!!\n", pcTaskName );

  Error_Handler(19);
}
#endif

//*----------------------------------------------------------------------------
//* Function Name       : tasks_pre_os_init
//* Object              :
//* Notes    			: All hardware init that needs to be done before the
//* Notes   			: OS start here
//* Notes    			:
//* Context    			: CONTEXT_RESET
//*----------------------------------------------------------------------------
static void tasks_pre_os_init(void)
{
	// Clear all handles
	ps.hIccTask		= NULL;
	ps.hTouchTask	= NULL;
	ps.hUiTask		= NULL;
	ps.hVfoTask		= NULL;
	ps.hAudioTask	= NULL;
	ps.hBandTask	= NULL;
	ps.hTrxTask		= NULL;
	ps.hKbdTask		= NULL;
	ps.hLraTask		= NULL;
	ps.hSdcTask		= NULL;
	ps.hAppTask		= NULL;

	#ifdef CONTEXT_SD
	storage_proc_init();
	#else
	radio_init_on_reset();
	#endif

	#ifdef CONTEXT_BMS
	bms_proc_hw_init();
	#endif

	#ifdef CONTEXT_ROTARY
	rotary_proc_hw_init();
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

	#ifdef CONTEXT_FAN
	fan_proc_hw_init();
	#endif

	#ifdef CONTEXT_KEYPAD
	keypad_proc_init();
	#endif

	#ifdef CONTEXT_LORA
	lora_proc_init();
	#endif

	#ifdef CONTEXT_APP
	os_apploader_init();
	#endif
}

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

	//hEspMessage = xQueueCreate(5, sizeof(struct ESPMessage *));

	//#ifdef CONTEXT_DSP
	//osMessageQDef(dsp_queue, 5, sizeof(struct DSPMessage *));
	//hDspMessage = osMessageCreate (osMessageQ(dsp_queue), NULL);
	//#endif

	// Create the queue RX and TX queue used by the App Loader service and the USB driver
	// Pass a pointer to the queue in the parameter structure.
	pxAppLoaderParameters.xAppLoaderRxQueue 		= xQueueCreate(APP_LOADER_QUEUE_SIZE,(unsigned portCHAR)sizeof(ulong));
    pxAppLoaderParameters.xAppLoaderTxQueue 		= xQueueCreate(APP_LOADER_QUEUE_SIZE,(unsigned portCHAR)sizeof(ulong));

  	pxAppLoaderParameters.xI2CDriverRxQueue 		= xQueueCreate(APP_LOADER_QUEUE_SIZE,(unsigned portCHAR)sizeof(ulong));
    pxAppLoaderParameters.xI2CDriverTxQueue 		= xQueueCreate(APP_LOADER_QUEUE_SIZE,(unsigned portCHAR)sizeof(ulong));

	pxAppLoaderParameters.xCryptoServiceRxQueue 	= xQueueCreate(APP_LOADER_QUEUE_SIZE,(unsigned portCHAR)sizeof(ulong));
    pxAppLoaderParameters.xCryptoServiceTxQueue 	= xQueueCreate(APP_LOADER_QUEUE_SIZE,(unsigned portCHAR)sizeof(ulong));

    pxAppLoaderParameters.ulTasksStatus 			= 0;	// nothing running

    // BMS messaging
    ps.xBmsRxQueue = xQueueCreate(APP_LOADER_QUEUE_SIZE,(unsigned portCHAR)sizeof(ulong));

    // UI notifications
    ps.xUiNotifRxQueue = xQueueCreate(APP_LOADER_QUEUE_SIZE,(unsigned portCHAR)sizeof(ulong));

	#ifdef CONTEXT_VIDEO
	res = xTaskCreate(	(TaskFunction_t)ui_proc_task,\
						UI_PROC_START_NAME,\
						UI_PROC_STACK_SIZE,\
						(void *)&(ps.xUiNotifRxQueue),\
						UI_PROC_PRIORITY,\
						&(ps.hUiTask));

	if(res != pdPASS)
	{
		printf("unable to create ui process\r\n");
		return 1;
	}
	#endif

	// Create TS Thread
	#ifdef CONTEXT_TOUCH
    res = xTaskCreate(	(TaskFunction_t)touch_proc_task,\
    					TOUCH_PROC_START_NAME,\
						TOUCH_PROC_STACK_SIZE,\
						NULL,\
						TOUCH_PROC_PRIORITY,\
						&(ps.hTouchTask));

    if(res != pdPASS)
    {
    	printf("unable to create touch process\r\n");
    	return 2;
    }
	#endif

	#ifdef CONTEXT_ICC
    res = xTaskCreate(	(TaskFunction_t)icc_proc_task,\
    					ICC_PROC_START_NAME,\
						ICC_PROC_STACK_SIZE,\
						NULL,\
						ICC_PROC_PRIORITY,\
						&(ps.hIccTask));

    if(res != pdPASS)
    {
       printf("unable to create icc process\r\n");
       return 3;
    }
	#endif

	#ifdef CONTEXT_ROTARY
    res = xTaskCreate(	(TaskFunction_t)rotary_proc_task,\
    					ROTARY_PROC_START_NAME,\
						ROTARY_PROC_STACK_SIZE,\
						NULL,\
						ROTARY_PROC_PRIORITY,\
						NULL);

    if(res != pdPASS)
    {
    	printf("unable to create rotary process\r\n");
    	return 4;
    }
	#endif

	#ifdef CONTEXT_VFO
    res = xTaskCreate(	(TaskFunction_t)vfo_proc_task,\
    					VFO_PROC_START_NAME,\
						VFO_PROC_STACK_SIZE,\
						NULL,\
						VFO_PROC_PRIORITY,\
						&(ps.hVfoTask));

    if(res != pdPASS)
    {
    	printf("unable to create vfo process\r\n");
    	return 5;
    }
	#endif

	#ifdef CONTEXT_AUDIO
    res = xTaskCreate(	(TaskFunction_t)audio_proc_task,\
    					AUDIO_PROC_START_NAME,\
						AUDIO_PROC_STACK_SIZE,\
						NULL,\
						AUDIO_PROC_PRIORITY,\
						&(ps.hAudioTask));

    if(res != pdPASS)
    {
        printf("unable to create audio process\r\n");
        return 6;
    }
    #endif

	#ifdef CONTEXT_BMS
    res = xTaskCreate(	(TaskFunction_t)bms_proc_task,\
    					BMS_PROC_START_NAME,\
						BMS_PROC_STACK_SIZE,\
						(void *)&(ps.xBmsRxQueue),\
						BMS_PROC_PRIORITY,\
						NULL);

    if(res != pdPASS)
    {
    	printf("unable to create bms process\r\n");
    	return 7;
    }
	#endif

    // PWM...

	#ifdef CONTEXT_BAND
    res = xTaskCreate(	(TaskFunction_t)band_proc_task,\
    					BAND_PROC_START_NAME,\
						BAND_PROC_STACK_SIZE,\
						NULL,\
						BAND_PROC_PRIORITY,\
						&(ps.hBandTask));

    if(res != pdPASS)
    {
    	printf("unable to create band control process\r\n");
    	return 8;
    }
	#endif

	#ifdef CONTEXT_TRX
    res = xTaskCreate(	(TaskFunction_t)trx_proc_task,\
    					TRX_PROC_START_NAME,\
						TRX_PROC_STACK_SIZE,\
						NULL,\
						TRX_PROC_PRIORITY,\
						&(ps.hTrxTask));

    if(res != pdPASS)
    {
    	printf("unable to create trx process\r\n");
    	return 9;
    }
	#endif

	#ifdef CONTEXT_FAN
    res = xTaskCreate((TaskFunction_t)fan_proc_task,\
					FAN_PROC_START_NAME,\
					FAN_PROC_STACK_SIZE,\
					NULL,\
					FAN_PROC_PRIORITY,\
					&(ps.hFanTask));

    if(res != pdPASS)
    {
    	printf("unable to create fan process\r\n");
    	return 10;
    }
	#endif

	#ifdef CONTEXT_KEYPAD
    res = xTaskCreate(	(TaskFunction_t)keypad_proc_task,\
    					KEYPAD_PROC_START_NAME,\
						KEYPAD_PROC_STACK_SIZE,\
						NULL,\
						KEYPAD_PROC_PRIORITY,\
						&(ps.hKbdTask));

    if(res != pdPASS)
    {
    	printf("unable to create kbd process\r\n");
    	return 11;
    }
	#endif

	#ifdef CONTEXT_LORA
    res = xTaskCreate(	(TaskFunction_t)lora_proc_task,\
    					LORA_PROC_START_NAME,\
						LORA_PROC_STACK_SIZE,\
						(void *)&(ps.xUiNotifRxQueue),\
						LORA_PROC_PRIORITY,\
						&(ps.hLraTask));

    if(res != pdPASS)
    {
    	printf("unable to create lora process\r\n");
    	return 12;
    }
	#endif

	#ifdef CONTEXT_SD
    res = xTaskCreate(	(TaskFunction_t)storage_proc_task,\
    					SD_PROC_START_NAME,\
						SD_PROC_STACK_SIZE,\
						NULL,\
						SD_PROC_PRIORITY,\
						&(ps.hSdcTask));

    if(res != pdPASS)
    {
    	printf("unable to create sd card process\r\n");
    	return 13;
    }
	#endif

	#ifdef CONTEXT_APP
    res = xTaskCreate(	(TaskFunction_t)os_apploader_task,\
    					APP_PROC_START_NAME,\
						APP_PROC_STACK_SIZE,\
						(void *)&pxAppLoaderParameters,\
						APP_PROC_PRIORITY,\
						&(ps.hAppTask));

    if(res != pdPASS)
    {
    	printf("unable to create app loader process\r\n");
    	return 14;
    }
	#endif

	#ifdef CONTEXT_GPS
    res = xTaskCreate(	(TaskFunction_t)gps_proc,\
    					GPS_PROC_START_NAME,\
						GPS_PROC_STACK_SIZE,\
						(void *)&pxAppLoaderParameters,\
						osPriorityNormal,\
						&(ps.hGpsTask));

    if(res != pdPASS)
    {
    	printf("unable to create gps process\r\n");
    	return 15;
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
	// Hold power line
	bsp_hold_power();

	// All GPIO clocks
	bsp_gpio_clocks_on();

	// Disable FMC Bank1 to avoid speculative/cache accesses
	FMC_Bank1_R->BTCR[0] &= ~FMC_BCRx_MBKEN;

	// ICC driver/printf needs this
	__HAL_RCC_HSEM_CLK_ENABLE();

    // Configure the MPU attributes as Write Through
    MPU_Config();

    // Enable the CPU Cache
    CPU_CACHE_Enable();

    // Clear system timer
    ps.epoch = 0;

    // HAL init
    HAL_Init();

    // Configure the system clock to 480 MHz
    SystemClock_Config();

	// ADC/SD card clock from PLL2
	PeriphCommonClock_Config();

    // HW init
    if(bsp_config() != 0)
    	goto stall_radio;

    // RTC init
    k_CalendarBkupInit();

    // Init each task hw
    tasks_pre_os_init();

    // Init ADC HW
    if(adc_init() != 0)
    	goto stall_radio;

    // Attenuator
    att_hw_init();

    // Define running processes
    if(start_proc())
    	goto stall_radio;

    // Start scheduler
    osKernelStart();

stall_radio:
    while(1);
}
