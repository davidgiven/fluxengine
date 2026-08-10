package com.cowlark.fluxengine.gui;

import com.formdev.flatlaf.FlatLightLaf;
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
    public static void main(String[] args)
    {
        SwingUtilities.invokeLater(() -> {
            setupLookAndFeel();
            createAndShowGui();
        });
    }

    private static void setupLookAndFeel()
    {
        try {
            UIManager.setLookAndFeel(new FlatLightLaf());
        } catch (Exception e) {
            e.printStackTrace();
        }
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
