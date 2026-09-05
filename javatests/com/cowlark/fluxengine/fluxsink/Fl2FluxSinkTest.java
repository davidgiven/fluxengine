package com.cowlark.fluxengine.fluxsink;

import static com.google.common.truth.Truth.assertThat;

import com.cowlark.fluxengine.config.ConfigBuilder;
import com.cowlark.fluxengine.config.ConfigProto;
import com.cowlark.fluxengine.data.Fluxmap;
import com.cowlark.fluxengine.external.FluxFileProto;
import com.cowlark.fluxengine.external.FluxFileVersion;
import com.cowlark.fluxengine.external.FluxMagic;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;
import java.io.IOException;
import java.nio.file.Files;
import java.nio.file.Path;

@RunWith(JUnit4.class)
public class Fl2FluxSinkTest
{
    @org.junit.Rule public final org.junit.rules.TestRule loggerRule =
            com.cowlark.fluxengine.testing.TestHelpers.loggerRule();

    private static ConfigProto makeConfig()
    {
        return new ConfigBuilder()
                .set("usb.serial", "test-serial")
                .set("drive.rotational_period_ms", "200")
                .build();
    }

    private static Fluxmap makeFluxmap()
    {
        Fluxmap fluxmap = new Fluxmap();
        fluxmap.appendInterval(100);
        fluxmap.appendPulse();
        fluxmap.appendInterval(50);
        fluxmap.appendPulse();
        return fluxmap;
    }

    @Test
    public void writesFile() throws IOException
    {
        Path path = Files.createTempFile("flux", ".fl2");
        Files.delete(path);

        Fl2FluxSink sink = new Fl2FluxSink(path.toString(), makeConfig());
        sink.addFlux(0, 0, makeFluxmap());
        sink.addFlux(0, 1, makeFluxmap());
        sink.close();

        byte[] data = Files.readAllBytes(path);
        assertThat(data.length).isGreaterThan(0);

        FluxFileProto proto = FluxFileProto.parseFrom(data);
        assertThat(proto.getMagic()).isEqualTo(FluxMagic.MAGIC.getNumber());
        assertThat(proto.getVersion()).isEqualTo(FluxFileVersion.VERSION_2);
        assertThat(proto.getRotationalPeriodMs()).isEqualTo(200.0);
        assertThat(proto.getTrackCount()).isEqualTo(2);
        assertThat(proto.getTrack(0).getTrack()).isEqualTo(0);
        assertThat(proto.getTrack(0).getHead()).isEqualTo(0);
        assertThat(proto.getTrack(0).getFluxCount()).isEqualTo(1);
        assertThat(proto.getTrack(1).getTrack()).isEqualTo(0);
        assertThat(proto.getTrack(1).getHead()).isEqualTo(1);
    }

    @Test
    public void factoryWiring()
    {
        ConfigProto config = new ConfigBuilder()
                .set("usb.serial", "test-serial")
                .set("flux_sink.type", "FLUXTYPE_FLUX")
                .set("flux_sink.fl2.filename", "test.fl2")
                .build();

        FluxSinkFactory factory = FluxSinkFactory.create(config, null);

        assertThat(factory).isInstanceOf(Fl2FluxSinkFactory.class);
        assertThat(factory.getPath()).isEqualTo("test.fl2");
        assertThat(factory.isHardware()).isFalse();
    }
}
