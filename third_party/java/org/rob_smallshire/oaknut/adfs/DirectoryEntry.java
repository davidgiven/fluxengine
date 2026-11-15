package org.rob_smallshire.oaknut.adfs;

/* Copyright (c) 2026 Robert Smallshire. MIT license; see ../LICENSE.
 * Access is the canonical OSFILE word; the directory flag is kept separately. */
public record DirectoryEntry(String name, long loadAddress, long executionAddress, int length,
                             int discAddress, int sequence, int access, boolean directory)
{
    public DirectoryEntry withName(String name)
    {
        return new DirectoryEntry(
                name,
                loadAddress,
                executionAddress,
                length,
                discAddress,
                sequence,
                access,
                directory);
    }

    public DirectoryEntry withAddress(int address, int size)
    {
        return new DirectoryEntry(
                name,
                loadAddress,
                executionAddress,
                size,
                address,
                sequence,
                access,
                directory);
    }
}
