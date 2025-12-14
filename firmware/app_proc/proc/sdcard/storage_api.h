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
**  Licence:                                                                       **
************************************************************************************/
#ifndef __STORAGE_API_H
#define __STORAGE_API_H

// -------------------------------------------------------------------------
// -------------  				Access calls			--------------------
// -------------------------------------------------------------------------

uint8_t     Storage_GetStatus	(uint8_t unit				);
uint32_t 	Storage_GetCapacity	(uint8_t unit				);
uint32_t 	Storage_GetLabel	(uint8_t unit, char *label	);
uint32_t 	Storage_GetFree		(uint8_t unit				);
uint32_t 	Storage_GetDrive	(uint8_t unit, char *disk	);

#endif
