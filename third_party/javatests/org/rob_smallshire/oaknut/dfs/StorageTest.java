package org.rob_smallshire.oaknut.dfs;

import static org.junit.Assert.assertArrayEquals;
import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertThrows;
import static org.junit.Assert.assertTrue;

import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;
import org.rob_smallshire.oaknut.discimage.Storage;
import java.io.IOException;
import java.util.Arrays;

/* Copyright (c) 2026 Robert Smallshire. MIT license; see
 * ../../../../../../../java/org/rob_smallshire/oaknut/LICENSE.
 * External storage tests protect lazy access, borrowed ownership and optional rollback. */
@RunWith(JUnit4.class)
public class StorageTest
{
    private static final DfsFormat FORMAT = DfsFormat.ACORN_DFS_80T_SINGLE_SIDED;

    private static final class RecordingStorage implements Storage
    {
        byte[] bytes;
        int reads, writes, readBytes, flushes;
        boolean readOnly, failRead, failNextWrite;
        int writesBeforeFailure = -1;

        RecordingStorage() throws IOException
        {
            try (Dfs disc = Dfs.create(FORMAT))
            {
                bytes = disc.toByteArray();
            }
        }

        @Override
        public int size()
        {
            return bytes.length;
        }

        @Override
        public boolean isReadOnly()
        {
            return readOnly;
        }

        @Override
        public void read(int offset, byte[] destination, int destinationOffset, int length) throws IOException
        {
            if (failRead)
                throw new IOException("read failed");
            reads++;
            readBytes += length;
            System.arraycopy(bytes, offset, destination, destinationOffset, length);
        }

        @Override
        public void write(int offset, byte[] source, int sourceOffset, int length) throws IOException
        {
            if (failNextWrite || writesBeforeFailure == 0)
            {
                failNextWrite = false;
                writesBeforeFailure = -1;
                throw new IOException("write failed");
            }
            if (writesBeforeFailure > 0)
                writesBeforeFailure--;
            writes++;
            System.arraycopy(source, sourceOffset, bytes, offset, length);
        }

        @Override
        public void flush()
        {
            flushes++;
        }
    }

    @Test
    public void mountsAndMutatesWithoutWholeImageReads() throws Exception
    {
        RecordingStorage storage = new RecordingStorage();
        try (Dfs disc = Dfs.fromStorage(storage, FORMAT, false))
        {
            assertTrue(storage.readBytes < 2048);
            disc.writeFile("DATA", new byte[]{10, 20});
            assertTrue(storage.readBytes < 10000);
            assertArrayEquals(new byte[]{10, 20}, disc.readFile("DATA"));
            storage.bytes[disc.stat("DATA").startSector() * 256] = 42;
            assertArrayEquals(new byte[]{42, 20}, disc.readFile("DATA"));
            disc.setTitle("TITLE");
            assertTrue(storage.readBytes < 20000);
            assertTrue(storage.writes > 0);
        }
        assertEquals(1, storage.flushes);
        /* Closing the handle leaves its borrowed storage usable. */
        byte[] first = new byte[1];
        storage.read(0, first, 0, 1);
        assertEquals('T', first[0]);
    }

    @Test
    public void rollbackRestoresDataWhenPlanningOrIoFails() throws Exception
    {
        RecordingStorage storage = new RecordingStorage();
        try (Dfs disc = Dfs.fromStorage(storage, FORMAT, true))
        {
            disc.writeFile("DATA", new byte[]{1, 2, 3});
            byte[] before = storage.bytes.clone();
            /* Write the new file bytes, then fail while replacing its catalogue entry. */
            storage.writesBeforeFailure = 1;
            assertEquals("write failed", assertThrows(IOException.class,
                    () -> disc.writeFile("DATA", new byte[]{4, 5})).getMessage());
            assertArrayEquals(before, storage.bytes);
            assertArrayEquals(new byte[]{1, 2, 3}, disc.readFile("DATA"));
        }
    }

    @Test
    public void storageIoFailuresAreNotWrappedOrSwallowed() throws Exception
    {
        RecordingStorage storage = new RecordingStorage();
        storage.failRead = true;
        assertEquals("read failed", assertThrows(IOException.class,
                () -> Dfs.fromStorage(storage, FORMAT, false)).getMessage());
        storage.failRead = false;
        try (Dfs disc = Dfs.fromStorage(storage, FORMAT, false))
        {
            storage.failRead = true;
            assertEquals("read failed", assertThrows(IOException.class, disc::files).getMessage());
        }
    }

    @Test
    public void readOnlyStorageRejectsMutationsBeforeWriting() throws Exception
    {
        RecordingStorage storage = new RecordingStorage();
        storage.readOnly = true;
        try (Dfs disc = Dfs.fromStorage(storage, FORMAT, false))
        {
            assertTrue(disc.isReadOnly());
            assertThrows(DfsReadOnlyException.class, () -> disc.setTitle("NEW"));
            assertThrows(DfsReadOnlyException.class, () -> disc.writeFile("DATA", new byte[0]));
            assertThrows(DfsReadOnlyException.class, disc::compact);
            assertEquals(0, storage.writes);
        }
    }

    @Test
    public void externalShortStorageDoesNotSilentlyInventMissingSectors() throws Exception
    {
        RecordingStorage storage = new RecordingStorage();
        try (Dfs disc = Dfs.fromStorage(storage, FORMAT, false))
        {
            disc.writeFile("DATA", new byte[512]);
        }
        storage.bytes = Arrays.copyOf(storage.bytes, 768);
        try (Dfs disc = Dfs.fromStorage(storage, FORMAT, false))
        {
            assertThrows(DfsFormatException.class, () -> disc.readFile("DATA"));
            assertFalse(disc.validate().isEmpty());
            assertEquals(0, disc.freeSectors());
        }
    }
}
