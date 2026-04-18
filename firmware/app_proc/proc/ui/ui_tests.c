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

#if defined (CONTEXT_VIDEO) && defined (UI_RUN_ALL_TESTS)

#include "gui.h"
#include "ui_tests.h"

static void ui_tests_dma_copy_test(void)
{
	int x, y;

	x = 5;
	y = 250;

	GUI_SetColor(GUI_RED);
	GUI_FillRect(	(x		  ),
				(y		  ),
				(x + 100 ),
				(y + 50 )
	);

	GUI_CopyRect(x,			// Upper left X-position of the source rectangle.
			 y,	    	// Upper left Y-position of the source rectangle.
			 x,			// Upper left X-position of the destination rectangle.
			 y + 100,	// Upper left Y-position of the destination rectangle.
			 100,		// X-size of the rectangle.
			 55);		// Y-size of the rectangle.

	//GUI_CopyRect(x,			// Upper left X-position of the source rectangle.
	//			 5,	    	// Upper left Y-position of the source rectangle.
	//			 x,			// Upper left X-position of the destination rectangle.
	//			 y + 200,	// Upper left Y-position of the destination rectangle.
	//			 100,		// X-size of the rectangle.
	//			 55);		// Y-size of the rectangle.

	#if 0
	GUI_SetColor(GUI_BLUE);
	GUI_FillRect(	(300		  ),
				(5		  ),
				(300 + 100 ),
				(5 + 50 )
	);

	GUI_CopyRect(300,	// Upper left X-position of the source rectangle.
			 5,	    // Upper left Y-position of the source rectangle.
			 300,	// Upper left X-position of the destination rectangle.
			 100,	// Upper left Y-position of the destination rectangle.
			 100,	// X-size of the rectangle.
			 55);	// Y-size of the rectangle.

	GUI_CopyRect(300,	// Upper left X-position of the source rectangle.
			 5,	    // Upper left Y-position of the source rectangle.
			 300,	// Upper left X-position of the destination rectangle.
			 200,	// Upper left Y-position of the destination rectangle.
			 100,	// X-size of the rectangle.
			 55);	// Y-size of the rectangle.
	#endif
}

//*----------------------------------------------------------------------------
//* Function Name       : ui_tests_show_boundary
//* Object              :
//* Input Parameters    : Create frame, to judge LTDC timing parameters
//* Output Parameters   :
//* Functions called    : CONTEXT_VIDEO
//*----------------------------------------------------------------------------
static void ui_tests_show_boundary(void)
{
	int i;

	GUI_SetColor(GUI_WHITE);

	// Top left
	for(i = 0; i < 5; i++)
	{
		GUI_DrawHLine((i + 0), 0, 49);
		GUI_DrawVLine((i + 0), 0, 49);
	}

	// Bottom left
	for(i = 0; i < 5; i++)
	{
		GUI_DrawHLine((479 - i),   0,  49);
		GUI_DrawVLine((i   + 0), 429, 479);
	}

	// Give it chance to show overflow before displaying
	// the right side of the frame
	GUI_Delay(3000);

	// Top right
	for(i = 0; i < 5; i++)
	{
		GUI_DrawHLine((i   + 0), 749, 799);
		GUI_DrawVLine((799 - i),   0,  49);
	}

	// Bottom right
	for(i = 0; i < 5; i++)
	{
		GUI_DrawHLine((479 - i), 749, 799);
		GUI_DrawVLine((799 - i), 429, 479);
	}
}

//*----------------------------------------------------------------------------
//* Function Name       : ui_tests_run_all
//* Object              :
//* Input Parameters    : repetitive paint
//* Output Parameters   :
//* Functions called    : CONTEXT_VIDEO
//*----------------------------------------------------------------------------
void ui_tests_run_all(void)
{
	//
}

//*----------------------------------------------------------------------------
//* Function Name       : ui_tests_init
//* Object              :
//* Input Parameters    : single run, on start
//* Output Parameters   :
//* Functions called    : CONTEXT_VIDEO
//*----------------------------------------------------------------------------
void ui_tests_init(void)
{
	//GUI_SelectLayer(1);
	GUI_SetFont(&GUI_Font24B_ASCII);
	GUI_SetColor(GUI_LIGHTGREEN);
	GUI_DispStringAt("ui lcd test", 688, 445);

	// Show screen frame
	ui_tests_show_boundary();

	#ifdef RECT_COPY_TEST
	ui_tests_dma_copy_test();
	#endif
}

#endif
