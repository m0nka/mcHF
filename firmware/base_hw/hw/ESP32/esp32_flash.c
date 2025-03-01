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
//---------------------------------------------------------------------------
#include "mchf_pro_board.h"
#include "main.h"

#include "esp32_flash.h"

int Sync(unsigned int timeout)
{
	int stat = 0;
	unsigned char syncData[36] = {
		0x07, 0x07, 0x12, 0x20, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55,
		0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55,
		0x55, 0x55, 0x55, 0x55
	};

	if (comand(ESP_SYNC, syncData,36,0))
	{
//		FlushComm();     +
		//OutputDebugString("Sync->Success");
	}
	else
	{
        return ESP_ERROR_GENERAL;
		// read and discard additional replies
//		while (readPacket(ESP_SYNC) == 2)
//			;
	}
	return ESP_SUCCESS;
}

bool comand(unsigned char op, unsigned char* data,unsigned int dataSize, unsigned char checksum)
{
	unsigned char op_ret = 0;
	unsigned long byteswritten = 0;
	unsigned long readExpected = 2;
	unsigned long bytesRead = 0;
	unsigned long val;
    unsigned char body[10];

	unsigned char ucWriteBuffer[8000];
	unsigned long ulWriteSize = dataSize + 10;

	unsigned char receive_responce = 100;

	memset(ucWriteBuffer, 0x00, sizeof(ucWriteBuffer));

	ucWriteBuffer[1] = 0x00;
	ucWriteBuffer[2] = op;
	ucWriteBuffer[3] = (unsigned char)dataSize;
	ucWriteBuffer[4] = (unsigned char)(dataSize >> 8);
	ucWriteBuffer[5] = checksum;
	ucWriteBuffer[6] = 0x00;

	//DebugPrintHexArray(data,ulWriteSize,"data:");
	//DebugPrintInteger(ulWriteSize,"ulWriteSize = ");

	memcpy(ucWriteBuffer+9,data,dataSize);

    // second - patch the file
	// replace 0xDB with 0xDB, 0xDC
	// replace 0x0C with 0xDB, 0xDC

	//DebugPrintHexArray(ucWriteBuffer,ulWriteSize,"ucWriteBuffer:");
	//DebugPrintInteger(ulWriteSize,"ulWriteSize = ");

	ulWriteSize = patch_block(ucWriteBuffer, ulWriteSize);

	// put 0xC0 - the first and the lates bytes of the transfer
	ucWriteBuffer[0] = 0xC0;
	ucWriteBuffer[ulWriteSize-1] = 0xC0;

	//DebugPrintInteger(ucWriteBuffer[130], "ucWriteBuffer[130] = ");
	//DebugPrintHexArray(data,dataSize,"ucBuffer:");
	//DebugPrintHexArray(ucWriteBuffer,ulWriteSize,"ucWriteBuffer:");
	//DebugPrintInteger(ulWriteSize,"ulWriteSize = ");

	if (WriteFile(ucWriteBuffer,ulWriteSize,&byteswritten,NULL))
	{
		//DebugPrintInteger(byteswritten,"byteswritten = ");
		//DebugPrintHexArray(ucWriteBuffer,ulWriteSize,"ucWriteBuffer:");
		//SaveFile("d:\\data.bin",ucWriteBuffer, ulWriteSize);
		if(byteswritten != ulWriteSize)
            return false;
		//Sleep(1000);

		while (receive_responce > 0)
		{
			if(receive_response(&op_ret,  &val,  body))
			{
				if(op == op_ret)
				{
                    //OutputDebugString("Return-True");
					return true;
                }
			}
			receive_responce = receive_responce - 1;
			(10);
		}
	}
	else
	{
            //DebugPrintInteger(byteswritten,"byteswritten = ");
			//OutputDebugString("Error write!");
            return false;
	}

	if(receive_responce == 0)
        return false;

	return true;
}

bool receive_response(unsigned char* op_ret, unsigned long* val, unsigned char* body)
{
	unsigned long readExpected = 1;
	unsigned long bytesRead = 0;
	unsigned char read[1024];

	unsigned char memtmp[512];

	// read frame start byte - 0xC0
	if (ReadFile(&read,readExpected,&bytesRead,NULL))
	{
		//DebugPrintHexArray(read, readExpected, "read: ");
		//DebugPrintInteger(bytesRead, "bytesread = ");
		if (bytesRead != readExpected)
		{
			//CloseHandle(hSerial);
			return false;
		}
	}
	else
	{
			//OutputDebugString("Error read!");
	}

	if(read[0] != 0xC0)
		return false;

	memset(read, 0x00, sizeof(read));

	// read responce header
	readExpected = 8;

	if (ReadFile(&read,readExpected,&bytesRead,NULL))
	{
		//DebugPrintHexArray(read, readExpected, "read: ");
		//DebugPrintInteger(bytesRead, "bytesread = ");

		if (bytesRead != readExpected)
		{
			//CloseHandle(hSerial);
			return false;
		}
	}
	else
	{
			//OutputDebugString("Error read!");
	}

	*op_ret = read[1];

	readExpected = read[2];

	memset(memtmp,0x00,sizeof(memtmp));
	memcpy(memtmp, read, 8);
	memset(read, 0x00, sizeof(read));

	if (ReadFile(&read,readExpected,&bytesRead,NULL))
	{
		//DebugPrintHexArray(read, readExpected, "read: ");
		//DebugPrintInteger(bytesRead, "bytesread = ");
		if (bytesRead != readExpected)
		{
			//CloseHandle(hSerial);
			return false;
		}
	}
	else
	{
			//OutputDebugString("Error read!");
	}

	readExpected = 1;

	// read frame end byte - 0xC0
	if (ReadFile(&read,readExpected,&bytesRead,NULL))
	{
		//DebugPrintHexArray(read, readExpected, "read: ");
		//DebugPrintInteger(bytesRead, "bytesread = ");
		if (bytesRead != readExpected)
		{
			//CloseHandle(hSerial);
			return false;
		}
	}
	else
	{
			//OutputDebugString("Error read!");
	}

	if(read[0] != 0xC0)
		return false;

	*val =  (memtmp[7] << 24) + (memtmp[6] << 16) + (memtmp[5] << 8) + memtmp[4];

    //DebugPrintInteger(*val, "val = ");

	return true;
}

int read_reg(unsigned long address, unsigned long* retData)
{
	unsigned char addr_array[4];

	addr_array[0] = (unsigned char)address;
	addr_array[1] = (unsigned char)(address >> 8);
	addr_array[2] = (unsigned char)(address >> 16);
	addr_array[3] = (unsigned char)(address >> 24);

	if (comand(ESP_READ_REG, addr_array,4,0))
	{
//		FlushComm();
		return ESP_SUCCESS;
	}
	else
	{
		// read and discard additional replies
//		while (readPacket(ESP_SYNC) == 2)
//			;
	}

	return 0;
}


int flash_erase(void)
{
	flash_begin(0, 0);
	mem_begin  (0,0,0,0x40100000);
	mem_finish (0x40004984);
	return 0;
}

int flash_begin(unsigned long size, unsigned long offset)
{
	unsigned long num_blocks;
	num_blocks = (size + ESP_FLASH_BLOCK - 1) / ESP_FLASH_BLOCK;

	unsigned long sectors_per_block = 16;
	unsigned long sector_size = 4096;
	unsigned long num_sectors = (size + sector_size - 1) / sector_size;
	unsigned long start_sector = offset / sector_size;
	unsigned long erase_size = 0;

	unsigned char flash_begin_command[16];

	unsigned long head_sectors = sectors_per_block - (start_sector % sectors_per_block);

	if(num_sectors < head_sectors)
		head_sectors = num_sectors;

	if (num_sectors < (head_sectors*2))
		erase_size = (num_sectors + 1) / 2 * sector_size;
	else
		erase_size = (num_sectors - head_sectors) * sector_size;

	memset(flash_begin_command,0x00,sizeof(flash_begin_command));

	flash_begin_command[0] = (unsigned char)erase_size;
	flash_begin_command[1] = (unsigned char)(erase_size >> 8);
	flash_begin_command[2] = (unsigned char)(erase_size >> 16);
	flash_begin_command[3] = (unsigned char)(erase_size >> 24);
	flash_begin_command[4] = (unsigned char)num_blocks;
	flash_begin_command[5] = (unsigned char)(num_blocks >> 8);
	flash_begin_command[6] = (unsigned char)(num_blocks >> 16);
	flash_begin_command[7] = (unsigned char)(num_blocks >> 24);
	flash_begin_command[8] = (unsigned char)ESP_FLASH_BLOCK;
	flash_begin_command[9] = (unsigned char)(ESP_FLASH_BLOCK >> 8);
	flash_begin_command[10] = (unsigned char)(ESP_FLASH_BLOCK >> 16);
	flash_begin_command[11] = (unsigned char)(ESP_FLASH_BLOCK >> 24);
	flash_begin_command[12] = (unsigned char)offset;
	flash_begin_command[13] = (unsigned char)(offset >> 8);
	flash_begin_command[14] = (unsigned char)(offset >> 16);
	flash_begin_command[15] = (unsigned char)(offset >> 24);

	//DebugPrintHexArray(flash_begin_command,16,"flash_begin_command: ");

	if (comand(ESP_FLASH_BEGIN, flash_begin_command,16,0))
	{
//		FlushComm();
		return ESP_SUCCESS;
	}
	else
	{
		// read and discard additional replies
//		while (readPacket(ESP_SYNC) == 2)
//			;
	}

	return 0;
}


int flash_finish(unsigned long reboot)
{
	unsigned char flash_finish_command[4];
	memset(flash_finish_command,0x00,sizeof(flash_finish_command));

	flash_finish_command[0] = (unsigned char)reboot;
	flash_finish_command[1] = (unsigned char)(reboot >> 8);
	flash_finish_command[2] = (unsigned char)(reboot >> 16);
	flash_finish_command[3] = (unsigned char)(reboot >> 24);

	//DebugPrintHexArray(flash_finish_command,4,"mem_begin_command: ");

	if (comand(ESP_FLASH_END, flash_finish_command,4,0)){

//		FlushComm();
		return ESP_SUCCESS;
	}
	else
	{
		// read and discard additional replies
//		while (readPacket(ESP_SYNC) == 2)
//			;
	}

	return 0;
}

int mem_begin(unsigned long size, unsigned long blocks, unsigned long blocksize, unsigned long offset)
{
	unsigned char mem_begin_command[16];
	memset(mem_begin_command,0x00,sizeof(mem_begin_command));

	mem_begin_command[0] = (unsigned char)size;
	mem_begin_command[1] = (unsigned char)(size >> 8);
	mem_begin_command[2] = (unsigned char)(size >> 16);
	mem_begin_command[3] = (unsigned char)(size >> 24);
	mem_begin_command[4] = (unsigned char)blocks;
	mem_begin_command[5] = (unsigned char)(blocks >> 8);
	mem_begin_command[6] = (unsigned char)(blocks >> 16);
	mem_begin_command[7] = (unsigned char)(blocks >> 24);
	mem_begin_command[8] = (unsigned char)blocksize;
	mem_begin_command[9] = (unsigned char)(blocksize >> 8);
	mem_begin_command[10] = (unsigned char)(blocksize >> 16);
	mem_begin_command[11] = (unsigned char)(blocksize >> 24);
	mem_begin_command[12] = (unsigned char)offset;
	mem_begin_command[13] = (unsigned char)(offset >> 8);
	mem_begin_command[14] = (unsigned char)(offset >> 16);
	mem_begin_command[15] = (unsigned char)(offset >> 24);

	//DebugPrintHexArray(mem_begin_command,16,"mem_begin_command: ");

	if (comand(ESP_MEM_BEGIN, mem_begin_command,16,0))
	{
//		FlushComm();
		return ESP_SUCCESS;
	}
	else
	{
		// read and discard additional replies
//		while (readPacket(ESP_SYNC) == 2)
//			;
	}

	return 0;
}

int mem_finish(unsigned long entrypoint1)
{

	unsigned long entrypoint0 = 0;
	unsigned char mem_finish_command[8];

	memset(mem_finish_command,0x00,sizeof(mem_finish_command));

	mem_finish_command[0] = (unsigned char)entrypoint0;
	mem_finish_command[1] = (unsigned char)(entrypoint0 >> 8);
	mem_finish_command[2] = (unsigned char)(entrypoint0 >> 16);
	mem_finish_command[3] = (unsigned char)(entrypoint0 >> 24);
	mem_finish_command[4] = (unsigned char)entrypoint1;
	mem_finish_command[5] = (unsigned char)(entrypoint1 >> 8);
	mem_finish_command[6] = (unsigned char)(entrypoint1 >> 16);
	mem_finish_command[7] = (unsigned char)(entrypoint1 >> 24);

	DebugPrintHexArray(mem_finish_command,16,"mem_finish_command: ");

	if (comand(ESP_MEM_END, mem_finish_command,8,0))
	{
//		FlushComm();
		return ESP_SUCCESS;
	}
	else
	{
		// read and discard additional replies
//		while (readPacket(ESP_SYNC) == 2)
//			;
	}

	return 0;
}

int flash_block(unsigned char* data, unsigned long dataSize, unsigned long seq)
{
	unsigned char ucpBlockTmp[8000];
	unsigned char checksum;

	memset(ucpBlockTmp,0x00,sizeof(ucpBlockTmp));

	// make the checksum
	checksum = make_xor_checksum(data, dataSize);

	ucpBlockTmp[0] = (unsigned char)dataSize;
	ucpBlockTmp[1] = (unsigned char)(dataSize >> 8);
	ucpBlockTmp[2] = (unsigned char)(dataSize >> 16);
	ucpBlockTmp[3] = (unsigned char)(dataSize >> 24);
	ucpBlockTmp[4] = (unsigned char)seq;
	ucpBlockTmp[5] = (unsigned char)(seq >> 8);
	ucpBlockTmp[6] = (unsigned char)(seq >> 16);
	ucpBlockTmp[7] = (unsigned char)(seq >> 24);

	memcpy(ucpBlockTmp + 16, data, dataSize);

	//DebugPrintHexArray(ucpBlockTmp, 64, "ucpBlockTmp:");

	if (comand(ESP_FLASH_DATA, ucpBlockTmp,dataSize+16,checksum))
	{
//		FlushComm();
		return ESP_SUCCESS;
	}
	else
	{
		// read and discard additional replies
//		while (readPacket(ESP_SYNC) == 2)
//			;
	}

}

unsigned char make_xor_checksum(unsigned char* data, unsigned long dataSize)
{
	unsigned long i;
	unsigned char check = 0xEF;

	//DebugPrintInteger(dataSize, "DataSize =");

	for (i = 0; i < dataSize; i++)
	{
		check = check ^ data[i];
	}	return check;
}

unsigned int patch_block(unsigned char* data, unsigned long dataSize)
{
	unsigned int i;
	unsigned char dataTmp[8000];
	unsigned long int k = 0;

	i = dataSize;

	memcpy(dataTmp,data,dataSize);

//	DebugPrintHexArray(dataTmp,dataSize,"dataTmp:");

	do{
		if(dataTmp[k] == 0xDB)
		{
			//OutputDebugString("Patching");
			memcpy(dataTmp+k+1,dataTmp+k,dataSize-k);
			dataTmp[k] = 0xDB;
			dataTmp[k+1] = 0xDD;
			i = i + 1;
			dataSize = dataSize + 1;

		}
		k = k + 1;
		i = i - 1;
		//OutputDebugString("Looooop");
	}
	while (i>0);

 //	memcpy(dataTmp,data,dataSize);

	i = dataSize;
	k = 0;

//    DebugPrintHexArray(dataTmp,dataSize,"dataTmp Before 2:");

	do{
		if(dataTmp[k] == 0xC0)
		{
			//OutputDebugString("Patching1");
			memcpy(dataTmp+k+1,dataTmp+k,dataSize-k);
			dataTmp[k] = 0xDB;
			dataTmp[k+1] = 0xDC;
			i = i + 1;
			dataSize = dataSize + 1;
		}
		k = k + 1;
		i = i - 1;
		//OutputDebugString("Looooop");
	}
	while (i>0);

//	DebugPrintHexArray(dataTmp,dataSize,"dataTmp:");
	memcpy(data,dataTmp,dataSize);

	return dataSize;
}

//    call as:
//    Write_flash(hSerial, espfile, sizeof(espfile), 0);
//
int Write_flash(unsigned char* pData, unsigned long pDataSize, unsigned long startAddress)
{
	unsigned long i=0;

	unsigned char pTemp[2000];
	unsigned long num_blocks;

	num_blocks = pDataSize / 0x400;

	//TCGauge      *pGauge = NULL;
	//pGauge = (TCGauge *)iSSClient->pGaugePtr;
	//pGauge->Progress = 0;
	//pGauge->MaxValue = num_blocks - 1;

	flash_begin(pDataSize, startAddress);

	for (i = 0; i < num_blocks; i++)
	{
		memcpy(pTemp, pData+(i*0x400),0x400);
		flash_block(pTemp, 0x400, i);
		//pGauge->Progress = i;
	}

	flash_begin (0, 0);
	flash_finish(1);
}

int uploadStubFile(unsigned char* pData, unsigned long pDataSize, unsigned long startAddress)
{
	unsigned long i=0;

	unsigned char pTemp[8000];
	unsigned long num_blocks;

	unsigned char sBlock[4] = {0x08, 0xc0, 0xfe, 0x3f};

	num_blocks = 1;//pDataSize / 0x1800;

	//TCGauge      *pGauge = NULL;
	//pGauge = (TCGauge *)iSSClient->pGaugePtr;
	//pGauge->Progress = 0;
	//pGauge->MaxValue = num_blocks - 1;

	//DebugPrintInteger(pDataSize, "pDataSize = ");
	//DebugPrintHexArray(pData, 48, "pData:");

	mem_begin(pDataSize, num_blocks, 0x1800, startAddress);


	for (i = 0; i < num_blocks; i++)
	{
		memcpy(pTemp, pData+(i*0x1800),0x1800);
		write_memory_block(pTemp, pDataSize, i);
		//pGauge->Progress = i;
	}

	mem_begin(4, num_blocks, 0x1800, 0x3fffeba4);

	write_memory_block(sBlock, 4, 0);

	mem_finish(0x4009f574);

	unsigned long readExpected = 1;
	unsigned long bytesRead = 0;
	unsigned char read[1024];
		readExpected = 6;

	// read frame end byte - 0xC0
	if (ReadFile(&read,readExpected,&bytesRead,NULL))
	{
		//DebugPrintHexArray(read, readExpected, "read: ");
		//DebugPrintInteger(bytesRead, "bytesread = ");
		if (bytesRead != readExpected)
		{
			//CloseHandle(hSerial);
			return false;
		}
	}
	else
	{
			//OutputDebugString("Error read!");
	}

//	flash_begin(hSerial, 0, 0);
//	flash_finish(hSerial, 1);
}

int write_memory_block(unsigned char* data, unsigned long dataSize, unsigned long seq)
{
	unsigned char ucpBlockTmp[8000];
	unsigned char checksum;

	memset(ucpBlockTmp,0x00,sizeof(ucpBlockTmp));

	// make the checksum
	checksum = make_xor_checksum(data, dataSize);

	ucpBlockTmp[0] = (unsigned char)dataSize;
	ucpBlockTmp[1] = (unsigned char)(dataSize >> 8);
	ucpBlockTmp[2] = (unsigned char)(dataSize >> 16);
	ucpBlockTmp[3] = (unsigned char)(dataSize >> 24);
	ucpBlockTmp[4] = (unsigned char)seq;
	ucpBlockTmp[5] = (unsigned char)(seq >> 8);
	ucpBlockTmp[6] = (unsigned char)(seq >> 16);
	ucpBlockTmp[7] = (unsigned char)(seq >> 24);

	memcpy(ucpBlockTmp + 16, data, dataSize);

	//DebugPrintHexArray(ucpBlockTmp, 128, "ucpBlockTmp:");

	if (comand(ESP_MEM_DATA, ucpBlockTmp,dataSize+16,checksum))
	{
//		FlushComm();
		return ESP_SUCCESS;
	}
	else
	{
		// read and discard additional replies
//		while (readPacket(ESP_SYNC) == 2)
//			;
	}
}

bool READESP32MAC(void)
{
#if 0
	char ret_buff[200];
	char comID[20];
	DWORD byteswritten;
	unsigned char read[64];
	DWORD bytesread;
	unsigned char ucBuffer[8] = {0x0B,0x00,0x00,0x00,0x00,0x00,0x00,0x00};
	unsigned char digital[10];
	HANDLE hSerial;
	unsigned char i;
	unsigned char j;
	bool connection_established = false;

	OutputDebugString("READESP32MAC->Entry.");


	memset(ret_buff,0x00,sizeof(ret_buff));
	memset(comID,0x00, sizeof(comID));

	strcpy(comID, "\\\\.\\");
	strcat(comID, iSSClient->asComPort.c_str());

	OutputDebugString(comID);

	hSerial = CreateFile( comID,
						 GENERIC_READ | GENERIC_WRITE,
						 0,
						 0,
						 OPEN_EXISTING,
						 FILE_ATTRIBUTE_NORMAL,
						 0);

	if (hSerial == INVALID_HANDLE_VALUE)
	{
		OutputDebugString("Cannot open COM port!");
		CloseHandle(hSerial);
		return false;
	}

	DCB dcb;
	memset(&dcb, 0, sizeof(dcb));
	dcb.DCBlength = sizeof(dcb);
	if (GetCommState(hSerial, &dcb))
	{
		dcb.BaudRate = ESP_ROM_BAUD;
		if (!SetCommState(hSerial, &dcb))
		{
			OutputDebugString("Error Set Comm State");
			return false;
			}
	}

	DebugPrintInteger((unsigned int)hSerial, "Com HANDLE = ");

	COMMTIMEOUTS timeouts;
	timeouts.ReadIntervalTimeout = 20;
	timeouts.ReadTotalTimeoutMultiplier = 10;
	timeouts.ReadTotalTimeoutConstant = 100;
	timeouts.WriteTotalTimeoutMultiplier = 10;
	timeouts.WriteTotalTimeoutConstant = 100;

	if (!SetCommTimeouts(hSerial, &timeouts)) // Error setting time-outs.
		return false;

	for (j = 0; j < 4; j++)
	{
		PurgeComm(hSerial, PURGE_TXCLEAR | PURGE_RXCLEAR);
		if (Sync(hSerial, 500) == ESP_SUCCESS)
		{
			connection_established = true;
			break;
		}
		else
		{
			OutputDebugString("Error Connection");
        }
	}

	if(!connection_established){
		CloseHandle(hSerial);

		Sleep(100);

		memset(ret_buff,0x00,sizeof(ret_buff));
		memset(comID,0x00, sizeof(comID));

		strcpy(comID, "\\\\.\\");
		strcat(comID, iSSClient->asComPort.c_str());

		OutputDebugString(comID);

		hSerial = CreateFile( comID,
							 GENERIC_READ | GENERIC_WRITE,
							 0,
							 0,
							 OPEN_EXISTING,
							 FILE_ATTRIBUTE_NORMAL,
							 0);

		if (hSerial == INVALID_HANDLE_VALUE)
		{
			OutputDebugString("Cannot open COM port!");
			CloseHandle(hSerial);
			return false;
		}

		memset(&dcb, 0, sizeof(dcb));
		dcb.DCBlength = sizeof(dcb);
		if (GetCommState(hSerial, &dcb))
		{
			dcb.BaudRate = ESP_ROM_BAUD;
			if (!SetCommState(hSerial, &dcb))
			{
				OutputDebugString("Error Set Comm State");
				return false;
			}
		}

		for (j = 0; j < 4; j++)
		{
			PurgeComm(hSerial, PURGE_TXCLEAR | PURGE_RXCLEAR);
			if (Sync(hSerial, 500) == ESP_SUCCESS)
			{
				break;
			}
			else
			{
				OutputDebugString("Error Connection");
			}
		}
	}

	unsigned long address = 0x00;
    unsigned long *retData = 0x00;

	read_reg(hSerial,ESP32_OTP_MAC0,retData);

	read_reg(hSerial,ESP32_OTP_MAC1,retData);

	read_reg(hSerial,ESP32_OTP_MAC2,retData);

	read_reg(hSerial,ESP32_OTP_MAC3,retData);

	read_reg(hSerial,ESP32_OTP_MAC3+4,retData);

	read_reg(hSerial,ESP32_OTP_MAC3+8,retData);

	read_reg(hSerial,ESP32_OTP_MAC3+12,retData);

//    Write_flash(hSerial, espfile, sizeof(espfile), 0);


	//ComExchange(hSerial,ucBuffer,8,read,3, &bytesread);


//	flash_begin(hSerial,0,0);
//	unsigned char checksum = make_xor_checksum(pBlock, sizeof(pBlock));
//	DebugPrintInteger(checksum, "checksum =");
//  	flash_block(hSerial,pBlock, sizeof(pBlock), 0);

	//Close COM
	CloseHandle(hSerial);

	OutputDebugString("READESP32MAC->Exit.");
#endif
	return true;
}

bool ESP32WriteStub(void)
{
#if 0
	char ret_buff[200];
	char comID[20];
	DWORD byteswritten;
	unsigned char read[64];
	DWORD bytesread;

	HANDLE hSerial;
	unsigned char i;
	unsigned char j;

	bool connection_established = false;

	OutputDebugString("ESP32WriteStub->Entry.");



//	if(!CheckFileExists(s2.c_str())){
//		OutputDebugString("bin file missing");
//        return false;
//	}

	unsigned long		ulLoaderAddr = NULL;
	unsigned long		usFileSize = 0;
	AnsiString s2;

	unsigned char *pNewfile;

	usFileSize = sizeof(ESP32Stub);
	unsigned long ulNumBlocks = usFileSize / 0x1800;

	DebugPrintInteger(ulNumBlocks, "ulNumBlocks =");

	if(ulNumBlocks*0x1800 != usFileSize){
		ulNumBlocks = ulNumBlocks + 1;
	}

	pNewfile =(UCHAR *)malloc(ulNumBlocks*0x1800);

	memset(pNewfile,0xFF, ulNumBlocks*0x1800);

	memcpy(pNewfile,ESP32Stub,ulNumBlocks*0x1800);

	DebugPrintInteger(usFileSize, "usFileSize =");

	memset(ret_buff,0x00,sizeof(ret_buff));
	memset(comID,0x00, sizeof(comID));

	strcpy(comID, "\\\\.\\");
	strcat(comID, iSSClient->asComPort.c_str());

	OutputDebugString(comID);

	hSerial = CreateFile( comID,
						 GENERIC_READ | GENERIC_WRITE,
						 0,
						 0,
						 OPEN_EXISTING,
						 FILE_ATTRIBUTE_NORMAL,
						 0);

	if (hSerial == INVALID_HANDLE_VALUE)
	{
		OutputDebugString("Cannot open COM port!");
		CloseHandle(hSerial);
		return false;
	}

	DCB dcb;
	memset(&dcb, 0, sizeof(dcb));
	dcb.DCBlength = sizeof(dcb);
	if (GetCommState(hSerial, &dcb))
	{
		dcb.BaudRate = ESP_ROM_BAUD;
		if (!SetCommState(hSerial, &dcb))
		{
			OutputDebugString("Error Set Comm State");
			return false;
			}
	}

	DebugPrintInteger((unsigned int)hSerial, "Com HANDLE = ");

	COMMTIMEOUTS timeouts;
	timeouts.ReadIntervalTimeout = 20;
	timeouts.ReadTotalTimeoutMultiplier = 20;
	timeouts.ReadTotalTimeoutConstant = 1000;
	timeouts.WriteTotalTimeoutMultiplier = 10;
	timeouts.WriteTotalTimeoutConstant = 1000;

	if (!SetCommTimeouts(hSerial, &timeouts)) // Error setting time-outs.
		return false;

	for (j = 0; j < 4; j++)
	{
        PurgeComm(hSerial, PURGE_TXCLEAR | PURGE_RXCLEAR);
		if (Sync(hSerial, 500) == ESP_SUCCESS)
		{
			connection_established = true;
			break;
		}
		else
		{
			OutputDebugString("Error Connection");
			CloseHandle(hSerial);

			Sleep(100);

			memset(ret_buff,0x00,sizeof(ret_buff));
			memset(comID,0x00, sizeof(comID));

			strcpy(comID, "\\\\.\\");
			strcat(comID, iSSClient->asComPort.c_str());

			OutputDebugString(comID);

			hSerial = CreateFile( comID,
							 GENERIC_READ | GENERIC_WRITE,
							 0,
							 0,
							 OPEN_EXISTING,
							 FILE_ATTRIBUTE_NORMAL,
							 0);

			if (hSerial == INVALID_HANDLE_VALUE)
			{
				OutputDebugString("Cannot open COM port!");
				CloseHandle(hSerial);
				return false;
			}

			memset(&dcb, 0, sizeof(dcb));
			dcb.DCBlength = sizeof(dcb);
			if (GetCommState(hSerial, &dcb))
			{
				dcb.BaudRate = ESP_ROM_BAUD;
				if (!SetCommState(hSerial, &dcb))
				{
					OutputDebugString("Error Set Comm State");
					return false;
				}
			}
        }
	}

	if(!connection_established){
		CloseHandle(hSerial);

		Sleep(100);

		memset(ret_buff,0x00,sizeof(ret_buff));
		memset(comID,0x00, sizeof(comID));

		strcpy(comID, "\\\\.\\");
		strcat(comID, iSSClient->asComPort.c_str());

		OutputDebugString(comID);

		hSerial = CreateFile( comID,
							 GENERIC_READ | GENERIC_WRITE,
							 0,
							 0,
							 OPEN_EXISTING,
							 FILE_ATTRIBUTE_NORMAL,
							 0);

		if (hSerial == INVALID_HANDLE_VALUE)
		{
			OutputDebugString("Cannot open COM port!");
			CloseHandle(hSerial);
			return false;
		}

		memset(&dcb, 0, sizeof(dcb));
		dcb.DCBlength = sizeof(dcb);
		if (GetCommState(hSerial, &dcb))
		{
			dcb.BaudRate = ESP_ROM_BAUD;
			if (!SetCommState(hSerial, &dcb))
			{
				OutputDebugString("Error Set Comm State");
				return false;
			}
		}

		for (j = 0; j < 4; j++)
		{
			PurgeComm(hSerial, PURGE_TXCLEAR | PURGE_RXCLEAR);
			if (Sync(hSerial, 500) == ESP_SUCCESS)
			{
				break;
			}
			else
			{
				OutputDebugString("Error Connection");
			}
		}
	}

	timeouts.ReadIntervalTimeout = 20;
	timeouts.ReadTotalTimeoutMultiplier = 20;
	timeouts.ReadTotalTimeoutConstant = 4000;
	timeouts.WriteTotalTimeoutMultiplier = 10;
	timeouts.WriteTotalTimeoutConstant = 1000;

	if (!SetCommTimeouts(hSerial, &timeouts)) // Error setting time-outs.
		return false;

	memset(&dcb, 0, sizeof(dcb));
	dcb.DCBlength = sizeof(dcb);
	if (GetCommState(hSerial, &dcb))
	{
		dcb.BaudRate = ESP_ROM_BAUD;
		if (!SetCommState(hSerial, &dcb))
		{
			OutputDebugString("Error Set Comm State");
			return false;
		}
	}

	OutputDebugString("ESP32WriteStub->1.");

	unsigned long *retData = 0x00;

	read_reg(hSerial,ESP32_OTP_MAC0,retData);



	read_reg(hSerial,ESP32_OTP_MAC2,retData);

	read_reg(hSerial,ESP32_OTP_MAC3,retData);

	read_reg(hSerial,ESP32_OTP_MAC3+4,retData);

	read_reg(hSerial,ESP32_OTP_MAC3+8,retData);

	read_reg(hSerial,ESP32_OTP_MAC3+12,retData);

    read_reg(hSerial,ESP32_OTP_MAC1,retData);


	uploadStubFile(iSSClient, aForm, hSerial, pNewfile, usFileSize, 0x4009F000);



	//Close COM
	CloseHandle(hSerial);

	OutputDebugString("ESP32WriteStub->Exit.");

#endif
	return true;
}

