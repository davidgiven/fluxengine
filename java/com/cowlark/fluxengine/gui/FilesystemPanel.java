package com.cowlark.fluxengine.gui;

import static swingtree.UIFactoryMethods.button;
import static swingtree.UIFactoryMethods.html;
import static swingtree.UIFactoryMethods.label;
import static swingtree.UIFactoryMethods.panel;
import static swingtree.UIFactoryMethods.scrollPane;
import static swingtree.UIFactoryMethods.separator;

import org.apache.commons.io.FileUtils;
import org.jdesktop.swingx.JXTreeTable;
import org.slf4j.LoggerFactory;
import sprouts.Tuple;
import sprouts.Val;
import sprouts.Var;
import sprouts.Viewable;
import swingtree.UI;
import javax.swing.JPanel;
import javax.swing.tree.TreePath;
import javax.swing.tree.TreeSelectionModel;

public class FilesystemPanel extends JPanel
{
    private static final org.slf4j.Logger logger = LoggerFactory.getLogger(FilesystemPanel.class);

    private final ImagerViewModel model;
    private final JXTreeTable treeTable;
    private final FilesystemTreeTableModel treeTableModel;
    private final TreeSelectionModel treeSelectionModel;

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

        treeTable = new JXTreeTable(model.getFilesystemTreeTableModel());
        treeTable.addTreeExpansionListener(model.getFilesystemTreeTableModel());
        treeTable.setRootVisible(true);
        treeTable.setShowGrid(true, true);
        treeTable.setLeafIcon(null);
        treeTable.setOpenIcon(null);
        treeTable.setClosedIcon(null);
        treeTable.putClientProperty("FlatLaf.style", "showHorizontalLines: true");

        Var<Tuple<TreePath>> filesSelected = Var.of(Tuple.of(TreePath.class));
        treeSelectionModel = treeTable.getTreeSelectionModel();
        treeSelectionModel.setSelectionMode(TreeSelectionModel.DISCONTIGUOUS_TREE_SELECTION);
        treeSelectionModel.addTreeSelectionListener(e -> {
            TreePath[] paths = treeSelectionModel.getSelectionPaths();
            filesSelected.set(Tuple.of(TreePath.class, paths));
        });

        Var<Integer> bytesTotal = model.getFilesystemTreeTableModel().getBytesTotal();
        Var<Integer> bytesUsed = model.getFilesystemTreeTableModel().getBytesUsed();

        Val<Boolean> canDoSingleFileOperation = Viewable.of(
                allowedWhenMounted,
                filesSelected,
                (mounted, files) -> isValidSelection(files) && (files.size() == 1) && mounted);
        Val<Boolean> canDoMultiFileOperation = Viewable.of(
                allowedWhenMounted,
                filesSelected,
                (mounted, files) -> isValidSelection(files) && (files.size() >= 1) && mounted);

        UI
                .of(this)
                .withLayout("fill, wrap 1, insets 5, hidemode 3")
                .add(
                        "growx", panel("insets 2")
                                .add("align left",
                                        button("Get")
                                                .isEnabledIf(canDoMultiFileOperation)
                                                .onClick(d -> treeTableModel.getFile(filesSelected.get())))
                                .add("align left",
                                        button("Put").isEnabledIf(canDoSingleFileOperation))
                                .add("align left",
                                        button("Rename").isEnabledIf(canDoSingleFileOperation))
                                .add("align left",
                                        button("Delete").isEnabledIf(canDoMultiFileOperation))
                                .add("align left",
                                        button("Info").isEnabledIf(canDoSingleFileOperation))
                                .add("align left",
                                        button("View").isEnabledIf(canDoSingleFileOperation)))
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

    /**
     * Returns true if the selection is valid — i.e. no selected path is an
     * ancestor (or duplicate) of another selected path. Selections spanning
     * multiple unrelated folders are fine.
     */
    private static boolean isValidSelection(Tuple<TreePath> paths)
    {
        for (int i = 0; i < paths.size(); i++)
        {
            for (int j = i + 1; j < paths.size(); j++)
            {
                if (paths.get(i).isDescendant(paths.get(j)) ||
                        paths.get(j).isDescendant(paths.get(i)))
                {
                    return false;
                }
            }
        }
        return true;
    }

}
