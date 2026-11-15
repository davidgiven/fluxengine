package org.rob_smallshire.oaknut.dfs;

import com.google.common.collect.ImmutableList;
import org.rob_smallshire.oaknut.file.AcornMeta;
import java.io.IOException;
import java.nio.file.NoSuchFileException;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Comparator;
import java.util.HashMap;
import java.util.HashSet;
import java.util.List;
import java.util.Map;
import java.util.Set;

/* Copyright (c) 2026 Robert Smallshire. MIT license; see ../LICENSE.
 * File allocation on one independent DFS side. Files are contiguous, allocated first-fit. */
public final class CataloguedSurface
{
    private final Catalogue catalogue;

    public CataloguedSurface(Catalogue catalogue)
    {
        this.catalogue = catalogue;
    }

    public Catalogue catalogue()
    {
        return catalogue;
    }

    public int usableSectors() throws IOException
    {
        return Math.min(catalogue.info().totalSectors(), catalogue.heldSectors());
    }

    public record FreeRegion(int startSector, int sectors)
    {
    }

    public ImmutableList<FreeRegion> freeMap() throws IOException
    {
        return freeMap(catalogue.files());
    }

    private ImmutableList<FreeRegion> freeMap(List<DfsFileEntry> files) throws IOException
    {
        boolean[] used = new boolean[usableSectors()];
        Arrays.fill(used, 0, Math.min(catalogue.catalogueSectors(), used.length), true);
        for (DfsFileEntry file : files)
            for (int i = file.startSector(); i < Math.min(used.length, file.startSector() + file.sectorsRequired()); i++)
                used[i] = true;
        ImmutableList.Builder<FreeRegion> regions = ImmutableList.builder();
        int start = -1;
        for (int sector = 0; sector <= used.length; sector++)
        {
            if (sector < used.length && !used[sector])
            {
                if (start == -1)
                    start = sector;
            } else if (start != -1)
            {
                regions.add(new FreeRegion(start, sector - start));
                start = -1;
            }
        }
        return regions.build();
    }

    public int freeSectors() throws IOException
    {
        int free = 0;
        for (FreeRegion region : freeMap())
            free += region.sectors();
        return free;
    }

    public byte[] readFile(String name) throws IOException
    {
        DfsFileEntry file = catalogue.stat(name);
        if (file.length() == 0)
            return new byte[0];
        return Arrays.copyOf(catalogue.read(file.startSector(), file.sectorsRequired()), file.length());
    }

    /* Plan overwrite and allocation before changing the original entry. This also permits
     * replacing a file in a full catalogue and reusing its old sectors. */
    public void writeFile(String name, byte[] data, AcornMeta metadata) throws IOException
    {
        writeFile(name, data, metadata, false);
    }

    public void writeFile(String name, byte[] data, AcornMeta metadata, boolean verbatimName) throws IOException
    {
        String full = Catalogue.filename(name, verbatimName);
        if (data.length > 0x3ffff)
            throw new IllegalArgumentException("DFS file exceeds 18-bit length");
        DfsFileEntry previous = catalogue.find(full);
        if (previous != null && previous.locked())
            throw new DfsFileLockedException(full);
        ArrayList<DfsFileEntry> files = new ArrayList<>(catalogue.files());
        if (previous != null)
            files.remove(previous);
        if (files.size() >= catalogue.maxFiles())
            throw new DfsCatalogueFullException("Catalogue full");
        int needed = (data.length + 255) / 256;
        int start = needed == 0 ? catalogue.catalogueSectors() : -1;
        for (FreeRegion region : freeMap(files))
            if (region.sectors() >= needed)
            {
                start = region.startSector();
                break;
            }
        if (start == -1)
            throw new DfsDiscFullException("Not enough contiguous free space: " + needed + " sectors");
        int access = metadata.access() == null ? 0 : metadata.access();
        files.add(new DfsFileEntry(full, (access & org.rob_smallshire.oaknut.file.Access.L) != 0,
                metadata.loadAddress(), metadata.executionAddress(), data.length, start));
        if (needed != 0)
            catalogue.write(start, Arrays.copyOf(data, needed * 256));
        catalogue.replaceEntries(files);
    }

    /* Read every source before moving it, since the target sectors can overlap old extents.
     * The partial order names files to promote; other files retain their physical order. */
    public int compact(List<String> order) throws IOException
    {
        ArrayList<DfsFileEntry> physical = new ArrayList<>(catalogue.files());
        physical.sort(Comparator.comparingInt(DfsFileEntry::startSector));
        Map<String, DfsFileEntry> byName = new HashMap<>();
        for (DfsFileEntry file : physical)
            byName.put(Catalogue.key(file.name()), file);
        ArrayList<DfsFileEntry> ordered = new ArrayList<>();
        Set<String> selected = new HashSet<>();
        for (String name : order)
        {
            String key = Catalogue.key(name);
            DfsFileEntry file = byName.get(key);
            if (file == null)
                throw new NoSuchFileException(name);
            if (selected.add(key))
                ordered.add(file);
        }
        for (DfsFileEntry file : physical)
            if (selected.add(Catalogue.key(file.name())))
                ordered.add(file);
        if (ordered.isEmpty())
            return 0;
        ArrayList<byte[]> contents = new ArrayList<>();
        ArrayList<DfsFileEntry> moved = new ArrayList<>();
        int next = catalogue.catalogueSectors();
        for (DfsFileEntry file : ordered)
        {
            contents.add(readFile(file.name()));
            moved.add(file.withStartSector(next));
            next += file.sectorsRequired();
        }
        if (next > usableSectors())
            throw new DfsDiscFullException("Compacted files exceed usable disc size");
        for (int i = 0; i < moved.size(); i++)
            if (moved.get(i).sectorsRequired() != 0)
                catalogue.write(moved.get(i).startSector(),
                        Arrays.copyOf(contents.get(i), moved.get(i).sectorsRequired() * 256));
        catalogue.replaceEntries(moved);
        return moved.size();
    }
}
