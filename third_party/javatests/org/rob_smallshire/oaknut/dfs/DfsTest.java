package org.rob_smallshire.oaknut.dfs;

import static org.junit.Assert.assertArrayEquals;
import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertThrows;
import static org.junit.Assert.assertTrue;

import org.junit.Rule;
import org.junit.Test;
import org.junit.rules.TemporaryFolder;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;
import org.rob_smallshire.oaknut.file.Access;
import org.rob_smallshire.oaknut.file.AcornMeta;
import java.io.IOException;
import java.nio.ByteBuffer;
import java.nio.file.FileAlreadyExistsException;
import java.nio.file.Files;
import java.nio.file.NoSuchFileException;
import java.nio.file.Path;
import java.util.Arrays;
import java.util.List;

/* Copyright (c) 2026 Robert Smallshire. MIT license; see
 * ../../../../../../../java/org/rob_smallshire/oaknut/LICENSE.
 * Translated DFS creation, metadata, naming, allocation and persistence scenarios. */
@RunWith(JUnit4.class)
public class DfsTest
{
    @Rule public TemporaryFolder temporary = new TemporaryFolder();

    private static byte[] payload(int size)
    {
        byte[] bytes = new byte[size];
        for (int i = 0; i < size; i++)
            bytes[i] = (byte) (i * 17 + 3);
        return bytes;
    }

    @Test
    public void everyFormatCreatesAndReopensBothSides() throws Exception
    {
        for (DfsFormat format : DfsFormat.values())
            try (Dfs disc = Dfs.create(format, "Title", 3))
            {
                assertEquals(format.size(), disc.toByteArray().length);
                assertEquals("Title", disc.title());
                assertEquals(3, disc.info().bootOption());
                assertEquals(format.tracks * 10, disc.info().totalSectors());
                assertEquals(format.tracks * 10 - (format.watford ? 4 : 2), disc.freeSectors());
                byte[] data = payload(5011);
                disc.writeFile("a.Hello", data, new AcornMeta(0xffff1900L, 0x28023, Access.R | Access.W));
                assertEquals("a.Hello", disc.stat("A.HELLO").name());
                assertEquals(0xffff1900L, disc.stat("a.hello").loadAddress());
                assertEquals(0x28023, disc.stat("a.hello").executionAddress());
                assertTrue(disc.validate().toString(), disc.validate().isEmpty());
                try (Dfs reopened = Dfs.fromBuffer(ByteBuffer.wrap(disc.toByteArray()), format))
                {
                    assertArrayEquals(data, reopened.readFile("A.hello"));
                }
                if (format.sides == 2)
                {
                    ByteBuffer backing = ByteBuffer.wrap(disc.toByteArray());
                    try (Dfs side1 = Dfs.fromBuffer(backing, format, 1);
                            Dfs side0 = Dfs.fromBuffer(backing, format, 0))
                    {
                        assertTrue(side1.files().isEmpty());
                        assertEquals("", side1.title());
                        side1.writeFile("SIDEONE", new byte[]{42});
                        assertFalse(side0.exists("SIDEONE"));
                        assertArrayEquals(data, side0.readFile("a.Hello"));
                        assertArrayEquals(new byte[]{42}, side1.readFile("SIDEONE"));
                    }
                }
            }
    }

    @Test
    public void writesReplaceAndRenamesRefuseCollisions() throws Exception
    {
        for (DfsFormat format : List.of(DfsFormat.ACORN_DFS_40T_SINGLE_SIDED, DfsFormat.WATFORD_DFS_40T_SINGLE_SIDED))
            try (Dfs disc = Dfs.create(format))
            {
                disc.writeFile("Hello", new byte[]{1});
                disc.writeFile("$.hELLO", new byte[]{2, 3});
                assertEquals(1, disc.files().size());
                assertArrayEquals(new byte[]{2, 3}, disc.readFile("HELLO"));
                disc.writeFile("A.Other", new byte[]{4});
                byte[] before = disc.toByteArray();
                assertThrows(FileAlreadyExistsException.class, () -> disc.renameFile("hello", "a.other"));
                assertArrayEquals(before, disc.toByteArray());
                disc.renameFile("hello", "B.New");
                assertFalse(disc.exists("hello"));
                assertArrayEquals(new byte[]{2, 3}, disc.readFile("b.new"));
                disc.copyFile("B.New", "C.Copy");
                assertArrayEquals(disc.readFile("B.New"), disc.readFile("C.Copy"));
                disc.deleteFile("B.New");
                assertThrows(NoSuchFileException.class, () -> disc.stat("B.New"));
                assertThrows(NoSuchFileException.class, () -> disc.deleteFile("missing"));
            }
    }

    @Test
    public void lockAndAddressMetadataFollowDfsConventions() throws Exception
    {
        try (Dfs disc = Dfs.create(DfsFormat.ACORN_DFS_80T_SINGLE_SIDED))
        {
            disc.writeFile("DATA", new byte[]{7});
            for (long address : new long[]{0, 0xffff, 0x10000, 0x20000, 0x2ffff, 0x30000, 0x3abcd, 0xffffffffL})
            {
                disc.setLoadAddress("DATA", address);
                disc.setExecutionAddress("DATA", address);
                assertEquals(Catalogue.expandHostAddress(address & 0x3ffff), disc.stat("DATA").loadAddress());
                assertEquals(disc.stat("DATA").loadAddress(), disc.stat("DATA").executionAddress());
            }
            disc.setLocked("DATA", true);
            assertEquals(Access.L | Access.R, (int) disc.stat("DATA").metadata().access());
            assertThrows(DfsFileLockedException.class, () -> disc.deleteFile("DATA"));
            assertThrows(DfsFileLockedException.class, () -> disc.writeFile("DATA", new byte[0]));
            disc.renameFile("DATA", "LOCKED");
            assertTrue(disc.stat("LOCKED").locked());
            disc.setLocked("LOCKED", false);
            assertEquals(Access.R | Access.W, (int) disc.stat("LOCKED").metadata().access());
            disc.deleteFile("LOCKED");
        }
    }

    @Test
    public void namesAreFlatCasePreservingAndBoundedByStorage() throws Exception
    {
        for (DfsFormat format : List.of(DfsFormat.ACORN_DFS_40T_SINGLE_SIDED, DfsFormat.WATFORD_DFS_40T_SINGLE_SIDED))
            try (Dfs disc = Dfs.create(format))
            {
                for (String name : List.of("GUARD#1", "GUARD#2", "SAVE*", "DATA!1", "A\u0001B", "a.Mixed"))
                {
                    disc.writeFile(name, new byte[]{1});
                    assertTrue(disc.exists(name));
                    assertEquals(Catalogue.filename(name, false), disc.stat(name).name());
                }
                for (String name : List.of("", "TOOLONGNAME", "A.B.C", "BAD:NAME", "A\u00ffB", "&.NEW"))
                    assertThrows(name, IllegalArgumentException.class, () -> disc.writeFile(name, new byte[]{1}));
                disc.touch("EMPTY", false);
                assertArrayEquals(new byte[0], disc.readFile("EMPTY"));
                disc.touch("EMPTY", true);
                assertThrows(FileAlreadyExistsException.class, () -> disc.touch("EMPTY", false));
                assertTrue(disc.validate().isEmpty());
            }
    }

    @Test
    public void metadataValidationAndTitlePadding() throws Exception
    {
        for (DfsFormat format : List.of(DfsFormat.ACORN_DFS_40T_SINGLE_SIDED, DfsFormat.WATFORD_DFS_40T_SINGLE_SIDED))
            try (Dfs disc = Dfs.create(format))
            {
                disc.setTitle(format.watford ? "1234567890" : "123456789012");
                assertEquals(format.watford ? 10 : 12, disc.title().length());
                disc.setTitle("short");
                assertEquals("short", disc.title());
                int cycle = disc.info().cycleNumber();
                disc.setBootOption(2);
                assertEquals((cycle + 1) & 255, disc.info().cycleNumber());
                assertEquals(2, disc.info().bootOption());
                byte[] before = disc.toByteArray();
                assertThrows(IllegalArgumentException.class, () -> disc.setTitle("TITLE MUCH TOO LONG"));
                assertThrows(IllegalArgumentException.class, () -> disc.setTitle("bad\n"));
                assertThrows(IllegalArgumentException.class, () -> disc.setBootOption(4));
                assertArrayEquals(before, disc.toByteArray());
                for (int i = 0; i < 256; i++)
                    disc.setBootOption(i & 3);
                assertEquals((cycle + 1) & 255, disc.info().cycleNumber());
            }
    }

    @Test
    public void fullCatalogueCanOverwriteButNotAdd() throws Exception
    {
        for (DfsFormat format : List.of(DfsFormat.ACORN_DFS_40T_SINGLE_SIDED, DfsFormat.WATFORD_DFS_40T_SINGLE_SIDED))
            try (Dfs disc = Dfs.create(format, "LongTitle!"))
            {
                int max = format.watford ? 62 : 31;
                for (int i = 0; i < max; i++)
                    disc.writeFile("F" + i, new byte[]{(byte) i});
                assertEquals(max, disc.files().size());
                assertThrows(DfsCatalogueFullException.class, () -> disc.writeFile("EXTRA", new byte[0]));
                disc.writeFile("f0", new byte[]{99});
                assertEquals(max, disc.files().size());
                assertArrayEquals(new byte[]{99}, disc.readFile("F0"));
                assertEquals("LongTitle!", disc.title());
                assertTrue(disc.validate().toString(), disc.validate().isEmpty());
                try (Dfs reopened = Dfs.fromBuffer(ByteBuffer.wrap(disc.toByteArray()), format))
                {
                    for (int i = 1; i < max; i++)
                        assertArrayEquals(new byte[]{(byte) i}, reopened.readFile("F" + i));
                }
            }
    }

    @Test
    public void firstFitCompactionAndFailedOverwritePreserveFiles() throws Exception
    {
        try (Dfs disc = Dfs.create(DfsFormat.ACORN_DFS_40T_SINGLE_SIDED))
        {
            disc.writeFile("FIRST", payload(512));
            disc.writeFile("MIDDLE", payload(768));
            disc.writeFile("LAST", payload(1024));
            assertEquals(2, disc.stat("FIRST").startSector());
            disc.deleteFile("MIDDLE");
            disc.writeFile("GAP", new byte[512]);
            assertEquals(4, disc.stat("GAP").startSector());
            disc.setLocked("LAST", true);
            assertEquals(3, disc.compact(List.of("LAST", "last")));
            assertEquals(2, disc.stat("LAST").startSector());
            assertTrue(disc.stat("LAST").locked());
            assertArrayEquals(payload(1024), disc.readFile("LAST"));
            assertEquals(1, disc.freeMap().size());
            byte[] before = disc.toByteArray();
            assertThrows(NoSuchFileException.class, () -> disc.compact(List.of("MISSING")));
            assertThrows(DfsDiscFullException.class, () -> disc.writeFile("FIRST", new byte[102400]));
            assertArrayEquals(before, disc.toByteArray());
            disc.setTotalSectors(20);
            assertEquals(10, disc.freeSectors());
            assertThrows(DfsDiscFullException.class, () -> disc.writeFile("BIG", new byte[21 * 256]));
        }
    }

    @Test
    public void largeFilesZeroLengthOnFullDiscAndFragmentation() throws Exception
    {
        try (Dfs disc = Dfs.create(DfsFormat.ACORN_DFS_80T_SINGLE_SIDED))
        {
            byte[] large = payload(70001);
            disc.writeFile("BIG", large);
            assertEquals(70001, disc.stat("BIG").length());
            assertArrayEquals(large, disc.readFile("BIG"));
            disc.deleteFile("BIG");
            disc.writeFile("FULL", new byte[798 * 256]);
            assertEquals(0, disc.freeSectors());
            disc.writeFile("EMPTY", new byte[0]);
            assertEquals(0, disc.stat("EMPTY").length());
            assertThrows(DfsDiscFullException.class, () -> disc.writeFile("ONE", new byte[1]));
        }
        try (Dfs disc = Dfs.create(DfsFormat.ACORN_DFS_40T_SINGLE_SIDED))
        {
            disc.writeFile("FIRST", payload(60 * 256));
            disc.writeFile("MIDDLE", payload(60 * 256));
            disc.writeFile("LAST", payload(60 * 256));
            disc.deleteFile("MIDDLE");
            assertEquals(278, disc.freeSectors());
            assertThrows(DfsDiscFullException.class, () -> disc.writeFile("BIG", new byte[230 * 256]));
            disc.compact();
            disc.writeFile("BIG", new byte[230 * 256]);
            assertArrayEquals(payload(60 * 256), disc.readFile("LAST"));
        }
    }

    @Test
    public void truncatedAndRaggedBuffersArePadded() throws Exception
    {
        DfsFormat format = DfsFormat.ACORN_DFS_80T_SINGLE_SIDED;
        byte[] image;
        try (Dfs disc = Dfs.create(format))
        {
            disc.writeFile("HELLO", new byte[]{10, 11, 12});
            image = Arrays.copyOf(disc.toByteArray(), 515);
        }
        ByteBuffer buffer = ByteBuffer.wrap(image).asReadOnlyBuffer();
        try (Dfs disc = Dfs.fromBuffer(buffer, format))
        {
            assertEquals(0, buffer.position());
            assertEquals(format.size(), disc.toByteArray().length);
            assertArrayEquals(new byte[]{10, 11, 12}, disc.readFile("HELLO"));
            assertTrue(disc.isReadOnly());
            assertThrows(DfsReadOnlyException.class, () -> disc.writeFile("NEW", new byte[]{1}));
        }
        assertThrows(DfsFormatException.class, () -> Dfs.fromBuffer(ByteBuffer.allocate(0), format));
    }

    @Test
    public void hostMetadataExportImportAndText() throws Exception
    {
        try (Dfs disc = Dfs.create(DfsFormat.ACORN_DFS_80T_SINGLE_SIDED))
        {
            disc.writeFile("A.File", payload(789), new AcornMeta(0xffff1900L, 0xffff8023L, Access.L));
            Path host = temporary.getRoot().toPath().resolve("file");
            disc.exportFile("A.File", host);
            assertTrue(Files.exists(host.resolveSibling("file.inf")));
            disc.importFile("B.Copy", host);
            assertArrayEquals(payload(789), disc.readFile("B.Copy"));
            assertEquals(disc.stat("A.File").metadata(), disc.stat("B.Copy").metadata());
            disc.writeText("TEXT", "one\ntwo\n");
            assertEquals("one\ntwo\n", disc.readText("TEXT"));
        }
    }

    @Test
    public void fileCreationDetectionExpansionAndLifecycle() throws Exception
    {
        for (String extension : List.of("ssd", "dsd"))
        {
            Path path = temporary.getRoot().toPath().resolve("disc." + extension);
            try (Dfs disc = Dfs.createFile(path, "Title"))
            {
                disc.writeFile("HELLO", payload(1000));
            }
            assertThrows(FileAlreadyExistsException.class, () -> Dfs.createFile(path, "Other"));
            try (Dfs disc = Dfs.fromFile(path, true))
            {
                assertEquals("Title", disc.title());
                assertArrayEquals(payload(1000), disc.readFile("HELLO"));
                assertThrows(DfsReadOnlyException.class, () -> disc.setTitle("Other"));
            }
        }
        Path watford = temporary.getRoot().toPath().resolve("watford.ssd");
        try (Dfs disc = Dfs.createFile(watford, DfsFormat.WATFORD_DFS_80T_SINGLE_SIDED, "Watford"))
        {
            disc.writeFile("DATA", new byte[]{8});
        }
        try (Dfs disc = Dfs.fromFile(watford))
        {
            assertTrue(disc.format().watford);
            assertArrayEquals(new byte[]{8}, disc.readFile("DATA"));
        }
        Path shortImage = temporary.getRoot().toPath().resolve("short.ssd");
        try (Dfs disc = Dfs.create(DfsFormat.ACORN_DFS_80T_SINGLE_SIDED))
        {
            Files.write(shortImage, Arrays.copyOf(disc.toByteArray(), 515));
        }
        assertEquals(204800 - 515, Dfs.expand(shortImage, DfsFormat.ACORN_DFS_80T_SINGLE_SIDED));
        assertEquals(0, Dfs.expand(shortImage, DfsFormat.ACORN_DFS_80T_SINGLE_SIDED));
        Dfs closed = Dfs.fromFile(shortImage);
        closed.close();
        closed.close();
        assertTrue(closed.closed());
        assertThrows(DfsClosedException.class, () -> closed.readFile("HELLO"));
        assertThrows(DfsClosedException.class, closed::files);
    }
}
