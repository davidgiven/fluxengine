package com.cowlark.fluxengine.cli;

import static com.google.common.truth.Truth.assertThat;

import com.cowlark.fluxengine.data.Fluxmap;
import com.cowlark.fluxengine.data.FluxmapReader;
import com.cowlark.fluxengine.decoders.DecoderProto;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;

@RunWith(JUnit4.class)
public class InspectCommandTest
{
    @Test
    public void guessClockDetectsTightClock()
    {
        /* Pulses every 12 ticks, giving a 12-tick clock. */
        Fluxmap fluxmap = new Fluxmap();
        for (int i = 0; i < 5000; i++)
        {
            fluxmap.appendInterval(12);
            fluxmap.appendPulse();
        }

        FluxmapReader fmr = new FluxmapReader(fluxmap, DecoderProto.getDefaultInstance());
        FluxmapReader.ClockData data = fmr.guessClock(0.01, 0.05);

        assertThat(data.medianTicks).isEqualTo(12);
        assertThat(data.buckets[12]).isGreaterThan(0);
    }

    @Test
    public void guessClockSkipsLongIntervals()
    {
        /* Intervals longer than 255 ticks are skipped by the histogram. */
        Fluxmap fluxmap = new Fluxmap();
        for (int i = 0; i < 100; i++)
        {
            fluxmap.appendInterval(300);
            fluxmap.appendPulse();
        }

        FluxmapReader fmr = new FluxmapReader(fluxmap, DecoderProto.getDefaultInstance());
        FluxmapReader.ClockData data = fmr.guessClock(0.01, 0.05);

        int total = 0;
        for (int b : data.buckets)
            total += b;
        assertThat(total).isEqualTo(0);
    }
}
