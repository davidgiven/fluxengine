package com.cowlark.fluxengine.gui;

import org.jdesktop.swingx.treetable.DefaultMutableTreeTableNode;
import org.jdesktop.swingx.treetable.DefaultTreeTableModel;

public class FilesystemTreeTableModel extends DefaultTreeTableModel
{
    public FilesystemTreeTableModel(
            DefaultMutableTreeTableNode root)
    {
        super(root);
    }

    @Override
    public int getColumnCount()
    {
        return 2;
    }

    @Override
    public String getColumnName(int column)
    {
        return column == 0 ? "Name" : "Size";
    }
}
