package com.cowlark.fluxengine.algorithms;

import static com.google.common.truth.Truth.assertThat;

import com.cowlark.fluxengine.config.ConfigBuilder;
import com.cowlark.fluxengine.config.ConfigProto;
import com.cowlark.fluxengine.core.Bytes;
import com.cowlark.fluxengine.data.CylinderHead;
import com.cowlark.fluxengine.data.DiskLayout;
import com.cowlark.fluxengine.data.Fluxmap;
import com.cowlark.fluxengine.data.Image;
import com.cowlark.fluxengine.data.LogicalTrackLayout;
import com.cowlark.fluxengine.data.Sector;
import com.cowlark.fluxengine.encoders.Encoder;
import com.cowlark.fluxengine.fluxsink.FluxSink;
import com.cowlark.fluxengine.fluxsink.FluxSinkFactory;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

@RunWith(JUnit4.class)
public class WriterTest
{
    private static class RecordingFluxSink extends FluxSink
    {
        final Map<CylinderHead, Fluxmap> written = new HashMap<>();

        @Override
        public void addFlux(int track, int head, Fluxmap fluxmap)
        {
            written.put(new CylinderHead(track, head), fluxmap);
        }
    }

    private static class RecordingFluxSinkFactory extends FluxSinkFactory
    {
        final RecordingFluxSink sink = new RecordingFluxSink();

        @Override
        public FluxSink create()
        {
            return sink;
        }
    }

    private static class TestEncoder extends Encoder
    {
        final List<LogicalTrackLayout> encoded = new ArrayList<>();

        @Override
        public Fluxmap encode(LogicalTrackLayout ltl, List<Sector> sectors, Image image)
        {
            encoded.add(ltl);
            Fluxmap fluxmap = new Fluxmap();
            fluxmap.appendInterval(100);
            fluxmap.appendPulse();
            return fluxmap;
        }
    }

    private static ConfigProto makeConfig()
    {
        return new ConfigBuilder().set("usb.serial", "test-serial")
                .set("drive.rotational_period_ms", "200")
                .set("layout.tracks", "1")
                .set("layout.sides", "1")
                .set("layout.layoutdata[0].sector_size", "256")
                .set("layout.layoutdata[0].physical.start_sector", "0")
                .set("layout.layoutdata[0].physical.count", "8")
                .build();
    }

    private static Image makeImage()
    {
        Image image = new Image();
        for (int sectorId = 0; sectorId < 8; sectorId++)
        {
            Sector sector = image.put(0, 0, sectorId);
            sector.status = Sector.Status.OK;
            sector.data = Bytes.of(sectorId);
        }
        return image;
    }

    @Test
    public void writesAllLogicalLocations()
    {
        ConfigProto config = makeConfig();
        DiskLayout diskLayout = new DiskLayout(config);
        Image image = makeImage();

        RecordingFluxSinkFactory factory = new RecordingFluxSinkFactory();
        TestEncoder encoder = new TestEncoder();

        Writer.writeDiskCommand(config, diskLayout, image, encoder, factory, null, null);

        assertThat(encoder.encoded).hasSize(1);
        assertThat(encoder.encoded.get(0).logicalCylinder).isEqualTo(0);
        assertThat(factory.sink.written.keySet()).containsExactly(new CylinderHead(0, 0));
        assertThat(factory.sink.written.get(new CylinderHead(0, 0)).bytes()).isGreaterThan(0);
    }

    @Test
    public void writesWithoutVerifyWhenNoSource()
    {
        ConfigProto config = makeConfig();
        DiskLayout diskLayout = new DiskLayout(config);
        Image image = makeImage();

        RecordingFluxSinkFactory factory = new RecordingFluxSinkFactory();
        TestEncoder encoder = new TestEncoder();

        /* decoder/fluxSource are null, so no verification happens. */
        Writer.writeDiskCommand(config, diskLayout, image, encoder, factory, null, null);

        assertThat(factory.sink.written).hasSize(1);
    }

    @Test
    public void emptyImageThrows()
    {
        ConfigProto config = makeConfig();
        DiskLayout diskLayout = new DiskLayout(config);
        Image image = new Image();

        RecordingFluxSinkFactory factory = new RecordingFluxSinkFactory();
        TestEncoder encoder = new TestEncoder();

        /* The encoder needs all sectors present in the image, so an empty
         * image is an error. */
        org.junit.Assert.assertThrows(
                com.cowlark.fluxengine.core.FluxEngineException.class,
                () -> Writer.writeDiskCommand(
                        config,
                        diskLayout,
                        image,
                        encoder,
                        factory,
                        null,
                        null));
    }
}
