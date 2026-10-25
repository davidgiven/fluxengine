package com.cowlark.fluxengine.gui;

import static swingtree.UI.button;
import static swingtree.UI.of;
import static swingtree.UI.scrollPane;

import com.formdev.flatlaf.util.UIScale;
import javax.swing.JDialog;
import javax.swing.JPanel;
import javax.swing.JTextArea;
import javax.swing.SwingUtilities;
import java.awt.Component;
import java.awt.Font;
import java.awt.Window;

/* A modeless, selectable text report for flux records and sectors. */
public class TextViewerDialogue extends JDialog
{
    private TextViewerDialogue(Window owner, String title, String text)
    {
        super(owner, title, ModalityType.MODELESS);

        /* Preserve report alignment and let long metadata and dumps scroll in either direction. */
        JTextArea area = new JTextArea(text);
        area.setEditable(false);
        area.setFont(new Font(Font.MONOSPACED, Font.PLAIN, UIScale.scale(UiUtils.pointsToPixels(10))));
        area.setCaretPosition(0);
        of((JPanel) getContentPane())
            .withLayout("fill, wrap 1, insets 12, gap 8")
            .add("grow, push", scrollPane().withPrefSize(600, 400).add(of(area)))
            .add("align right", button("Close").onClick(event -> dispose()));

        /* Keep reports independent of the originating viewer's lifetime. */
        setDefaultCloseOperation(DISPOSE_ON_CLOSE);
        pack();
        setLocationRelativeTo(owner);
    }

    public static void show(Component parent, String title, String text)
    {
        Window owner = SwingUtilities.getWindowAncestor(parent);
        new TextViewerDialogue(owner, title, text).setVisible(true);
    }
}
