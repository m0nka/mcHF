/************************************************************************************
**                                                                                 **
**                                 mcHF QRP Transceiver                            **
**                         Krassi Atanassov - M0NKA, 2013-2026                     **
**                                                                                 **
**---------------------------------------------------------------------------------**
**                                                                                 **
**  File name:		ft8_radio.c                                                    **
**  Description:	FT8 mode radio setup - dial, mode and filter per band          **
**  Last Modified:                                                                 **
**  Licence:		https://github.com/m0nka/mcHF/blob/main/LICENSE                **
************************************************************************************/
#include "mchf_pro_board.h"
#include "main.h"

#ifdef CONTEXT_FT8

#include "mchf_icc_def.h"

#include "ui_actions.h"

#include "ft8_radio.h"

// FreeRTOS process state
extern struct PROC_STATE			ps;

// Public radio state
extern struct TRANSCEIVER_STATE_UI	tsu;

// FT8 dial frequencies (USB carrier), indexed by radio band (BAND_MODE_xx)
static const struct
{
	ulong		dial_hz;
	const char	*name;

} ft8_band_plan[MAX_BANDS] =
{
	{ 0,		""    },			// 0  - 2200m
	{ 0,		""    },			// 1  - 630m
	{ 1840000,	"160M" },			// 2
	{ 3573000,	"80M"  },			// 3
	{ 5357000,	"60M"  },			// 4
	{ 7074000,	"40M"  },			// 5
	{ 10136000,	"30M"  },			// 6
	{ 14074000,	"20M"  },			// 7
	{ 18100000,	"17M"  },			// 8
	{ 21074000,	"15M"  },			// 9
	{ 24915000,	"12M"  },			// 10
	{ 28074000,	"10M"  },			// 11
};

// What the operator had on each band FT8 touched, restored on exit
static struct
{
	uchar	saved;
	ulong	vfo;						// active VFO frequency
	uchar	demod;
	uchar	filter;

} ft8_saved[MAX_BANDS];

static uchar	ft8_home_band = 0xFF;	// band on entry, 0xFF = not entered

//*----------------------------------------------------------------------------
//* Function Name       : ft8_radio_band_dial / ft8_radio_band_name
//* Context    			: any
//*----------------------------------------------------------------------------
ulong ft8_radio_band_dial(uchar band)
{
	if(band >= MAX_BANDS)
		return 0;

	return ft8_band_plan[band].dial_hz;
}

const char *ft8_radio_band_name(uchar band)
{
	if(band >= MAX_BANDS)
		return "";

	return ft8_band_plan[band].name;
}

//*----------------------------------------------------------------------------
//* Function Name       : ft8_radio_setup_band
//* Object              : save a band's user settings (once per FT8 session)
//*						: and put it on its FT8 dial, USB, 3.6 kHz
//* Notes    			: the caller tells the tasks - the band may or may
//*						: not be the current one yet
//* Context    			: CONTEXT_VIDEO (gui task)
//*----------------------------------------------------------------------------
static void ft8_radio_setup_band(uchar band)
{
	struct BAND_INFO	*b = &tsu.band[band];
	ulong				dial = ft8_band_plan[band].dial_hz;

	if(!ft8_saved[band].saved)
	{
		ft8_saved[band].vfo    = (b->active_vfo == VFO_A) ? b->vfo_a : b->vfo_b;
		ft8_saved[band].demod  = b->demod_mode;
		ft8_saved[band].filter = b->filter;
		ft8_saved[band].saved  = 1;
	}

	if(b->active_vfo == VFO_A)
		b->vfo_a = dial;
	else
		b->vfo_b = dial;

	// The FT8 passband is 200..3000 Hz of USB audio
	b->demod_mode = DEMOD_USB;
	b->filter     = AUDIO_3P6KHZ;

	printf("ft8: band %d -> %u Hz usb (was %u Hz, mode %d, filter %d) \r\n",
			band, (uint)dial, (uint)ft8_saved[band].vfo, ft8_saved[band].demod, ft8_saved[band].filter);
}

//*----------------------------------------------------------------------------
//* Function Name       : ft8_radio_apply
//* Object              : push the current band's VFO, mode and filter out
//* Notes    			: one ICC_CHANGE_BAND carries NCO, mode and filter to
//*						: the M4 together - separate DEMOD_MODE and FITER
//*						: notifications would overwrite each other
//* Context    			: CONTEXT_VIDEO (gui task)
//*----------------------------------------------------------------------------
static void ft8_radio_apply(void)
{
	if(ps.hVfoTask != NULL)
		xTaskNotify(ps.hVfoTask, UI_NEW_FREQ_EVENT, eSetValueWithOverwrite);

	if(ps.hIccTask != NULL)
		xTaskNotify(ps.hIccTask, UI_ICC_CHANGE_BAND, eSetValueWithOverwrite);
}

//*----------------------------------------------------------------------------
//* Function Name       : ft8_radio_enter
//* Object              : FT8 screen entry - tune the current band to FT8
//* Context    			: CONTEXT_VIDEO (gui task)
//*----------------------------------------------------------------------------
void ft8_radio_enter(void)
{
	int	i;

	for(i = 0; i < MAX_BANDS; i++)
		ft8_saved[i].saved = 0;

	ft8_home_band = tsu.curr_band;

	// A band without FT8 (GEN, 2200m, 630m) - hop to the next one that has
	if(ft8_radio_band_dial(tsu.curr_band) == 0)
	{
		printf("ft8: band %d has no ft8 frequency \r\n", tsu.curr_band);
		ft8_radio_next_band();
		return;
	}

	ft8_radio_setup_band(tsu.curr_band);
	ft8_radio_apply();
}

//*----------------------------------------------------------------------------
//* Function Name       : ft8_radio_next_band
//* Object              : BAND button - next band with an FT8 frequency
//* Context    			: CONTEXT_VIDEO (gui task)
//*----------------------------------------------------------------------------
void ft8_radio_next_band(void)
{
	uchar	band = tsu.curr_band;
	int		i;

	for(i = 0; i < MAX_BANDS; i++)
	{
		band = (uchar)((band + 1) % MAX_BANDS);

		if(ft8_radio_band_dial(band) != 0)
			break;
	}

	ft8_radio_setup_band(band);

	// Full band change - analogue filters, VFO, audio and the DSP (the
	// ICC band change carries the mode and filter set above)
	ui_actions_change_band(band, 1);
}

//*----------------------------------------------------------------------------
//* Function Name       : ft8_radio_exit
//* Object              : FT8 screen exit - give every touched band its
//*						: settings back and return to the entry band
//* Context    			: CONTEXT_VIDEO (gui task)
//*----------------------------------------------------------------------------
void ft8_radio_exit(void)
{
	struct BAND_INFO	*b;
	int					i;

	if(ft8_home_band == 0xFF)
		return;

	for(i = 0; i < MAX_BANDS; i++)
	{
		if(!ft8_saved[i].saved)
			continue;

		b = &tsu.band[i];

		if(b->active_vfo == VFO_A)
			b->vfo_a = ft8_saved[i].vfo;
		else
			b->vfo_b = ft8_saved[i].vfo;

		b->demod_mode = ft8_saved[i].demod;
		b->filter     = ft8_saved[i].filter;

		ft8_saved[i].saved = 0;
	}

	if(tsu.curr_band != ft8_home_band)
		ui_actions_change_band(ft8_home_band, 1);
	else
		ft8_radio_apply();

	printf("ft8: radio restored, band %d \r\n", ft8_home_band);

	ft8_home_band = 0xFF;
}

#endif
