package com.cowlark.fluxengine.gui;

import static swingtree.UI.label;
import static swingtree.UI.of;

import com.cowlark.fluxengine.data.CylinderHead;
import com.cowlark.fluxengine.data.Disk;
import com.cowlark.fluxengine.data.DiskLayout;
import com.cowlark.fluxengine.data.Sector;
import com.cowlark.fluxengine.data.Sector.Status;
import com.google.common.collect.ImmutableSet;
import sprouts.From;
import sprouts.Viewable;
import swingtree.UI;
import swingtree.UI.HorizontalAlignment;
import swingtree.UI.VerticalAlignment;
import swingtree.UIForPanel;
import javax.swing.JButton;
import javax.swing.JPanel;
import javax.swing.UIManager;
import java.awt.Color;
import java.awt.FontMetrics;
import java.awt.Graphics;
import java.awt.GridLayout;

public class SummaryPanel extends JPanel
{
    private final ImagerViewModel model;
    private final int smallFont;

    public SummaryPanel(ImagerViewModel model)
    {
        this.model = model;
        smallFont = UIManager.getFont("Label.font").getSize() / 3;

        Viewable.cast(model.getDisk()).onChange(From.ALL, it -> rebuildUi());
        Viewable.cast(model.getDriveActivity()).onChange(From.ALL, it -> rebuildUi());
        rebuildUi();
    }

    private void rebuildUi()
    {
        removeAll();

        DiskLayout layout = model.getDisk().get().diskLayout;
        if (layout != null)
        {
            UIForPanel<SummaryPanel> ui = UI.of(this).withLayout(new GridLayout(6, 1, 1, 1));
            ui = addPhysicalView(ui);
            ui = addLogicalView(ui);
        }

        revalidate();
        repaint();
    }

    private static class TrackAnalysis
    {
        String tooltip;
        Color colour;
        String label;
    }

    private ImmutableSet<Sector> findSectors(Disk disk, int physicalCylinder, int physicalHead)
    {
        return ImmutableSet.copyOf(disk.sectorsByPhysicalLocation.get(new CylinderHead(
                physicalCylinder,
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
        result.colour = getBackground();
        result.tooltip = "No data";
        result.label = "";
        if (!sectors.isEmpty())
        {
            int totalSectors = sectors.size();
            int goodSectors = (int) sectors.stream().filter(it -> it.status == Status.OK).count();
            int badSectors = totalSectors - goodSectors;

            if ((goodSectors == totalSectors) && (goodSectors != 0) && (totalSectors != 0))
                result.colour = UiUtils.colorForStatus(Status.OK);
            else
                result.colour = UiUtils.colorForStatus(Status.BAD_CHECKSUM);

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

    private UIForPanel<SummaryPanel> addPhysicalView(UIForPanel<SummaryPanel> ui)
    {
        Disk disk = model.getDisk().get();
        DiskLayout layout = disk.diskLayout;

        return ui.add(label("Physical layout (what your drive sees)").withHorizontalAlignment(
                        HorizontalAlignment.CENTER))
                .add(of(makeHeaderPanel(layout.minPhysicalCylinder, layout.maxPhysicalCylinder)))
                .add(of(makeGridPanel(layout.minPhysicalCylinder,
                        layout.maxPhysicalCylinder,
                        layout.minPhysicalHead,
                        layout.maxPhysicalHead,
                        ((cylinder, head) -> new SummaryButton(analyseTrack(disk,
                                model.getDriveActivity().get(),
                                cylinder,
                                head))))));
    }

    private UIForPanel<SummaryPanel> addLogicalView(UIForPanel<SummaryPanel> ui)
    {
        Disk disk = model.getDisk().get();
        DiskLayout layout = disk.diskLayout;

        return ui.add(label("Logical layout (what's on the disk)").withHorizontalAlignment(
                HorizontalAlignment.CENTER));
    }

    private JPanel makeHeaderPanel(int minCylinder, int maxCylinder)
    {
        JPanel panel = new JPanel(new GridLayout(1, maxCylinder - minCylinder + 2, 1, 1));
        UIForPanel<JPanel> ui = UI.of(panel).add(label("").withFontSize(smallFont));
        for (int cylinder = minCylinder; cylinder <= maxCylinder; cylinder++)
            ui = ui.add(label(String.format("c%d", cylinder)).withHorizontalAlignment(
                            HorizontalAlignment.CENTER)
                    .withVerticalAlignment(VerticalAlignment.BOTTOM)
                    .withFontSize(smallFont));
        return panel;
    }

    private JPanel makeGridPanel(
            int minCylinder,
            int maxCylinder,
            int minHead,
            int maxHead,
            ButtonBuilder builder)
    {
        JPanel panel = new JPanel(new GridLayout(maxHead - minHead + 1,
                maxCylinder - minCylinder + 2,
                1,
                1));
        UIForPanel<JPanel> ui = UI.of(panel);
        for (int head = minHead; head <= maxHead; head++)
        {
            ui = ui.add(label(String.format("h%d", head)).withFontSize(smallFont)
                    .withHorizontalAlignment(HorizontalAlignment.CENTER));
            for (int cylinder = minCylinder; cylinder <= maxCylinder; cylinder++)
                ui = ui.add(of(builder.create(cylinder, head)).withMinSize(5, 5));
        }
        return panel;
    }

    /* A single cell of the summary grid. */
    private static class SummaryButton extends JButton
    {
        private final TrackAnalysis analysis;

        SummaryButton(TrackAnalysis analysis)
        {
            super("");
            this.analysis = analysis;
            setBorderPainted(false);
            setContentAreaFilled(false);
            setFocusPainted(false);
            setOpaque(true);
            setBackground(analysis.colour);
            setToolTipText(analysis.tooltip);
        }

        @Override
        protected void paintComponent(Graphics g)
        {
            g.setColor(analysis.colour);
            g.fillRect(0, 0, getWidth(), getHeight());

            if (!analysis.label.isEmpty())
            {
                g.setColor(getForeground());
                FontMetrics metrics = g.getFontMetrics();
                int x = (getWidth() - metrics.stringWidth(analysis.label)) / 2;
                int y = (getHeight() - metrics.getHeight()) / 2 + metrics.getAscent();
                g.drawString(analysis.label, x, y);
            }
        }
    }

    interface ButtonBuilder
    {
        SummaryButton create(int cylinder, int head);
    }
}
