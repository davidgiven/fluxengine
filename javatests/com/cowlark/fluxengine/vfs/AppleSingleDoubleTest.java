package com.cowlark.fluxengine.vfs;

import static com.google.common.truth.Truth.assertThat;

import com.cowlark.fluxengine.core.Bytes;
import com.cowlark.fluxengine.core.KaitaiByteReaderStream;
import com.cowlark.fluxengine.core.KaitaiByteWriterStream;
import com.google.common.collect.ImmutableList;
import io.kaitai.formats.AppleSingleDouble;
import org.apache.commons.codec.DecoderException;
import org.apache.commons.codec.binary.Hex;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;
import java.nio.charset.StandardCharsets;

@RunWith(JUnit4.class)
public class AppleSingleDoubleTest
{
    private static final byte[] TEST_DATA = getDecoded(
            "000516070002000000000000000000000000000000000000000200000002000000000000000E00000001000000000000000A5265736F7572636520666F726B214461746120666F726B21");

    @Test
    public void testCreation()
    {
        AppleSingleDouble data = new AppleSingleDouble();
        data.setMagic(AppleSingleDouble.FileType.APPLE_DOUBLE);
        data.setVersion(AppleSingleDouble.FileVersion.VERSION_2);
        data.setReserved(new byte[16]);

        AppleSingleDouble.Entry dataFork = new AppleSingleDouble.Entry(null, data, data);
        dataFork.setType(AppleSingleDouble.Entry.Types.DATA_FORK);
        dataFork.setBody("Data fork!".getBytes(StandardCharsets.UTF_8));
        dataFork.setLenBody(10);
        dataFork._check();

        AppleSingleDouble.Entry rsrcFork = new AppleSingleDouble.Entry(null, data, data);
        rsrcFork.setType(AppleSingleDouble.Entry.Types.RESOURCE_FORK);
        rsrcFork.setBody("Resource fork!".getBytes(StandardCharsets.UTF_8));
        rsrcFork.setLenBody(14);
        rsrcFork._check();

        data.setNumEntries(2);
        data.setEntries(ImmutableList.of(rsrcFork, dataFork));
        data._check();

        Bytes bytes = new Bytes();
        KaitaiByteWriterStream kbws = new KaitaiByteWriterStream(bytes.writer());
        data._write(kbws);

        assertThat(bytes.toByteArray()).isEqualTo(TEST_DATA);
    }

    @Test
    public void testParsing()
    {
        AppleSingleDouble data =
                new AppleSingleDouble(new KaitaiByteReaderStream(new Bytes(TEST_DATA).reader()));
        data._read();

        assertThat(data.numEntries()).isEqualTo(2);
        assertThat(data.entries().stream().map(AppleSingleDouble.Entry::type)).containsExactly(
                AppleSingleDouble.Entry.Types.RESOURCE_FORK,
                AppleSingleDouble.Entry.Types.DATA_FORK);
    }

    private static byte[] getDecoded(String data)
    {
        try
        {
            return Hex.decodeHex(data);
        } catch (DecoderException e)
        {
            throw new RuntimeException(e);
        }
    }
}
