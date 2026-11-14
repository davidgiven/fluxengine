package org.rob_smallshire.oaknut.discimage;

import static org.junit.Assert.*;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;
import java.nio.ByteBuffer;
import java.util.List;

/* Copyright (c) 2026 Robert Smallshire. MIT license; see the oaknut LICENSE.
 * Ported sector-view, surface and unified-disc invariants. */
@RunWith(JUnit4.class) public class DiscImageTest
{
    @Test public void fragmentedViewsWriteThroughAcrossBoundaries()
    {
        byte[] bytes = {0, 1, 2, 3, 4, 5, 6, 7};
        ByteBuffer original = ByteBuffer.wrap(bytes);
        SectorsView view = new SectorsView(
            List.of(original.slice(6, 2), original.slice(0, 2), original.slice(3, 2)));
        assertArrayEquals(new byte[] {6, 7, 0, 1, 3, 4}, view.toByteArray());
        view.slice(1, 4).write(0, new byte[] {10, 11, 12, 13});
        assertArrayEquals(new byte[] {11, 12, 2, 13, 4, 5, 6, 10}, bytes);
        view.writeLe(0, 0xfedcba98L, 4);
        assertEquals(0xfedcba98L, view.readLe(0, 4));
        assertEquals(0, original.position());
        assertThrows(IndexOutOfBoundsException.class, () -> view.slice(5, 2));
        assertThrows(IndexOutOfBoundsException.class, () -> view.get(6));
        assertThrows(IllegalArgumentException.class, () -> view.put(0, 256));
    }

    @Test public void readOnlyWriteDoesNotPartiallyModifyEarlierBuffers()
    {
        byte[] first = {1, 2};
        SectorsView mixed = new SectorsView(
            List.of(ByteBuffer.wrap(first), ByteBuffer.allocate(2).asReadOnlyBuffer()));
        assertThrows(java.nio.ReadOnlyBufferException.class, () -> mixed.write(0, new byte[4]));
        assertArrayEquals(new byte[] {1, 2}, first);
    }

    @Test public void unifiedSectorOrderDiffersFromInterleavedPhysicalOrder()
    {
        ByteBuffer buffer = ByteBuffer.allocate(32);
        SurfaceSpec first = new SurfaceSpec(2, 2, 4, 0, 16);
        SurfaceSpec second = new SurfaceSpec(2, 2, 4, 8, 16);
        DiscImage image = new DiscImage(buffer, List.of(first, second));
        UnifiedDisc unified = new UnifiedDisc(image);
        for (int sector = 0; sector < 8; sector++)
            unified.sectorRange(sector, 1).write(0, new byte[] {(byte)sector, 0, 0, 0});
        assertEquals(0, buffer.get(0));
        assertEquals(1, buffer.get(4));
        assertEquals(4, buffer.get(8));
        assertEquals(5, buffer.get(12));
        assertEquals(2, buffer.get(16));
        assertEquals(3, buffer.get(20));
        assertEquals(6, buffer.get(24));
        assertEquals(7, buffer.get(28));
        unified.bytes(15, 2).write(0, new byte[] {99, 98});
        assertEquals(99, buffer.get(23));
        assertEquals(98, buffer.get(8));
        assertThrows(IndexOutOfBoundsException.class, () -> unified.sectorRange(7, 2));
    }

    @Test public void truncatedAndInvalidSurfacesAreRejected()
    {
        assertThrows(IllegalArgumentException.class, () -> new SurfaceSpec(0, 2, 4, 0, 8));
        assertThrows(IllegalArgumentException.class, () -> new SurfaceSpec(2, 2, 4, 0, 4));
        DiscImage shortImage =
            new DiscImage(ByteBuffer.allocate(8), List.of(new SurfaceSpec(2, 2, 4, 0, 8)));
        assertThrows(IndexOutOfBoundsException.class,
                     () -> shortImage.surface(0).sectorRange(2, 1));
    }
}
