package org.rob_smallshire.oaknut.adfs;

import static org.junit.Assert.*;

import java.io.IOException;
import java.io.UncheckedIOException;
import java.util.Arrays;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;
import org.rob_smallshire.oaknut.discimage.Storage;

/* External storage must not require file contents or free space to be readable at mount time. */
@RunWith(JUnit4.class)
public class StorageTest
{
    @Test
    public void mountDoesNotReadFileContentsAndMutationDoesNotSnapshotFreeSpace() throws Exception
    {
        byte[] payload = new byte[8192];
        Arrays.fill(payload, (byte) 0xa5);
        byte[] bytes;
        int fileStart;
        try (Adfs source = Adfs.create(AdfsFormat.S))
        {
            source.path("File").writeBytes(payload);
            fileStart = source.path("File").stat().discAddress() * 256;
            bytes = source.toByteArray();
        }
        GuardedStorage storage = new GuardedStorage(bytes);
        storage.forbiddenStart = fileStart;
        storage.forbiddenEnd = fileStart + payload.length;
        try (Adfs disc = Adfs.fromStorage(storage, false))
        {
            assertEquals(payload.length, disc.path("File").stat().length());
            assertTrue(storage.reads > 0);
            assertEquals(0, storage.writes);

            /* File data is fetched only on demand. Unrelated free space stays inaccessible. */
            storage.forbiddenStart = 65536;
            storage.forbiddenEnd = bytes.length;
            assertArrayEquals(payload, disc.path("File").readBytes());
            disc.path("New").writeBytes(new byte[]{1, 2, 3});
            assertArrayEquals(new byte[]{1, 2, 3}, disc.path("New").readBytes());
            assertTrue(storage.writes > 0);
        }
        assertEquals(1, storage.flushes);
    }

    @Test
    public void rollbackCanStillBeEnabledForOtherStorageUsers() throws Exception
    {
        byte[] bytes;
        try (Adfs source = Adfs.create(AdfsFormat.S))
        {
            bytes = source.toByteArray();
        }
        GuardedStorage storage = new GuardedStorage(bytes);
        try (Adfs disc = Adfs.fromStorage(storage, true))
        {
            storage.forbiddenStart = 65536;
            storage.forbiddenEnd = bytes.length;
            assertThrows(
                    UncheckedIOException.class,
                    () -> disc.path("New").writeBytes(new byte[]{1}));
            assertEquals(0, storage.writes);
        }
    }

    @Test
    public void mountPreservesIoFailureAndDoesNotTreatItAsAnInvalidFormat() throws Exception
    {
        GuardedStorage storage = new GuardedStorage(new byte[163840]);
        storage.forbiddenStart = 0;
        storage.forbiddenEnd = storage.size();
        IOException failure =
                assertThrows(IOException.class, () -> Adfs.fromStorage(storage, false));
        assertEquals("unreadable range", failure.getMessage());
    }

    @Test
    public void explicitFormatStorageWorksForEveryFloppyFormat() throws Exception
    {
        for (AdfsFormat format : AdfsFormat.values())
        {
            byte[] bytes;
            try (Adfs source = Adfs.create(format))
            {
                bytes = source.toByteArray();
            }
            GuardedStorage storage = new GuardedStorage(bytes);
            try (Adfs disc = Adfs.fromStorage(storage, format, false))
            {
                disc.path("File").writeBytes(new byte[]{4, 5, 6});
                assertArrayEquals(
                        format.toString(),
                        new byte[]{4, 5, 6},
                        disc.path("File").readBytes());
                assertTrue(format.toString(), disc.validate().isEmpty());
            }
            try (Adfs reopened = Adfs.fromBuffer(java.nio.ByteBuffer.wrap(bytes), format))
            {
                assertArrayEquals(new byte[]{4, 5, 6}, reopened.path("File").readBytes());
            }
        }
    }

    /* Rejecting ranges, rather than just counting callbacks, detects eager scans and snapshots. */
    private static final class GuardedStorage implements Storage
    {
        private final byte[] bytes;
        int forbiddenStart = -1;
        int forbiddenEnd = -1;
        int reads;
        int writes;
        int flushes;

        GuardedStorage(byte[] bytes)
        {
            this.bytes = bytes;
        }

        @Override
        public int size()
        {
            return bytes.length;
        }

        @Override
        public void read(int offset, byte[] destination, int start, int length) throws IOException
        {
            if (offset < forbiddenEnd && offset + length > forbiddenStart)
                throw new IOException("unreadable range");
            reads++;
            System.arraycopy(bytes, offset, destination, start, length);
        }

        @Override
        public void write(int offset, byte[] source, int start, int length)
        {
            writes++;
            System.arraycopy(source, start, bytes, offset, length);
        }

        @Override
        public void flush()
        {
            flushes++;
        }
    }
}
