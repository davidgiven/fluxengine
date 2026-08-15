package com.cowlark.fluxengine.gui;

import io.reactivex.rxjava3.core.Scheduler;
import io.reactivex.rxjava3.schedulers.Schedulers;
import sprouts.Association;
import sprouts.Pair;
import javax.swing.Action;
import javax.swing.SwingUtilities;
import java.awt.event.ActionEvent;
import java.util.function.Function;
import java.util.stream.Collector;

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
}
