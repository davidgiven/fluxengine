package org.rob_smallshire.oaknut.dfs;

import org.rob_smallshire.oaknut.file.Access;
import org.rob_smallshire.oaknut.file.AcornMeta;

/* Copyright (c) 2026 Robert Smallshire. MIT license; see ../LICENSE. */
public record DfsFileEntry(String name, boolean locked, long loadAddress, long executionAddress,
                           int length, int startSector)
{
    /* The directory character is part of the flat filename, for example A.HELLO. */
    public int sectorsRequired()
    {
        return (length + 255) / 256;
    }

    public AcornMeta metadata()
    {
        return new AcornMeta(
                loadAddress,
                executionAddress,
                locked ? Access.L | Access.R : Access.R | Access.W);
    }

    public DfsFileEntry withName(String value)
    {
        return new DfsFileEntry(value, locked, loadAddress, executionAddress, length, startSector);
    }

    public DfsFileEntry withMetadata(AcornMeta value)
    {
        return new DfsFileEntry(
                name,
                value.access() == null ? locked : (value.access() & Access.L) != 0,
                value.loadAddress(),
                value.executionAddress(),
                length,
                startSector);
    }

    public DfsFileEntry withStartSector(int value)
    {
        return new DfsFileEntry(name, locked, loadAddress, executionAddress, length, value);
    }
}
