package com.cowlark.fluxengine.gui;

import com.cowlark.fluxengine.data.Disk;
import com.cowlark.fluxengine.data.DiskLayout;
import sprouts.From;
import sprouts.Viewable;
import javax.swing.JPanel;
import java.awt.Color;
import java.awt.FontMetrics;
import java.awt.Graphics;
import java.awt.Graphics2D;
import java.awt.RenderingHints;

public class SummaryPanel extends JPanel
{
    private final ImagerViewModel model;

    public SummaryPanel(ImagerViewModel model)
    {
        this.model = model;

        Viewable.cast(model.getDisk()).onChange(From.ALL, it -> repaint());
    }

    @Override
    public void paint(Graphics g)
    {
        Graphics2D g2 = (Graphics2D) g;
        g2.setRenderingHint(RenderingHints.KEY_TEXT_ANTIALIASING,
                RenderingHints.VALUE_TEXT_ANTIALIAS_ON);

        g2.setColor(getBackground());
        g2.fillRect(0, 0, getWidth(), getHeight());

        Disk disk = model.getDisk().get();
        DiskLayout layout = disk.diskLayout;

        if (layout == null)
        {
            String text = "No data yet!";
            FontMetrics metrics = g2.getFontMetrics();
            int x = (getWidth() - metrics.stringWidth(text)) / 2;
            int y = (getHeight() - metrics.getHeight()) / 2 + metrics.getAscent();
            g2.setColor(Color.RED);
            g2.drawString(text, x, y);
            return;
        }
    }
}
