package com.cowlark.fluxengine.gui;

import static swingtree.UIFactoryMethods.button;
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
    private final ConfigurationPanel configurationPanel;
    private final VisualiserPanel visualiserPanel;
    private final ImagePanel imagePanel;
    private final LogPanel logPanel;
    private final SummaryPanel summaryPanel;
    private final StatusbarPanel statusbarPanel;

    private final ImagerViewModel model;

    ApplicationFrame(ImagerViewModel model)
    {
        this.model = model;
        statusbarPanel = new StatusbarPanel(model);
        summaryPanel = new SummaryPanel();
        logPanel = new LogPanel();
        imagePanel = new ImagePanel();
        visualiserPanel = new VisualiserPanel();
        configurationPanel = new ConfigurationPanel(model);

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
                                                                                "Read disk").onClick(model::onReadDisk))
                                                                        .add(button("Reread disk").onClick(
                                                                                model::onRereadDisk))
                                                                        .add(button("Write disk").onClick(
                                                                                model::onWriteDisk))))))))
                        .add("growx", statusbarPanel));
    }
}
