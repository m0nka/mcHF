/************************************************************************************
**                                                                                 **
**                                 mcHF QRP Transceiver                            **
**                         Krassi Atanassov - M0NKA, 2013-2026                     **
**                                                                                 **
**---------------------------------------------------------------------------------**
**                                                                                 **
**  File name:       usbh_diskio.c                                                **
**  Description:     FatFS disk I/O driver for USB Host MSC                        **
**  Last Modified:                                                                 **
**  Licence:         https://github.com/m0nka/mcHF/blob/main/LICENSE              **
************************************************************************************/
#include "mchf_pro_board.h"

#include "ff_gen_drv.h"
#include "usbh_diskio.h"
#include "usbh_msc.h"

// -----------------------------------------------------------------------
// Extern — the USB host handle lives in usb_host.c
// -----------------------------------------------------------------------
extern USBH_HandleTypeDef hUSBHost;

// -----------------------------------------------------------------------
// Scratch buffer for unaligned reads (no DMA, so only alignment matters)
// -----------------------------------------------------------------------
static DWORD scratch[_MAX_SS / 4];

// -----------------------------------------------------------------------
// FatFS disk I/O interface
// -----------------------------------------------------------------------

static DSTATUS usb_initialize(BYTE lun)
{
    // USB host library handles all init
    return RES_OK;
}

static DSTATUS usb_status(BYTE lun)
{
    if(USBH_MSC_UnitIsReady(&hUSBHost, lun))
        return RES_OK;

    return STA_NOINIT;
}

static DRESULT usb_read(BYTE lun, BYTE *buff, DWORD sector, UINT count)
{
    USBH_StatusTypeDef status;

    // Handle unaligned buffer
    if((DWORD)buff & 3)
    {
        while(count--)
        {
            status = USBH_MSC_Read(&hUSBHost, lun, sector + count,
                                   (uint8_t *)scratch, 1);
            if(status != USBH_OK)
                goto read_error;

            memcpy(&buff[count * _MAX_SS], scratch, _MAX_SS);
        }
    }
    else
    {
        status = USBH_MSC_Read(&hUSBHost, lun, sector, buff, count);
        if(status != USBH_OK)
            goto read_error;
    }

    return RES_OK;

read_error:
    {
        MSC_LUNTypeDef info;
        USBH_MSC_GetLUNInfo(&hUSBHost, lun, &info);

        switch(info.sense.asc)
        {
            case SCSI_ASC_LOGICAL_UNIT_NOT_READY:
            case SCSI_ASC_MEDIUM_NOT_PRESENT:
            case SCSI_ASC_NOT_READY_TO_READY_CHANGE:
                return RES_NOTRDY;

            default:
                return RES_ERROR;
        }
    }
}

#if _USE_WRITE == 1
static DRESULT usb_write(BYTE lun, const BYTE *buff, DWORD sector, UINT count)
{
    USBH_StatusTypeDef status;

    // Handle unaligned buffer
    if((DWORD)buff & 3)
    {
        while(count--)
        {
            memcpy(scratch, &buff[count * _MAX_SS], _MAX_SS);

            status = USBH_MSC_Write(&hUSBHost, lun, sector + count,
                                    (BYTE *)scratch, 1);
            if(status != USBH_OK)
                goto write_error;
        }
    }
    else
    {
        status = USBH_MSC_Write(&hUSBHost, lun, sector,
                                (BYTE *)buff, count);
        if(status != USBH_OK)
            goto write_error;
    }

    return RES_OK;

write_error:
    {
        MSC_LUNTypeDef info;
        USBH_MSC_GetLUNInfo(&hUSBHost, lun, &info);

        switch(info.sense.asc)
        {
            case SCSI_ASC_WRITE_PROTECTED:
                return RES_WRPRT;

            case SCSI_ASC_LOGICAL_UNIT_NOT_READY:
            case SCSI_ASC_MEDIUM_NOT_PRESENT:
            case SCSI_ASC_NOT_READY_TO_READY_CHANGE:
                return RES_NOTRDY;

            default:
                return RES_ERROR;
        }
    }
}
#endif

#if _USE_IOCTL == 1
static DRESULT usb_ioctl(BYTE lun, BYTE cmd, void *buff)
{
    MSC_LUNTypeDef info;

    switch(cmd)
    {
        case CTRL_SYNC:
            return RES_OK;

        case GET_SECTOR_COUNT:
            if(USBH_MSC_GetLUNInfo(&hUSBHost, lun, &info) == USBH_OK)
            {
                *(DWORD *)buff = info.capacity.block_nbr;
                return RES_OK;
            }
            break;

        case GET_SECTOR_SIZE:
            if(USBH_MSC_GetLUNInfo(&hUSBHost, lun, &info) == USBH_OK)
            {
                *(DWORD *)buff = info.capacity.block_size;
                return RES_OK;
            }
            break;

        case GET_BLOCK_SIZE:
            if(USBH_MSC_GetLUNInfo(&hUSBHost, lun, &info) == USBH_OK)
            {
                *(DWORD *)buff = info.capacity.block_size / 512;
                return RES_OK;
            }
            break;

        default:
            return RES_PARERR;
    }

    return RES_ERROR;
}
#endif

// -----------------------------------------------------------------------
// FatFS driver structure — registered via FATFS_LinkDriver()
// -----------------------------------------------------------------------
const Diskio_drvTypeDef USBH_Driver =
{
    usb_initialize,
    usb_status,
    usb_read,
#if _USE_WRITE == 1
    usb_write,
#endif
#if _USE_IOCTL == 1
    usb_ioctl,
#endif
};
