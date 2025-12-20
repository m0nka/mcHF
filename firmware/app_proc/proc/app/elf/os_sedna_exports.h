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

#ifndef __OS_SEDNA_EXPORTS_H
#define __OS_SEDNA_EXPORTS_H

/* Count of API array elements */
#define SEDNA_EXPORTS_API_COUNT				19

#define SEDNA_APP_DESC_PRIORITY_SHIFT		0x08
#define SEDNA_APP_DESC_OS_VER_SHIFT			0x0C
#define SEDNA_APP_DESC_NAME_SHIFT			0x10

/* API calls structure decl */
typedef struct SEDNA_API_EXPORTS 
{   
   void *func_address; 
                                             
} SEDNA_API_EXPORTS;

typedef struct MENU_DEF 
{   
   const char 	*menu_text;     
   const void  	*handler_ptr;            
                                           
} MENU_DEF, *PMENU_DEF;

ulong 	ulSednaExportsBaseAddress(void);

//void 	vSednaExportsDrawScriptUI(void);
//void 	vSednaExportsPrintOnScreen(char *text,int x,int y,int foreground,int background);

#endif
