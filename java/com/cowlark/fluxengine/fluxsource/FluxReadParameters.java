package com.cowlark.fluxengine.fluxsource;

import lombok.Builder;

/**
 * The parameters for reading flux from a track, passed to
 * {@link FluxSource#readFlux}.
 */
@Builder(setterPrefix = "set")
public record FluxReadParameters
        (int cylinder, int head, boolean syncWithIndex, double readTimeNs,
         double hardSectorThresholdNs)
{
}
