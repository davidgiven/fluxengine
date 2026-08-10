package com.cowlark.fluxengine.gui;

import javax.swing.JFrame;
import javax.swing.JLabel;
import javax.swing.SwingUtilities;
import javax.swing.WindowConstants;

/**
 * The FluxEngine GUI, ported from src/gui/main.cc.
 */
public class Gui
{
    public static void main(String[] args)
    {
        SwingUtilities.invokeLater(Gui::createAndShowGui);
    }

    private static void createAndShowGui()
    {
        JFrame frame = new JFrame("FluxEngine");
        frame.setDefaultCloseOperation(WindowConstants.EXIT_ON_CLOSE);

        JLabel label = new JLabel("FluxEngine", JLabel.CENTER);
        frame.getContentPane().add(label);
        frame.setSize(800, 600);
        frame.setLocationRelativeTo(null);
        frame.setVisible(true);
    }
}
