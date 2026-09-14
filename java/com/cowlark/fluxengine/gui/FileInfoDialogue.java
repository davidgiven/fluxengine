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
import java.util.List;
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
                .add(
                        "grow, push, wmin 0",
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

    private static TableData createTableData(String col1, String col2, List<String[]> rows)
    {
        TableData tableData = TableData.of(UI.CellOrder.ROW_MAJOR, col1, col2);
        for (String[] row : rows)
        {
            String first = row.length > 0 ? row[0] : "";
            String second = row.length > 1 ? row[1] : "";
            tableData = tableData.addRow(first, second);
        }
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

    /**
     * Shows a modal dialog with a two-column table.
     *
     * @param parent      owner component for positioning (may be null)
     * @param title       dialog title
     * @param column1Name header for first column
     * @param column2Name header for second column
     * @param rows        table rows, each as two strings
     */
    public static void show(
            Component parent,
            String title,
            String column1Name,
            String column2Name,
            List<String[]> rows)
    {
        Window window = parent != null ? SwingUtilities.getWindowAncestor(parent) : null;
        Var<TableData> tableData = Var.of(createTableData(column1Name, column2Name, rows));

        JDialog dialog = new JDialog(window, title, ModalityType.APPLICATION_MODAL);
        UI
                .of((JPanel) dialog.getContentPane())
                .withLayout("fill, wrap 1, insets 12, gap 8")
                .add(
                        "grow, push, wmin 0",
                        scrollPane()
                                .withPrefSize(420, 260)
                                .add(UI.table(tableData).withPrefSize(400, 200)))
                .add("align right", button("Close").onClick(it -> dialog.dispose()));
        dialog.pack();
        dialog.setLocationRelativeTo(window);
        dialog.setDefaultCloseOperation(JDialog.DISPOSE_ON_CLOSE);
        dialog.setVisible(true);
    }
}
