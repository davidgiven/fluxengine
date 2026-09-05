package com.cowlark.fluxengine.data;

import static com.google.common.truth.Truth.assertThat;

import com.cowlark.fluxengine.core.Bytes;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;

@RunWith(JUnit4.class)
public class RecordTest
{
    @Test
    public void defaultsAreEmptyRecord()
    {
        Record record = new Record();

        assertThat(record.clockNs).isEqualTo(0.0);
        assertThat(record.startTimeNs).isEqualTo(0.0);
        assertThat(record.endTimeNs).isEqualTo(0.0);
        assertThat(record.position).isEqualTo(0);
        assertThat(record.rawData.isEmpty()).isTrue();
    }

    @Test
    public void holdsFields()
    {
        Record record = new Record();
        record.clockNs = 123.0;
        record.startTimeNs = 456.0;
        record.endTimeNs = 789.0;
        record.position = 42;
        record.rawData = Bytes.of(0x11, 0x22);

        assertThat(record.clockNs).isEqualTo(123.0);
        assertThat(record.startTimeNs).isEqualTo(456.0);
        assertThat(record.endTimeNs).isEqualTo(789.0);
        assertThat(record.position).isEqualTo(42);
        assertThat(record.rawData).isEqualTo(Bytes.of(0x11, 0x22));
    }
}