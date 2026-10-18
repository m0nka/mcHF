/************************************************************************************
**                                                                                 **
**                                 mcHF QRP Transceiver                            **
**                         Krassi Atanassov - M0NKA, 2013-2026                     **
**                                                                                 **
**---------------------------------------------------------------------------------**
**                                                                                 **
**  File name:		dsp_s.h                                                        **
**  Description:	Baseband menu - UHSDR DSP settings, one tab per group         **
**  Licence:		https://github.com/m0nka/mcHF/blob/main/LICENSE                **
************************************************************************************/
#ifndef __DSP_S_H
#define __DSP_S_H

#define ID_WINDOW_0               	(GUI_ID_USER + 0x00)
#define ID_DSM_MULTIPAGE			(GUI_ID_USER + 0x01)

// Per setting widget ids, + DSP_SET_xxx
#define ID_DSM_CTRL					(GUI_ID_USER + 0x100)	// spinbox/checkbox/list, choice value text
#define ID_DSM_PREV					(GUI_ID_USER + 0x140)	// choice '<'
#define ID_DSM_NEXT					(GUI_ID_USER + 0x180)	// choice '>'

#define ID_DSM_PAGE					(GUI_ID_USER + 0x1E0)	// + page number
#define ID_DSM_DEFAULTS				(GUI_ID_USER + 0x1F0)

// Widget kinds
#define DSK_SPIN					0		// numeric, - value +
#define DSK_CHECK					1		// on/off
#define DSK_CHOICE					2		// < option >
#define DSK_LIST					3		// scrolling list, two rows tall

// Pages
#define DSM_PAGE_AGC				0
#define DSM_PAGE_NOISE				1
#define DSM_PAGE_RX					2
#define DSM_PAGE_FM					3
#define DSM_PAGE_TX					4
#define DSM_PAGE_CW					5
#define DSM_PAGE_NUM				6

// Page layout - three columns of five rows, slot = column * DSM_ROWS + row,
// the last slot holds the page 'Defaults' button
#define DSM_ROWS					5
#define DSM_SLOT_DEFAULTS			14

// Multipage as in the Battery Manager, the tabs take the right 40 pixels
#define DSM_MULTI_X					776
#define DSM_PAGE_X					736
#define DSM_PAGE_Y					420

#define DSM_COL_X(slot)				(10 + (((slot) / DSM_ROWS) * 242))
#define DSM_ROW_Y(slot)				(6  + (((slot) % DSM_ROWS) *  80))

#define DSM_CTRL_X					228
#define DSM_DEFAULTS_X				140
#define DSM_CTRL_Y					46
#define DSM_LABEL_Y					20
#define DSM_LIST_Y					(76 + DSM_CTRL_Y)

#endif
