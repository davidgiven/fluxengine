package com.cowlark.fluxengine.data;

import static com.google.common.truth.Truth.assertThat;

import com.google.common.collect.ImmutableList;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;

@RunWith(JUnit4.class)
public class FluxPatternTest
{
    /* Ported from tests/fluxpattern.cc. */

    @Test
    public void testPatternConstruction()
    {
        FluxPattern fp1 = new FluxPattern(16, 0x0003);
        assertThat(fp1.getBitCount()).isEqualTo(16);
        assertThat(fp1.getIntervals()).containsExactlyElementsIn(ImmutableList.of(1));

        FluxPattern fp2 = new FluxPattern(16, 0xc000);
        assertThat(fp2.getBitCount()).isEqualTo(16);
        assertThat(fp2.getIntervals()).containsExactlyElementsIn(ImmutableList.of(1, 15));

        FluxPattern fp3 = new FluxPattern(16, 0x0050);
        assertThat(fp3.getBitCount()).isEqualTo(16);
        assertThat(fp3.getIntervals()).containsExactlyElementsIn(ImmutableList.of(2, 5));

        FluxPattern fp4 = new FluxPattern(16, 0x0070);
        assertThat(fp4.getBitCount()).isEqualTo(16);
        assertThat(fp4.getIntervals()).containsExactlyElementsIn(ImmutableList.of(1, 1, 5));

        FluxPattern fp5 = new FluxPattern(16, 0x0070);
        assertThat(fp5.getBitCount()).isEqualTo(16);
        assertThat(fp5.getIntervals()).containsExactlyElementsIn(ImmutableList.of(1, 1, 5));

        FluxPattern fp6 = new FluxPattern(16, 0x0110);
        assertThat(fp6.getBitCount()).isEqualTo(16);
        assertThat(fp6.getIntervals()).containsExactlyElementsIn(ImmutableList.of(4, 5));
    }

    @Test
    public void testPatternMatchingWithoutTrailingZeroes()
    {
        FluxPattern fp = new FluxPattern(16, 0x000b);
        final long[] matching = {100, 100, 200, 100};
        final long[] notMatching = {100, 200, 100, 100};
        final long[] closeMatch1 = {90, 90, 180, 90};
        final long[] closeMatch2 = {110, 110, 220, 110};

        FluxMatch match = new FluxMatch();
        assertThat(fp.matches(matching, 4, 0.40, match)).isTrue();
        assertThat(match.intervals).isEqualTo(2);

        assertThat(fp.matches(notMatching, 4, 0.40, match)).isFalse();

        assertThat(fp.matches(closeMatch1, 4, 0.40, match)).isTrue();
        assertThat(match.intervals).isEqualTo(2);

        assertThat(fp.matches(closeMatch2, 4, 0.40, match)).isTrue();
        assertThat(match.intervals).isEqualTo(2);
    }

    @Test
    public void testPatternMatchingWithTrailingZeroes()
    {
        FluxPattern fp = new FluxPattern(16, 0x0016);
        final long[] matching = {100, 100, 200, 100, 200};
        final long[] notMatching = {100, 200, 100, 100, 100};
        final long[] closeMatch1 = {90, 90, 180, 90, 300};
        final long[] closeMatch2 = {110, 110, 220, 110, 220};

        FluxMatch match = new FluxMatch();
        assertThat(fp.matches(matching, 5, 0.40, match)).isTrue();
        assertThat(match.intervals).isEqualTo(3);

        assertThat(fp.matches(notMatching, 5, 0.40, match)).isFalse();

        assertThat(fp.matches(closeMatch1, 5, 0.40, match)).isTrue();
        assertThat(match.intervals).isEqualTo(3);

        assertThat(fp.matches(closeMatch2, 5, 0.40, match)).isTrue();
        assertThat(match.intervals).isEqualTo(3);
    }
}