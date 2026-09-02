package com.cowlark.fluxengine.vfs;

import static com.google.common.truth.Truth.assertThat;
import static org.junit.Assert.assertThrows;

import com.cowlark.fluxengine.core.ByteReader;
import com.cowlark.fluxengine.core.ByteWriter;
import com.cowlark.fluxengine.core.Bytes;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;
import java.nio.charset.StandardCharsets;

@RunWith(JUnit4.class)
public class AppleSingleTest
{
    @Test
    public void overheadConstantIs0x5e()
    {
        assertThat(AppleSingle.OVERHEAD).isEqualTo(0x5e);
    }

    @Test
    public void invalidFileExceptionIsIllegalArgumentException()
    {
        assertThat(AppleSingle.InvalidFileException.class.getSuperclass()).isEqualTo(
                IllegalArgumentException.class);
    }

    @Test
    public void renderAndParseRoundTrip()
    {
        AppleSingle original = new AppleSingle();
        original.data = new Bytes("Hello data".getBytes(StandardCharsets.UTF_8));
        original.rsrc = new Bytes("Resource fork".getBytes(StandardCharsets.UTF_8));
        original.type = new Bytes("BLAT".getBytes(StandardCharsets.UTF_8));
        original.creator = new Bytes("FLOP".getBytes(StandardCharsets.UTF_8));

        Bytes rendered = original.render();
        assertThat(rendered.size()).isEqualTo(
                AppleSingle.OVERHEAD + original.data.size() + original.rsrc.size());

        AppleSingle parsed = new AppleSingle();
        parsed.parse(rendered);

        assertThat(parsed.data).isEqualTo(original.data);
        assertThat(parsed.rsrc).isEqualTo(original.rsrc);
        assertThat(parsed.type).isEqualTo(original.type);
        assertThat(parsed.creator).isEqualTo(original.creator);
    }

    @Test
    public void emptyForksRoundTrip()
    {
        AppleSingle original = new AppleSingle();
        // defaults are empty
        Bytes rendered = original.render();
        assertThat(rendered.size()).isEqualTo(AppleSingle.OVERHEAD);

        AppleSingle parsed = new AppleSingle();
        parsed.parse(rendered);
        assertThat(parsed.data.size()).isEqualTo(0);
        assertThat(parsed.rsrc.size()).isEqualTo(0);
        assertThat(parsed.type.size()).isEqualTo(4);
        assertThat(parsed.creator.size()).isEqualTo(4);
        // empty type/creator render as zero bytes, parsed back as 4 zero bytes
        assertThat(parsed.type.toByteArray()).isEqualTo(new byte[4]);
        assertThat(parsed.creator.toByteArray()).isEqualTo(new byte[4]);
    }

    @Test
    public void typeCreatorPaddingAndTruncation()
    {
        AppleSingle as = new AppleSingle();
        as.type = new Bytes("AB".getBytes(StandardCharsets.UTF_8)); // 2 bytes -> padded to 4 on
        // render
        as.creator =
                new Bytes("TOOLONG".getBytes(StandardCharsets.UTF_8)); // 7 bytes -> truncated to 4
        as.data = new Bytes("d".getBytes(StandardCharsets.UTF_8));
        as.rsrc = new Bytes();

        Bytes rendered = as.render();
        AppleSingle parsed = new AppleSingle();
        parsed.parse(rendered);

        assertThat(parsed.type.toByteArray()).isEqualTo(new byte[]{'A', 'B', 0, 0});
        assertThat(parsed.creator.toByteArray()).isEqualTo(new byte[]{'T', 'O', 'O', 'L'});
    }

    @Test
    public void invalidMagicThrows()
    {
        AppleSingle as = new AppleSingle();
        as.data = new Bytes("x".getBytes(StandardCharsets.UTF_8));
        Bytes rendered = as.render();
        // corrupt magic
        rendered.setByte(0, 0xFF);
        AppleSingle parsed = new AppleSingle();
        assertThrows(AppleSingle.InvalidFileException.class, () -> parsed.parse(rendered));
        assertThrows(IllegalArgumentException.class, () -> parsed.parse(rendered));
    }

    @Test
    public void invalidVersionThrows()
    {
        AppleSingle as = new AppleSingle();
        as.data = new Bytes("x".getBytes(StandardCharsets.UTF_8));
        Bytes rendered = as.render();
        // version is at offset 4, big-endian 0x00020000; set to 0x00030000
        ByteWriter bw = new ByteWriter(rendered);
        bw.seek(4);
        bw.writeBe32(0x00030000);
        AppleSingle parsed = new AppleSingle();
        assertThrows(AppleSingle.InvalidFileException.class, () -> parsed.parse(rendered));
    }

    @Test
    public void unknownEntryIsIgnored()
    {
        // Build a custom AppleSingle with an extra unknown entry type 99
        Bytes custom = new Bytes();
        ByteWriter bw = new ByteWriter(custom);
        bw.writeBe32(0x00051600);
        bw.writeBe32(0x00020000);
        bw.pad(16);
        bw.writeBe16(4); // 4 entries
        // finder info
        bw.writeBe32(9);
        bw.writeBe32(0x3e + 12); // shift by one extra entry (12 bytes header)
        bw.writeBe32(32);
        // data
        bw.writeBe32(1);
        bw.writeBe32(0x5e + 12);
        bw.writeBe32(5);
        // rsrc
        bw.writeBe32(2);
        bw.writeBe32(0x5e + 12 + 5);
        bw.writeBe32(3);
        // unknown
        bw.writeBe32(99);
        bw.writeBe32(0x5e + 12 + 5 + 3);
        bw.writeBe32(2);

        // bodies - need to account for shifted offsets
        // compute actual offsets: header 26 + 4*12 = 74 = 0x4a, so finder at 0x4a
        Bytes finder = new Bytes(32);
        ByteWriter fbw = new ByteWriter(finder);
        fbw.write("TYPE".getBytes(StandardCharsets.UTF_8));
        fbw.write("CREA".getBytes(StandardCharsets.UTF_8));
        fbw.pad(24);
        bw.write(finder);
        bw.write("hello".getBytes(StandardCharsets.UTF_8));
        bw.write("bye".getBytes(StandardCharsets.UTF_8));
        bw.write("ZZ".getBytes(StandardCharsets.UTF_8));

        // But easier: just test that our normal render with an appended unknown entry doesn't break
        // Simpler: take normal rendered and inject unknown entry via rebuilding with correct
        // offsets
        // For this test, manually construct minimal valid file with unknown entry and verify
        // parse succeeds
        Bytes simple = new Bytes();
        ByteWriter sw = new ByteWriter(simple);
        sw.writeBe32(0x00051600);
        sw.writeBe32(0x00020000);
        sw.pad(16);
        sw.writeBe16(2);
        sw.writeBe32(99); // unknown
        sw.writeBe32(26 + 2 * 12); // offset of unknown data = 50
        sw.writeBe32(4); // len
        sw.writeBe32(1); // data fork
        sw.writeBe32(26 + 2 * 12 + 4); // offset 54
        sw.writeBe32(3);
        sw.write("test".getBytes(StandardCharsets.UTF_8)); // unknown body
        sw.write("abc".getBytes(StandardCharsets.UTF_8)); // data fork
        AppleSingle parsed = new AppleSingle();
        parsed.parse(simple);
        assertThat(parsed.data.toByteArray()).isEqualTo("abc".getBytes(StandardCharsets.UTF_8));
        assertThat(parsed.rsrc.size()).isEqualTo(0);
    }

    @Test
    public void parseAndRenderWithKnownBytes()
    {
        // Verify byte-level layout matches C++ render: magic, version, 16 zero, 3 entries,
        // finder, data, rsrc
        AppleSingle as = new AppleSingle();
        as.type = new Bytes("TEXT".getBytes(StandardCharsets.UTF_8));
        as.creator = new Bytes("ttxt".getBytes(StandardCharsets.UTF_8));
        as.data = new Bytes("data".getBytes(StandardCharsets.UTF_8));
        as.rsrc = new Bytes("rsrc".getBytes(StandardCharsets.UTF_8));
        Bytes rendered = as.render();
        ByteReader br = new ByteReader(rendered);
        assertThat(br.readBe32()).isEqualTo(0x00051600);
        assertThat(br.readBe32()).isEqualTo(0x00020000);
        // 16 bytes padding
        for (int i = 0; i < 16; i++)
            assertThat(br.read8()).isEqualTo(0);
        assertThat(br.readBe16()).isEqualTo(3);
        // finder entry
        assertThat(br.readBe32()).isEqualTo(9);
        assertThat(br.readBe32()).isEqualTo(0x3e);
        assertThat(br.readBe32()).isEqualTo(32);
        // data entry
        assertThat(br.readBe32()).isEqualTo(1);
        assertThat(br.readBe32()).isEqualTo(AppleSingle.OVERHEAD);
        assertThat(br.readBe32()).isEqualTo(4);
        // rsrc entry
        assertThat(br.readBe32()).isEqualTo(2);
        assertThat(br.readBe32()).isEqualTo(AppleSingle.OVERHEAD + 4);
        assertThat(br.readBe32()).isEqualTo(4);
        // finder body
        assertThat(new String(br.read(4).toByteArray(), StandardCharsets.UTF_8)).isEqualTo("TEXT");
        assertThat(new String(br.read(4).toByteArray(), StandardCharsets.UTF_8)).isEqualTo("ttxt");
        for (int i = 0; i < 24; i++)
            assertThat(br.read8()).isEqualTo(0);
        assertThat(new String(br.read(4).toByteArray(), StandardCharsets.UTF_8)).isEqualTo("data");
        assertThat(new String(br.read(4).toByteArray(), StandardCharsets.UTF_8)).isEqualTo("rsrc");
        assertThat(br.eof()).isTrue();
    }
}
