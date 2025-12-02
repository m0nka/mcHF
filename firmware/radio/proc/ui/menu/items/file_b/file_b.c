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
#include "mchf_pro_board.h"
#include "main.h"

#include "ui_proc.h"
#include "gui.h"
#include "dialog.h"
#include "desktop\ui_controls_layout.h"
#include "ui_menu_layout.h"
#include "ui_menu_module.h"

#include "browser\filebrowser_app.h"
#include "storage_proc.h"
#include "ff_gen_drv.h"

#include "file_b.h"

extern GUI_CONST_STORAGE GUI_BITMAP bmicon_gps;

// Menu layout definitions from Flash
extern const struct UIMenuLayout menu_layout[];

// UI driver public state
extern struct	UI_DRIVER_STATE			ui_s;

// Menu image
extern GUI_CONST_STORAGE GUI_BITMAP bmProgramGroup;
//extern GUI_CONST_STORAGE GUI_BITMAP bmicon_compref;

// TreeView images
extern GUI_CONST_STORAGE GUI_BITMAP bmClosedFolder;
extern GUI_CONST_STORAGE GUI_BITMAP bmOpenFolder;
extern GUI_CONST_STORAGE GUI_BITMAP bmTextLog;

static void Startup(WM_HWIN hWin, uint16_t xpos, uint16_t ypos);
static void KillFileb(void);
//static void _RefreshBrowser ( WM_HWIN hWin);

K_ModuleItem_Typedef  file_b =
{
	10,
	"File Browser",
	&bmProgramGroup,
	Startup,
	NULL,
	KillFileb
};

FILELIST_FileTypeDef  *pFileList = NULL;

WM_HWIN 	hExplorerWin = 0;
#ifdef USE_POPUP
WM_HWIN 	hPopUp = 0;
WM_HWIN 	hFileInfo = 0;
MENU_Handle hMenu = 0;
#endif

uint16_t FolderLevel  = 0;
char SelectedFileName[FILEMGR_FILE_NAME_SIZE];
char    str[FILEMGR_FILE_NAME_SIZE];

static void Startup(WM_HWIN hWin, uint16_t xpos, uint16_t ypos);
static void _RefreshBrowser ( WM_HWIN hWin);

static const GUI_WIDGET_CREATE_INFO _aDialog[] = 
{
// -----------------------------------------------------------------------------------------------------------------------------
//							name			id					x		y		xsz		ysz		?		?		?
// -----------------------------------------------------------------------------------------------------------------------------
// Self
{ WINDOW_CreateIndirect,	"",				ID_WINDOW_0,		0,    	0,		800,	430, 	0, 		0x64,	0 },
// Eject Button
{ BUTTON_CreateIndirect, 	"Eject",		ID_BUTTON_EJECT, 	670, 	375, 	120, 	45, 	0, 		0x0, 	0 },
//
{ BUTTON_CreateIndirect, 	"Details",		ID_BUTTON_OPEN, 	670, 	310, 	120, 	45, 	0, 		0x0, 	0 },
{ TREEVIEW_CreateIndirect, 	"Treeview", 	ID_TREEVIEW, 		7, 		6, 		635, 	380, 	0, 		0x0, 	0 },
//
{ EDIT_CreateIndirect,     	"msd.Edit",    	ID_EDIT_MSD,   		7,  	395,  	100,  	25, 	0,		0x0,	0 },
{ PROGBAR_CreateIndirect, 	"Progbar", 		ID_PROGBAR_MSD, 	117,	395, 	525, 	25, 	0, 		0x0, 	0 },
// Power out and Vcc text
{ EDIT_CreateIndirect, 		"Edit1", 		GUI_ID_EDIT1,		670,	40,		120, 	30,  	0, 		0x0,	0 },
{ EDIT_CreateIndirect, 		"Edit2", 		GUI_ID_EDIT2,		670,	80,		120, 	30,  	0, 		0x0,	0 },
{ EDIT_CreateIndirect, 		"Edit3", 		GUI_ID_EDIT3,		670,	120,	120, 	30,  	0, 		0x0,	0 },
};

#ifdef USE_POPUP
static const GUI_WIDGET_CREATE_INFO _aFileInfoDialogCreate[] =
{
// -----------------------------------------------------------------------------------------------------------------------------
//							name						id				 x	  y	  xsize	ysize	?		?		?
// -----------------------------------------------------------------------------------------------------------------------------
// Self
{ FRAMEWIN_CreateIndirect, "File Information", 	ID_FRAMEWIN_0, 			 0,    0, 500, 	300, 	0, 0x0, 0 },
//{ TEXT_CreateIndirect, 		"file name", 		ID_TEXT_FILENAME, 		 10,   5,  80, 	20, 	0, 0x0, 0 },
//{ TEXT_CreateIndirect, 		"Location: ", 		ID_TEXT_SOLID_LOCATION,  10,  25,  80, 	20, 	0, 0x0, 0 },
//{ TEXT_CreateIndirect, 		"", 				ID_TEXT_LOCATION, 		 89,  25, 300, 	20, 	0, 0x0, 0 },
//{ TEXT_CreateIndirect, 		"Created on : ", 	ID_TEXT_SOLID_CREATION,  10,  40,  80, 	20, 	0, 0x0, 0 },
//{ TEXT_CreateIndirect, 		"", 				ID_TEXT_CREATION, 		 84,  40, 300, 	20, 	0, 0x0, 0 },
//{ TEXT_CreateIndirect, 		"File Size : ", 	ID_TEXT_SOLID_FILESIZE,  10,  55,  80, 	20, 	0, 0x0, 0 },
//{ TEXT_CreateIndirect, 		"", 				ID_TEXT_FILESIZE, 		 87,  55, 300, 	20, 	0, 0x0, 0 },
{ BUTTON_CreateIndirect, 	"OK", 				ID_BUTTON_OK_FILEINFO, 	190, 181, 100, 	26, 	0, 0x0, 0 },
};
#endif

#ifdef USE_POPUP
/* Array of menu items */
static MENU_ITEM _aMenuItems[] = 
{
  {"Open File"          , ID_MENU_OPENFILE,  	0},
  {0                    , 0           ,  		MENU_IF_SEPARATOR},
  {"Delete File"        , ID_MENU_DELETE, 		0},
  {0                    , 0           ,  		MENU_IF_SEPARATOR},
  {"Properties"         , ID_MENU_PROPRIETIES, 	0},
  {0                    , 0           ,  		MENU_IF_SEPARATOR},
  {0                    , 0           ,  		MENU_IF_SEPARATOR},
  {0                    , 0           ,  		MENU_IF_SEPARATOR},
  {"Cancel"             , ID_MENU_EXIT, 		0},
};
#endif

static void file_b_free_memory(void)
{
	// Free list memory
	if(pFileList != NULL)
	{
		//--printf("List free on exit \r\n");
		vPortFree(pFileList);
		pFileList = NULL;
	}
}

static void file_b_set_edit_look(WM_HWIN hItem)
{
	EDIT_SetFont(hItem,&GUI_Font16B_1);
	EDIT_SetBkColor(hItem,EDIT_CI_ENABLED, LIGHT_PINK);
	EDIT_SetTextColor(hItem,EDIT_CI_ENABLED, GUI_WHITE);
	EDIT_SetTextAlign(hItem,TEXT_CF_HCENTER|TEXT_CF_VCENTER);
	EDIT_SetMaxLen(hItem, 32);
}

//*----------------------------------------------------------------------------
//* Function Name       : vUsbSamSendQueuedMessage
//* Object              : Send message to queue
//* Input Parameters    : none
//* Output Parameters   : none
//* Functions called    : none
//*----------------------------------------------------------------------------
static uchar vUiSendQueuedMessage(xQueueHandle pvQueueHandle, ulong *ulMessageBuffer, uchar ucNumberOfItems)
{
#if 0
	ulong ulDummy;
	uchar ucCount;

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
//* Function Name       : vUsbSamSednaWaitMessage
//* Object              : Read pending messages
//* Input Parameters    : Rx Queue ptr and items buffer
//* Output Parameters   : none.
//*--------------------------------------------------------------------------------------
static uchar  vUiSednaWaitMessage(xQueueHandle pRxQueue,ulong *ulQueueBuffer)
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

static void vUiLoadApplication(char *chAppName,uchar ucIsScript)
{
	printf("run app: %s \r\n", chAppName);

#if 0
	ulong 			ulData[10];
	ulong			ulTimeout = 600;
	uchar			ucSkip = 1;

	uchar 			app_det[20];
	char			chCertPath[32]; //= "C:\\System\\30000.crt";

	//DebugPrint(chDeviceSN);
	//DebugPrint("\n\r");

	//vUiEpsonInfoBox("Loading...",0,0);

	if(!ucIsScript)
	{
		// Check if sec processor was detected on load up
		//if(chDeviceSN[0] == 0)
		//{
		//	#if (SEDNA_DEBUG_BUILD == 1)
		//	//DebugPrint("err SN!\n\r");
		//	#endif
		//
		//	vUiEpsonInfoBox("Not Allowed!",0,0);
		//	goto clean_up;
		//}

		// Create device certificate path
		vUiMemSet((uchar *)chCertPath,0,sizeof(chCertPath));
		vTaskSuspendAll();
		strcpy(chCertPath,"C:\\System\\");
		//strcat(chCertPath,chDeviceSN);
		strcat(chCertPath,"100000");
		strcat(chCertPath,".crt");
		cTaskResumeAll();

		// Insert the requst as a message
		ulData[0] = 0xC3;
		ulData[2] = (ulong)chCertPath;
	}
	else
	{
		// Insert the requst as a message
		ulData[0] = 0xD1;
		ulData[2] = 0;
	}

	// Insert path
	ulData[1] = (ulong)chAppName;

	#if (SEDNA_DEBUG_BUILD == 1)
	//DebugPrint("Load app...");
	#endif

	// Post load request
	if(vUiSendQueuedMessage(pxUsbFtdiParams->xAppLoaderRxQueue,ulData,3) != 0)
	{
		#if (SEDNA_DEBUG_BUILD == 1)
		//DebugPrint("err load!\n\r");
		#endif

		vUiEpsonInfoBox("Error msg1!",0,0);
		goto clean_up;
	}

	#if (SEDNA_DEBUG_BUILD == 1)
	//DebugPrint("ok.\n\r");
	//DebugPrint("Check status...");
	#endif

	/* Wait to check loading result */
	while((vUiSednaWaitMessage(pxUsbFtdiParams->xAppLoaderTxQueue,ulData) == 0) && ulTimeout)
	{
		OsSleep(20);

		ulTimeout--;
	}

	// Check if msg received
	if(ulTimeout == 0)
	{
		#if (SEDNA_DEBUG_BUILD == 1)
		//DebugPrint("err timeout!\n\r");
		#endif

		vUiEpsonInfoBox("Timeout Err!",0,0);
		goto clean_up;
	}

	// Check return msg
	if(ulData[0] != 0xD6)
	{
		#if (SEDNA_DEBUG_BUILD == 1)
		//DebugPrint("err ret msg!\n\r");
		#endif

		vUiEpsonInfoBox("Err Msg2!",0,0);
		goto clean_up;
	}

	// Check func result
	if((ulData[1] & 0xFF) != 0x00)
	{
		#if (SEDNA_DEBUG_BUILD == 1)
		//DebugPrintInt("err exec: ",ulData[1] & 0xFF);
		#endif

		//vTaskSuspendAll();
		s_sprintf(chCertPath,"Exec Err: %d",(uchar)(ulData[1] & 0xFF));
		//cTaskResumeAll();

		vUiEpsonInfoBox(chCertPath,0,0);
		goto clean_up;
	}

	#if (SEDNA_DEBUG_BUILD == 1)
	//DebugPrint("ok.\n\r");
	#endif

	vUiExtractHandleAndName(app_det,ulData);

	//  --- Access violation here !!! ---
	//DebugPrint((char *)(app_det + 4));

	//vUiEpsonInfoBox("Success.");
	// ----------------------------------

	ucSkip = 0;

	// Overwrite pub var
	isScript = ucIsScript;

clean_up:

	if(ucSkip)
	{
		/* Small Delay */
		OsSleep(100);

		/* Return in the Programs Menu */
   		vUiListPrograms();
		ucMsCurrentID = CURR_STATE_PROGRAM;
	}
#endif
}

#ifdef USE_POPUP
static void _AddMenuItem(MENU_Handle hMenu, MENU_Handle hSubmenu, const char* pText, U16 Id, U16 Flags)
{
	MENU_ITEM_DATA Item;

	Item.pText    = pText;
	Item.hSubmenu = hSubmenu;
	Item.Flags    = Flags;
	Item.Id       = Id;

	MENU_AddItem(hMenu, &Item);
}

static void _cbPopup(WM_MESSAGE * pMsg)
{
	K_GET_DIRECT_OPEN_FUNC 	*pfOpen;
	MENU_MSG_DATA			*pData;
	TREEVIEW_ITEM_Handle  	hTreeView;
	TREEVIEW_ITEM_INFO    	Info;
	WM_HWIN 				hItem;
	char 					ext[4];

	switch (pMsg->MsgId)
	{
		case WM_INIT_DIALOG:
		{
			//printf("popup init\r\n");
			break;
		}

		case WM_MENU:
		{
			//printf("popup menu\r\n");

			pData = (MENU_MSG_DATA*)pMsg->Data.p;
			switch (pData->MsgType)
			{
				case MENU_ON_ITEMSELECT:
				case MENU_ON_ITEMPRESSED:
				{
					//printf("popup item select\r\n");

					switch (pData->ItemId)
					{
						case ID_MENU_OPENFILE:
						{
							printf("popup open file\r\n");

							vUiLoadApplication(SelectedFileName, 0);

							#if 0
							k_GetExtOnly(SelectedFileName, ext);
							pfOpen = k_ModuleCheckLink(ext);

							if(pfOpen != NULL)
							{
								pfOpen(SelectedFileName);
							}
							else
							{
							//	_ShowMessageBox(hExplorerWin, "File Browser", "No external module is linked to\n this extension!", 0);
							}
							#endif

							break;
						}

						case ID_MENU_DELETE:
						{
							printf("popup delete file\r\n");
							//if (_ShowMessageBox(hExplorerWin, "File Browser", "Are you sure you want to\ndelete selected file?", 1))
							//{
							//	f_unlink (SelectedFileName);
							//	_RefreshBrowser(hExplorerWin);
							//}
							break;
						}

						case ID_MENU_PROPRIETIES:
						{
							printf("popup properties\r\n");

#if 0
							if(hFileInfo == 0)
							{
								hFileInfo = GUI_CreateDialogBox(_aFileInfoDialogCreate,
																GUI_COUNTOF(_aFileInfoDialogCreate),
																_cbFileInfoDialog,
																hExplorerWin,
																100,
																75);
							}
#endif
							break;
						}

						case ID_MENU_EXIT:
						{
							//printf("popup exit\r\n");
							break;
						}

						default:
							WM_DefaultProc(pMsg);
							break;
					}

					hItem = WM_GetDialogItem(hExplorerWin, ID_TREEVIEW);
					hTreeView = TREEVIEW_GetSel(hItem);
					TREEVIEW_ITEM_GetInfo(hTreeView, &Info);
					if(Info.IsNode == 1)
					{
						hTreeView = TREEVIEW_GetItem(hItem, hTreeView, TREEVIEW_GET_PARENT);
						TREEVIEW_SetSel(hItem, hTreeView);
					}
					break;
				}

				default:
					//printf("popup msg type %d\r\n",pData->MsgType);
					WM_DefaultProc(pMsg);
					break;
			}
			break;
		}

		default:
		{
			WM_DefaultProc(pMsg);
			break;
		}
	}
}

static void _OpenPopup(WM_HWIN hParent, MENU_ITEM * pMenuItems, int NumItems, int x, int y) 
{
	//printf("_OpenPopup\r\n");

	if(!hMenu)
	{
		int i;

		//printf("_OpenPopup create\r\n");

		// Create the popup window only one time
		hMenu = MENU_CreateEx(0, 0, 0, 0, WM_UNATTACHED, 0, MENU_CF_VERTICAL, 0);

		MENU_SetBkColor(hMenu, MENU_CI_SELECTED, GUI_LIGHTBLUE);
		MENU_SetFont(hMenu,GUI_FONT_32B_ASCII);

		for (i = 0; i < NumItems; i++)
		{
			_AddMenuItem(hMenu, 0, pMenuItems[i].sText, pMenuItems[i].Id, pMenuItems[i].Flags);
		}
	}

	/* Open the popup menu. After opening the menu the function returns immediately.
	 * After selecting menu item or after touching the display outside the menu the
	 * popup menu will be closed, but not deleted.
	 */
	MENU_Popup(hMenu, hParent, x, y, 0, 0, 0);
}
#endif

#ifdef USE_POPUP
static void _cbFileInfoDialog(WM_MESSAGE * pMsg)
{
	WM_HWIN hItem;
	int     NCode;
	int     Id;
	//FILINFO fno;
 
	switch (pMsg->MsgId)
	{
		case WM_INIT_DIALOG:
		{
#if 0
			hItem = pMsg->hWin;

			// File information dialog
			FRAMEWIN_SetClientColor   (hItem, GUI_LIGHTGRAY);
			FRAMEWIN_SetFont          (hItem, &GUI_Font20B_ASCII);
			FRAMEWIN_SetTextAlign     (hItem, GUI_TA_HCENTER);

			hItem = WM_GetDialogItem(pMsg->hWin, ID_TEXT_FILENAME);
			FILEMGR_GetFileOnly(str, SelectedFileName);
			TEXT_SetText(hItem, str);
    
			hItem = WM_GetDialogItem(pMsg->hWin, ID_TEXT_LOCATION);

			printf("path: %s \r\n", SelectedFileName);
			if(SelectedFileName[0] == '0')
			{
				//  TEXT_SetText(hItem, "[USB Disk]");
				//}
				//else if(SelectedFileName[0] == '1')
				//{
				TEXT_SetText(hItem, "[SD Card Slot]");
			}

			f_stat (SelectedFileName, &fno);
    
			if(fno.fdate == 0)
			{
				fno.fdate = (1 << 5) | 1; /* Set January, 1st */
			}

			sprintf(str, "%02hu/%02hu/%hu %02hu:%02hu:%02hu", ( fno.fdate) & 0x1F,
                                                ((fno.fdate) >> 5) & 0x0F,
                                                (((fno.fdate) >> 9) & 0x3F) + 1980,
                                                ((fno.ftime) >> 11) & 0x1F,
                                                ((fno.ftime) >> 5)  & 0x3F,
                                                (fno.ftime) & 0x1F);
    
			hItem = WM_GetDialogItem(pMsg->hWin, ID_TEXT_CREATION);
			TEXT_SetText(hItem, str);
			if (fno.fsize < 1024)
			{
				sprintf(str, "%lu Byte(s)", fno.fsize);
			}
			else if (fno.fsize < (1024 * 1024))
			{
				sprintf(str, "%lu KByte(s)", fno.fsize/ 1024);
			}
			else
			{
				sprintf(str, "%lu MByte(s)", fno.fsize/ 1024 / 1024);
			}
			hItem = WM_GetDialogItem(pMsg->hWin, ID_TEXT_FILESIZE);
			TEXT_SetText(hItem, str);
#endif
			break;
		}

		case WM_NOTIFY_PARENT:
		{
			Id    = WM_GetId(pMsg->hWinSrc);
			NCode = pMsg->Data.v;

			if((Id == ID_BUTTON_OK_FILEINFO)&&(NCode == WM_NOTIFICATION_RELEASED))
			{
				GUI_EndDialog(pMsg->hWin, 0);
				hFileInfo = 0;
				printf("exit ok\r\n");
				break;
			}

			break;
		}

		default:
			WM_DefaultProc(pMsg);
			break;
	}
}
#endif

static void _cbHint(WM_MESSAGE * pMsg) 
{
	GUI_RECT Rect;
  
	switch (pMsg->MsgId)
	{
		case WM_PAINT:
			GUI_SetBkColor(GUI_LIGHTBLUE);
			GUI_Clear();
			GUI_SetColor(GUI_WHITE);
			GUI_SetFont(&GUI_Font16_1HK);
			GUI_DispStringHCenterAt("Loading card content...", 110 , 10);
			GUI_SetFont(GUI_DEFAULT_FONT);
			WM_GetClientRect(&Rect);
			GUI_SetColor(GUI_DARKGRAY);
			GUI_DrawRectEx(&Rect);
			break;
    
		default:
			WM_DefaultProc(pMsg);
			break;
	}
}

#ifdef USE_MSGBOX
static void _cbMessageBox(WM_MESSAGE* pMsg)
{
  WM_HWIN hWin;
  int Id;

  hWin = pMsg->hWin;
  switch (pMsg->MsgId) {
  case WM_NOTIFY_PARENT:
    if (pMsg->Data.v == WM_NOTIFICATION_RELEASED) {
      Id = WM_GetId(pMsg->hWinSrc);
       GUI_EndDialog(hWin, (Id == GUI_ID_OK) ? 1 : 0);
    }
    
    break;
  default:
    WM_DefaultProc(pMsg);
  }
}

static int _ShowMessageBox(WM_HWIN hWin, const char* pTitle, const char* pText, int YesNo) 
{
	WM_HWIN hFrame, hClient, hBut;
	int r = 0;

	// Create frame win
	hFrame = FRAMEWIN_CreateEx(0, 0, 390, 290, hWin, WM_CF_SHOW, FRAMEWIN_CF_MOVEABLE, 0, pTitle, &_cbMessageBox);

	FRAMEWIN_SetClientColor   (hFrame, GUI_WHITE);
	FRAMEWIN_SetFont          (hFrame, &GUI_Font16B_ASCII);
	FRAMEWIN_SetTextAlign     (hFrame, GUI_TA_HCENTER);

	// Create dialog items
	hClient = WM_GetClientWindow(hFrame);
	TEXT_CreateEx(10, 7, 370, 230, hClient, WM_CF_SHOW, GUI_TA_HCENTER, 0, pText);

	if(YesNo)
	{
		hBut = BUTTON_CreateEx(97, 45, 55, 18, hClient, WM_CF_SHOW, 0, GUI_ID_CANCEL);
		BUTTON_SetText        (hBut, "No");
		hBut = BUTTON_CreateEx(32, 45, 55, 18, hClient, WM_CF_SHOW, 0, GUI_ID_OK);
		BUTTON_SetText        (hBut, "Yes");
	}
	else
	{
		hBut = BUTTON_CreateEx(64, 45, 55, 18, hClient, WM_CF_SHOW, 0, GUI_ID_OK);
		BUTTON_SetText        (hBut, "Ok");
	}

	WM_SetFocus(hFrame);
	WM_MakeModal(hFrame);
  
	r = GUI_ExecCreatedDialog(hFrame);

	return r;
}
#endif

void _FindFullPath(TREEVIEW_Handle hObj, TREEVIEW_ITEM_Handle hTVItem, char *str)
{
	TREEVIEW_ITEM_INFO hInfo;
	char strtmp[FILEMGR_FILE_NAME_SIZE];
	uint8_t Level = FILEMGR_MAX_LEVEL;
  
	/* Find File name */
	TREEVIEW_ITEM_GetText(hTVItem, (uint8_t *)str, FILEMGR_FILE_NAME_SIZE);

	/* Find folders */
	while(Level > 1)
	{
		TREEVIEW_ITEM_GetInfo(hTVItem, &hInfo);
		hTVItem = TREEVIEW_GetItem(hObj, hTVItem,TREEVIEW_GET_PARENT);
		TREEVIEW_ITEM_GetText(hTVItem, (uint8_t *)strtmp, FILEMGR_FILE_NAME_SIZE);
    
		if(strcmp(strtmp, "SD Card Slot") == 0)
		{
			strcpy(strtmp, "0:");
		}
		//else if(strcmp(strtmp, "USB Disk") == 0)
		//{
		//  strcpy(strtmp, "0:");
		//}
		strcat(strtmp, "/");
		strcat(strtmp, str);
		strcpy(str, strtmp);
		TREEVIEW_ITEM_GetInfo(hTVItem, &hInfo);
		Level = hInfo.Level;
	}

	//printf("path name: %s \r\n", str);
}

static void ShowFileDetails(WM_HWIN hWin)
{
	WM_HWIN hItem1, hItem2, hItem3;
	FILINFO fno;

	hItem1 = WM_GetDialogItem(hWin, GUI_ID_EDIT1);
	hItem2 = WM_GetDialogItem(hWin, GUI_ID_EDIT2);
	hItem3 = WM_GetDialogItem(hWin, GUI_ID_EDIT3);

	//printf("path: %s \r\n", SelectedFileName);

	int res = f_stat (SelectedFileName, &fno);
	if(res != FR_OK)
	{
		printf("res: %d \r\n", res);
		EDIT_SetText(hItem1, "");
		EDIT_SetText(hItem2, "");
		EDIT_SetText(hItem3, "");
		return;
	}

	// File name
	FILEMGR_GetFileOnly(str, SelectedFileName);
	//printf("file: %s \r\n", str);
	EDIT_SetText(hItem1, str);

	// File date
	//printf("date: %d \r\n", fno.fdate);
	if(fno.fdate == 0)
	{
		fno.fdate = (1 << 5) | 1; /* Set January, 1st */
	}

	sprintf(str, "%02d/%02d/%d %02d:%02d:%02d", ( fno.fdate) & 0x1F,
                                        		((fno.fdate) >> 5) & 0x0F,
												(((fno.fdate) >> 9) & 0x3F) + 1980,
												((fno.ftime) >> 11) & 0x1F,
												((fno.ftime) >> 5)  & 0x3F,
												(fno.ftime) & 0x1F);

	//printf("str: %s \r\n", str);

	EDIT_SetText(hItem2, str);

	// File size
	//printf("size: %d \r\n", fno.fsize);
	if (fno.fsize < 1024)
	{
		sprintf(str, "%d Byte(s)", (int)fno.fsize);
	}
	else if (fno.fsize < (1024 * 1024))
	{
		sprintf(str, "%d KByte(s)", (int)(fno.fsize/1024));
	}
	else
	{
		sprintf(str, "%d MByte(s)", (int)(fno.fsize/1024/1024));
	}
	//printf("str1: %s \r\n", str);

	EDIT_SetText(hItem3, str);
}

static void ShowNodeContent(WM_HWIN hTree, TREEVIEW_ITEM_Handle hNode, char *path, FILELIST_FileTypeDef *list) 
{
  uint32_t i = 0, Position = 0;
  TREEVIEW_ITEM_Handle hItem = 0;
  FILEMGR_ParseDisks(path, list); 
  char fullpath[FILEMGR_FILE_NAME_SIZE];

  /*Create root nodes */
  if(list->ptr > 0)
  {
    for (i = 0; i < list->ptr; i++)
    {
      Position = hItem ? TREEVIEW_INSERT_BELOW : TREEVIEW_INSERT_FIRST_CHILD;
      hItem = hItem ? hItem : hNode;
      if(list->file[i].type == FILETYPE_DIR)
      {
        strcpy(fullpath, path);
        strcat (fullpath, "/");
        strcat (fullpath, (char *)list->file[i].name);        
        hItem = TREEVIEW_InsertItem(hTree, TREEVIEW_ITEM_TYPE_NODE, hItem, Position, (char *)list->file[i].name);
        list->next = malloc(sizeof(FILELIST_FileTypeDef));
        list->next->prev = list;
        list = list->next;
        if(FolderLevel++ < FILEMGR_MAX_LEVEL)
        {
          ShowNodeContent(hTree, hItem, fullpath, list);
        }
        FolderLevel--;
        list = list->prev;
        free(list->next);
      }
      else
      {
        hItem = TREEVIEW_InsertItem(hTree, TREEVIEW_ITEM_TYPE_LEAF, hItem, Position, (char *)list->file[i].name);
      }
    }
  }

}

/**
  * @brief  Explores disk.
  * @param  hTree: tree view handle
  * @retval None
  */
static void ExploreDisks(WM_HWIN hTree) 
{
	TREEVIEW_ITEM_Handle 	hItem = 0;
	TREEVIEW_ITEM_Handle 	Node = 0;
	//uint32_t 				Position = 0;
	char 					disk_label[64];

	// Get Card label
	if(Storage_GetLabel(MSD_DISK_UNIT, disk_label) != 0)
	{
		strcpy(disk_label, "NONAME");
	}
	//--Node = TREEVIEW_InsertItem(hTree, TREEVIEW_ITEM_TYPE_NODE, 0, 0, "SD Card Slot");
	Node = TREEVIEW_InsertItem(hTree, TREEVIEW_ITEM_TYPE_NODE, 0, 0, disk_label);
  
	if(Storage_GetStatus(MSD_DISK_UNIT) == 1)
	{
		memset(disk_label, 0, sizeof(disk_label));

		// Get drive letter
		if(Storage_GetDrive(MSD_DISK_UNIT, disk_label) != 0)
		{
			strcpy(disk_label, "0:/");
		}
		//else
		//	printf("get drive: %s \r\n", disk_label);

		hItem = TREEVIEW_InsertItem(hTree, TREEVIEW_ITEM_TYPE_NODE, Node, TREEVIEW_INSERT_FIRST_CHILD, disk_label);
		ShowNodeContent(hTree, hItem, StorageDISK_Drive, pFileList);
	}
  
	TREEVIEW_SetAutoScrollH(hTree, 1);
	TREEVIEW_SetAutoScrollV(hTree, 1);
	TREEVIEW_SetIndent(hTree, 22);

	hItem = TREEVIEW_GetItem(hTree, 0, TREEVIEW_GET_FIRST);
	TREEVIEW_ITEM_Expand(hItem);
  
	hItem = TREEVIEW_GetItem(hTree, hItem, TREEVIEW_GET_FIRST_CHILD);
	if(hItem != 0)
	{
		TREEVIEW_ITEM_Expand(hItem);
		hItem = TREEVIEW_GetItem(hTree, hItem, TREEVIEW_GET_NEXT_SIBLING);
		if(hItem != 0)
		{
			TREEVIEW_ITEM_Expand(hItem);
		}
	}
  
	WM_SetFocus(hTree);
}

static void _RefreshBrowser(WM_HWIN hWin)
{
	WM_HWIN 				hItem, Hint;
	TREEVIEW_ITEM_Handle  	hTreeView;
	uint32_t 				free_size = 0, total_size = 0, perc = 0;
	char 					str[FILEMGR_FILE_NAME_SIZE];
	uchar					detected = 0;

	//printf("refresh \r\n");

	// Delete file selection
	SelectedFileName[0] = 0;

	// Clear details
	ShowFileDetails(hWin);

	// Create progress hint
	GUI_Exec();
	Hint = WM_CreateWindowAsChild(80, 120, 200, 32, hWin, WM_CF_SHOW, _cbHint, 0);
	GUI_Exec();

	hItem = WM_GetDialogItem(hWin, ID_PROGBAR_MSD);

	if(Storage_GetStatus (MSD_DISK_UNIT))
	{
		free_size  = Storage_GetFree(MSD_DISK_UNIT);
		if(free_size != 0)
		{
			total_size = Storage_GetCapacity(MSD_DISK_UNIT);
			if(total_size != 0)
			{
				perc = ((total_size - free_size)*100)/total_size;

				if(((total_size - free_size)*100)%total_size)
					perc++;

				//PROGBAR_SetMinMax(hItem, 0, 100);
				PROGBAR_SetValue (hItem,perc);
				hItem = WM_GetDialogItem(hWin, ID_EDIT_MSD);
				sprintf(str, "%d MB", (int)(total_size/(2*1024)));
				EDIT_SetText(hItem, str);

				detected = 1;
			}
		}
	}
	else
	{
		PROGBAR_SetValue (hItem, 0);
		hItem = WM_GetDialogItem(hWin, ID_EDIT_MSD);
		EDIT_SetText(hItem, "" );
	}
  
	hTreeView = WM_GetDialogItem(hWin, ID_TREEVIEW);
	hItem = TREEVIEW_GetItem(hTreeView, 0, TREEVIEW_GET_FIRST);
	if(hItem != 0)
	{
		TREEVIEW_ITEM_Delete (hItem);
	}

	if(detected)
		ExploreDisks(hTreeView);

	// Remove progress hint
	WM_DeleteWindow(Hint);
}

static void _cbMediaConnection(WM_MESSAGE * pMsg) 
{
	static WM_HTIMER      hStatusTimer;
	static uint8_t        prev_sd_status = 0;
   
	switch (pMsg->MsgId)
	{
		case WM_CREATE:
		{
			prev_sd_status = Storage_GetStatus(MSD_DISK_UNIT);
			hStatusTimer = WM_CreateTimer(pMsg->hWin, 0, 500, 0);
			break;
		}
    
		case WM_TIMER:
		{
			if(prev_sd_status !=  Storage_GetStatus(MSD_DISK_UNIT))
			{
				prev_sd_status = Storage_GetStatus(MSD_DISK_UNIT);
				_RefreshBrowser(hExplorerWin);
			}
			WM_RestartTimer(pMsg->Data.v, 500);
			break;
		}
    
		case WM_DELETE:
		{
			if(hStatusTimer != 0)
			{
				WM_DeleteTimer(hStatusTimer);
				hStatusTimer = 0;
			}
			break;
		}
    
		default:
			WM_DefaultProc(pMsg);
			break;
	}
}

static void _cbControl(WM_MESSAGE * pMsg, int Id, int NCode)
{
	WM_HWIN hItem;
	char	capt[30];

	switch(Id)
	{
		case ID_BUTTON_EJECT:
		{
			if(NCode == WM_NOTIFICATION_RELEASED)
			{
				// ToDo: Send message to storage process..
			}

			break;
		}

		case ID_BUTTON_OPEN:
		{
			if(NCode == WM_NOTIFICATION_RELEASED)
			{
				hItem = WM_GetDialogItem(pMsg->hWin, ID_BUTTON_OPEN);
				BUTTON_GetText(hItem, capt, sizeof(capt));

				if(strcmp(capt, "Run") == 0)
					vUiLoadApplication(SelectedFileName, 0);
				else if(strcmp(capt, "Open") == 0)
				{
					// ToDo: Open in text window
					printf("show text file: %s \r\n", SelectedFileName);
				}
				else
				{
					// Nothing ?
				}
			}

			break;
		}

		default:
			WM_DefaultProc(pMsg);
			break;
	}
}

static void _cbDialog(WM_MESSAGE * pMsg)
{
	WM_HWIN 				hItem;
	int 					Id, NCode;
	TREEVIEW_ITEM_Handle  	hTreeView;
	TREEVIEW_ITEM_INFO    	Info;
	#ifdef USE_POPUP
	GUI_PID_STATE 			State;
	#endif

	switch (pMsg->MsgId)
	{
		case WM_INIT_DIALOG:
		{
			// TreeView control
			hTreeView = WM_GetDialogItem(pMsg->hWin, ID_TREEVIEW);

			// Set spacing
			TREEVIEW_SetIndent(hTreeView,48);
			TREEVIEW_SetTextIndent(hTreeView,48);

			// Set better looking images
			TREEVIEW_SetImage(hTreeView,TREEVIEW_BI_CLOSED,	&bmClosedFolder);
			TREEVIEW_SetImage(hTreeView,TREEVIEW_BI_OPEN,	&bmOpenFolder);
			TREEVIEW_SetImage(hTreeView,TREEVIEW_BI_LEAF,	&bmTextLog);

			// MSD Size Edit
			#if 0
			hItem = WM_GetDialogItem(pMsg->hWin, ID_EDIT_MSD);
			EDIT_SetFont(hItem,&GUI_Font20_1);
			EDIT_SetBkColor(hItem,EDIT_CI_ENABLED, GUI_WHITE );
			EDIT_SetTextColor(hItem,EDIT_CI_ENABLED, GUI_STCOLOR_LIGHTBLUE);
			EDIT_SetTextAlign(hItem,TEXT_CF_HCENTER|TEXT_CF_VCENTER);
			EDIT_SetText(hItem, "12345678");
			#else
			file_b_set_edit_look(WM_GetDialogItem(pMsg->hWin, ID_EDIT_MSD));
			#endif

			// File details
			file_b_set_edit_look(WM_GetDialogItem(pMsg->hWin, GUI_ID_EDIT1));
			file_b_set_edit_look(WM_GetDialogItem(pMsg->hWin, GUI_ID_EDIT2));
			file_b_set_edit_look(WM_GetDialogItem(pMsg->hWin, GUI_ID_EDIT3));

			// Allocate space for file list
			pFileList = (FILELIST_FileTypeDef *)pvPortMalloc(sizeof(FILELIST_FileTypeDef));
			if(pFileList == NULL)
			{
				printf("List alloc error !!! \r\n");
				GUI_EndDialog(pMsg->hWin, 0);
				break;
			}
			//printf("ptr %08x\r\n",pFileList);

			pFileList->ptr = 0;

			#ifdef USE_POPUP
			hPopUp = WM_CreateWindowAsChild(0,
											26,
											LCD_GetXSize(),
											LCD_GetYSize()-26,
											pMsg->hWin,
											WM_CF_SHOW | WM_CF_HASTRANS ,
											_cbPopup,
											0);
			WM_BringToBottom(hPopUp);
			#endif

			// Background refresh process
			WM_CreateWindowAsChild(479, 250, 1, 1, pMsg->hWin, WM_CF_SHOW | WM_CF_HASTRANS, _cbMediaConnection , 0);
    
			// Initial detect
			_RefreshBrowser(pMsg->hWin);

			break;
		}

		case WM_NOTIFY_PARENT:
		{
			Id    = WM_GetId(pMsg->hWinSrc);    // Id of widget
			NCode = pMsg->Data.v;               // Notification code

			_cbControl(pMsg,Id,NCode);

			switch (NCode)
			{
				case WM_NOTIFICATION_CHILD_DELETED:
				{
					// Free list memory
					file_b_free_memory();

					#ifdef USE_POPUP
					if(hFileInfo != 0)		hFileInfo = 0;
					#endif

					break;
				}
       
				case WM_NOTIFICATION_CLICKED:
				{
#if 1
					if(Id == ID_TREEVIEW)
					{
						char ext[10];

						hTreeView = TREEVIEW_GetSel(pMsg->hWinSrc);
						TREEVIEW_ITEM_GetInfo(hTreeView, &Info);

						hItem = WM_GetDialogItem(pMsg->hWin, ID_BUTTON_OPEN);

						if(Info.IsNode != 0)
						{
							SelectedFileName[0] = 0;
							BUTTON_SetText(hItem, "Details");
							break;
						}

						#ifdef USE_POPUP
						GUI_TOUCH_GetState(&State);
						#endif

						// Get file path
						_FindFullPath(pMsg->hWinSrc, hTreeView, SelectedFileName);
						//printf("path name: %s \r\n", SelectedFileName);

						// Show file details in the right panel
						ShowFileDetails(pMsg->hWin);

						// Need extension to decide button caption
						k_GetExtOnly(SelectedFileName, ext);

						// Change caption
						if(strcmp(ext, "bin") == 0)
							BUTTON_SetText(hItem, "Run");
						else if(strcmp(ext, "ini") == 0)
							BUTTON_SetText(hItem, "Open");

						#ifdef USE_POPUP
						// Open options popup
						//_OpenPopup(hPopUp, _aMenuItems, GUI_COUNTOF(_aMenuItems), State.x, State.y);
						#endif
					}
#endif
					break;
				}
        
				case WM_NOTIFICATION_RELEASED:
				{
					//if(Id == ID_BUTTON_OK_FILEINFO)
					//	GUI_EndDialog(pMsg->hWin, 0);

					break;
				}
			}

			break;
		}

		// Process key messages not supported by ICON_VIEW control
		case WM_KEY:
		{
			switch (((WM_KEY_INFO*)(pMsg->Data.p))->Key)
			{
		        // Return from menu
		        case GUI_KEY_HOME:
		        {
		        	//printf("GUI_KEY_HOME\r\n");
		        	GUI_EndDialog(pMsg->hWin, 0);
		        	break;
		        }
			}
			break;
		}

		default:
		{
			WM_DefaultProc(pMsg);
			break;
		}
	}
}

static void Startup(WM_HWIN hWin, uint16_t xpos, uint16_t ypos)
{
	// Does the current theme require shift of the window ?
	if(menu_layout[ui_s.theme_id].iconview_y == 0)
		goto use_const_decl;

	GUI_WIDGET_CREATE_INFO *p_widget = malloc(sizeof(_aDialog));
	if(p_widget == NULL)
		goto use_const_decl;	// looking ugly is the least of our problems now

	memcpy(p_widget, _aDialog,sizeof(_aDialog));
	p_widget[0].y0 = menu_layout[ui_s.theme_id].iconview_y;	// shift

	hExplorerWin = GUI_CreateDialogBox(p_widget, GUI_COUNTOF(_aDialog), _cbDialog, hWin, xpos, ypos);

	free(p_widget);
	return;

use_const_decl:
	hExplorerWin = GUI_CreateDialogBox(_aDialog, GUI_COUNTOF(_aDialog), _cbDialog, hWin, xpos, ypos);
}
static void KillFileb(void)
{
	//printf("kill browser\r\n");

	// Free list memory
	file_b_free_memory();

	GUI_EndDialog(hExplorerWin, 0);
}

