package com.cowlark.fluxengine.fluxsource;

import com.cowlark.fluxengine.config.ConfigBuilder;
import com.cowlark.fluxengine.config.ConfigProto;
import com.cowlark.fluxengine.data.Fluxmap;
import com.cowlark.fluxengine.wiring.FluxEngine;

/**
 * A flux source which generates a synthetic test pattern, ported from
 * lib/fluxsource/testpatternfluxsource.cc.
 */
public class TestPatternFluxSource extends TrivialFluxSource
{
    private final double intervalUs;
    private final double sequenceLengthMs;
    protected ConfigProto extraConfig;

    public TestPatternFluxSource(TestPatternFluxSourceProto config)
    {
        intervalUs = config.getIntervalUs();
        sequenceLengthMs = config.getSequenceLengthMs();

        ConfigProto.Builder builder = ConfigProto.newBuilder();
        builder.getDriveBuilder().setTracks("c0-255h0-1");
        extraConfig = builder.build();
    }

    @Override
    public void adjustConfig(ConfigBuilder configBuilder)
    {
        configBuilder.mergeConfig(extraConfig);
    }

    @Override
    public Fluxmap readSingleFlux(FluxReadParameters parameters)
    {
        Fluxmap fluxmap = new Fluxmap();

        while (fluxmap.durationNs() < (sequenceLengthMs * 1e6))
        {
            fluxmap.appendInterval((int) (intervalUs * FluxEngine.TICKS_PER_US));
            fluxmap.appendPulse();
        }

        return fluxmap;
    }
}