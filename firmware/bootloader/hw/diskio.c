/************************************************************************************
**  diskio.c — FatFs disk I/O dispatcher (ff16-compatible)                        **
**  Based on STMicroelectronics template, adapted for FatFs R0.16                 **
************************************************************************************/
#include "ff.h"
#include "diskio.h"
#include "ff_gen_drv.h"

extern Disk_drvTypeDef disk;

DSTATUS disk_status(BYTE pdrv)
{
  return disk.drv[pdrv]->disk_status(disk.lun[pdrv]);
}

DSTATUS disk_initialize(BYTE pdrv)
{
  DSTATUS stat = RES_OK;

  if(disk.is_initialized[pdrv] == 0)
  {
    stat = disk.drv[pdrv]->disk_initialize(disk.lun[pdrv]);
    if(stat == RES_OK)
    {
      disk.is_initialized[pdrv] = 1;
    }
  }
  return stat;
}

DRESULT disk_read(BYTE pdrv, BYTE *buff, LBA_t sector, UINT count)
{
  return disk.drv[pdrv]->disk_read(disk.lun[pdrv], buff, (DWORD)sector, count);
}

#if FF_FS_READONLY == 0
DRESULT disk_write(BYTE pdrv, const BYTE *buff, LBA_t sector, UINT count)
{
  return disk.drv[pdrv]->disk_write(disk.lun[pdrv], buff, (DWORD)sector, count);
}
#endif

DRESULT disk_ioctl(BYTE pdrv, BYTE cmd, void *buff)
{
  return disk.drv[pdrv]->disk_ioctl(disk.lun[pdrv], cmd, buff);
}

__attribute__((weak)) DWORD get_fattime(void)
{
  return 0;
}

