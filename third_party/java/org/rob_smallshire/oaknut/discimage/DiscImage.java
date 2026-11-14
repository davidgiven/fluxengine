package org.rob_smallshire.oaknut.discimage;

import java.nio.ByteBuffer;
import java.util.List;
import com.google.common.collect.ImmutableList;
import java.util.ArrayList;

/* Copyright (c) 2026 Robert Smallshire. MIT license; see ../LICENSE.
 * Physical image storage; short images remain short so missing sectors cannot appear valid. */
public final class DiscImage
{
    private final ByteBuffer buffer;
    private final ImmutableList<SurfaceSpec> surfaces;

    public DiscImage(ByteBuffer buffer, List<SurfaceSpec> surfaces)
    {
        this.buffer = buffer.slice();
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
        return buffer.duplicate();
    }

    public Surface surface(int side)
    {
        return new Surface(this, side);
    }

    public SectorsView sectors(int side, int first, int count)
    {
        SurfaceSpec spec = surfaces.get(side);
        java.util.Objects.checkFromIndexSize(first, count, spec.sectors());
        ArrayList<ByteBuffer> parts = new ArrayList<>();
        for (int sector = first; sector < first + count; sector++)
        {
            int offset = spec.offset(sector);
            if (offset + spec.bytesPerSector() > buffer.capacity())
                throw new IndexOutOfBoundsException("Sector lies beyond truncated image");
            parts.add(buffer.slice(offset, spec.bytesPerSector()));
        }
        return new SectorsView(parts);
    }
}
