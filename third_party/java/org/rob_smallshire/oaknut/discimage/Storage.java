package org.rob_smallshire.oaknut.discimage;

import java.io.IOException;

/* Random-access image storage. Offsets are physical image bytes, before surface mapping.
 * Reads and writes must transfer exactly length bytes or throw IOException. Storage is borrowed:
 * closing an Adfs or Dfs handle does not close it. Owners control commit/revert independently. */
public interface Storage
{
    /* Returns the image size in bytes without reading the image contents. */
    int size();

    /* Copies exactly length bytes from the image at offset into destination, beginning at
     * destinationOffset. Both ranges must be within bounds. A storage I/O failure throws
     * IOException; implementations must not silently return a short read. */
    void read(int offset, byte[] destination, int destinationOffset, int length) throws IOException;

    /* Writes exactly length bytes from source, beginning at sourceOffset, to the image at offset.
     * Both ranges must be within bounds. A storage I/O failure throws IOException, and earlier
     * writes may already have taken effect: this method does not guarantee atomicity or commit
     * the owner's transaction. */
    void write(int offset, byte[] source, int sourceOffset, int length) throws IOException;

    /* Reports whether writes are prohibited. The default assumes writable storage. */
    default boolean isReadOnly()
    {
        return false;
    }

    /* Flushes any implementation-specific write buffers without closing the storage. Handles call
     * this explicitly and on close. Transaction commit/revert remains the owner's responsibility;
     * implementations with no buffered writes can retain the default no-op. */
    default void flush() throws IOException
    {
    }
}
