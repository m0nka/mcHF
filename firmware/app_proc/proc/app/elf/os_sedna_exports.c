/************************************************************************************
**                                                                                 **
**                          GENIE RTOS AT91 IMPLEMENTATION                         **
**                                  B-phreaks, 2005                                **
**                                                                                 **
**---------------------------------------------------------------------------------**
**                                                                                 **
**  File name:                                                                     **
**  Description:                                                                   **
**  Last Modified:                                                                 **
**                                                                                 **
************************************************************************************/
#include "main.h"
#include "mchf_pro_board.h"

#ifdef CONTEXT_APP

#include "os_apploader.h"
#include "os_sedna_exports.h"

extern APPLOADER_QUEUE_PARAMETERS 	*pxAppLdrParametersPub;

static uchar ProcessUI	 (uchar ucMenu,MENU_DEF *pmenu,MENU_DEF *popts,uchar ucItemsCount);
static void  DrawMenu    (uchar ucSelection,uchar ucMenuCount,PMENU_DEF menu);
static void  SelectMenu  (uchar ucButtonID,uchar *ucSelection,uchar ucMaxCount);
static void  UpdateOption(uchar ucSelection,PMENU_DEF menu);

uchar ucMenuStatus 	  		= 0; // Menu status - Up/Down
uchar ucMenuSelection 		= 0; // Menu scroll selection

uchar ucOptionsStatus 		= 0; // Options status - Up/Down
uchar ucOptionsSelection 	= 0; // Options scroll selection

//*--------------------------------------------------------------------------------------
//* Function Name       : SednaWaitMessage
//* Object              : Read pending messages sent by the OS to this application
//* Input Parameters    : Rx Queue ptr and items buffer
//* Output Parameters   : none.
//*--------------------------------------------------------------------------------------
uchar SednaWaitMessage(void *pRxQueue,ulong *ulQueueBuffer)
{
	uchar ucNext = 0;
#if 0
	*ulQueueBuffer = 0;	
	while( ucQueueMessagesWaiting( pRxQueue ) )
	{			
		if(cQueueReceive( pRxQueue, (ulQueueBuffer + ucNext),0) == 1)
		{
			ucNext++;									
		}
	}
#endif
	return ucNext;
}

//*--------------------------------------------------------------------------------------
//* Function Name       : SednaClearTxQueue
//* Object              : Clear any items in the Tx queue that were not cleared by the OS
//* Input Parameters    : Tx Queue ptr
//* Output Parameters   : none.
//*--------------------------------------------------------------------------------------
void SednaClearTxQueue(void *pTxQueue)
{
	ulong ulDummy;
#if 0
	/* Clear Tx Queue before posting */
	while( ucQueueMessagesWaiting( pTxQueue ) )
	{
		cQueueReceive(pTxQueue, &ulDummy, 0 );																
	}
#endif
}

//*--------------------------------------------------------------------------------------
//* Function Name       : SednaSendMessage
//* Object              : Post application message back to the OS
//* Input Parameters    : Tx Queue ptr and items buffer
//* Output Parameters   : none.
//*--------------------------------------------------------------------------------------
void SednaSendMessage(void *pTxQueue,ulong *ulQueueBuffer)
{
	ulong ulDummy;
#if 0
	SednaClearTxQueue( pTxQueue );
					
	/* Insert the function result */
	ulDummy = *ulQueueBuffer++;
				   
	/* Send */
	cQueueSend( pTxQueue, ( void * ) &ulDummy, 0);
				
	/* Insert the return buffer ptr */
	ulDummy = *ulQueueBuffer++;
				   
	/* Send */
	cQueueSend( pTxQueue, ( void * ) &ulDummy, 0);
	
	/* Insert the return buffer size */
	ulDummy = *ulQueueBuffer++;
				   
	/* Send */
	cQueueSend( pTxQueue, ( void * ) &ulDummy, 0);
#endif
}

//*----------------------------------------------------------------------------
//* Function Name       : ucSednaExportsSendQueuedMessageA
//* Object              : Send message to queue
//* Input Parameters    : none
//* Output Parameters   : none
//* Functions called    : none
//*----------------------------------------------------------------------------
static uchar ucSednaExportsSendQueuedMessageA(xQueueHandle pvQueueHandle, ulong *ulMessageBuffer, uchar ucNumberOfItems)
{
	ulong ulDummy;
	uchar ucCount;
#if 0
	/* Clear Rx Queue before posting */
	while( ucQueueMessagesWaiting(pvQueueHandle))
	{
		cQueueReceive(pvQueueHandle,(void *)&ulDummy, ( portTickType ) 0 );																
	}
	
	/* Send all items */			
	for(ucCount = 0;ucCount < ucNumberOfItems;ucCount++)
	{		   
    	ulDummy = *ulMessageBuffer++;
				    
	    /* Insert the item */
		if( cQueueSend(pvQueueHandle, (void *)&ulDummy, ( portTickType ) 0 ) != pdPASS )
			return 1;
	}			    
#endif
	return 0;				    
}

//*--------------------------------------------------------------------------------------
//* Function Name       : ucSednaExportsSednaWaitMessageA
//* Object              : Read pending messages
//* Input Parameters    : Rx Queue ptr and items buffer
//* Output Parameters   : none.
//*--------------------------------------------------------------------------------------
static uchar ucSednaExportsSednaWaitMessageA(xQueueHandle pRxQueue,ulong *ulQueueBuffer)
{
	uchar ucNext = 0;
#if 0
	*ulQueueBuffer = 0;	
	while( ucQueueMessagesWaiting( pRxQueue ) )
	{			
		if(cQueueReceive( pRxQueue, (ulQueueBuffer + ucNext), ( portTickType ) 0 ) == pdPASS)
		{
			ucNext++;									
		}
	}
#endif
	return ucNext;
}

/* For Application access copy */
static void vSednaExportsMemCopyA(uchar *pDestBuffer, uchar *pSourceBuffer,uint nCpySize)
{
ulong    i;

    for(i=0;i<nCpySize;i++)
	{
		*pDestBuffer++ = *pSourceBuffer++;
	}

}

/* For Application access copy */
static void vSednaExportsMemSetA(uchar *pDestBuffer, uchar ucValue,uint nCpySize)
{
ulong    i;

    for(i=0;i<nCpySize;i++)
	{
		*pDestBuffer++ = ucValue;
	}
}

//*----------------------------------------------------------------------------
//* Function Name       : LcdDrawA
//* Object              : Local impl to fix stack corruption
//* Input Parameters    : 
//* Output Parameters   : 
//* Functions called    : none
//*----------------------------------------------------------------------------
#if 0
uchar LcdDrawA(uchar ucFuncId,X_DRAW_PARAMS *pDP)
{
	X_DRAW_PARAMS xdp;
	
	xdp.x1 			= pDP->x1;
	xdp.x2 			= pDP->x2;
	xdp.y1 			= pDP->y1;
	xdp.y2 			= pDP->y2;
	xdp.colour 		= pDP->colour;
	xdp.background 	= pDP->background;
	xdp.character 	= pDP->character;
	xdp.text 		= pDP->text;
	xdp.font 		= pDP->font;
	xdp.language 	= pDP->language;
	
	return LcdDraw(ucFuncId,&xdp);
}
#endif

//*----------------------------------------------------------------------------
//* Function Name       : SednaCreateScreenFrameX
//* Object              : moved from application
//* Input Parameters    : none
//* Output Parameters   : none
//* Functions called    : none
//*----------------------------------------------------------------------------
void SednaCreateAppSplashScreen(char *chDesc,uchar *ucVersion,ulong ulDelay)
{
#if 0
	char 			chTemp[50];
	X_DRAW_PARAMS 	dr;
	
	dr.language	= 0;
	
	dr.x1 			= 0;
	dr.x2 			= 0;
	dr.y1 			= 0;
	dr.y2 			= 0;
	LcdDrawA(DRAW_CLEAR_LCD,&dr);
	
	vTaskSuspendAll();	
	sprintf(chTemp,"%s %d.%d.%d.%d",chDesc,ucVersion[0],ucVersion[1],ucVersion[2],ucVersion[3]);
	cTaskResumeAll();
	
	/* Create splash screen */
	dr.x1 			= 10;
	dr.y1 			= 60;
	dr.colour 		= BLUE;
	dr.font			= 1;	
	dr.background 	= 0;
	dr.text		= chTemp;
	LcdDrawA(DRAW_PRINT_TXT,&dr);	
	
	/* Create splash screen delay */
	if(ulDelay)
		OsSleep(ulDelay);
#endif
}

//*----------------------------------------------------------------------------
//* Function Name       : SednaCreateScreenFrameX
//* Object              : moved from application
//* Input Parameters    : none
//* Output Parameters   : none
//* Functions called    : none
//*----------------------------------------------------------------------------
void SednaCreateScreenFrameX(char *chTopCaption,uchar *ucVersion)
{
#if 0
	char 			chTemp[50];
	X_DRAW_PARAMS 	dr;
	
	dr.language	= 0;
	
	if((chTopCaption) && (ucVersion))
	{
			
		dr.x1 			= 0;
		dr.x2 			= 0;
		dr.y1 			= 0;
		dr.y2 			= 0;
		LcdDrawA(DRAW_CLEAR_LCD,&dr);
		
		s_sprintf(chTemp,"%s %d.%d.%d.%d",chTopCaption,ucVersion[0],ucVersion[1],ucVersion[2],ucVersion[3]);	
	}
		
	// Create main frame 
	dr.x1 			= 1;
	dr.x2 			= 130;
	dr.y1 			= 1;
	dr.y2 			= 118;
	dr.colour 		= BLUE;
	LcdDrawA(DRAW_FRAME_BOX,&dr);	
	
	// Create buttons box 
	dr.x1 			= 1;
	dr.x2 			= 132;
	dr.y1 			= 118;
	dr.y2 			= 132;	
	LcdDrawA(DRAW_SOLID_BOX,&dr);
	
	// Create bottom line of top caption 
	dr.x1 			= 2;
	dr.x2 			= 130;
	dr.y1 			= 12;
	LcdDrawA(DRAW_HORZ_LINE,&dr);
	
	// Create Top Caption 
	if((chTopCaption) && (ucVersion))
	{
		dr.x1 			= 5;
		dr.y1 			= 4;
		dr.colour 		= BLUE;
		dr.font			= 1;	
		dr.background 	= 0;
		dr.text			= chTemp;
		LcdDrawA(DRAW_PRINT_TXT,&dr);	
	}
	
	// Create the Left button Caption 
	dr.x1 			= 6;
	dr.y1 			= 120;
	dr.colour 		= WHITE;
	dr.font			= 2;
	dr.background 	= NONE;
	dr.text			= " Menu  ";
	LcdDrawA(DRAW_PRINT_TXT,&dr);	
	
	// Create the Right button Caption 
	dr.x1 			= 90;
	dr.text			= " Options ";
	LcdDrawA(DRAW_PRINT_TXT,&dr);	
#endif
}

static uchar ProcessUI(uchar ucMenu,MENU_DEF *pmenu,MENU_DEF *popts,uchar ucItemsCount)
{
	uchar ucResult = 0;

	/* Keypad event processor */
	switch(ucMenu)
	{
		// Menu Up/Down
		case 1:
		{
			if(!ucOptionsStatus)
			{
				ucMenuStatus ^= 1; 
				DrawMenu(ucMenuStatus,((ucItemsCount >> 4) & 0xF),pmenu);
				
				// Select first menu
				if(ucMenuStatus)
				{
					ucMenuSelection = 0;								
					if(ucMenuStatus) SelectMenu(3,&ucMenuSelection,((ucItemsCount >> 4) & 0xF));
				}
			}
				
			break;
		}
			
		// Options Up/Down				
		case 2:
		{						
			if(!ucMenuStatus)
			{
				ucOptionsStatus ^= 1; 
				DrawMenu(ucOptionsStatus|0x80,(ucItemsCount & 0xF),popts);
				
				// Select first menu
				if(ucOptionsStatus)
				{
					ucOptionsSelection = 0;								
					if(ucOptionsStatus) SelectMenu(0x83,&ucOptionsSelection,(ucItemsCount & 0xF));
				}
			}
					
			break;
		}
			
		// Change Menu Selection
		case 3:
		case 4:
		{
			if(ucMenuStatus)    SelectMenu  (ucMenu,     &ucMenuSelection,   ((ucItemsCount >> 4) & 0xF));								
			if(ucOptionsStatus) SelectMenu  (ucMenu|0x80,&ucOptionsSelection,(ucItemsCount & 0xF));
			
			break;	
		}
			
		// Select Menu
		case 6:	
		{
			// Handle menu click
			if(ucMenuStatus)
			{			
				ucMenuStatus = 0;
		
				// Clear Menu
				DrawMenu(ucMenuStatus,((ucItemsCount >> 4) & 0xF),pmenu);
																																
				if((((ucItemsCount >> 4) & 0xF) - ucMenuSelection) == 0)
				{
					/* Quit application, default menu */							
					ucResult = 0xF8;
				}
				else
				{
					void 	(*pvMenuHandler)(void);
					uchar	ucCallIndex = (((ucItemsCount >> 4) & 0xF) - ucMenuSelection - 1);
																																	
					/* Menu click */
					if(pmenu[ucCallIndex].handler_ptr != NULL)
					{										
						pvMenuHandler = (void (*)(void))(pmenu[ucCallIndex].handler_ptr);																																						
						(pvMenuHandler)(); 
					}
				}
			}
							
			// Handle option click
			if(ucOptionsStatus)							
				UpdateOption((ucItemsCount & 0xF) - ucOptionsSelection - 1,popts);							
							
			break;
		}
			
		// Not handled	
		default:
			break;	
	}				
																									
	return ucResult;
}
	
// Show/Hide menu/options
static void DrawMenu(uchar ucSelection,uchar ucMenuCount,PMENU_DEF menu)
{
#if 0
	X_DRAW_PARAMS dr;	
				
	if(ucSelection & 0xF)
	{
		uchar i;
		
		/* Menu size/position */
		dr.x1 			=   5;
		dr.x2 			=  60;			
		dr.y1 			=  90 - (ucMenuCount * 10);
		dr.y2 			= 110;	
		dr.colour 		= BLUE;			
		
		if(ucSelection & 0x80)
		{
			dr.x1 += 72;
			dr.x2 += 67;
			dr.y1 += 10;			
		}
		
		// fix
		dr.x1++;
		dr.x2--;
		dr.y1++;
		dr.y2--;
		
		// Addon - clear inside of a menu box						
		LcdDrawA(DRAW_CLEAR_LCD,&dr); 
		
		// restore
		dr.x1--;
		dr.x2++;
		dr.y1--;
		dr.y2++;	
		
		// Draw the menu
		LcdDrawA(DRAW_FRAME_BOX,&dr);	
						
		if(!(ucSelection & 0x80))	
		{			
			/* Print Quit Button */				
			dr.x1 			=  15;
			dr.y1 			=  93;
			dr.colour 		= BLUE;
			dr.font			=   1;	
			dr.background 	=   0;
			dr.language		=   0;	
			dr.text			= "Quit";
			
			LcdDrawA(DRAW_PRINT_TXT,&dr);	
		}
											
		/* Print all menues - if any */
		for(i = 0; i < ucMenuCount; i++)
		{											
			dr.x1 			= 15;
			dr.y1 			= 83 - (i * 10);
			dr.colour 		= BLUE;
			dr.font			= 1;	
			dr.background 	= 0;
			dr.language		= 0;							
			dr.text			= (char *)menu[i].menu_text;
			
			if(ucSelection & 0x80)
			{	
				dr.x1 += 70;			
				dr.y1 += 10;				
			}
				
			LcdDrawA(DRAW_PRINT_TXT,&dr);		
			
			// Draw checks
			if(ucSelection & 0x80)
			{	
				dr.x1 			= 122;
				dr.y1 			= 93 - (i * 10);
				dr.colour 		= BLUE;
				dr.font			= 1;	
				dr.background 	= 0;
				dr.language		= 0;					
				
				if((ulong)menu[i].handler_ptr)		
					dr.text			= "y";
				else
					dr.text			= "n";		
				
				LcdDrawA(DRAW_PRINT_TXT,&dr);			
			}
		}
		
		/* Clear part of buttons box */		
		dr.y1 			= 118;
		dr.y2 			= 132;	
		dr.colour 		= BLUE;
		
		// Old code
		/*if(ucSelection & 0x80)	
		{
			dr.x1 			= 66;
			dr.x2 			= 132;
		}
		else
		{
			dr.x1 			= 1;
			dr.x2 			= 66;	
		}*/
		
		// Edit to fix dissapearing menues on Epson LCD
		dr.x1 			= 1;
		dr.x2 			= 132;
		
		LcdDrawA(DRAW_SOLID_BOX,&dr);
		
		// Old code - bottom bar gone on Epson LCD		
		/* Update the Left button Caption 
		dr.y1 			= 120;
		dr.colour 		= WHITE;
		dr.font			=   2;
		dr.background 	= NONE;
				
		if(ucSelection & 0x80)	
		{
			dr.x1 = 90;
			dr.text			= "     Hide   ";
		}
		else
		{
			dr.x1 =  6;
			dr.text			= " Hide  ";
		}
		
		LcdDrawA(DRAW_PRINT_TXT,&dr);		*/
		
		// New code
		dr.y1 			= 120;
		dr.colour 		= WHITE;
		dr.font			=   2;
		dr.background 	= NONE;
		
		if(ucSelection & 0x80)	
		{
			dr.x1 = 90;
			dr.text			= "     Hide   ";
						
			LcdDrawA(DRAW_PRINT_TXT,&dr);
			
			dr.x1   =  6;
			dr.text	= " Menu  ";
			
			LcdDrawA(DRAW_PRINT_TXT,&dr);
		}
		else
		{
			dr.x1 =  6;
			dr.text			= " Hide  ";
			
			LcdDrawA(DRAW_PRINT_TXT,&dr);
			
			dr.x1   = 90;
			dr.text	= " Options ";
			
			LcdDrawA(DRAW_PRINT_TXT,&dr);
		}							
	}
	else
	{
		dr.x1 			=   3;
		dr.x2 			= 128;
		dr.y1 			=  14;
		dr.y2 			= 112;
		LcdDrawA(DRAW_CLEAR_LCD,&dr);
		
		/* Clear part of buttons box */		
		dr.y1 			= 118;
		dr.y2 			= 132;	
		dr.colour 		= BLUE;
		
		// Old code
		/*if(ucSelection & 0x80)	
		{
			dr.x1 			= 66;
			dr.x2 			= 132;
		}
		else
		{
			dr.x1 			= 1;
			dr.x2 			= 66;	
		}*/
		
		// Edit to fix dissapearing menues on Epson LCD
		dr.x1 			= 1;
		dr.x2 			= 132;
		
		LcdDrawA(DRAW_SOLID_BOX,&dr);
		
		// Old code - bottom bar gone on Epson LCD	
		/* Update the Left button Caption 		
		dr.y1 			= 120;
		dr.colour 		= WHITE;
		dr.background 	=   0;
		dr.language		=   0;
		dr.font			=   2;
		dr.background 	= NONE;
				
		if(ucSelection & 0x80)	
		{
			dr.x1   = 90;
			dr.text	= " Options ";
		}
		else
		{
			dr.x1   =  6;
			dr.text	= " Menu  ";
		}
		
		LcdDrawA(DRAW_PRINT_TXT,&dr);		*/
		
		// New code
		dr.y1 			= 120;
		dr.colour 		= WHITE;
		dr.background 	=   0;
		dr.language		=   0;
		dr.font			=   2;
		dr.background 	= NONE;
		
		if(ucSelection & 0x80)	
		{					
			dr.x1   = 90;
			dr.text	= " Options ";
			
			LcdDrawA(DRAW_PRINT_TXT,&dr);
			
			dr.x1   =  6;
			dr.text	= " Menu  ";
			
			LcdDrawA(DRAW_PRINT_TXT,&dr);
		}
		else
		{							
			dr.x1   =  6;
			dr.text	= " Menu  ";
			
			LcdDrawA(DRAW_PRINT_TXT,&dr);
			
			dr.x1   = 90;
			dr.text	= " Options ";
			
			LcdDrawA(DRAW_PRINT_TXT,&dr);
		}		
	}			
	
	// Re-draw main frame - Epson LCD fixup
	dr.x1 			= 1;
	dr.x2 			= 130;
	dr.y1 			= 1;
	dr.y2 			= 118;
	dr.colour 		= BLUE;
	LcdDrawA(DRAW_FRAME_BOX,&dr);
#endif
}

// Move Up/Down the opened menu/options scroll
static void SelectMenu(uchar ucButtonID,uchar *ucSelection,uchar ucMaxCount)
{
#if 0
	X_DRAW_PARAMS 	dr;
								
	// Clear old
	dr.x1 			=    8;
	dr.x2 			=   50;			
	dr.y1 			= ( 92 - (ucMaxCount * 10)) + (*ucSelection * 10);
	dr.y2 			= (102 - (ucMaxCount * 10)) + (*ucSelection * 10);
	dr.colour 		= WHITE;
	
	if(ucButtonID & 0x80)
	{
		dr.x1 += 72;
		dr.x2 += 62;
		dr.y1 += 10;
		dr.y2 += 10;
	}
							
	LcdDrawA(DRAW_FRAME_BOX,&dr);
	
	dr.x1 = dr.x2 + 2;
	dr.x2 = dr.x1 + 10;
	dr.character = 1;
	
	LcdDrawA(DRAW_TRIANGLE,&dr);
	
	// Update new selection
	if((ucButtonID & 0xF) == 4)
	{
		if(ucButtonID & 0x80) ucMaxCount--;	
		if(*ucSelection < ucMaxCount) (*ucSelection)++;
		if(ucButtonID & 0x80) ucMaxCount++;	
	}
	else
	{
		if(*ucSelection)     		  (*ucSelection)--;	
	}
	
	// Select new				
	dr.x1 			= 8;
	dr.x2 			= 50;			
	dr.y1 			= ( 92 - (ucMaxCount * 10)) + (*ucSelection * 10);
	dr.y2 			= (102 - (ucMaxCount * 10)) + (*ucSelection * 10);
	dr.colour 		= GREEN;					
	
	if(ucButtonID & 0x80)
	{
		dr.x1 += 72;
		dr.x2 += 62;
		dr.y1 += 10;
		dr.y2 += 10;		
	}
	
	LcdDrawA(DRAW_FRAME_BOX,&dr);
	
	dr.x1 = dr.x2 + 2;
	dr.x2 = dr.x1 + 10;
	dr.character = 1;	
			
	LcdDrawA(DRAW_TRIANGLE,&dr);
#endif
}

// Toggle option status
static void UpdateOption(uchar ucSelection,PMENU_DEF menu)
{
#if 0
	X_DRAW_PARAMS 	dr;
	
	dr.x1 			= 122;
	dr.y1 			= 93 - (ucSelection * 10);
	dr.colour 		= BLUE;
	dr.font			= 1;	
	dr.background 	= 0;
	dr.language		= 0;					
		
	// Toggle state			
	if((ulong)menu[ucSelection].handler_ptr)
	{		
		dr.text	= "n";
		menu[ucSelection].handler_ptr = NULL;
	}
	else
	{		
		dr.text	= "y";
		menu[ucSelection].handler_ptr = (void *)1;
	}					
	LcdDrawA(DRAW_PRINT_TXT,&dr);			
#endif
}


//*----------------------------------------------------------------------------
//* Function Name       : ucVersion
//* Object              : Return version
//* Input Parameters    : none
//* Output Parameters   : none
//* Functions called    : none
//*----------------------------------------------------------------------------
static uchar ucVersion(ulong *ulOsVersion,uchar *ucHwVersion)
{
#if 0
	if((ulOsVersion == NULL) || (ucHwVersion == NULL))
		return 1;
		
	*ulOsVersion  = SEDNA_MAJOR 	<< 24;
	*ulOsVersion |= SEDNA_MINOR 	<< 16;
	*ulOsVersion |= SEDNA_RELEASE 	<<  8;
	*ulOsVersion |= SEDNA_BUILD;	
	
	*ucHwVersion  = ucUiHwVersion();
#endif
	return 0;
}

#if 0
static FL_FILE *CreateFileProxy(char *path)
{
	return fl_fopen(path,"w+");
}

static FL_FILE *OpenFileProxy(char *path)
{
	return fl_fopen(path,"r+");
}

static int ReadFileProxy(FL_FILE *f,uchar *b,ulong s)
{
	return fl_fread(b,s,1,f);
}

static int WriteFileProxy(FL_FILE *f,uchar *b,ulong s)
{
	return fl_fwrite(b,s,1,f);
}

static int RemoveFileProxy(char *path)
{
	if(fl_remove(path) != 0)
		return 1;
		
	return 0;	
}
#endif

//*----------------------------------------------------------------------------
//* Function Name       : ucSednaExportsNullSub
//* Object              :  Return error on none featured calls
//* Input Parameters    : none
//* Output Parameters   : none
//* Functions called    : none
//*----------------------------------------------------------------------------
static uchar ucSednaExportsNullSub(void)
{
	//DebugPrint(" -- nop called --\n\r");
	return 0x42;
}

/* API calls structure init */
const struct SEDNA_API_EXPORTS saeBlock[SEDNA_EXPORTS_API_COUNT + 1] = 
{      
    /* Device HW and version API's */
 	{(void *)ucVersion},
	{(void *)ucSednaExportsNullSub},
 	
 	/* Task control */
 	{(void *)vTaskDelay},
 	{(void *)vTaskDelay},
 	
 	/* Memory management */
 	//{(void *)os_heap_alloc},
 	//{(void *)os_heap_free},
	{(void *)ucSednaExportsNullSub},
	{(void *)ucSednaExportsNullSub},
 	
 	/* Application messaging */
 	//{(void *)ucQueueMessagesWaiting},
 	//{(void *)cQueueReceive},
 	//{(void *)cQueueSend},
	{(void *)ucSednaExportsNullSub},
	{(void *)ucSednaExportsNullSub},
	{(void *)ucSednaExportsNullSub},
 	
 	#if 1
   	/* Debug print */
 	{(void *)printf},
 	{(void *)printf},
 	{(void *)printf},
 	#else
 	{(void *)ucSednaExportsNullSub},
 	{(void *)ucSednaExportsNullSub},
 	{(void *)ucSednaExportsNullSub},
 	#endif
 
 	/* AT91 UART */
 	//{(void *)vUartOpenCom},
 	//{(void *)vUartCloseCom},
 	//{(void *)vUartWriteCom},
 	//{(void *)vUartReadCom},
 	//{(void *)vUartGetQueueStatus},
 	//{(void *)vUartPurge},
 		 	 	 	
 	/* Application Thread API's */
 	//{(void *)pvCreateThread},
 	//{(void *)os_task_delete},

 	/* LCD Draw Api Call */
 	//{(void *)LcdDrawA},
 	 	
 	/* File Lib API's */
 	//{(void *)OpenFileProxy},
 	//{(void *)fl_fclose},
 	//{(void *)ReadFileProxy},
        
    /* Nokia Fbus API's */
 	//{(void *)FbusInit},
 	//{(void *)FbusCleanUp},
 	//{(void *)CreateFbusMessageA},
 	//{(void *)FbusSetBus},
 	
 	/* Parallel port related */
 	//{(void *)vParPortMakeSound},
 	//{(void *)vParPortTogglePhonePower},
 	//{(void *)vParPortTogglePhoneVpp},
 	
 	/* UART addon */
 	//{(void *)vUartChangeBaud},
 	
 	/* USB Host API interface */
 	//{(void *)UsbHOpen},
 	//{(void *)UsbHClose},
 	//{(void *)UsbHReset},
 	//{(void *)UsbHEnumerateBasic},
 	//{(void *)UsbHDataRW},
 	
 	{(void *)SednaCreateScreenFrameX},
 	{(void *)SednaCreateAppSplashScreen},
 	{(void *)ucSednaExportsNullSub},
 	{(void *)ucSednaExportsNullSub},
 	
 	{(void *)SednaWaitMessage},
 	{(void *)SednaSendMessage},
 	
 	{(void *)ProcessUI},
 	
 	/* USB Host API interface - continued */
 	//{(void *)UsbHVendorCmd},
 	
 	/* File Lib API's - continued */
 	//{(void *)WriteFileProxy },
 	//{(void *)CreateFileProxy},
 	//{(void *)RemoveFileProxy},
 	{(void *)ucSednaExportsNullSub},/* Reserved */
 	{(void *)ucSednaExportsNullSub},
 	{(void *)ucSednaExportsNullSub},
 	
 	/* Show BMP picture */
 	//{(void *)LcdShowBmp},

 	// Reserved for flashing
 	{(void *)ucSednaExportsNullSub},
 	{(void *)ucSednaExportsNullSub},
 	{(void *)ucSednaExportsNullSub},
 	{(void *)ucSednaExportsNullSub},
 	{(void *)ucSednaExportsNullSub},

    0	
};

//*----------------------------------------------------------------------------
//* Function Name       : ulSednaExportsBaseAddress
//* Object              : Returns base address of API struct
//* Input Parameters    : so we dont use extern ptr
//* Output Parameters   : 
//* Functions called    : none
//*----------------------------------------------------------------------------
ulong ulSednaExportsBaseAddress(void)
{	
	return (ulong)&saeBlock[0];
}

#endif
