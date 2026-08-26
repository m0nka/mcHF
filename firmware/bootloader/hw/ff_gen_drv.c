/************************************************************************************
**  ff_gen_drv.c — FatFs generic driver registration (ff16-compatible)            **
**  Based on STMicroelectronics template, adapted for FatFs R0.16                 **
************************************************************************************/
#include "ff_gen_drv.h"

Disk_drvTypeDef disk = {{0},{0},{0},0};

uint8_t FATFS_LinkDriverEx(const Diskio_drvTypeDef *drv, char *path, uint8_t lun)
{
  uint8_t ret = 1;
  uint8_t DiskNum = 0;

  if(disk.nbr < FF_VOLUMES)
  {
    disk.is_initialized[disk.nbr] = 0;
    disk.drv[disk.nbr] = drv;
    disk.lun[disk.nbr] = lun;
    DiskNum = disk.nbr++;
    path[0] = DiskNum + '0';
    path[1] = ':';
    path[2] = '/';
    path[3] = 0;
    ret = 0;
  }

  return ret;
}

uint8_t FATFS_LinkDriver(const Diskio_drvTypeDef *drv, char *path)
{
  return FATFS_LinkDriverEx(drv, path, 0);
}

uint8_t FATFS_UnLinkDriverEx(char *path, uint8_t lun)
{
  uint8_t DiskNum = 0;
  uint8_t ret = 1;

  if(disk.nbr >= 1)
  {
    DiskNum = path[0] - '0';
    if(disk.drv[DiskNum] != 0)
    {
      disk.drv[DiskNum] = 0;
      disk.lun[DiskNum] = 0;
      disk.nbr--;
      ret = 0;
    }
  }

  return ret;
}

uint8_t FATFS_UnLinkDriver(char *path)
{
  return FATFS_UnLinkDriverEx(path, 0);
}

uint8_t FATFS_GetAttachedDriversNbr(void)
{
  return disk.nbr;
}
