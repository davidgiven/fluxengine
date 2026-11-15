package org.rob_smallshire.oaknut.dfs;

import static org.junit.Assert.assertArrayEquals;
import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertThrows;
import static org.junit.Assert.assertTrue;

import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;
import org.rob_smallshire.oaknut.discimage.DiscImage;
import org.rob_smallshire.oaknut.file.AcornMeta;
import org.rob_smallshire.oaknut.filesystem.ValidationFinding;
import java.nio.ByteBuffer;
import java.util.Arrays;
import java.util.List;

/* Copyright (c) 2026 Robert Smallshire. MIT license; see
 * ../../../../../../../java/org/rob_smallshire/oaknut/LICENSE.
 * Catalogue byte-layout and recognition scenarios translated from oaknut-dfs tests. */
@RunWith(JUnit4.class)
public class CatalogueTest
{
    private static Catalogue catalogue(byte[] bytes, boolean watford)
    {
        DiscImage image = new DiscImage(
                ByteBuffer.wrap(bytes),
                DfsFormat.ACORN_DFS_80T_SINGLE_SIDED.surfaces());
        return watford ?
                new WatfordDfsCatalogue(image.surface(0)) :
                new AcornDfsCatalogue(image.surface(0));
    }

    private static byte[] blank(boolean watford) throws Exception
    {
        try (Dfs disc = Dfs.create(
                watford ?
                        DfsFormat.WATFORD_DFS_80T_SINGLE_SIDED :
                        DfsFormat.ACORN_DFS_80T_SINGLE_SIDED, "ABCDEFGHIJ", 2))
        {
            return disc.toByteArray();
        }
    }

    @Test
    public void entryCodecMatchesHandAssembledCatalogue() throws Exception
    {
        byte[] bytes = blank(false);
        System.arraycopy(
                new byte[]{'H', 'e', 'l', 'l', 'o', ' ', ' ', (byte) ('A' | 128)},
                0,
                bytes,
                8,
                8);
        bytes[261] = 8;
        /* Load 0x31900, exec 0x28023, length 0x12345, sector 0x201. */
        System.arraycopy(
                new byte[]{0, 0x19, 0x23, (byte) 0x80, 0x45, 0x23, (byte) 0x9e, 1},
                0,
                bytes,
                264,
                8);
        Catalogue catalogue = catalogue(bytes, false);
        DfsFileEntry entry = catalogue.stat("a.HELLO");
        assertEquals("A.Hello", entry.name());
        assertTrue(entry.locked());
        assertEquals(0xffff1900L, entry.loadAddress());
        assertEquals(0x28023, entry.executionAddress());
        assertEquals(0x12345, entry.length());
        assertEquals(0x201, entry.startSector());
        byte[] raw = bytes.clone();
        catalogue.replaceEntries(List.of(entry));
        assertArrayEquals(Arrays.copyOfRange(raw, 8, 16), Arrays.copyOfRange(bytes, 8, 16));
        assertArrayEquals(Arrays.copyOfRange(raw, 264, 272), Arrays.copyOfRange(bytes, 264, 272));
    }

    @Test
    public void watfordMarkersTitlesAndIndependentSequenceNumbers() throws Exception
    {
        byte[] bytes = blank(true);
        assertArrayEquals(
                "ABCDEFGH".getBytes(java.nio.charset.StandardCharsets.US_ASCII),
                Arrays.copyOf(bytes, 8));
        assertEquals('I', bytes[256]);
        assertEquals('J', bytes[257]);
        for (int i = 512; i < 520; i++)
            assertEquals(0xaa, bytes[i] & 255);
        bytes[260] = 10;
        bytes[772] = 50;
        Catalogue catalogue = catalogue(bytes, true);
        for (int i = 0; i < 34; i++)
        {
            java.util.ArrayList<DfsFileEntry> files = new java.util.ArrayList<>(catalogue.files());
            files.add(new DfsFileEntry("$.F" + i, false, 0, 0, 1, 4 + i));
            catalogue.replaceEntries(files);
        }
        assertEquals(31 * 8, bytes[261] & 255);
        assertEquals(3 * 8, bytes[773] & 255);
        assertEquals(41, bytes[260] & 255);
        assertEquals(53, bytes[772] & 255);
        assertEquals("ABCDEFGHIJ", catalogue.info().title());
        assertTrue(catalogue.assess().reason(), catalogue.assess().matches());
        assertFalse(catalogue(bytes, false).assess().matches());
        assertEquals(0, bytes[258]);
        assertEquals(0, bytes[259]);
        assertEquals(0, bytes[768]);
        assertEquals(0, bytes[769]);
        assertEquals(0xaa, bytes[519] & 255);
        assertEquals('$', bytes[527]);
        catalogue.setBootOption(3);
        assertEquals(bytes[262], bytes[774]);
        assertEquals(bytes[263], bytes[775]);
    }

    @Test
    public void acornCatalogueIsDescendingByStartSector() throws Exception
    {
        try (Dfs disc = Dfs.create(DfsFormat.ACORN_DFS_80T_SINGLE_SIDED))
        {
            disc.writeFile("LOW", new byte[256]);
            disc.writeFile("HIGH", new byte[256]);
            assertEquals("$.HIGH", disc.files().get(0).name());
            assertEquals("$.LOW", disc.files().get(1).name());
        }
    }

    @Test
    public void malformedCountsHaveExplicitNonMutatingRepairs() throws Exception
    {
        for (int count : new int[]{3, 0, 1600, 10})
        {
            byte[] bytes = blank(false);
            Catalogue catalogue = catalogue(bytes, false);
            catalogue.replaceEntries(List.of(new DfsFileEntry("$.DATA", false, 0, 0, 256, 20)));
            bytes[262] = (byte) ((bytes[262] & 0x30) | (count >>> 8));
            bytes[263] = (byte) count;
            byte[] before = bytes.clone();
            assertTrue(catalogue.assess().reason(), catalogue.assess().matches());
            assertEquals(800, catalogue.info().totalSectors());
            List<ValidationFinding> findings = catalogue.validate();
            assertEquals(1, findings.size());
            assertTrue(findings.get(0).repairable());
            assertArrayEquals(before, bytes);
            findings.get(0).fix();
            assertTrue(catalogue.validate().isEmpty());
            assertEquals(0, bytes[262] & 12);
            assertEquals(2, catalogue.info().bootOption());
        }
    }

    @Test
    public void recognitionReportsReasonsAndRejectsForeignCatalogues() throws Exception
    {
        byte[] bytes = blank(false);
        assertTrue(catalogue(bytes, false).assess().matches());
        bytes[261] = 13;
        assertTrue(catalogue(bytes, false).assess().reason().contains("multiple of eight"));
        bytes = blank(false);
        bytes[262] |= (byte) 0x80;
        assertFalse(catalogue(bytes, false).assess().matches());
        bytes = blank(false);
        bytes[1] = (byte) 0x80;
        assertFalse(catalogue(bytes, false).assess().matches());
        bytes = blank(false);
        bytes[1] = 13;
        assertFalse(catalogue(bytes, false).assess().matches());
        Catalogue catalogue = catalogue(bytes, false);
        catalogue.replaceEntries(List.of(new DfsFileEntry("$.DATA", false, 0, 0, 256, 20)));
        bytes[1] = 13;
        assertTrue(catalogue.assess().reason(), catalogue.assess().matches());
        bytes = blank(true);
        bytes[773] = 1;
        assertFalse(catalogue(bytes, true).assess().matches());
        bytes = blank(true);
        bytes[774] ^= 0x10;
        assertFalse(catalogue(bytes, true).assess().matches());
        bytes = blank(true);
        Catalogue watford = catalogue(bytes, true);
        watford.replaceEntries(List.of(new DfsFileEntry("$.DATA", false, 0, 0, 256, 4)));
        bytes[13] |= (byte) 0x80;
        assertTrue(watford.assess().reason().contains(">256K"));
    }

    @Test
    public void overlapDuplicatesAndOutOfImageFilesAreReported() throws Exception
    {
        byte[] bytes = blank(false);
        Catalogue catalogue = catalogue(bytes, false);
        catalogue.replaceEntries(List.of(
                new DfsFileEntry("$.ONE", false, 0, 0, 256, 2),
                new DfsFileEntry("$.TWO", false, 0, 0, 256, 3)));
        /* Force two entries to share the same sector and name, bypassing write guards. */
        bytes[279] = bytes[271];
        System.arraycopy(bytes, 8, bytes, 16, 8);
        assertEquals(2, catalogue.validate().size());
        bytes[271] = (byte) 0xff;
        bytes[270] |= 3;
        assertTrue(catalogue.validate().stream().anyMatch(f -> f.message().contains("outside")));
    }

    @Test
    public void storedNonCommandDirectoryRemainsReadable() throws Exception
    {
        byte[] bytes = blank(false);
        Catalogue catalogue = catalogue(bytes, false);
        catalogue.replaceEntries(List.of(new DfsFileEntry("&.UTIL", false, 0, 0, 0, 2)));
        try (Dfs disc = Dfs.fromBuffer(
                ByteBuffer.wrap(bytes),
                DfsFormat.ACORN_DFS_80T_SINGLE_SIDED))
        {
            assertTrue(disc.exists("&.UTIL"));
            assertArrayEquals(new byte[0], disc.readFile("&.UTIL"));
            disc.writeFile("&.COPY", new byte[]{1, 2}, new AcornMeta(0x1900, 0, null), true);
            assertArrayEquals(new byte[]{1, 2}, disc.readFile("&.COPY"));
            assertThrows(
                    IllegalArgumentException.class,
                    () -> disc.writeFile(
                            "\u00e9.BAD",
                            new byte[]{1},
                            new AcornMeta(0, 0, null),
                            true));
            disc.renameFile("&.UTIL", "A.UTIL");
            assertFalse(disc.exists("&.UTIL"));
        }
    }

    @Test
    public void formatDetectionByExtensionAndSize() throws Exception
    {
        assertEquals(DfsFormat.ACORN_DFS_40T_SINGLE_SIDED, DfsFormat.detect("disc.ssd", 100000));
        assertEquals(DfsFormat.ACORN_DFS_80T_SINGLE_SIDED, DfsFormat.detect("disc.SSD", 200000));
        assertEquals(
                DfsFormat.ACORN_DFS_80T_DOUBLE_SIDED_INTERLEAVED,
                DfsFormat.detect("disc.dsd", 400000));
        assertThrows(DfsFormatException.class, () -> DfsFormat.detect("unknown.img", 100000));
        assertThrows(DfsFormatException.class, () -> DfsFormat.detect("large.ssd", 500000));
    }
}
