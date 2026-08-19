package com.cowlark.fluxengine.external;

import static com.google.common.truth.Truth.assertThat;

import com.cowlark.fluxengine.core.Bytes;
import com.cowlark.fluxengine.external.Flx;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;

/**
 * Unit tests for {@link com.cowlark.fluxengine.data.Flx#readFlxBytes}, ported
 * from tests/flx.cc.
 */
@RunWith(JUnit4.class)
public class FlxTest
{
    @org.junit.Rule public final org.junit.rules.TestRule loggerRule =
            com.cowlark.fluxengine.testing.TestHelpers.loggerRule();

    private static void assertConvert(Bytes flx, Bytes expected)
    {
        assertThat(Flx.readFlxBytes(flx).rawBytes()).isEqualTo(expected);
    }

    @Test
    public void streamReader()
    {
        /* Header only: no flux. */
        assertConvert(Bytes.of(0), Bytes.of());

        /* Simple one-byte intervals. */
        assertConvert(Bytes.of(0, 0x64, Flx.FLX_STOP), Bytes.of(0xb0));

        /* Index pulse. */
        assertConvert(Bytes.of(0, 0x64, Flx.FLX_INDEX, 0x64, Flx.FLX_STOP), Bytes.of(0xf0, 0xb0));
    }
}
