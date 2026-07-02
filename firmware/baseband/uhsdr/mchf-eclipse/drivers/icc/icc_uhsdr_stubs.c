/************************************************************************************
**                                                                                 **
**                                 mcHF QRP Transceiver                            **
**                         Krassi Atanassov - M0NKA, 2013-2026                     **
**                                                                                 **
**---------------------------------------------------------------------------------**
**                                                                                 **
**  File name:		icc_uhsdr_stubs.c                                              **
**  Description:	M4 core implementations/stubs of small UHSDR helpers that     **
**					normally live in modules not compiled for the baseband        **
**					build (radio_management.c, ui_driver.c, uhsdr_board.c, USB).   **
**					Function bodies are copied from the originals where they      **
**					only depend on the ts/ads state                                **
**  Licence:		https://github.com/m0nka/mcHF/blob/main/LICENSE                **
************************************************************************************/

// Compiled only for the STM32H747 CM4 baseband build
#ifdef H7_M4_CORE

#include "uhsdr_board.h"
#include "audio_driver.h"
#include "radio_management.h"
#include "profiling.h"

// ------------------------------------------------------------------
// radio_management.c helpers (copied 1:1)
//
bool RadioManagement_LSBActive(uint16_t dmod_mode)
{
    bool    is_lsb;

    switch(dmod_mode)        // determine if the receiver is set to LSB or USB or FM
    {
    case DEMOD_SAM:
        is_lsb = ads.sam_sideband == SAM_SIDEBAND_LSB;
        break;
    case DEMOD_LSB:
        is_lsb = true;      // it is LSB
        break;
    case DEMOD_CW:
        is_lsb = ts.cw_lsb; // is this USB RX mode?  (LSB of mode byte was zero)
        break;
    case DEMOD_DIGI:
        is_lsb = ts.digi_lsb;
        break;
    case DEMOD_USB:
    default:
        is_lsb = false;     // it is USB
        break;
    }

    return is_lsb;
}

bool RadioManagement_UsesBothSidebands(uint16_t dmod_mode)
{
    bool retval =
            (
                    (dmod_mode == DEMOD_AM)
                    ||(dmod_mode == DEMOD_SAM && (ads.sam_sideband == SAM_SIDEBAND_BOTH))
                    || (dmod_mode == DEMOD_FM)
            );

    return retval;
}

bool RadioManagement_IsTxAtZeroIF(uint8_t dmod_mode, uint8_t digital_mode)
{
    return  (
                dmod_mode == DEMOD_CW ||
                (dmod_mode == DEMOD_DIGI &&
                    (
#ifdef USE_FREEDV
                            digital_mode == DigitalMode_FreeDV
#else
                            false
#endif
                            || is_demod_psk()
#ifdef USE_RTTY_PROCESSOR
                            || is_demod_rtty()
#endif
                    )
                )
             );
}

bool RadioManagement_FmDevIs5khz(void)
{
    return (ts.flags2 & FLAGS2_FM_MODE_DEVIATION_5KHZ) != 0;
}

void RadioManagement_Request_TxOn(void)
{
    ts.ptt_req = true;
}

void RadioManagement_Request_TxOff(void)
{
    // we have to do both here to handle a potential race condition if
    // the CAT code deasserts RTS before we actually switched to TX.
    ts.ptt_req = false;
    ts.tx_stop_req = true;
}

// ------------------------------------------------------------------
// ui_driver.c DSP mode query helpers (copied 1:1)
//
bool is_dsp_nb(void)
{
	return (ts.dsp.active & DSP_NB_ENABLE) != 0;
}

bool is_dsp_nb_active(void)
{
    return is_dsp_nb() && (ts.dsp.nb_setting > 0);
}

bool is_dsp_nr(void)
{
	return (ts.dsp.active & DSP_NR_ENABLE) != 0;
}

bool is_dsp_nr_postagc(void)
{
	return (ts.dsp.active & DSP_NR_POSTAGC_ENABLE) != 0;
}

bool is_dsp_notch(void)
{
	return (ts.dsp.active & DSP_NOTCH_ENABLE) != 0;
}

bool is_dsp_mnotch(void)
{
	return (ts.dsp.active & DSP_MNOTCH_ENABLE) != 0;
}

bool is_dsp_mpeak(void)
{
	return (ts.dsp.active & DSP_MPEAK_ENABLE) != 0;
}

// ------------------------------------------------------------------
// ui_driver.c callbacks - no UI on this core
//
// called by the audio ISR, used on the F4 for encoder/keyboard timing
void UiDriver_Callback_AudioISR(void)
{
}

// decoded RTTY/PSK characters - ToDo: forward to the M7 core via ICC
void UiDriver_TextMsgPutChar(char ch)
{
	(void)ch;
}

// ------------------------------------------------------------------
// USB audio/CDC - no USB on this core
//
void UsbdAudio_PutSample(int16_t sample)
{
	(void)sample;
}

void UsbdAudio_FillTxBuffer(AudioSample_t *buffer, uint32_t len)
{
	// no USB audio source, feed silence
	for(uint32_t i = 0; i < len; i++)
	{
		buffer[i].l = 0;
		buffer[i].r = 0;
	}
}

// CAT virtual COM port control lines (same layout as usbd_cdc_if.h)
typedef struct {
	uint16_t
	dtr:1,
	rts:1;
} CdcVcp_CtrlLines_t;

__IO CdcVcp_CtrlLines_t cdcvcp_ctrllines;

// ------------------------------------------------------------------
// profiling.c data - the inline profiling macros reference this
//
EventProfile_t eventProfile;

#endif // H7_M4_CORE
