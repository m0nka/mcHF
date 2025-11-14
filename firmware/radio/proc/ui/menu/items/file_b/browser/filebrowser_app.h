
#ifndef __FILEBROWSER_APP_H
#define __FILEBROWSER_APP_H

#include "storage_proc.h"

void k_GetExtOnly(char * pFile, char * pExt);
int 	k_GetData(CHOOSEFILE_INFO * pInfo);

void     FILEMGR_GetParentDir (char *dir);
void     FILEMGR_GetFileOnly (char *file, char *path);
uchar    FILEMGR_ParseDisks (char *path, FILELIST_FileTypeDef *list);

#endif
