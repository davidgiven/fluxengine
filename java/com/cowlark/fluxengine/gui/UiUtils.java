package com.cowlark.fluxengine.gui;

import static java.lang.Math.round;
import static swingtree.UIFactoryMethods.menuItem;
import static swingtree.UIFactoryMethods.popupMenu;
import static swingtree.UIFactoryMethods.splitButton;

import io.reactivex.rxjava3.core.Scheduler;
import io.reactivex.rxjava3.schedulers.Schedulers;
import lombok.Builder;
import sprouts.Tuple;
import sprouts.Val;
import sprouts.Var;
import swingtree.ComponentDelegate;
import swingtree.UI;
import swingtree.UIForAnySwing;
import swingtree.UIForPopup;
import swingtree.layout.Size;
import javax.swing.Action;
import javax.swing.JButton;
import javax.swing.JPopupMenu;
import javax.swing.SwingUtilities;
import java.awt.Color;
import java.awt.Dimension;
import java.awt.FontMetrics;
import java.awt.Graphics;
import java.awt.Graphics2D;
import java.awt.RenderingHints;
import java.awt.event.ActionEvent;
import java.util.Arrays;
import java.util.function.Consumer;

public class UiUtils
{
    public static final Scheduler EDT = Schedulers.from(SwingUtilities::invokeLater);

    static final float PIXELS_PER_POINT = 96f / 72f;
    static final Color HEADER_COLOUR = new Color(0x00b4d8);
    static final Color DATA_OK_COLOUR = new Color(0x2ecc71);
    static final Color DATA_BAD_COLOUR = new Color(0xe74c3c);
    static final Color INDEX_LINE_COLOUR = new Color(0xf39c12);

    /* Fires the given action with the clicked component as its source, so that
     * actions which resolve their target from the event source work correctly.
     */
    static void fireAction(Action action, java.awt.Component source)
    {
        action.actionPerformed(new ActionEvent(
                source,
                ActionEvent.ACTION_PERFORMED,
                (String) action.getValue(Action.ACTION_COMMAND_KEY)));
    }

    public static Graphics2D getGraphics2D(Graphics g)
    {
        Graphics2D g2 = (Graphics2D) g;
        g2.setRenderingHint(
                RenderingHints.KEY_TEXT_ANTIALIASING,
                RenderingHints.VALUE_TEXT_ANTIALIAS_ON);
        return g2;
    }


    public static Dimension dimensionPts(float widthPt, float heightPt)
    {
        return new Dimension(round(widthPt * PIXELS_PER_POINT), round(heightPt * PIXELS_PER_POINT));
    }

    public static Size sizePts(float widthPt, float heightPt)
    {
        return Size.of(round(widthPt * PIXELS_PER_POINT), round(heightPt * PIXELS_PER_POINT));
    }

    public static UIForAnySwing<?, ?> createSplitButton(SplitPanelAction... actions)
    {
        Var<SplitPanelAction> selectedItem = Var.of(actions[0]);

        JPopupMenu popupMenu = popupMenu().addAll(
                Tuple.of(SplitPanelAction.class, actions),
                action -> menuItem(action.label()).onClick(it1 -> {
                    selectedItem.set(action);
                    action.onClick().accept(null);
                })).get(JPopupMenu.class);

        return splitButton(actions[0].label())
                .peek(b -> b.setPopupMenu(popupMenu))
                .withText(selectedItem.viewAs(String.class, SplitPanelAction::label))
                .onClick(it -> selectedItem.get().onClick().accept(
                        new ComponentDelegate<>(
                                it.getComponent(),
                                it.getEvent())))
                .peek(b -> {
                    /* Pin the button's preferred width to fit the longest
                     * label it can ever show, so that picking another action
                     * from the menu does not resize it and shift the layout. */
                    FontMetrics metrics = b.getFontMetrics(b.getFont());
                    int maxLabelWidth = Arrays
                            .stream(actions)
                            .map(action -> metrics.stringWidth(action.label()))
                            .max(Integer::compareTo)
                            .get();
                    Dimension preferred = b.getPreferredSize();
                    int borderPadding =
                            preferred.width - metrics.stringWidth(actions[0].label());
                    b.setPreferredSize(new Dimension(
                            maxLabelWidth + borderPadding + b.getSplitWidth(),
                            preferred.height));
                });
    }

    @Builder(setterPrefix = "set")
    record SplitPanelAction(String label, Val<Boolean> isEnabled,
                            Consumer<ComponentDelegate<JButton, ActionEvent>> onClick)
    {
    }
}
