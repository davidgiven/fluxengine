package com.cowlark.fluxengine.arch.brother;

/**
 * Constants for the Brother word processor format (or at least, one of them),
 * ported from arch/brother/brother.h.
 */
public final class Brother
{
    public static final int BROTHER_SECTOR_RECORD = 0xFFFFFD57;
    public static final int BROTHER_DATA_RECORD = 0xFFFFFDDB;
    public static final int BROTHER_DATA_RECORD_PAYLOAD = 256;
    public static final int BROTHER_DATA_RECORD_CHECKSUM = 3;
    public static final int BROTHER_DATA_RECORD_ENCODED_SIZE = 415;

    public static final int BROTHER_TRACKS_PER_240KB_DISK = 78;
    public static final int BROTHER_TRACKS_PER_120KB_DISK = 39;
    public static final int BROTHER_SECTORS_PER_TRACK = 12;

    private Brother()
    {
    }
}