package com.cowlark.fluxengine.arch.ibm;

/**
 * Constants for the IBM format (i.e. ordinary PC floppies), ported from
 * arch/ibm/ibm.h.
 */
public final class Ibm
{
    public static final int IBM_MFM_SYNC = 0xA1; /* sync byte for MFM */
    public static final int IBM_IAM = 0xFC;      /* start-of-track record */
    public static final int IBM_IAM_LEN = 1;     /* plus prologue */
    public static final int IBM_IDAM = 0xFE;     /* sector header */
    public static final int IBM_IDAM_LEN = 7;    /* plus prologue */
    public static final int IBM_DAM1 = 0xF8;     /* sector data (type 1) */
    public static final int IBM_DAM2 = 0xFB;     /* sector data (type 2) */
    public static final int IBM_TRS80DAM1 = 0xF9; /* sector data (TRS-80 directory) */
    public static final int IBM_TRS80DAM2 = 0xFA; /* sector data (TRS-80 directory) */
    public static final int IBM_DAM_LEN = 1;     /* plus prologue and user data */

    private Ibm()
    {
    }
}