/************************************************************************************
**                                                                                 **
**                                 mcHF QRP Transceiver                            **
**                         Krassi Atanassov - M0NKA, 2013-2026                     **
**                                                                                 **
**---------------------------------------------------------------------------------**
**                                                                                 **
**  File name:		meshcore_ui.c                                                  **
**  Description:	MeshCore chat dialog - channels, direct messages, contacts     **
**  Licence:		https://github.com/m0nka/mcHF/blob/main/LICENSE                **
************************************************************************************/
//
// A top level screen (MODE_DESKTOP_MESHCORE), built the same way as the
// FT8 and MarsChat desktops: the dialog is a child of the desktop window
// with nothing else on screen, so nothing repaints over it.
//
// Two panes and two views. In the chat view the left pane lists the
// conversations - every channel we hold a key for, then every saved
// contact - and the right pane the selected conversation's history. The
// contacts view reuses both panes: everyone heard advertising on the
// left, the selected node's detail on the right, with ADD promoting it
// to a real contact.
//
// The dialog owns no protocol state. It polls meshcore_revision() twice
// a second and rebuilds only when the service says something moved
//
#include "mchf_pro_board.h"
#include "main.h"

#include "ui_proc.h"
#include "gui.h"
#include "dialog.h"
#include "SCROLLBAR.h"
#include "desktop\ui_controls_layout.h"

#include "rtc.h"

#include "sd_card.h"						// is a card even in the slot ?
#include "lora_radio.h"						// modem settings for the title strip

#include "advert.h"							// meshcore_device_role_t names
#include "mc_identity.h"
#include "mc_contacts.h"
#include "meshcore_proc.h"

#include "meshcore_ui.h"

// UI driver public state
extern struct	UI_DRIVER_STATE			ui_s;
extern struct	PROC_STATE				ps;

static WM_HWIN	hMxDialog;
static WM_HWIN	hMxTimer;

static WM_HWIN	hMxConvList;
static WM_HWIN	hMxMsgList;

// ---------------------------------------------------------------------
// Palette
//
// GUI_USE_ARGB is 1 in this build, so every custom colour has to go
// through GUI_MAKE_COLOR or it draws fully transparent, and the value is
// 0x00BBGGRR rather than RGB.
//
// Deliberately low contrast: black on white is punishing on a backlit
// panel you are staring at in the dark. Text is a dark slate rather than
// black, panels are a soft off-white rather than white, and the screen
// behind them is a shade darker again so the panels read as panels
#define MX_TITLE_BK				GUI_MAKE_COLOR(0x00B48246)	// steel blue
#define MX_TITLE_EDGE			GUI_MAKE_COLOR(0x008C6432)	// a shade darker, for the rule

#define MX_PANE_BK				GUI_MAKE_COLOR(0x00F2EEE9)	// off-white, very slightly blue
#define MX_PANE_TX				GUI_MAKE_COLOR(0x003E3226)	// dark slate, not black
#define MX_PANE_DIM				GUI_MAKE_COLOR(0x00908070)	// secondary text
#define MX_SCREEN_BK			GUI_MAKE_COLOR(0x00D8D0C6)	// behind the panels
#define MX_EDGE					GUI_MAKE_COLOR(0x00ACA296)	// panel borders

// Selection picks up the title colour, so the screen has one accent
#define MX_SEL_BK				MX_TITLE_BK
#define MX_SEL_TX				GUI_WHITE

// Muted enough to sit in the same picture as the rest
#define MX_WARN_TX				GUI_MAKE_COLOR(0x003030B0)	// soft red
#define MX_GOOD_TX				GUI_MAKE_COLOR(0x003C6E1E)	// soft green

// Per line colours in the message pane, so the state of an outgoing
// direct message is readable at a glance rather than from a two
// character marker
#define MX_LINE_NORMAL			0					// received, or a channel message
#define MX_LINE_WAIT			1					// sent, not acknowledged yet
#define MX_LINE_OK				2					// acknowledged by the far end
#define MX_LINE_INFO			3					// local notice

static const GUI_COLOR	mx_line_colour[] =
{
	MX_PANE_TX,											// normal
	GUI_MAKE_COLOR(0x002020B4),							// waiting - red
	GUI_MAKE_COLOR(0x00287818),							// delivered - green
	MX_PANE_DIM											// notice
};

// Which pair of panes we are showing
#define MX_VIEW_CHAT			0
#define MX_VIEW_CONTACTS		1

static uint8_t	mx_view = MX_VIEW_CHAT;

// Set while both panes are being refilled, so the selection notifications
// emWin sends synchronously from LISTBOX_SetSel do not re-enter
static uint8_t	mx_rebuilding = 0;

// Compose buffer - the local draft, handed to the service on SEND
static char		mx_compose[MX_COMPOSE_MAX + 1];
static int		mx_compose_len = 0;

// What the panes were built from, so a poll that finds nothing new does
// not blow the listbox selection away
static uint32_t	mx_seen_revision = 0xFFFFFFFF;
static uint8_t	mx_seen_view	 = 0xFF;
static int		mx_seen_conv	 = -1;
static int		mx_seen_compose	 = -1;

// Which half of the conversation list the left pane is showing. Mixing
// channels and people in one list made it hard to read once more than a
// couple of each had accumulated, so the pane shows one kind at a time
// and the third button under it swaps them over
#define MX_FILT_CHAN			0
#define MX_FILT_DM				1

static uint8_t	mx_filter = MX_FILT_CHAN;

// The conversation currently selected in the chat view. Held by value,
// not by index - the list can be rebuilt underneath us
static MESHCORE_CONV	mx_conv;
static uint8_t			mx_conv_valid = 0;

// Which conversation each row of the filtered left pane came from. Row
// number and conversation number are no longer the same thing once half
// the conversations are being left out
#define MX_CONV_ROW_MAX			48

static uint8_t			mx_conv_row[MX_CONV_ROW_MAX];
static uint8_t			mx_conv_rows = 0;

// Which message each row of the right pane came from. A long message
// folds over several rows, so a tapped row has to be mapped back before
// REPLY can tell whose message it was
static uint8_t			mx_row_msg[MX_MSG_ROW_MAX];
static uint8_t			mx_row_count = 0;

// Colour class of each row, filled while the pane is built and read back
// by the owner draw callback - emWin has no per item colour of its own
static uint8_t			mx_row_class[MX_MSG_ROW_MAX];

// The row the user picked, kept across rebuilds so a repaint does not
// snatch the selection back. A new message still jumps to the bottom,
// which is what a chat should do
static int				mx_msg_sel = -1;
static uint8_t			mx_msg_seen = 0;		// messages at the last rebuild

// ---------------------------------------------------------------------
// Collapsible keyboard, same behaviour as the MarsChat one: the key grid
// lives in a child window that slides up from below the screen and back
// down again, while the action row stays put

static WM_HWIN			hMxKeyboard;
static WM_HWIN			hMxCharKeys[MX_KEY_CHARS];
static WM_HWIN			hMxShiftKey;
static uint8_t			mx_kb_shown = 0;

static GUI_ANIM_HANDLE	hMxAnim;

typedef struct
{
	WM_HWIN	hWin;
	int		dir;								// 0 = show (up), 1 = hide (down)

} MX_KB_ANIM;

static MX_KB_ANIM		mx_kb_anim;

// Keyboard pages. Unlike MarsChat - whose charset is a 6 bit code with
// no notion of case - MeshCore text is plain ASCII and case carries
// meaning in a chat, so lower and upper are separate pages and lower is
// the default. One key cycles the three; its face names the page it
// takes you to, so the next tap is always predictable
#define MX_PAGE_LOWER		0
#define MX_PAGE_UPPER		1
#define MX_PAGE_SYMBOL		2
#define MX_PAGE_COUNT		3

// Character order is the reading order of the QWERTY grid, not the
// alphabet - key i of a page sits at slot mx_char_slot[i]
static const char	mx_page_lower[MX_KEY_CHARS + 1]   = "qwertyuiopasdfghjklzxcvbnm";
static const char	mx_page_upper[MX_KEY_CHARS + 1]   = "QWERTYUIOPASDFGHJKLZXCVBNM";

// The symbols page rides the same grid, which puts the digits along the
// top row where a real keyboard has them
static const char	mx_page_symbols[MX_KEY_CHARS + 1] = "0123456789.,?!-/@:'()\"+=&;";

// Where each character lands. Slots 19, 20 and 28/29 are the function
// keys, so the last seven characters skip over slot 20
static const uint8_t	mx_char_slot[MX_KEY_CHARS] =
{
	 0,  1,  2,  3,  4,  5,  6,  7,  8,  9,		// q w e r t y u i o p
	10, 11, 12, 13, 14, 15, 16, 17, 18,			// a s d f g h j k l
	21, 22, 23, 24, 25, 26, 27					// z x c v b n m
};

static uint8_t		mx_page_id = MX_PAGE_LOWER;

static void mx_rebuild(void);
static void mx_layout_action_row(int kb_shown);
static void mx_apply_page(void);

static const char *mx_page(void)
{
	switch(mx_page_id)
	{
		case MX_PAGE_UPPER:		return mx_page_upper;
		case MX_PAGE_SYMBOL:	return mx_page_symbols;
		default:				return mx_page_lower;
	}
}

// What the cycle key should say - the page one more tap gets you to
static const char *mx_page_next_label(void)
{
	switch(mx_page_id)
	{
		case MX_PAGE_LOWER:		return "ABC";
		case MX_PAGE_UPPER:		return "123";
		default:				return "abc";
	}
}

// Only the window is in the static template - everything else is created
// in WM_INIT_DIALOG so it can be shown, hidden and moved
static const GUI_WIDGET_CREATE_INFO _aDialog[] =
{
	// -----------------------------------------------------------------------------------------------------------
	//							name				id				x	y	xsize		ysize
	// -----------------------------------------------------------------------------------------------------------
	{ WINDOW_CreateIndirect,	"",					ID_MX_WINDOW,	0,	0,	MX_UI_W,	MX_UI_H,	0,	0x64,	0 },
};

// ---------------------------------------------------------------------
// Keyboard slide

static void mx_anim_step(GUI_ANIM_INFO *pInfo, void *pVoid)
{
	MX_KB_ANIM	*p = (MX_KB_ANIM *)pVoid;
	int			y;

	// The animation outlives the dialog if the screen is closed mid
	// slide - F3 deletes the window while emWin is still stepping this.
	// Moving a window that has been freed corrupts the window list, and
	// the damage only shows up later as a fault inside WM__Paint
	if((hMxDialog == 0) || (p->hWin == 0))
		return;

	if(p->dir)
		y = MX_KB_Y + (((MX_UI_H - MX_KB_Y) * pInfo->Pos) / GUI_ANIM_RANGE);
	else
		y = MX_UI_H - (((MX_UI_H - MX_KB_Y) * pInfo->Pos) / GUI_ANIM_RANGE);

	WM_MoveTo(p->hWin, 0, y);
}

static void mx_anim_done(void *pVoid)
{
	(void)pVoid;

	hMxAnim = 0;

	if(hMxDialog)
		WM_InvalidateWindow(hMxDialog);
}

static void mx_anim_start(int dir)
{
	mx_kb_anim.hWin = hMxKeyboard;
	mx_kb_anim.dir	= dir;

	hMxAnim = GUI_ANIM_Create(MX_ANIM_TIME, 10, &mx_kb_anim, 0);

	GUI_ANIM_AddItem(hMxAnim, 0, MX_ANIM_TIME, ANIM_ACCELDECEL, &mx_kb_anim, mx_anim_step);
	GUI_ANIM_StartEx(hMxAnim, 1, mx_anim_done);
}

static void mx_show_keyboard(void)
{
	if(mx_kb_shown || hMxAnim)
		return;

	mx_kb_shown = 1;

	mx_layout_action_row(1);

	WM_InvalidateWindow(hMxDialog);
	mx_anim_start(0);
}

static void mx_hide_keyboard(void)
{
	if((!mx_kb_shown) || hMxAnim)
		return;

	mx_kb_shown = 0;

	// Back to lower case for next time, the way a phone keyboard does.
	// Leaving it on the symbols or capitals page means the next message
	// starts in whatever mode the last one happened to end in
	mx_page_id = MX_PAGE_LOWER;
	mx_apply_page();

	mx_layout_action_row(0);

	WM_InvalidateWindow(hMxDialog);
	mx_anim_start(1);
}

//*----------------------------------------------------------------------------
//* Function Name       : mx_apply_page
//* Object              : relabel the character keys for the active page
//* Context    			: CONTEXT_VIDEO (gui task)
//*----------------------------------------------------------------------------
static void mx_apply_page(void)
{
	const char	*page = mx_page();
	int			i;

	for(i = 0; i < MX_KEY_CHARS; i++)
	{
		char	label[2];

		label[0] = page[i];
		label[1] = 0;

		BUTTON_SetText(hMxCharKeys[i], label);
		WM_InvalidateWindow(hMxCharKeys[i]);
	}

	BUTTON_SetText(hMxShiftKey, mx_page_next_label());
	WM_InvalidateWindow(hMxShiftKey);
}

//*----------------------------------------------------------------------------
//* Function Name       : mx_create_keys
//* Object              : the key grid, children of the sliding container
//* Context    			: CONTEXT_VIDEO (gui task, WM_INIT_DIALOG)
//*----------------------------------------------------------------------------
static void mx_create_keys(void)
{
	WM_HWIN	hBtn;
	int		i, slot;

	for(i = 0; i < MX_KEY_CHARS; i++)
	{
		slot = mx_char_slot[i];

		hMxCharKeys[i] = BUTTON_CreateEx(MX_SLOT_X(slot), MX_SLOT_Y(slot),
										 MX_KEY_W, MX_KEY_H,
										 hMxKeyboard, WM_CF_SHOW, 0,
										 ID_MX_CHAR_0 + i);

		BUTTON_SetFont(hMxCharKeys[i], &GUI_Font24B_1);
	}

	// Page key - abc / ABC / 123
	hMxShiftKey = BUTTON_CreateEx(MX_SLOT_X(MX_SLOT_SHIFT), MX_SLOT_Y(MX_SLOT_SHIFT),
								  MX_KEY_W, MX_KEY_H,
								  hMxKeyboard, WM_CF_SHOW, 0,
								  ID_MX_SHIFT);

	BUTTON_SetFont(hMxShiftKey, &GUI_Font20B_1);

	// Backspace, where a phone keyboard puts it - end of the home row
	hBtn = BUTTON_CreateEx(MX_SLOT_X(MX_SLOT_DEL), MX_SLOT_Y(MX_SLOT_DEL),
						   MX_KEY_W, MX_KEY_H,
						   hMxKeyboard, WM_CF_SHOW, 0,
						   ID_MX_BACKSPACE);

	BUTTON_SetText(hBtn, "DEL");
	BUTTON_SetFont(hBtn, &GUI_Font20B_1);

	// Space bar - two slots wide, so it reads as a space bar
	hBtn = BUTTON_CreateEx(MX_SLOT_X(MX_SLOT_SPACE), MX_SLOT_Y(MX_SLOT_SPACE),
						   (MX_KEY_W * 2) + MX_KEY_GAP, MX_KEY_H,
						   hMxKeyboard, WM_CF_SHOW, 0,
						   ID_MX_SPACE);

	BUTTON_SetText(hBtn, "SPACE");
	BUTTON_SetFont(hBtn, &GUI_Font20B_1);

	mx_apply_page();
}

// ---------------------------------------------------------------------
// Action row

//*----------------------------------------------------------------------------
//* Function Name       : mx_layout_action_row
//* Object              : five slots across the bottom, their meaning
//*						: depending on the view and whether the keyboard
//*						: is up
//* Context    			: CONTEXT_VIDEO (gui task)
//*----------------------------------------------------------------------------
static void mx_layout_action_row(int kb_shown)
{
	// SPACE and DEL are keys on the keyboard itself now, not buttons
	// down here - so this row only carries what is useful with the
	// keyboard closed, plus the few things that go with typing
	WM_HWIN	hType	 = WM_GetDialogItem(hMxDialog, ID_MX_TYPE);
	WM_HWIN	hSend	 = WM_GetDialogItem(hMxDialog, ID_MX_SEND);
	WM_HWIN	hClear	 = WM_GetDialogItem(hMxDialog, ID_MX_CLEAR);
	WM_HWIN	hContact = WM_GetDialogItem(hMxDialog, ID_MX_CONTACTS);
	WM_HWIN	hAdvert	 = WM_GetDialogItem(hMxDialog, ID_MX_ADVERT);
	WM_HWIN	hAdd	 = WM_GetDialogItem(hMxDialog, ID_MX_ADD);
	WM_HWIN	hForget	 = WM_GetDialogItem(hMxDialog, ID_MX_FORGET);
	WM_HWIN	hAddChan = WM_GetDialogItem(hMxDialog, ID_MX_ADDCHAN);
	WM_HWIN	hReply	 = WM_GetDialogItem(hMxDialog, ID_MX_REPLY);
	WM_HWIN	hDelChan = WM_GetDialogItem(hMxDialog, ID_MX_DELCHAN);
	WM_HWIN	hFilt	 = WM_GetDialogItem(hMxDialog, ID_MX_CONVFILT);

	// Everything off, then only what this state needs back on
	WM_HideWindow(hType);
	WM_HideWindow(hSend);
	WM_HideWindow(hClear);
	WM_HideWindow(hContact);
	WM_HideWindow(hAdvert);
	WM_HideWindow(hAdd);
	WM_HideWindow(hForget);
	WM_HideWindow(hReply);

	// The list buttons live under the conversation list, not in this
	// row - they follow the view, not the keyboard. What the first two
	// do depends on which list the pane is showing, so they are
	// relabelled rather than duplicated
	if(mx_view == MX_VIEW_CHAT)
	{
		// Add and Del read the same either way - what they add and
		// delete is whatever the pane is listing. The third button
		// names the list you get by pressing it
		BUTTON_SetText(hAddChan, "Add");
		BUTTON_SetText(hDelChan, "Del");
		BUTTON_SetText(hFilt, (mx_filter == MX_FILT_DM) ? "CH" : "DM");

		WM_ShowWindow(hAddChan);
		WM_ShowWindow(hDelChan);
		WM_ShowWindow(hFilt);
	}
	else
	{
		WM_HideWindow(hAddChan);
		WM_HideWindow(hDelChan);
		WM_HideWindow(hFilt);
	}

	if(mx_view == MX_VIEW_CONTACTS)
	{
		// [ADD] [FORGET] [BACK] [ADVERT]
		WM_SetWindowPos(hAdd,		6,   MX_ACT_Y, 190, MX_ACT_H);
		WM_SetWindowPos(hForget,	202, MX_ACT_Y, 190, MX_ACT_H);
		WM_SetWindowPos(hContact,	398, MX_ACT_Y, 190, MX_ACT_H);
		WM_SetWindowPos(hAdvert,	594, MX_ACT_Y, 200, MX_ACT_H);

		BUTTON_SetText(hContact, "BACK");

		WM_ShowWindow(hAdd);
		WM_ShowWindow(hForget);
		WM_ShowWindow(hContact);
		WM_ShowWindow(hAdvert);

		return;
	}

	BUTTON_SetText(hContact, "CONTACTS");

	if(kb_shown)
	{
		// [HIDE] [CLEAR] [SEND] - space and backspace are on the keys
		BUTTON_SetText(hType, "HIDE");

		WM_SetWindowPos(hType,	6,   MX_ACT_Y, 200, MX_ACT_H);
		WM_SetWindowPos(hClear,	212, MX_ACT_Y, 200, MX_ACT_H);
		WM_SetWindowPos(hSend,	418, MX_ACT_Y, 376, MX_ACT_H);

		WM_ShowWindow(hType);
		WM_ShowWindow(hClear);
		WM_ShowWindow(hSend);

		return;
	}

	// [TYPE] [REPLY] [SEND] [CONTACTS] [ADVERT] - the channel buttons
	// sit under the conversation list instead
	// No EXIT - F3 closes the screen, the same key that opens it
	BUTTON_SetText(hType, "TYPE");

	WM_SetWindowPos(hType,		6,   MX_ACT_Y, 154, MX_ACT_H);
	WM_SetWindowPos(hReply,		164, MX_ACT_Y, 154, MX_ACT_H);
	WM_SetWindowPos(hSend,		322, MX_ACT_Y, 154, MX_ACT_H);
	WM_SetWindowPos(hContact,	480, MX_ACT_Y, 154, MX_ACT_H);
	WM_SetWindowPos(hAdvert,	638, MX_ACT_Y, 154, MX_ACT_H);

	WM_ShowWindow(hType);
	WM_ShowWindow(hReply);
	WM_ShowWindow(hSend);
	WM_ShowWindow(hContact);
	WM_ShowWindow(hAdvert);
}

// ---------------------------------------------------------------------
// List building

//*----------------------------------------------------------------------------
//* Function Name       : mx_msg_owner_draw
//* Object              : paint one row of the message pane
//* Notes    			: emWin colours a listbox as a whole, not per
//*						: item, so the only way to give a single line its
//*						: own colour is to take over the drawing. The
//*						: default handler still does the work for
//*						: everything except the text colour
//* Context    			: CONTEXT_VIDEO (gui task)
//*----------------------------------------------------------------------------
static int mx_msg_owner_draw(const WIDGET_ITEM_DRAW_INFO *pDrawItemInfo)
{
	char		text[96];
	uint8_t		cls;

	switch(pDrawItemInfo->Cmd)
	{
		// A listbox asks for the whole row in one go. WIDGET_ITEM_DRAW_TEXT
		// is what the table style widgets send and is handled as well, so
		// this works whichever way round the library dispatches
		case WIDGET_ITEM_DRAW:
		case WIDGET_ITEM_DRAW_TEXT:
		{
			int	ret = LISTBOX_OwnerDraw(pDrawItemInfo);
			int	sel = LISTBOX_GetSel(pDrawItemInfo->hWin);

			cls = (pDrawItemInfo->ItemIndex < (int)MX_MSG_ROW_MAX)
					? mx_row_class[pDrawItemInfo->ItemIndex] : MX_LINE_NORMAL;

			// The selected row keeps its white-on-blue, or the colour
			// would fight the highlight and read as unselected. A plain
			// row is already the right colour and is left alone
			if((pDrawItemInfo->ItemIndex == sel) || (cls == MX_LINE_NORMAL))
				return ret;

			// Let the default draw the background, the highlight and the
			// text, then put the text back over itself in the colour this
			// row wants. Same font and same origin means the same pixels,
			// so nothing of the first pass shows through
			LISTBOX_GetItemText(pDrawItemInfo->hWin, pDrawItemInfo->ItemIndex,
								text, sizeof(text));

			GUI_SetColor(mx_line_colour[cls]);
			GUI_SetFont(LISTBOX_GetFont(pDrawItemInfo->hWin));
			GUI_SetTextMode(GUI_TM_TRANS);
			GUI_DispStringAt(text, pDrawItemInfo->x0, pDrawItemInfo->y0);

			return ret;
		}

		default:
			// Sizing and everything else stays default
			return LISTBOX_OwnerDraw(pDrawItemInfo);
	}
}

static void mx_list_clear(WM_HWIN hLb)
{
	int	n = LISTBOX_GetNumItems(hLb);

	while(n--)
		LISTBOX_DeleteItem(hLb, 0);
}

//*----------------------------------------------------------------------------
//* Function Name       : mx_list_add_wrapped
//* Object              : add a line, folding it over several listbox rows
//*						: when it is too wide for the pane
//* Notes    			: a listbox row cannot wrap by itself and a chat
//*						: message is routinely longer than the pane
//* Context    			: CONTEXT_VIDEO (gui task)
//*----------------------------------------------------------------------------
static int mx_list_add_wrapped(WM_HWIN hLb, const char *text, int width)
{
	char	line[96];
	int		len = (int)strlen(text);
	int		pos = 0;
	int		rows = 0;

	if(width > (int)(sizeof(line) - 1))
		width = (int)(sizeof(line) - 1);

	if(len == 0)
	{
		LISTBOX_AddString(hLb, "");
		return 1;
	}

	while(pos < len)
	{
		int	take = len - pos;
		int	brk;

		if(take > width)
		{
			take = width;

			// Prefer to break on a space rather than mid word
			for(brk = take; brk > (width / 2); brk--)
			{
				if(text[pos + brk] == ' ')
				{
					take = brk;
					break;
				}
			}
		}

		memcpy(line, text + pos, take);
		line[take] = 0;

		LISTBOX_AddString(hLb, line);
		rows++;

		pos += take;

		// Swallow the space we broke on
		while((pos < len) && (text[pos] == ' '))
			pos++;
	}

	return rows;
}

//*----------------------------------------------------------------------------
//* Function Name       : mx_build_conv_list
//* Object              : left pane, chat view
//* Context    			: CONTEXT_VIDEO (gui task)
//*----------------------------------------------------------------------------
static void mx_build_conv_list(void)
{
	MESHCORE_CONV	conv;
	char			label[48];
	uint8_t			i, n = meshcore_conv_count();
	uint8_t			want = (mx_filter == MX_FILT_DM)
							? MESHCORE_CONV_DIRECT : MESHCORE_CONV_CHANNEL;
	int				sel = -1;

	mx_list_clear(hMxConvList);

	mx_conv_rows = 0;

	for(i = 0; i < n; i++)
	{
		if(meshcore_conv_at(i, &conv))
			continue;

		// Only the kind this pane is showing
		if(conv.kind != want)
			continue;

		if(mx_conv_rows >= MX_CONV_ROW_MAX)
			break;

		meshcore_conv_label(&conv, label, sizeof(label));

		LISTBOX_AddString(hMxConvList, label);

		// Keep the highlight on whatever was selected before the rebuild
		if((mx_conv_valid) &&
		   (conv.kind == mx_conv.kind) &&
		   (((conv.kind == MESHCORE_CONV_CHANNEL) && (conv.chan_hash == mx_conv.chan_hash)) ||
		    ((conv.kind == MESHCORE_CONV_DIRECT)  && (memcmp(conv.peer, mx_conv.peer, sizeof(conv.peer)) == 0))))
			sel = mx_conv_rows;

		mx_conv_row[mx_conv_rows++] = i;
	}

	// Nothing selected yet, or what was selected is not in this list -
	// fall back to its first row. An empty list leaves the previous
	// conversation open on the right, which is better than blanking it
	if((sel < 0) && (mx_conv_rows > 0))
	{
		if(meshcore_conv_at(mx_conv_row[0], &mx_conv) == 0)
		{
			mx_conv_valid = 1;
			sel = 0;
		}
	}

	if(sel >= 0)
		LISTBOX_SetSel(hMxConvList, sel);
}

//*----------------------------------------------------------------------------
//* Function Name       : mx_build_msg_list
//* Object              : right pane, chat view
//* Context    			: CONTEXT_VIDEO (gui task)
//*----------------------------------------------------------------------------
static void mx_build_msg_list(void)
{
	char	line[160];
	uint8_t	i, n, cls;
	int		rows;

	mx_list_clear(hMxMsgList);

	mx_row_count = 0;

	if(!mx_conv_valid)
		return;

	// Whatever is on screen has been seen - drop its activity badge.
	// Done before the rows are built so the count the conversation list
	// draws in the same pass is already clear
	meshcore_mark_read(&mx_conv);

	n = meshcore_msg_count(&mx_conv);

	for(i = 0; i < n; i++)
	{
		const MESHCORE_MSG	*m = meshcore_msg_at(&mx_conv, i);

		if(m == NULL)
			continue;

		// Colour class for every row this message produces
		cls = MX_LINE_NORMAL;

		if(m->dir == MESHCORE_DIR_INFO)
			cls = MX_LINE_INFO;
		else if(m->dir == MESHCORE_DIR_TX)
			cls = m->delivered ? MX_LINE_OK : (m->ack_wait ? MX_LINE_WAIT : MX_LINE_NORMAL);

		switch(m->dir)
		{
			case MESHCORE_DIR_TX:
				// Every outgoing line reads the same. Whether it has been
				// acknowledged is carried by the colour alone - a marker
				// in front of the text pushes the message about as the
				// state changes and is harder to read than the text it
				// is meant to annotate
				snprintf(line, sizeof(line), "%s  >> %s", m->time, m->text);
				break;

			case MESHCORE_DIR_INFO:
				snprintf(line, sizeof(line), "%s  -- %s", m->time, m->text);
				break;

			default:
				if(mx_conv.kind == MESHCORE_CONV_CHANNEL)
					snprintf(line, sizeof(line), "%s  %s: %s", m->time, m->sender, m->text);
				else
					snprintf(line, sizeof(line), "%s  << %s", m->time, m->text);
				break;
		}

		rows = mx_list_add_wrapped(hMxMsgList, line, MX_MSG_WRAP);

		// Remember which message these rows belong to, so a tap on any
		// line of a folded message still finds its sender, and give
		// every one of them the message's colour
		while((rows--) && (mx_row_count < MX_MSG_ROW_MAX))
		{
			mx_row_class[mx_row_count] = cls;
			mx_row_msg[mx_row_count++] = i;
		}
	}

	rows = LISTBOX_GetNumItems(hMxMsgList);

	if(rows <= 0)
		return;

	// A new message parks the view on the newest line, the way a chat
	// should. Otherwise the row the user picked is put back - this runs
	// on every repaint, and stealing the selection would make REPLY
	// impossible to aim
	if(n != mx_msg_seen)
	{
		mx_msg_seen	= n;
		mx_msg_sel	= rows - 1;
	}

	if((mx_msg_sel < 0) || (mx_msg_sel >= rows))
		mx_msg_sel = rows - 1;

	LISTBOX_SetSel(hMxMsgList, mx_msg_sel);
}

//*----------------------------------------------------------------------------
//* Function Name       : mx_build_heard_list
//* Object              : left pane, contacts view - everyone we have
//*						: heard advertise, saved ones marked
//* Context    			: CONTEXT_VIDEO (gui task)
//*----------------------------------------------------------------------------
static void mx_build_heard_list(void)
{
	char	label[48];
	uint8_t	i, n = mc_contacts_count();
	int		sel = LISTBOX_GetSel(hMxConvList);

	mx_list_clear(hMxConvList);

	for(i = 0; i < n; i++)
	{
		MC_CONTACT	*c = mc_contacts_at(i);

		if(c == NULL)
			continue;

		snprintf(label, sizeof(label), "%s %s", c->saved ? "*" : " ", c->name);

		LISTBOX_AddString(hMxConvList, label);
	}

	if(n == 0)
		LISTBOX_AddString(hMxConvList, "(nothing heard yet)");

	if((sel >= 0) && (sel < (int)n))
		LISTBOX_SetSel(hMxConvList, sel);
	else if(n)
		LISTBOX_SetSel(hMxConvList, 0);
}

//*----------------------------------------------------------------------------
//* Function Name       : mx_build_contact_detail
//* Object              : right pane, contacts view
//* Context    			: CONTEXT_VIDEO (gui task)
//*----------------------------------------------------------------------------
static void mx_build_contact_detail(void)
{
	char		line[96];
	MC_CONTACT	*c;
	int			sel = LISTBOX_GetSel(hMxConvList);
	int			i;

	mx_list_clear(hMxMsgList);

	// The contacts view shares this listbox, so its rows need a class
	// too or they would inherit whatever the chat view last set
	memset(mx_row_class, MX_LINE_NORMAL, sizeof(mx_row_class));
	mx_row_count = 0;

	// Explain the empty list rather than just showing nothing - a node
	// only lands here when it advertises, which can be a long wait
	if(mc_contacts_count() == 0)
	{
		LISTBOX_AddString(hMxMsgList, "No nodes heard yet.");
		LISTBOX_AddString(hMxMsgList, "");
		LISTBOX_AddString(hMxMsgList, "A node appears here only when it");
		LISTBOX_AddString(hMxMsgList, "advertises - that is the only packet");
		LISTBOX_AddString(hMxMsgList, "carrying a public key, and it is what");
		LISTBOX_AddString(hMxMsgList, "ADD needs to store a contact.");
		LISTBOX_AddString(hMxMsgList, "");
		LISTBOX_AddString(hMxMsgList, "Press ADVERT to announce this radio -");
		LISTBOX_AddString(hMxMsgList, "nodes in range usually answer with");
		LISTBOX_AddString(hMxMsgList, "their own advert.");
		return;
	}

	if(sel < 0)
		return;

	c = mc_contacts_at((uint8_t)sel);

	if(c == NULL)
		return;

	snprintf(line, sizeof(line), "name   %s", c->name);
	LISTBOX_AddString(hMxMsgList, line);

	snprintf(line, sizeof(line), "role   %s",
			(c->role == MESHCORE_DEVICE_ROLE_CHAT_NODE)   ? "chat node" :
			(c->role == MESHCORE_DEVICE_ROLE_REPEATER)    ? "repeater"  :
			(c->role == MESHCORE_DEVICE_ROLE_ROOM_SERVER) ? "room server" :
			(c->role == MESHCORE_DEVICE_ROLE_SENSOR)      ? "sensor" : "unknown");
	LISTBOX_AddString(hMxMsgList, line);

	snprintf(line, sizeof(line), "status %s%s",
			c->saved ? "saved contact" : "heard only",
			c->have_shared ? ", key ready" : "");
	LISTBOX_AddString(hMxMsgList, line);

	snprintf(line, sizeof(line), "snr    %d dB", (int)c->snr);
	LISTBOX_AddString(hMxMsgList, line);

	snprintf(line, sizeof(line), "hops   %d", (int)c->path_len);
	LISTBOX_AddString(hMxMsgList, line);

	LISTBOX_AddString(hMxMsgList, "");
	LISTBOX_AddString(hMxMsgList, "public key");

	// 32 bytes, eight per row
	for(i = 0; i < MC_EC_KEY_SIZE; i += 8)
	{
		snprintf(line, sizeof(line), "  %02X %02X %02X %02X %02X %02X %02X %02X",
				 c->pub_key[i + 0], c->pub_key[i + 1], c->pub_key[i + 2], c->pub_key[i + 3],
				 c->pub_key[i + 4], c->pub_key[i + 5], c->pub_key[i + 6], c->pub_key[i + 7]);

		LISTBOX_AddString(hMxMsgList, line);
	}
}

//*----------------------------------------------------------------------------
//* Function Name       : mx_rebuild
//* Object              : refill both panes for the current view
//* Context    			: CONTEXT_VIDEO (gui task)
//*----------------------------------------------------------------------------
static void mx_rebuild(void)
{
	if((hMxConvList == 0) || (hMxMsgList == 0))
		return;

	mx_rebuilding = 1;

	if(mx_view == MX_VIEW_CONTACTS)
	{
		mx_build_heard_list();
		mx_build_contact_detail();
	}
	else
	{
		// Clear the open conversation's badge before the list is drawn,
		// or the row would show a count for messages already on screen
		if(mx_conv_valid)
			meshcore_mark_read(&mx_conv);

		mx_build_conv_list();
		mx_build_msg_list();
	}

	mx_rebuilding = 0;

	WM_InvalidateWindow(hMxDialog);
}

// ---------------------------------------------------------------------
// Painting - only the title strip, the compose bar and the status block
// are drawn by hand, the rest is stock widgets

//*----------------------------------------------------------------------------
//* Function Name       : mx_paint_title
//* Object              : name, node hash, clock
//* Context    			: CONTEXT_VIDEO (gui task, WM_PAINT)
//*----------------------------------------------------------------------------
static void mx_paint_title(void)
{
	RTC_TimeTypeDef	tm = {0};
	RTC_DateTypeDef	dt = {0};
	char			buf[64];

	k_GetTime(&tm);
	k_GetDate(&dt);

	GUI_SetBkColor(MX_TITLE_BK);
	GUI_SetColor(MX_TITLE_BK);
	GUI_FillRect(0, 0, MX_UI_W - 1, MX_TITLE_H - 1);

	GUI_SetTextMode(GUI_TM_TRANS);
	GUI_SetFont(&GUI_Font24B_1);

	// The screen is named for the protocol it is speaking, even though
	// the app and its sources stay MeshCore - that name has to cover
	// Meshtastic later
	GUI_SetColor(GUI_WHITE);
	GUI_DispStringAt("MESHCORE", 8, 4);

	GUI_SetFont(&GUI_Font20B_1);
	GUI_SetColor(GUI_WHITE);

	// Node name, clipped - it can be 32 characters and the strip has to
	// hold the radio settings and the clock as well
	{
		char	name[MX_TITLE_NAME_MAX + 1];

		strncpy(name, meshcore_node_name(), MX_TITLE_NAME_MAX);
		name[MX_TITLE_NAME_MAX] = 0;

		snprintf(buf, sizeof(buf), "%s [%02X]", name, meshcore_node_hash());
		GUI_DispStringAt(buf, MX_TITLE_NAME_X, 7);
	}

	// What the modem is actually tuned to and running - the settings are
	// compile time, but they are the first thing to check when nothing
	// is being heard
	lora_radio_config_text(buf, sizeof(buf));
	GUI_DispStringAt(buf, MX_TITLE_LORA_X, 7);

	snprintf(buf, sizeof(buf), "%02d:%02d:%02d", tm.Hours, tm.Minutes, tm.Seconds);
	GUI_DispStringAt(buf, MX_UI_W - 100, 7);

	GUI_SetColor(MX_TITLE_EDGE);
	GUI_DrawHLine(MX_TITLE_H - 1, 0, MX_UI_W - 1);
}

//*----------------------------------------------------------------------------
//* Function Name       : mx_paint_compose
//* Object              : the draft, with a caret so an empty one still
//*						: looks like an input
//* Context    			: CONTEXT_VIDEO (gui task, WM_PAINT)
//*----------------------------------------------------------------------------
static void mx_paint_compose(void)
{
	char	buf[MX_COMPOSE_MAX + 8];

	GUI_SetColor(MX_PANE_BK);
	GUI_FillRect(MX_COMP_X, MX_COMP_Y, MX_COMP_X + MX_COMP_W - 1, MX_COMP_Y + MX_COMP_H - 1);

	GUI_SetColor(MX_EDGE);
	GUI_DrawRect(MX_COMP_X, MX_COMP_Y, MX_COMP_X + MX_COMP_W - 1, MX_COMP_Y + MX_COMP_H - 1);

	GUI_SetTextMode(GUI_TM_TRANS);
	GUI_SetFont(&GUI_Font20B_1);

	if(mx_compose_len)
	{
		GUI_SetColor(MX_PANE_TX);
		snprintf(buf, sizeof(buf), "%s_", mx_compose);
	}
	else
	{
		GUI_SetColor(MX_PANE_DIM);
		snprintf(buf, sizeof(buf), "type a message...");
	}

	GUI_DispStringAt(buf, MX_COMP_X + 6, MX_COMP_Y + 6);
}

//*----------------------------------------------------------------------------
//* Function Name       : mx_paint_status
//* Object              : the block the collapsed keyboard leaves free -
//*						: what the service is doing and what it knows
//* Context    			: CONTEXT_VIDEO (gui task, WM_PAINT, keyboard down)
//*----------------------------------------------------------------------------
static void mx_paint_status(void)
{
	const MC_IDENTITY	*id = mc_identity_get();
	char				buf[96];
	int					y = MX_STAT_Y + 8;
	uint8_t				i, heard = 0, saved = 0;

	GUI_SetColor(MX_PANE_BK);
	GUI_FillRect(MX_STAT_X, MX_STAT_Y, MX_STAT_X + MX_STAT_W - 1, MX_STAT_Y + MX_STAT_H - 1);

	GUI_SetColor(MX_EDGE);
	GUI_DrawRect(MX_STAT_X, MX_STAT_Y, MX_STAT_X + MX_STAT_W - 1, MX_STAT_Y + MX_STAT_H - 1);

	GUI_SetTextMode(GUI_TM_TRANS);
	GUI_SetFont(&GUI_Font20B_1);
	GUI_SetColor(MX_PANE_TX);

	if(!meshcore_ready())
	{
		if(meshcore_state() == MESHCORE_STATE_WAIT_SD)
		{
			// Say what it is waiting for and for how much longer - this
			// is ten seconds on a radio with no card in it
			snprintf(buf, sizeof(buf), "looking for the SD card... %us",
					 (unsigned int)meshcore_sd_wait_left());
			GUI_DispStringAt(buf, MX_STAT_X + 12, y);

			GUI_SetFont(&GUI_Font16B_1);
			GUI_SetColor(MX_PANE_DIM);
			GUI_DispStringAt("the radio will run without one, but nothing will be saved",
							 MX_STAT_X + 12, y + 26);
		}
		else
			GUI_DispStringAt("meshcore service starting...", MX_STAT_X + 12, y);

		return;
	}

	for(i = 0; i < mc_contacts_count(); i++)
	{
		MC_CONTACT	*c = mc_contacts_at(i);

		if(c == NULL)
			continue;

		if(c->saved)
			saved++;

		if(c->heard)
			heard++;
	}

	snprintf(buf, sizeof(buf), "node %s   hash %02X   channels %d   key %s",
			 id->name, id->pub[0], (int)mc_channels_count(),
			 (id->source == MC_ID_SRC_CARD)   ? "card"   :
			 (id->source == MC_ID_SRC_BACKUP) ? "backup" :
			 (id->source == MC_ID_SRC_NEW)    ? "NEW"    : "?");
	GUI_DispStringAt(buf, MX_STAT_X + 12, y);

	y += 26;

	// emWin free memory is here on purpose: the screen rebuilds its two
	// listboxes every time a message lands, and if any of that leaked,
	// this number would walk downwards until the allocator failed and
	// handed out a bad handle. Watching it is how that gets ruled in or
	// out after the WM__Paint fault on 2026-09-12
	snprintf(buf, sizeof(buf), "contacts %d saved, %d heard   gui free %dk",
			 (int)saved, (int)heard, (int)(GUI_ALLOC_GetNumFreeBytes() / 1024));
	GUI_DispStringAt(buf, MX_STAT_X + 12, y);

	y += 26;

	// What came back of the last transmission. On a quiet mesh this is
	// the only thing that tells you the signal is getting out at all
	{
		MESHCORE_ECHO	echo;

		if(meshcore_tx_pending())
		{
			GUI_SetColor(MX_PANE_TX);
			snprintf(buf, sizeof(buf), "tx: %d packet(s) waiting for the modem",
					 (int)meshcore_tx_pending());
		}
		else if(meshcore_last_echo(&echo))
		{
			unsigned int	age = (unsigned int)((xTaskGetTickCount() - echo.tick) /
												 configTICK_RATE_HZ);

			if(echo.repeats)
			{
				GUI_SetColor(MX_GOOD_TX);
				snprintf(buf, sizeof(buf),
						 "sent %us ago - repeated %u time(s), %u hop(s), snr %d",
						 age, (unsigned int)echo.repeats,
						 (unsigned int)echo.min_hops, (int)echo.best_snr);
			}
			else
			{
				GUI_SetColor(MX_PANE_TX);
				snprintf(buf, sizeof(buf), "sent %us ago - no repeat heard yet", age);
			}
		}
		else
		{
			GUI_SetColor(MX_PANE_TX);
			snprintf(buf, sizeof(buf), "tx: idle - nothing sent yet");
		}

		GUI_DispStringAt(buf, MX_STAT_X + 12, y);
		GUI_SetColor(MX_PANE_TX);
	}

	y += 26;

	if(!mc_store_is_writable())
	{
		GUI_SetColor(MX_WARN_TX);

		// Card in the slot but no filesystem means its init failed at
		// boot, and nothing re-runs that until the card is reseated.
		// Say so rather than just reporting "no card", which sends the
		// user looking for a card that is already there
		if(sd_card_is_detected() == SD_PRESENT)
			GUI_DispStringAt("SD card present but did not mount - reseat it to retry",
							 MX_STAT_X + 12, y);
		else
			GUI_DispStringAt("no SD card - channels and contacts will not be saved",
							 MX_STAT_X + 12, y);
	}
	else if(id->weak_entropy)
	{
		GUI_SetColor(MX_WARN_TX);
		GUI_DispStringAt("identity key was seeded without the TRNG", MX_STAT_X + 12, y);
	}
	else
	{
		GUI_SetColor(MX_PANE_DIM);

		if(mx_filter == MX_FILT_DM)
			GUI_DispStringAt("Add picks someone the radio has heard",
							 MX_STAT_X + 12, y);
		else
			GUI_DispStringAt("type a name then Add to join a channel",
							 MX_STAT_X + 12, y);
	}
}

// ---------------------------------------------------------------------
// Input

static void mx_compose_append(char c)
{
	if(mx_compose_len >= MX_COMPOSE_MAX)
		return;

	mx_compose[mx_compose_len++] = c;
	mx_compose[mx_compose_len]	 = 0;
}

//*----------------------------------------------------------------------------
//* Function Name       : mx_do_send
//* Object              : hand the draft to the service
//* Context    			: CONTEXT_VIDEO (gui task)
//*----------------------------------------------------------------------------
static void mx_do_send(void)
{
	if((mx_compose_len == 0) || (!mx_conv_valid))
		return;

	if(meshcore_send_text(&mx_conv, mx_compose) != 0)
		return;									// queue full, keep the draft

	mx_compose_len	= 0;
	mx_compose[0]	= 0;

	mx_hide_keyboard();
}

//*----------------------------------------------------------------------------
//* Function Name       : mx_on_button
//* Object              : one place for presses from either the dialog or
//*						: the keyboard container
//* Context    			: CONTEXT_VIDEO (gui task)
//*----------------------------------------------------------------------------
static void mx_on_button(int id, int ncode)
{
	if(ncode != WM_NOTIFICATION_RELEASED)
		return;

	if((id >= ID_MX_CHAR_0) && (id < (ID_MX_CHAR_0 + MX_KEY_CHARS)))
	{
		mx_compose_append(mx_page()[id - ID_MX_CHAR_0]);
		WM_InvalidateWindow(hMxDialog);
		return;
	}

	if(id == ID_MX_SHIFT)
	{
		mx_page_id = (uint8_t)((mx_page_id + 1) % MX_PAGE_COUNT);
		mx_apply_page();
		return;
	}

	switch(id)
	{
		case ID_MX_SPACE:
			mx_compose_append(' ');
			break;

		case ID_MX_BACKSPACE:
			if(mx_compose_len > 0)
				mx_compose[--mx_compose_len] = 0;
			break;

		case ID_MX_CLEAR:
			mx_compose_len	= 0;
			mx_compose[0]	= 0;
			break;

		case ID_MX_SEND:
			mx_do_send();
			break;

		case ID_MX_TYPE:
			if(mx_kb_shown)
				mx_hide_keyboard();
			else
				mx_show_keyboard();
			break;

		case ID_MX_CONTACTS:
		{
			// Doubles as BACK - the button is relabelled by the layout
			mx_view = (mx_view == MX_VIEW_CHAT) ? MX_VIEW_CONTACTS : MX_VIEW_CHAT;

			// Coming back from the node browser, the one thing that can
			// have changed is who is in the contact list - so land on
			// the list that shows them
			if(mx_view == MX_VIEW_CHAT)
			{
				mx_filter		 = MX_FILT_DM;
				mx_conv_valid	 = 0;
				mx_seen_revision = 0xFFFFFFFF;
			}

			mx_hide_keyboard();
			mx_layout_action_row(mx_kb_shown);

			// Force a rebuild, the panes now mean something else
			mx_seen_view = 0xFF;
			break;
		}

		case ID_MX_ADVERT:
			meshcore_send_advert();
			break;

		case ID_MX_DELCHAN:
		{
			// Removes the channel picked in the left pane. Deliberately
			// not guarded by a confirmation: the key derives from the
			// name, so getting it back is a matter of typing the name
			// in and pressing Add again
			if((mx_view != MX_VIEW_CHAT) || (!mx_conv_valid))
				break;

			// Same button, the other list: forget the person instead.
			// The conversation holds the leading bytes of their key, so
			// the contact it names has to be looked back up by them
			if(mx_conv.kind == MESHCORE_CONV_DIRECT)
			{
				uint8_t	i;

				for(i = 0; i < mc_contacts_count(); i++)
				{
					MC_CONTACT	*c = mc_contacts_at(i);

					if((c == NULL) || (!c->saved))
						continue;

					if(memcmp(c->pub_key, mx_conv.peer, sizeof(mx_conv.peer)) != 0)
						continue;

					meshcore_forget_contact(i);

					mx_conv_valid	 = 0;	// it is going away, pick another
					mx_seen_revision = 0xFFFFFFFF;
					break;
				}

				break;
			}

			if(mx_conv.kind != MESHCORE_CONV_CHANNEL)
			{
				printf("meshcore: Del - pick a channel, not a contact \r\n");
				break;
			}

			// The default channel is refused by the store, and the
			// service says so in the conversation itself. Post it
			// either way so that notice appears, but only give up the
			// selection when the channel really is going away
			{
				MC_CHANNEL	*ch = mc_channels_find_by_hash(mx_conv.chan_hash);
				uint8_t		keep = mc_channel_is_default(ch);

				if(meshcore_remove_channel(&mx_conv) == 0)
				{
					if(!keep)
						mx_conv_valid = 0;	// it is going away, pick another

					mx_seen_revision = 0xFFFFFFFF;
				}
			}
			break;
		}

		//*------------------------------------------------------------
		// Reply to whichever message is picked in the right pane.
		//
		// MeshCore has no reply field on the wire - a reply is simply a
		// message whose text begins "@[name] ", and that is what every
		// client renders as a quote. Confirmed from this radio's own
		// logs, where the bots answer us with "ack @[mcHF-0BA1] ..."
		// and stations answer each other with "@[jonnyboy] 22 hops"
		//*------------------------------------------------------------
		case ID_MX_REPLY:
		{
			const MESHCORE_MSG	*m;
			int					row = LISTBOX_GetSel(hMxMsgList);
			int					i;

			if((mx_view != MX_VIEW_CHAT) || (!mx_conv_valid))
				break;

			if((row < 0) || (row >= (int)mx_row_count))
				break;

			m = meshcore_msg_at(&mx_conv, mx_row_msg[row]);

			// Only an incoming message from a named sender is worth
			// quoting - replying to our own, or to a local notice, is
			// not a thing
			if((m == NULL) || (m->dir != MESHCORE_DIR_RX) || (m->sender[0] == 0))
				break;

			mx_compose_len	= 0;
			mx_compose[0]	= 0;

			// Direct messages already have exactly one other party, so
			// the quote would be noise - just open the keyboard
			if(mx_conv.kind == MESHCORE_CONV_CHANNEL)
			{
				mx_compose_append('@');
				mx_compose_append('[');

				for(i = 0; m->sender[i]; i++)
					mx_compose_append(m->sender[i]);

				mx_compose_append(']');
				mx_compose_append(' ');
			}

			mx_show_keyboard();
			break;
		}

		case ID_MX_ADDCHAN:
		{
			// Showing people rather than channels: there is nothing to
			// type, a contact can only come from an advert we have
			// heard. Hand over to the browser of heard nodes, which is
			// where ADD lives
			if(mx_filter == MX_FILT_DM)
			{
				mx_view			 = MX_VIEW_CONTACTS;
				mx_seen_revision = 0xFFFFFFFF;

				mx_hide_keyboard();
				mx_layout_action_row(0);
				break;
			}

			// The compose bar doubles as the entry field - type the
			// channel name, then press this. Everything the radio needs
			// follows from the name, so there is no key to type in
			if(mx_compose_len == 0)
			{
				printf("meshcore: Add - type a channel name first, e.g. #uk \r\n");
				break;
			}

			if(meshcore_add_channel(mx_compose) == 0)
			{
				mx_compose_len	 = 0;
				mx_compose[0]	 = 0;
				mx_seen_revision = 0xFFFFFFFF;

				mx_hide_keyboard();
			}
			break;
		}

		//*------------------------------------------------------------
		// Swap the left pane between the channels and the people. Both
		// are conversations and both draw in the same listbox - only
		// one kind is listed at a time, which is the whole point
		//*------------------------------------------------------------
		case ID_MX_CONVFILT:
		{
			mx_filter = (mx_filter == MX_FILT_CHAN) ? MX_FILT_DM : MX_FILT_CHAN;

			// Whatever was open belongs to the list we just left, so
			// the rebuild picks the first row of the new one
			mx_conv_valid	 = 0;
			mx_msg_sel		 = -1;
			mx_seen_revision = 0xFFFFFFFF;

			mx_layout_action_row(mx_kb_shown);
			break;
		}

		case ID_MX_ADD:
		{
			int	sel = LISTBOX_GetSel(hMxConvList);

			// Nothing to add until a node has advertised - the list is
			// built from adverts, which is the only packet that carries
			// a public key
			if((mx_view != MX_VIEW_CONTACTS) || (sel < 0) || (mc_contacts_count() == 0))
			{
				printf("meshcore: ADD - nothing heard yet, press ADVERT and wait \r\n");
				break;
			}

			meshcore_add_contact((uint8_t)sel);
			mx_seen_revision = 0xFFFFFFFF;
			break;
		}

		case ID_MX_FORGET:
		{
			int	sel = LISTBOX_GetSel(hMxConvList);

			if((mx_view == MX_VIEW_CONTACTS) && (sel >= 0) && (mc_contacts_count() > 0))
			{
				meshcore_forget_contact((uint8_t)sel);
				mx_seen_revision = 0xFFFFFFFF;
			}
			break;
		}

		default:
			return;
	}

	WM_InvalidateWindow(hMxDialog);
}

//*----------------------------------------------------------------------------
//* Function Name       : mx_on_select
//* Object              : the left pane selection moved
//* Context    			: CONTEXT_VIDEO (gui task)
//*----------------------------------------------------------------------------
static void mx_on_select(void)
{
	int	sel;

	// LISTBOX_SetSel inside a rebuild notifies synchronously, so this
	// lands in the middle of mx_build_conv_list and would start editing
	// the other listbox while emWin is still working through this one.
	// The rebuild refreshes both panes anyway
	if(mx_rebuilding)
		return;

	if(hMxConvList == 0)
		return;

	sel = LISTBOX_GetSel(hMxConvList);

	if(sel < 0)
		return;

	if(mx_view == MX_VIEW_CONTACTS)
	{
		mx_build_contact_detail();
		return;
	}

	if(sel >= (int)mx_conv_rows)
		return;

	if(meshcore_conv_at(mx_conv_row[sel], &mx_conv) == 0)
	{
		mx_conv_valid = 1;

		// A different conversation starts at its newest line, not at
		// whatever row happened to be picked in the previous one
		mx_msg_sel	= -1;
		mx_msg_seen	= 0xFF;

		mx_build_msg_list();
	}
}

// ---------------------------------------------------------------------

static void _cbKeyboard(WM_MESSAGE *pMsg)
{
	switch(pMsg->MsgId)
	{
		case WM_PAINT:
			GUI_SetColor(MX_SCREEN_BK);
			GUI_FillRect(0, 0, MX_KB_W - 1, MX_KB_H - 1);
			break;

		case WM_NOTIFY_PARENT:
			mx_on_button(WM_GetId(pMsg->hWinSrc), pMsg->Data.v);
			break;

		default:
			WM_DefaultProc(pMsg);
			break;
	}
}

//*----------------------------------------------------------------------------
//* Function Name       : mx_create_widgets
//* Object              : the two listboxes and the action row
//* Context    			: CONTEXT_VIDEO (gui task, WM_INIT_DIALOG)
//*----------------------------------------------------------------------------
static void mx_create_widgets(WM_HWIN hWin)
{
	static const struct
	{
		int			id;
		const char	*text;

	} buttons[] =
	{
		{ ID_MX_TYPE,		"TYPE"		},
		{ ID_MX_SEND,		"SEND"		},
		{ ID_MX_CLEAR,		"CLEAR"		},
		{ ID_MX_CONTACTS,	"CONTACTS"	},
		{ ID_MX_ADVERT,		"ADVERT"	},
		{ ID_MX_ADD,		"ADD"		},
		{ ID_MX_FORGET,		"FORGET"	},
		{ ID_MX_ADDCHAN,	"Add"		},
		{ ID_MX_REPLY,		"REPLY"		},
		{ ID_MX_DELCHAN,	"Del"		},
		{ ID_MX_CONVFILT,	"DM"		}
	};

	int	i;

	// Shorter than the right pane - the channel buttons take the rest
	hMxConvList = LISTBOX_CreateEx(MX_CONV_X, MX_LIST_Y, MX_CONV_W, MX_CONV_H,
								   hWin, WM_CF_SHOW, 0, ID_MX_LIST_LEFT, NULL);

	hMxMsgList  = LISTBOX_CreateEx(MX_MSG_X, MX_LIST_Y, MX_MSG_W, MX_LIST_H,
								   hWin, WM_CF_SHOW, 0, ID_MX_LIST_RIGHT, NULL);

	LISTBOX_SetFont(hMxConvList, &GUI_Font24B_1);
	LISTBOX_SetFont(hMxMsgList,  &GUI_Font24_1);

	// Off the stock white/black. SELFOCUS matters as much as SEL - the
	// panes lose focus to each other, and without it the highlighted row
	// changes colour depending on which list was touched last
	for(i = 0; i < 2; i++)
	{
		WM_HWIN	hLb = i ? hMxMsgList : hMxConvList;

		LISTBOX_SetBkColor  (hLb, LISTBOX_CI_UNSEL,     MX_PANE_BK);
		LISTBOX_SetTextColor(hLb, LISTBOX_CI_UNSEL,     MX_PANE_TX);

		LISTBOX_SetBkColor  (hLb, LISTBOX_CI_SEL,       MX_SEL_BK);
		LISTBOX_SetTextColor(hLb, LISTBOX_CI_SEL,       MX_SEL_TX);

		LISTBOX_SetBkColor  (hLb, LISTBOX_CI_SELFOCUS,  MX_SEL_BK);
		LISTBOX_SetTextColor(hLb, LISTBOX_CI_SELFOCUS,  MX_SEL_TX);

		LISTBOX_SetBkColor  (hLb, LISTBOX_CI_DISABLED,  MX_PANE_BK);
		LISTBOX_SetTextColor(hLb, LISTBOX_CI_DISABLED,  MX_PANE_DIM);
	}

	// Scrollbars are attached once here rather than left to
	// LISTBOX_SetAutoScrollV.
	//
	// Auto scroll works, but it CREATES AND DESTROYS the scrollbar
	// window as the item count crosses what fits - and these lists are
	// emptied and refilled every time a message lands, so that would be
	// a window churning under the window manager several times a second.
	// The WM__Paint fault is still unexplained and looks like a window
	// being painted after its block went invalid, so until that is
	// understood the scrollbars stay permanent: created once, never
	// freed. The lists drive them the same either way
	{
		SCROLLBAR_Handle	hSb;

		LISTBOX_SetAutoScrollV(hMxMsgList,  0);
		LISTBOX_SetAutoScrollV(hMxConvList, 0);

		hSb = SCROLLBAR_CreateAttached(hMxMsgList, SCROLLBAR_CF_VERTICAL);
		SCROLLBAR_SetWidth(hSb, MX_SCROLL_W);

		// Per line colour needs the drawing taken over - see
		// mx_msg_owner_draw
		LISTBOX_SetOwnerDraw(hMxMsgList, mx_msg_owner_draw);

		hSb = SCROLLBAR_CreateAttached(hMxConvList, SCROLLBAR_CF_VERTICAL);
		SCROLLBAR_SetWidth(hSb, MX_SCROLL_W);
	}

	for(i = 0; i < (int)GUI_COUNTOF(buttons); i++)
	{
		WM_HWIN	hBtn = BUTTON_CreateEx(6, MX_ACT_Y, 190, MX_ACT_H,
									   hWin, WM_CF_SHOW, 0, buttons[i].id);

		BUTTON_SetText(hBtn, buttons[i].text);
		BUTTON_SetFont(hBtn, &GUI_Font20B_1);
	}

	// The two channel buttons are not part of the action row - they are
	// parked under the conversation list, share its width and are short
	// enough to read as a footer to it. Positioned once here, since
	// unlike the action row they never move
	{
		WM_HWIN	hAdd  = WM_GetDialogItem(hWin, ID_MX_ADDCHAN);
		WM_HWIN	hDel  = WM_GetDialogItem(hWin, ID_MX_DELCHAN);
		WM_HWIN	hFilt = WM_GetDialogItem(hWin, ID_MX_CONVFILT);

		WM_SetWindowPos(hAdd,  MX_CONV_X,     MX_CHANBTN_Y, MX_CHANBTN_W, MX_CHANBTN_H);
		WM_SetWindowPos(hDel,  MX_CHANBTN_X2, MX_CHANBTN_Y, MX_CHANBTN_W, MX_CHANBTN_H);
		WM_SetWindowPos(hFilt, MX_CHANBTN_X3, MX_CHANBTN_Y, MX_CHANBTN_W, MX_CHANBTN_H);

		BUTTON_SetFont(hAdd, &GUI_Font16B_1);
		BUTTON_SetFont(hDel, &GUI_Font16B_1);
	}
}

static void _cbDialog(WM_MESSAGE *pMsg)
{
	int	Id, NCode;

	switch(pMsg->MsgId)
	{
		case WM_INIT_DIALOG:
		{
			// GUI_CreateDialogBox has not returned yet, so latch the
			// handle here - the helpers below need it
			hMxDialog = pMsg->hWin;

			// Created before the keys so it can parent them. Starts off
			// the bottom of the screen
			hMxKeyboard = WM_CreateWindowAsChild(0, MX_UI_H, MX_KB_W, MX_KB_H,
												 pMsg->hWin, WM_CF_SHOW, _cbKeyboard, 0);

			mx_create_widgets(pMsg->hWin);

			// Always open on the lower case page, however the screen was
			// left last time
			mx_page_id = MX_PAGE_LOWER;

			mx_create_keys();

			mx_kb_shown = 0;
			hMxAnim		= 0;

			mx_layout_action_row(0);

			// The widgets are rebuilt on every entry, the conversation
			// is not - F3 toggles this screen away and back while the
			// service keeps running, so the draft and the selected
			// conversation live in statics that survive it
			mx_seen_revision	= 0xFFFFFFFF;
			mx_seen_view		= 0xFF;
			mx_seen_conv		= -1;
			mx_seen_compose		= -1;

			mx_rebuild();

			hMxTimer = WM_CreateTimer(pMsg->hWin, 0, 500, 0);

			#ifdef MX_DEBUG_GUI_MEM
			// Every window this screen owns, so the handle in a fault
			// dump can be named instead of guessed at
			printf("meshcore ui: dlg %d kb %d conv %d msg %d shift %d key0 %d \r\n",
					(int)hMxDialog, (int)hMxKeyboard, (int)hMxConvList,
					(int)hMxMsgList, (int)hMxShiftKey, (int)hMxCharKeys[0]);

			printf("meshcore ui: type %d reply %d send %d chan %d cont %d adv %d \r\n",
					(int)WM_GetDialogItem(pMsg->hWin, ID_MX_TYPE),
					(int)WM_GetDialogItem(pMsg->hWin, ID_MX_REPLY),
					(int)WM_GetDialogItem(pMsg->hWin, ID_MX_SEND),
					(int)WM_GetDialogItem(pMsg->hWin, ID_MX_ADDCHAN),
					(int)WM_GetDialogItem(pMsg->hWin, ID_MX_CONTACTS),
					(int)WM_GetDialogItem(pMsg->hWin, ID_MX_ADVERT));
			#endif
			break;
		}

		case WM_PAINT:
		{
			GUI_SetColor(MX_SCREEN_BK);
			GUI_FillRect(0, MX_TITLE_H, MX_UI_W - 1, MX_UI_H - 1);

			mx_paint_title();
			mx_paint_compose();

			// Painted underneath the keyboard container - revealed when
			// it slides away
			if(!mx_kb_shown)
				mx_paint_status();

			break;
		}

		case WM_TIMER:
		{
			uint32_t	rev = meshcore_revision();
			int			sel = LISTBOX_GetSel(hMxConvList);

			if((rev != mx_seen_revision) || (mx_view != mx_seen_view) || (sel != mx_seen_conv))
			{
				mx_seen_revision	= rev;
				mx_seen_view		= mx_view;
				mx_seen_conv		= sel;

				mx_rebuild();
			}
			else if(mx_compose_len != mx_seen_compose)
			{
				mx_seen_compose = mx_compose_len;
				WM_InvalidateWindow(hMxDialog);
			}
			else
			{
				// The clock always moves
				WM_InvalidateWindow(hMxDialog);
			}

			// Trail of emWin free memory and how many rows are live, so
			// the log around a fault shows whether the allocator was
			// being drained by the per-message rebuilds. Cheap, and it
			// only runs while this screen is up
			#ifdef MX_DEBUG_GUI_MEM
			{
				static uint8_t	tick;

				if((++tick) >= 10)					// every 5 s
				{
					tick = 0;

					printf("meshcore ui: gui free %d, conv %d, msg %d \r\n",
							(int)GUI_ALLOC_GetNumFreeBytes(),
							(int)LISTBOX_GetNumItems(hMxConvList),
							(int)LISTBOX_GetNumItems(hMxMsgList));
				}
			}
			#endif

			WM_RestartTimer(pMsg->Data.v, 500);
			break;
		}

		case WM_DELETE:
		{
			// Zeroed, a stale handle deleted again later frees whatever emWin
			// reused the number for (hard fault in WM__Paint, handle 7)
			if(hMxTimer)
				WM_DeleteTimer(hMxTimer);
			hMxTimer = 0;

			// The children go with the dialog - drop the handles so a
			// stray repaint cannot reach a dead window. hMxDialog and
			// the animation's copy of the keyboard handle go too: a
			// slide still in flight steps again after this, and moving
			// a freed window is what corrupts emWin's window list
			hMxDialog		= 0;
			hMxKeyboard		= 0;
			hMxConvList		= 0;
			hMxMsgList		= 0;
			hMxShiftKey		= 0;
			hMxAnim			= 0;
			mx_kb_anim.hWin	= 0;

			memset(hMxCharKeys, 0, sizeof(hMxCharKeys));
			break;
		}

		case WM_NOTIFY_PARENT:
		{
			Id	  = WM_GetId(pMsg->hWinSrc);
			NCode = pMsg->Data.v;

			if((Id == ID_MX_LIST_LEFT) && (NCode == WM_NOTIFICATION_SEL_CHANGED))
			{
				mx_on_select();
				break;
			}

			// The user picked a message row - remember it so the next
			// repaint puts it back rather than jumping to the newest,
			// and so REPLY knows what was tapped
			if((Id == ID_MX_LIST_RIGHT) && (NCode == WM_NOTIFICATION_SEL_CHANGED))
			{
				if(!mx_rebuilding)
					mx_msg_sel = LISTBOX_GetSel(hMxMsgList);

				break;
			}

			mx_on_button(Id, NCode);
			break;
		}

		case WM_KEY:
		{
			switch(((WM_KEY_INFO *)(pMsg->Data.p))->Key)
			{
				// Back to the radio. Scheduled rather than done here -
				// the mode switch is what deletes this dialog
				case GUI_KEY_HOME:
					ui_s.req_state = MODE_DESKTOP;
					xTaskNotify(ps.hUiTask, UI_NEW_MODE_EVENT, eSetValueWithOverwrite);
					break;
			}
			break;
		}

		default:
			WM_DefaultProc(pMsg);
			break;
	}
}

//*----------------------------------------------------------------------------
//* Function Name       : _cbBkWindow
//* Object              : desktop window behind the dialog
//* Context    			: CONTEXT_VIDEO (gui task)
//*----------------------------------------------------------------------------
static void _cbBkWindow(WM_MESSAGE *pMsg)
{
	switch(pMsg->MsgId)
	{
		case WM_PAINT:
			// Not GUI_Clear() - the driver runs in LCD_DRAWMODE_TRANS
			// where a clear does nothing
			GUI_SetColor(MX_SCREEN_BK);
			GUI_FillRect(0, 0, MX_UI_W - 1, MX_UI_H - 1);
			break;

		default:
			WM_DefaultProc(pMsg);
			break;
	}
}

//*----------------------------------------------------------------------------
//* Function Name       : meshcore_ui_create
//* Object              : bring the screen up, called by the UI mode
//*						: switch on entry to MODE_DESKTOP_MESHCORE
//* Context    			: CONTEXT_VIDEO (gui task)
//*----------------------------------------------------------------------------
void meshcore_ui_create(void)
{
	// The menu leaves the default window background at GUI_WHITE and it
	// is a sticky global, so set what this screen wants every time
	WINDOW_SetDefaultBkColor(MX_SCREEN_BK);

	WM_SetCallback(WM_HBKWIN, &_cbBkWindow);

	hMxDialog = GUI_CreateDialogBox(_aDialog, GUI_COUNTOF(_aDialog), _cbDialog, 0, 0, 0);
}

//*----------------------------------------------------------------------------
//* Function Name       : meshcore_ui_destroy
//* Object              : tear it down on the way back to the desktop.
//*						: Safe to call when it was never up
//* Context    			: CONTEXT_VIDEO (gui task)
//*----------------------------------------------------------------------------
void meshcore_ui_destroy(void)
{
	if(hMxDialog)
	{
		WM_SetCallback		(WM_HBKWIN, 0);
		WM_InvalidateWindow	(WM_HBKWIN);

		WM_DeleteWindow(hMxDialog);

		hMxDialog = 0;
	}
}
