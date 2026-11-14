package org.rob_smallshire.oaknut.adfs;

import org.rob_smallshire.oaknut.file.AcornMeta;
import org.rob_smallshire.oaknut.file.Access;

/* Copyright (c) 2026 Robert Smallshire. MIT license; see ../LICENSE. */
public record AdfsStat(String name,
                       int length,
                       long loadAddress,
                       long executionAddress,
                       int discAddress,
                       int sequence,
                       int access,
                       boolean directory)
{
    public boolean locked()
    { return (access & Access.L) != 0; }
    public AcornMeta metadata()
    { return new AcornMeta(loadAddress, executionAddress, access); }
    static AdfsStat of(DirectoryEntry e)
    {
        return new AdfsStat(e.name(),
                            e.length(),
                            e.loadAddress(),
                            e.executionAddress(),
                            e.discAddress(),
                            e.sequence(),
                            e.access(),
                            e.directory());
    }
}
