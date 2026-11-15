package org.rob_smallshire.oaknut.discimage;

import java.nio.ByteBuffer;

/* Buffer-backed implementation preserving the caller's position and limit. */
public final class BufferStorage implements Storage
{
    private final ByteBuffer buffer;

    public BufferStorage(ByteBuffer buffer)
    {
        this.buffer = buffer.slice();
    }

    public ByteBuffer buffer()
    {
        return buffer.duplicate();
    }

    @Override
    public int size()
    {
        return buffer.capacity();
    }

    @Override
    public boolean isReadOnly()
    {
        return buffer.isReadOnly();
    }

    @Override
    public void read(int offset, byte[] destination, int destinationOffset, int length)
    {
        buffer.get(offset, destination, destinationOffset, length);
    }

    @Override
    public void write(int offset, byte[] source, int sourceOffset, int length)
    {
        buffer.put(offset, source, sourceOffset, length);
    }
}
