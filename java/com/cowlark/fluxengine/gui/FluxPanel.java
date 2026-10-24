package com.cowlark.fluxengine.gui;

import static swingtree.UI.of;

import javax.swing.JComponent;
import javax.swing.JInternalFrame;
import javax.swing.JPanel;
import javax.swing.event.InternalFrameAdapter;
import javax.swing.event.InternalFrameEvent;
import javax.swing.plaf.basic.BasicInternalFrameUI;
import java.awt.Dimension;
import java.awt.event.MouseMotionListener;

public class FluxPanel extends JPanel
{
    private final ImageViewModel model;
    private final FluxViewerPanel fluxViewerPanel;

    public FluxPanel(ImageViewModel model)
    {
        /* Create a flux viewer sharing the tab's model. */
        this.model = model;
        fluxViewerPanel = new FluxViewerPanel(model);

        /* Use the look and feel's title bar, but leave positioning to the row layout. */
        JInternalFrame frame = new JInternalFrame("Flux view", false, true, false, false)
        {
            @Override
            public void updateUI()
            {
                super.updateUI();

                /* Disable title-bar dragging while preserving its close button and decoration.
                 * Reapply this whenever the look and feel installs a new title bar. */
                if (getUI() instanceof BasicInternalFrameUI frameUI)
                {
                    JComponent titleBar = frameUI.getNorthPane();
                    if (titleBar != null)
                    {
                        for (MouseMotionListener listener : titleBar.getMouseMotionListeners())
                            titleBar.removeMouseMotionListener(listener);
                    }
                }
            }
        };
        frame.setFrameIcon(null);
        frame.setContentPane(fluxViewerPanel);
        frame.setDefaultCloseOperation(JInternalFrame.DISPOSE_ON_CLOSE);
        int height = frame.getPreferredSize().height;
        frame.setMinimumSize(new Dimension(0, height));
        frame.setPreferredSize(new Dimension(0, height));
        frame.setMaximumSize(new Dimension(Integer.MAX_VALUE, height));

        /* Remove closed rows so that the remaining viewers move up. */
        frame.addInternalFrameListener(new InternalFrameAdapter()
        {
            @Override
            public void internalFrameClosed(InternalFrameEvent event)
            {
                remove(frame);
                revalidate();
                repaint();
            }
        });

        /* Stack fixed-height viewers in rows, retaining the centre-window panels' padding. */
        of(this).withLayout("fillx, wrap 1, insets 5, aligny top").add("growx, pushx", of(frame));
        frame.setVisible(true);
    }
}
