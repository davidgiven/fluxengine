package com.cowlark.fluxengine.gui;

import static swingtree.UI.label;
import static swingtree.UI.panel;

import com.formdev.flatlaf.FlatDarkLaf;
import com.google.common.collect.ImmutableList;
import swingtree.UI;
import swingtree.threading.EventProcessor;
import javax.swing.JPanel;
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

        UI.show(
                "FluxEngine", frame -> {
                    frame.setJMenuBar(AppMenu.createMenu());
                    frame.setSize(800, 600);
                    frame.setLocationRelativeTo(null);

                    return panel("fill").add(label("FluxEngine")).get(JPanel.class);
                });

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
