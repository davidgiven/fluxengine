package com.cowlark.fluxengine.encoders;

import com.cowlark.fluxengine.config.ConfigProto;
import com.cowlark.fluxengine.core.FluxEngineException;
import com.cowlark.fluxengine.data.CylinderHead;
import com.cowlark.fluxengine.data.Fluxmap;
import com.cowlark.fluxengine.data.Image;
import com.cowlark.fluxengine.data.LogicalTrackLayout;
import com.cowlark.fluxengine.data.Sector;
import com.google.common.collect.ImmutableList;
import java.util.List;

/**
 * A track encoder, ported from lib/encoders/encoders.{h,cc}.
 */
public abstract class Encoder
{
    private final double diskRotationalPeriodNs;

    public Encoder(double diskRotationalPeriodNs)
    {
        this.diskRotationalPeriodNs = diskRotationalPeriodNs;
    }

    public static Encoder create(ConfigProto config)
    {
        throw new FluxEngineException("encoders are not implemented yet");
    }

    public Sector getSector(CylinderHead ch, Image image, int sectorId)
    {
        return image.get(ch.cylinder(), ch.head(), sectorId);
    }

    public ImmutableList<Sector> collectSectors(LogicalTrackLayout ltl, Image image)
    {
        ImmutableList.Builder<Sector> sectors = ImmutableList.builder();

        for (int sectorId : ltl.diskSectorOrder)
        {
            Sector sector = getSector(
                    new CylinderHead(ltl.logicalCylinder, ltl.logicalHead),
                    image,
                    sectorId);
            if (sector == null)
                throw new FluxEngineException(String.format(
                        "sector %d.%d.%d is missing from the image",
                        ltl.logicalCylinder,
                        ltl.logicalHead,
                        sectorId));
            sectors.add(sector);
        }

        return sectors.build();
    }

    public abstract Fluxmap encode(LogicalTrackLayout ltl, List<Sector> sectors, Image image);

    public double calculatePhysicalClockPeriodNs(double targetClockPeriodNs,
                                                 double targetRotationalPeriodNs)
    {
        if (diskRotationalPeriodNs == 0)
            throw new FluxEngineException(
                    "you must set --drive.rotational_period_ms as it can't be autodetected");

        return targetClockPeriodNs * (diskRotationalPeriodNs / targetRotationalPeriodNs);
    }
}
