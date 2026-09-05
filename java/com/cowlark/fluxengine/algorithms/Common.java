package com.cowlark.fluxengine.algorithms;

import com.cowlark.fluxengine.core.EmergencyStopException;
import com.cowlark.fluxengine.data.CylinderHead;
import com.cowlark.fluxengine.fluxsource.FluxReadParameters;
import com.cowlark.fluxengine.fluxsource.FluxSource;
import com.cowlark.fluxengine.fluxsource.FluxSourceIterator;
import java.util.HashMap;
import java.util.Map;

class Common
{
    /* If set, any running job will terminate as soon as possible (with an
     * error), ported from lib/core/utils.cc. */
    private static volatile boolean emergencyStop = false;

    static void testForEmergencyStop() throws EmergencyStopException
    {
        if (emergencyStop)
            throw new EmergencyStopException();
    }

    static void setEmergencyStop(boolean value)
    {
        emergencyStop = value;
    }

    static class FluxSourceIteratorHolder
    {
        private final FluxSource fluxSource;
        private final Map<CylinderHead, FluxSourceIterator> cache = new HashMap<>();

        FluxSourceIteratorHolder(FluxSource fluxSource)
        {
            this.fluxSource = fluxSource;
        }

        FluxSourceIterator getIterator(FluxReadParameters parameters)
        {
            CylinderHead key = new CylinderHead(parameters.cylinder(), parameters.head());
            FluxSourceIterator it = cache.get(key);
            if (it == null)
            {
                it = fluxSource.readFlux(parameters);
                cache.put(key, it);
            }
            return it;
        }
    }
}
