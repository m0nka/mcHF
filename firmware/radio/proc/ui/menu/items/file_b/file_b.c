/**
  ******************************************************************************
  * @file    filebrowser_win.c
  * @author  MCD Application Team
  * @brief   File browser functions
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2017 STMicroelectronics International N.V. 
  * All rights reserved.</center></h2>
  *
  * Redistribution and use in source and binary forms, with or without 
  * modification, are permitted, provided that the following conditions are met:
  *
  * 1. Redistribution of source code must retain the above copyright notice, 
  *    this list of conditions and the following disclaimer.
  * 2. Redistributions in binary form must reproduce the above copyright notice,
  *    this list of conditions and the following disclaimer in the documentation
  *    and/or other materials provided with the distribution.
  * 3. Neither the name of STMicroelectronics nor the names of other 
  *    contributors to this software may be used to endorse or promote products 
  *    derived from this software without specific written permission.
  * 4. This software, including modifications and/or derivative works of this 
  *    software, must execute solely and exclusively on microcontroller or
  *    microprocessor devices manufactured by or for STMicroelectronics.
  * 5. Redistribution and use of this software other than as permitted under 
  *    this license is void and will automatically terminate your rights under 
  *    this license. 
  *
  * THIS SOFTWARE IS PROVIDED BY STMICROELECTRONICS AND CONTRIBUTORS "AS IS" 
  * AND ANY EXPRESS, IMPLIED OR STATUTORY WARRANTIES, INCLUDING, BUT NOT 
  * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A 
  * PARTICULAR PURPOSE AND NON-INFRINGEMENT OF THIRD PARTY INTELLECTUAL PROPERTY
  * RIGHTS ARE DISCLAIMED TO THE FULLEST EXTENT PERMITTED BY LAW. IN NO EVENT 
  * SHALL STMICROELECTRONICS OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
  * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
  * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, 
  * OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF 
  * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING 
  * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
  * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
  *
  ******************************************************************************
  */
/* Includes ------------------------------------------------------------------*/

#include "mchf_pro_board.h"

#include "gui.h"
#include "dialog.h"
//#include "ST_GUI_Addons.h"

#ifdef CONTEXT_SD

#include "k_module.h"

#include "browser\filebrowser_app.h"
#include "storage.h"

// ToDo: clean up this path
#include "ff_gen_drv.h"

#define ID_WINDOW_0            		(GUI_ID_USER + 0x00)
#define ID_BUTTON_EXIT          	(GUI_ID_USER + 0x01)

#define ID_BUTTON_REFRESH    		(GUI_ID_USER + 0x02)
#define ID_TREEVIEW          		(GUI_ID_USER + 0x03)
#define ID_EDIT_MSD 	       		(GUI_ID_USER + 0x04)
#define ID_PROGBAR_MSD       		(GUI_ID_USER + 0x05)

#define ID_MENU_OPENFILE     		(GUI_ID_USER + 0x09)
#define ID_MENU_DELETE       		(GUI_ID_USER + 0x0A)
#define ID_MENU_EXIT         		(GUI_ID_USER + 0x0B)
#define ID_MENU_PROPRIETIES  		(GUI_ID_USER + 0x0C)

#define ID_FRAMEWIN_0				(GUI_ID_USER + 0x0D)
#define ID_TEXT_SOLID_LOCATION     	(GUI_ID_USER + 0x0E)
#define ID_TEXT_SOLID_CREATION     	(GUI_ID_USER + 0x0F)
#define ID_TEXT_LOCATION           	(GUI_ID_USER + 0x10)
#define ID_TEXT_CREATION           	(GUI_ID_USER + 0x11)
#define ID_TEXT_SOLID_FILESIZE     	(GUI_ID_USER + 0x12)
#define ID_TEXT_FILESIZE         	(GUI_ID_USER + 0x13)
#define ID_BUTTON_OK_FILEINFO  		(GUI_ID_USER + 0x14)
#define ID_TEXT_FILENAME           	(GUI_ID_USER + 0x15)

#define WM_FORCE_ITEM_DESELECT     	(WM_USER + 0)

//extern char USBDISK_Drive[];
extern char mSDDISK_Drive[];  

FILELIST_FileTypeDef  *pFileList;

WM_HWIN hExplorerWin = 0;
WM_HWIN hPopUp = 0;
WM_HWIN hFileInfo = 0;

uint16_t FolderLevel  = 0;
char SelectedFileName[FILEMGR_FULL_PATH_SIZE];
char    str[FILEMGR_FILE_NAME_SIZE];

static void Startup(WM_HWIN hWin, uint16_t xpos, uint16_t ypos);
static void _RefreshBrowser ( WM_HWIN hWin);

// Menu image
extern GUI_CONST_STORAGE GUI_BITMAP bmProgramGroup;
// TreeView images
extern GUI_CONST_STORAGE GUI_BITMAP bmClosedFolder;
extern GUI_CONST_STORAGE GUI_BITMAP bmOpenFolder;
extern GUI_CONST_STORAGE GUI_BITMAP bmTextLog;

K_ModuleItem_Typedef  file_b =
{
  3,
  "File Browser",
  &bmProgramGroup,
  Startup,
  NULL,
};

//WM_HWIN  hBrowser = 0;

static const GUI_WIDGET_CREATE_INFO _aDialog[] = 
{
	// -----------------------------------------------------------------------------------------------------------------------------
	//							name						id					x		y		xsize	ysize	?		?		?
	// -----------------------------------------------------------------------------------------------------------------------------
	// Self
	{ WINDOW_CreateIndirect,	"",							ID_WINDOW_0,		0,    	0,		800,	430, 	0, 		0x64,	0 },
	// Back Button
	{ BUTTON_CreateIndirect, 	"Back",			 			ID_BUTTON_EXIT, 	670, 	375, 	120, 	45, 	0, 		0x0, 	0 },
	//
	{ BUTTON_CreateIndirect, 	"Refresh", 					ID_BUTTON_REFRESH, 	670, 	310, 	120, 	45, 	0, 		0x0, 	0 },
	{ TREEVIEW_CreateIndirect, 	"Treeview", 				ID_TREEVIEW, 		7, 		6, 		635, 	380, 	0, 		0x0, 	0 },
	//
	{ EDIT_CreateIndirect,     	"msd.Edit",    				ID_EDIT_MSD,   		7,  	395,  	100,  	25, 	0,		0x0,	0 	},
	{ PROGBAR_CreateIndirect, 	"Progbar", 					ID_PROGBAR_MSD, 	117,	395, 	525, 	25, 	0, 		0x0, 	0 },
};

static const GUI_WIDGET_CREATE_INFO _aFileInfoDialogCreate[] =
{
	// -----------------------------------------------------------------------------------------------------------------------------
	//							name						id					x		y		xsize	ysize	?		?		?
	// -----------------------------------------------------------------------------------------------------------------------------
	// Self
	{ FRAMEWIN_CreateIndirect, "File Information", ID_FRAMEWIN_0, 0, 0, 280, 130, 0, 0x0, 0 },
	{ TEXT_CreateIndirect, 		"file name", ID_TEXT_FILENAME, 10, 5, 300, 20, 0, 0x0, 0 },
	{ TEXT_CreateIndirect, 		"Location: ", ID_TEXT_SOLID_LOCATION, 10, 25, 51, 20, 0, 0x0, 0 },
	{ TEXT_CreateIndirect, 		"", ID_TEXT_LOCATION, 59, 25, 300, 20, 0, 0x0, 0 },
	{ TEXT_CreateIndirect, 		"Created on : ", ID_TEXT_SOLID_CREATION, 10, 40, 80, 20, 0, 0x0, 0 },
	{ TEXT_CreateIndirect, 		"", ID_TEXT_CREATION, 74, 40, 300, 20, 0, 0x0, 0 },
	{ TEXT_CreateIndirect, 		"File Size : ", ID_TEXT_SOLID_FILESIZE, 10, 55, 80, 20, 0, 0x0, 0 },
	{ TEXT_CreateIndirect, 		"", ID_TEXT_FILESIZE, 57, 55, 300, 20, 0, 0x0, 0 },
	{ BUTTON_CreateIndirect, 	"OK", ID_BUTTON_OK_FILEINFO, 90, 81, 80, 26, 0, 0x0, 0 },
};

/* Array of menu items */
static MENU_ITEM _aMenuItems[] = 
{
  {"Open File"          , ID_MENU_OPENFILE,  0},
  {"Delete File"        , ID_MENU_DELETE, 0},
  {"Properties"         , ID_MENU_PROPRIETIES, 0},
  {0                    , 0           ,  MENU_IF_SEPARATOR},
  {0                    , 0           ,  MENU_IF_SEPARATOR},
  {0                    , 0           ,  MENU_IF_SEPARATOR},
  {"Cancel"             , ID_MENU_EXIT, 0},
};

MENU_Handle hMenu;

/**
  * @brief  Adds one menu item to the given menu
  * @param  hMenu:    pointer to the handle of menu
  * @param  hSubmenu: pointer to the handle of Sub menu
  * @param  pText:    pointer to menu item description
  * @param  Id:       ID of the menu item
  * @param  Flags:    window creation flags
  * @retval None
  */
static void _AddMenuItem(MENU_Handle hMenu, MENU_Handle hSubmenu, const char* pText, U16 Id, U16 Flags) {
  MENU_ITEM_DATA Item;
  Item.pText    = pText;
  Item.hSubmenu = hSubmenu;
  Item.Flags    = Flags;
  Item.Id       = Id;
  MENU_AddItem(hMenu, &Item);
}

/**
  * @brief  Opens a popup menu at the given position.
  * @note   It returns immediately after creation. 
  *         On the first call it creates the menu
  * @param  hParent:    pointer to the handle of the parent
  * @param  pMenuItems: pointer to menu items 
  * @param  NumItems:   number of menu items 
  * @param  x:          x position of the popup
  * @param  y:          y position of the popup 
  * @retval None
  */
static void _OpenPopup(WM_HWIN hParent, MENU_ITEM * pMenuItems, int NumItems, int x, int y) 
{
#if 0
  printf("_OpenPopup\r\n");

  if(!hMenu)
  {
    int i;

    printf("_OpenPopup create\r\n");

    /* Create the popup window only one time */
    hMenu = MENU_CreateEx(0, 0, 0, 0, WM_UNATTACHED, 0, MENU_CF_VERTICAL, 0);
    MENU_SetBkColor(hMenu, MENU_CI_SELECTED, GUI_LIGHTBLUE);
    MENU_SetFont(hMenu,GUI_FONT_32B_ASCII);

    for (i = 0; i < NumItems; i++) {
      _AddMenuItem(hMenu, 0, pMenuItems[i].sText, pMenuItems[i].Id, pMenuItems[i].Flags);
    }

  }

  /* Open the popup menu. After opening the menu the function returns immediately.
   * After selecting menu item or after touching the display outside the menu the
   * popup menu will be closed, but not deleted.
   */
  MENU_Popup(hMenu, hParent, x, y, 0, 0, 0);
#endif
}

/**
  * @brief  Callback routine of Info dialog
  * @param  pMsg: pointer to data structure of type WM_MESSAGE 
  * @retval None
  */
static void _cbFileInfoDialog(WM_MESSAGE * pMsg) {
  WM_HWIN hItem;
  int     NCode;
  int     Id;
  FILINFO fno;
 
  switch (pMsg->MsgId) {
  case WM_INIT_DIALOG:

    hItem = pMsg->hWin;
    FRAMEWIN_SetClientColor   (hItem, GUI_WHITE);
    FRAMEWIN_SetFont          (hItem, &GUI_Font16B_ASCII);
    FRAMEWIN_SetTextAlign     (hItem, GUI_TA_HCENTER);
  
    hItem = WM_GetDialogItem(pMsg->hWin, ID_TEXT_FILENAME);
    FILEMGR_GetFileOnly(str, SelectedFileName);
    TEXT_SetText(hItem, str);
    
    hItem = WM_GetDialogItem(pMsg->hWin, ID_TEXT_LOCATION);
    if(SelectedFileName[0] == '0')
    {
    //  TEXT_SetText(hItem, "[USB Disk]");
    //}
    //else if(SelectedFileName[0] == '1')
    //{
      TEXT_SetText(hItem, "[SD Card]");
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
    
    break;
  case WM_NOTIFY_PARENT:
    Id    = WM_GetId(pMsg->hWinSrc);
    NCode = pMsg->Data.v;
    switch(Id) {
    case ID_BUTTON_OK_FILEINFO: /* Notifications sent by 'OK' */
      switch(NCode) {
      case WM_NOTIFICATION_RELEASED:
        GUI_EndDialog(pMsg->hWin, 0); 
        hFileInfo = 0;
        break;
      }
      break;
    }
    break;
  default:
    WM_DefaultProc(pMsg);
    break;
  }
}

/**
  * @brief  Callback routine for informing user about exploring disk
  * @param  pMsg: pointer to data structure of type WM_MESSAGE
  * @retval None
  */
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
    GUI_DispStringHCenterAt("Populating Tree view...", 110 , 10);
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

/**
  * @brief  callback for Message Box 
  * @param  pMsg : pointer to data structure
  * @retval None
  */
static void _cbMessageBox(WM_MESSAGE* pMsg) {
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

/**
  * @brief  Show Message Box
  * @param  hWin:   pointer to the parent handle
  * @param  pTitle: pointer to the title
  * @param  pText:  pointer to the text
  * @retval int 
  */ 
static int _ShowMessageBox(WM_HWIN hWin, const char* pTitle, const char* pText, int YesNo) 
{
  WM_HWIN hFrame, hClient, hBut;
  int r = 0;
  /* Create frame win */
  hFrame = FRAMEWIN_CreateEx(0, 0, 390, 290, hWin, WM_CF_SHOW, FRAMEWIN_CF_MOVEABLE, 0, pTitle, &_cbMessageBox);
  FRAMEWIN_SetClientColor   (hFrame, GUI_WHITE);
  FRAMEWIN_SetFont          (hFrame, &GUI_Font16B_ASCII);
  FRAMEWIN_SetTextAlign     (hFrame, GUI_TA_HCENTER);
  /* Create dialog items */
  hClient = WM_GetClientWindow(hFrame);
  TEXT_CreateEx(10, 7, 370, 230, hClient, WM_CF_SHOW, GUI_TA_HCENTER, 0, pText);

  if (YesNo) {
    hBut = BUTTON_CreateEx(97, 45, 55, 18, hClient, WM_CF_SHOW, 0, GUI_ID_CANCEL);
    BUTTON_SetText        (hBut, "No");
    hBut = BUTTON_CreateEx(32, 45, 55, 18, hClient, WM_CF_SHOW, 0, GUI_ID_OK);
    BUTTON_SetText        (hBut, "Yes");
  } else {
    hBut = BUTTON_CreateEx(64, 45, 55, 18, hClient, WM_CF_SHOW, 0, GUI_ID_OK);
    BUTTON_SetText        (hBut, "Ok");
  }
  
  WM_SetFocus(hFrame);  
  WM_MakeModal(hFrame);
  r = GUI_ExecCreatedDialog(hFrame);  
  return r;
}

/**
  * @brief  Callback routine of popup menu.
  * @param  pMsg: pointer to data structure of type WM_MESSAGE 
  * @retval None
  */
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
			printf("popup init\r\n");
			break;
		}

		case WM_MENU:
		{
			printf("popup menu\r\n");

			pData = (MENU_MSG_DATA*)pMsg->Data.p;
			switch (pData->MsgType)
			{
				case MENU_ON_ITEMSELECT:
				case MENU_ON_ITEMPRESSED:
				{
					printf("popup item select\r\n");

					switch (pData->ItemId)
					{
						case ID_MENU_OPENFILE:
						{
							printf("popup open file\r\n");
							k_GetExtOnly(SelectedFileName, ext);
							pfOpen = k_ModuleCheckLink(ext);

							if(pfOpen != NULL)
							{
								pfOpen(SelectedFileName);
							}
							else
							{
								_ShowMessageBox(hExplorerWin, "File Browser", "No external module is linked to\n this extension!", 0);
							}
							break;
						}

						case ID_MENU_DELETE:
						{
							printf("popup delete file\r\n");
							if (_ShowMessageBox(hExplorerWin, "File Browser", "Are you sure you want to\ndelete selected file?", 1))
							{
								f_unlink (SelectedFileName);
								_RefreshBrowser(hExplorerWin);
							}
							break;
						}

						case ID_MENU_PROPRIETIES:
						{
							printf("popup properties\r\n");

							if(hFileInfo == 0)
							{
								hFileInfo = GUI_CreateDialogBox(_aFileInfoDialogCreate,
																GUI_COUNTOF(_aFileInfoDialogCreate),
																_cbFileInfoDialog,
																hExplorerWin,
																100,
																75);
							}
							break;
						}

						case ID_MENU_EXIT:
						{
							printf("popup exit\r\n");
							break;
						}

						default:
							//WM_DefaultProc(pMsg);
							break;
					}

					/*hItem = WM_GetDialogItem(hExplorerWin, ID_TREEVIEW);
					hTreeView = TREEVIEW_GetSel(hItem);
					TREEVIEW_ITEM_GetInfo(hTreeView, &Info);
					if(Info.IsNode == 0)
					{
						hTreeView = TREEVIEW_GetItem(hItem, hTreeView, TREEVIEW_GET_PARENT);
						TREEVIEW_SetSel(hItem, hTreeView);
					}*/
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

/**
  * @brief  Finds full path of selected file.
  * @param  hObj:    object handle
  * @param  hTVItem: window handle
  * @param  str: Pointer to str
  * @retval None
  */
void _FindFullPath(TREEVIEW_Handle hObj, TREEVIEW_ITEM_Handle hTVItem, char *str)
{
  TREEVIEW_ITEM_INFO hInfo;
  char strtmp[FILEMGR_FULL_PATH_SIZE];
  uint8_t Level = FILEMGR_MAX_LEVEL;
  
  /* Find File name */
  TREEVIEW_ITEM_GetText(hTVItem, (uint8_t *)str, FILEMGR_FULL_PATH_SIZE);

  /* Find folders */
  while( Level > 1)
  {
    TREEVIEW_ITEM_GetInfo(hTVItem, &hInfo);
    hTVItem = TREEVIEW_GetItem(hObj, hTVItem,TREEVIEW_GET_PARENT);
    TREEVIEW_ITEM_GetText(hTVItem, (uint8_t *)strtmp, FILEMGR_FULL_PATH_SIZE);
    
    if(strcmp(strtmp, "SD Card") == 0)
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
}

/**
  * @brief  Shows node content.
  * @param  hTree: Tree view handle
  * @param  hNode: Tree Node handle
  * @param  list: pointer to file list structure
  * @retval None
  */
static void ShowNodeContent(WM_HWIN hTree, TREEVIEW_ITEM_Handle hNode, char *path, FILELIST_FileTypeDef *list) 
{
  uint32_t i = 0, Position = 0;
  TREEVIEW_ITEM_Handle hItem = 0;
  FILEMGR_ParseDisks(path, list); 
  char fullpath[FILEMGR_FULL_PATH_SIZE];

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
	uint32_t 				Position = 0;

	Node = TREEVIEW_InsertItem(hTree, TREEVIEW_ITEM_TYPE_NODE, 0, 0, "Radio");
  
	if(k_StorageGetStatus(MSD_DISK_UNIT) == 1)
	{
		hItem = TREEVIEW_InsertItem(hTree, TREEVIEW_ITEM_TYPE_NODE, Node, TREEVIEW_INSERT_FIRST_CHILD, "SD Card");
		ShowNodeContent(hTree, hItem, mSDDISK_Drive, pFileList);
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

/**
  * @brief  Refresh browser.
  * @param  hWin: pointer to the parent handle
  * @retval None
  */
static void _RefreshBrowser ( WM_HWIN hWin)
{
	WM_HWIN 				hItem, Hint;
	TREEVIEW_ITEM_Handle  	hTreeView;
	uint32_t 				free, total,perc;
	char 					str[FILEMGR_FULL_PATH_SIZE];
  
	//GUI_Exec();
  
	// Show Hint
	//Hint = WM_CreateWindowAsChild(80, 120, 200, 32, hWin, WM_CF_SHOW, _cbHint, 0);
  
	//GUI_Exec();

  hItem = WM_GetDialogItem(hWin, ID_PROGBAR_MSD);
  if(k_StorageGetStatus (MSD_DISK_UNIT))
  {
    free =  k_StorageGetFree(MSD_DISK_UNIT);
    total = k_StorageGetCapacity(MSD_DISK_UNIT);

    //printf("free: %d\r\n",free);
    //printf("total: %d\r\n",total);

    perc = ((total - free)*100)/total;

    if(((total - free)*100)%total)
    	perc++;

    //printf("perc: %d\r\n",perc);

    //PROGBAR_SetMinMax(hItem, 0, 100);
    PROGBAR_SetValue (hItem,perc);
    hItem = WM_GetDialogItem(hWin, ID_EDIT_MSD);
    sprintf(str, "%d MB", total / (2 * 1024));
    EDIT_SetText(hItem, str);
  }
  else
  {
    PROGBAR_SetValue (hItem, 0); 
    hItem = WM_GetDialogItem(hWin, ID_EDIT_MSD);
    EDIT_SetText(hItem, "[N/A]" );
  }
  
  hTreeView = WM_GetDialogItem(hWin, ID_TREEVIEW);
  hItem = TREEVIEW_GetItem(hTreeView, 0, TREEVIEW_GET_FIRST);
  if(hItem != 0)
  {
    TREEVIEW_ITEM_Delete (hItem);
  }

  ExploreDisks(hTreeView);  
  //WM_DeleteWindow(Hint);
}


/**
  * @brief  Callback function of the media connection status
  * @param  pMsg: pointer to data structure of type WM_MESSAGE
  * @retval None
  */
static void _cbMediaConnection(WM_MESSAGE * pMsg) 
{
  
  static WM_HTIMER      hStatusTimer;  
  static uint8_t        prev_sd_status = 0;
  //static uint8_t        prev_usb_status = 0;
   
  switch (pMsg->MsgId) 
  {
  case WM_CREATE:
    prev_sd_status = k_StorageGetStatus(MSD_DISK_UNIT);
    //prev_usb_status = k_StorageGetStatus(USB_DISK_UNIT);
    hStatusTimer = WM_CreateTimer(pMsg->hWin, 0, 500, 0);      
    break;
    
  case WM_TIMER:
    if(prev_sd_status != k_StorageGetStatus(MSD_DISK_UNIT))
    {
      prev_sd_status = k_StorageGetStatus(MSD_DISK_UNIT);
      _RefreshBrowser(hExplorerWin);
    }
    //else if(prev_usb_status != k_StorageGetStatus(USB_DISK_UNIT))
    //{
      //prev_usb_status = k_StorageGetStatus(USB_DISK_UNIT);
      //_RefreshBrowser(hExplorerWin);
    //}
    WM_RestartTimer(pMsg->Data.v, 500);
    break;
    
  case WM_DELETE:
    if(hStatusTimer != 0)
    {
      WM_DeleteTimer(hStatusTimer);
      hStatusTimer = 0;
    }
    break;   
    
  default:
    WM_DefaultProc(pMsg);
  }
}

static void _cbControl(WM_MESSAGE * pMsg, int Id, int NCode)
{
	WM_HWIN hItem;

	switch(Id)
	{
		// -------------------------------------------------------------
		// Button
		case ID_BUTTON_EXIT:
		{
			switch(NCode)
			{
				case WM_NOTIFICATION_RELEASED:
					GUI_EndDialog(pMsg->hWin, 0);
					break;
			}
			break;
		}

		case ID_BUTTON_REFRESH:
			_RefreshBrowser (pMsg->hWin);
			break;

		// -------------------------------------------------------------
		default:
			break;
	}
}

static void _cbDialog(WM_MESSAGE * pMsg)
{
	WM_HWIN 				hItem;
	int 					Id, NCode;
	TREEVIEW_ITEM_Handle  	hTreeView;
	TREEVIEW_ITEM_INFO    	Info;
	GUI_PID_STATE 			State;

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
			hItem = WM_GetDialogItem(pMsg->hWin, ID_EDIT_MSD);
			EDIT_SetFont(hItem,&GUI_FontAvantGarde16B);
			EDIT_SetBkColor(hItem,EDIT_CI_ENABLED,GUI_STCOLOR_LIGHTBLUE);
			EDIT_SetTextColor(hItem,EDIT_CI_ENABLED,GUI_WHITE);
			EDIT_SetTextAlign(hItem,TEXT_CF_HCENTER|TEXT_CF_VCENTER);

			pFileList = (FILELIST_FileTypeDef *)pvPortMalloc(sizeof(FILELIST_FileTypeDef));
			//printf("ptr %08x\r\n",pFileList);

			pFileList->ptr = 0;

			hPopUp = WM_CreateWindowAsChild(0,
											26,
											LCD_GetXSize(),
											LCD_GetYSize()-26,
											pMsg->hWin,
											WM_CF_SHOW | WM_CF_HASTRANS ,
											_cbPopup,
											0);
    
			WM_BringToBottom(hPopUp);

			WM_CreateWindowAsChild(479, 250, 1, 1, pMsg->hWin, WM_CF_SHOW | WM_CF_HASTRANS, _cbMediaConnection , 0);
    
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
					if(pFileList == NULL) 	vPortFree(pFileList);
					if(hFileInfo != 0)		hFileInfo = 0;
					break;
				}
       
				case WM_NOTIFICATION_CLICKED:
				{
					if(Id == ID_TREEVIEW)
					{
						hTreeView = TREEVIEW_GetSel(pMsg->hWinSrc);
						TREEVIEW_ITEM_GetInfo(hTreeView, &Info);
						if(Info.IsNode == 0)
						{
							//printf("treeview\r\n");

							// Causes the screen to go mad!!!
							//--GUI_TOUCH_GetState(&State);

							// Test
							State.x = 0;
							State.y = 0;

							State.x += 20;
							State.y-= 50;
							if(State.y > 150) State.y -= 70;
						}
						_FindFullPath(pMsg->hWinSrc, hTreeView, SelectedFileName);
						_OpenPopup(hPopUp,_aMenuItems,GUI_COUNTOF(_aMenuItems),State.x,State.y);
					}

					break;
				}
        
				//case ID_BUTTON_REFRESH:
				//	_RefreshBrowser (pMsg->hWin);
				//	break;

				case WM_NOTIFICATION_RELEASED:
					if(Id == ID_BUTTON_EXIT) GUI_EndDialog(pMsg->hWin, 0);
					break;
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
	pFileList = NULL;

	#ifdef CONTEXT_SD
	hExplorerWin = GUI_CreateDialogBox(_aDialog, GUI_COUNTOF(_aDialog), _cbDialog, hWin, xpos, ypos);
	#endif
}

#endif
