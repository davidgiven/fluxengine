package com.cowlark.fluxengine.fluxsource;

import static com.google.common.truth.Truth.assertThat;

import com.cowlark.fluxengine.config.ConfigBuilder;
import com.cowlark.fluxengine.config.ConfigProto;
import com.cowlark.fluxengine.external.FluxFileProto;
import com.cowlark.fluxengine.external.FluxFileVersion;
import com.cowlark.fluxengine.external.TrackFluxProto;
import com.google.protobuf.ByteString;
import java.io.IOException;
import java.nio.file.Files;
import java.nio.file.Path;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;

@RunWith(JUnit4.class)
public class Fl2FluxSourceTest
{
    @Test
    public void readsTracks() throws IOException
    {
        TrackFluxProto track = TrackFluxProto.newBuilder()
                .setTrack(0)
                .setHead(0)
                .addFlux(ByteString.copyFrom(new byte[]{(byte) 0xb0}))
                .build();
        Path path = writeTemp(FluxFileProto.newBuilder()
                .setVersion(FluxFileVersion.VERSION_2)
                .addTrack(track)
                .setRotationalPeriodMs(200.0)
                .build());

        Fl2FluxSource source = new Fl2FluxSource(Fl2FluxSourceProto.newBuilder()
                .setFilename(path.toString())
                .build());

        FluxSourceIterator iterator = source.readFlux(0, 0);
        assertThat(iterator.hasNext()).isTrue();
        assertThat(iterator.next()).isNotNull();
        assertThat(iterator.hasNext()).isFalse();
        assertThat(source.readFlux(1, 0)).isInstanceOf(EmptyFluxSourceIterator.class);

        ConfigBuilder configBuilder = new ConfigBuilder().set("usb.serial", "test-serial");
        source.adjustConfig(configBuilder);
        ConfigProto config = configBuilder.build();
        assertThat(config.getDrive().getTracks()).isEqualTo("c0h0");
        assertThat(config.getDrive().getRotationalPeriodMs()).isEqualTo(200.0);
    }

    @Test
    public void upgradesVersion1() throws IOException
    {
        /* A single flux segment containing a desync byte should be split into
         * two segments. */
        TrackFluxProto track = TrackFluxProto.newBuilder()
                .setTrack(0)
                .setHead(0)
                .addFlux(ByteString.copyFrom(new byte[]{(byte) 0xb0, 0x00, (byte) 0xb0}))
                .build();
        Path path = writeTemp(FluxFileProto.newBuilder()
                .setVersion(FluxFileVersion.VERSION_1)
                .addTrack(track)
                .build());

        Fl2FluxSource source = new Fl2FluxSource(Fl2FluxSourceProto.newBuilder()
                .setFilename(path.toString())
                .build());

        FluxSourceIterator iterator = source.readFlux(0, 0);
        assertThat(iterator.hasNext()).isTrue();
        iterator.next();
        assertThat(iterator.hasNext()).isTrue();
        iterator.next();
        assertThat(iterator.hasNext()).isFalse();
    }

    private static Path writeTemp(FluxFileProto file) throws IOException
    {
        Path path = Files.createTempFile("flux", ".fl2");
        Files.write(path, file.toByteArray());
        return path;
    }
}
