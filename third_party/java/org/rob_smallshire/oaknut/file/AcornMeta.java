package org.rob_smallshire.oaknut.file;

import java.time.Instant;

/* Copyright (c) 2026 Robert Smallshire. MIT license; see ../LICENSE. */
public record AcornMeta(long loadAddress, long executionAddress, Integer access)
{
    public AcornMeta
    {
        if (loadAddress < 0 || loadAddress > 0xffffffffL || executionAddress < 0 ||
            executionAddress > 0xffffffffL)
            throw new IllegalArgumentException("Address outside unsigned 32-bit range");
    }
    public boolean isDatestamped()
    { return (loadAddress & 0xfff00000L) == 0xfff00000L && loadAddress != executionAddress; }
    public Integer filetype()
    { return isDatestamped() ? (int)((loadAddress >>> 8) & 4095) : null; }
    public Instant datestamp()
    {
        if (!isDatestamped())
            return null;
        long centiseconds = ((loadAddress & 255) << 32) | executionAddress;
        return Datestamp.EPOCH.plusMillis(centiseconds * 10);
    }
    public AcornMeta withFiletype(int type)
    {
        if (type < 0 || type > 4095)
            throw new IllegalArgumentException("Invalid filetype");
        return new AcornMeta(0xfff00000L | ((long)type << 8) |
                                 (isDatestamped() ? loadAddress & 255 : 0),
                             isDatestamped() ? executionAddress : 0,
                             access);
    }
    public AcornMeta withDatestamp(Instant when)
    {
        long cs = Datestamp.encode(when);
        int type = filetype() == null ? 0xffd : filetype();
        return new AcornMeta(
            0xfff00000L | ((long)type << 8) | (cs >>> 32), cs & 0xffffffffL, access);
    }
}
