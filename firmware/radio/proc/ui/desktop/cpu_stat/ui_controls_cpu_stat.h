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
#ifndef UI_CONTROLS_CPU_STAT_H
#define UI_CONTROLS_CPU_STAT_H

// ----------------------------------------------------------------------------
// Still alive blinker
//
#define BLINKER_X					(CPU_L_X + 5)
#define BLINKER_Y					(CPU_L_Y + 4)

// Exports
void ui_controls_cpu_stat_init(void);
void ui_controls_cpu_stat_quit(void);

void ui_controls_cpu_stat_touch(void);
void ui_controls_cpu_stat_refresh(void);

#endif
