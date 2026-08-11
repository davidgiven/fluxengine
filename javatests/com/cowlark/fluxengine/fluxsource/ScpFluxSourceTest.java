package com.cowlark.fluxengine.fluxsource;

import static com.google.common.truth.Truth.assertThat;

import com.cowlark.fluxengine.config.ConfigBuilder;
import com.cowlark.fluxengine.config.ConfigProto;
import com.cowlark.fluxengine.core.ByteWriter;
import com.cowlark.fluxengine.core.Bytes;
import com.cowlark.fluxengine.external.DriveType;
import com.cowlark.fluxengine.external.Scp;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;
import java.io.IOException;
import java.nio.file.Files;
import java.nio.file.Path;

@RunWith(JUnit4.class)
public class ScpFluxSourceTest
{
    @org.junit.Rule
    public final org.junit.rules.TestRule loggerRule =
            com.cowlark.fluxengine.testing.TestHelpers.loggerRule();

    /* Builds an SCP file containing a single track 0/0 (strack 0), encoded
     * with two intervals of 100 and 200 at a 25ns resolution. */
    private static Path writeTempFile() throws IOException
    {
        Bytes result = new Bytes(Scp.SCP_HEADER_SIZE + 4 + 12 + 4);
        ByteWriter bw = new ByteWriter(result);

        bw.write8('S');
        bw.write8('C');
        bw.write8('P');
        bw.write8(0x18); /* version 1.8 */
        bw.write8(0xff); /* type */
        bw.write8(1); /* revolutions */
        bw.write8(Scp.strackno(0, 0)); /* start track */
        bw.write8(Scp.strackno(0, 0)); /* end track */
        bw.write8(0); /* flags: not 96tpi */
        bw.write8(0); /* cell width: 16-bit cells */
        bw.write8(1); /* heads: side 0 only */
        bw.write8(0); /* resolution: 25ns */
        bw.writeLe32(0); /* checksum */

        /* Track offset table; only strack 0 is present. */
        int trackOffset = Scp.SCP_HEADER_SIZE;
        for (int i = 0; i < 168; i++)
            bw.writeLe32(i == 0 ? trackOffset : 0);

        /* Track header: 'TRK' + strack, then one revolution record. */
        bw.write8('T');
        bw.write8('R');
        bw.write8('K');
        bw.write8(0); /* strack */
        bw.writeLe32(0); /* index */
        bw.writeLe32(2); /* length: two cells */
        bw.writeLe32(16); /* offset to cell data, relative to track header */

        /* Cell data: two big-endian intervals. */
        bw.writeBe16(100);
        bw.writeBe16(200);

        Path path = Files.createTempFile("flux", ".scp");
        Files.write(path, result.toByteArray());
        return path;
    }

    @Test
    public void readsTracks() throws IOException
    {
        Path path = writeTempFile();

        ScpFluxSource source = new ScpFluxSource(ScpFluxSourceProto.newBuilder()
                .setFilename(path.toString())
                .build());

        FluxSourceIterator iterator =
                source.readFlux(FluxReadParameters.builder().setCylinder(0).setHead(0).build());
        assertThat(iterator.hasNext()).isTrue();
        Bytes expected = Bytes.of(0x9e, 0xbc);
        assertThat(iterator.next().rawBytes().toByteArray()).isEqualTo(expected.toByteArray());
        assertThat(iterator.hasNext()).isFalse();

        ConfigBuilder configBuilder = new ConfigBuilder().set("usb.serial", "test-serial");
        source.adjustConfig(configBuilder);
        ConfigProto config = configBuilder.build();
        assertThat(config.getDrive().getTracks()).isEqualTo("c0h0");
        assertThat(config.getDrive().getDriveType()).isEqualTo(DriveType.DRIVETYPE_40TRACK);
    }

    @Test
    public void missingTrackReturnsEmptyFluxmap() throws IOException
    {
        Path path = writeTempFile();

        ScpFluxSource source = new ScpFluxSource(ScpFluxSourceProto.newBuilder()
                .setFilename(path.toString())
                .build());

        FluxSourceIterator iterator =
                source.readFlux(FluxReadParameters.builder().setCylinder(1).setHead(0).build());
        assertThat(iterator.hasNext()).isTrue();
        assertThat(iterator.next().ticks()).isEqualTo(0);
    }
}
