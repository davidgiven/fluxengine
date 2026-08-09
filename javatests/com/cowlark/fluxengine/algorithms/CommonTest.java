package com.cowlark.fluxengine.algorithms;

import static com.google.common.truth.Truth.assertThat;

import com.cowlark.fluxengine.data.CylinderHead;
import com.cowlark.fluxengine.fluxsource.FluxSource;
import com.cowlark.fluxengine.fluxsource.FluxSourceIterator;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;

@RunWith(JUnit4.class)
public class CommonTest
{
    private static class RecordingFluxSource extends FluxSource
    {
        @Override
        public FluxSourceIterator readFlux(int cylinder, int head)
        {
            return null;
        }
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

    @Test
    public void testForEmergencyStopDoesNotThrow()
    {
        Common.testForEmergencyStop();
    }
}
