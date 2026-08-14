package com.cowlark.fluxengine.gui;

import static swingtree.UIFactoryMethods.button;
import static swingtree.UIFactoryMethods.label;
import static swingtree.UIFactoryMethods.of;

import java.awt.Color;
import javax.swing.JPanel;

public class StatusbarPanel extends JPanel
{
    StatusbarPanel(ImagerViewModel model)
    {
        of(this).withLayout("fillx, insets 2").add(label(model.getStatusMessage())).add(
                "right",
                button("Stop").isEnabledIf(model.getBusy())
                        .withForeground(Color.RED)
                        .onClick(model::onEmergencyStop));
    }
}
