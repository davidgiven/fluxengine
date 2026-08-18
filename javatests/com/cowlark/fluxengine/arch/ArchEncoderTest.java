package com.cowlark.fluxengine.arch;

import static com.google.common.truth.Truth.assertThat;
import static org.junit.Assert.assertThrows;

import com.cowlark.fluxengine.config.ConfigBuilder;
import com.cowlark.fluxengine.config.ConfigProto;
import com.cowlark.fluxengine.core.FluxEngineException;
import com.cowlark.fluxengine.encoders.Encoder;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;

@RunWith(JUnit4.class)
public class ArchEncoderTest
{
    @org.junit.Rule public final org.junit.rules.TestRule loggerRule =
            com.cowlark.fluxengine.testing.TestHelpers.loggerRule();

    @Test
    public void noEncoderConfiguredThrows()
    {
        ConfigProto config = new ConfigBuilder().set("usb.serial", "test-serial").build();

        assertThrows(FluxEngineException.class, () -> Arch.createEncoder(config));
    }

    @Test
    public void createAmigaEncoder()
    {
        ConfigProto config = new ConfigBuilder().set("usb.serial", "test-serial")
                .set("drive.rotational_period_ms", "200")
                .set("encoder.amiga.clock_rate_us", "2.0")
                .build();

        Encoder encoder = Arch.createEncoder(config);

        assertThat(encoder).isInstanceOf(com.cowlark.fluxengine.arch.amiga.AmigaEncoder.class);
    }

    @Test
    public void createIbmEncoder()
    {
        ConfigProto config = new ConfigBuilder().set("usb.serial", "test-serial")
                .set("drive.rotational_period_ms", "200")
                .set("encoder.ibm.trackdata[0].emit_iam", "false")
                .build();

        Encoder encoder = Arch.createEncoder(config);

        assertThat(encoder).isInstanceOf(com.cowlark.fluxengine.arch.ibm.IbmEncoder.class);
    }

    @Test
    public void createTartuEncoder()
    {
        ConfigProto config = new ConfigBuilder().set("usb.serial", "test-serial")
                .set("drive.rotational_period_ms", "200")
                .set("encoder.tartu.clock_period_us", "2.0")
                .build();

        Encoder encoder = Arch.createEncoder(config);

        assertThat(encoder).isInstanceOf(com.cowlark.fluxengine.arch.tartu.TartuEncoder.class);
    }
}
