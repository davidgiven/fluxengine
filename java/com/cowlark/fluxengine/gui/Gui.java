package com.cowlark.fluxengine.gui;

import com.cowlark.fluxengine.core.FluxEngineException;
import com.formdev.flatlaf.FlatDarkLaf;
import com.google.common.collect.ImmutableList;
import swingtree.UI;
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
    private ImageViewModel model = new ImageViewModel(preferencesReaderWriter);

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

    public void run(ImmutableList<String> args) throws Exception
    {
        UIManager.setLookAndFeel(new FlatDarkLaf());
        System.setProperty("apple.laf.useScreenMenuBar", "true");

        UiUtils.EDT_EXECUTOR.execute(() -> {
            ApplicationFrame frame = new ApplicationFrame(model, preferencesReaderWriter);
            frame.pack();
            frame.setVisible(true);

            /* Restore after the window is visible so load errors have a usable GUI. */
            String filename = model.getCustomFormatFilename().get();
            if (!filename.isEmpty())
            {
                try
                {
                    model.onCustomFormatChanged(filename);
                } catch (FluxEngineException e)
                {
                    UI.message(e.getMessage()).showAsError();
                }
            }
        });

        EventProcessor.DECOUPLED.join();
    }
}
