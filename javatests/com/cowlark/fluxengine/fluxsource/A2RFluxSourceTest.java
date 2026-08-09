package com.cowlark.fluxengine.fluxsource;

import static com.google.common.truth.Truth.assertThat;

import com.cowlark.fluxengine.config.ConfigBuilder;
import com.cowlark.fluxengine.config.ConfigProto;
import com.cowlark.fluxengine.core.ByteWriter;
import com.cowlark.fluxengine.core.Bytes;
import com.cowlark.fluxengine.external.DriveType;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;
import java.io.IOException;
import java.nio.file.Files;
import java.nio.file.Path;

@RunWith(JUnit4.class)
public class A2RFluxSourceTest
{
    /* Builds an A2R file containing a single track 0/0, encoded as a 3.5"
     * disk with two short intervals. */
    private static Path writeTempFile() throws IOException
    {
        Bytes result = new Bytes(256);
        ByteWriter bw = new ByteWriter(result);

        for (int b : new int[]{'A', '2', 'R', '2', 0xff, 0x0a, 0x0d, 0x0a})
            bw.write8(b);

        // INFO chunk: version, 32-char padding, disktype (=2, 3.5"), ...
        writeChunk(bw, "INFO");
        int sizePos = bw.pos();
        bw.writeLe32(0);
        bw.write8(1);
        for (int i = 0; i < 32; i++)
            bw.write8('x');
        bw.write8(2);
        bw.write8(1);
        bw.write8(1);
        int infoEnd = bw.pos();
        bw.seek(sizePos);
        bw.writeLe32(infoEnd - sizePos - 4);
        bw.seek(infoEnd);

        // STRM chunk: one record for track 0 head 0 with flux data 30,30 (pulses
        // at 30 a2r ticks). The headed iterating sums bytes until non-0xff,
        // so this encodes three intervals: 30, 30 and a trailing 255-less end.
        writeChunk(bw, "STRM");
        int sizePos2 = bw.pos();
        bw.writeLe32(0);
        bw.write8(0); // location: cylinder 0, head 0
        bw.write8(0); // unused byte
        bw.writeLe32(3); // data length
        bw.writeLe32(0); // index
        bw.write8(30);
        bw.write8(30);
        bw.write8(30);
        bw.write8(0xff); // stream terminator
        int strmSize = bw.pos();
        bw.seek(sizePos2);
        bw.writeLe32(strmSize - sizePos2 - 4);
        bw.seek(strmSize);

        Bytes bytes = result.slice(0, strmSize);
        Path path = Files.createTempFile("flux", ".a2r");
        Files.write(path, bytes.toByteArray());
        return path;
    }

    private static void writeChunk(ByteWriter bw, String id)
    {
        for (int i = 0; i < 4; i++)
            bw.write8(id.charAt(i));
    }

    @Test
    public void readsTracks() throws IOException
    {
        Path path = writeTempFile();

        A2RFluxSource source = new A2RFluxSource(A2rFluxSourceProto.newBuilder()
                .setFilename(path.toString())
                .build());

        FluxSourceIterator iterator =
                source.readFlux(FluxReadParameters.builder().setCylinder(0).setHead(0).build());
        assertThat(iterator.hasNext()).isTrue();
        Bytes expected = Bytes.of(0x40, 0xad, 0xad, 0xad);
        assertThat(iterator.next().rawBytes().toByteArray()).isEqualTo(expected.toByteArray());
        assertThat(iterator.hasNext()).isFalse();
        assertThat(source.readFlux(FluxReadParameters.builder().setCylinder(1).setHead(0).build())).isInstanceOf(
                EmptyFluxSourceIterator.class);

        ConfigBuilder configBuilder = new ConfigBuilder().set("usb.serial", "test-serial");
        source.adjustConfig(configBuilder);
        ConfigProto config = configBuilder.build();
        assertThat(config.getDrive().getTracks()).isEqualTo("c0h0");
        assertThat(config.getDrive().getDriveType()).isEqualTo(DriveType.DRIVETYPE_80TRACK);
    }
}