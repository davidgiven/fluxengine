package com.cowlark.fluxengine.gui;

import io.reactivex.rxjava3.core.Scheduler;
import io.reactivex.rxjava3.schedulers.Schedulers;
import javax.swing.Action;
import javax.swing.SwingUtilities;
import java.awt.Graphics;
import java.awt.Graphics2D;
import java.awt.RenderingHints;
import java.awt.event.ActionEvent;

public class UiUtils
{
    public static final Scheduler EDT = Schedulers.from(SwingUtilities::invokeLater);

    /* Fires the given action with the clicked component as its source, so that
     * actions which resolve their target from the event source work correctly.
     */
    static void fireAction(Action action, java.awt.Component source)
    {
        action.actionPerformed(new ActionEvent(source,
                ActionEvent.ACTION_PERFORMED,
                (String) action.getValue(Action.ACTION_COMMAND_KEY)));
    }

    public static Graphics2D getGraphics2D(Graphics g)
    {
        Graphics2D g2 = (Graphics2D) g;
        g2.setRenderingHint(RenderingHints.KEY_TEXT_ANTIALIASING,
                RenderingHints.VALUE_TEXT_ANTIALIAS_ON);
        return g2;
    }
}
