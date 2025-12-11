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
#include "version.h"

#include "adc.h"

#ifdef CONTEXT_VIDEO

#include "gui.h"
#include "dialog.h"
#include "ui_cool_progress.h"

#include "ui_controls_tx_stat.h"
#include "desktop\ui_controls_layout.h"

// Public radio state
extern struct	TRANSCEIVER_STATE_UI	tsu;

// Refresh flags
uchar ui_power_factor 	= 0xFF;
uchar ui_tx_power 		= 0xFF;
ushort bias0 			= 0;
ushort bias1 			= 0;
uchar  txs				= 0;

ushort fwd_volts		= 0;
ushort f_volts			= 0;

//*----------------------------------------------------------------------------
//* Function Name       : ui_controls_tx_stat_repaint
//* Object              :
//* Notes    			:
//* Notes   			:
//* Notes    			:
//* Context    			: CONTEXT_VIDEO
//*----------------------------------------------------------------------------
static void ui_controls_tx_stat_repaint(void)
{
	//char buf[40];

	// Clear control
	//GUI_SetColor(GUI_BLACK);
	//GUI_FillRect(TX_STAT_X, TX_STAT_Y,	TX_STAT_X + TX_STAT_SIZE_X, TX_STAT_Y + TX_STAT_SIZE_Y);

	// Clear dynamic part
	//GUI_SetColor(GUI_WHITE);
	//GUI_FillRect(TX_STAT_X + 53, TX_STAT_Y + 1,	TX_STAT_X + TX_STAT_SIZE_X - 4,	TX_STAT_Y + TX_STAT_SIZE_Y - 1);

	// Create frame
	//GUI_SetColor(GUI_GRAY);
	//GUI_DrawRect(TX_STAT_X,		 TX_STAT_Y,      TX_STAT_X + TX_STAT_SIZE_X - 1,  TX_STAT_Y + TX_STAT_SIZE_Y);
	//GUI_DrawRect(TX_STAT_X - 1,	 TX_STAT_Y - 1,  TX_STAT_X + TX_STAT_SIZE_X,  TX_STAT_Y + TX_STAT_SIZE_Y + 1);
	//GUI_FillRect(TX_STAT_X + 50, TX_STAT_Y + 1,	 TX_STAT_X + 53,   TX_STAT_Y + TX_STAT_SIZE_Y - 1);
	//GUI_FillRect(TX_STAT_X + 0,	 TX_STAT_Y + 20, TX_STAT_X +  49,  TX_STAT_Y + TX_STAT_SIZE_Y);
	//GUI_FillRect(TX_STAT_X + 123,TX_STAT_Y - 1,  TX_STAT_X +  TX_STAT_SIZE_X, TX_STAT_Y + TX_STAT_SIZE_Y + 1);

	//GUI_SetFont(&GUI_Font8x16_1);
	//GUI_SetColor(GUI_GREEN);
	switch(tsu.band[tsu.curr_band].tx_power)
	{
		case PA_LEVEL_0_5W:
			ui_cool_progress_tx_pwr(TX_STAT_X, TX_STAT_Y - 5, 3, ".5");
			break;

		case PA_LEVEL_1W:
			ui_cool_progress_tx_pwr(TX_STAT_X, TX_STAT_Y - 5, 5, " 1");
			break;

		case PA_LEVEL_2W:
			ui_cool_progress_tx_pwr(TX_STAT_X, TX_STAT_Y - 5, 10, " 2");
			break;

		case PA_LEVEL_5W:
			ui_cool_progress_tx_pwr(TX_STAT_X, TX_STAT_Y - 5, 25, " 5");
			break;

		case PA_LEVEL_10W:
			ui_cool_progress_tx_pwr(TX_STAT_X, TX_STAT_Y - 5, 50, "10");
			break;

		case PA_LEVEL_15W:
			ui_cool_progress_tx_pwr(TX_STAT_X, TX_STAT_Y - 5, 75, "15");
			break;

		case PA_LEVEL_20W:
			ui_cool_progress_tx_pwr(TX_STAT_X, TX_STAT_Y - 5, 100, "20");
			break;

		default:
			ui_cool_progress_tx_pwr(TX_STAT_X, TX_STAT_Y - 5, 0, "00");
			break;
	}


#if 0
	GUI_SetColor(GUI_WHITE);
	GUI_SetFont(&GUI_Font8x13_1);
	sprintf(buf, "PF %d", tsu.band[tsu.curr_band].power_factor);
	GUI_DispStringAt(buf,  TX_STAT_X + 4, TX_STAT_Y + 22);

	GUI_SetFont(&GUI_Font8x16_1);

	// Show gate text
	GUI_SetColor(GUI_GRAY);
	GUI_DispStringAt("G1", TX_STAT_X + 57, TX_STAT_Y +  2);
	GUI_DispStringAt("G2", TX_STAT_X + 57, TX_STAT_Y + 20);

	if(tsu.rxtx)
		GUI_SetColor(GUI_RED);
	else
		GUI_SetColor(GUI_GRAY);

	sprintf(buf, "%d.%dV", tsu.bias0/1000, (tsu.bias0%1000)/10);
	GUI_DispStringAt(buf,  TX_STAT_X + 79, TX_STAT_Y + 2);

	sprintf(buf, "%d.%dV", tsu.bias1/1000, (tsu.bias1%1000)/10);
	GUI_DispStringAt(buf, TX_STAT_X + 79, TX_STAT_Y + 20);

	// ----------------------------------------------------------
	// Prototype new control positions
	GUI_SetColor(GUI_WHITE);

	// Forward voltage on the bridge
	f_volts = adc_read_fwd_power();
	if(f_volts == 0xFFFF)
		sprintf(buf, "FWD %d.%dV", 0, 0);
	else
	{
		sprintf(buf, "FWD %d.%dV", f_volts/1000, (f_volts%1000)/10);
		printf("%4dmV(fwd) \r\n", f_volts);
	}

	GUI_DispStringAt(buf, TX_STAT_X + 130, TX_STAT_Y +  2);

	// Reflected voltage on the bridge
	sprintf(buf, "REF %d.%dV", 0, 42);
	GUI_DispStringAt(buf, TX_STAT_X + 130, TX_STAT_Y + 20);
	// PA Temp
	sprintf(buf, "TMP %d.%dC", 31, 4);
	GUI_DispStringAt(buf, TX_STAT_X + 130, TX_STAT_Y + 38);
	// PA Rail Voltage
	GUI_SetColor(GUI_GRAY);
	sprintf(buf, "%d.%dV", 20, 5);
	GUI_DispStringAt("Dv", TX_STAT_X + 57, TX_STAT_Y + 38);
	GUI_DispStringAt(buf,  TX_STAT_X + 79, TX_STAT_Y + 38);
	// ----------------------------------------------------------
#endif
	// Save
	ui_power_factor = tsu.band[tsu.curr_band].power_factor;
	ui_tx_power 	= tsu.band[tsu.curr_band].tx_power;
	bias0			= tsu.bias0;
	bias1			= tsu.bias1;
	txs   			= tsu.rxtx;
	fwd_volts		= f_volts;
}

//*----------------------------------------------------------------------------
//* Function Name       : ui_controls_tx_stat_init
//* Object              :
//* Notes    			:
//* Notes   			:
//* Notes    			:
//* Context    			: CONTEXT_VIDEO
//*----------------------------------------------------------------------------
void ui_controls_tx_stat_init(void)
{
	ui_controls_tx_stat_repaint();
}

//*----------------------------------------------------------------------------
//* Function Name       : ui_controls_tx_stat_quit
//* Object              :
//* Notes    			:
//* Notes   			:
//* Notes    			:
//* Context    			: CONTEXT_VIDEO
//*----------------------------------------------------------------------------
void ui_controls_tx_stat_quit(void)
{

}

//*----------------------------------------------------------------------------
//* Function Name       : ui_controls_tx_stat_touch
//* Object              :
//* Notes    			:
//* Notes   			:
//* Notes    			:
//* Context    			: CONTEXT_VIDEO
//*----------------------------------------------------------------------------
void ui_controls_tx_stat_touch(void)
{
	//
}

//*----------------------------------------------------------------------------
//* Function Name       : ui_controls_tx_stat_refresh
//* Object              :
//* Notes    			:
//* Notes   			:
//* Notes    			:
//* Context    			: CONTEXT_VIDEO
//*----------------------------------------------------------------------------
void ui_controls_tx_stat_refresh(void)
{
	// ----------------------------------------------------------
	// Unit test cool progress bar
	#if 0
	char buff[20];
	static uchar prog = 0;
	static uchar skip = 0;

	if(skip++ < 5)
		return;
	skip = 0;

	sprintf(buff,"%2d", prog);
	ui_cool_progress_tx_pwr(TX_STAT_X + 120, TX_STAT_Y + 25, prog++, buff);
	if(prog == 100) prog = 0;
	return;
	#endif
	// ----------------------------------------------------------

	// Skip needless update
	if(	(ui_power_factor != tsu.band[tsu.curr_band].power_factor)||\
		(ui_tx_power != tsu.band[tsu.curr_band].tx_power)||\
		(bias0 != tsu.bias0)||\
		(bias1 != tsu.bias1)||\
		(txs   != tsu.rxtx) ||\
		(fwd_volts != f_volts)\
	   )
	{
		ui_controls_tx_stat_repaint();
	}
}
#endif
