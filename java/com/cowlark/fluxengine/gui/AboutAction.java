package com.cowlark.fluxengine.gui;

import com.cowlark.fluxengine.core.Version;
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
                "FluxEngine\nA disk-flux reader/writer\nVersion: " + Version.get(),
                "About FluxEngine",
                JOptionPane.INFORMATION_MESSAGE);
    }
}
