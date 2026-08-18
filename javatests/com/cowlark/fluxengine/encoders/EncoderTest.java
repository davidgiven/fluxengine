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
import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;
import java.util.List;

@RunWith(JUnit4.class)
public class EncoderTest
{
    @org.junit.Rule public final org.junit.rules.TestRule loggerRule =
            com.cowlark.fluxengine.testing.TestHelpers.loggerRule();

    @Test
    public void createThrowsNotImplemented()
    {
        ConfigProto config = new ConfigBuilder().set("usb.serial", "test-serial").build();

        assertThrows(FluxEngineException.class, () -> Encoder.create(config));
    }

    @Test
    public void collectSectorsCollectsInDiskOrder()
    {
        /* A single-track, single-side disk with sectors 0 and 1. */
        DiskLayout layout = new DiskLayout(1, 1, 2, 256);
        LogicalTrackLayout ltl =
                layout.layoutByLogicalLocation.get(new com.cowlark.fluxengine.data.CylinderHead(0,
                        0));
        assertThat(ltl).isNotNull();

        Image image = new Image();
        image.put(0, 0, 0);
        image.put(0, 0, 1);

        TestEncoder encoder = new TestEncoder(200 * 1e6);

        ImmutableList<Sector> sectors = encoder.collectSectors(ltl, image);

        assertThat(sectors).hasSize(2);
        assertThat(sectors.get(0).location.logicalSector()).isEqualTo(0);
        assertThat(sectors.get(1).location.logicalSector()).isEqualTo(1);
    }

    @Test
    public void collectSectorsMissingSectorThrows()
    {
        DiskLayout layout = new DiskLayout(1, 1, 2, 256);
        LogicalTrackLayout ltl =
                layout.layoutByLogicalLocation.get(new com.cowlark.fluxengine.data.CylinderHead(0,
                        0));

        Image image = new Image();
        image.put(0, 0, 0); /* sector 1 missing */

        TestEncoder encoder = new TestEncoder(200 * 1e6);

        assertThrows(FluxEngineException.class, () -> encoder.collectSectors(ltl, image));
    }

    @Test
    public void calculatePhysicalClockPeriodNs()
    {
        TestEncoder encoder = new TestEncoder(200 * 1e6);

        assertThat(encoder.calculatePhysicalClockPeriodNs(4000, 200e6)).isEqualTo(4000.0);
    }

    @Test
    public void calculatePhysicalClockPeriodNsUnsetThrows()
    {
        TestEncoder encoder = new TestEncoder(0);

        assertThrows(FluxEngineException.class,
                () -> encoder.calculatePhysicalClockPeriodNs(4000, 200e6));
    }

    private static final class TestEncoder extends Encoder
    {
        TestEncoder(double diskRotationalPeriodNs)
        {
            super(diskRotationalPeriodNs);
        }

        @Override
        public Fluxmap encode(LogicalTrackLayout ltl, List<Sector> sectors, Image image)
        {
            return new Fluxmap();
        }
    }
}
