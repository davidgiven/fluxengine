package com.cowlark.fluxengine.gui;

import static com.cowlark.fluxengine.gui.PreferencesReaderWriter.FORMAT;

import com.cowlark.fluxengine.data.Image;
import lombok.Getter;
import sprouts.Association;
import sprouts.From;
import sprouts.Var;
import sprouts.Viewable;
import swingtree.ComponentDelegate;
import javax.swing.JButton;
import java.awt.event.ActionEvent;

public class ImagerViewModel
{
    private final PreferencesReaderWriter preferencesReaderWriter;

    @Getter private Var<String> statusMessage = Var.of("Ready");
    @Getter private Var<String> format;
    @Getter private Var<Association<String, String>> options =
            Var.of(Association.between(String.class, String.class));
    @Getter private Var<Image> diskImage = Var.of(new Image());
    @Getter private Var<Boolean> busy = Var.of(false);

    ImagerViewModel(PreferencesReaderWriter preferencesReaderWriter)
    {
        this.preferencesReaderWriter = preferencesReaderWriter;

        format = Var.of(preferencesReaderWriter.getPreference(FORMAT, "ibm"));
        options.set(preferencesReaderWriter.getOptionsForFormat(format.get()));

        /* Viewable.cast reinterprets the property itself as a Viewable, so the
         * listener lives exactly as long as the property (unlike view(), which
         * returns a weakly-held view that must be kept in a field). */
        Viewable.cast(format).onChange(
                From.VIEW,
                it -> preferencesReaderWriter.setPreference(
                        FORMAT,
                        it.currentValue().orElseThrowUnchecked()));
    }

    Var<Association<String, String>> getOptionsForFormat(String format)
    {
        Var<Association<String, String>> value =
                Var.of(preferencesReaderWriter.getOptionsForFormat(format));
        Viewable.cast(value).onChange(
                From.VIEW, it -> {
                    System.out.println(it.currentValue().orElseThrowUnchecked());
                    preferencesReaderWriter.setOptionsForFormat(
                            format,
                            it.currentValue().orElseThrowUnchecked());
                });
        return value;
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
