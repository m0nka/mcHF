/************************************************************************************
**                                                                                 **
**                          GENIE RTOS AT91 IMPLEMENTATION                         **
**                                  B-phreaks, 2005                                **
**                                                                                 **
**---------------------------------------------------------------------------------**
**                                                                                 **
**  File name:                                                                     **
**  Description:                                                                   **
**  Last Modified:                                                                 **
**                                                                                 **
************************************************************************************/

#ifndef __OS_SEDNAELF_H
#define __OS_SEDNAELF_H

#define EM_ARM		1
#define BIG_ENDIAN  0

#define ARM_RW_VIRTUAL_PTR		0x4444

uchar ucSednaElfProcessElf(uchar *ucProcessImage);


#endif
