/************************************************************************************
**                                                                                 **
**                          GENIE RTOS SAM7 IMPLEMENTATION                         **
**                                  B-phreaks, 2006                                **
**                                                                                 **
**---------------------------------------------------------------------------------**
**                                                                                 **
**  File name:                                                                     **
**  Description:                                                                   **
**  Last Modified:                                                                 **
**                                                                                 **
************************************************************************************/
#include "main.h"
#include "mchf_pro_board.h"

#ifdef CONTEXT_APP

#include "os_sednadecl.h"
#include "os_elf.h"
#include "os_sednaelf.h"

/* Validates that the file is actually an ELF file
 * by checking the first four bytes. Returns zero if it isn't
 * an ELF file, non-zero if it is */
static int iSednaElfIsValidElf(uchar *ucImagePtr);

/* Loads the string table section pointed to by the ELF header's
 * e_shstrndx field and also loads the symbol table's string table */
static int iSednaElfLoadNames(Elf32_Ehdr *header, uchar *ucImagePtr);

/* Procedure that displays section information in rather brief columns */
static void vSednaElfParceSections(Elf32_Ehdr *header, uchar *image, uint *offcet, uint *size,uint *virt_addr,const char *cSectionName);

/* Parce all instructions in the program data section to find any offcet
   and patch it with correct data section pointer - the Program data section 
   ptr + Program Data Section size should be the pointer to Data section */
static void vSednaElfParceOffcets(uchar *ucProgramSection,uint uiProgramSectionSize,uint uiR0VirtAddr);

/* Function to swap the byte-order for Elf32_Word items (ints) */
static Elf32_Word ewSednaElfEwordSwap(Elf32_Word word);

/* Function to swap the byte-order for Elf32_Half items (ushorts) */
static Elf32_Half ehSednaElfEhalfSwap(Elf32_Half half);

/* Local memcpy */
static void vElfMemCopy(uchar *pDestBuffer, uchar *pSourceBuffer,uint nCpySize);

/* Local memset */
static void vElfMemSet(uchar *pDestBuffer, uchar ucValue,uint nCpySize);

/* global sections name table */
char *name_table = NULL;

/* global name table for the symbol table */
char *sym_name_table = NULL;

/* Endian var */
uchar byte_order;

//*----------------------------------------------------------------------------
//* Function Name       : ucSednaElfProcessElf
//* Object              : Extract the process image from ELF
//* Input Parameters    : elf ptr
//* Output Parameters   : func ptr
//* Functions called    : none
//*----------------------------------------------------------------------------
uchar ucSednaElfProcessElf(uchar *ucProcessImage)
{
	Elf32_Ehdr 	*elf_head;
	uchar 		*ucProcessImageBackUp;
	
	/* Offcet and size of the 'ER_RO' section in ELF, added virt addr */
	uint 		uiFileOffcet1 = 0x00,uiFuncSize1 = 0x00,uiVirtualAddr1 = 0x00;
	
	/* Offcet and size of the 'ER_RW' section in ELF */
	uint 		uiFileOffcet2 = 0x00,uiFuncSize2 = 0x00,uiVirtualAddr2 = 0x00;
	
	/* Offcet and size of the 'ER_ZI' section in ELF */
	uint 		uiFileOffcet3 = 0x00,uiFuncSize3 = 0x00,uiVirtualAddr3 = 0x00;
	
	/* Temp buffer for 'ER_R0' section */
	uchar 		*ucProgramSectionBuffer = NULL;
	
	/* Temp buffer for 'ER_RW' section */
	uchar 		*ucIDataSectionBuffer = NULL;
	
	/* Temp buffer for 'ER_ZI' section */
	uchar 		*ucUDataSectionBuffer = NULL;
	
	/* Test for valid ELF file */
	if(!iSednaElfIsValidElf(ucProcessImage)) 
	  return SEDNA_APP_ELF_NOT_VALID;
   
   	/* Fill the header structure */
    elf_head = (Elf32_Ehdr *)ucProcessImage;
    byte_order = elf_head->e_ident[EI_DATA];

	/* Load the string table pointed out by the segment header */
    if(!iSednaElfLoadNames(elf_head, ucProcessImage))
      return SEDNA_APP_ELF_NO_SYMBOL_TABLE;

	/* Extract Section Info */
	vSednaElfParceSections(	elf_head, ucProcessImage, &uiFileOffcet1, &uiFuncSize1, &uiVirtualAddr1, "ER_RO");
	vSednaElfParceSections(	elf_head, ucProcessImage, &uiFileOffcet2, &uiFuncSize2, &uiVirtualAddr2, "ER_RW");
	vSednaElfParceSections(	elf_head, ucProcessImage, &uiFileOffcet3, &uiFuncSize3, &uiVirtualAddr3, "ER_ZI");
							
	/* Check if program section is present in ELF */
	if(uiFuncSize1 == NULL)
	  return SEDNA_APP_ELF_PRG_SECT_MISSING;

	/* Check if the sum of both data section sizes is over the
	   limit of 0xFFFF */
	if((uiFuncSize2 + uiFuncSize3) > 0xFFFF)
	  return SEDNA_APP_ELF_DATA_SECTION_TOO_BIG;
   
	/* Stop the kernel */		
	//vTaskSuspendAll();
	
	//DebugPrintValue("uiFuncSize1",uiFuncSize1);					
									
	/* Allocate temp program section buffer - ER_R0 */
	ucProgramSectionBuffer = (uchar *) pvPortMalloc(uiFuncSize1);
	
	/* If the 'ER_RW' section is present allocate buffer for it */
	if(uiFuncSize2 != NULL)
   	  ucIDataSectionBuffer = (uchar *) pvPortMalloc(uiFuncSize2);
							
    /* If the 'ER_ZI' section is present allocate buffer for it */
	if(uiFuncSize3 != NULL)
   	  ucUDataSectionBuffer = (uchar *) pvPortMalloc(uiFuncSize3);
																	
	/* Restart the kernel */
   	//cTaskResumeAll();		
   	
	/* Copy Program Data Section */
	if(ucProgramSectionBuffer != NULL)
		vElfMemCopy(ucProgramSectionBuffer,(ucProcessImage + uiFileOffcet1),uiFuncSize1);
	else
		return SEDNA_APP_ELF_PRG_SECT_ALLOC_FAIL;
	
	/* Copy IData Section */
	if(uiFuncSize2 != NULL)
	{
		if(ucIDataSectionBuffer == NULL)
			return SEDNA_APP_ELF_IDA_SECT_ALLOC_FAIL;
	
		vElfMemCopy(ucIDataSectionBuffer,(ucProcessImage + uiFileOffcet2),uiFuncSize2);		
	}
		
	/* Fill UData Section with zeros, basically we create the
	   unitilized data section here */
	if(uiFuncSize3 != NULL)
	{	
		if(ucUDataSectionBuffer == NULL)
			return SEDNA_APP_ELF_UDA_SECT_ALLOC_FAIL;

		vElfMemSet(ucUDataSectionBuffer,0x00,uiFuncSize3);			
	}
		
	/* ---------------    Copy extracted process image ---------------*/
	
	/* Program section */
	vElfMemCopy(ucProcessImage,ucProgramSectionBuffer,uiFuncSize1);
	
	/* Save ptr */
	ucProcessImageBackUp = ucProcessImage;
	
	/* Shift ptr to start of next section */
	ucProcessImage += uiFuncSize1;

	/* Initilized Data section */
	if(uiFuncSize2 != NULL)
	{
		vElfMemCopy(ucProcessImage,ucIDataSectionBuffer,uiFuncSize2);
		
		/* Shift ptr to start of next section */
		ucProcessImage += uiFuncSize2;
	}
		
	/* Unitilized Data section */	
	if(uiFuncSize3 != NULL)
		vElfMemCopy(ucProcessImage,ucUDataSectionBuffer,uiFuncSize3);
			
	/* ---------------- Copy extracted process image end -------------*/
	
	/* Parse Program Data section for offcets and patch them */
	vSednaElfParceOffcets(ucProcessImageBackUp,uiFuncSize1,uiVirtualAddr1);
					
	/* Free program section buffer */
	vPortFree( ucProgramSectionBuffer );
   	
   	/* Free idata section buffer */
   	if(uiFuncSize2 != NULL)
   		vPortFree( ucIDataSectionBuffer );
	  
	/* Free udata section buffer */  
	if(uiFuncSize3 != NULL)
		vPortFree( ucUDataSectionBuffer );
	  	
	return SEDNA_APP_LOAD_SUCCESS;
}

//*----------------------------------------------------------------------------
//* Function Name       : iSednaElfIsValidElf
//* Object              : Check for start bytes
//* Input Parameters    : none
//* Output Parameters   : none
//* Functions called    : none
//*----------------------------------------------------------------------------
static int iSednaElfIsValidElf(uchar *ucImagePtr)
{   
	int res;
	
	//vTaskSuspendAll();
	
    if ((*ucImagePtr == 0x7F) && (strncmp((char *)(ucImagePtr + 1), "ELF", 3) == 0))
      res = 1;
    else
      res = 0;
    
    //cTaskResumeAll();
    
	return res;
}

//*----------------------------------------------------------------------------
//* Function Name       : iSednaElfLoadNames
//* Object              : Load names
//* Input Parameters    : none
//* Output Parameters   : none
//* Functions called    : none
//*----------------------------------------------------------------------------
static int iSednaElfLoadNames(Elf32_Ehdr *header, uchar *ucImagePtr)
{
    Elf32_Shdr *ptr;
    int i;
    
    /* load the section names string table */
    ptr = (Elf32_Shdr *)(ucImagePtr + ewSednaElfEwordSwap(header->e_shoff));
    
    for (i=0;i < ehSednaElfEhalfSwap(header->e_shstrndx); i++)
      ptr++;
    name_table = (char *)(ucImagePtr + ewSednaElfEwordSwap(ptr->sh_offset));

    /* load the general string table */
    ptr = (Elf32_Shdr *)(ucImagePtr + ewSednaElfEwordSwap(header->e_shoff));
    
    //vTaskSuspendAll();
    
    for (i=0; i < (int)ehSednaElfEhalfSwap(header->e_shnum); i++) 
    {        	
		if (strncmp((name_table + ewSednaElfEwordSwap(ptr->sh_name)),".strtab", 7) == 0) 
		{
	    	sym_name_table = (char *)(ucImagePtr + ewSednaElfEwordSwap(ptr->sh_offset));
	    	break;
		} 
		else 
	    	ptr++;			    		    	
    }
    
    //cTaskResumeAll();
    
    //if (sym_name_table == NULL) {
	//	//printf("\nNo symbol table defined!\n\n");
	//	return 0;
    //}
    
    return 1;
}

//*----------------------------------------------------------------------------
//* Function Name       : vSednaElfParceSections
//* Object              : Parse ELF sections
//* Input Parameters    : none
//* Output Parameters   : none
//* Functions called    : none
//*----------------------------------------------------------------------------
static void vSednaElfParceSections(Elf32_Ehdr *header, uchar *image, uint *offcet, uint *size,uint *virt_addr,const char *cSectionName)
{
    Elf32_Off  sec_offset;	       /* section table offset  			*/
    Elf32_Half sec_size;      	   /* size of section table 			*/
    Elf32_Half num_secs;	       /* num. elements in section table 	*/
    int i,iResult;
    Elf32_Shdr *sec_table, *ptr;   /* section table buffer */
    
    sec_offset = ewSednaElfEwordSwap(header->e_shoff);
    sec_size   = ehSednaElfEhalfSwap(header->e_shentsize) * ehSednaElfEhalfSwap(header->e_shnum);
    num_secs   = ehSednaElfEhalfSwap(header->e_shnum);
      
    /* Point to the section table in the ELF image */
    sec_table = (Elf32_Shdr *)(image + sec_offset);
    
    /* ptr is an increment pointer here.. */            
    ptr = (Elf32_Shdr *)sec_table;      
    
	/* Process sections, one at a time */
    for (i=0;i<num_secs;i++) 
	{ 		
		/* Extract program data section params */
		//vTaskSuspendAll();
		iResult = strncmp((char *)(name_table + ewSednaElfEwordSwap(ptr->sh_name)), cSectionName, 5);
		//cTaskResumeAll();
		
		if(iResult == 0)
		{
			*virt_addr 	= ptr->sh_addr;
			*offcet 	= ewSednaElfEwordSwap(ptr->sh_offset);
			*size   	= ewSednaElfEwordSwap(ptr->sh_size);
			break;
		}							
		ptr++;
    }    
}

//*----------------------------------------------------------------------------
//* Function Name       : vSednaElfParceOffcets
//* Object              : Parse offcets
//* Input Parameters    : none
//* Output Parameters   : none
//* Functions called    : none
//*----------------------------------------------------------------------------
static void vSednaElfParceOffcets(uchar *ucProgramSection,uint uiProgramSectionSize,uint uiR0VirtAddr)
{
	uint 	uiPDScount;
	ulong	ulCurrentInstruction,ulNewOffcet;	//ulDataSectionPtr

	/* Calculate Data Section Address from end of the Program Data Section */
	//ulDataSectionPtr = (ulong)ucProgramSection + uiProgramSectionSize;
	
	/* Shift to the lower 16 bits */
	uiR0VirtAddr = (uiR0VirtAddr >> 16) & 0xFFFF;
	
	/* Parse the whole section, as its 4 bytes alligned
	   we can check every 4 bytes as next instruction,
	   probably different code for Thumb !!! The alligment
	   of the section could be checked in the ELF stucture! */	
	for(uiPDScount=0;uiPDScount<(uiProgramSectionSize/4);uiPDScount++)
	{	
		/* Load current instruction value */
		ulCurrentInstruction  = *(ucProgramSection +     (uiPDScount * 4)) << 24;
		ulCurrentInstruction |= *(ucProgramSection + 1 + (uiPDScount * 4)) << 16;
		ulCurrentInstruction |= *(ucProgramSection + 2 + (uiPDScount * 4)) <<  8;
		ulCurrentInstruction |= *(ucProgramSection + 3 + (uiPDScount * 4));

		/* Search for offcet */
		if((ulCurrentInstruction & 0xFFFF) == uiR0VirtAddr)		   
		{	
			/* Get only the high part of the old offcet (the one that shows the
			   shift in the data section) 
			   
			   Example:
			   
			   ER_RO:00001028 	C6 03 44 44 	off_1028        DCD unk_444403C6       
			   (section)		(offcet value)  (name)			(ptr)
			   
			   0x44440000 is the virtual address of the data section
			   0x000003C6 is the actual shift in the section, being the first
			              or second data section doesnt matter, because the 
			              second section follows directly after the first one
			              
			   If the relocation is at address 0x3A00, then we subsitute this 
			   offcet by adding the real shift to the base address of the code
			   section + the size of the code section.
			   
			   Code section size 0x100, then base of the data section is 0x3B00
			   and our new offcet will be 0x00003EC6 ( C6 3E 00 00 in memory )           
			   
			*/
			ulCurrentInstruction &= 0xFFFF0000;
			
			
			/* Then we add the 16 bit shift in the data section */
			ulNewOffcet  = ((uchar)(ulCurrentInstruction >> 16)) & 0xFF;
			ulNewOffcet  = ulNewOffcet << 8;
			ulNewOffcet |= ((uchar)(ulCurrentInstruction >> 24)) & 0xFF;
			
			/* The base is the start of the Data section, we add it to 
			   the shift - old code with only the data segments placed
			   at virtual address */
			//ulNewOffcet += ulDataSectionPtr;	
			
			/* New implementation - the whole image is placed at virtual
			   address, we shift from the start of the R0 section */
			ulNewOffcet += (ulong)ucProgramSection;
									
			/* Patch to point to the correct Data Section 
			   we assume that both Data sections are located at
			   the end of the Program Data Section */
			*(ucProgramSection + 3 + (uiPDScount * 4)) = (uchar)(ulNewOffcet >> 24);
			*(ucProgramSection + 2 + (uiPDScount * 4)) = (uchar)(ulNewOffcet >> 16);
			*(ucProgramSection + 1 + (uiPDScount * 4)) = (uchar)(ulNewOffcet >>  8);
			*(ucProgramSection + 0 + (uiPDScount * 4)) = (uchar)(ulNewOffcet & 0xFF);		  
		}	
	}

}

//*----------------------------------------------------------------------------
//* Function Name       : ewSednaElfEwordSwap
//* Object              : Swap the byte-order for Elf32_Word items (ints) 
//* Input Parameters    : none
//* Output Parameters   : none
//* Functions called    : none
//*----------------------------------------------------------------------------
static Elf32_Word ewSednaElfEwordSwap(Elf32_Word word)
{
    unsigned char wa[4];
    
#ifdef BIG_ENDIAN
   if (byte_order != ELFDATA2MSB)
      return word;
#else
    if (byte_order != ELFDATA2LSB)
      return word;
#endif
    
    wa[0] = (word & 0xff000000) >> 24;
    wa[1] = (word & 0x00ff0000) >> 16;
    wa[2] = (word & 0x0000ff00) >> 8;
    wa[3] = (word & 0x000000ff);
    
    return *(Elf32_Word *)wa;
}

//*----------------------------------------------------------------------------
//* Function Name       : ehSednaElfEhalfSwap
//* Object              : Swap the byte-order for Elf32_Half items (ushorts)
//* Input Parameters    : none
//* Output Parameters   : none
//* Functions called    : none
//*----------------------------------------------------------------------------
static Elf32_Half ehSednaElfEhalfSwap(Elf32_Half half)
{
    unsigned char ha[2];
#ifdef BIG_ENDIAN
    if (byte_order != ELFDATA2MSB)
      return half;
#else
    if (byte_order != ELFDATA2LSB)
      return half;
#endif
    
    ha[0] = (half & 0xff00) >> 8;
    ha[1] = (half & 0x00ff);
    return *(Elf32_Half *)ha;
}

//*----------------------------------------------------------------------------
//* Function Name       : vElfMemCopy
//* Object              : Copy buffers
//* Input Parameters    : us delay
//* Output Parameters   : none
//* Functions called    : none
//*----------------------------------------------------------------------------
static void vElfMemCopy(uchar *pDestBuffer, uchar *pSourceBuffer,uint nCpySize)
{
ulong    i;

    for(i=0;i<nCpySize;i++)
	{
		*pDestBuffer++ = *pSourceBuffer++;
	}

}

//*----------------------------------------------------------------------------
//* Function Name       : vElfMemSet
//* Object              : Set buffer
//* Input Parameters    : 
//* Output Parameters   : none
//* Functions called    : none
//*----------------------------------------------------------------------------
static void vElfMemSet(uchar *pDestBuffer, uchar ucValue,uint nCpySize)
{
ulong    i;

    for(i=0;i<nCpySize;i++)
	{
		*pDestBuffer++ = ucValue;
	}

}
#endif
