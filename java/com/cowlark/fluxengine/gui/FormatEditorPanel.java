package com.cowlark.fluxengine.gui;

import static com.cowlark.fluxengine.gui.UiUtils.and;
import static com.cowlark.fluxengine.gui.UiUtils.not;
import static com.google.common.base.MoreObjects.firstNonNull;
import static swingtree.UIFactoryMethods.button;
import static swingtree.UIFactoryMethods.panel;

import com.cowlark.fluxengine.data.Formats;
import com.formdev.flatlaf.util.UIScale;
import org.fife.ui.rsyntaxtextarea.AbstractTokenMakerFactory;
import org.fife.ui.rsyntaxtextarea.RSyntaxTextArea;
import org.fife.ui.rsyntaxtextarea.SyntaxConstants;
import org.fife.ui.rsyntaxtextarea.Theme;
import org.fife.ui.rsyntaxtextarea.TokenMakerFactory;
import org.fife.ui.rtextarea.RTextScrollPane;
import org.jspecify.annotations.NonNull;
import sprouts.From;
import sprouts.Val;
import sprouts.Viewable;
import swingtree.UI;
import javax.swing.JLayeredPane;
import javax.swing.JPanel;
import javax.swing.KeyStroke;
import javax.swing.text.DefaultEditorKit;
import javax.swing.UIManager;
import java.awt.Color;
import java.awt.Font;
import java.awt.event.FocusAdapter;
import java.awt.event.FocusEvent;
import java.io.IOException;
import java.io.InputStream;
import java.io.UncheckedIOException;

public class FormatEditorPanel extends JPanel
{
    /* Register the custom lexer once for all format editor documents. */
    static
    {
        AbstractTokenMakerFactory factory =
                (AbstractTokenMakerFactory) TokenMakerFactory.getDefaultInstance();
        factory.putMapping(PxfTokenMaker.SYNTAX_STYLE, PxfTokenMaker.class.getName());
    }

    private final RSyntaxTextArea textEditor;
    private final Val<Boolean> customSelected;
    private String displayedFormat;

    public FormatEditorPanel(ImageViewModel model)
    {
        /* Editing requires an idle engine and the custom format slot. */
        Val<Boolean> notBusy = not(model.getBusy());
        customSelected = model
                .getSelectedFormat()
                .viewAs(Boolean.class, ImageViewModel.CUSTOM_FORMAT::equals);

        /* Provide code editing features for PXF source. */
        textEditor = new RSyntaxTextArea();
        textEditor.setTabSize(4);
        textEditor.setAutoIndentEnabled(true);

        /* Insert double quotes literally so triple-quoted PXF strings can be typed without
         * RSyntaxTextArea adding matching quotes. */
        textEditor.getInputMap().put(KeyStroke.getKeyStroke('"'),
                DefaultEditorKit.defaultKeyTypedAction);

        /* Follow the selected format while preserving the editor's document notifications.
         * Only the custom slot follows the editable source property. */
        showSelectedFormat(model);
        Viewable
                .cast(model.getSelectedFormat())
                .onChange(From.ALL, change -> UI.run(() -> showSelectedFormat(model)));
        Viewable.cast(model.getCustomFormatText()).onChange(
                From.ALL, change -> {
                    UI.run(() -> {
                        if (ImageViewModel.CUSTOM_FORMAT.equals(displayedFormat) &&
                                ImageViewModel.CUSTOM_FORMAT.equals(model
                                        .getSelectedFormat()
                                        .get()))
                            replaceText(model.getCustomFormatText().get());
                    });
                });
        UI.of(textEditor).isEditableIf(and(notBusy, customSelected));
        textEditor.addFocusListener(new FocusAdapter()
        {
            @Override
            public void focusLost(FocusEvent event)
            {
                saveCustomText(model);
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
        } catch (IOException e)
        {
            throw new UncheckedIOException("Cannot load the format editor's dark theme", e);
        }
        textEditor.setFont(new Font(Font.MONOSPACED, Font.PLAIN, UIScale.scale(14)));

        /* Mute read-only source without disabling selection or copying. Restore the theme's
         * original colours when the custom format becomes editable again. */
        Runnable updateReadOnlyAppearance = getUpdateReadOnlyAppearance(editorScrollPane);
        textEditor.addPropertyChangeListener("editable", event -> updateReadOnlyAppearance.run());
        updateReadOnlyAppearance.run();

        /* Keep the copy action above the viewport without covering its scrollbars or
         * intercepting mouse events elsewhere in the editor. */
        int inset = Math.round(2 * textEditor.getFont().getSize2D());

        /* Toolbar. */
        UI.of(this).withLayout("fill, wrap 1, insets 5, hidemode 3")
                .add(
                        "growx",
                        panel("insets 2")
                                .add("align left", button("Load").isEnabledIf(notBusy))
                                .add("align left", button("Save").isEnabledIf(notBusy)))
                /* Let the existing window layout determine the viewport size instead of
                 * deriving it from the length and number of lines in the document. */.add(
                        "grow, push",
                        UI
                                .of(new JLayeredPane())
                                .withLayout("fill, insets 0, hidemode 3")
                                .add("grow, push", UI.of(editorScrollPane))
                                .add(
                                        "pos (container.w-pref-" + inset + "px) " + inset + "px",
                                        button("Customise")
                                                .isEnabledIf(notBusy)
                                                .isVisibleIfNot(customSelected)
                                                .onClick(event -> model.onCopyFromCurrentFormat())
                                                .peek(button -> JLayeredPane.putLayer(
                                                        button,
                                                        JLayeredPane.PALETTE_LAYER)))
                                .withPrefSize(0, 0)
                                .withMinSize(0, 0));
    }

    private @NonNull Runnable getUpdateReadOnlyAppearance(RTextScrollPane editorScrollPane)
    {
        Color editableForeground = textEditor.getForeground();
        Color editableBackground = textEditor.getBackground();
        Color gutterForeground = editorScrollPane.getGutter().getLineNumberColor();
        Color gutterBackground = editorScrollPane.getGutter().getBackground();
        return () -> {
            Color readOnlyForeground =
                    firstNonNull(
                            UIManager.getColor("TextArea.inactiveForeground"),
                            textEditor.getDisabledTextColor());
            Color readOnlyBackground =
                    firstNonNull(
                            UIManager.getColor("TextArea.inactiveBackground"),
                            editableBackground);
            boolean editable = textEditor.isEditable();
            textEditor.setSyntaxEditingStyle(editable ?
                    PxfTokenMaker.SYNTAX_STYLE :
                    SyntaxConstants.SYNTAX_STYLE_NONE);
            textEditor.setForeground(editable ? editableForeground : readOnlyForeground);
            textEditor.setBackground(editable ? editableBackground : readOnlyBackground);
            editorScrollPane
                    .getGutter()
                    .setLineNumberColor(editable ? gutterForeground : readOnlyForeground);
            editorScrollPane
                    .getGutter()
                    .setBackground(editable ? gutterBackground : readOnlyBackground);
        };
    }

    /* Save pending custom edits before a selection change replaces the document, even if
     * the selection changed before Swing delivered the focus-lost event. */
    private void saveCustomText(ImageViewModel model)
    {
        if (ImageViewModel.CUSTOM_FORMAT.equals(displayedFormat))
            model.getCustomFormatText().set(From.VIEW, textEditor.getText());
    }

    private void showSelectedFormat(ImageViewModel model)
    {
        String format = model.getSelectedFormat().get();
        if (!format.equals(displayedFormat))
            saveCustomText(model);

        /* Custom source comes from the saved editor value; built-in source uses its resource. */
        boolean custom = ImageViewModel.CUSTOM_FORMAT.equals(format);
        String text;
        try
        {
            text = custom ? model.getCustomFormatText().get() : Formats.loadText(format);
        } catch (IOException e)
        {
            throw new UncheckedIOException("Cannot load format source " + format, e);
        }
        displayedFormat = format;
        replaceText(text);
    }

    /* Avoid resetting the caret when committing unchanged text on focus loss. */
    private void replaceText(String text)
    {
        if (!textEditor.getText().equals(text))
        {
            textEditor.setText(text);
            textEditor.setCaretPosition(0);
        }
    }
}
