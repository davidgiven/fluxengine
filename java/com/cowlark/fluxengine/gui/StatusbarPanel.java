package com.cowlark.fluxengine.gui;

import static swingtree.UIFactoryMethods.button;
import static swingtree.UIFactoryMethods.label;
import static swingtree.UIFactoryMethods.of;

import javax.swing.JPanel;

public class StatusbarPanel extends JPanel
{
    StatusbarPanel()
    {
        of(this).withLayout("fillx, insets 2")
                .add(label("Hello, world!"))
                .add("right", button("Button"));
    }
}
