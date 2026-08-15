package com.cowlark.fluxengine.gui;

import static com.google.common.collect.ImmutableMap.toImmutableMap;
import static swingtree.UI.label;
import static swingtree.UI.of;
import static swingtree.UI.panel;
import static swingtree.UIFactoryMethods.comboBox;
import static swingtree.UIFactoryMethods.separator;

import com.cowlark.fluxengine.config.ConfigProto;
import com.cowlark.fluxengine.config.OptionProto;
import com.cowlark.fluxengine.data.Formats;
import com.google.common.collect.ImmutableList;
import com.google.common.collect.ImmutableMap;
import sprouts.Association;
import sprouts.From;
import sprouts.Pair;
import sprouts.Var;
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

    private static final ImmutableList<Boolean> YES_NO = ImmutableList.of(false, true);

    private static final String LABEL_FORMAT = "wmax 150lp";
    private static final String SETTING_FORMAT = "growx, pushx";

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

        panel = buildFormatPane(panel).add(
                        "span 2, growx, wrap",
                        namedSeparator("Device properties"))
                .add(label("Device:"))
                .add(SETTING_FORMAT, comboBox("1", "2"));
    }

    private UIForPanel<ConfigurationPanel> emitOptions(UIForPanel<ConfigurationPanel> panel,
                                                       Var<Association<String, String>> options,
                                                       ConfigProto config)
    {
        for (OptionProto option : config.getOptionList())
        {
            String optionName = option.getName();
            Var<Boolean> selected = options.zoomTo(
                    it -> it.containsKey(optionName), (oldValue, newSetting) -> {
                        if (newSetting)
                            return oldValue.put(optionName, "");
                        else
                            return oldValue.remove(optionName);
                    });

            panel = panel.add(
                            LABEL_FORMAT,
                            label(String.format("<html>%s:</html>", option.getComment())))
                    .add(
                            SETTING_FORMAT,
                            comboBox(selected, YES_NO, ConfigurationPanel::yesNoRenderer));
        }

        return panel;
    }

    private UIForPanel<ConfigurationPanel> buildFormatPane(UIForPanel<ConfigurationPanel> panel)
    {
        panel = panel.add("span 2, growx, wrap", namedSeparator("Format properties"))
                .add(LABEL_FORMAT, label("Format:"))
                .add(
                        SETTING_FORMAT, comboBox(
                                model.getFormat(),
                                ImmutableList.copyOf(formatData.keySet()),
                                ConfigurationPanel::formatRenderer).onSelection(it -> model.getFormat()
                                .set(From.VIEW, (String) it.get().getSelectedItem())));
        panel = emitOptions(
                panel,
                model.getOptionsForFormat(model.getFormat().get()),
                formatData.getOrDefault(model.getFormat().get(), ConfigProto.getDefaultInstance()));
        return panel;
    }

    private static UIForPanel<JPanel> namedSeparator(String label)
    {
        return panel("fillx, insets 5 0").add("w 10!", separator())
                .add(label(label))
                .add("growx, pushx", separator());
    }

    private static String yesNoRenderer(boolean yesno)
    {
        return yesno ? "Yes" : "No";
    }

    private static String formatRenderer(String format)
    {
        return formatData.get(format).getShortname();
    }
}
