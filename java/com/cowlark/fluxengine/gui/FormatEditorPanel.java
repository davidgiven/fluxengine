package com.cowlark.fluxengine.gui;

import static swingtree.UIFactoryMethods.button;
import static swingtree.UIFactoryMethods.panel;

import com.formdev.flatlaf.util.UIScale;
import org.fife.ui.rsyntaxtextarea.RSyntaxTextArea;
import org.fife.ui.rsyntaxtextarea.Theme;
import org.fife.ui.rtextarea.RTextScrollPane;
import swingtree.UI;
import javax.swing.JPanel;
import java.awt.Font;
import java.io.IOException;
import java.io.InputStream;
import java.io.UncheckedIOException;

public class FormatEditorPanel extends JPanel
{
    private final RSyntaxTextArea textEditor;

    public FormatEditorPanel()
    {
        /* Provide code editing features for PXF source. A PXF syntax highlighter can be
         * added when format loading is implemented. */
        textEditor = new RSyntaxTextArea();
        textEditor.setTabSize(4);
        textEditor.setAutoIndentEnabled(true);

        /* Include the editor's line-number gutter in its scrolling container. */
        RTextScrollPane editorScrollPane = new RTextScrollPane(textEditor);
        editorScrollPane.setLineNumbersEnabled(true);

        /* Apply the bundled dark theme after attaching the gutter so both are styled. */
        try (InputStream themeStream = RSyntaxTextArea.class.getResourceAsStream(
                     "/org/fife/ui/rsyntaxtextarea/themes/dark.xml"))
        {
            Theme.load(themeStream).apply(textEditor);
        }
        catch (IOException e)
        {
            throw new UncheckedIOException("Cannot load the format editor's dark theme", e);
        }
        textEditor.setFont(new Font(Font.MONOSPACED, Font.PLAIN, UIScale.scale(14)));

        /* Follow the Files panel's toolbar and scrolling content layout. File operations
         * remain disabled until they are implemented. */
        UI.of(this).withLayout("fill, wrap 1, insets 5, hidemode 3")
                .add(
                        "growx",
                        panel("insets 2")
                                .add("align left", button("Load").isEnabledIf(false))
                                .add("align left", button("Save").isEnabledIf(false)))
                .add("grow, push", UI.of(editorScrollPane));
    }
}
