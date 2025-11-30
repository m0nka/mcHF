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

#ifndef OS_SEDNADECL_H
#define OS_SEDNADECL_H

/* Usb driver menues */
#define DEVICE_MENU_INFORMATION				0xC1
#define SEDNA_READ_STATUS					0xC2
#define SEDNA_LOAD_APPLICATION				0xC3
#define SEDNA_UNLOAD_APPLICATION	    	0xC4
#define SEDNA_CHECK_LOAD_STATUS 	    	0xC5
#define SEDNA_SEND_APP_MESSAGE		    	0xC6
#define SEDNA_LOAD_FPGA_CONFIG		    	0xC7
#define SEDNA_TRACE_STATUS			    	0xC8
#define SEDNA_LOAD_MASTER_KEY		    	0xC9
#define SEDNA_RELOAD_OS_IMAGE		    	0xCA
#define SEDNA_READ_CERTIFICATE		    	0xCB
#define SEDNA_WRITE_CERTIFICATE		    	0xCC

#define SEDNA_LOAD_SCRIPT					0xD1
#define SEDNA_UNLOAD_SCRIPT					0xD2

/* App loader messages */
#define SEDNA_RETURN_LOAD_STATUS 	    	0xD6
#define SEDNA_RETURN_UNLOAD_STATUS 	    	0xD7
#define SEDNA_RETURN_APP_MSG_STATUS 	    0xD8


/* Comm states */
#define SEDNA_TRANSFER_SUCCESS				0x00


/* Application states */
#define SEDNA_APP_LOAD_SUCCESS				0x00
#define SEDNA_APP_UNLOAD_SUCCESS			0x00
#define SEDNA_APP_ALLOC_ERROR				0x01
#define SEDNA_APP_EXECUTE_ERROR				0x02
#define SEDNA_APP_ALREADY_RUNNING			0x03
#define SEDNA_APP_TOO_MANY_LOADED			0x04
#define SEDNA_APP_INVALID_HANDLE			0x05
#define SEDNA_APP_NOT_RUNNING				0x06
#define SEDNA_APP_LOAD_MSG_ERROR			0x07
#define SEDNA_APP_LOAD_ALOC_ERROR			0x08
#define SEDNA_APP_FUNC_SIZE_MSG_ERROR		0x09
#define SEDNA_APP_CHECK_QUEUE_EMPTY			0x10
#define SEDNA_APP_CHECK_QUEUE_WRONG_ID		0x11
#define SEDNA_APP_UNLOAD_MSG_ERROR			0x12
#define SEDNA_APP_API_BLOCK_START_ERROR		0x13
#define SEDNA_APP_API_BLOCK_CORRUPTED		0x14
#define SEDNA_APP_ELF_NOT_VALID				0x15
#define SEDNA_APP_ELF_NO_SYMBOL_TABLE 		0x16
#define SEDNA_APP_ELF_PRG_SECT_MISSING 		0x17
#define SEDNA_APP_ELF_PRG_SECT_ALLOC_FAIL	0x18
#define SEDNA_APP_ELF_IDA_SECT_ALLOC_FAIL 	0x19
#define SEDNA_APP_ELF_UDA_SECT_ALLOC_FAIL 	0x20
#define SEDNA_APP_RETURN_MSG_BUFFER_EMPTY 	0x21
#define SEDNA_APP_ELF_DATA_SECTION_TOO_BIG	0x22
#define SEDNA_FPGA_CONFIG_LOAD_ERROR		0x23
#define SEDNA_FPGA_UNSUPPORTED_MODE			0x24

#define SEDNA_MAXIMUM_APPLICATIONS			   2
#define MAX_APPLICATION_THREADS				   2

#define SEDNA_USB_FTDI_RET_BUFF_ALLOC_ERR	0x44
#define SEDNA_APP_HANDLE_MISMATCH			0x45


#endif
