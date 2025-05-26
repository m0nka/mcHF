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
#include "version.h"

#ifdef CONTEXT_VIDEO

#include "ui_menu_layout.h"
#include "gui.h"
#include "dialog.h"
#include "FreeRTOS.h"
#include "task.h"
#include "radio_init.h"

#include "ui_proc.h"
#include "ui_menu_module.h"

// temp, from digitizer driver
//extern uchar digitizer_info[];
  
// Core unique regs loaded to RAM
//extern struct	CM7_CORE_DETAILS	ccd;

static void Startup(WM_HWIN hWin, uint16_t xpos, uint16_t ypos);
static void KillInfo(void);

// Public radio state
extern struct	TRANSCEIVER_STATE_UI	tsu;

// UI driver public state
extern struct	UI_DRIVER_STATE			ui_s;

// Menu layout definitions from Flash
extern const struct UIMenuLayout menu_layout[];

extern GUI_CONST_STORAGE GUI_BITMAP bmgeneralinfoA;

K_ModuleItem_Typedef  info =
{
  0,
  "About",
  &bmgeneralinfoA,
  Startup,
  NULL,
  KillInfo
};

WM_HWIN   	hIdialog;

#define ID_WINDOW_0               		(GUI_ID_USER + 0x00)

#define ID_BUTTON_FW_UPDATE         	(GUI_ID_USER + 0x01)
#define ID_BUTTON_SYS_RESTART           (GUI_ID_USER + 0x02)
#define ID_BUTTON_EEP_RESET           	(GUI_ID_USER + 0x03)

#define ID_LISTBOX1           			(GUI_ID_USER + 0x04)
#define ID_LISTBOX2           		(GUI_ID_USER + 0x05)

static const GUI_WIDGET_CREATE_INFO _aDialog[] = 
{
// -----------------------------------------------------------------------------------------------------------------------------
//							name		id						x		y		xsize	ysize	?		?		?
// -----------------------------------------------------------------------------------------------------------------------------
// Self
{ WINDOW_CreateIndirect,	"", 		ID_WINDOW_0,			0,    	0,		854,	430, 	0, 		0x64, 	0 },

// Buttons
{ BUTTON_CreateIndirect, 	"Update",	ID_BUTTON_FW_UPDATE,	690, 	55, 	120, 	45, 	0, 		0x0, 	0 },
{ BUTTON_CreateIndirect, 	"Restart",	ID_BUTTON_SYS_RESTART,	690, 	115, 	120, 	45, 	0, 		0x0, 	0 },
{ BUTTON_CreateIndirect, 	"Defaults",	ID_BUTTON_EEP_RESET,	690, 	175, 	120, 	45, 	0, 		0x0, 	0 },

// List boxes
{ LISTBOX_CreateIndirect, 	"Listbox",	ID_LISTBOX1, 		 	5, 	 	10, 	640, 	300, 	0, 		0x0, 	0 },
{ LISTBOX_CreateIndirect, 	"Listbox",	ID_LISTBOX2, 			5, 		320, 	640, 	100, 	0, 		0x0, 	0 },
};

// ------------------------------------------------------
//
// List update state machine
//
#define	LIST_TIMER_RESOLUTION	100
//
WM_HTIMER 						hTimerListFill;
//
extern 	osMessageQId 			hEspMessage;
struct 	ESPMessage				esp_msg_i;
//
ulong 							state_id  = 0xFF;

static void about_print_fw_vers(WM_HWIN hItem)
{
	char fw_id[200];
	char *p = fw_id;

    memset(fw_id,0,sizeof(fw_id));
    sprintf(p,"UI Firmware v: %d.%d.%d.%d",MCHF_R_VER_MAJOR, MCHF_R_VER_MINOR, MCHF_R_VER_RELEASE,MCHF_R_VER_BUILD);

    LISTBOX_AddString(hItem,fw_id);
}

#if 1
static void about_print_fw_auth(WM_HWIN hItem)
{
	char fw_id[200];
	char *p = fw_id;

    memset(fw_id,0,sizeof(fw_id));
    strcpy(p,AUTHOR_STRING);

    LISTBOX_AddString(hItem, fw_id);
    LISTBOX_SetSel(hItem, -1);
}
#endif

static void about_print_fw_rtos(WM_HWIN hItem)
{
	char fw_id[200];
	char *p = fw_id;

    memset(fw_id,0,sizeof(fw_id));
    sprintf(p,"%s FreeRTOS %s","OS:",tskKERNEL_VERSION_NUMBER);

    LISTBOX_AddString(hItem, fw_id);
}

static void about_print_fw_gui(WM_HWIN hItem)
{
	char  fw_id[200];
	char  *p = fw_id;
	ulong hal = HAL_GetHalVersion();

    memset(fw_id,0,sizeof(fw_id));
    sprintf(p,"Misc: HAL v: %d.%d.%d, emWin v %s", (hal >> 24), (hal >> 16)&0xFF, (hal >> 8)&0xFF, GUI_GetVersionString());

    LISTBOX_AddString(hItem, fw_id);
}

static void about_print_fw_dsp(WM_HWIN hItem)
{
	char fw_id[200];

    memset(fw_id,0,sizeof(fw_id));
    sprintf(fw_id,"DSP Firmware v: %d.%d.%d.%d",tsu.dsp_rev1,tsu.dsp_rev2,tsu.dsp_rev3,tsu.dsp_rev4);

    LISTBOX_AddString(hItem, fw_id);
}

static void about_print_fw_cpu_id(WM_HWIN hItem)
{
	char fw_id[200];
	char *p = fw_id;
	ulong chip_rev,dev_id, sn1, sn2, sn3;

	chip_rev 	= HAL_GetREVID();
	dev_id 		= HAL_GetDEVID();
	sn1			= HAL_GetUIDw0();
	sn2			= HAL_GetUIDw1();
	sn3			= HAL_GetUIDw2();

    memset(fw_id,0,sizeof(fw_id));
    sprintf(p,"CPUID: 0x%x(Rev: 0x%x), SN: %x-%x-%x",dev_id, chip_rev, sn1, sn2, sn3);

    LISTBOX_AddString(hItem, fw_id);
}

static void _cbControl(WM_MESSAGE * pMsg, int Id, int NCode)
{
	WM_HWIN hItem;

	switch(Id)
	{
		// -------------------------------------------------------------
		// Update Firmware Button
		case ID_BUTTON_FW_UPDATE:
		{
			switch(NCode)
			{
				case WM_NOTIFICATION_RELEASED:
				{
					printf("update firmware\r\n");

					// Enter reason for reset
					WRITE_REG(BKP_REG_RESET_REASON, RESET_UPDATE_FW);
					HAL_PWR_DisableBkUpAccess();

					// Restart to bootloader
					NVIC_SystemReset();

					break;
				}
			}
			break;
		}

		// -------------------------------------------------------------
		// System Restart
		case ID_BUTTON_SYS_RESTART:
		{
			switch(NCode)
			{
				case WM_NOTIFICATION_RELEASED:
				{
					printf("system restart\r\n");

					vTaskDelay(100);
					NVIC_SystemReset();

					break;
				}
			}
			break;
		}

		// -------------------------------------------------------------
		// Eeprom Defaults
		case ID_BUTTON_EEP_RESET:
		{
			switch(NCode)
			{
				case WM_NOTIFICATION_RELEASED:
				{
					printf("eeprom defaults\r\n");

					// Direct call, no locking
					// ToDo: check if this might be a problem in the future
					//
					radio_init_eep_defaults();

					break;
				}
			}
			break;
		}

		// -------------------------------------------------------------
		default:
			break;
	}
}

static void info_state_machine(WM_MESSAGE * pMsg)
{
	WM_HWIN 	hList;
	char 		buf[200];

	// Is it enabled ?
	if(state_id == 0xFF)
		return;

	hList = WM_GetDialogItem(pMsg->hWin, ID_LISTBOX1);
	LISTBOX_SetSel(hList, -1);

	// Caller finished ?
	if(esp_msg_i.ucProcStatus == TASK_PROC_DONE)
	{
		state_id++;									// next state
		esp_msg_i.ucProcStatus = TASK_PROC_IDLE;		// reset
	}
	else if(esp_msg_i.ucProcStatus == TASK_PROC_WORK)
		return;

	//--printf("state id: %d\r\n", state_id);

	// State machine
	switch(state_id)
	{
		// Radio firmware version
		case 0:
		{
			about_print_fw_vers(hList);
			state_id++;
			break;
		}

		// DSP firmware version
		case 1:
		{
			about_print_fw_dsp(hList);
			state_id++;
			break;
		}

		// OS version
		case 2:
		{
			about_print_fw_rtos(hList);
			state_id++;
			break;
		}

		// OS version
		case 3:
		{
			about_print_fw_gui(hList);
			state_id++;
			break;
		}

		// CPU Speed
		case 4:
		{
			memset(buf, 0, sizeof(buf));
			sprintf(buf, "CPU Clock: %d MHz, DSP Clock: %d MHz",	(int)(HAL_RCC_GetSysClockFreq()/1000000U),
																	(int)(HAL_RCC_GetHCLKFreq()/1000000U));

			LISTBOX_AddString(hList, buf);
			state_id++;
			break;
		}

		case 5:
			about_print_fw_cpu_id(hList);
			state_id++;
			break;

		// ESP32 Firmware version request
		case 6:
		{
			esp_msg_i.ucMessageID = 0x01;
			esp_msg_i.ucProcStatus = TASK_PROC_WORK;
			osMessagePut(hEspMessage, (ulong)&esp_msg_i, osWaitForever);
			break;
		}

		// Result from previous state
		case 7:
		{
			if((esp_msg_i.ucExecResult == 0) && (esp_msg_i.ucDataReady))
			{
				//printf("ui: %s\r\n", (char *)(esp_msg_i.ucData));
				memset(buf, 0, sizeof(buf));
				sprintf(buf,  "ESP32 firmware: %s", (char *)(esp_msg_i.ucData + 10));
				LISTBOX_AddString(hList, buf);
			}
			state_id++;
			break;
		}

		// ESP32 WiFi details
		case 8:
		{
			esp_msg_i.ucMessageID  = 0x02;
			esp_msg_i.ucProcStatus = TASK_PROC_WORK;
			osMessagePut(hEspMessage, (ulong)&esp_msg_i, osWaitForever);
			break;
		}

		// Result from previous state
		case 9:
		{
			if((esp_msg_i.ucExecResult == 0) && (esp_msg_i.ucDataReady) && (esp_msg_i.ucData[9]))
			{
				memset(buf, 0, sizeof(buf));
				sprintf(buf,  "MAC: %02x:%02x:%02x:%02x:%02x:%02x, IP: %d.%d.%d.%d", 	esp_msg_i.ucData[10],
																						esp_msg_i.ucData[11],
																						esp_msg_i.ucData[12],
																						esp_msg_i.ucData[13],
																						esp_msg_i.ucData[14],
																						esp_msg_i.ucData[15],
																						esp_msg_i.ucData[16],
																						esp_msg_i.ucData[17],
																						esp_msg_i.ucData[18],
																						esp_msg_i.ucData[19]);

				LISTBOX_AddString(hList, buf);

				memset(buf, 0, sizeof(buf));
				sprintf(buf, "SSID: %s", (char *)(&esp_msg_i.ucData[21]));
				LISTBOX_AddString(hList, buf);
			}

			state_id++;
			break;
		}

		default:
			break;
	}
}

static void _cbDialog(WM_MESSAGE * pMsg)
{
	//WM_HWIN 			hItem;
	int 				Id, NCode;
	WM_HWIN 			hList;
	SCROLLBAR_Handle 	hScrollV;

	switch (pMsg->MsgId)
	{
		case WM_INIT_DIALOG:
		{
			hList = WM_GetDialogItem(pMsg->hWin, ID_LISTBOX2);
			LISTBOX_SetFont(hList, &GUI_Font24B_ASCII);
			LISTBOX_SetTextColor(hList,LISTBOX_CI_UNSEL, GUI_DARKBLUE);
			hScrollV = SCROLLBAR_CreateAttached(hList, SCROLLBAR_CF_VERTICAL);
			about_print_fw_auth(hList);

			hList = WM_GetDialogItem(pMsg->hWin, ID_LISTBOX1);
			LISTBOX_SetFont(hList, &GUI_Font24B_ASCII);
			LISTBOX_SetTextColor(hList,LISTBOX_CI_UNSEL, GUI_DARKBLUE);
			//hScrollV = SCROLLBAR_CreateAttached(hList, SCROLLBAR_CF_VERTICAL);
			//LISTBOX_SetTextColor(hList,LISTBOX_CI_UNSEL|LISTBOX_CI_SEL|LISTBOX_CI_SELFOCUS|LISTBOX_CI_DISABLED,GUI_BLUE);

			// Start the state machine
			hTimerListFill = WM_CreateTimer(pMsg->hWin, 0, LIST_TIMER_RESOLUTION, 0);
			esp_msg_i.ucProcStatus = TASK_PROC_IDLE;
			state_id  = 0;

			break;
		}

		case WM_TIMER:
		{
			// Next line
			info_state_machine(pMsg);

			// Update state
			if(state_id < 9)
				WM_RestartTimer(pMsg->Data.v, LIST_TIMER_RESOLUTION);

			break;
		}

		case WM_PAINT:
			break;

		case WM_DELETE:
			WM_DeleteTimer(hTimerListFill);
			break;

		case WM_NOTIFY_PARENT:
		{
			Id    = WM_GetId(pMsg->hWinSrc);    // Id of widget
			NCode = pMsg->Data.v;               // Notification code

			_cbControl(pMsg,Id,NCode);
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
			WM_DefaultProc(pMsg);
			break;
	}
}

//*----------------------------------------------------------------------------
//* Function Name       : Startup
//* Object              :
//* Input Parameters    :
//* Output Parameters   :
//* Functions called    :
//*----------------------------------------------------------------------------
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

	//LISTBOX_SetDefaultBkColor(LISTBOX_CI_UNSEL,GUI_STCOLOR_LIGHTBLUE);

	hIdialog = GUI_CreateDialogBox(p_widget, GUI_COUNTOF(_aDialog), _cbDialog, hWin, xpos, ypos);

	free(p_widget);
	return;

use_const_decl:
	hIdialog = GUI_CreateDialogBox(_aDialog, GUI_COUNTOF(_aDialog), _cbDialog, hWin, xpos, ypos);
}

static void KillInfo(void)
{
	//printf("kill menu\r\n");
	GUI_EndDialog(hIdialog, 0);

	//LISTBOX_SetDefaultBkColor(LISTBOX_CI_UNSEL,GUI_WHITE);
}

#endif
