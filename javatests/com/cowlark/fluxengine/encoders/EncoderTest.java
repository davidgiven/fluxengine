package com.cowlark.fluxengine.encoders;

import static com.google.common.truth.Truth.assertThat;

import static org.junit.Assert.assertThrows;

import com.cowlark.fluxengine.config.ConfigBuilder;
import com.cowlark.fluxengine.config.ConfigProto;
import com.cowlark.fluxengine.core.FluxEngineException;
import com.cowlark.fluxengine.data.DiskLayout;
import com.cowlark.fluxengine.data.Fluxmap;
import com.cowlark.fluxengine.data.Image;
import com.cowlark.fluxengine.data.LogicalTrackLayout;
import com.cowlark.fluxengine.data.Sector;
import com.google.common.collect.ImmutableList;
import java.util.List;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;

@RunWith(JUnit4.class)
public class EncoderTest
{
    private static final class TestEncoder extends Encoder
    {
        @Override
        public Fluxmap encode(LogicalTrackLayout ltl, List<Sector> sectors, Image image)
        {
            return new Fluxmap();
        }
    }

    @Test
    public void createThrowsNotImplemented()
    {
        ConfigProto config = new ConfigBuilder()
                .set("usb.serial", "test-serial")
                .build();

        assertThrows(FluxEngineException.class, () -> Encoder.create(config));
    }

    @Test
    public void collectSectorsCollectsInDiskOrder()
    {
        /* A single-track, single-side disk with sectors 0 and 1. */
        DiskLayout layout = new DiskLayout(1, 1, 2, 256);
        LogicalTrackLayout ltl = layout.layoutByLogicalLocation.get(
                new com.cowlark.fluxengine.data.CylinderHead(0, 0));
        assertThat(ltl).isNotNull();

        Image image = new Image();
        image.put(0, 0, 0);
        image.put(0, 0, 1);

        TestEncoder encoder = new TestEncoder();

        ImmutableList<Sector> sectors = encoder.collectSectors(ltl, image);

        assertThat(sectors).hasSize(2);
        assertThat(sectors.get(0).location.logicalSector()).isEqualTo(0);
        assertThat(sectors.get(1).location.logicalSector()).isEqualTo(1);
    }

    @Test
    public void collectSectorsMissingSectorThrows()
    {
        DiskLayout layout = new DiskLayout(1, 1, 2, 256);
        LogicalTrackLayout ltl = layout.layoutByLogicalLocation.get(
                new com.cowlark.fluxengine.data.CylinderHead(0, 0));

        Image image = new Image();
        image.put(0, 0, 0); /* sector 1 missing */

        TestEncoder encoder = new TestEncoder();

        assertThrows(
                FluxEngineException.class,
                () -> encoder.collectSectors(ltl, image));
    }

    @Test
    public void calculatePhysicalClockPeriod()
    {
        ConfigProto config = new ConfigBuilder()
                .set("usb.serial", "test-serial")
                .set("drive.rotational_period_ms", "200")
                .build();

        TestEncoder encoder = new TestEncoder();

        assertThat(encoder.calculatePhysicalClockPeriod(config, 4000, 200e6))
                .isEqualTo(4000.0);
    }

    @Test
    public void calculatePhysicalClockPeriodUnsetThrows()
    {
        ConfigProto config = new ConfigBuilder()
                .set("usb.serial", "test-serial")
                .build();

        TestEncoder encoder = new TestEncoder();

        assertThrows(
                FluxEngineException.class,
                () -> encoder.calculatePhysicalClockPeriod(config, 4000, 200e6));
    }
}
