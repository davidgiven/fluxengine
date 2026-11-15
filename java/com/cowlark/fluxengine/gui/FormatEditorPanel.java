package com.cowlark.fluxengine.gui;

import static swingtree.UIFactoryMethods.button;
import static swingtree.UIFactoryMethods.panel;

import com.formdev.flatlaf.util.UIScale;
import org.fife.ui.rsyntaxtextarea.RSyntaxTextArea;
import org.fife.ui.rsyntaxtextarea.Theme;
import org.fife.ui.rtextarea.RTextScrollPane;
import sprouts.From;
import sprouts.Val;
import sprouts.Viewable;
import swingtree.UI;
import javax.swing.JPanel;
import java.awt.Font;
import java.awt.event.FocusAdapter;
import java.awt.event.FocusEvent;
import java.io.IOException;
import java.io.InputStream;
import java.io.UncheckedIOException;

public class FormatEditorPanel extends JPanel
{
    private final RSyntaxTextArea textEditor;

    public FormatEditorPanel(ImageViewModel model)
    {
        /* Bind each interactive control because disabling a panel leaves its children enabled. */
        Val<Boolean> notBusy = model.getBusy().viewAs(Boolean.class, busy -> !busy);

        /* Provide code editing features for PXF source. A PXF syntax highlighter can be
         * added when format loading is implemented. */
        textEditor = new RSyntaxTextArea();
        textEditor.setTabSize(4);
        textEditor.setAutoIndentEnabled(true);

        /* Apply model changes without suppressing the document listeners used by the editor
         * to refresh its layout and scrollbars. Commit edits only when focus leaves. */
        Val<String> savedText = model.getCustomFormatText();
        textEditor.setText(savedText.get());
        textEditor.setCaretPosition(0);
        Viewable.cast(savedText).onChange(From.ALL, change -> {
            String text = change.currentValue().orElseThrowUnchecked();
            UI.run(() -> {
                if (!textEditor.getText().equals(text))
                {
                    textEditor.setText(text);
                    textEditor.setCaretPosition(0);
                }
            });
        });
        UI.of(textEditor).isEnabledIf(notBusy);
        textEditor.addFocusListener(new FocusAdapter()
        {
            @Override
            public void focusLost(FocusEvent event)
            {
                model.getCustomFormatText().set(From.VIEW, textEditor.getText());
            }
        });

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

        /* Toolbar. */
        UI.of(this).withLayout("fill, wrap 1, insets 5, hidemode 3")
                .add(
                        "growx",
                        panel("insets 2")
                                .add("align left", button("Load").isEnabledIf(notBusy))
                                .add("align left", button("Save").isEnabledIf(notBusy))
                                .add("align left", button("Copy from current format")
                                        .isEnabledIf(notBusy)
                                        .onClick(event -> model.onCopyFromCurrentFormat())))
                /* Let the existing window layout determine the viewport size instead of
                 * deriving it from the length and number of lines in the document. */
                .add("grow, push", UI.of(editorScrollPane)
                        .withPrefSize(0, 0)
                        .withMinSize(0, 0));
    }
}
