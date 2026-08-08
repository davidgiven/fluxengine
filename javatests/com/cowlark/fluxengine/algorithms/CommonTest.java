package com.cowlark.fluxengine.algorithms;

import static com.google.common.truth.Truth.assertThat;

import com.cowlark.fluxengine.config.ConfigBuilder;
import com.cowlark.fluxengine.config.ConfigProto;
import com.cowlark.fluxengine.data.CylinderHead;
import com.cowlark.fluxengine.fluxsource.FluxSource;
import com.cowlark.fluxengine.fluxsource.FluxSourceIterator;
import java.util.ArrayList;
import java.util.List;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;

@RunWith(JUnit4.class)
public class CommonTest
{
    private static ConfigProto makeConfig()
    {
        return new ConfigBuilder()
                .set("usb.serial", "test-serial")
                .set("drive.rotational_period_ms", "200")
                .build();
    }

    private static class RecordingFluxSource extends FluxSource
    {
        final List<Integer> seeks = new ArrayList<>();
        int recalibrations = 0;

        @Override
        public void recalibrate()
        {
            recalibrations++;
        }

        @Override
        public void seek(int cylinder)
        {
            seeks.add(cylinder);
        }

        @Override
        public FluxSourceIterator readFlux(int cylinder, int head)
        {
            return null;
        }
    }

    @Test
    public void getRotationalPeriodFromConfig()
    {
        assertThat(Common.getRotationalPeriodFromConfig(makeConfig())).isEqualTo(200e6);
    }

    @Test
    public void measureDiskRotationUsesConfigPeriod()
    {
        /* The period is set in the config, so no hardware access happens. */
        assertThat(Common.measureDiskRotation(makeConfig())).isEqualTo(200e6);
    }

    @Test
    public void adjustTrackOnErrorNothing()
    {
        ConfigProto config = new ConfigBuilder()
                .set("usb.serial", "test-serial")
                .set("drive.error_behaviour", "NOTHING")
                .build();
        RecordingFluxSource fluxSource = new RecordingFluxSource();

        Common.adjustTrackOnError(fluxSource, 5, config);

        assertThat(fluxSource.recalibrations).isEqualTo(0);
        assertThat(fluxSource.seeks).isEmpty();
    }

    @Test
    public void adjustTrackOnErrorRecalibrate()
    {
        ConfigProto config = new ConfigBuilder()
                .set("usb.serial", "test-serial")
                .set("drive.error_behaviour", "RECALIBRATE")
                .build();
        RecordingFluxSource fluxSource = new RecordingFluxSource();

        Common.adjustTrackOnError(fluxSource, 5, config);

        assertThat(fluxSource.recalibrations).isEqualTo(1);
        assertThat(fluxSource.seeks).isEmpty();
    }

    @Test
    public void adjustTrackOnErrorJiggle()
    {
        ConfigProto config = new ConfigBuilder()
                .set("usb.serial", "test-serial")
                .set("drive.error_behaviour", "JIGGLE")
                .build();
        RecordingFluxSource fluxSource = new RecordingFluxSource();

        Common.adjustTrackOnError(fluxSource, 5, config);
        assertThat(fluxSource.seeks).containsExactly(4);

        Common.adjustTrackOnError(fluxSource, 0, config);
        assertThat(fluxSource.seeks).containsExactly(4, 1);
    }

    @Test
    public void fluxSourceIteratorHolderCaches()
    {
        final int[] reads = {0};
        FluxSource fluxSource = new FluxSource()
        {
            @Override
            public FluxSourceIterator readFlux(int cylinder, int head)
            {
                reads[0]++;
                return new FluxSourceIterator()
                {
                    @Override
                    public boolean hasNext()
                    {
                        return false;
                    }

                    @Override
                    public com.cowlark.fluxengine.data.Fluxmap next()
                    {
                        return null;
                    }
                };
            }
        };

        Common.FluxSourceIteratorHolder holder = new Common.FluxSourceIteratorHolder(fluxSource);

        FluxSourceIterator it1 = holder.getIterator(1, 0);
        FluxSourceIterator it2 = holder.getIterator(1, 0);
        FluxSourceIterator it3 = holder.getIterator(2, 1);

        assertThat(reads[0]).isEqualTo(2);
        assertThat(it1).isSameInstanceAs(it2);
        assertThat(it3).isNotSameInstanceAs(it1);
        assertThat(new CylinderHead(1, 0)).isEqualTo(new CylinderHead(1, 0));
    }
}
