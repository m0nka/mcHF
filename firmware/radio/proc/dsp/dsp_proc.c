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
#include "main.h"
#include "mchf_pro_board.h"

#ifdef CONTEXT_DSP

#include "dsp_proc.h"

#include "digi\ft8_lib-master\gen_ft8.h"
#include "digi\ft8_lib-master\decode_ft8.h"

// Driver communication
extern 			osMessageQId 			hDspMessage;

//*----------------------------------------------------------------------------
//* Function Name       : dsp_proc_hw_init
//* Object              :
//* Notes    			:
//* Notes   			:
//* Notes    			:
//* Context    			: CONTEXT_DSP
//*----------------------------------------------------------------------------
static void dsp_proc_hw_init(void)
{
	// nothing ?
}

//*----------------------------------------------------------------------------
//* Function Name       : dsp_proc_msg_parser
//* Object              :
//* Notes    			:
//* Notes   			:
//* Notes    			:
//* Context    			: CONTEXT_DSP
//*----------------------------------------------------------------------------
static void dsp_proc_msg_parser(void)
{
	osEvent 			event;
	struct DSPMessage 	*dsp_msg;

	// Wait for a short time for pending messages
	event = osMessageGet(hDspMessage, 150);		// second arg is driver idle time in mS
	if(event.status != osEventMessage)
		return;

	// Check status type
	if(event.status != osEventMessage)
		return;

	dsp_msg = (struct DSPMessage *)event.value.p;

	// Process menu calls
	switch(dsp_msg->ucMessageID)
	{
		// Encode FT8
		case 1:
		{
			//char 	bufa[40];
			//strcpy(bufa,"CQ M0NKA IO92");

			// ToDo: strxxx checks on passed buffer ??

			encode_ft8_message(dsp_msg->cData,1);

			// Signal UI driver
			dsp_msg->ucProcessDone = 1;

			break;
		}

		// Decode FT8
		case 2:
		{
			char 	buf[40];
			uchar 	i;

			//printf("menu 1 called\r\n");

//!			decode_ft8_message(buf);
			//printf("decoded: %s\r\n",buf);

			// unsafe strcpy...
			for(i = 0; i < 50; i++)
			{
				if(buf[i] == 0) break;
				dsp_msg->cData[i] = buf[i];
			}
			if(i) dsp_msg->ucDataReady = 1;

			break;
		}

		default:
			break;
	}
}

//*----------------------------------------------------------------------------
//* Function Name       : dsp_proc_task
//* Object              :
//* Notes    			:
//* Notes   			:
//* Notes    			:
//* Context    			: CONTEXT_DSP
//*----------------------------------------------------------------------------
void dsp_proc_task(void const * argument)
{
	vTaskDelay(5000);
	printf("dsp proc start\r\n");

	dsp_proc_hw_init();

dsp_proc_loop:
	dsp_proc_msg_parser();
	goto dsp_proc_loop;
}

#endif
