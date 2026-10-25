package com.cowlark.fluxengine.gui;

import static swingtree.UI.of;
import static swingtree.UIFactoryMethods.label;

import com.cowlark.fluxengine.data.Track;
import com.google.common.collect.ImmutableList;
import io.reactivex.rxjava3.disposables.Disposable;
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
    private final ViewerRows viewerRows;
    private Disposable fluxViewSubscription;

    public FluxPanel(ImageViewModel model)
    {
        /* Create a flux viewer sharing the tab's model. */
        this.model = model;
        viewerRows = new ViewerRows();

        /* Stack fixed-height viewers in rows, retaining the centre-window panels' padding. */
        of(viewerRows).withLayout("fillx, wrap 1, insets 3, gapy 2pt, aligny top")
                .add("growx, pushx", label("To add flux views, select a track below")
                        .withHorizontalAlignment(HorizontalAlignment.CENTER));

        /* Scroll overflowing rows vertically while fitting them to the viewport width. */
        JScrollPane scrollPane = new JScrollPane(
                viewerRows,
                JScrollPane.VERTICAL_SCROLLBAR_AS_NEEDED,
                JScrollPane.HORIZONTAL_SCROLLBAR_NEVER);
        of(this).withLayout("fill, insets 0").add("grow, push", of(scrollPane));
    }

    /* Listen while attached, delivering viewer requests on the Swing event thread. */
    @Override
    public void addNotify()
    {
        super.addNotify();
        fluxViewSubscription = model.getFluxViewRequests()
                .observeOn(UiUtils.EDT)
                .subscribe(this::openFluxView);
    }

    /* Release the model's reference to this panel when it leaves the component hierarchy. */
    @Override
    public void removeNotify()
    {
        if (fluxViewSubscription != null)
        {
            fluxViewSubscription.dispose();
            fluxViewSubscription = null;
        }
        super.removeNotify();
    }

    /* Handle a request to display captured tracks. */
    private void openFluxView(ImmutableList<Track> tracks)
    {
        /* Insert new rows immediately before the instruction label. */
        FluxViewerPanel viewer = new FluxViewerPanel(model, tracks);
        viewer.addInternalFrameListener(new InternalFrameAdapter()
        {
            @Override
            public void internalFrameClosed(InternalFrameEvent event)
            {
                viewerRows.remove(viewer);
                viewerRows.revalidate();
                viewerRows.repaint();
            }
        });
        viewerRows.add(viewer, "growx, pushx", viewerRows.getComponentCount() - 1);
        viewerRows.revalidate();
        viewerRows.repaint();
    }

    /* Give every viewer the same close behavior, including the initial row. */

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
