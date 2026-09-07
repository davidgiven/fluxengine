package com.cowlark.fluxengine.fluxsource;

import com.cowlark.fluxengine.config.ConfigProto;
import com.cowlark.fluxengine.data.Fluxmap;

/**
 * A flux source which produces no flux, ported from
 * lib/fluxsource/erasefluxsource.cc.
 */
public class EraseFluxSource extends TrivialFluxSource
{
    protected ConfigProto extraConfig;

    public EraseFluxSource(EraseFluxSourceProto config)
    {
        ConfigProto.Builder builder = ConfigProto.newBuilder();
        builder.getDriveBuilder().setTracks("c0-255h0-1");
        extraConfig = builder.build();
    }

    @Override
    public ConfigProto getExtraConfig()
    {
        return extraConfig;
    }

    @Override
    public Fluxmap readSingleFlux(FluxReadParameters parameters)
    {
        return null;
    }
}
