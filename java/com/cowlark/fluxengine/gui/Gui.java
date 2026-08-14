package com.cowlark.fluxengine.gui;

import com.formdev.flatlaf.FlatDarkLaf;
import com.google.common.collect.ImmutableList;
import swingtree.threading.EventProcessor;
import javax.swing.UIManager;
import java.util.prefs.Preferences;

/**
 * The FluxEngine GUI, ported from src/gui/main.cc.
 */
public class Gui
{
    private final Preferences preferences = Preferences.userNodeForPackage(Gui.class);
    private PreferencesReaderWriter preferencesReaderWriter =
            new PreferencesReaderWriter(preferences);
    private ImagerViewModel model = new ImagerViewModel(preferencesReaderWriter);

    public void run(ImmutableList<String> args) throws Exception
    {
        UIManager.setLookAndFeel(new FlatDarkLaf());
        System.setProperty("apple.laf.useScreenMenuBar", "true");

        ApplicationFrame frame = new ApplicationFrame(model);
        frame.show();

        EventProcessor.DECOUPLED.join();
    }

    public static void main(String[] args)
    {
        try
        {
            new Gui().run(ImmutableList.copyOf(args));
        } catch (Exception e)
        {
            throw new RuntimeException(e);
        }
    }
}
