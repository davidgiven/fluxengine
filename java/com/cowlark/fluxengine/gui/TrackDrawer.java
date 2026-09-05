package com.cowlark.fluxengine.gui;

import static com.cowlark.fluxengine.gui.UiUtils.DATA_BAD_COLOUR;
import static com.cowlark.fluxengine.gui.UiUtils.DATA_OK_COLOUR;
import static com.cowlark.fluxengine.gui.UiUtils.HEADER_COLOUR;

import com.cowlark.fluxengine.data.Disk;
import com.cowlark.fluxengine.data.Sector;
import com.cowlark.fluxengine.data.Track;
import com.google.common.collect.ImmutableSortedSet;
import lombok.Builder;
import java.awt.BasicStroke;
import java.awt.Color;
import java.awt.Graphics2D;
import java.awt.geom.Arc2D;
import java.awt.geom.Point2D;

class TrackDrawer
{
    final Graphics2D graphics2D;
    final Point2D.Double pos;
    final Track track;
    final ImmutableSortedSet<Double> indexMarks;
    final double degreesPerNano;
    final double trackRadius;
    final double trackSpacing;
    double rotationalPeriodNs;
    boolean badData = false;

    @Builder(setterPrefix = "set")
    TrackDrawer(
            Graphics2D graphics2D,
            Point2D.Double position,
            Disk disk,
            Track track,
            double trackRadius,
            double trackSpacing)
    {
        this.graphics2D = graphics2D;
        this.pos = position;
        this.track = track;
        this.trackRadius = trackRadius;
        this.trackSpacing = trackSpacing;

        indexMarks = track.fluxmap.getIndexMarks();
        if (indexMarks.isEmpty())
            badData = true;

        if (indexMarks.size() >= 2)
            rotationalPeriodNs = indexMarks.asList().get(1) - indexMarks.asList().get(0);
        else
        {
            rotationalPeriodNs = disk.rotationalPeriodNs;
            if (rotationalPeriodNs == 0)
                badData = true;
        }

        degreesPerNano = 360.0 / rotationalPeriodNs;
    }

    /* Offset since the index mark preceding the given timestamp,
     * wrapping into the previous rotation when necessary. */
    private double normalisePosition(double timestamp)
    {
        if (timestamp < indexMarks.first())
            return timestamp - (indexMarks.first() - rotationalPeriodNs);

        return timestamp - indexMarks.floor(timestamp);
    }

    void drawArcDegrees(double startDegrees, double endDegrees, Color colour)
    {
        double start = 90 - startDegrees;
        double sweep = -(endDegrees - startDegrees);
        double d = trackRadius * 2;
        graphics2D.setColor(colour);
        graphics2D.setStroke(new BasicStroke((float) (trackSpacing * 0.75)));
        graphics2D.draw(new Arc2D.Double(
                pos.getX() - trackRadius,
                pos.getY() - trackRadius,
                d,
                d,
                start,
                sweep,
                Arc2D.OPEN));
        graphics2D.setStroke(new BasicStroke(1.0f));
    }

    void drawArcNs(double startNs, double endNs, Color colour)
    {
        drawArcDegrees(
                normalisePosition(startNs) * degreesPerNano,
                normalisePosition(endNs) * degreesPerNano,
                colour);
    }

    void drawSectors()
    {
        for (Sector sector : track.normalisedSectors)
        {
            if ((sector.headerStartTimeNs != 0) && (sector.headerEndTimeNs != 0))
                drawArcNs(sector.headerStartTimeNs, sector.headerEndTimeNs, HEADER_COLOUR);
            if ((sector.dataStartTimeNs != 0) && (sector.dataEndTimeNs != 0))
                drawArcNs(
                        sector.dataStartTimeNs,
                        sector.dataEndTimeNs,
                        sector.status == Sector.Status.OK ? DATA_OK_COLOUR : DATA_BAD_COLOUR);
        }
        if (track.normalisedSectors.isEmpty())
            drawArcDegrees(0, 360, DATA_BAD_COLOUR);
    }
}
