package com.cowlark.fluxengine.gui;

import static com.cowlark.fluxengine.gui.UiUtils.getGraphics2D;
import static swingtree.UI.label;
import static swingtree.UI.of;

import com.cowlark.fluxengine.data.CylinderHead;
import com.cowlark.fluxengine.data.Disk;
import com.cowlark.fluxengine.data.DiskLayout;
import com.cowlark.fluxengine.data.LogicalTrackLayout;
import com.cowlark.fluxengine.data.PhysicalTrackLayout;
import com.cowlark.fluxengine.data.Sector;
import com.cowlark.fluxengine.data.Sector.Status;
import com.google.common.collect.ImmutableSet;
import sprouts.From;
import sprouts.Viewable;
import swingtree.UI.HorizontalAlignment;
import swingtree.UI.VerticalAlignment;
import swingtree.UIForPanel;
import javax.swing.JButton;
import javax.swing.JComponent;
import javax.swing.JPanel;
import javax.swing.UIManager;
import java.awt.Color;
import java.awt.Dimension;
import java.awt.FontMetrics;
import java.awt.Graphics;
import java.awt.Graphics2D;
import java.awt.Insets;
import java.util.HashMap;

public class SummaryPanel extends JPanel
{
    private final ImagerViewModel model;
    private final int smallFont;
    private DiskLayout currentLayout;
    private HashMap<CylinderHead, SummaryButton> physicalTrackButtons = new HashMap<>();
    private HashMap<CylinderHead, SummaryButton> logicalTrackButtons = new HashMap<>();

    public SummaryPanel(ImagerViewModel model)
    {
        this.model = model;
        smallFont = UIManager.getFont("Label.font").getSize() / 3;

        Viewable.cast(model.getDisk()).onChange(From.ALL, it -> diskChanged());
        Viewable.cast(model.getDriveActivity()).onChange(From.ALL, it -> updateUi());

        currentLayout = model.getDisk().get().diskLayout;
        rebuildUi();
    }

    private void diskChanged()
    {
        DiskLayout newLayout = model.getDisk().get().diskLayout;
        if ((currentLayout == null) || !currentLayout.equals(newLayout))
        {
            currentLayout = newLayout;
            rebuildUi();
        }
        updateUi();
    }

    private void updateUi()
    {
        if (currentLayout == null)
            return;

        Disk disk = model.getDisk().get();
        DriveActivity driveActivity = model.getDriveActivity().get();
        for (int head = currentLayout.minPhysicalHead; head <= currentLayout.maxPhysicalHead;
             head++)
        {
            for (int cylinder = currentLayout.minPhysicalCylinder;
                 cylinder <= currentLayout.maxPhysicalCylinder; cylinder++)
            {
                SummaryButton button = physicalTrackButtons.get(new CylinderHead(cylinder, head));
                if (button == null)
                    continue;
                button.setTrackAnalysis(analyseTrack(disk, driveActivity, cylinder, head));
            }
        }

        for (int head = 0; head < currentLayout.numLogicalHeads; head++)
        {
            for (int cylinder = 0; cylinder < currentLayout.numLogicalCylinders; cylinder++)
            {
                CylinderHead lch = new CylinderHead(cylinder, head);
                LogicalTrackLayout ptl = currentLayout.layoutByLogicalLocation.get(lch);
                if (ptl == null)
                    continue;

                SummaryButton button = logicalTrackButtons.get(lch);
                if (button == null)
                    continue;
                button.setTrackAnalysis(analyseTrack(
                        disk,
                        driveActivity,
                        ptl.physicalCylinder,
                        ptl.physicalHead));
            }
        }

        revalidate();
        repaint();
    }

    private void rebuildUi()
    {
        removeAll();

        if (currentLayout != null)
        {
            physicalTrackButtons.clear();
            logicalTrackButtons.clear();

            int columns =
                    currentLayout.maxPhysicalCylinder - currentLayout.minPhysicalCylinder + 1 + 2;
            UIForPanel<SummaryPanel> ui = of(this).withLayout(
                    "wrap " + columns + ",gap 1 1,fill",
                    "[sizegroup 1,grow,fill]",
                    "grow,fill");

            ui = ui.add(
                    "span " + columns + ", growx",
                    label("Physical layout (what your drive sees)").withHorizontalAlignment(
                            HorizontalAlignment.CENTER));
            ui = addHeader(ui, VerticalAlignment.BOTTOM);
            ui = addButtonRows(
                    ui,
                    currentLayout.minPhysicalHead,
                    currentLayout.maxPhysicalHead,
                    this::createPhysicalButton);

            ui = ui.add(
                    "span " + columns + ", growx, gaptop 5pt",
                    label("Logical layout (what's on the disk)").withHorizontalAlignment(
                            HorizontalAlignment.CENTER));
            ui = addButtonRows(
                    ui,
                    currentLayout.minPhysicalHead,
                    currentLayout.maxPhysicalHead,
                    this::createLogicalButton);
            ui = addHeader(ui, VerticalAlignment.TOP);
        }
    }

    private ButtonAndSpan createPhysicalButton(int cylinder, int head)
    {
        SummaryButton button = new SummaryButton();
        physicalTrackButtons.put(new CylinderHead(cylinder, head), button);
        return new ButtonAndSpan(button, 1);
    }

    private ButtonAndSpan createLogicalButton(int cylinder, int head)
    {
        CylinderHead pch = new CylinderHead(cylinder, head);
        PhysicalTrackLayout ptl = currentLayout.layoutByPhysicalLocation.get(pch);
        if ((ptl == null) || (ptl.groupOffset != 0))
            return new ButtonAndSpan(new JPanel(), 1);

        SummaryButton button = new SummaryButton();
        logicalTrackButtons.put(
                new CylinderHead(
                        ptl.logicalTrackLayout.logicalCylinder,
                        ptl.logicalTrackLayout.logicalHead),
                button);
        return new ButtonAndSpan(button, ptl.logicalTrackLayout.groupSize);
    }

    private UIForPanel<SummaryPanel> addHeader(
            UIForPanel<SummaryPanel> ui,
            VerticalAlignment verticalAlignment)
    {
        ui = ui.add(label("").withFontSize(smallFont));
        for (int cylinder = currentLayout.minPhysicalCylinder;
             cylinder <= currentLayout.maxPhysicalCylinder; cylinder++)
            ui = ui.add(label(String.format("c%d", cylinder))
                    .withHorizontalAlignment(HorizontalAlignment.CENTER)
                    .withVerticalAlignment(verticalAlignment)
                    .withFontSize(smallFont));
        ui = ui.add(label("").withFontSize(smallFont));
        return ui;
    }

    private UIForPanel<SummaryPanel> addButtonRows(
            UIForPanel<SummaryPanel> ui,
            int minHead,
            int maxHead,
            ButtonBuilder builder)
    {
        for (int head = minHead; head <= maxHead; head++)
        {
            ui = ui.add(label(String.format("h%d", head))
                    .withFontSize(smallFont)
                    .withPrefSize(0, smallFont)
                    .withHorizontalAlignment(HorizontalAlignment.CENTER));

            for (int cylinder = currentLayout.minPhysicalCylinder;
                 cylinder <= currentLayout.maxPhysicalCylinder; )
            {
                ButtonAndSpan result = builder.create(cylinder, head);
                ui = ui.add(
                        "span " + result.span() + ", growx, growy",
                        of(result.button()).withMinSize(5, 5));
                cylinder += result.span();
            }

            ui = ui.add(label(String.format("h%d", head))
                    .withFontSize(smallFont)
                    .withPrefSize(0, smallFont)
                    .withHorizontalAlignment(HorizontalAlignment.CENTER));
        }
        return ui;
    }

    private ImmutableSet<Sector> findSectors(Disk disk, int physicalCylinder, int physicalHead)
    {
        return ImmutableSet.copyOf(disk.sectorsByPhysicalLocation.get(new CylinderHead(physicalCylinder,
                physicalHead)));
    }

    private TrackAnalysis analyseTrack(
            Disk disk,
            DriveActivity activity,
            int physicalCylinder,
            int physicalHead)
    {
        ImmutableSet<Sector> sectors = findSectors(disk, physicalCylinder, physicalHead);
        TrackAnalysis result = new TrackAnalysis();
        result.colour = StatusColour.MISSING.getColour();
        result.tooltip = "No data";
        result.label = "";
        if (!sectors.isEmpty())
        {
            int totalSectors = sectors.size();
            int goodSectors = (int) sectors.stream().filter(it -> it.status == Status.OK).count();
            int badSectors = totalSectors - goodSectors;

            if ((goodSectors == totalSectors) && (goodSectors != 0) && (totalSectors != 0))
                result.colour = StatusColour.OK.getColour();
            else
                result.colour = StatusColour.BAD.getColour();

            result.tooltip = String.format(
                    "c%dh%d\n%d sectors read\n%d good sectors\n%d bad sectors",
                    physicalCylinder,
                    physicalHead,
                    totalSectors,
                    goodSectors,
                    badSectors);
        }

        if ((physicalCylinder == activity.cylinder()) && (physicalHead == activity.head()))
            result.label = switch (activity.type())
            {
                case IDLE -> "";
                case READING -> "R";
                case WRITING -> "W";
            };

        return result;
    }

    interface ButtonBuilder
    {
        ButtonAndSpan create(int cylinder, int head);
    }

    private static class TrackAnalysis
    {
        String tooltip;
        Color colour;
        String label;
    }

    /* A single cell of the summary grid. */
    private static class SummaryButton extends JButton
    {
        private TrackAnalysis analysis;

        SummaryButton()
        {
            super("");
            setBorderPainted(false);
            setContentAreaFilled(false);
            setFocusPainted(false);
            setOpaque(true);
            setMargin(new Insets(0, 0, 0, 0));
            setMinimumSize(new Dimension(10, 10));
            setPreferredSize(new Dimension(15, 15));
            setBackground(StatusColour.NOT_PRESENT.getColour());
        }

        void setTrackAnalysis(TrackAnalysis analysis)
        {
            this.analysis = analysis;
            setToolTipText(analysis.tooltip);
            repaint();
        }

        @Override
        protected void paintComponent(Graphics g)
        {
            if (analysis != null)
            {
                g.setColor(analysis.colour);
                g.fillRect(0, 0, getWidth(), getHeight());

                if (!analysis.label.isEmpty())
                {
                    Graphics2D g2 = getGraphics2D(g);
                    g2.setXORMode(Color.WHITE);
                    FontMetrics metrics = g2.getFontMetrics();
                    int x = (getWidth() - metrics.stringWidth(analysis.label)) / 2;
                    int y = (getHeight() - metrics.getHeight()) / 2 + metrics.getAscent();
                    g2.drawString(analysis.label, x, y);
                }
            }
        }
    }

    record ButtonAndSpan(JComponent button, int span)
    {
    }
}
