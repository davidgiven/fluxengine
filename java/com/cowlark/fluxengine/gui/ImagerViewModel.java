package com.cowlark.fluxengine.gui;

import static com.cowlark.fluxengine.gui.PreferencesReaderWriter.DEVICE;
import static com.cowlark.fluxengine.gui.PreferencesReaderWriter.DEVICE_FLUXFILE;
import static com.cowlark.fluxengine.gui.PreferencesReaderWriter.DEVICE_MANUAL;
import static com.cowlark.fluxengine.gui.PreferencesReaderWriter.FORMAT;
import static com.google.common.collect.ImmutableList.toImmutableList;

import com.cowlark.fluxengine.config.UsbFinder;
import com.cowlark.fluxengine.config.UsbFinder.CandidateDevice;
import com.cowlark.fluxengine.data.Image;
import com.google.common.collect.ImmutableMap;
import lombok.Getter;
import sprouts.Association;
import sprouts.From;
import sprouts.Var;
import sprouts.Viewable;
import swingtree.ComponentDelegate;
import javax.swing.JButton;
import java.awt.event.ActionEvent;
import java.util.Map;

public class ImagerViewModel
{
    private final PreferencesReaderWriter preferencesReaderWriter;

    @Getter private Var<String> statusMessage = Var.of("Ready");
    @Getter private Var<String> format;
    @Getter private Var<String> device;
    @Getter private Var<Association<String, String>> options =
            Var.of(Association.between(String.class, String.class));
    @Getter private Var<Image> diskImage = Var.of(new Image());
    @Getter private Var<Boolean> busy = Var.of(false);

    @Getter private ImmutableMap<String, CandidateDevice> devices = ImmutableMap.of();

    ImagerViewModel(PreferencesReaderWriter preferencesReaderWriter)
    {
        this.preferencesReaderWriter = preferencesReaderWriter;

        format = Var.of(preferencesReaderWriter.getPreference(FORMAT, "ibm"));
        device = Var.of(preferencesReaderWriter.getPreference(DEVICE, DEVICE_FLUXFILE));
        options.set(preferencesReaderWriter.getOptionsForFormat(format.get()));

        refreshUsbDevices();

        /* Viewable.cast reinterprets the property itself as a Viewable, so the
         * listener lives exactly as long as the property (unlike view(), which
         * returns a weakly-held view that must be kept in a field). */
        Viewable.cast(format).onChange(
                From.VIEW, it -> preferencesReaderWriter.setPreference(
                        FORMAT,
                        it.currentValue().orElseThrowUnchecked()));
        Viewable.cast(device).onChange(
                From.VIEW, it -> preferencesReaderWriter.setPreference(
                        DEVICE,
                        it.currentValue().orElseThrowUnchecked()));
    }

    Var<Association<String, String>> getOptionsForFormat(String format)
    {
        Var<Association<String, String>> value =
                Var.of(preferencesReaderWriter.getOptionsForFormat(format));
        Viewable.cast(value).onChange(
                From.VIEW, it -> preferencesReaderWriter.setOptionsForFormat(
                        format,
                        it.currentValue().orElseThrowUnchecked()));
        return value;
    }

    Var<Association<String, String>> getOptionsForDevice()
    {
        Var<Association<String, String>> value =
                Var.of(preferencesReaderWriter.getOptions(PreferencesReaderWriter.DEVICE_OPTIONS));
        Viewable.cast(value).onChange(
                From.VIEW,
                it -> preferencesReaderWriter.setOptions(
                        PreferencesReaderWriter.DEVICE_OPTIONS,
                        it.currentValue().orElseThrowUnchecked()));
        return value;
    }

    void refreshUsbDevices()
    {
        devices = new ImmutableMap.Builder<String, CandidateDevice>().put(
                        DEVICE_FLUXFILE,
                        new CandidateDevice())
                .put(DEVICE_MANUAL, new CandidateDevice())
                .putAll(UsbFinder.findUsbDevices()
                        .stream()
                        .map(candidate -> Map.entry(candidate.getSerial(), candidate))
                        .collect(toImmutableList()))
                .build();
    }

    void onReadDisk(ComponentDelegate<JButton, ActionEvent> delegate)
    {
    }

    void onRereadDisk(ComponentDelegate<JButton, ActionEvent> delegate)
    {
    }

    void onWriteDisk(ComponentDelegate<JButton, ActionEvent> delegate)
    {
    }

    void onEmergencyStop(ComponentDelegate<JButton, ActionEvent> delegate)
    {
    }
}
