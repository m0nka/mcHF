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
#include "main.h"
#include "mchf_pro_board.h"
#include "mchf_icc_def.h"

#include <limits.h>

#include "touch_proc.h"

#ifdef CONTEXT_TOUCH

#ifdef BOARD_MCHF_PRO
#include "touch_i2c.h"
#endif

extern TaskHandle_t hTouchTask;
extern uchar lcd_touch_reset_done;

uchar tp_init_done = 0;

//typedef void (* BSP_EXTI_LineCallback) (void);
EXTI_HandleTypeDef hts_exti_[2] = {0};

void EXTI9_5_IRQHandler(void)
{
	if (__HAL_GPIO_EXTI_GET_IT(TS_INT_PIN) != 0x00U)
	{
	    if(tp_init_done)
	    {
	    	BaseType_t xHigherPriorityTaskWoken;
	    	xHigherPriorityTaskWoken = pdFALSE;
	    	xTaskNotifyFromISR(hTouchTask, 0x01, eSetBits, &xHigherPriorityTaskWoken );
	    	portYIELD_FROM_ISR(xHigherPriorityTaskWoken );
	    }
	    __HAL_GPIO_EXTI_CLEAR_IT(TS_INT_PIN);
	 }
}

static void touch_proc_irq_setup(void)
{
	GPIO_InitTypeDef gpio_init_structure;

	gpio_init_structure.Pin 	= TS_INT_PIN;
	gpio_init_structure.Pull 	= GPIO_PULLDOWN;
	gpio_init_structure.Speed 	= GPIO_SPEED_FREQ_VERY_HIGH;
	gpio_init_structure.Mode 	= GPIO_MODE_IT_RISING;
	HAL_GPIO_Init(TS_INT_GPIO_PORT, &gpio_init_structure);

	HAL_EXTI_GetHandle(&hts_exti_[1],EXTI_LINE_6);

	HAL_NVIC_SetPriority(EXTI9_5_IRQn, 15U, 0x00);
	HAL_NVIC_EnableIRQ  (EXTI9_5_IRQn);
}

static int touch_proc_read_td(struct GT911_TS *ts)
{
	uchar 	data[100];
	uchar 	touch_no = 0;
	uchar	clear_reg = 0;

	if(ts == NULL)
		return 1;

	// Read status
	if(BSP_I2C4_ReadReg16(GT911_I2C_ADDRESS, 0x814E, data, 1) != 0)
		return 2;

	ts->status = data[0];
	//printf("stat: %x\r\n", data[0]);

	if((data[0] & 0x80) == 0x00)
	{
		BSP_I2C4_WriteReg16(GT911_I2C_ADDRESS, 0x814E, &clear_reg, 1);
		vTaskDelay(10);	// debounce ?
		return 0;
	}

	touch_no = data[0] & 0x0F;

	if(touch_no == 0)
	{
		BSP_I2C4_WriteReg16(GT911_I2C_ADDRESS, 0x814E, &clear_reg, 1);
		return 0;
	}

	if(touch_no > 5)
	{
		BSP_I2C4_WriteReg16(GT911_I2C_ADDRESS, 0x814E, &clear_reg, 1);
		return 5;
	}

	// Read data
	if(BSP_I2C4_ReadReg16(GT911_I2C_ADDRESS, 0x814F, data + 1, (touch_no * 8)) != 0)
		return 6;

	BSP_I2C4_WriteReg16(GT911_I2C_ADDRESS, 0x814E, &clear_reg, 1);

	//print_hex_array(data, 10);
	memcpy(ts, data, sizeof(struct GT911_TS));

	return 0;
}

static int touch_proc_gt911_init(void)
{
	uchar data[100];
	ulong i2c_err;

	// Read mode(swapped return FW ID)
	i2c_err = BSP_I2C4_ReadReg(GT911_I2C_ADDRESS, 0x4880, data, 16);
	if(i2c_err != 0)
	{
		printf("gt911 init err %d\r\n", i2c_err);
		return 1;
	}
	//printf("FW: %s\r\n", (char *)data);

	// Read mode (which is correct ?)
	// ff 00 00 00 00 00 00 41 BSP_I2C4_ReadReg16
	// 66 00 00 00 00 07 00 00 BSP_I2C4_ReadReg
	if(BSP_I2C4_ReadReg16(GT911_I2C_ADDRESS, 0x8040, data, 8) != 0)
	{
		printf("gt911 init err2!\r\n");
		return 1;
	}
	//printf("mode reg:\r\n");
	//print_hex_array(data, 8);

	// Read configuration
	if(BSP_I2C4_ReadReg16(GT911_I2C_ADDRESS, 0x8048, data, 13) != 0)
	{
		printf("gt911 init err3!\r\n");
		return 1;
	}
	//printf("config reg:\r\n");
	//print_hex_array(data, 13);

	struct GT911_CONFIG *gtc = (struct GT911_CONFIG *)data;
	//printf("x size %d, y size %d\r\n", gtc->x_max, gtc->y_max);

	#if 0
	printf("numtouch_max %d, switch1 %d switch2 %d shake_count %d filter %X\r\n",
				gtc->numtouch_max,
				gtc->switch1,
				gtc->switch2,
				gtc->shake_count,
				gtc->filter);

	printf(" large_touch %d nr %d touch level %d leave level %d\r\n",
				gtc->large_touch,
				gtc->noise_reduction,
				gtc->touch_level,
				gtc->leave_level);
	#endif

	return 0;
}

// LCD controller needs to be initialised
// but before Touch process init, as INT
// pin on the GT911 is output during reset!
//
// 0 - reset
// 1 - OS
static void touch_proc_lcd_reset(uchar context)
{
	GPIO_InitTypeDef  gpio_init_structure;

	gpio_init_structure.Mode  = GPIO_MODE_OUTPUT_PP;
	gpio_init_structure.Speed = GPIO_SPEED_FREQ_LOW;

	// Reset output
	gpio_init_structure.Pin   = LCD_RESET_PIN;
	gpio_init_structure.Pull  = LCD_RESET_PULL;
	HAL_GPIO_Init(LCD_RESET_GPIO_PORT, &gpio_init_structure);

	// INT as output
	gpio_init_structure.Pin   = TS_INT_PIN;
	gpio_init_structure.Pull  = GPIO_PULLUP;
	HAL_GPIO_Init(TS_INT_GPIO_PORT, &gpio_init_structure);

	// Reset high, INT low
	HAL_GPIO_WritePin(LCD_RESET_GPIO_PORT, 	LCD_RESET_PIN, 	GPIO_PIN_SET);
	HAL_GPIO_WritePin(TS_INT_GPIO_PORT, 	TS_INT_PIN, 	GPIO_PIN_RESET);

	if(!context)
		HAL_Delay(20);
	else
		vTaskDelay(20);

	HAL_GPIO_WritePin(LCD_RESET_GPIO_PORT, LCD_RESET_PIN, GPIO_PIN_RESET);

	if(!context)
		HAL_Delay(20);
	else
		vTaskDelay(20);

	#if GT911_I2C_ADDRESS == (0x28>>1)
	HAL_Delay(1);
	HAL_GPIO_WritePin(GPIOH, GPIO_PIN_6, GPIO_PIN_SET);
	HAL_Delay(1);
	#endif

	HAL_GPIO_WritePin(LCD_RESET_GPIO_PORT, LCD_RESET_PIN, GPIO_PIN_SET);

	if(!context)
		HAL_Delay(150);
	else
		vTaskDelay(150);

	HAL_GPIO_WritePin(TS_INT_GPIO_PORT, TS_INT_PIN, GPIO_PIN_RESET);

	if(!context)
		HAL_Delay(150);
	else
		vTaskDelay(150);

	// INT pin as input
	gpio_init_structure.Pin   = TS_INT_PIN;
	gpio_init_structure.Mode  = GPIO_MODE_INPUT;
	gpio_init_structure.Pull  = GPIO_NOPULL;
	HAL_GPIO_Init(TS_INT_GPIO_PORT, &gpio_init_structure);

	lcd_touch_reset_done = 1;
}

//
// CONTEXT_RESET !!!!!!!
//
void touch_proc_hw_init(void)
{
	// Combined reset only
	touch_proc_lcd_reset(0);
}

void touch_proc_power_cleanup(void)
{
	// Need to send sleep command...
}

static void touch_proc_post_init(void)
{
	// I2C init
	if(BSP_I2C4_Init() != 0)
	{
		printf("i2c init err!\r\n");
		return;
	}

	// GT911 init
	if(touch_proc_gt911_init() != 0)
	{
		//printf("gt911 init err!\r\n");
		return;
	}

	// Set up interrupt
	touch_proc_irq_setup();

	//printf("touch ok\r\n");

	// Allow process
	tp_init_done = 1;
}

#if 0
//
// with static touch state structure
//
static void touch_proc_ts_update_a(struct GT911_TS ts_g)
{
	static GUI_PID_STATE TS_State = {0, 0, 0, 0};
	__IO TS_State_t  ts;
	uint16_t xDiff, yDiff;

	//printf("---------------------------------------------------------\r\n");
	//printf("TD: s: %x - x:%d, y:%d\r\n", ts_g.status, ts_g.t[0].x, ts_g.t[0].y);

	if(ts_g.status == 0x00)
		return;

	if((ts_g.t[0].x > 854)||(ts_g.t[0].y > 854))
		return;

	if(ts_g.status == 0x81)
	{
		ts.TouchDetected = 1;
		ts.TouchX = ts_g.t[0].y;
		ts.TouchY = (480 - ts_g.t[0].x);
	}
	else
	{
		ts.TouchDetected = 0;
		ts.TouchX = TS_State.x;
		ts.TouchY =	TS_State.y;
	}

	xDiff = (TS_State.x > ts.TouchX) ? (TS_State.x - ts.TouchX) : (ts.TouchX - TS_State.x);
	yDiff = (TS_State.y > ts.TouchY) ? (TS_State.y - ts.TouchY) : (ts.TouchY - TS_State.y);
	//printf("TD: x_diff: %d - y_diff:%d, detected: %d\r\n", xDiff, yDiff, ts.TouchDetected);

	if((TS_State.Pressed != ts.TouchDetected)||(xDiff > 8 )||(yDiff > 8))
	{
		TS_State.Pressed = ts.TouchDetected;
		TS_State.Layer = 0;

		// Further it is quite important to generate the "up event" (StatePID.Pressed = 0)
		// at the same position as the last down event. Otherwise it a button might get
		// just a "leave' message instead of a release message.
		if(ts.TouchDetected)
		{
			TS_State.x = ts.TouchX;
			TS_State.y = ts.TouchY ;

			//printf("TD: x:%d, y:%d\r\n", TS_State.x, TS_State.y);
			GUI_TOUCH_StoreStateEx(&TS_State);
		}
		else
		{
			//printf("TD: release\r\n");
			GUI_TOUCH_StoreStateEx(&TS_State);
			TS_State.x = 0;
			TS_State.y = 0;
		}
	}
}
#else
//
// use GT911 touch data as state (drag as well)
//
static void touch_proc_ts_update_a(struct GT911_TS ts_g)
{
	GUI_PID_STATE TS_State;

	//printf("---------------------------------------------------------\r\n");
	//printf("TD: s: %x - x:%d, y:%d\r\n", ts_g.status, ts_g.t[0].x, ts_g.t[0].y);

	// Not suppose to have this - low level read is bad ?
	if(ts_g.status == 0x00)
		return;

	// Overflow protect from garbage on the bus
	if((ts_g.t[0].x > 854)||(ts_g.t[0].y > 854))
		return;

	// Swap coordinates
	TS_State.x 		= (854 - ts_g.t[0].y);
	TS_State.y 		= ts_g.t[0].x;
	TS_State.Layer 	= 0;

	//printf("TD: x:%d, y:%d\r\n", TS_State.x, TS_State.y);

	if(ts_g.status == 0x81)
		TS_State.Pressed = 1;
	else
		TS_State.Pressed = 0;

	GUI_TOUCH_StoreStateEx(&TS_State);
}
#endif

void touch_proc_task(void const *argument)
{
	ulong 	ulNotificationValue = 0, ulNotif;

	vTaskDelay(TOUCH_PROC_START_DELAY);
	//printf("touch proc start\r\n");

	// Interface init
	touch_proc_post_init();

touch_proc:

	ulNotif = xTaskNotifyWait(0x00, ULONG_MAX, &ulNotificationValue, TOUCH_PROC_SLEEP_TIME);
	if((tp_init_done)&&(ulNotif) && (ulNotificationValue))
	{
		struct GT911_TS ts;

		uchar err = touch_proc_read_td(&ts);
		if(err == 0)
		{
			// Push to UI driver
			touch_proc_ts_update_a(ts);
		}
		//else
		//	printf("err: %x\r\n", err);
	}

	goto touch_proc;
}
#endif

