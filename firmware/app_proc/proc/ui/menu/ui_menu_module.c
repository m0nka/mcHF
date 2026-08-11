
#include "mchf_pro_board.h"

#include "gui.h"
#include "dialog.h"
#include "ui_menu_module.h"   

#define MAX_MODULES_NUM                                                       15

K_ModulePropertyTypedef    module_prop [MAX_MODULES_NUM];
K_ModuleOpenTypedef        open_link   [MAX_MODULES_NUM];
uint16_t module_num = 0;
uint16_t openlink_num = 0;

void k_ModuleInit(void) 
{
  module_num = 0;
  openlink_num = 0;
  memset(module_prop, 0, sizeof(module_prop));
  memset(open_link, 0, sizeof(module_prop));  
}

uint8_t k_ModuleGetNumber(void) 
{
  return module_num;
}

uint8_t k_ModuleAdd(K_ModuleItem_Typedef *module) 
{
  module_prop[module_num].in_use = 0;
  module_prop[module_num].win_state = 0;
  module_prop[module_num].module = module;
  module_num++;
  return 0;
}


uint8_t k_ModuleGetIndex(K_ModuleItem_Typedef *module)
{
  uint8_t ModuleIdx;
  
  for (ModuleIdx = 0; ModuleIdx < module_num; ModuleIdx++)
  {
    if (module_prop[ModuleIdx].module->id == module->id)
    {
      return ModuleIdx;
    }
  }
  return 0;
}

uint8_t k_ModuleOpenLink(K_ModuleItem_Typedef *module, char *ext) 
{
  if(openlink_num < (MAX_MODULES_NUM - 1))
  {
    open_link[openlink_num].module = module;
    strcpy(open_link[openlink_num].ext, ext);
    openlink_num++;
    return 0;
   }
  else
  {
    return 1;
  }
}

K_GET_DIRECT_OPEN_FUNC *k_ModuleCheckLink(char *ext) 
{
  uint8_t i   = 0;
  
  for (i = 0; i < MAX_MODULES_NUM; i++)
  {
    if(strcmp(open_link[i].ext, ext) == 0)
    {
      return (K_GET_DIRECT_OPEN_FUNC *)(open_link[i].module->DirectOpen);
    }
  }
  return NULL;
}

void k_ModuleRemove(K_ModuleItem_Typedef *module) 
{
  uint8_t idx = 0;
  
  for (idx  = 0; idx < MAX_MODULES_NUM; idx ++)
  {
    if(module_prop[module_num].module->id == module->id)
    {
      module_prop[module_num].module = NULL;
      module_num--;
    }
  }
}
