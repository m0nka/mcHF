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
#ifndef __CW_GEN_H
#define __UI_GEN_H

#include "math.h"
#include "arm_math.h"

#include "mchf_pro_pinmap.h"

// Break timeout on straight key
#define CW_BREAK			800

// States
#define	CW_IDLE				0
#define	CW_DIT_CHECK		1
#define	CW_DAH_CHECK		3
#define	CW_KEY_DOWN			4
#define	CW_KEY_UP			5
#define	CW_PAUSE			6
#define	CW_WAIT				7

#define CW_DIT_L      		0x01
#define CW_DAH_L      		0x02
#define CW_DIT_PROC   		0x04

#define CW_IAMBIC_A    		0x00
#define CW_IAMBIC_B    		0x10

#define CW_SMOOTH_LEN		16
//

//#define PADDLE_DAH_PIO 			GPIOG
//#define PADDLE_DAH				LL_GPIO_PIN_12

//#define PADDLE_DIT_PIO			GPIOG
//#define PADDLE_DIT				LL_GPIO_PIN_13

//
typedef struct PaddleState
{
	// State machine and port states
	ulong 	port_state;
	ulong	cw_state;

	// Smallest element duration
	ulong   dit_time;

	// Timers
	ulong   key_timer;
	ulong 	break_timer;

	// Key clicks smoothing table current ptr
	ulong	sm_tbl_ptr;

	uchar 	virtual_dah_down;
	uchar 	virtual_dit_down;

} PaddleState;

// Exports
uchar cw_gen_get_line_state(GPIO_TypeDef *GPIOx, uint32_t PinMask);
void 	cw_gen_init(void);
ulong	cw_gen_process(float32_t *i_buffer,float32_t *q_buffer,ulong size);
ulong 	cw_gen_process_strk(float32_t *i_buffer,float32_t *q_buffer,ulong size);
ulong 	cw_gen_process_iamb(float32_t *i_buffer,float32_t *q_buffer,ulong size);

void 	cw_gen_dah_IRQ(void);
void 	cw_gen_dit_IRQ(void);

#endif
