package com.cowlark.fluxengine.gui;

import com.formdev.flatlaf.FlatDarkLaf;
import com.google.common.collect.ImmutableList;
import swingtree.UI;
import swingtree.threading.EventProcessor;
import javax.swing.UIManager;

/**
 * The FluxEngine GUI, ported from src/gui/main.cc.
 */
public class Gui
{

    public void run(ImmutableList<String> args) throws Exception
    {
        UIManager.setLookAndFeel(new FlatDarkLaf());
        System.setProperty("apple.laf.useScreenMenuBar", "true");

        UI.of(new ApplicationFrame())
                .withOnCloseOperation(UI.OnWindowClose.DISPOSE)
                .onClose(it -> System.exit(0))
                .peek(frame -> frame.setLocationRelativeTo(null))
                .show();

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
