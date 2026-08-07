package com.cowlark.fluxengine.data;

import static com.google.common.truth.Truth.assertThat;

import com.cowlark.fluxengine.core.Bytes;
import java.util.List;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;

@RunWith(JUnit4.class)
public class FluxmapTest
{
    @Test
    public void appendIntervalAndPulse()
    {
        Fluxmap map = new Fluxmap();
        map.appendInterval(0x30);
        map.appendPulse();

        assertThat(map.rawBytes()).isEqualTo(Bytes.of(0x30 | 0x80));
        assertThat(map.ticks()).isEqualTo(0x30);
    }

    @Test
    public void appendIntervalSplitsLargeValues()
    {
        Fluxmap map = new Fluxmap();
        map.appendInterval(100);

        assertThat(map.rawBytes()).isEqualTo(Bytes.of(0x3f, 100 - 0x3f));
        assertThat(map.ticks()).isEqualTo(100);
    }

    @Test
    public void appendIndex()
    {
        Fluxmap map = new Fluxmap();
        map.appendInterval(0x30);
        map.appendIndex();

        assertThat(map.rawBytes()).isEqualTo(Bytes.of(0x30 | 0x40));
    }

    @Test
    public void appendDesync()
    {
        Fluxmap map = new Fluxmap();
        map.appendInterval(0x30);
        map.appendDesync();

        assertThat(map.rawBytes()).isEqualTo(Bytes.of(0x30, 0x00));
    }

    @Test
    public void split()
    {
        Fluxmap map = new Fluxmap();
        map.appendInterval(0x30);
        map.appendDesync();
        map.appendInterval(0x30);
        map.appendPulse();

        List<Fluxmap> parts = map.split();

        assertThat(parts).hasSize(2);
        assertThat(parts.get(0).bytes()).isEqualTo(1);
        assertThat(parts.get(1).bytes()).isEqualTo(1);
    }

    @Test
    public void getIndexMarks()
    {
        Fluxmap map = new Fluxmap();
        map.appendInterval(100);
        map.appendIndex();
        map.appendInterval(50);
        map.appendIndex();

        List<Long> marks = map.getIndexMarks();

        assertThat(marks).containsExactly(100L, 150L);
    }

    @Test
    public void indexMarksFlushOnAppend()
    {
        Fluxmap map = new Fluxmap();
        map.appendInterval(100);
        map.appendIndex();
        map.appendInterval(50);
        map.appendIndex();

        assertThat(map.getIndexMarks()).hasSize(2);

        map.appendInterval(50);
        map.appendIndex();

        assertThat(map.getIndexMarks()).hasSize(3);
    }
}
