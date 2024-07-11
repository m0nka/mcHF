
#ifndef __UI_MENU_MODULE_H
#define __UI_MENU_MODULE_H

#include "gui.h"
#include "dialog.h"
   
typedef struct
{
 uchar     id;
 const char  *name;
 //GUI_CONST_STORAGE GUI_BITMAP  ** open_icon;
 GUI_CONST_STORAGE GUI_BITMAP  *icon;
 //GUI_CONST_STORAGE GUI_BITMAP  ** close_icon;
 void        (*startup) (WM_HWIN , uint16_t, uint16_t );
 void        (*DirectOpen) (char * ); 
 void        (*kill) (void );
}
K_ModuleItem_Typedef;

typedef struct
{
  const K_ModuleItem_Typedef   *module;  
  uchar  in_use;
  uchar  win_state;
}
K_ModulePropertyTypedef;

typedef struct
{
  char   ext[4];
  const K_ModuleItem_Typedef   *module;
}
K_ModuleOpenTypedef;

typedef void K_GET_DIRECT_OPEN_FUNC(char *);

/* Structure for menu items */
typedef struct {
  char * sText;
  U16 Id;
  U16 Flags;
} MENU_ITEM;

extern K_ModulePropertyTypedef    module_prop[];

void    k_ModuleInit(void);
void    k_UpdateLog(char *Msg);
uint8_t k_ModuleAdd(K_ModuleItem_Typedef *module);
uint8_t k_ModuleGetIndex(K_ModuleItem_Typedef *module);
void    k_ModuleRemove(K_ModuleItem_Typedef *module); 
uint8_t k_ModuleGetNumber(void);
uint8_t k_ModuleOpenLink(K_ModuleItem_Typedef *module, char *ext);

K_GET_DIRECT_OPEN_FUNC *k_ModuleCheckLink(char *ext);

#endif


