/*-----------------------------------------------------------------------/
/  Low level disk interface modlue include file   (C)ChaN, 2025          /
/-----------------------------------------------------------------------*/

package org.elm_chan.ff;

/* Status of Disk Functions */
 /* Results of Disk Functions - see DResult enum */

 /**
 * Low level disk interface, translated from diskio.h.
 * Backed by a single injected {@code DiskIo} instance per {@link FatFs}
 * (the C {@code BYTE pdrv} parameter is eliminated — see plan).
 */
public interface DiskIo {

    /* Disk Status Bits (DSTATUS) */

    int STA_NOINIT = 0x01 /* Drive not initialized */;
    int STA_NODISK = 0x02 /* No medium in the drive */;
    int STA_PROTECT = 0x04 /* Write protected */;

    /* Command code for disk_ioctl fucntion */

    /* Generic command (Used by FatFs) */
    int CTRL_SYNC = 0 /* Complete pending write process (needed at FF_FS_READONLY == 0) */;
    int GET_SECTOR_COUNT = 1 /* Get media size (needed at FF_USE_MKFS == 1) */;
    int GET_SECTOR_SIZE = 2 /* Get sector size (needed at FF_MAX_SS != FF_MIN_SS) */;
    int GET_BLOCK_SIZE = 3 /* Get erase block size (needed at FF_USE_MKFS == 1) */;
    int CTRL_TRIM = 4 /* Inform device that the data on the block of sectors is no longer used (needed at FF_USE_TRIM == 1) */;

    /* Generic command (Not used by FatFs) */
    int CTRL_POWER = 5 /* Get/Set power status */;
    int CTRL_LOCK = 6 /* Lock/Unlock media removal */;
    int CTRL_EJECT = 7 /* Eject media */;
    int CTRL_FORMAT = 8 /* Create physical format on the media */;

    /* MMC/SDC specific ioctl command (Not used by FatFs) */
    int MMC_GET_TYPE = 10 /* Get card type */;
    int MMC_GET_CSD = 11 /* Get CSD */;
    int MMC_GET_CID = 12 /* Get CID */;
    int MMC_GET_OCR = 13 /* Get OCR */;
    int MMC_GET_SDSTAT = 14 /* Get SD status */;
    int ISDIO_READ = 55 /* Read data form SD iSDIO register */;
    int ISDIO_WRITE = 56 /* Write data to SD iSDIO register */;
    int ISDIO_MRITE = 57 /* Masked write data to SD iSDIO register */;

    /* ATA/CF specific ioctl command (Not used by FatFs) */
    int ATA_GET_REV = 20 /* Get F/W revision */;
    int ATA_GET_MODEL = 21 /* Get model name */;
    int ATA_GET_SN = 22 /* Get serial number */;

    /*---------------------------------------*/
    /* Prototypes for disk control functions */

    /**
     * Corresponds to disk_initialize() - see diskio.h: DSTATUS disk_initialize (BYTE pdrv);
     */
    int diskInitialize();

    /**
     * Corresponds to disk_status() - see diskio.h: DSTATUS disk_status (BYTE pdrv);
     */
    int diskStatus();

    /**
     * Corresponds to disk_read(pdrv, buff, sector, count) - see diskio.h.
     * The buffer length must be exactly {@code count * 512} (FF_MIN_SS == FF_MAX_SS == 512).
     */
    DResult diskRead(long sector, byte[] buff, int count);

    /**
     * Corresponds to disk_write(pdrv, buff, sector, count) - see diskio.h.
     */
    DResult diskWrite(long sector, byte[] buff, int count);

    /**
     * Corresponds to disk_ioctl(pdrv, cmd, buff) - see diskio.h.
     * Under the baked-in configuration only CTRL_SYNC is used by FatFs;
     * other commands may return RES_PARERR.
     */
    DResult diskIoctl(int cmd, Object buff);
}
