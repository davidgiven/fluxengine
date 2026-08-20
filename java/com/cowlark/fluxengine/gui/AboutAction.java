package com.cowlark.fluxengine.gui;

import javax.swing.AbstractAction;
import javax.swing.JOptionPane;
import java.awt.event.ActionEvent;

class AboutAction extends AbstractAction
{
    @Override
    public void actionPerformed(ActionEvent e)
    {
        JOptionPane.showMessageDialog(
                null,
                "FluxEngine\nA disk-flux reader/writer",
                "About FluxEngine",
                JOptionPane.INFORMATION_MESSAGE);
    }
}
