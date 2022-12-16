/*-----------------------------------------------------------------------*/
/* Low level disk I/O module for FatFs     (C)ChaN 2019, J. Herold 2022  */
/*-----------------------------------------------------------------------*/

#include "ff.h"
#include "diskio.h"

#include "flash_functions.h"
#include "msc_disk.h"

/* Definitions of physical drive number for each drive */
#define DEV_USB			0


/*-----------------------------------------------------------------------*/
/* Get Drive Status                                                      */
/*-----------------------------------------------------------------------*/

DSTATUS disk_status (
	BYTE pdrv
)
{
	if (pdrv != 0 || !disk_initialized) return STA_NOINIT;
	else return 0;	
}



/*-----------------------------------------------------------------------*/
/* Inidialize a Drive                                                    */
/*-----------------------------------------------------------------------*/

DSTATUS disk_initialize (
	BYTE pdrv
)
{
	if (pdrv != 0) return STA_NOINIT;
	if (!disk_initialized) Flash_Init();
	return 0;
}



/*-----------------------------------------------------------------------*/
/* Read Sector(s)                                                        */
/*-----------------------------------------------------------------------*/

DRESULT disk_read (
	BYTE pdrv,		/* Physical drive nmuber to identify the drive */
	BYTE *buff,		/* Data buffer to store read data */
	LBA_t sector,	/* Start sector in LBA */
	UINT count		/* Number of sectors to read */
)
{
	if (pdrv != 0) return RES_NOTRDY;
	
	for (int i = 0; i < count; i++)
	{
		Flash_ReadQueued(sector + i, 0, buff + i * DISK_SECTOR_SIZE, DISK_SECTOR_SIZE);
	}
	return RES_OK;
}



/*-----------------------------------------------------------------------*/
/* Write Sector(s)                                                       */
/*-----------------------------------------------------------------------*/

#if FF_FS_READONLY == 0

DRESULT disk_write (
	BYTE pdrv,			/* Physical drive nmuber to identify the drive */
	const BYTE *buff,	/* Data to be written */
	LBA_t sector,		/* Start sector in LBA */
	UINT count			/* Number of sectors to write */
)
{
	return RES_OK;
}

#endif


/*-----------------------------------------------------------------------*/
/* Miscellaneous Functions                                               */
/*-----------------------------------------------------------------------*/

DRESULT disk_ioctl (
	BYTE pdrv,		/* Physical drive nmuber (0..) */
	BYTE cmd,		/* Control code */
	void *buff		/* Buffer to send/receive control data */
)
{
	if (pdrv != 0) return RES_NOTRDY;

	LBA_t *sector_count = buff;
	WORD *sector_size = buff;
	DWORD *block_size = buff;

	switch (cmd)
	{
		case CTRL_SYNC:
			Flash_WriteCycle(true);
			break;
		
		case GET_SECTOR_COUNT:
			*sector_count = DISK_SECTOR_NUM;
			break;

		case GET_SECTOR_SIZE:
			*sector_size = DISK_SECTOR_SIZE;
			break;
		
		case GET_BLOCK_SIZE:
			*block_size = DISK_CLUSTER_SIZE;
			break;

		default:
			return RES_PARERR;
	}

	return RES_OK;
}

