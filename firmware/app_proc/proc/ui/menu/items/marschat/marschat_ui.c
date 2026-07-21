/************************************************************************************
**                                                                                 **
**                                 mcHF QRP Transceiver                            **
**                         Krassi Atanassov - M0NKA, 2013-2026                     **
**                                                                                 **
**---------------------------------------------------------------------------------**
**                                                                                 **
**  File name:		marschat_ui.c                                                  **
**  Description:	MarsChat chat dialog - history + free-text compose (M6 seed)  **
**  Licence:		https://github.com/m0nka/mcHF/blob/main/LICENSE                **
************************************************************************************/
#include "mchf_pro_board.h"
#include "main.h"

#include "ui_proc.h"
#include "gui.h"
#include "dialog.h"
#include "desktop\ui_controls_layout.h"
#include "ui_menu_layout.h"
#include "ui_menu_module.h"

#include "rtc.h"

#include "mc_frame.h"
#include "marschat_proc.h"

#include "marschat_ui.h"

extern GUI_CONST_STORAGE GUI_BITMAP bmicon_gps;

// Menu layout definitions from Flash
extern const struct UIMenuLayout menu_layout[];

// UI driver public state
extern struct	UI_DRIVER_STATE			ui_s;

static void Startup(WM_HWIN hWin, uint16_t xpos, uint16_t ypos);
static void KillMarschat(void);

K_ModuleItem_Typedef  marschat =
{
  11,
  "MarsChat",
  &bmicon_gps,
  Startup,
  NULL,
  KillMarschat
};

WM_HWIN			hMcDialog;

// Free-text compose buffer - typed via the A-P/space/backspace buttons,
// split into MC_PAYLOAD_CHARS chunks and queued on Send (marschat_proc.c
// owns the actual tx queue/timing; this is just the local draft)
#define MC_UI_COMPOSE_MAX	(MARSCHAT_TX_QUEUE_LEN * MC_PAYLOAD_CHARS)

static WM_HTIMER	hMcTimer;
static char			mc_ui_compose[MC_UI_COMPOSE_MAX + 1];
static int			mc_ui_compose_len = 0;
static uint8_t		mc_ui_sending = 0;			// input locked, message in flight
static uint8_t		mc_ui_last_seen_seq = 0;
static uint8_t		mc_ui_last_seen_ack = 0;

static const GUI_WIDGET_CREATE_INFO _aDialog[] =
{
	// -----------------------------------------------------------------------------------------------------------------------------
	//							name						id						x		y		xsize	ysize	?		?		?
	// -----------------------------------------------------------------------------------------------------------------------------
	// Self
	{ WINDOW_CreateIndirect,	"", 						ID_WINDOW_0,			0,		0,		800,	430,	0,		0x64,	0 },
	// History
	{ LISTBOX_CreateIndirect,	"History",					ID_LISTBOX_HISTORY,	10,		5,		780,	110,	0,		0x0,	0 },
	// Status + draft lines
	{ TEXT_CreateIndirect,		"",							ID_TEXT_STATUS,			10,		118,	780,	18,		0,		0,		0 },
	{ TEXT_CreateIndirect,		"",							ID_TEXT_DRAFT_SENT,		10,		138,	780,	20,		0,		0,		0 },
	{ TEXT_CreateIndirect,		"",							ID_TEXT_DRAFT_PENDING,	10,		160,	780,	20,		0,		0,		0 },
	// Char buttons - 4x4 grid, A-P (row major)
	{ BUTTON_CreateIndirect,	"",							ID_BUTTON_CHAR_0 + 0,	10,		184,	185,	40,		0,		0x0,	0 },
	{ BUTTON_CreateIndirect,	"",							ID_BUTTON_CHAR_0 + 1,	205,	184,	185,	40,		0,		0x0,	0 },
	{ BUTTON_CreateIndirect,	"",							ID_BUTTON_CHAR_0 + 2,	400,	184,	185,	40,		0,		0x0,	0 },
	{ BUTTON_CreateIndirect,	"",							ID_BUTTON_CHAR_0 + 3,	595,	184,	185,	40,		0,		0x0,	0 },
	{ BUTTON_CreateIndirect,	"",							ID_BUTTON_CHAR_0 + 4,	10,		228,	185,	40,		0,		0x0,	0 },
	{ BUTTON_CreateIndirect,	"",							ID_BUTTON_CHAR_0 + 5,	205,	228,	185,	40,		0,		0x0,	0 },
	{ BUTTON_CreateIndirect,	"",							ID_BUTTON_CHAR_0 + 6,	400,	228,	185,	40,		0,		0x0,	0 },
	{ BUTTON_CreateIndirect,	"",							ID_BUTTON_CHAR_0 + 7,	595,	228,	185,	40,		0,		0x0,	0 },
	{ BUTTON_CreateIndirect,	"",							ID_BUTTON_CHAR_0 + 8,	10,		272,	185,	40,		0,		0x0,	0 },
	{ BUTTON_CreateIndirect,	"",							ID_BUTTON_CHAR_0 + 9,	205,	272,	185,	40,		0,		0x0,	0 },
	{ BUTTON_CreateIndirect,	"",							ID_BUTTON_CHAR_0 + 10,	400,	272,	185,	40,		0,		0x0,	0 },
	{ BUTTON_CreateIndirect,	"",							ID_BUTTON_CHAR_0 + 11,	595,	272,	185,	40,		0,		0x0,	0 },
	{ BUTTON_CreateIndirect,	"",							ID_BUTTON_CHAR_0 + 12,	10,		316,	185,	40,		0,		0x0,	0 },
	{ BUTTON_CreateIndirect,	"",							ID_BUTTON_CHAR_0 + 13,	205,	316,	185,	40,		0,		0x0,	0 },
	{ BUTTON_CreateIndirect,	"",							ID_BUTTON_CHAR_0 + 14,	400,	316,	185,	40,		0,		0x0,	0 },
	{ BUTTON_CreateIndirect,	"",							ID_BUTTON_CHAR_0 + 15,	595,	316,	185,	40,		0,		0x0,	0 },
	// Control row
	{ BUTTON_CreateIndirect,	"SPACE",					ID_BUTTON_SPACE,		10,		360,	185,	40,		0,		0x0,	0 },
	{ BUTTON_CreateIndirect,	"DEL",						ID_BUTTON_BACKSPACE,	205,	360,	185,	40,		0,		0x0,	0 },
	{ BUTTON_CreateIndirect,	"CLEAR",					ID_BUTTON_CLEAR,		400,	360,	185,	40,		0,		0x0,	0 },
	{ BUTTON_CreateIndirect,	"SEND",						ID_BUTTON_SEND,			595,	360,	185,	40,		0,		0x0,	0 },
};

//*----------------------------------------------------------------------------
//* Function Name       : mc_ui_add_line
//* Object              : append a timestamped line to the history listbox,
//*						: trimming the oldest entries once the cap is hit
//* Context    			: CONTEXT_VIDEO (gui task)
//*----------------------------------------------------------------------------
#define MC_UI_HISTORY_CAP	50

static void mc_ui_add_line(WM_HWIN hWin, const char *dir, const char *text)
{
	WM_HWIN			hItem;
	RTC_TimeTypeDef	tm = {0};
	char			line[MC_UI_COMPOSE_MAX + 16];

	k_GetTime(&tm);

	hItem = WM_GetDialogItem(hWin, ID_LISTBOX_HISTORY);

	snprintf(line, sizeof(line), "%02d:%02d %s: %s", tm.Hours, tm.Minutes, dir, text);

	while(LISTBOX_GetNumItems(hItem) >= MC_UI_HISTORY_CAP)
		LISTBOX_DeleteItem(hItem, 0);

	LISTBOX_AddString(hItem, line);
	LISTBOX_SetSel(hItem, LISTBOX_GetNumItems(hItem) - 1);
}

//*----------------------------------------------------------------------------
//* Function Name       : mc_ui_update_status
//* Object              : refresh the seq/ack/busy status line
//* Context    			: CONTEXT_VIDEO (gui task)
//*----------------------------------------------------------------------------
static void mc_ui_update_status(WM_HWIN hWin)
{
	WM_HWIN	hItem;
	char	line[64];

	hItem = WM_GetDialogItem(hWin, ID_TEXT_STATUS);

	snprintf(line, sizeof(line), "last rx seq %d ack %d  |  %s",
			mc_ui_last_seen_seq, mc_ui_last_seen_ack,
			marschat_tx_busy() ? "TX IN PROGRESS" : "idle");

	TEXT_SetText(hItem, line);
}

//*----------------------------------------------------------------------------
//* Function Name       : mc_ui_set_input_enabled
//* Object              : lock/unlock the char + control buttons while a
//*						: composed message is being drained out
//* Context    			: CONTEXT_VIDEO (gui task)
//*----------------------------------------------------------------------------
static void mc_ui_set_input_enabled(WM_HWIN hWin, int enabled)
{
	static const int ids[] =
	{
		ID_BUTTON_CHAR_0 + 0,  ID_BUTTON_CHAR_0 + 1,  ID_BUTTON_CHAR_0 + 2,  ID_BUTTON_CHAR_0 + 3,
		ID_BUTTON_CHAR_0 + 4,  ID_BUTTON_CHAR_0 + 5,  ID_BUTTON_CHAR_0 + 6,  ID_BUTTON_CHAR_0 + 7,
		ID_BUTTON_CHAR_0 + 8,  ID_BUTTON_CHAR_0 + 9,  ID_BUTTON_CHAR_0 + 10, ID_BUTTON_CHAR_0 + 11,
		ID_BUTTON_CHAR_0 + 12, ID_BUTTON_CHAR_0 + 13, ID_BUTTON_CHAR_0 + 14, ID_BUTTON_CHAR_0 + 15,
		ID_BUTTON_SPACE, ID_BUTTON_BACKSPACE, ID_BUTTON_CLEAR, ID_BUTTON_SEND
	};
	int	i;

	for(i = 0; i < (int)GUI_COUNTOF(ids); i++)
	{
		WM_HWIN	hItem = WM_GetDialogItem(hWin, ids[i]);

		if(enabled)
			WM_EnableWindow(hItem);
		else
			WM_DisableWindow(hItem);
	}
}

//*----------------------------------------------------------------------------
//* Function Name       : mc_ui_update_draft
//* Object              : split the draft line into an already-committed
//*						: part (being sent / already queued as chunks
//*						: consumed from mc_tx_queue) and a still-waiting
//*						: part (diff color), and unlock input once the
//*						: whole message has drained
//* Context    			: CONTEXT_VIDEO (gui task)
//*----------------------------------------------------------------------------
static void mc_ui_update_draft(WM_HWIN hWin)
{
	WM_HWIN		hItem;
	uint16_t	pending = marschat_tx_pending_chunks();
	uint8_t		busy    = marschat_tx_busy();
	int			committed;
	char		buf[MC_UI_COMPOSE_MAX + 8];

	committed = mc_ui_compose_len - (int)pending * MC_PAYLOAD_CHARS;
	if(committed < 0)
		committed = 0;
	if(committed > mc_ui_compose_len)
		committed = mc_ui_compose_len;

	snprintf(buf, sizeof(buf), "tx: %.*s", committed, mc_ui_compose);
	hItem = WM_GetDialogItem(hWin, ID_TEXT_DRAFT_SENT);
	TEXT_SetText(hItem, buf);

	snprintf(buf, sizeof(buf), "..: %.*s", mc_ui_compose_len - committed, mc_ui_compose + committed);
	hItem = WM_GetDialogItem(hWin, ID_TEXT_DRAFT_PENDING);
	TEXT_SetText(hItem, buf);

	// Fully drained - clear the draft and unlock for the next message
	if(mc_ui_sending && !busy && (pending == 0))
	{
		mc_ui_sending      = 0;
		mc_ui_compose_len  = 0;
		mc_ui_compose[0]   = 0;

		mc_ui_set_input_enabled(hWin, 1);

		hItem = WM_GetDialogItem(hWin, ID_TEXT_DRAFT_SENT);
		TEXT_SetText(hItem, "tx:");
		hItem = WM_GetDialogItem(hWin, ID_TEXT_DRAFT_PENDING);
		TEXT_SetText(hItem, "..:");
	}
}

//*----------------------------------------------------------------------------
//* Function Name       : mc_ui_drain_rx
//* Object              : pull any decoded frames queued by the marschat
//*						: task into the history listbox
//* Context    			: CONTEXT_VIDEO (gui task, WM_TIMER poll)
//*----------------------------------------------------------------------------
static void mc_ui_drain_rx(WM_HWIN hWin)
{
	xQueueHandle	q = marschat_rx_queue();
	MC_UI_RX_MSG	m;

	if(q == NULL)
		return;

	while(xQueueReceive(q, &m, 0) == pdPASS)
	{
		mc_ui_last_seen_seq = m.seq;
		mc_ui_last_seen_ack = m.ack;

		mc_ui_add_line(hWin, "rx", m.text);
	}
}

static void _cbControl(WM_MESSAGE * pMsg, int Id, int NCode)
{
	if(NCode != WM_NOTIFICATION_RELEASED)
		return;

	if((Id >= ID_BUTTON_CHAR_0) && (Id < ID_BUTTON_CHAR_0 + 16))
	{
		if((!mc_ui_sending) && (mc_ui_compose_len < MC_UI_COMPOSE_MAX))
		{
			mc_ui_compose[mc_ui_compose_len++] = (char)('A' + (Id - ID_BUTTON_CHAR_0));
			mc_ui_compose[mc_ui_compose_len]   = 0;
			mc_ui_update_draft(pMsg->hWin);
		}
		return;
	}

	switch(Id)
	{
		case ID_BUTTON_SPACE:
		{
			if((!mc_ui_sending) && (mc_ui_compose_len < MC_UI_COMPOSE_MAX))
			{
				mc_ui_compose[mc_ui_compose_len++] = ' ';
				mc_ui_compose[mc_ui_compose_len]   = 0;
				mc_ui_update_draft(pMsg->hWin);
			}
			break;
		}

		case ID_BUTTON_BACKSPACE:
		{
			if((!mc_ui_sending) && (mc_ui_compose_len > 0))
			{
				mc_ui_compose[--mc_ui_compose_len] = 0;
				mc_ui_update_draft(pMsg->hWin);
			}
			break;
		}

		case ID_BUTTON_CLEAR:
		{
			if(!mc_ui_sending)
			{
				mc_ui_compose_len = 0;
				mc_ui_compose[0]  = 0;
				mc_ui_update_draft(pMsg->hWin);
			}
			break;
		}

		case ID_BUTTON_SEND:
		{
			uint8_t	err;

			if(mc_ui_sending || (mc_ui_compose_len == 0))
				break;

			err = marschat_ui_send_text(mc_ui_compose);

			if((err == 0) || (err == 4))
			{
				mc_ui_add_line(pMsg->hWin, "tx", mc_ui_compose);
				mc_ui_sending = 1;
				mc_ui_set_input_enabled(pMsg->hWin, 0);
			}
			else
			{
				mc_ui_add_line(pMsg->hWin, "!!", "send queue full, try again");
			}

			mc_ui_update_draft(pMsg->hWin);
			mc_ui_update_status(pMsg->hWin);
			break;
		}

		default:
			break;
	}
}

static void _cbDialog(WM_MESSAGE * pMsg)
{
	WM_HWIN	hItem;
	int		Id, NCode;
	int		i;

	switch (pMsg->MsgId)
	{
		case WM_INIT_DIALOG:
		{
			hItem = WM_GetDialogItem(pMsg->hWin, ID_LISTBOX_HISTORY);
			LISTBOX_SetFont(hItem, &GUI_Font20B_1);

			hItem = WM_GetDialogItem(pMsg->hWin, ID_TEXT_STATUS);
			TEXT_SetFont(hItem, &GUI_Font13B_1);
			TEXT_SetTextColor(hItem, GUI_WHITE);

			hItem = WM_GetDialogItem(pMsg->hWin, ID_TEXT_DRAFT_SENT);
			TEXT_SetFont(hItem, &GUI_Font13B_1);
			TEXT_SetTextColor(hItem, GUI_WHITE);
			TEXT_SetText(hItem, "tx:");

			hItem = WM_GetDialogItem(pMsg->hWin, ID_TEXT_DRAFT_PENDING);
			TEXT_SetFont(hItem, &GUI_Font13B_1);
			TEXT_SetTextColor(hItem, GUI_GRAY);
			TEXT_SetText(hItem, "..:");

			for(i = 0; i < 16; i++)
			{
				char	label[2];

				label[0] = (char)('A' + i);
				label[1] = 0;

				hItem = WM_GetDialogItem(pMsg->hWin, ID_BUTTON_CHAR_0 + i);
				BUTTON_SetFont(hItem, &GUI_Font16B_1);
				BUTTON_SetText(hItem, label);
			}

			mc_ui_compose_len = 0;
			mc_ui_compose[0]  = 0;
			mc_ui_sending     = 0;

			mc_ui_update_status(pMsg->hWin);

			hMcTimer = WM_CreateTimer(pMsg->hWin, 0, 500, 0);
			break;
		}

		case WM_TIMER:
		{
			mc_ui_drain_rx(pMsg->hWin);
			mc_ui_update_draft(pMsg->hWin);
			mc_ui_update_status(pMsg->hWin);
			WM_RestartTimer(pMsg->Data.v, 500);
			break;
		}

		case WM_DELETE:
		{
			WM_DeleteTimer(hMcTimer);
			break;
		}

		case WM_NOTIFY_PARENT:
		{
			Id    = WM_GetId(pMsg->hWinSrc);
			NCode = pMsg->Data.v;

			_cbControl(pMsg, Id, NCode);
			break;
		}

		case WM_KEY:
		{
			switch (((WM_KEY_INFO*)(pMsg->Data.p))->Key)
			{
				case GUI_KEY_HOME:
					GUI_EndDialog(pMsg->hWin, 0);
					break;
			}
			break;
		}

		default:
			WM_DefaultProc(pMsg);
			break;
	}
}

static void Startup(WM_HWIN hWin, uint16_t xpos, uint16_t ypos)
{
	if(menu_layout[ui_s.theme_id].iconview_y == 0)
		goto use_const_decl;

	GUI_WIDGET_CREATE_INFO *p_widget = malloc(sizeof(_aDialog));
	if(p_widget == NULL)
		goto use_const_decl;

	memcpy(p_widget, _aDialog, sizeof(_aDialog));
	p_widget[0].y0 = menu_layout[ui_s.theme_id].iconview_y;

	hMcDialog = GUI_CreateDialogBox(p_widget, GUI_COUNTOF(_aDialog), _cbDialog, hWin, xpos, ypos);

	free(p_widget);
	return;

use_const_decl:
	hMcDialog = GUI_CreateDialogBox(_aDialog, GUI_COUNTOF(_aDialog), _cbDialog, hWin, xpos, ypos);
}

static void KillMarschat(void)
{
	GUI_EndDialog(hMcDialog, 0);
}
