package com.cowlark.fluxengine.gui;

import static swingtree.UI.button;
import static swingtree.UI.scrollPane;

import com.google.common.collect.ImmutableMap;
import sprouts.Var;
import swingtree.UI;
import swingtree.api.model.TableData;
import javax.swing.JDialog;
import javax.swing.JPanel;
import javax.swing.SwingUtilities;
import java.awt.Component;
import java.awt.Window;
import java.util.Map;

/**
 * A simple modal dialogue box containing a two-column table and a Close button.
 * Built with SwingTree.
 *
 * <p>Usage:
 * <pre>
 *   SimpleTableDialog.show(parent, "File Info", ImmutableMap.of("Name", "foo.txt", "Size", "12K"));
 * </pre>
 */
public class FileInfoDialogue extends JDialog
{
    private FileInfoDialogue(Window owner, String title, ImmutableMap<String, String> data)
    {
        super(owner, title, ModalityType.MODELESS);

        Var<TableData> tableData = Var.of(createTableData(data));

        UI
                .of((JPanel) getContentPane())
                .withLayout("fill, wrap 1, insets 12, gap 8")
                .add("grow, push, wmin 0",
                        scrollPane()
                                .withPrefSize(420, 260)
                                .add(UI.table(tableData).withPrefSize(400, 200)))
                .add("align right", button("Close").onClick(it -> dispose()));

        pack();
        setLocationRelativeTo(owner);
        setDefaultCloseOperation(DISPOSE_ON_CLOSE);
    }

    private static TableData createTableData(ImmutableMap<String, String> data)
    {
        TableData tableData = TableData.of(UI.CellOrder.ROW_MAJOR, "Property", "Value");
        for (Map.Entry<String, String> entry : data.entrySet())
            tableData = tableData.addRow(entry.getKey(), entry.getValue());
        return tableData;
    }

    /**
     * Shows a modal dialog with a two-column table.
     *
     * @param parent owner component for positioning (may be null)
     * @param title  dialog title
     * @param data   two-column data as key/value map
     */
    public static void show(Component parent, String title, ImmutableMap<String, String> data)
    {
        Window window = parent != null ? SwingUtilities.getWindowAncestor(parent) : null;
        FileInfoDialogue dialog = new FileInfoDialogue(window, title, data);
        dialog.setVisible(true);
    }
}
