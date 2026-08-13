package com.cowlark.fluxengine.gui;

import static swingtree.UI.comboBox;
import static swingtree.UI.label;
import static swingtree.UI.of;

import swingtree.UI;
import javax.swing.JPanel;

public class ConfigurationPanel extends JPanel
{
    public ConfigurationPanel()
    {
        swingtree.UIForPanel<ConfigurationPanel> panel =
                of(this).withLayout("wrap 2, insets 5");
        for (int i = 0; i < 10; i++)
            panel = panel.add(label(String.format("label %d", i)))
                    .add("growx, pushx", comboBox(1, 2, 3, 4, 5));
    }
}
