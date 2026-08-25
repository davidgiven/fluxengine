/*----------------------------------------------------------------------------/
/  FatFs - Generic FAT Filesystem module  R0.16                               /
/-----------------------------------------------------------------------------/
/
/ Copyright (C) 2025, ChaN, all right reserved.
/
/ FatFs module is an open source software. Redistribution and use of FatFs in
/ source and binary forms, with or without modification, are permitted provided
/ that the following condition is met:
/
/ 1. Redistributions of source code must retain the above copyright notice,
/    this condition and the following disclaimer.
/
/ This software is provided by the copyright holder and contributors "AS IS"
/ and any warranties related to this software are DISCLAIMED.
/ The copyright owner or contributors be NOT LIABLE for any damages caused
/ by use of this software.
/
/----------------------------------------------------------------------------*/

package org.elm_chan.ff;

/* File function return code (FRESULT) */

public enum FResult {
    FR_OK(0) /* (0) Succeeded */,
    FR_DISK_ERR(1) /* (1) A hard error occurred in the low level disk I/O layer */,
    FR_INT_ERR(2) /* (2) Assertion failed */,
    FR_NOT_READY(3) /* (3) The physical drive does not work */,
    FR_NO_FILE(4) /* (4) Could not find the file */,
    FR_NO_PATH(5) /* (5) Could not find the path */,
    FR_INVALID_NAME(6) /* (6) The path name format is invalid */,
    FR_DENIED(7) /* (7) Access denied due to a prohibited access or directory full */,
    FR_EXIST(8) /* (8) Access denied due to a prohibited access */,
    FR_INVALID_OBJECT(9) /* (9) The file/directory object is invalid */,
    FR_WRITE_PROTECTED(10) /* (10) The physical drive is write protected */,
    FR_INVALID_DRIVE(11) /* (11) The logical drive number is invalid */,
    FR_NOT_ENABLED(12) /* (12) The volume has no work area */,
    FR_NO_FILESYSTEM(13) /* (13) Could not find a valid FAT volume */,
    FR_MKFS_ABORTED(14) /* (14) The f_mkfs function aborted due to some problem */,
    FR_TIMEOUT(15) /* (15) Could not take control of the volume within defined period */,
    FR_LOCKED(16) /* (16) The operation is rejected according to the file sharing policy */,
    FR_NOT_ENOUGH_CORE(17) /* (17) LFN working buffer could not be allocated, given buffer size is insufficient or too deep path */,
    FR_TOO_MANY_OPEN_FILES(18) /* (18) Number of open files > FF_FS_LOCK */,
    FR_INVALID_PARAMETER(19) /* (19) Given parameter is invalid */;

    private final int value;

    FResult(int value) {
        this.value = value;
    }

    public int getValue() {
        return value;
    }
}
