package org.rob_smallshire.oaknut.adfs;

import com.google.common.collect.ImmutableList;
import java.util.List;

/* Copyright (c) 2026 Robert Smallshire. MIT license; see ../LICENSE. */
public record Directory(String name, String title, int parentAddress, int discAddress,
                        ImmutableList<DirectoryEntry> entries, int sequence, String signature,
                        int bigSize)
{
    public Directory(
            String name,
            String title,
            int parent,
            int address,
            List<DirectoryEntry> entries,
            int sequence,
            String signature,
            int bigSize)
    {
        this(
                name,
                title,
                parent,
                address,
                ImmutableList.copyOf(entries),
                sequence,
                signature,
                bigSize);
    }

    public DirectoryEntry find(String name)
    {
        String key = DirectoryFormat.nameKey(name);
        return entries
                .stream()
                .filter(e -> DirectoryFormat.nameKey(e.name()).equals(key))
                .findFirst()
                .orElse(null);
    }

    public Directory withEntries(List<DirectoryEntry> entries)
    {
        return new Directory(
                name,
                title,
                parentAddress,
                discAddress,
                entries,
                (sequence + 1) & 255,
                signature,
                bigSize);
    }
}
