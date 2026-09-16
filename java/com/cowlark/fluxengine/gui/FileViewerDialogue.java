package com.cowlark.fluxengine.gui;

import static swingtree.UI.button;
import static swingtree.UI.scrollPane;
import static swingtree.UI.textArea;
import static swingtree.UIFactoryMethods.of;
import static swingtree.UIFactoryMethods.tab;
import static swingtree.UIFactoryMethods.tabbedPane;

import com.cowlark.fluxengine.core.Bytes;
import com.formdev.flatlaf.util.UIScale;
import org.exbin.auxiliary.binary_data.array.ByteArrayData;
import org.exbin.bined.CodeCharactersCase;
import org.exbin.bined.EditMode;
import org.exbin.bined.swing.basic.CodeArea;
import swingtree.UI;
import javax.swing.JDialog;
import javax.swing.JPanel;
import javax.swing.SwingUtilities;
import javax.swing.UIManager;
import javax.swing.border.LineBorder;
import java.awt.Component;
import java.awt.Font;
import java.awt.Window;
import java.nio.charset.StandardCharsets;

/**
 * A modal dialogue showing the contents of a file.
 *
 * <p>Two tabs are displayed: a read-only text view and a hex view using the
 * bined {@link CodeArea} (see {@link ImagePanel} for the bined setup). A Close
 * button is shown below the tabs.
 */
public class FileViewerDialogue extends JDialog
{
    private final CodeArea codeArea;

    private FileViewerDialogue(Window owner, String title, Bytes data)
    {
        super(owner, title, ModalityType.MODELESS);

        byte[] bytes = data.toByteArray();
        String text = UiUtils.buildSanitisedString(bytes);

        Font font = new Font(Font.MONOSPACED, Font.PLAIN, UIScale.scale(14));

        codeArea = new CodeArea();
        codeArea.setBorder(new LineBorder(UIManager.getColor("TextArea.background"), 3));
        codeArea.setCodeFont(font);
        codeArea.setContentData(new ByteArrayData(bytes));
        codeArea.setCharset(StandardCharsets.ISO_8859_1);
        codeArea.setCodeCharactersCase(CodeCharactersCase.LOWER);
        codeArea.setEditMode(EditMode.READ_ONLY);

        UI
                .of((JPanel) getContentPane())
                .withLayout("fill, wrap 1, insets 12, gap 8")
                .add(
                        "grow, push, wmin 0, hmin 300",
                        tabbedPane()
                                .add(tab("Text").add(scrollPane()
                                        .withPrefSize(600, 400)
                                        .add(textArea(text).isEditableIf(false).peek(ta -> {
                                            ta.setLineWrap(true);
                                            ta.setWrapStyleWord(true);
                                        }))))
                                .add(tab("Hex").add(scrollPane()
                                        .withPrefSize(600, 400)
                                        .add(of(codeArea)))))
                .add("align right", button("Close").onClick(it -> dispose()));

        pack();
        setLocationRelativeTo(owner);
        setDefaultCloseOperation(DISPOSE_ON_CLOSE);
    }

    /**
     * Shows a modeless dialog with a tabbed file viewer.
     *
     * @param parent owner component for positioning (may be null)
     * @param title  dialog title
     * @param data   file contents
     */
    public static void show(Component parent, String title, Bytes data)
    {
        Window window = parent != null ? SwingUtilities.getWindowAncestor(parent) : null;
        FileViewerDialogue dialog = new FileViewerDialogue(window, title, data);
        dialog.setVisible(true);
    }

}
