package com.cowlark.fluxengine.gui;

import static com.google.common.collect.ImmutableMap.toImmutableMap;
import static swingtree.UI.label;
import static swingtree.UI.of;
import static swingtree.UI.panel;
import static swingtree.UIFactoryMethods.comboBox;
import static swingtree.UIFactoryMethods.separator;

import com.cowlark.fluxengine.config.ConfigProto;
import com.cowlark.fluxengine.data.Formats;
import com.google.common.collect.ImmutableList;
import com.google.common.collect.ImmutableMap;
import sprouts.From;
import sprouts.Pair;
import sprouts.Viewable;
import swingtree.UIForPanel;
import javax.swing.JPanel;

public class ConfigurationPanel extends JPanel
{
    private static ImmutableMap<String, ConfigProto> formatData = Formats.all()
            .stream()
            .map(it -> Pair.of(it, Formats.get(it)))
            .filter(p -> !p.second().getIsExtension())
            .collect(toImmutableMap(Pair::first, Pair::second));

    private final ImagerViewModel model;

    public ConfigurationPanel(ImagerViewModel model)
    {
        this.model = model;

        /* Rebuild whenever the format changes (and once at startup). */
        Viewable.cast(model.getFormat()).onChange(From.ALL, it -> rebuildUi());
        rebuildUi();
    }

    /* Removes the existing UI and recreates it. */
    private void rebuildUi()
    {
        removeAll();

        UIForPanel<ConfigurationPanel> panel = of(this).withLayout("wrap 2, insets 5");

        panel = panel.add("span 2, growx, wrap", namedSeparator("Format properties"))
                .add(label("Format:"))
                .add(
                        "growx, pushx", comboBox(
                                model.getFormat(),
                                ImmutableList.copyOf(formatData.keySet()),
                                ConfigurationPanel::formatRenderer).onSelection(it -> model.getFormat()
                                .set(From.VIEW, (String) it.get().getSelectedItem())))
                .add("span 2, growx, wrap", namedSeparator("Device properties"))
                .add(label("Device:"))
                .add("growx, pushx", comboBox(new String[]{"1", "2"}));
    }

    private static UIForPanel<JPanel> namedSeparator(String label)
    {
        return panel("fillx, insets 5 0").add("w 10!", separator())
                .add(label(label))
                .add("growx, pushx", separator());
    }

    private static String formatRenderer(String format)
    {
        return formatData.get(format).getShortname();
    }
}
