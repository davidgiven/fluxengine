package com.cowlark.fluxengine.gui;

import static com.cowlark.fluxengine.gui.UiUtils.sizePts;
import static swingtree.UIFactoryMethods.button;
import static swingtree.UIFactoryMethods.of;
import static swingtree.UIFactoryMethods.panel;
import static swingtree.UIFactoryMethods.scrollPane;
import static swingtree.UIFactoryMethods.splitPane;
import static swingtree.UIFactoryMethods.tab;
import static swingtree.UIFactoryMethods.tabbedPane;
import static swingtree.UILayoutConstants.BOTTOM;
import static swingtree.UILayoutConstants.TOP;

import swingtree.UI;
import swingtree.UIForSplitPane;
import swingtree.UIForTabbedPane;
import javax.swing.JFrame;
import javax.swing.JSplitPane;
import javax.swing.JTabbedPane;

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
        summaryPanel = new SummaryPanel(model);
        logPanel = new LogPanel(model);
        imagePanel = new ImagePanel(model);
        visualiserPanel = new VisualiserPanel(model);
        configurationPanel = new ConfigurationPanel(model);

        UIForTabbedPane<JTabbedPane> leftPane =
                tabbedPane().add(tab("Configuration").add(scrollPane().add(of(configurationPanel))));

        UIForSplitPane<JSplitPane> topPane =
                splitPane(UI.Align.HORIZONTAL)
                        .peek(pane -> pane.setResizeWeight(1.0))
                        .add(
                                TOP,
                                tabbedPane()
                                        .add(tab("Image").add(of(imagePanel).withPrefSize(sizePts(
                                                500,
                                                300))))
                                        .add(tab("Log").add(of(logPanel))))
                        .add(BOTTOM, tabbedPane().add(tab("Visualiser").add(of(visualiserPanel))));

        UIForTabbedPane<JTabbedPane> bottomPane =
                tabbedPane().add(tab("Summary").add(panel("fillx, wrap 1, aligny " + "center")
                        .add("growx, h 100pt!", of(summaryPanel))
                        .add(
                                "growx",
                                panel("wrap 3, " + "alignx " + "center")
                                        .add(button("Read disk").onClick(model::onReadDisk))
                                        .add(button("Reread disk").onClick(model::onRereadDisk))
                                        .add(button("Write disk").onClick(model::onWriteDisk)))));

        UI
                .of(this)
                .withOnCloseOperation(UI.OnWindowClose.DISPOSE)
                .onClose(it -> System.exit(0))
                .peek(frame -> {
                    frame.setJMenuBar(ApplicationMenu.createMenu());
                })
                .add(panel("fill, wrap 2").add("growy", leftPane).add(
                        "grow, push",
                        splitPane(UI.Align.VERTICAL)
                                .peek(pane -> pane.setResizeWeight(1.0))
                                .add(TOP, topPane)
                                .add(BOTTOM, bottomPane)).add("growx, span 2", statusbarPanel));
    }
}
