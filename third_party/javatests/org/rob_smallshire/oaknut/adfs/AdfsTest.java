package org.rob_smallshire.oaknut.adfs;

import static org.junit.Assert.*;

import org.junit.Test;
import org.junit.Rule;
import org.junit.rules.TemporaryFolder;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;
import java.nio.ByteBuffer;
import java.nio.file.Files;
import java.nio.file.Path;
import java.time.Instant;
import java.util.Arrays;
import java.util.List;
import org.rob_smallshire.oaknut.file.Access;
import org.rob_smallshire.oaknut.file.AcornMeta;
import org.rob_smallshire.oaknut.discimage.SectorsView;

/* Copyright (c) 2026 Robert Smallshire. MIT license; see
 * ../../../../../../../java/org/rob_smallshire/oaknut/LICENSE. Ported floppy scenarios from
 * oaknut-adfs's pytest suite. */
@RunWith(JUnit4.class)
public class AdfsTest
{
    @Rule public TemporaryFolder temporary = new TemporaryFolder();

    @Test
    public void allFloppyFormatsCreateReopenReadWrite() throws Exception
    {
        for (AdfsFormat format : AdfsFormat.values())
        {
            try (Adfs adfs = Adfs.create(format, "Title"))
            {
                assertEquals(format.toString(), format.size(), adfs.totalSize());
                assertEquals("Title", adfs.title());
                assertTrue(adfs.root().list().isEmpty());
                byte[] bytes = new byte[5000];
                for (int i = 0; i < bytes.length; i++)
                    bytes[i] = (byte) i;
                adfs
                        .path("$.File")
                        .writeBytes(bytes, new AcornMeta(0xffff1900L, 0x8023, Access.WR));
                assertArrayEquals(format.toString(), bytes, adfs.path("$.file").readBytes());
                assertEquals(0xffff1900L, adfs.path("$.File").stat().loadAddress());
                assertTrue(format + ": " + adfs.validate(), adfs.validate().isEmpty());
                try (Adfs reopened = Adfs.fromBuffer(
                        ByteBuffer.wrap(adfs.toByteArray()),
                        format == AdfsFormat.L_SEQUENTIAL))
                {
                    assertArrayEquals(
                            format.toString(),
                            bytes,
                            reopened.path("$.File").readBytes());
                    assertEquals("Title", reopened.title());
                }
            }
        }
    }

    @Test
    public void floppyGeometryAndFileExtensionDefaults() throws Exception
    {
        for (AdfsFormat format : AdfsFormat.values())
        {
            try (Adfs adfs = Adfs.create(format))
            {
                assertEquals(format.size() / 256, adfs.geometry().totalSectors());
                assertEquals(format.heads, adfs.geometry().heads());
                assertEquals(format.tracks, adfs.geometry().cylinders());
                assertEquals(
                        format.sectorsPerTrack * format.sectorSize / 256,
                        adfs.geometry().sectorsPerTrack());
            }
        }
        for (String extension : List.of("ads", "adm", "adl"))
        {
            Path file = temporary.getRoot().toPath().resolve("disc." + extension);
            try (Adfs adfs = Adfs.createFile(file))
            {
                assertEquals(AdfsFormat.forExtension(extension), adfs.format());
            }
            assertThrows(
                    java.nio.file.FileAlreadyExistsException.class,
                    () -> Adfs.createFile(file));
        }
        assertThrows(
                AdfsFormatException.class,
                () -> Adfs.createFile(temporary.getRoot().toPath().resolve("ambiguous.adf")));
        Path explicit = temporary.getRoot().toPath().resolve("interleave.adf");
        try (Adfs adfs = Adfs.createFile(explicit, AdfsFormat.L, "Title", 3))
        {
            assertEquals(AdfsFormat.L, adfs.format());
            assertEquals(3, adfs.bootOption());
        }
    }

    @Test
    public void missingPathsAreFalseQueriesAndRootMetadataIsCanonical() throws Exception
    {
        try (Adfs adfs = Adfs.create(AdfsFormat.S))
        {
            assertFalse(adfs.path("Missing").isDirectory());
            assertFalse(adfs.path("Missing").isFile());
            assertEquals(Access.WR | Access.PR, adfs.root().stat().access());
            assertThrows(AdfsPathException.class, () -> adfs.root().writeBytes(new byte[0]));
        }
    }

    @Test
    public void directoryOperationsAndCasePreservingSort() throws Exception
    {
        for (AdfsFormat format : AdfsFormat.values())
        {
            try (Adfs adfs = Adfs.create(format))
            {
                adfs.path("$.Dir.Sub").mkdir(true, false);
                adfs.path("$.Dir.Sub.zebra").writeBytes(new byte[]{3});
                adfs.path("$.Dir.Sub.Alpha").writeBytes(new byte[]{1});
                adfs.path("$.Dir.Sub.middle").writeBytes(new byte[]{2});
                assertEquals(
                        List.of("Alpha", "middle", "zebra"),
                        adfs.path("$.DIR.SUB").list().stream().map(AdfsPath::name).toList());
                adfs.path("$.Dir.Sub.zebra").rename("$.Dir.Sub.Beta");
                assertEquals(
                        List.of("Alpha", "Beta", "middle"),
                        adfs.path("$.Dir.Sub").list().stream().map(AdfsPath::name).toList());
                adfs.path("$.Dir.Sub.Beta").rename("$.Other");
                assertArrayEquals(new byte[]{3}, adfs.path("$.Other").readBytes());
                adfs.path("$.Other").unlink();
                assertFalse(adfs.path("$.Other").exists());
                assertThrows(
                        AdfsDirectoryNotEmptyException.class,
                        () -> adfs.path("$.Dir").rmdir());
                adfs.path("$.Dir.Sub.Alpha").unlink();
                adfs.path("$.Dir.Sub.middle").unlink();
                adfs.path("$.Dir.Sub").rmdir();
                adfs.path("$.Dir").rmdir();
                assertTrue(adfs.root().list().isEmpty());
                assertTrue(adfs.validate().toString(), adfs.validate().isEmpty());
            }
        }
    }

    @Test
    public void failedWritesAndLocksPreserveImage() throws Exception
    {
        for (AdfsFormat format : AdfsFormat.values())
        {
            try (Adfs adfs = Adfs.create(format))
            {
                AdfsPath path = adfs.path("$.Locked");
                path.writeBytes(new byte[]{1}, new AcornMeta(0, 0, Access.LWR));
                byte[] snapshot = adfs.toByteArray();
                assertThrows(AdfsFileLockedException.class, path::unlink);
                assertThrows(AdfsFileLockedException.class, () -> path.writeBytes(new byte[20]));
                assertArrayEquals(snapshot, adfs.toByteArray());
                path.unlock();
                snapshot = adfs.toByteArray();
                assertThrows(
                        AdfsDiscFullException.class,
                        () -> adfs.path("$.Huge").writeBytes(new byte[format.size()]));
                assertArrayEquals(snapshot, adfs.toByteArray());
                assertArrayEquals(new byte[]{1}, path.readBytes());
            }
        }
    }

    @Test
    public void namesAndTitlesKeepUpstreamRules() throws Exception
    {
        try (Adfs adfs = Adfs.create(AdfsFormat.S, "Before"))
        {
            for (String name : List.of("GAME*", "A#B", "MAP&1", "MY GAME", "MixedCase"))
            {
                adfs.path(name).writeBytes(new byte[]{1});
                assertTrue(adfs.path(name).exists());
            }
            assertArrayEquals(new byte[]{1}, adfs.path("mixedcase").readBytes());
            for (String name : List.of("ELEVENCHARS", "A\u00aaB", "A\rB", "A:B"))
                assertThrows(
                        name,
                        AdfsPathException.class,
                        () -> adfs.path(name).writeBytes(new byte[]{1}));
            byte[] before = adfs.toByteArray();
            assertThrows(IllegalArgumentException.class, () -> adfs.setTitle("CAFÉ"));
            assertThrows(
                    IllegalArgumentException.class,
                    () -> adfs.setTitle("Title that is too long"));
            assertArrayEquals(before, adfs.toByteArray());
            adfs.setTitle("Multi Word Title");
            assertEquals("Multi Word Title", adfs.title());
            adfs.setBootOption(3);
            assertEquals(3, adfs.bootOption());
        }
    }

    @Test
    public void fileBackedReadOnlyAndCloseLifecycle() throws Exception
    {
        Path image = temporary.getRoot().toPath().resolve("disc.ads");
        AdfsPath surviving;
        try (Adfs adfs = Adfs.createFile(image, AdfsFormat.S, "Persistent"))
        {
            surviving = adfs.path("$.File");
            surviving.writeText("Hello\nWorld\n");
            adfs.flush();
            try (Adfs reader = Adfs.fromFile(image, true))
            {
                assertEquals("Hello\nWorld\n", reader.path("$.File").readText());
                assertThrows(AdfsReadOnlyException.class, () -> reader.path("$.New").touch());
            }
        }
        assertEquals("File", surviving.name());
        assertThrows(AdfsClosedException.class, surviving::readBytes);
        try (Adfs adfs = Adfs.fromFile(image))
        {
            assertEquals("Persistent", adfs.title());
        }
    }

    @Test
    public void bigDirectoriesGrowAndMove() throws Exception
    {
        for (AdfsFormat format : List.of(AdfsFormat.E_PLUS, AdfsFormat.F_PLUS, AdfsFormat.G_PLUS))
        {
            try (Adfs adfs = Adfs.create(format))
            {
                adfs.path("$.Directory with a long name").mkdir();
                for (int i = 0; i < 100; i++)
                    adfs
                            .path(String.format("$.Directory with a long name.File number %03d", i))
                            .writeBytes(new byte[]{(byte) i});
                assertEquals(100, adfs.path("$.Directory with a long name").list().size());
                for (int i = 0; i < 100; i++)
                    assertArrayEquals(
                            new byte[]{(byte) i},
                            adfs
                                    .path(String.format(
                                            "$.Directory with a long name.File number %03d",
                                            i))
                                    .readBytes());
                try (Adfs reopened = Adfs.fromBuffer(adfs.toByteArray()))
                {
                    assertEquals(100, reopened.path("$.Directory with a long name").list().size());
                }
            }
        }
    }

    @Test
    public void sharingDeletionDoesNotFreeSibling() throws Exception
    {
        for (AdfsFormat format : List.of(AdfsFormat.E, AdfsFormat.F, AdfsFormat.G))
        {
            try (Adfs adfs = Adfs.create(format))
            {
                int free = adfs.freeSpace();
                adfs.path("$.One").writeBytes(new byte[]{1});
                adfs.path("$.Two").writeBytes(new byte[]{2});
                if (format == AdfsFormat.E)
                    assertEquals(
                            adfs.path("$.One").stat().discAddress() >>> 8,
                            adfs.path("$.Two").stat().discAddress() >>> 8);
                else
                    assertNotEquals(
                            adfs.path("$.One").stat().discAddress() >>> 8,
                            adfs.path("$.Two").stat().discAddress() >>> 8);
                adfs.path("$.One").unlink();
                assertArrayEquals(new byte[]{2}, adfs.path("$.Two").readBytes());
                adfs.path("$.Two").unlink();
                assertEquals(free, adfs.freeSpace());
            }
        }
    }

    @Test
    public void oldMapAllocateAlignMergeAndChecksums() throws Exception
    {
        try (Adfs adfs = Adfs.create(AdfsFormat.D))
        {
            byte[] image = adfs.toByteArray();
            OldFreeSpaceMap map = new OldFreeSpaceMap(SectorsView.wrap(image).slice(0, 512));
            int original = map.freeSpace();
            int first = map.allocate(1);
            int aligned = map.allocate(8, 4, image.length / 256);
            assertEquals(0, aligned % 4);
            map.free(first, 1);
            map.free(aligned, 8);
            assertEquals(original, map.freeSpace());
            assertEquals(1, map.entries().size());
            assertTrue(map.validate().isEmpty());
            map.installAfsPointers(1234, 1240);
            assertArrayEquals(new long[]{1234, 1240}, map.afsInfoPointers());
            assertTrue(map.validate().isEmpty());
        }
    }

    @Test
    public void accessAndTimestampRoundTrip() throws Exception
    {
        try (Adfs adfs = Adfs.create(AdfsFormat.E))
        {
            AdfsPath file = adfs.path("$.Typed");
            file.touch();
            file.setFiletype(0xffd);
            Instant time = Instant.parse("2024-02-03T12:34:56.780Z");
            file.setDatestamp(time);
            assertEquals(Integer.valueOf(0xffd), file.filetype());
            assertEquals(time, file.datestamp());
            file.chmod(Access.R | Access.PR | Access.E | Access.PL);
            assertEquals(Access.R | Access.PR, file.stat().access());
        }
    }

    @Test
    public void compactPreservesContentMetadataAndTitles() throws Exception
    {
        try (Adfs adfs = Adfs.create(AdfsFormat.S, "Title"))
        {
            adfs.path("$.Dir").mkdir();
            adfs.path("$.Dir").setTitle("Directory");
            adfs.path("$.A").writeBytes(new byte[512]);
            adfs.path("$.B").writeBytes(new byte[]{5});
            adfs.path("$.A").unlink();
            adfs
                    .path("$.Dir.File")
                    .writeBytes(new byte[]{9}, new AcornMeta(0x1900, 0x8023, Access.LWR));
            assertEquals(3, adfs.compact());
            assertArrayEquals(new byte[]{9}, adfs.path("$.Dir.File").readBytes());
            assertTrue(adfs.path("$.Dir.File").stat().locked());
            assertEquals("Directory", adfs.path("$.Dir").title());
            assertTrue(adfs.validate().toString(), adfs.validate().isEmpty());
        }
    }
}
