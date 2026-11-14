package org.rob_smallshire.oaknut.adfs;

import static org.junit.Assert.*;

import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;
import java.util.List;
import java.util.ArrayList;
import org.rob_smallshire.oaknut.discimage.SectorsView;
import org.rob_smallshire.oaknut.file.Access;

/* Copyright (c) 2026 Robert Smallshire. MIT license; see the oaknut LICENSE.
 * Directory serialization, format creation and map mutation cases from upstream's unit tests. */
@RunWith(JUnit4.class)
public class LayoutTest
{
    @Test
    public void everyOldAccessBitAndUnsignedAddressSurvivesSerialization() throws Exception
    {
        for (int bits = 0; bits < 256; bits++)
        {
            DirectoryEntry entry = new DirectoryEntry(
                    "Mixed",
                    0xffffffffL,
                    0xfedcba98L,
                    123456,
                    0xabcdef,
                    0x42,
                    bits,
                    true);
            Directory dir =
                    new Directory("Dir", "Two Word Title", 2, 7, List.of(entry), 0x99, "Hugo", 0);
            byte[] raw = DirectoryFormat.OLD.serialize(dir);
            Directory parsed = DirectoryFormat.OLD.parse(SectorsView.wrap(raw), 7);
            assertEquals(entry, parsed.entries().getFirst());
            assertEquals("Two Word Title", parsed.title());
            assertEquals(0, raw[1279]);
        }
    }

    @Test
    public void newDirectoriesPreserveHugoOrNickAndCheckTheirChecksum() throws Exception
    {
        for (String signature : List.of("Hugo", "Nick"))
        {
            DirectoryEntry entry = new DirectoryEntry(
                    "Name",
                    0xfff00001L,
                    0x12345678L,
                    1234,
                    0x123456,
                    0,
                    Access.LWR | Access.PR,
                    false);
            Directory dir = new Directory(
                    "Dir",
                    "Latin é",
                    0x345678,
                    123,
                    List.of(entry),
                    42,
                    signature,
                    0);
            byte[] bytes = DirectoryFormat.NEW.serialize(dir);
            Directory parsed = DirectoryFormat.NEW.parse(SectorsView.wrap(bytes), 123);
            assertEquals(signature, parsed.signature());
            assertEquals(entry, parsed.entries().getFirst());
            assertEquals(42, parsed.sequence());
            bytes[0x20] ^= 1;
            assertEquals(
                    1,
                    DirectoryFormat.NEW.parse(SectorsView.wrap(bytes), 123).entries().size());
            bytes[0x10] ^= 1;
            assertThrows(
                    AdfsDirectoryException.class,
                    () -> DirectoryFormat.NEW.parse(SectorsView.wrap(bytes), 123));
        }
    }

    @Test
    public void bigNamesUseHeapAndVaryingDirectorySizes() throws Exception
    {
        List<DirectoryEntry> entries = new ArrayList<>();
        for (int i = 0; i < 100; i++)
            entries.add(new DirectoryEntry(
                    "Name " + i + "é".repeat(20),
                    i,
                    i + 1,
                    i,
                    i << 8,
                    0,
                    Access.WR,
                    false));
        Directory dir = new Directory(
                "Long Directory Name",
                "",
                0x12345678,
                0x345678,
                entries,
                7,
                "SBPr",
                8192);
        byte[] raw = DirectoryFormat.BIG.serialize(dir);
        Directory parsed = DirectoryFormat.BIG.parse(SectorsView.wrap(raw), dir.discAddress());
        assertEquals(entries, parsed.entries());
        assertEquals(dir.name(), parsed.name());
        assertEquals(8192, parsed.bigSize());
        assertEquals(dir.parentAddress(), parsed.parentAddress());
        SectorsView.wrap(raw).writeLe(28 + ((dir.name().length() + 4) & ~3) + 24, 0xffffffffL, 4);
        assertThrows(
                AdfsDirectoryException.class,
                () -> DirectoryFormat.BIG.parse(SectorsView.wrap(raw), 0));
    }

    @Test
    public void serializationFailureNeverTouchesDestination() throws Exception
    {
        byte[] raw = new byte[1280];
        java.util.Arrays.fill(raw, (byte) 0xa5);
        Directory dir = new Directory("Dir", "CAFÉ", 2, 7, List.of(), 0, "Hugo", 0);
        byte[] before = raw.clone();
        assertThrows(
                IllegalArgumentException.class,
                () -> DirectoryFormat.OLD.serialize(dir, SectorsView.wrap(raw)));
        assertArrayEquals(before, raw);
    }

    @Test
    public void fragmentedObjectsSpanHolesAndZones() throws Exception
    {
        for (AdfsFormat format : List.of(AdfsFormat.E, AdfsFormat.F, AdfsFormat.G))
            try (Adfs adfs = Adfs.create(format))
            {
                for (int i = 0; i < 20; i++)
                    adfs.path(String.format("F%02d", i)).writeBytes(new byte[4000]);
                for (int i = 0; i < 20; i += 2)
                    adfs.path(String.format("F%02d", i)).unlink();
                byte[] payload = new byte[format == AdfsFormat.E ? 30000 : 600000];
                for (int i = 0; i < payload.length; i++)
                    payload[i] = (byte) (i * 11);
                adfs.path("Huge").writeBytes(payload);
                assertArrayEquals(payload, adfs.path("Huge").readBytes());
                assertTrue(adfs.validate().toString(), adfs.validate().isEmpty());
                adfs.path("Huge").unlink();
                assertTrue(adfs.validate().isEmpty());
            }
    }

    @Test
    public void oldMapFullFailureLeavesItsBytesUnchanged() throws Exception
    {
        try (Adfs blank = Adfs.create(AdfsFormat.S))
        {
            SectorsView data = SectorsView.wrap(blank.toByteArray()).slice(0, 512);
            OldFreeSpaceMap map = new OldFreeSpaceMap(data);
            List<OldFreeSpaceMap.Extent> entries = new ArrayList<>();
            for (int i = 0; i < 82; i++)
                entries.add(new OldFreeSpaceMap.Extent(8 + i * 4, 2));
            map.replaceEntries(entries);
            byte[] before = data.toByteArray();
            assertThrows(AdfsMapException.class, () -> map.free(600, 1));
            assertArrayEquals(before, data.toByteArray());
            map.setBootOption(3);
            assertEquals(3, map.bootOption());
            assertTrue(map.validate().isEmpty());
        }
    }
}
