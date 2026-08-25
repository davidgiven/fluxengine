/*-----------------------------------------------------------------------/
/  Low level disk interface modlue include file   (C)ChaN, 2025          /
/-----------------------------------------------------------------------*/

package org.elm_chan.ff;

/* Results of Disk Functions */
public enum DResult {
    RES_OK(0) /* 0: Successful */,
    RES_ERROR(1) /* 1: R/W Error */,
    RES_WRPRT(2) /* 2: Write Protected */,
    RES_NOTRDY(3) /* 3: Not Ready */,
    RES_PARERR(4) /* 4: Invalid Parameter */;

    private final int value;

    DResult(int value) {
        this.value = value;
    }

    public int getValue() {
        return value;
    }
}
