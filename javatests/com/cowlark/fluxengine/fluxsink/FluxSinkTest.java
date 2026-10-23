package com.cowlark.fluxengine.fluxsink;

import static com.google.common.truth.Truth.assertThat;
import static org.junit.Assert.assertThrows;

import com.cowlark.fluxengine.config.ConfigBuilder;
import com.cowlark.fluxengine.config.ConfigProto;
import com.cowlark.fluxengine.core.ByteReader;
import com.cowlark.fluxengine.core.Bytes;
import com.cowlark.fluxengine.core.FluxEngineException;
import com.cowlark.fluxengine.data.Fluxmap;
import com.cowlark.fluxengine.external.Scp;
import com.cowlark.fluxengine.fluxsource.A2RFluxSource;
import com.cowlark.fluxengine.fluxsource.A2R3FluxSource;
import com.cowlark.fluxengine.fluxsource.A2rFluxSourceProto;
import com.cowlark.fluxengine.fluxsource.FluxSource;
import com.cowlark.fluxengine.fluxsource.FluxSourceIterator;
import com.cowlark.fluxengine.fluxsource.FluxReadParameters;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;
import java.io.IOException;
import java.nio.file.Files;
import java.nio.file.Path;

@RunWith(JUnit4.class)
public class FluxSinkTest
{
    @org.junit.Rule public final org.junit.rules.TestRule loggerRule =
            com.cowlark.fluxengine.testing.TestHelpers.loggerRule();

    private static ConfigProto makeConfig()
    {
        return new ConfigBuilder()
                .set("usb.serial", "test-serial")
                .set("drive.rotational_period_ms", "200")
                .set("layout.tracks", "1")
                .set("layout.sides", "1")
                .set("layout.layoutdata[0].sector_size", "256")
                .set("layout.layoutdata[0].physical.start_sector", "0")
                .set("layout.layoutdata[0].physical.count", "8")
                .build();
    }

    private static Fluxmap makeFluxmap()
    {
        Fluxmap fluxmap = new Fluxmap();
        fluxmap.appendInterval(100);
        fluxmap.appendPulse();
        fluxmap.appendInterval(50);
        fluxmap.appendPulse();
        fluxmap.appendIndex();
        return fluxmap;
    }

    @Test
    public void vcdWritesFile() throws IOException
    {
        Path dir = Files.createTempDirectory("vcd");
        VcdFluxSink sink = new VcdFluxSink(dir.toString());
        sink.addFlux(0, 0, makeFluxmap());

        String contents = Files.readString(dir.resolve("c00.h0.vcd"));
        assertThat(contents).contains("$timescale 1ns $end");
        assertThat(contents).contains("$var wire 1 p pulse $end");
        assertThat(contents).contains("$enddefinitions $end");
    }

    @Test
    public void auWritesFile() throws IOException
    {
        Path dir = Files.createTempDirectory("au");
        AuFluxSink sink = new AuFluxSink(dir.toString(), true);
        sink.addFlux(0, 0, makeFluxmap());

        Bytes data = new Bytes(Files.readAllBytes(dir.resolve("c00.h0.au")));
        ByteReader br = new ByteReader(data);
        assertThat(br.readBe32()).isEqualTo(0x2e736e64);
        assertThat(br.readBe32()).isEqualTo(24);
        assertThat(br.readBe32()).isEqualTo((makeFluxmap().ticks() + 2) * 2);
        assertThat(br.readBe32()).isEqualTo(2); /* 8-bit PCM */
        assertThat(br.readBe32()).isEqualTo(12000000); /* TICK_FREQUENCY */
        assertThat(br.readBe32()).isEqualTo(2); /* channels */
    }

    @Test
    public void a2rWritesFile() throws IOException
    {
        Path path = Files.createTempFile("flux", ".a2r");
        Files.delete(path);

        A2R3FluxSink sink = new A2R3FluxSink(path.toString(), makeConfig());
        sink.addFlux(0, 0, makeFluxmap());
        sink.close();

        Bytes data = new Bytes(Files.readAllBytes(path));
        assertThat(data.size()).isGreaterThan(0);
        /* File header: A2R3 then 0xff 0x0a 0x0d 0x0a. */
        assertThat(new String(data.slice(0, 4).toByteArray())).isEqualTo("A2R3");
        assertThat(data.getByte(4) & 0xff).isEqualTo(0xff);
        assertThat(data.getByte(5) & 0xff).isEqualTo(0x0a);
    }

    @Test
    public void scpWritesFile() throws IOException
    {
        Path path = Files.createTempFile("flux", ".scp");
        Files.delete(path);

        ScpFluxSink sink = new ScpFluxSink(path.toString(), 0xff, false, makeConfig());
        sink.addFlux(0, 0, makeFluxmap());
        sink.close();

        Bytes data = new Bytes(Files.readAllBytes(path));
        assertThat(data.size()).isGreaterThan(Scp.SCP_HEADER_SIZE);
        assertThat(new String(data.slice(0, 3).toByteArray())).isEqualTo("SCP");
        assertThat(data.getByte(3) & 0xff).isEqualTo(0x18); /* version */
        assertThat(data.getByte(4) & 0xff).isEqualTo(0xff); /* type byte */
    }

    @Test
    public void scpRejectsApple2()
    {
        ConfigProto config = new ConfigBuilder()
                .set("usb.serial", "test-serial")
                .set("drive.rotational_period_ms", "200")
                .set("drive.drive_type", "DRIVETYPE_APPLE2")
                .set("layout.tracks", "1")
                .set("layout.sides", "1")
                .build();

        assertThrows(
                FluxEngineException.class,
                () -> new ScpFluxSink("test.scp", 0xff, false, config));
    }

    @Test
    public void factoryWiring()
    {
        ConfigProto config = new ConfigBuilder()
                .set("usb.serial", "test-serial")
                .set("flux_sink.type", "FLUXTYPE_A2R")
                .set("flux_sink.a2r.filename", "test.a2r")
                .build();

        FluxSinkFactory factory = FluxSinkFactory.create(config, null);
        assertThat(factory).isInstanceOf(A2R3FluxSinkFactory.class);
        assertThat(factory.getPath()).isEqualTo("test.a2r");
        assertThat(factory.isHardware()).isFalse();
    }
    @Test
    public void a2r3CaptureTypeBoundary() throws Exception
    {
        /* At 12 MHz, 6,000,000 ticks are exactly 2.5 revolutions at 300 RPM. */
        Path path = Files.createTempFile("flux", ".a2r");
        try
        {
            for (int duration : new int[] {5_999_999, 6_000_000, 6_000_001})
            {
                Fluxmap map = new Fluxmap();
                map.appendInterval(duration).appendPulse();
                try (A2R3FluxSink sink = new A2R3FluxSink(path.toString(), makeConfig()))
                {
                    sink.addFlux(0, 0, map);
                }

                /* Inspect the on-disk type independently of the reader. */
                ByteReader reader = Bytes.readFromFile(path).reader();
                reader.skip(8);
                assertThat(reader.readLe32()).isEqualTo(0x4f464e49);
                reader.skip(reader.readLe32());
                assertThat(reader.readLe32()).isEqualTo(0x50435752);
                reader.readLe32();
                reader.skip(16);
                assertThat(reader.read8()).isEqualTo('C');
                assertThat(reader.read8()).isEqualTo(duration < 6_000_000 ? 1 : 3);
            }
        } finally
        {
            Files.delete(path);
        }
    }

    @Test
    public void a2r3RoundTripsCapturesAndIndexes() throws Exception
    {
        /* Record two captures at the same location, with two index signals per capture. */
        Path path = Files.createTempFile("flux", ".a2r");
        Fluxmap map = new Fluxmap();
        map.appendInterval(100).appendPulse();
        map.appendInterval(2_399_900).appendIndex();
        map.appendInterval(2_400_000).appendIndex().appendPulse();
        try
        {
            try (A2R3FluxSink sink = new A2R3FluxSink(path.toString(), makeConfig()))
            {
                sink.addFlux(0, 0, map);
                sink.addFlux(0, 0, map);
            }

            /* Read the completed file through version dispatch and compare both captures. */
            FluxSource source = A2RFluxSource.create(A2rFluxSourceProto.newBuilder()
                    .setFilename(path.toString()).build());
            assertThat(source).isInstanceOf(A2R3FluxSource.class);
            FluxSourceIterator iterator = source.readFlux(FluxReadParameters.builder()
                    .setCylinder(0).setHead(0).build());
            for (int i = 0; i < 2; i++)
            {
                Fluxmap decoded = iterator.next();
                assertThat(decoded.getIndexMarks()).containsExactly(200_000_000.0, 400_000_000.0);
                assertThat(decoded.durationNs()).isWithin(100).of(map.durationNs());
            }
            assertThat(iterator.hasNext()).isFalse();
        } finally
        {
            Files.delete(path);
        }
    }
}
