package com.cowlark.fluxengine.arch.amiga;

import static com.google.common.truth.Truth.assertThat;
import static org.junit.Assert.assertThrows;

import com.cowlark.fluxengine.config.ConfigBuilder;
import com.cowlark.fluxengine.config.ConfigProto;
import com.cowlark.fluxengine.core.Bytes;
import com.cowlark.fluxengine.core.FluxEngineException;
import com.cowlark.fluxengine.data.Fluxmap;
import com.cowlark.fluxengine.data.Image;
import com.cowlark.fluxengine.data.Sector;
import com.google.common.collect.ImmutableList;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;
import java.util.List;

@RunWith(JUnit4.class)
public class AmigaEncoderTest
{
    @org.junit.Rule public final org.junit.rules.TestRule loggerRule =
            com.cowlark.fluxengine.testing.TestHelpers.loggerRule();

    private ConfigProto makeConfig()
    {
        return new ConfigBuilder()
                .set("usb.serial", "test-serial")
                .set("drive.rotational_period_ms", "200")
                .set("encoder.amiga.clock_rate_us", "2.0")
                .build();
    }

    @Test
    public void encodeProducesPulses()
    {
        ConfigProto config = makeConfig();
        AmigaEncoder encoder = new AmigaEncoder(config, 200 * 1e6);

        Image image = new Image();
        Sector sector = image.put(0, 0, 0);
        sector.data = new Bytes(512);

        List<Sector> sectors = ImmutableList.of(sector);

        Fluxmap fluxmap = encoder.encode(null, sectors, image);

        assertThat(fluxmap.ticks()).isGreaterThan(0);
        assertThat(fluxmap.bytes()).isGreaterThan(0);
    }

    @Test
    public void encodeRejectsBadSectorSize()
    {
        ConfigProto config = makeConfig();
        AmigaEncoder encoder = new AmigaEncoder(config, 200 * 1e6);

        Image image = new Image();
        Sector sector = image.put(0, 0, 0);
        sector.data = new Bytes(511);

        List<Sector> sectors = ImmutableList.of(sector);

        FluxEngineException e =
                assertThrows(FluxEngineException.class, () -> encoder.encode(null, sectors, image));
        assertThat(e.getMessage()).contains("unsupported sector size");
    }
}
