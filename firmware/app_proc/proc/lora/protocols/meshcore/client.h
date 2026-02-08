/************************************************************************************
**                                                                                 **
**                                 mcHF QRP Transceiver                            **
**                         Krassi Atanassov - M0NKA, 2013-2026                     **
**                                                                                 **
**---------------------------------------------------------------------------------**
**                                                                                 **
**  File name:                                                                     **
**  Description:                                                                   **
**  Last Modified:                                                                 **
**  Licence:			https://github.com/m0nka/mcHF/blob/main/LICENSE            **
************************************************************************************/
#ifndef __CLIENT_H
#define __CLIENT_H

// Full test of the library
//#define MESHCORE_UNIT_TEST

void client_unit_test(void);
void client_decode(struct LORA_PACKET_RX *lp, uchar *msg, ushort size, char *notif);

#endif
