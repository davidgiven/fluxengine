package com.cowlark.fluxengine.gui;

import static swingtree.UI.of;
import static swingtree.UIFactoryMethods.scrollPane;

import lombok.With;
import sprouts.HasId;
import sprouts.Tuple;
import sprouts.Val;
import swingtree.UI;
import javax.swing.JPanel;
import javax.swing.JScrollPane;
import java.awt.BorderLayout;
import org.jdesktop.swingx.JXTreeTable;
import org.jdesktop.swingx.treetable.DefaultMutableTreeTableNode;
import org.jdesktop.swingx.treetable.DefaultTreeTableModel;


public class FilesystemPanel extends JPanel
{
    private final ImagerViewModel model;


    interface FsNode extends HasId<String>
    {
        String name();
    }

    @With
    record Dir(String id, String name, Tuple<FsNode> entries) implements FsNode
    {
    }

    @With
    record Doc(String id, String name, String body) implements FsNode
    {
    }

    public FilesystemPanel(ImagerViewModel model)
    {
        this.model = model;
        setLayout(new BorderLayout());

        DefaultMutableTreeTableNode root = new DefaultMutableTreeTableNode("Root");

        DefaultMutableTreeTableNode docs = new DefaultMutableTreeTableNode("Documents");
        docs.add(new DefaultMutableTreeTableNode("Invoice.docx"));
        docs.add(new DefaultMutableTreeTableNode("Report.pdf"));

        DefaultMutableTreeTableNode pics = new DefaultMutableTreeTableNode("Pictures");
        pics.add(new DefaultMutableTreeTableNode("Holiday.png"));

        root.add(docs);
        root.add(pics);

        FilesystemTreeTableModel tableModel = new FilesystemTreeTableModel(root);

        JXTreeTable treeTable = new JXTreeTable(tableModel);
        treeTable.setRootVisible(true);
        treeTable.setShowGrid(true, true);
        treeTable.setLeafIcon(null);   // let FlatLaf's own icons show through
        treeTable.setOpenIcon(null);
        treeTable.setClosedIcon(null);

        treeTable.putClientProperty("FlatLaf.style", "showHorizontalLines: true");

        of(this).withLayout("fill, insets 5").add("grow, push", scrollPane().add(of(treeTable)));
    }
}
