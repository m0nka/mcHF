/************************************************************************************
**                                                                                 **
**                             mcHF Pro QRP Transceiver                            **
**                         Krassi Atanassov - M0NKA, 2013-2021                     **
**                                                                                 **
**---------------------------------------------------------------------------------**
**                                                                                 **
**  File name:                                                                     **
**  Description:                                                                   **
**  Last Modified:                                                                 **
**  Licence:       The mcHF project is released for radio amateurs experimentation **
**               and non-commercial use only.Check 3rd party drivers for licensing **
************************************************************************************/
#ifndef __ON_SCREEN_AUDIO
#define __ON_SCREEN_AUDIO

uchar on_screen_audio_init(WM_HWIN hParent);
void on_screen_audio_quit(void);
void on_screen_audio_refresh(void);

#endif
