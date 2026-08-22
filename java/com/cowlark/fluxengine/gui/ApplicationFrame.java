package com.cowlark.fluxengine.gui;

import static com.cowlark.fluxengine.gui.UiUtils.createSplitButton;
import static com.cowlark.fluxengine.gui.UiUtils.sizePts;
import static swingtree.UIFactoryMethods.button;
import static swingtree.UIFactoryMethods.comboBox;
import static swingtree.UIFactoryMethods.label;
import static swingtree.UIFactoryMethods.of;
import static swingtree.UIFactoryMethods.panel;
import static swingtree.UIFactoryMethods.scrollPane;
import static swingtree.UIFactoryMethods.splitPane;
import static swingtree.UIFactoryMethods.tab;
import static swingtree.UIFactoryMethods.tabbedPane;
import static swingtree.UILayoutConstants.BOTTOM;
import static swingtree.UILayoutConstants.TOP;

import com.cowlark.fluxengine.gui.UiUtils.SplitPanelAction;
import org.jspecify.annotations.NonNull;
import sprouts.Var;
import swingtree.UI;
import swingtree.UIForPanel;
import swingtree.UIForSplitPane;
import swingtree.UIForTabbedPane;
import swingtree.api.mvvm.ViewSupplier;
import javax.swing.JFrame;
import javax.swing.JPanel;
import javax.swing.JSplitPane;
import javax.swing.JTabbedPane;
import java.awt.Color;

public class ApplicationFrame extends JFrame
{
    /* Fixed size of the workflow card area, so that switching between
     * workflows does not shift the surrounding controls. */
    private static final int CARD_WIDTH_PTS = 430;
    private static final int CARD_HEIGHT_PTS = 40;

    private final ConfigurationPanel configurationPanel;
    private final VisualiserPanel visualiserPanel;
    private final ImagePanel imagePanel;
    private final LogPanel logPanel;
    private final SummaryPanel summaryPanel;

    private final ImagerViewModel model;

    enum Workflow
    {
        DISK_READING("read a disk"), DISK_WRITING("write a disk");

        private final String displayName;

        Workflow(String displayName)
        {
            this.displayName = displayName;
        }

        public String getDisplayName()
        {
            return displayName;
        }
    }

    private final Var<Workflow> currentWorkflow = Var.of(Workflow.DISK_READING);

    ApplicationFrame(ImagerViewModel model)
    {
        this.model = model;
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

        UIForPanel<JPanel> controlPanel =
                panel("wrap 4, center").add("aligny center", label("I want to")).add(
                        "aligny center",
                        comboBox(currentWorkflow, Workflow.values(), Workflow::getDisplayName)).add(
                        "aligny center",
                        panel("fill, ins 0")
                                .withSizeExactly(sizePts(CARD_WIDTH_PTS, CARD_HEIGHT_PTS))
                                .add(
                                        "align center",
                                        currentWorkflow,
                                        createControlPanelCard(model))).add(
                        "aligny center",
                        button("Stop")
                                .isEnabledIf(model.getBusy())
                                .withForeground(Color.RED)
                                .onClick(model::onEmergencyStop));

        UIForTabbedPane<JTabbedPane> bottomPane =
                tabbedPane().add(tab("Summary").add(panel("fillx, wrap 1, aligny center")
                        .add("growx, h 100pt!", of(summaryPanel))
                        .add("growx", controlPanel)));

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
                                .add(BOTTOM, bottomPane)));
    }

    private static @NonNull ViewSupplier<Workflow> createControlPanelCard(ImagerViewModel model)
    {
        return workflow -> switch (workflow)
        {
            case DISK_READING -> panel("center, nogrid, ins 0")
                    .add(button("Read disk").onClick(model::onReadDisk))
                    .add(label(" → "))
                    .add(createSplitButton(
                            SplitPanelAction
                                    .builder()
                                    .setLabel("Re-read bad tracks")
                                    .setOnClick(model::onRereadDisk)
                                    .build(),
                            SplitPanelAction
                                    .builder()
                                    .setLabel("Re-run the decode")
                                    .setOnClick(model::onRedecodeDisk)
                                    .build()))
                    .add(label(" → "))
                    .add(createSplitButton(
                            SplitPanelAction
                                    .builder()
                                    .setLabel("Save disk image")
                                    .setOnClick(model::onSaveDiskImage)
                                    .build(),
                            SplitPanelAction
                                    .builder()
                                    .setLabel("Save disk flux")
                                    .setOnClick(model::onSaveDiskFlux)
                                    .build()));
            case DISK_WRITING -> panel("center, nogrid, ins 0")
                    .add(button("Load disk image").onClick(model::onLoadDiskImage))
                    .add(label(" → "))
                    .add(createSplitButton(
                            SplitPanelAction
                                    .builder()
                                    .setLabel("Write flux to disk")
                                    .setOnClick(model::onWriteDisk)
                                    .build(),
                            SplitPanelAction
                                    .builder()
                                    .setLabel("Save flux to file")
                                    .setOnClick(model::onSaveDiskFlux)
                                    .build()));
        };
    }
}
