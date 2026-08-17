package com.cowlark.fluxengine.gui;

import static com.cowlark.fluxengine.config.OptionApplicabilityHint.ANY_SOURCESINK;
import static com.cowlark.fluxengine.config.OptionApplicabilityHint.FLUXFILE_SOURCESINK;
import static com.cowlark.fluxengine.config.OptionApplicabilityHint.HARDWARE_SOURCESINK;
import static com.cowlark.fluxengine.gui.PreferencesReaderWriter.DEVICE_FLUXFILE;
import static com.cowlark.fluxengine.gui.PreferencesReaderWriter.DEVICE_MANUAL;
import static com.google.common.base.Strings.isNullOrEmpty;
import static com.google.common.collect.ImmutableMap.toImmutableMap;
import static swingtree.UI.label;
import static swingtree.UI.of;
import static swingtree.UI.panel;
import static swingtree.UIFactoryMethods.comboBox;
import static swingtree.UIFactoryMethods.separator;

import com.cowlark.fluxengine.config.ConfigProto;
import com.cowlark.fluxengine.config.OptionApplicabilityHint;
import com.cowlark.fluxengine.config.OptionGroupProto;
import com.cowlark.fluxengine.config.OptionProto;
import com.cowlark.fluxengine.config.UsbFinder;
import com.cowlark.fluxengine.data.Formats;
import com.google.common.collect.ImmutableList;
import com.google.common.collect.ImmutableMap;
import com.google.common.collect.ImmutableSet;
import org.apache.commons.lang3.StringUtils;
import org.jspecify.annotations.Nullable;
import sprouts.Association;
import sprouts.From;
import sprouts.Pair;
import sprouts.Var;
import sprouts.Viewable;
import swingtree.UIForPanel;
import javax.swing.JPanel;
import java.util.Collection;
import java.util.HashSet;
import java.util.List;
import java.util.Optional;
import java.util.Set;
import java.util.function.BiFunction;
import java.util.function.Function;

public class ConfigurationPanel extends JPanel
{
    private static final ImmutableList<Boolean> YES_NO = ImmutableList.of(false, true);
    private static final ImmutableList<Integer> DRIVES = ImmutableList.of(0, 1);
    private static final ConfigProto GLOBAL_CONFIG = Formats.get("_global_options");
    private static final String LABEL_FORMAT = "wmax 100lp";
    private static final String SETTING_FORMAT = "wmax 200lp, growx, pushx";
    private static ImmutableMap<String, ConfigProto> formatData = Formats.all()
            .stream()
            .map(it -> Pair.of(it, Formats.get(it)))
            .filter(p -> !p.second().getIsExtension())
            .collect(toImmutableMap(Pair::first, Pair::second));
    private final ImagerViewModel model;

    public ConfigurationPanel(ImagerViewModel model)
    {
        this.model = model;

        Viewable.cast(model.getFormat()).onChange(From.ALL, it -> rebuildUi());
        Viewable.cast(model.getDevice()).onChange(From.ALL, it -> rebuildUi());
        rebuildUi();
    }

    private static Function<Association<String, String>, OptionProto> getOption(OptionGroupProto optionGroup)
    {
        String optionGroupName = optionGroup.getName();
        OptionProto defaultOption = findDefaultOption(optionGroup);
        if (isNullOrEmpty(optionGroupName))
            return assoc -> {
                for (OptionProto option : optionGroup.getOptionList())
                    if (assoc.containsKey(option.getName()))
                        return option;
                return defaultOption;
            };
        else
            return it -> {
                for (OptionProto option : optionGroup.getOptionList())
                    if (option.getName().equals(optionGroupName))
                        return option;
                return defaultOption;
            };
    }

    private static BiFunction<Association<String, String>, OptionProto, Association<String,
            String>> withOption(
            OptionGroupProto optionGroup)
    {
        String optionGroupName = optionGroup.getName();
        if (isNullOrEmpty(optionGroupName))
            return (parent, newOption) -> {
                for (OptionProto option : optionGroup.getOptionList())
                    parent = parent.remove(option.getName());
                return parent.put(newOption.getName(), "");
            };
        else
            return (parent, newOption) -> parent.put(optionGroupName, newOption.getName());
    }

    private static UIForPanel<JPanel> namedSeparator(String label)
    {
        return panel("fillx, insets 5 0").add("w 10!", separator())
                .add(label(label))
                .add("growx, pushx", separator());
    }

    private static OptionProto findDefaultOption(OptionGroupProto optionGroup)
    {
        for (OptionProto option : optionGroup.getOptionList())
            if (option.getSetByDefault())
                return option;
        return null;
    }

    /* Returns true if the given applicability hints are all applicable for
     * the current format. */
    static <T> boolean testApplicability(
            Set<T> enabledApplicabilities,
            Collection<T> optionApplicabilities)
    {
        if (enabledApplicabilities.isEmpty())
            return true;
        if (optionApplicabilities.stream().noneMatch(enabledApplicabilities::contains))
            return false;
        return true;
    }

    private static String yesNoRenderer(boolean yesno)
    {
        return yesno ? "Yes" : "No";
    }

    private static String formatRenderer(String format)
    {
        return formatData.get(format).getShortname();
    }

    private static String optionRenderer(@Nullable OptionProto option)
    {
        return Optional.ofNullable(option)
                .map(OptionProto::getComment)
                .orElse("*** missing default ***");
    }

    private static String driveRenderer(int drive)
    {
        return switch (drive)
        {
            case 0 -> "Drive 0";
            case 1 -> "Drive 1";
            default -> throw new RuntimeException("bad state");
        };
    }

    private static String commentRenderer(String comment)
    {
        if (comment.equals("$formats"))
            return "Variant:";
        return String.format("<html>%s:</html>", StringUtils.capitalize(comment));
    }

    /* Removes the existing UI and recreates it. */
    private void rebuildUi()
    {
        removeAll();

        UIForPanel<ConfigurationPanel> panel = of(this).withLayout("wrap 2, insets 5");

        panel = buildFormatPane(panel);
        panel = buildDevicesPane(panel);

        revalidate();
        repaint();
    }

    private UIForPanel<ConfigurationPanel> emitOptions(
            UIForPanel<ConfigurationPanel> panel,
            Var<Association<String, String>> options,
            ConfigProto config,
            Set<OptionApplicabilityHint> applicableOptions)
    {
        for (OptionGroupProto optionGroup : config.getOptionGroupList())
        {
            if (!testApplicability(applicableOptions, optionGroup.getApplicabilityList()))
                continue;

            Var<OptionProto> selected =
                    options.zoomTo(getOption(optionGroup), withOption(optionGroup));

            panel = panel.add(LABEL_FORMAT, label(commentRenderer(optionGroup.getComment())))
                    .add(SETTING_FORMAT,
                            comboBox(selected,
                                    optionGroup.getOptionList(),
                                    ConfigurationPanel::optionRenderer));
        }

        for (OptionProto option : config.getOptionList())
        {
            if (!testApplicability(applicableOptions, option.getApplicabilityList()))
                continue;

            String optionName = option.getName();
            Var<Boolean> selected =
                    options.zoomTo(it -> it.containsKey(optionName), (parent, newValue) -> {
                        if (newValue)
                            return parent.put(optionName, "");
                        else
                            return parent.remove(optionName);
                    });

            panel = panel.add(LABEL_FORMAT, label(commentRenderer(option.getComment())))
                    .add(SETTING_FORMAT,
                            comboBox(selected, YES_NO, ConfigurationPanel::yesNoRenderer));
        }

        return panel;
    }

    private UIForPanel<ConfigurationPanel> buildFormatPane(UIForPanel<ConfigurationPanel> panel)
    {
        panel = panel.add("span 2, growx, wrap", namedSeparator("Format properties"))
                .add(LABEL_FORMAT, label("Format:"))
                .add(SETTING_FORMAT,
                        comboBox(model.getFormat(),
                                ImmutableList.copyOf(formatData.keySet()),
                                ConfigurationPanel::formatRenderer).onSelection(it -> model.getFormat()
                                .set(From.VIEW, (String) it.get().getSelectedItem())));
        panel = emitOptions(panel,
                model.getOptionsForFormat(model.getFormat().get()),
                formatData.getOrDefault(model.getFormat().get(), ConfigProto.getDefaultInstance()),
                ImmutableSet.of());
        return panel;
    }

    private UIForPanel<ConfigurationPanel> buildDevicesPane(UIForPanel<ConfigurationPanel> panel)
    {
        panel = panel.add("span 2, growx, wrap", namedSeparator("Device properties"));

        Set<OptionApplicabilityHint> applicabilities = new HashSet<>();
        applicabilities.add(ANY_SOURCESINK);

        panel = panel.add(label("Device:"))
                .add(SETTING_FORMAT,
                        comboBox(model.getDevice(),
                                List.copyOf(model.getDevices().keySet()),
                                this::deviceRenderer));

        switch (model.getDevice().get())
        {
            case DEVICE_FLUXFILE -> applicabilities.add(FLUXFILE_SOURCESINK);
            case DEVICE_MANUAL ->
            {
                applicabilities.add(FLUXFILE_SOURCESINK);
                applicabilities.add(HARDWARE_SOURCESINK);
            }
            default -> applicabilities.add(HARDWARE_SOURCESINK);
        }

        if (applicabilities.contains(HARDWARE_SOURCESINK))
            panel = panel.add(label("Drive:"))
                    .add(SETTING_FORMAT,
                            comboBox(model.getDrive(), DRIVES, ConfigurationPanel::driveRenderer));

        panel = emitOptions(panel, model.getOptionsForDevice(), GLOBAL_CONFIG, applicabilities);
        return panel;
    }

    private String deviceRenderer(String device)
    {
        return switch (device)
        {
            case DEVICE_MANUAL -> "Greaseweazle: serial port";
            case DEVICE_FLUXFILE -> "Flux file";
            default ->
            {
                ImmutableMap<String, UsbFinder.CandidateDevice> devices = model.getDevices();
                UsbFinder.CandidateDevice candidate = devices.get(device);
                if (candidate == null)
                    yield String.format("disconnected: %s", candidate.serial);
                else
                    yield String.format("%s: %s", candidate.type.getDeviceName(), candidate.serial);
            }
        };
    }
}
