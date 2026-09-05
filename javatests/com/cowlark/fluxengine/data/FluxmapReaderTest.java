package com.cowlark.fluxengine.data;

import static com.google.common.truth.Truth.assertThat;

import com.cowlark.fluxengine.core.Bytes;
import com.cowlark.fluxengine.decoders.DecoderProto;
import com.cowlark.fluxengine.wiring.FluxEngine;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;

@RunWith(JUnit4.class)
public class FluxmapReaderTest
{
    private static final DecoderProto DECODER = DecoderProto.getDefaultInstance();

    @Test
    public void readsEvents()
    {
        Fluxmap map = new Fluxmap(Bytes.of(
                FluxEngine.F_DESYNC,
                FluxEngine.F_BIT_PULSE | 0x30,
                FluxEngine.F_BIT_INDEX | 0x30,
                FluxEngine.F_BIT_PULSE | FluxEngine.F_BIT_INDEX | 0x30,
                FluxEngine.F_DESYNC,
                FluxEngine.F_BIT_PULSE | 0x30,
                FluxEngine.F_DESYNC,
                FluxEngine.F_BIT_PULSE | 0x30));

        FluxmapReader r = new FluxmapReader(map, DECODER);

        assertThat(r.getNextEvent().event()).isEqualTo(FluxEngine.F_DESYNC);
        assertThat(r.getNextEvent().event()).isEqualTo(FluxEngine.F_BIT_PULSE);
        assertThat(r.getNextEvent().event()).isEqualTo(FluxEngine.F_BIT_INDEX);
        assertThat(r.getNextEvent().event()).isEqualTo(
                FluxEngine.F_BIT_PULSE | FluxEngine.F_BIT_INDEX);
        assertThat(r.getNextEvent().event()).isEqualTo(FluxEngine.F_DESYNC);
        assertThat(r.getNextEvent().event()).isEqualTo(FluxEngine.F_BIT_PULSE);
        assertThat(r.getNextEvent().event()).isEqualTo(FluxEngine.F_DESYNC);
        assertThat(r.getNextEvent().event()).isEqualTo(FluxEngine.F_BIT_PULSE);
        assertThat(r.getNextEvent().event()).isEqualTo(FluxEngine.F_EOF);
        assertThat(r.eof()).isTrue();
    }

    @Test
    public void ticksAccumulate()
    {
        Fluxmap map =
                new Fluxmap(Bytes.of(FluxEngine.F_BIT_PULSE | 0x30, FluxEngine.F_BIT_PULSE | 0x30));

        FluxmapReader r = new FluxmapReader(map, DECODER);

        assertThat(r.getNextEvent().ticks()).isEqualTo(0x30L);
        assertThat(r.getNextEvent().ticks()).isEqualTo(0x30L);
        assertThat(r.tell().ticks()).isEqualTo(0x30 + 0x30);
    }

    @Test
    public void findEvent()
    {
        Fluxmap map =
                new Fluxmap(Bytes.of(FluxEngine.F_BIT_PULSE | 0x30, FluxEngine.F_BIT_INDEX | 0x30));

        FluxmapReader r = new FluxmapReader(map, DECODER);

        FluxmapReader.EventResult result = r.findEvent(FluxEngine.F_BIT_INDEX);

        assertThat(result.found()).isTrue();
        assertThat(result.ticks()).isEqualTo(0x60L);
    }

    @Test
    public void findEventNotFound()
    {
        Fluxmap map =
                new Fluxmap(Bytes.of(FluxEngine.F_BIT_PULSE | 0x30, FluxEngine.F_BIT_PULSE | 0x30));

        FluxmapReader r = new FluxmapReader(map, DECODER);

        FluxmapReader.EventResult result = r.findEvent(FluxEngine.F_BIT_INDEX);

        assertThat(result.found()).isFalse();
    }

    @Test
    public void rewindResets()
    {
        Fluxmap map = new Fluxmap(Bytes.of(FluxEngine.F_BIT_PULSE | 0x30));

        FluxmapReader r = new FluxmapReader(map, DECODER);
        r.getNextEvent();
        assertThat(r.eof()).isTrue();

        r.rewind();

        assertThat(r.eof()).isFalse();
        assertThat(r.getNextEvent().event()).isEqualTo(FluxEngine.F_BIT_PULSE);
    }

    @Test
    public void guessClock()
    {
        Fluxmap map = new Fluxmap();
        for (int i = 0; i < 100; i++)
        {
            map.appendInterval(0x30);
            map.appendPulse();
        }

        FluxmapReader r = new FluxmapReader(map, DECODER);
        FluxmapReader.ClockData data = r.guessClock();

        assertThat(data.medianTicks).isEqualTo(0x30L);
    }
}
