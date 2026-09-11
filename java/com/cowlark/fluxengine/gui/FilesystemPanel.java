package com.cowlark.fluxengine.gui;

import static swingtree.UIFactoryMethods.button;
import static swingtree.UIFactoryMethods.html;
import static swingtree.UIFactoryMethods.label;
import static swingtree.UIFactoryMethods.panel;
import static swingtree.UIFactoryMethods.scrollPane;
import static swingtree.UIFactoryMethods.separator;

import org.apache.commons.io.FileUtils;
import org.jdesktop.swingx.JXTreeTable;
import sprouts.Val;
import sprouts.Var;
import sprouts.Viewable;
import swingtree.UI;
import javax.swing.JPanel;


public class FilesystemPanel extends JPanel
{
    private final ImagerViewModel model;
    private final FilesystemTreeTableModel treeTableModel;

    public FilesystemPanel(ImagerViewModel model)
    {
        this.model = model;
        this.treeTableModel = model.getFilesystemTreeTableModel();

        Val<Boolean> allowedWhenMounted = treeTableModel.getIsMounted().view();
        Val<Boolean> notMounted = allowedWhenMounted.viewAs(Boolean.class, m -> !m);
        Val<Boolean> allowedWhenNotMounted = Viewable.of(
                model.getBusy(),
                treeTableModel.getIsMounted(),
                (busy, mounted) -> !mounted && !busy);

        JXTreeTable treeTable = new JXTreeTable(model.getFilesystemTreeTableModel());
        treeTable.addTreeExpansionListener(model.getFilesystemTreeTableModel());
        treeTable.setRootVisible(true);
        treeTable.setShowGrid(true, true);
        treeTable.setLeafIcon(null);
        treeTable.setOpenIcon(null);
        treeTable.setClosedIcon(null);
        treeTable.expandRow(0);

        treeTable.putClientProperty("FlatLaf.style", "showHorizontalLines: true");

        Var<Integer> bytesTotal = model.getFilesystemTreeTableModel().getBytesTotal();
        Var<Integer> bytesUsed = model.getFilesystemTreeTableModel().getBytesUsed();


        UI
                .of(this)
                .withLayout("fill, wrap 1, insets 5, hidemode 3")
                .add("grow, push",
                        scrollPane().add(UI.of(treeTable)).isVisibleIf(allowedWhenMounted))
                .add(
                        "grow, push, align center, w 100%!", html("""
                                <html><center><b>Filesystem not mounted</b>
                                <br>Load some data and press 'Mount' to see files!</center></html>""")
                                .withHorizontalAlignment(UI.HorizontalAlignment.CENTER)
                                .isVisibleIf(notMounted))
                .add(
                        "growx", panel("insets 2")
                                .add("align left",
                                        button("Mount")
                                                .isEnabledIf(allowedWhenNotMounted)
                                                .onClick(delegate -> treeTableModel.mount()))
                                .add("align left",
                                        button("Discard")
                                                .isEnabledIf(allowedWhenMounted)
                                                .onClick(delegate -> treeTableModel.discard()))
                                .add("align left",
                                        button("Commit")
                                                .isEnabledIf(allowedWhenMounted)
                                                .onClick(delegate -> treeTableModel.commit()))
                                .add("align left",
                                        button("Unmount")
                                                .isEnabledIf(allowedWhenMounted)
                                                .onClick(delegate -> treeTableModel.unmount()))
                                .add("push", separator())
                                .add(
                                        "align right", label(bytesUsed.viewAs(
                                                String.class,
                                                FileUtils::byteCountToDisplaySize)).isVisibleIf(
                                                allowedWhenMounted))
                                .add("align right", label("/").isVisibleIf(allowedWhenMounted))
                                .add(
                                        "align right", label(bytesTotal.viewAs(
                                                String.class,
                                                FileUtils::byteCountToDisplaySize)).isVisibleIf(
                                                allowedWhenMounted))
                                .add(
                                        "align right", UI
                                                .progressBar(
                                                        UI.Axis.HORIZONTAL, 0, 100, Viewable.of(
                                                                bytesUsed,
                                                                bytesTotal,
                                                                (used, total) -> (total == 0) ?
                                                                        0 :
                                                                        (100 * used / total)))
                                                .isVisibleIf(allowedWhenMounted)

                                ));
    }

}
