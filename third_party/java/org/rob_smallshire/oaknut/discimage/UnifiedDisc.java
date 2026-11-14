package org.rob_smallshire.oaknut.discimage;

import java.util.ArrayList;
import java.nio.ByteBuffer;

/* Copyright (c) 2026 Robert Smallshire. MIT license; see ../LICENSE.
 * ADFS logical sectors run through all of side zero before side one. */
public final class UnifiedDisc
{
    private final DiscImage image;
    private final int sectorSize;

    public UnifiedDisc(DiscImage image)
    {
        this.image = image;
        sectorSize = image.surfaces().getFirst().bytesPerSector();
        if (image.surfaces().stream().anyMatch(s -> s.bytesPerSector() != sectorSize))
            throw new IllegalArgumentException("Surfaces have different sector sizes");
    }

    public int sectorSize()
    {
        return sectorSize;
    }

    public int sectors()
    {
        return image.surfaces().stream().mapToInt(SurfaceSpec::sectors).sum();
    }

    public int size()
    {
        return sectors() * sectorSize;
    }

    public SectorsView sectorRange(int start, int count)
    {
        java.util.Objects.checkFromIndexSize(start, count, sectors());
        ArrayList<ByteBuffer> parts = new ArrayList<>();
        for (SurfaceSpec spec : image.surfaces())
        {
            int take = Math.min(count, Math.max(0, spec.sectors() - start));
            for (int i = 0; i < take; i++)
                parts.add(image.buffer().slice(spec.offset(start + i), sectorSize));
            count -= take;
            start = Math.max(0, start - spec.sectors());
        }
        return new SectorsView(parts);
    }

    public SectorsView bytes(int offset, int length)
    {
        java.util.Objects.checkFromIndexSize(offset, length, size());
        if (length == 0)
            return SectorsView.wrap(new byte[0]);
        int first = offset / sectorSize;
        int last = (offset + length + sectorSize - 1) / sectorSize;
        return sectorRange(first, last - first).slice(offset % sectorSize, length);
    }
}
