package com.cowlark.fluxengine.gui;

import static swingtree.UI.of;

import javax.swing.JPanel;

public class FluxPanel extends JPanel
{
    private final ImageViewModel model;

    public FluxPanel(ImageViewModel model)
    {
        /* Retain the shared model for the flux viewer's future controls and display. */
        this.model = model;

        /* Match the layout and padding of the other centre-window panels. */
        of(this).withLayout("fill, insets 5");
    }
}
