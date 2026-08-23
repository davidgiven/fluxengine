package com.cowlark.fluxengine.gui;

import static com.cowlark.fluxengine.config.OptionApplicabilityHint.ANY_SOURCESINK;
import static com.cowlark.fluxengine.config.OptionApplicabilityHint.FLUXFILE_SOURCESINK;
import static com.cowlark.fluxengine.config.OptionApplicabilityHint.HARDWARE_SOURCESINK;
import static com.cowlark.fluxengine.gui.PreferencesReaderWriter.DEVICE_FLUXFILE;
import static com.cowlark.fluxengine.gui.PreferencesReaderWriter.DEVICE_SERIALPORT;
import static com.google.common.base.Strings.isNullOrEmpty;
import static com.google.common.collect.ImmutableMap.toImmutableMap;
import static javax.swing.JFileChooser.APPROVE_OPTION;
import static swingtree.UI.label;
import static swingtree.UI.of;
import static swingtree.UI.panel;
import static swingtree.UIFactoryMethods.button;
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
import swingtree.ComponentDelegate;
import swingtree.UI;
import swingtree.UIForPanel;
import javax.swing.JButton;
import javax.swing.JFileChooser;
import javax.swing.JPanel;
import javax.swing.SwingUtilities;
import java.awt.event.ActionEvent;
import java.io.File;
import java.util.Collection;
import java.util.HashSet;
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
    private static ImmutableMap<String, ConfigProto> formatData = Formats
            .all()
            .stream()
            .map(it -> Pair.of(it, Formats.get(it)))
            .filter(p -> !p.second().getIsExtension())
            .collect(toImmutableMap(Pair::first, Pair::second));
    private final ImagerViewModel model;

    public ConfigurationPanel(ImagerViewModel model)
    {
        this.model = model;

        Viewable.cast(model.getSelectedFormat()).onChange(From.ALL, it -> rebuildUi());
        Viewable.cast(model.getSelectedDevice()).onChange(From.ALL, it -> rebuildUi());
        Viewable.cast(model.getSelectedFluxFile()).onChange(From.ALL, it -> rebuildUi());
        Viewable.cast(model.getUsbDevices()).onChange(From.ALL, it -> rebuildUi());
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
            return assoc -> {
                Optional<String> value = assoc.get(optionGroupName);
                if (!value.isEmpty())
                    for (OptionProto option : optionGroup.getOptionList())
                        if (value.get().equals(option.getName()))
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
        return panel("fillx, insets 5 0")
                .add("w 10!", separator())
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
        return Optional
                .ofNullable(option)
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

            panel = panel.add(LABEL_FORMAT, label(commentRenderer(optionGroup.getComment()))).add(
                    SETTING_FORMAT, comboBox(
                            selected,
                            optionGroup.getOptionList(),
                            ConfigurationPanel::optionRenderer));
        }

        for (OptionProto option : config.getOptionList())
        {
            if (!testApplicability(applicableOptions, option.getApplicabilityList()))
                continue;

            String optionName = option.getName();
            Var<Boolean> selected = options.zoomTo(
                    it -> it.containsKey(optionName), (parent, newValue) -> {
                        if (newValue)
                            return parent.put(optionName, "");
                        else
                            return parent.remove(optionName);
                    });

            panel = panel
                    .add(LABEL_FORMAT, label(commentRenderer(option.getComment())))
                    .add(
                            SETTING_FORMAT,
                            comboBox(selected, YES_NO, ConfigurationPanel::yesNoRenderer));
        }

        return panel;
    }

    private UIForPanel<ConfigurationPanel> buildFormatPane(UIForPanel<ConfigurationPanel> panel)
    {
        panel = panel
                .add("span 2, growx, wrap", namedSeparator("Format properties"))
                .add(LABEL_FORMAT, label("Format:"))
                .add(
                        SETTING_FORMAT, comboBox(
                                model.getSelectedFormat(),
                                ImmutableList.copyOf(formatData.keySet()),
                                ConfigurationPanel::formatRenderer).onSelection(it -> model
                                .getSelectedFormat()
                                .set(From.VIEW, (String) it.get().getSelectedItem())));
        panel = emitOptions(
                panel,
                model.getOptionsForFormat(model.getSelectedFormat().get()),
                formatData.getOrDefault(
                        model.getSelectedFormat().get(),
                        ConfigProto.getDefaultInstance()),
                ImmutableSet.of());
        return panel;
    }

    private UIForPanel<ConfigurationPanel> buildDevicesPane(UIForPanel<ConfigurationPanel> panel)
    {
        panel = panel.add("span 2, growx, wrap", namedSeparator("Device properties"));

        Set<OptionApplicabilityHint> applicabilities = new HashSet<>();
        applicabilities.add(ANY_SOURCESINK);

        panel = panel
                .add(label("Device:"))
                .add(
                        SETTING_FORMAT, comboBox(
                                model.getSelectedDevice(),
                                model.getUsbDevices().get().keySet().stream().toList(),
                                this::deviceRenderer))
                .add(
                        "skip 1, split 2, align right",
                        button("Rescan USB").onClick(delegate -> model.refreshUsbDevices()))
                .add(button("Use flux file").onClick(this::onUseFluxFile));

        if (model.getSelectedDevice().get().equals(DEVICE_FLUXFILE))
            applicabilities.add(FLUXFILE_SOURCESINK);
        else
            applicabilities.add(HARDWARE_SOURCESINK);

        if (applicabilities.contains(FLUXFILE_SOURCESINK))
            panel = panel.add(LABEL_FORMAT, label("Flux file:")).add(
                    SETTING_FORMAT,
                    UI.textField(model.getSelectedFluxFile()).isEditableIf(false).peek(field -> {
                        /* Show the tail of an over-long path: keep
                         * the caret pinned to the end, since
                         * alignment has no effect once the content
                         * overflows. */
                        Runnable pin =
                                () -> SwingUtilities.invokeLater(() -> field.setCaretPosition(field
                                        .getDocument()
                                        .getLength()));
                        pin.run();
                        Viewable
                                .cast(model.getSelectedFluxFile())
                                .onChange(From.ALL, it -> pin.run());
                    }));

        if (model.getSelectedDevice().get().equals(DEVICE_SERIALPORT))
            panel = panel
                    .add(LABEL_FORMAT, label("Serial port:"))
                    .add(SETTING_FORMAT, UI.textField(model.getSelectedSerialPort()));

        if (applicabilities.contains(HARDWARE_SOURCESINK))
            panel = panel.add(LABEL_FORMAT, label("Drive:")).add(
                    SETTING_FORMAT,
                    comboBox(model.getSelectedDrive(), DRIVES, ConfigurationPanel::driveRenderer));

        panel = emitOptions(panel, model.getOptionsForDevice(), GLOBAL_CONFIG, applicabilities);
        return panel;
    }

    private String deviceRenderer(String device)
    {
        return switch (device)
        {
            case DEVICE_SERIALPORT -> "Greaseweazle: serial port";
            case DEVICE_FLUXFILE -> "Flux file";
            default ->
            {
                Association<String, UsbFinder.CandidateDevice> devices =
                        model.getUsbDevices().get();
                UsbFinder.CandidateDevice candidate = devices.get(device).orElse(null);
                if (candidate == null)
                    yield String.format("disconnected: %s", candidate.serial);
                else
                    yield String.format("%s: %s", candidate.type.getDeviceName(), candidate.serial);
            }
        };
    }

    private void onUseFluxFile(ComponentDelegate<JButton, ActionEvent> delegate)
    {
        JFileChooser fileChooser = new JFileChooser();
        String oldFile = model.getSelectedFluxFile().get();
        if (!isNullOrEmpty(oldFile))
            fileChooser.setCurrentDirectory(new File(oldFile).getParentFile());
        fileChooser.setDialogTitle("Open flux file");
        fileChooser.setApproveButtonText("Select");
        if (fileChooser.showOpenDialog(this) == APPROVE_OPTION)
        {
            model.getSelectedDevice().set(From.VIEW, DEVICE_FLUXFILE);
            model.getSelectedFluxFile().set(From.VIEW, fileChooser.getSelectedFile().getPath());
        }
    }
}
