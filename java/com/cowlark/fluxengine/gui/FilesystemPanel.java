package com.cowlark.fluxengine.gui;

import static swingtree.UIFactoryMethods.button;
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
        Val<Boolean> allowedWhenNotMounted = Viewable.of(
                model.getBusy(),
                treeTableModel.getIsMounted(),
                (busy, mounted) -> !mounted && !busy);

        //        DefaultMutableTreeTableNode root = new DefaultMutableTreeTableNode("Root");
        //
        //        DefaultMutableTreeTableNode docs = new DefaultMutableTreeTableNode("Documents");
        //        docs.add(new DefaultMutableTreeTableNode("Invoice.docx"));
        //        docs.add(new DefaultMutableTreeTableNode("Report.pdf"));
        //
        //        DefaultMutableTreeTableNode pics = new DefaultMutableTreeTableNode("Pictures");
        //        pics.add(new DefaultMutableTreeTableNode("Holiday.png"));
        //
        //        root.add(docs);
        //        root.add(pics);


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

        UI.of(this).withLayout("fill, wrap 1, insets 5")
                //                .add(
                //                        "growx",
                //                        panel("insets 2, gap 4")
                //                                .add(button("Up").isEnabledIf(allowedWhenMounted))
                //                                .add(button("New Folder").isEnabledIf
                //                                (allowedWhenMounted))
                //                                .add(button("Delete"))
                //                                .isEnabledIf(allowedWhenMounted))
                .add("grow, push", scrollPane().add(UI.of(treeTable))).add(
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
                                        "align right",
                                        label(bytesUsed.viewAs(
                                                String.class,
                                                FileUtils::byteCountToDisplaySize)).isVisibleIf(
                                                allowedWhenMounted))
                                .add("align right", label("/").isVisibleIf(allowedWhenMounted))
                                .add(
                                        "align right",
                                        label(bytesTotal.viewAs(
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
