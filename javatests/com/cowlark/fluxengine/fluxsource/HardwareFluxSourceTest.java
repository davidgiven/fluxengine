package com.cowlark.fluxengine.fluxsource;

import static com.google.common.truth.Truth.assertThat;
import static org.mockito.ArgumentMatchers.any;
import static org.mockito.ArgumentMatchers.anyDouble;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.when;

import com.cowlark.fluxengine.config.ConfigBuilder;
import com.cowlark.fluxengine.config.ConfigProto;
import com.cowlark.fluxengine.core.Bytes;
import com.cowlark.fluxengine.data.Fluxmap;
import com.cowlark.fluxengine.testing.TestHelpers;
import com.cowlark.fluxengine.usb.UsbDevice;
import com.cowlark.fluxengine.usb.VoltageMeasurements;
import org.junit.Rule;
import org.junit.Test;
import org.junit.rules.TestRule;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;
import org.mockito.Mock;
import org.mockito.junit.MockitoJUnit;
import org.mockito.junit.MockitoRule;

@RunWith(JUnit4.class)
public class HardwareFluxSourceTest
{
    @Rule public final TestRule loggerRule = TestHelpers.loggerRule();
    @Rule public final MockitoRule mockitoRule = MockitoJUnit.rule();

    @Mock private UsbDevice mockUsbDevice;

    private static ConfigProto config()
    {
        return new ConfigBuilder().set("usb.serial", "test-serial")
                .set("drive.sync_with_index", "true")
                .set("drive.revolutions", "3")
                .set("drive.rotational_period_ms", "200")
                .set("drive.hard_sector_threshold_ns", "1000")
                .build();
    }

    @Test
    public void isHardware()
    {
        HardwareFluxSource source = new HardwareFluxSource(config(), mockUsbDevice);

        assertThat(source.isHardware()).isTrue();
    }

    @Test
    public void readFluxReadsAndWrapsFluxmap()
    {
        Bytes readResult = Bytes.of(0x01, 0x02, 0x03, 0x04);
        when(mockUsbDevice.read(any(), anyDouble())).thenReturn(readResult);
        HardwareFluxSource source = new HardwareFluxSource(config(), mockUsbDevice);

        FluxSourceIterator iterator = source.readFlux(FluxReadParameters.builder()
                .setCylinder(17)
                .setHead(1)
                .setSyncWithIndex(true)
                .setReadTimeNs(3 * 200 * 1e6)
                .setHardSectorThresholdNs(1000)
                .build());

        assertThat(iterator.hasNext()).isTrue();
        Fluxmap fluxmap = iterator.next();

        assertThat(fluxmap.rawBytes()).isEqualTo(readResult);
        assertThat(iterator.hasNext()).isTrue();
    }
}
