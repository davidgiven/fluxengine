package org.rob_smallshire.oaknut.discimage;

import com.google.common.collect.ImmutableList;
import java.nio.ByteBuffer;
import java.util.ArrayList;
import java.util.List;

/* Copyright (c) 2026 Robert Smallshire. MIT license; see ../LICENSE.
 * Physical image storage; short images remain short so missing sectors cannot appear valid. */
public final class DiscImage
{
    private final Storage storage;
    private final ImmutableList<SurfaceSpec> surfaces;

    public DiscImage(ByteBuffer buffer, List<SurfaceSpec> surfaces)
    {
        this(new BufferStorage(buffer), surfaces);
    }

    public DiscImage(Storage storage, List<SurfaceSpec> surfaces)
    {
        this.storage = storage;
        this.surfaces = ImmutableList.copyOf(surfaces);
        if (surfaces.isEmpty())
            throw new IllegalArgumentException("No surfaces");
    }

    public ImmutableList<SurfaceSpec> surfaces()
    {
        return surfaces;
    }

    public ByteBuffer buffer()
    {
        if (storage instanceof BufferStorage buffer)
            return buffer.buffer();
        throw new UnsupportedOperationException("Storage is not buffer-backed");
    }

    public Surface surface(int side)
    {
        return new Surface(this, side);
    }

    public SectorsView sectors(int side, int first, int count)
    {
        SurfaceSpec spec = surfaces.get(side);
        java.util.Objects.checkFromIndexSize(first, count, spec.sectors());
        ArrayList<SectorsView> parts = new ArrayList<>();
        for (int sector = first; sector < first + count; sector++)
        {
            int offset = spec.offset(sector);
            if (offset + spec.bytesPerSector() > storage.size())
                throw new IndexOutOfBoundsException("Sector lies beyond truncated image");
            parts.add(new SectorsView(storage, offset, spec.bytesPerSector()));
        }
        return SectorsView.join(parts);
    }
}
