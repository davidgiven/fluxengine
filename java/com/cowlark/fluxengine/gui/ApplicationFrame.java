package com.cowlark.fluxengine.gui;

import static swingtree.UIFactoryMethods.button;
import static swingtree.UIFactoryMethods.label;
import static swingtree.UIFactoryMethods.of;
import static swingtree.UIFactoryMethods.panel;
import static swingtree.UIFactoryMethods.scrollPane;
import static swingtree.UIFactoryMethods.splitPane;
import static swingtree.UIFactoryMethods.tab;
import static swingtree.UIFactoryMethods.tabbedPane;
import static swingtree.UILayoutConstants.BOTTOM;
import static swingtree.UILayoutConstants.LEFT;
import static swingtree.UILayoutConstants.RIGHT;
import static swingtree.UILayoutConstants.TOP;

import swingtree.UI;
import javax.swing.JFrame;

public class ApplicationFrame extends JFrame
{
    private final ConfigurationPanel configurationPanel = new ConfigurationPanel();
    private final VisualiserPanel visualiserPanel = new VisualiserPanel();
    private final ImagePanel imagePanel = new ImagePanel();
    private final LogPanel logPanel = new LogPanel();
    private final SummaryPanel summaryPanel = new SummaryPanel();
    private final StatusbarPanel statusbarPanel = new StatusbarPanel();

    ApplicationFrame()
    {
        UI.of(this)
                .withOnCloseOperation(UI.OnWindowClose.DISPOSE)
                .onClose(it -> System.exit(0))
                .peek(frame -> {
                    frame.setJMenuBar(ApplicationMenu.createMenu());
                    frame.setSize(1280, 720);
                    frame.setLocationRelativeTo(null);
                })
                .add(panel("fill, wrap 1").add(
                                "grow, push", splitPane(UI.Align.HORIZONTAL).add(
                                        LEFT,
                                        tabbedPane().add(tab("Configuration").add(scrollPane().add(of(
                                                configurationPanel))))).add(
                                        RIGHT,
                                        splitPane(UI.Align.VERTICAL).peek(pane -> pane.setResizeWeight(1.0))
                                                .add(
                                                        TOP,
                                                        tabbedPane().add(tab("Visualiser").add(of(
                                                                        visualiserPanel)))
                                                                .add(tab("Image").add(of(imagePanel)))
                                                                .add(tab("Log").add(of(logPanel))))
                                                .add(
                                                        BOTTOM,
                                                        tabbedPane().add(tab("Summary").add(panel(
                                                                "fillx, wrap 1, aligny center").add("growx, h 100!",
                                                                of(summaryPanel)).add(
                                                                "growx",
                                                                panel("wrap 3, alignx center").add(button(
                                                                                "Read disk"))
                                                                        .add(button("Reread disk"))
                                                                        .add(button("Write disk"))))))))
                        .add("growx", statusbarPanel));
    }
}
