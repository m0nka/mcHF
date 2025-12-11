
#include "mchf_pro_board.h"

#include "gui.h"
#include "dialog.h"

#include "filebrowser_app.h"
#include <string.h>

#include "ff_gen_drv.h"

DIR dir;
static char         		acAttrib[10];
static char         		acExt[FILEMGR_MAX_EXT_SIZE];

static struct {
  U32 Mask;
  char c;
} _aAttrib[] = {
  { AM_RDO, 'R' },
  { AM_HID, 'H' },
  { AM_SYS, 'S' },
  { AM_DIR, 'D' },
  { AM_ARC, 'A' },
};

/**
  * @brief  Return file extension and removed from file name.
  * @param  pFile: pointer to the file name.
  * @param  pExt:  pointer to the file extension
  * @retval None
  */
static void GetExt(char * pFile, char * pExt)
{
  int Len;
  int i;
  int j;

  /* Search beginning of extension */
  Len = strlen(pFile);
  for (i = Len; i > 0; i--) {
    if (*(pFile + i) == '.') {
      *(pFile + i) = '\0';     /* Cut extension from filename */
      break;
    }
  }

  /* Copy extension */
  j = 0;
  while (*(pFile + ++i) != '\0') {
    *(pExt + j) = *(pFile + i);
    j++;
  }
  *(pExt + j) = '\0';          /* Set end of string */
}

/**
  * @brief  Return the extension Only
  * @param  pFile: pointer to the file name.
  * @param  pExt:  pointer to the file extension
  * @retval None
  */
void k_GetExtOnly(char * pFile, char * pExt)
{
  int Len;
  int i;
  int j;

  /* Search beginning of extension */
  Len = strlen(pFile);
  for (i = Len; i > 0; i--) {
    if (*(pFile + i) == '.') {
      break;
    }
  }

  /* Copy extension */
  j = 0;
  while (*(pFile + ++i) != '\0') {
    *(pExt + j) = *(pFile + i);
    j++;
  }
  *(pExt + j) = '\0';          /* Set end of string */
}

/**
  * @brief  This function is responsible to pass information about the requested file
  * @param  pInfo: Pointer to structure which contains all details of the requested file.
  * @retval None
  */
int k_GetData(CHOOSEFILE_INFO * pInfo)
{
  char                c;
  int                 i = 0;
  char               tmp[CHOOSEFILE_MAXLEN];
  static char fn[CHOOSEFILE_MAXLEN];

  FRESULT res = FR_INT_ERR;

  FILINFO fno;

  switch (pInfo->Cmd)
  {
  case CHOOSEFILE_FINDFIRST:
    f_closedir(&dir);

    /* reformat path */
    memset(tmp, 0, CHOOSEFILE_MAXLEN);
    strcpy(tmp, pInfo->pRoot);

    for(i= CHOOSEFILE_MAXLEN; i > 0; i--)
    {
      if(tmp[i] == '/')
      {
        tmp[i] = 0;
        break;
      }
    }

    res = f_opendir(&dir, tmp);

    if (res == FR_OK)
    {

      res = f_readdir(&dir, &fno);
    }
    break;

  case CHOOSEFILE_FINDNEXT:
    res = f_readdir(&dir, &fno);
    break;
  }

  if (res == FR_OK)
  {
    strcpy(fn, fno.fname);

    while (((fno.fattrib & AM_DIR) == 0) && (res == FR_OK))
    {

      if((strstr(pInfo->pMask, ".img")))
      {
        if((strstr(fn, ".bmp")) || (strstr(fn, ".jpg")) || (strstr(fn, ".BMP")) || (strstr(fn, ".JPG")))
        {
          break;
        }
        else
        {
          res = f_readdir(&dir, &fno);

          if (res != FR_OK || fno.fname[0] == 0)
          {
            f_closedir(&dir);
            return 1;
          }
          else
          {
            strcpy(fn, fno.fname);
          }
        }

      }
      else if((strstr(pInfo->pMask, ".audio")))
      {
        if((strstr(fn, ".wav")) || (strstr(fn, ".WAV")))
        {
          break;
        }
        else
        {
          res = f_readdir(&dir, &fno);

          if (res != FR_OK || fno.fname[0] == 0)
          {
            f_closedir(&dir);
            return 1;
          }
          else
          {
            strcpy(fn, fno.fname);
          }
        }

      }

      else if((strstr(pInfo->pMask, ".video")))
      {
        if((strstr(fn, ".emf")) || (strstr(fn, ".EMF")))
        {
          break;
        }
        else
        {
          res = f_readdir(&dir, &fno);

          if (res != FR_OK || fno.fname[0] == 0)
          {
            f_closedir(&dir);
            return 1;
          }
          else
          {
            strcpy(fn, fno.fname);
          }
        }

      }
      else if(strstr(fn, pInfo->pMask) == NULL)
      {

        res = f_readdir(&dir, &fno);

        if (res != FR_OK || fno.fname[0] == 0)
        {
          f_closedir(&dir);
          return 1;
        }
        else
        {
          strcpy(fn, fno.fname);
        }
      }
      else
      {
        break;
      }
    }

    if(fn[0] == 0)
    {
      f_closedir(&dir);
      return 1;
    }

    pInfo->Flags = ((fno.fattrib & AM_DIR) == AM_DIR) ? CHOOSEFILE_FLAG_DIRECTORY : 0;

    for (i = 0; i < GUI_COUNTOF(_aAttrib); i++)
    {
      if (fno.fattrib & _aAttrib[i].Mask)
      {
        c = _aAttrib[i].c;
      }
      else
      {
        c = '-';
      }
      acAttrib[i] = c;
    }
    if((fno.fattrib & AM_DIR) == AM_DIR)
    {
      acExt[0] = 0;
    }
    else
    {
      GetExt(fn, acExt);
    }
    pInfo->pAttrib = acAttrib;
    pInfo->pName = fn;
    pInfo->pExt = acExt;
    pInfo->SizeL = fno.fsize;
    pInfo->SizeH = 0;

  }
  return res;
}

/**
  * @brief  Copy disk content in the explorer list
  * @param  path: pointer to root path
  * @param  list: pointer to file list
  * @retval Status
  */
uchar  FILEMGR_ParseDisks (char *path, FILELIST_FileTypeDef *list)
{
  FRESULT res;
  FILINFO fno;
  DIR dir;
  char *fn;
  
  res = f_opendir(&dir, path);
  list->ptr = 0;
  
  if (res == FR_OK)
  {
    
    while (1)
    {
      res = f_readdir(&dir, &fno);
      
      if (res != FR_OK || fno.fname[0] == 0)
      {
        break;
      }

      //printf("next file: %s, attrib: 0x%02x\n\r",fno.fname,fno.fattrib);

      if (fno.fname[0] == '.')
      {
        continue;
      }

      // Do not show deleted files
      if ((fno.fattrib & AM_HID) == AM_HID)
    	  continue;

      fn = fno.fname;

      if (list->ptr < FILEMGR_LIST_DEPDTH)
      {
        if ((fno.fattrib & AM_DIR) == AM_DIR)
        {
          strncpy((char *)list->file[list->ptr].name, (char *)fn, FILEMGR_FILE_NAME_SIZE);
          list->file[list->ptr].type = FILETYPE_DIR;
          list->ptr++;
        }
        else
        {
          strncpy((char *)list->file[list->ptr].name, (char *)fn, FILEMGR_FILE_NAME_SIZE);
          list->file[list->ptr].type = FILETYPE_FILE;
          list->ptr++;
        }
      }   
    }
  }
  f_closedir(&dir);
  return res;
}

/**
  * @brief  Retrieve the parent directory from full file path
  * @param  dir: pointer to parent directory
  * @retval None
*/
void FILEMGR_GetParentDir (char *dir)
{
  uint16_t idx = FILEMGR_FILE_NAME_SIZE;
  
  for ( ; idx > 0; idx --)
  {
    
    if (dir [idx] == '/')
    {
      dir [idx + 1] = 0;
      break;
    }
    dir [idx + 1] = 0;
  }
}


/**
  * @brief  Retrieve the file name from a full file path
  * @param  file: pointer to base path
  * @param  path: pointer to full path
  * @retval None
*/
void FILEMGR_GetFileOnly (char *file, char *path)
{
  char *baseName1, *baseName2;
  baseName1 = strrchr(path,'/');
  baseName2 = strrchr(path,':');
  
  if(baseName1++) 
  { 
    strcpy(file, baseName1);
  }
  else 
  {
    if (baseName2++) 
    {
      
      strcpy(file, baseName2);
    }
    else
    {
      strcpy(file, path);
    }
  }
}
