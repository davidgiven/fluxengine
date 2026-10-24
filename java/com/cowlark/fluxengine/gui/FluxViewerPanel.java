package com.cowlark.fluxengine.gui;

import static swingtree.UI.of;

import javax.swing.JPanel;
import javax.swing.JScrollPane;
import javax.swing.JViewport;
import javax.swing.SwingUtilities;
import java.awt.Cursor;
import java.awt.Dimension;
import java.awt.Graphics;
import java.awt.Graphics2D;
import java.awt.Point;
import java.awt.event.MouseAdapter;
import java.awt.event.MouseEvent;

public class FluxViewerPanel extends JPanel
{
    /* Fixed viewer height in typographic points. */
    private static final int HEIGHT_PTS = 150;

    private final ImageViewModel model;
    private final JPanel canvas;
    private Point dragPosition;

    public FluxViewerPanel(ImageViewModel model)
    {
        /* Retain the shared model for the flux viewer's future controls and display. */
        this.model = model;

        /* Draw in content coordinates; the viewport handles translation and clipping. */
        canvas = new JPanel()
        {
            @Override
            protected void paintComponent(Graphics graphics)
            {
                super.paintComponent(graphics);
                Graphics2D contentGraphics = (Graphics2D) graphics.create();
                try
                {
                    redraw(contentGraphics);
                }
                finally
                {
                    contentGraphics.dispose();
                }
            }
        };

        /* Keep the viewer a fixed height and allow scrolling only along the flux timeline. */
        int height = UiUtils.pointsToPixels(HEIGHT_PTS);
        setMinimumSize(new Dimension(0, height));
        setPreferredSize(new Dimension(0, height));
        setMaximumSize(new Dimension(Integer.MAX_VALUE, height));
        canvas.setPreferredSize(new Dimension(2000, height));
        JScrollPane scrollPane = new JScrollPane(
                canvas,
                JScrollPane.VERTICAL_SCROLLBAR_NEVER,
                JScrollPane.HORIZONTAL_SCROLLBAR_AS_NEEDED);
        JViewport viewport = scrollPane.getViewport();

        /* Drag the content with the primary mouse button, ending the gesture on release. */
        MouseAdapter mouseHandler = new MouseAdapter()
        {
            @Override
            public void mousePressed(MouseEvent event)
            {
                if (!SwingUtilities.isLeftMouseButton(event))
                    return;
                dragPosition = SwingUtilities.convertPoint(canvas, event.getPoint(), viewport);
                canvas.setCursor(Cursor.getPredefinedCursor(Cursor.MOVE_CURSOR));
            }

            @Override
            public void mouseDragged(MouseEvent event)
            {
                if (dragPosition == null)
                    return;
                /* Measure in viewport coordinates because the canvas moves during the drag. */
                Point position = SwingUtilities.convertPoint(canvas, event.getPoint(), viewport);
                Point viewPosition = viewport.getViewPosition();
                Dimension extent = viewport.getExtentSize();
                Dimension viewSize = viewport.getViewSize();
                int x = viewPosition.x + dragPosition.x - position.x;
                x = Math.max(0, Math.min(x, Math.max(0, viewSize.width - extent.width)));
                viewport.setViewPosition(new Point(x, 0));
                dragPosition = position;
            }

            @Override
            public void mouseReleased(MouseEvent event)
            {
                if (!SwingUtilities.isLeftMouseButton(event))
                    return;
                dragPosition = null;
                canvas.setCursor(Cursor.getDefaultCursor());
            }
        };
        canvas.addMouseListener(mouseHandler);
        canvas.addMouseMotionListener(mouseHandler);

        /* Fill this viewer with the scrollable canvas. */
        of(this).withLayout("fill, insets 0").add("grow, push", of(scrollPane));
    }

    /* Draw the flux content in canvas coordinates. */
    protected void redraw(Graphics2D graphics)
    {
    }
}
