package org.rob_smallshire.oaknut.file;

import static org.junit.Assert.assertArrayEquals;
import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertNull;
import static org.junit.Assert.assertThrows;
import static org.junit.Assert.assertTrue;

import org.junit.Rule;
import org.junit.Test;
import org.junit.rules.TemporaryFolder;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;
import org.rob_smallshire.oaknut.adfs.Adfs;
import org.rob_smallshire.oaknut.adfs.AdfsFormat;
import org.rob_smallshire.oaknut.adfs.AdfsPath;
import java.nio.file.Files;
import java.nio.file.Path;
import java.time.Instant;
import java.util.List;

/* Copyright (c) 2026 Robert Smallshire. MIT license; see the oaknut LICENSE.
 * Ported metadata, text-I/O, access and host-bridge scenarios. */
@RunWith(JUnit4.class)
public class FileUtilitiesTest
{
    @Rule public TemporaryFolder temporary = new TemporaryFolder();

    @Test
    public void accessConventionsRoundTripAllRepresentableBits()
    {
        for (int value = 0; value < 256; value++)
        {
            assertEquals(value, Access.parse(Access.format(value)));
            int pieb = HostBridge.toPieb(value);
            assertEquals(pieb, HostBridge.toPieb(HostBridge.fromPieb(pieb)));
        }
        assertEquals(Access.LWR | Access.PR, Access.parse("LWR/R"));
        assertTrue(Access.isRunOnly(Access.E));
        assertFalse(Access.isRunOnly(Access.E | Access.R));
        assertThrows(IllegalArgumentException.class, () -> Access.parse("X"));
    }

    @Test
    public void datestampsPreserveTypeAndCentiseconds()
    {
        Instant when = Instant.parse("2024-02-03T12:34:56.789Z");
        AcornMeta meta = new AcornMeta(0, 0, Access.WR).withDatestamp(when);
        assertEquals(Integer.valueOf(0xffd), meta.filetype());
        assertEquals(Instant.parse("2024-02-03T12:34:56.780Z"), meta.datestamp());
        assertEquals(meta.datestamp(), meta.withFiletype(0xfff).datestamp());
        assertEquals(Integer.valueOf(0xfff), meta.withFiletype(0xfff).filetype());
        assertNull(new AcornMeta(0xffff1900L, 0xffff1900L, null).datestamp());
        assertNull(new AcornMeta(0xffffffffL, 0xffffffffL, null).filetype());
        assertEquals(0, Datestamp.encode(Datestamp.EPOCH));
        assertThrows(
                IllegalArgumentException.class,
                () -> Datestamp.encode(Datestamp.EPOCH.minusMillis(1)));
        assertThrows(
                IllegalArgumentException.class,
                () -> Datestamp.encode(Datestamp.EPOCH.plusMillis((1L << 40) * 10)));
    }

    @Test
    public void traditionalAndPiebInfParseAndFormat()
    {
        AcornMeta meta = new AcornMeta(0xffff1900L, 0x8023, Access.LWR | Access.PR);
        assertEquals(meta, HostBridge.parseInf(HostBridge.formatInf("Code", meta, 123)));
        assertEquals(meta, HostBridge.parseInf(HostBridge.formatPiebInf(meta, 42)));
        assertEquals(
                0x33,
                HostBridge.parseInf("Code 00001900 00008023 00000100").access().intValue());
        assertEquals(0x19, HostBridge.parseInf("Code 001900 008023 L").access().intValue());
        assertEquals(0xffff1900L, HostBridge.parseInf("Code FF1900 FF8023").loadAddress());
        assertEquals(0x1900L, HostBridge.parseInf("Code 1900").executionAddress());
        assertEquals(Access.PR, HostBridge.parseInf("Code r").access().intValue());
        assertEquals(meta, HostBridge.parseInf(HostBridge.formatInf("File with spaces", meta, 0)));
        assertEquals(meta, HostBridge.parseInf(HostBridge.formatInf("TAPE", meta, 0)));
        assertTrue(HostBridge
                .formatInf("File with spaces", meta, 0)
                .startsWith("\"File with spaces\""));
        assertNull(HostBridge.parseInf("invalid"));
        assertNull(HostBridge.parseInf("\"unterminated 1900 1900"));
    }

    @Test
    public void filenameEncodingPreservesAddressesAndTypedChoice()
    {
        AcornMeta literal = new AcornMeta(0xffff1900L, 0xffff1900L, null);
        assertEquals(",ffff1900-ffff1900", HostBridge.filenameSuffix(literal, false));
        assertEquals(literal, HostBridge.parseFilename("Code,ffff1900-ffff1900"));
        assertEquals(literal, HostBridge.parseFilename("Code,ffff1900ffff1900"));
        AcornMeta typed = new AcornMeta(0, 0, null).withFiletype(0xffd);
        assertEquals(",ffd", HostBridge.filenameSuffix(typed, true));
        assertEquals(typed, HostBridge.parseFilename("Data,ffd"));
        assertNull(HostBridge.parseFilename("Plain"));
    }

    @Test
    public void acornTextGlyphsAndUniversalNewlines()
    {
        assertArrayEquals(new byte[]{0x60, 0x7c, 13}, TextIo.encode("£¦\n", "\r"));
        assertEquals(
                "£¦\na\nb\nc",
                TextIo.decode(new byte[]{0x60, 0x7c, 13, 10, 'a', 13, 'b', 10, 'c'}, true));
        assertEquals("\r\n", TextIo.decode(new byte[]{13, 10}, false));
        assertThrows(IllegalArgumentException.class, () -> TextIo.encode("Ā", "\r"));
    }

    @Test
    public void everyPortableHostFormatRoundTripsThroughAdfs() throws Exception
    {
        for (MetaFormat format : List.of(
                MetaFormat.INF_TRAD,
                MetaFormat.INF_PIEB,
                MetaFormat.FILENAME_RISCOS,
                MetaFormat.FILENAME_LOADEXEC))
        {
            try (Adfs source = Adfs.create(AdfsFormat.S); Adfs dest = Adfs.create(AdfsFormat.S))
            {
                AdfsPath file = source.path("$.Code");
                file.writeBytes(
                        new byte[]{1, 2, 3},
                        new AcornMeta(0x1900, 0x8023, Access.WR | Access.PR));
                Path output = temporary.getRoot().toPath().resolve(format.name()).resolve("Code");
                Path written = file.exportFile(output, format);
                AdfsPath imported = dest.path("$.Other");
                imported.importFile(written, List.of(format));
                assertArrayEquals(file.readBytes(), imported.readBytes());
                assertEquals(0x1900, imported.stat().loadAddress());
                assertEquals(0x8023, imported.stat().executionAddress());
            }
        }
    }

    @Test
    public void sidecarPrecedenceCaseInsensitivityAndDataOnlyExport() throws Exception
    {
        Path file = temporary.getRoot().toPath().resolve("Data,fff");
        Files.write(file, new byte[]{5});
        Files.writeString(
                file.resolveSibling("Data,fff.INF"),
                "ORIGINAL 00001900 00008023 00000001 1B\n");
        HostBridge.Imported imported = HostBridge.importFile(file);
        assertEquals(0x1900L, imported.metadata().loadAddress());
        assertEquals(Integer.valueOf(0x1b), imported.metadata().access());
        assertEquals(
                Integer.valueOf(0xfff),
                HostBridge
                        .importFile(file, List.of(MetaFormat.FILENAME_RISCOS))
                        .metadata()
                        .filetype());
        Path dataOnly = temporary.getRoot().toPath().resolve("Plain");
        HostBridge.exportFile(dataOnly, "Plain", new byte[]{7}, new AcornMeta(0, 0, null), null);
        assertFalse(Files.exists(dataOnly.resolveSibling("Plain.inf")));
        assertArrayEquals(new byte[]{7}, HostBridge.importFile(dataOnly).bytes());
    }

    @Test
    public void xattrsRoundTripWhenSupported() throws Exception
    {
        Path file = temporary.newFile().toPath();
        org.junit.Assume.assumeTrue(Files.getFileStore(file).supportsFileAttributeView("user"));
        AcornMeta expected = new AcornMeta(0xffff1900L, 0x8023, Access.LWR | Access.PR);
        HostBridge.writeXattrs(file, expected, false, 0);
        assertEquals(expected, HostBridge.readXattrs(file, false));
        Path pieb = temporary.newFile().toPath();
        HostBridge.writeXattrs(pieb, expected, true, 12);
        assertEquals(expected, HostBridge.readXattrs(pieb, false));
    }
}
