package com.cowlark.fluxengine.gui;

import static swingtree.UI.label;
import static swingtree.UI.menu;
import static swingtree.UI.menuItem;
import static swingtree.UI.of;
import static swingtree.UI.panel;

import com.formdev.flatlaf.FlatDarkLaf;
import com.google.common.collect.ImmutableList;
import javax.swing.JOptionPane;
import javax.swing.JPanel;
import javax.swing.UIManager;
import swingtree.UI;
import swingtree.threading.EventProcessor;

/**
 * The FluxEngine GUI, ported from src/gui/main.cc.
 */
public class Gui
{
    public void run(ImmutableList<String> args) throws Exception
    {
        UIManager.setLookAndFeel(new FlatDarkLaf());
        System.setProperty("apple.laf.useScreenMenuBar", "true");

        UI.MenuBar menuBar = of(new UI.MenuBar())
                .add(menu("File")
                        .add(menuItem("About FluxEngine...").onClick(it ->
                                JOptionPane.showMessageDialog(
                                        null,
                                        "FluxEngine\nA disk-flux reader/writer",
                                        "About FluxEngine",
                                        JOptionPane.INFORMATION_MESSAGE)))
                        .add(menuItem("Exit").onClick(it -> System.exit(0))))
                .get(UI.MenuBar.class);

        UI.show("FluxEngine", frame -> {
            frame.setJMenuBar(menuBar);
            frame.setSize(800, 600);
            frame.setLocationRelativeTo(null);

            return panel("fill")
                    .add(label("FluxEngine"))
                    .get(JPanel.class);
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
