package com.cowlark.fluxengine.gui;

import static com.cowlark.fluxengine.gui.UiUtils.dimensionPts;

import com.cowlark.fluxengine.data.CylinderHead;
import com.cowlark.fluxengine.data.Disk;
import com.cowlark.fluxengine.data.DiskLayout;
import com.cowlark.fluxengine.data.Track;
import sprouts.From;
import sprouts.Viewable;
import javax.swing.JPanel;
import javax.swing.UIManager;
import java.awt.Color;
import java.awt.FontMetrics;
import java.awt.Graphics;
import java.awt.Graphics2D;
import java.awt.event.ComponentEvent;
import java.awt.event.ComponentListener;
import java.awt.geom.Point2D;
import java.util.Collection;
import java.util.Map;

public class VisualiserPanel extends JPanel implements ComponentListener
{
    public static final double WINDOW_PADDING_X = 8;
    public static final double WINDOW_PADDING_Y = 8;
    private static final float INNER_RADIUS = 50;

    private final ImagerViewModel model;
    private DiskLayout currentLayout;

    private int width;
    private int height;
    private Color fg;
    private Color bg;
    private Color diskbg;
    private double centreX;
    private double centreY;
    private double outerRadius;
    private Point2D.Double side0pos;
    private Point2D.Double side1pos;

    public VisualiserPanel(ImagerViewModel model)
    {
        this.model = model;

        setPreferredSize(dimensionPts(300, 600));
        addComponentListener(this);

        Viewable.cast(model.getDisk()).onChange(From.ALL, it -> diskChanged());
    }

    private void diskChanged()
    {
        DiskLayout newLayout = model.getDisk().get().diskLayout;
        if ((currentLayout == null) || !currentLayout.equals(newLayout))
            currentLayout = newLayout;
        invalidate();
        repaint();
    }

    private void updateGeometry()
    {
        width = getWidth();
        height = getHeight();

        fg = getForeground();
        bg = getBackground();
        Color uiPanelBackground = UIManager.getColor("Panel.background");
        diskbg = (uiPanelBackground != null) ? uiPanelBackground : bg;

        centreX = width / 2.0;
        centreY = height / 2.0;
        outerRadius = (width - WINDOW_PADDING_X * 2) / 2;
        side0pos = new Point2D.Double(centreX, centreY - outerRadius - WINDOW_PADDING_Y);
        side1pos = new Point2D.Double(centreX, centreY + outerRadius + WINDOW_PADDING_Y);
    }

    @Override
    public void componentHidden(ComponentEvent e)
    {

    }

    @Override
    public void componentMoved(ComponentEvent e)
    {

    }

    @Override
    public void componentResized(ComponentEvent e)
    {
        updateGeometry();
    }

    @Override
    public void componentShown(ComponentEvent e)
    {

    }

    private void drawCentered(Graphics2D g2, Point2D pos, Color colour, String s)
    {
        FontMetrics metrics = g2.getFontMetrics();
        g2.setColor(colour);
        g2.drawString(
                s,
                (float) (pos.getX() - metrics.stringWidth(s) / 2.0),
                (float) (pos.getY() - metrics.getHeight() / 2.0 + metrics.getAscent()));
    }

    private void drawSide(Graphics2D g2, int head, Point2D.Double pos)
    {
        g2.setColor(diskbg);
        g2.fillOval(
                (int) (pos.getX() - outerRadius),
                (int) (pos.getY() - outerRadius),
                (int) (outerRadius * 2),
                (int) (outerRadius * 2));
        g2.setColor(bg);
        g2.fillOval(
                (int) (pos.getX() - INNER_RADIUS),
                (int) (pos.getY() - INNER_RADIUS),
                (int) (INNER_RADIUS * 2),
                (int) (INNER_RADIUS * 2));
        g2.setColor(fg);
        g2.drawOval(
                (int) (pos.getX() - outerRadius),
                (int) (pos.getY() - outerRadius),
                (int) (outerRadius * 2),
                (int) (outerRadius * 2));
        g2.drawOval(
                (int) (pos.getX() - INNER_RADIUS),
                (int) (pos.getY() - INNER_RADIUS),
                (int) (INNER_RADIUS * 2),
                (int) (INNER_RADIUS * 2));
        drawCentered(g2, pos, fg, String.format("h%d", head));

        Disk disk = model.getDisk().get();
        DiskLayout diskLayout = (disk == null) ? null : disk.diskLayout;
        if ((diskLayout == null) || (disk == null))
            return;

        int numPhysicalTracks = diskLayout.maxPhysicalCylinder - diskLayout.minPhysicalCylinder;
        double trackSpacing = (outerRadius - INNER_RADIUS) / (numPhysicalTracks + 2);

        for (Map.Entry<CylinderHead, Collection<Track>> e : disk.tracksByPhysicalLocation
                .asMap()
                .entrySet())
        {
            CylinderHead ch = e.getKey();
            if (ch.head() != head)
                continue;

            for (Track t : e.getValue())
            {
                if (t.fluxmap == null)
                    continue;

                TrackDrawer drawer = TrackDrawer
                        .builder()
                        .setGraphics2D(g2)
                        .setPosition(pos)
                        .setDisk(disk)
                        .setTrack(t)
                        .setTrackRadius(outerRadius - (ch.cylinder() + 0.5) * trackSpacing)
                        .setTrackSpacing(trackSpacing)
                        .build();
                if (drawer.badData)
                    continue;

                drawer.drawSectors();
            }
        }

        g2.setColor(UiUtils.INDEX_LINE_COLOUR);
        g2.drawLine(
                (int) pos.getX(),
                (int) (pos.getY() - INNER_RADIUS),
                (int) pos.getX(),
                (int) (pos.getY() - outerRadius));
    }

    @Override
    protected void paintComponent(Graphics g)
    {
        super.paintComponent(g);
        Graphics2D g2 = UiUtils.getGraphics2D(g);
        try
        {
            drawSide(g2, 0, side0pos);
            drawSide(g2, 1, side1pos);
        } finally
        {
            g2.dispose();
        }
    }
}
