package com.cowlark.fluxengine.algorithms;

import com.cowlark.fluxengine.data.CylinderHead;
import com.cowlark.fluxengine.fluxsource.FluxSource;
import com.cowlark.fluxengine.fluxsource.FluxSourceIterator;
import java.util.HashMap;
import java.util.Map;

class Common
{
    static void testForEmergencyStop()
    {
    }

    static class FluxSourceIteratorHolder
    {
        private final FluxSource fluxSource;
        private final Map<CylinderHead, FluxSourceIterator> cache = new HashMap<>();

        FluxSourceIteratorHolder(FluxSource fluxSource)
        {
            this.fluxSource = fluxSource;
        }

        FluxSourceIterator getIterator(int physicalCylinder, int head)
        {
            CylinderHead key = new CylinderHead(physicalCylinder, head);
            FluxSourceIterator it = cache.get(key);
            if (it == null)
            {
                it = fluxSource.readFlux(physicalCylinder, head);
                cache.put(key, it);
            }
            return it;
        }
    }
}
