package com.cowlark.fluxengine.external;

/**
 * Constants and structures for the SCP flux file format, ported from
 * lib/external/scp.h.
 */
public final class Scp
{
    public static final int SCP_FLAG_INDEXED = (1 << 0);
    public static final int SCP_FLAG_96TPI = (1 << 1);
    public static final int SCP_FLAG_360RPM = (1 << 2);
    public static final int SCP_FLAG_NORMALIZED = (1 << 3);
    public static final int SCP_FLAG_READWRITE = (1 << 4);
    public static final int SCP_FLAG_FOOTER = (1 << 5);

    /* Size of the file header, including the 168 track offsets. */
    public static final int SCP_HEADER_SIZE = 16 + 168 * 4;

    /* Size of a track header (the 'TRK' id plus 5 revolution records). */
    public static final int SCP_TRACK_SIZE = 4 + 5 * 12;

    public static int trackno(int strack)
    {
        return strack >> 1;
    }

    public static int headno(int strack)
    {
        return strack & 1;
    }

    public static int strackno(int track, int side)
    {
        return (track << 1) | side;
    }

    private Scp()
    {
    }
}
