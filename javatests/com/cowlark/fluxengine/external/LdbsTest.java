package com.cowlark.fluxengine.external;

import static com.google.common.truth.Truth.assertThat;

import com.cowlark.fluxengine.core.Bytes;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;

/* Tests for the LDBS container format, ported from tests/ldbs.cc. */
@RunWith(JUnit4.class)
public class LdbsTest
{
    private static final Bytes TEST_DATA = Bytes.of(
            'L',
            'B',
            'S',
            0x01,
            'D',
            'S',
            'K',
            0x02,
            0x29,
            0x00,
            0x00,
            0x00,
            0x00,
            0x00,
            0x00,
            0x00,
            0x34,
            0x12,
            0x00,
            0x00,
            'L',
            'D',
            'B',
            0x01,
            0x00,
            0x00,
            0x00,
            0x01,
            0x01,
            0x00,
            0x00,
            0x00,
            0x01,
            0x00,
            0x00,
            0x00,
            0x00,
            0x00,
            0x00,
            0x00,
            0x01,
            'L',
            'D',
            'B',
            0x01,
            0x00,
            0x00,
            0x00,
            0x02,
            0x01,
            0x00,
            0x00,
            0x00,
            0x01,
            0x00,
            0x00,
            0x00,
            0x14,
            0x00,
            0x00,
            0x00,
            0x02);

    @Test
    public void getSet()
    {
        Ldbs ldbs = new Ldbs();

        int block1 = ldbs.put(Bytes.of(1), 1);
        int block2 = ldbs.put(Bytes.of(2), 2);
        assertThat(block1).isNotEqualTo(block2);

        assertThat(ldbs.get(block1)).isEqualTo(Bytes.of(1));
        assertThat(ldbs.get(block2)).isEqualTo(Bytes.of(2));
    }

    @Test
    public void write()
    {
        Ldbs ldbs = new Ldbs();

        ldbs.put(Bytes.of(1), 1);
        ldbs.put(Bytes.of(2), 2);
        Bytes data = ldbs.write(0x1234);

        assertThat(data).isEqualTo(TEST_DATA);
    }

    @Test
    public void read()
    {
        Ldbs ldbs = new Ldbs();
        int trackDirectory = ldbs.read(TEST_DATA);

        assertThat(trackDirectory).isEqualTo(0x1234);
        assertThat(ldbs.get(0x14)).isEqualTo(Bytes.of(1));
        assertThat(ldbs.get(0x29)).isEqualTo(Bytes.of(2));
    }
}
