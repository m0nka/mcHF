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
#include "version.h"

#include "icc_proc.h"
#include "dsp_proc.h"
#include "dsp_idle_proc.h"
#include "cw_gen.h"

#include "stm32h7xx_ll_bus.h"
#include "stm32h7xx_ll_cortex.h"
#include "stm32h7xx_ll_gpio.h"
#include "stm32h7xx_ll_exti.h"
#include "stm32h7xx_ll_rcc.h"

//#define HSEM_ID_0 (0U)

// Transceiver state public structure
__IO TransceiverState ts;

extern __IO PaddleState				ps;
extern ulong *Reset_Handler;

//*----------------------------------------------------------------------------
//* Function Name       : CriticalError
//* Object              : should never be here, really
//* Object              :
//* Input Parameters    :
//* Output Parameters   :
//* Functions called    :
//*----------------------------------------------------------------------------
void CriticalError(unsigned long error)
{
	printf("critical err %d\r\n", error);
	//NVIC_SystemReset();
	while(1);
}

//*----------------------------------------------------------------------------
//* Function Name       : NMI_Handler
//* Object              :
//* Object              :
//* Input Parameters    :
//* Output Parameters   :
//* Functions called    :
//*----------------------------------------------------------------------------
void NMI_Handler(void)
{
	CriticalError(1);
}

//*----------------------------------------------------------------------------
//* Function Name       : HardFault_Handler
//* Object              :
//* Object              :
//* Input Parameters    :
//* Output Parameters   :
//* Functions called    :
//*----------------------------------------------------------------------------
void HardFault_Handler(void)
{
	CriticalError(2);
}

//*----------------------------------------------------------------------------
//* Function Name       : MemManage_Handler
//* Object              :
//* Object              :
//* Input Parameters    :
//* Output Parameters   :
//* Functions called    :
//*----------------------------------------------------------------------------
void MemManage_Handler(void)
{
	CriticalError(3);
}

//*----------------------------------------------------------------------------
//* Function Name       : BusFault_Handler
//* Object              :
//* Object              :
//* Input Parameters    :
//* Output Parameters   :
//* Functions called    :
//*----------------------------------------------------------------------------
void BusFault_Handler(void)
{
	CriticalError(4);
}

//*----------------------------------------------------------------------------
//* Function Name       : UsageFault_Handler
//* Object              :
//* Object              :
//* Input Parameters    :
//* Output Parameters   :
//* Functions called    :
//*----------------------------------------------------------------------------
void UsageFault_Handler(void)
{
	CriticalError(5);
}

//*----------------------------------------------------------------------------
//* Function Name       : SVC_Handler
//* Object              :
//* Object              :
//* Input Parameters    :
//* Output Parameters   :
//* Functions called    :
//*----------------------------------------------------------------------------
void SVC_Handler(void)
{
	CriticalError(6);
}

//*----------------------------------------------------------------------------
//* Function Name       : DebugMon_Handler
//* Object              :
//* Object              :
//* Input Parameters    :
//* Output Parameters   :
//* Functions called    :
//*----------------------------------------------------------------------------
void DebugMon_Handler(void)
{
	CriticalError(7);
}

//*----------------------------------------------------------------------------
//* Function Name       :
//* Object              :
//* Object              :
//* Input Parameters    :
//* Output Parameters   :
//* Functions called    :
//*----------------------------------------------------------------------------
void SysTick_Handler(void)
{
#if 0
	static ulong skip = 0;

	skip++;
	if(skip > 1000)
	{
		BSP_LED_Toggle(LED_BLUE);
		skip  = 0;
	}
#endif

	HAL_IncTick();
}

//*----------------------------------------------------------------------------
//* Function Name       : core_hw_init
//* Object              :
//* Object              : modified HAL_Init();
//* Input Parameters    :
//* Output Parameters   :
//* Functions called    :
//*----------------------------------------------------------------------------
static void core_hw_init(void)
{
	uint32_t common_system_clock;

   // Set Interrupt Group Priority
   HAL_NVIC_SetPriorityGrouping(NVIC_PRIORITYGROUP_4);

   // Update the SystemCoreClock global variable
   common_system_clock = HAL_RCC_GetSysClockFreq() >> ((D1CorePrescTable[(RCC->D1CFGR & RCC_D1CFGR_D1CPRE)>> RCC_D1CFGR_D1CPRE_Pos]) & 0x1FU);
   SystemD2Clock = (common_system_clock >> ((D1CorePrescTable[(RCC->D1CFGR & RCC_D1CFGR_HPRE)>> RCC_D1CFGR_HPRE_Pos]) & 0x1FU));
   SystemCoreClock = SystemD2Clock;

   // Use systick as time base source and configure 1ms tick (default clock after Reset is HSI)
   if(HAL_InitTick(TICK_INT_PRIORITY) != HAL_OK)
	   CriticalError(8);
}

#if 1
//*----------------------------------------------------------------------------
//* Function Name       : go_to_sleep
//* Object              :
//* Object              : enter sleep mode and wait IRQ notification
//* Input Parameters    :
//* Output Parameters   :
//* Functions called    :
//*----------------------------------------------------------------------------
void go_to_sleep_a(void)
{
	// ---------------------------------------------------------------------------------------------
	// ---------------------------------------------------------------------------------------------
	// HW semaphore Clock enable
	__HAL_RCC_HSEM_CLK_ENABLE();

	// ---------------------------------------------------------------------------------------------
	// ---------------------------------------------------------------------------------------------
	// Activate HSEM notification for Cortex-M4
    HSEM_COMMON->IER |= __HAL_HSEM_SEMID_TO_MASK(HSEM_ID_0);

    // ---------------------------------------------------------------------------------------------
    // ---------------------------------------------------------------------------------------------
    // PWR Clear Pending Event
    __SEV ();
    __WFE ();

    // ---------------------------------------------------------------------------------------------
    // ---------------------------------------------------------------------------------------------
	// Domain D2 goes to STOP mode (Cortex-M4 in deep-sleep) waiting for Cortex-M7 to
	// perform system initialisation (system clock config, external memory configuration.. )
    //
    MODIFY_REG (PWR->CR1, PWR_CR1_LPDS, PWR_MAINREGULATOR_ON);	// Select the regulator state in Stop mode
    CLEAR_BIT (PWR->CPUCR, PWR_CPUCR_PDDS_D2);					// Keep DSTOP mode when D2 domain enters Deepsleep
    SET_BIT (SCB->SCR, SCB_SCR_SLEEPDEEP_Msk); 					// Set SLEEPDEEP bit of Cortex System Control Register
    __DSB ();													// Ensure that all instructions are done before entering STOP mode
    __ISB ();
    __WFE ();													// Request Wait For Event
    CLEAR_BIT (SCB->SCR, SCB_SCR_SLEEPDEEP_Msk); 				// Clear SLEEPDEEP bit of Cortex-M in the System Control Register

    // ---------------------------------------------------------------------------------------------
    // ---------------------------------------------------------------------------------------------
	// Clear Flags generated during the wakeup notification
	HSEM_COMMON->ICR |= ((uint32_t)__HAL_HSEM_SEMID_TO_MASK(HSEM_ID_0));

	// ---------------------------------------------------------------------------------------------
	// ---------------------------------------------------------------------------------------------
	__NVIC_ClearPendingIRQ(HSEM2_IRQn);
}
#endif

#if 0
void EXTI15_10_IRQHandler(void)
{
	if (__HAL_GPIO_EXTID2_GET_IT(GPIO_PIN_12) != 0x00U)
	{
		//printf("dah irq\r\n");

		if((!LL_GPIO_IsInputPinSet(PADDLE_DAH_PIO,PADDLE_DAH))||(ps.virtual_dah_down))
		{
			if(ts.dmod_mode == DEMOD_CW)
			{
				cw_gen_dah_IRQ();
			}
			else if((ts.dmod_mode == DEMOD_USB)||(ts.dmod_mode == DEMOD_LSB) || (ts.dmod_mode == DEMOD_AM) || (ts.dmod_mode == DEMOD_FM))
			{
				//printf("ptt on\r\n");
				ts.ptt_req = 1;
			}
		}

		__HAL_GPIO_EXTID2_CLEAR_IT(GPIO_PIN_12);
	}

	if (__HAL_GPIO_EXTID2_GET_IT(GPIO_PIN_13) != 0x00U)
	{
		//printf("dit irq\r\n");

		if((!LL_GPIO_IsInputPinSet(PADDLE_DIT_PIO,PADDLE_DIT))||(ps.virtual_dit_down))
		{
			if(ts.dmod_mode == DEMOD_CW)
			{
				cw_gen_dit_IRQ();
			}
		}

		__HAL_GPIO_EXTID2_CLEAR_IT(GPIO_PIN_13);
	}
}
#else
void EXTI2_IRQHandler(void)
{
	if(__HAL_GPIO_EXTID2_GET_IT(PADDLE_DIT_PIN) != 0x00U)
	{
		//printf("dit irq\r\n");

		if((!LL_GPIO_IsInputPinSet(PADDLE_DIT_PORT, PADDLE_DIT_PIN))||(ps.virtual_dit_down))
		{
			if(ts.dmod_mode == DEMOD_CW)
			{
				cw_gen_dit_IRQ();
			}
		}

		__HAL_GPIO_EXTID2_CLEAR_IT(PADDLE_DIT_PIN);
	}
}
void EXTI3_IRQHandler(void)
{
	if (__HAL_GPIO_EXTID2_GET_IT(PADDLE_DAH) != 0x00U)
	{
		//printf("dah irq\r\n");

		if((!LL_GPIO_IsInputPinSet(PADDLE_DAH_PIO, PADDLE_DAH_LL))||(ps.virtual_dah_down))
		{
			if(ts.dmod_mode == DEMOD_CW)
			{
				cw_gen_dah_IRQ();
			}
			else if((ts.dmod_mode == DEMOD_USB)||(ts.dmod_mode == DEMOD_LSB) || (ts.dmod_mode == DEMOD_AM) || (ts.dmod_mode == DEMOD_FM))
			{
				//printf("ptt on\r\n");
				ts.ptt_req = 1;
			}
		}

		__HAL_GPIO_EXTID2_CLEAR_IT(PADDLE_DAH);
	}
}
#endif

void set_cw_irq(void)
{
	LL_EXTI_InitTypeDef EXTI_InitStruct = {0};

	 /* Enable GPIO Clock (to be able to program the configuration registers) */
	// done by M7 ?
	LL_AHB4_GRP1_EnableClock(LL_AHB4_GRP1_PERIPH_GPIOG);

	HAL_NVIC_SetPriority((IRQn_Type)(EXTI2_IRQn), 2U, 0x00);
	HAL_NVIC_EnableIRQ	((IRQn_Type)(EXTI2_IRQn));

	HAL_NVIC_SetPriority((IRQn_Type)(EXTI3_IRQn), 2U, 0x00);
	HAL_NVIC_EnableIRQ	((IRQn_Type)(EXTI3_IRQn));
}

void loc_delay(ulong ms)
{
	ulong i,j;

	for(i = 0; i < ms; i++)
	{
		for(j = 0; j < 5000; j++)
		{
			//
		}
	}
}

//*----------------------------------------------------------------------------
//* Function Name       : main
//* Object              :
//* Object              : dsp code entry
//* Input Parameters    :
//* Output Parameters   :
//* Functions called    :
//*----------------------------------------------------------------------------
int main(void)
{
	//__HAL_ART_CONFIG_BASE_ADDRESS(D2_AXISRAM_BASE);

    // Sleep until MCU notification (now in .s file)
    //go_to_sleep_a();

    // HAL init
	//core_hw_init();
	//HAL_Init();

	// HW semaphore Clock enable
	__HAL_RCC_HSEM_CLK_ENABLE();

	// HW semaphore Notification enable
	HAL_HSEM_ActivateNotification(__HAL_HSEM_SEMID_TO_MASK(HSEM_ID_0));

	// When system initialization is finished, Cortex-M7 will release Cortex-M4  by means of
	//  HSEM notification
	HAL_PWREx_ClearPendingEvent();
	HAL_PWREx_EnterSTOPMode(PWR_MAINREGULATOR_ON, PWR_STOPENTRY_WFE, PWR_D2_DOMAIN);

	// Initialize HAL : systick
	if (HAL_Init() != HAL_OK)
	{
		CriticalError(123);
	}

	// Clear Flags generated during the wakeup notification
	HSEM_COMMON->ICR |= ((uint32_t)__HAL_HSEM_SEMID_TO_MASK(HSEM_ID_0));
	HAL_NVIC_ClearPendingIRQ(HSEM2_IRQn);

    // Init debug print in shared mode
    printf_init(1);

	printf("-->%s v: %d.%d\r\n", DEVICE_STRING, MCHF_D_VER_RELEASE, MCHF_D_VER_BUILD);
	//printf("exec at 0x%08x\r\n", (int)&Reset_Handler);

	set_cw_irq();

	// ICC driver init
	icc_proc_hw_init();

	for(;;)
	{
		icc_proc_task(NULL);
		audio_driver_thread();
		ui_driver_thread();
	}
}
