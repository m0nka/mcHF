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

static void ui_tests_show_boundary(void)
{

}

void ui_tests_run_all(void)
{
	int i;

	GUI_SetColor(GUI_RED);

	// Top left
	for(i = 0; i < 5; i++)
	{
		GUI_DrawHLine((i + 1), 1, 50);
		GUI_DrawVLine((i + 1), 1, 50);
	}

	// Bottom left
	for(i = 0; i < 5; i++)
	{
		GUI_DrawHLine((479 - i), 1, 50);
		//GUI_DrawVLine((479 - i), 1, 50);
	}

}

void ui_tests_init(void)
{
	//GUI_SelectLayer(1);
	GUI_SetFont(&GUI_Font24B_ASCII);
	GUI_SetColor(GUI_WHITE);
	GUI_DispStringAt("unit test", 700, 450);

	ui_tests_show_boundary();

	#ifdef RECT_COPY_TEST
	//ui_tests_dma_copy_test();
	#endif
}


#endif
