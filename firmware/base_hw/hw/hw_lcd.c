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

#include "stm32h7xx_hal.h"
#include "hw_lcd.h"

void hw_lcd_gpio_init(void)
{
	GPIO_InitTypeDef  gpio_init_structure;

	gpio_init_structure.Pin       = LCD_BL_CTRL_PIN;
	gpio_init_structure.Mode      = GPIO_MODE_OUTPUT_PP;
	gpio_init_structure.Speed     = GPIO_SPEED_FREQ_LOW;
	HAL_GPIO_Init(LCD_BL_CTRL_GPIO_PORT, &gpio_init_structure);

	/* Assert back-light LCD_BL_CTRL pin */
	//HAL_GPIO_WritePin(LCD_BL_CTRL_GPIO_PORT, LCD_BL_CTRL_PIN, GPIO_PIN_SET);

	/* Configure the GPIO on PG3 */
	gpio_init_structure.Mode  = GPIO_MODE_OUTPUT_PP;
	gpio_init_structure.Pull  = GPIO_PULLUP;
	gpio_init_structure.Speed = GPIO_SPEED_FREQ_LOW;

	#ifdef BOARD_EVAL_747
	gpio_init_structure.Pin   = GPIO_PIN_3;
	HAL_GPIO_Init(GPIOG, &gpio_init_structure);
	#endif

	#ifdef BOARD_MCHF_PRO
	gpio_init_structure.Pin   = GPIO_PIN_7;
	HAL_GPIO_Init(GPIOH, &gpio_init_structure);
	#endif
}

void hw_lcd_reset(void)
{
	GPIO_InitTypeDef  gpio_init_structure;

	/* Activate XRES active low */
	#ifdef BOARD_EVAL_747
	HAL_GPIO_WritePin(GPIOG, GPIO_PIN_3, GPIO_PIN_RESET);
	#endif
	#ifdef BOARD_MCHF_PRO
	HAL_GPIO_WritePin(GPIOH, GPIO_PIN_7, GPIO_PIN_RESET);
	#endif

	HAL_Delay(20); /* wait 20 ms */

	/* Desactivate XRES */
	#ifdef BOARD_EVAL_747
	HAL_GPIO_WritePin(GPIOG, GPIO_PIN_3, GPIO_PIN_SET);
	#endif
	#ifdef BOARD_MCHF_PRO
	HAL_GPIO_WritePin(GPIOH, GPIO_PIN_7, GPIO_PIN_SET);
	#endif

	/* Wait for 300 ms after releasing XRES before sending commands */
	HAL_Delay(300);
}
