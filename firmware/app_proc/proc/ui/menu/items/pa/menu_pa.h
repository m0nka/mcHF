/************************************************************************************
**                                                                                 **
**                             mcHF Pro QRP Transceiver                            **
**                         Krassi Atanassov - M0NKA 2012-2020                      **
**                            mail: djchrismarc@gmail.com                          **
**                                 twitter: @bph_co                                **
**---------------------------------------------------------------------------------**
**                                                                                 **
**  File name:                                                                     **
**  Description:                                                                   **
**  Last Modified:                                                                 **
**  Licence:                                                                       **
**          The mcHF project is released for radio amateurs experimentation,       **
**          non-commercial use only. All source files under GPL-3.0, unless        **
**          third party drivers specifies otherwise. Thank you!                    **
************************************************************************************/
#ifndef __MENU_PA_H
#define __MENU_PA_H

#define ID_WINDOW_0               	(GUI_ID_USER + 0x00)
//#define ID_BUTTON_EXIT            (GUI_ID_USER + 0x01)

#define ID_CHECKBOX_0				(GUI_ID_USER + 0x02)
#define ID_CHECKBOX_1				(GUI_ID_USER + 0x03)

#define ID_SLIDER_0 			  	(GUI_ID_USER + 0x04)
#define ID_SLIDER_1 			  	(GUI_ID_USER + 0x05)
#define ID_SLIDER_2 			  	(GUI_ID_USER + 0x06)

// -----------------------------------------------------
#define FRAME_X						150
#define FRAME_Y						90

#define FRAME_SZ_X					550
#define FRAME_SZ_Y					205
// -----------------------------------------------------
#define DIAG_X						260
#define DIAG_Y						90

#define DIAG_SZ_X					397
#define DIAG_SZ_Y					300
// -----------------------------------------------------
#define CHK1X						FRAME_X + 10
#define CHK1Y						FRAME_Y + 20

#define CHK2X						FRAME_X + 10
#define CHK2Y						FRAME_Y + 80
// -----------------------------------------------------
#define SLD1X						FRAME_X + 80
#define SLD1Y						FRAME_Y + 15

#define SLD1SX						FRAME_SZ_X - 150
#define SLD1SY						40
// -----------------------------------------------------
#define SLD2X						FRAME_X + 80
#define SLD2Y						FRAME_Y + 75

#define SLD2SX						FRAME_SZ_X - 150
#define SLD2SY						40
// -----------------------------------------------------
#define SLD3X						FRAME_X + 80
#define SLD3Y						FRAME_Y + 150

#define SLD3SX						FRAME_SZ_X - 150
#define SLD3SY						40
// -----------------------------------------------------
#define TXT1X						SLD1X + SLD1SX + 20
#define TXT1Y						FRAME_Y + 20

#define TXT1SX						40
#define TXT1SY						30
// -----------------------------------------------------
#define TXT2X						SLD2X + SLD2SX + 20
#define TXT2Y						FRAME_Y + 80

#define TXT2SX						40
#define TXT2SY						30
// -----------------------------------------------------
#define TXT3X						SLD3X + SLD3SX + 20
#define TXT3Y						FRAME_Y + 155

#define TXT3SX						40
#define TXT3SY						30
// -----------------------------------------------------
#define TXT4X						SLD3X + SLD3SX + 20
#define TXT4Y						320

#define TXT4SX						40
#define TXT4SY						30
// -----------------------------------------------------
#define TXT5X						SLD3X + SLD3SX + 20
#define TXT5Y						360

#define TXT5SX						40
#define TXT5SY						30
// -----------------------------------------------------


#endif
