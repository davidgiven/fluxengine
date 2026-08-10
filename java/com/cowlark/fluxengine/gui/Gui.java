package com.cowlark.fluxengine.gui;

import com.formdev.flatlaf.FlatDarkLaf;
import com.google.common.collect.ImmutableList;
import javax.swing.JFrame;
import javax.swing.JLabel;
import javax.swing.SwingUtilities;
import javax.swing.UIManager;
import javax.swing.WindowConstants;

/**
 * The FluxEngine GUI, ported from src/gui/main.cc.
 */
public class Gui
{
    public void run(ImmutableList<String> args) throws Exception
    {
        UIManager.setLookAndFeel(new FlatDarkLaf());
        SwingUtilities.invokeLater(() -> {
            createAndShowGui();
        });
    }

    private static void createAndShowGui()
    {
        JFrame frame = new NewJFrame();
        frame.setDefaultCloseOperation(WindowConstants.EXIT_ON_CLOSE);
        frame.setLocationRelativeTo(null);
        frame.setVisible(true);
    }
}
