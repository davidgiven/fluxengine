package com.cowlark.fluxengine.gui;

import static swingtree.UI.of;
import static swingtree.UIFactoryMethods.label;

import swingtree.UI.HorizontalAlignment;
import javax.swing.JPanel;
import javax.swing.JScrollPane;
import javax.swing.Scrollable;
import javax.swing.event.InternalFrameAdapter;
import javax.swing.event.InternalFrameEvent;
import java.awt.Dimension;
import java.awt.Rectangle;

public class FluxPanel extends JPanel
{
    private final ImageViewModel model;
    private final FluxViewerPanel fluxViewerPanel;
    private final ViewerRows viewerRows;

    public FluxPanel(ImageViewModel model)
    {
        /* Create a flux viewer sharing the tab's model. */
        this.model = model;
        fluxViewerPanel = new FluxViewerPanel(model);
        viewerRows = new ViewerRows();

        /* Remove closed rows so that the remaining viewers move up. */
        fluxViewerPanel.addInternalFrameListener(new InternalFrameAdapter()
        {
            @Override
            public void internalFrameClosed(InternalFrameEvent event)
            {
                viewerRows.remove(fluxViewerPanel);
                viewerRows.revalidate();
                viewerRows.repaint();
            }
        });

        /* Stack fixed-height viewers in rows, retaining the centre-window panels' padding. */
        of(viewerRows).withLayout("fillx, wrap 1, insets 5, aligny top")
                .add("growx, pushx", of(fluxViewerPanel))
                .add("growx, pushx", label("To add more flux views, select a track below")
                        .withHorizontalAlignment(HorizontalAlignment.CENTER));

        /* Scroll overflowing rows vertically while fitting them to the viewport width. */
        JScrollPane scrollPane = new JScrollPane(
                viewerRows,
                JScrollPane.VERTICAL_SCROLLBAR_AS_NEEDED,
                JScrollPane.HORIZONTAL_SCROLLBAR_NEVER);
        of(this).withLayout("fill, insets 0").add("grow, push", of(scrollPane));
    }

    /* Track the available width while preserving the full combined height of the rows. */
    private static class ViewerRows extends JPanel implements Scrollable
    {
        @Override
        public Dimension getPreferredScrollableViewportSize()
        {
            return getPreferredSize();
        }

        @Override
        public int getScrollableUnitIncrement(Rectangle visibleRect, int orientation, int direction)
        {
            return UiUtils.pointsToPixels(16);
        }

        @Override
        public int getScrollableBlockIncrement(Rectangle visibleRect, int orientation, int direction)
        {
            return Math.max(1, visibleRect.height - UiUtils.pointsToPixels(16));
        }

        @Override
        public boolean getScrollableTracksViewportWidth()
        {
            return true;
        }

        @Override
        public boolean getScrollableTracksViewportHeight()
        {
            return false;
        }
    }
}
