package com.cowlark.fluxengine.gui;

import static com.google.common.truth.Truth.assertThat;
import static org.junit.Assert.assertThrows;

import com.cowlark.fluxengine.config.ConfigBuilder;
import com.cowlark.fluxengine.core.FluxEngineException;
import com.cowlark.fluxengine.data.Formats;
import com.cowlark.fluxengine.usb.UsbFinder;
import com.google.common.collect.ImmutableList;
import org.fife.ui.rsyntaxtextarea.RSyntaxTextArea;
import org.fife.ui.rsyntaxtextarea.SyntaxConstants;
import org.fife.ui.rsyntaxtextarea.TextEditorPane;
import org.junit.After;
import org.junit.Before;
import org.junit.Rule;
import org.junit.Test;
import org.junit.rules.TemporaryFolder;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;
import org.mockito.MockedStatic;
import org.mockito.Mockito;
import sprouts.Association;
import sprouts.From;
import swingtree.SwingTree;
import swingtree.threading.EventProcessor;
import javax.swing.JButton;
import javax.swing.JLabel;
import javax.swing.JOptionPane;
import javax.swing.SwingUtilities;
import java.awt.Component;
import java.util.HashMap;
import java.util.Map;
import java.util.prefs.Preferences;

@RunWith(JUnit4.class)
public class CustomFormatTest
{
    @Rule public TemporaryFolder temporaryFolder = new TemporaryFolder();
    private Preferences preferences;
    private PreferencesReaderWriter writer;
    private ImageViewModel model;
    private EventProcessor previousEventProcessor;

    private static JButton button(java.awt.Container panel, String text)
    {
        for (Component component : panel.getComponents())
        {
            if (component instanceof JButton button && button.getText().equals(text))
                return button;
            if (component instanceof java.awt.Container container)
            {
                JButton found = button(container, text);
                if (found != null)
                    return found;
            }
        }
        return null;
    }

    private static boolean hasLabel(ConfigurationPanel panel, String text)
    {
        for (Component component : panel.getComponents())
            if (component instanceof JLabel label && label.getText().contains(text))
                return true;
        return false;
    }

    private static RSyntaxTextArea editor(java.awt.Container panel)
    {
        for (Component component : panel.getComponents())
        {
            if (component instanceof RSyntaxTextArea area)
                return area;
            if (component instanceof java.awt.Container container)
            {
                RSyntaxTextArea found = editor(container);
                if (found != null)
                    return found;
            }
        }
        return null;
    }

    /* Double quotes must be inserted literally, including consecutive triple delimiters. */
    @Test
    public void typingQuotesDoesNotInsertMatchingQuotes() throws Exception
    {
        SwingUtilities.invokeAndWait(() -> {
            model.getSelectedFormat().set(From.VIEW, ImageViewModel.CUSTOM_FORMAT);
            RSyntaxTextArea area = editor(new FormatEditorPanel(model));
            area.setText("");
            Object actionKey = area.getInputMap().get(javax.swing.KeyStroke.getKeyStroke('"'));
            javax.swing.Action action = area.getActionMap().get(actionKey);
            for (int count = 1; count <= 3; count++)
            {
                action.actionPerformed(new java.awt.event.ActionEvent(
                        area,
                        java.awt.event.ActionEvent.ACTION_PERFORMED,
                        "\""));
                assertThat(area.getText()).isEqualTo("\"".repeat(count));
            }
        });
    }

    /* Highlighting is active only while editing is allowed, including busy-state changes. */
    @Test
    public void syntaxHighlightingFollowsEditability() throws Exception
    {
        SwingUtilities.invokeAndWait(() -> {
            FormatEditorPanel panel = new FormatEditorPanel(model);
            RSyntaxTextArea area = editor(panel);
            assertThat(area.getSyntaxEditingStyle()).isEqualTo(SyntaxConstants.SYNTAX_STYLE_NONE);
            model.getSelectedFormat().set(From.VIEW, ImageViewModel.CUSTOM_FORMAT);
            assertThat(area.getSyntaxEditingStyle()).isEqualTo(PxfTokenMaker.SYNTAX_STYLE);
            area.setEditable(false);
            assertThat(area.getSyntaxEditingStyle()).isEqualTo(SyntaxConstants.SYNTAX_STYLE_NONE);
            area.setEditable(true);
            assertThat(area.getSyntaxEditingStyle()).isEqualTo(PxfTokenMaker.SYNTAX_STYLE);
            model.getSelectedFormat().set(From.VIEW, "ibm");
            assertThat(area.getSyntaxEditingStyle()).isEqualTo(SyntaxConstants.SYNTAX_STYLE_NONE);
        });
    }

    @Before
    public void setUp() throws Exception
    {
        /* Isolate preferences, disk files, and USB discovery from the user's environment. */
        System.setProperty("java.awt.headless", "true");
        previousEventProcessor = SwingTree.get().getEventProcessor();
        SwingTree.get().setEventProcessor(EventProcessor.COUPLED);
        Map<String, String> savedPreferences = new HashMap<>();
        preferences = Mockito.mock(Preferences.class);
        Mockito
                .when(preferences.get(Mockito.anyString(), Mockito.anyString()))
                .thenAnswer(call -> savedPreferences.getOrDefault(
                        call.getArgument(0),
                        call.getArgument(1)));
        Mockito.doAnswer(call -> {
            savedPreferences.put(call.getArgument(0), call.getArgument(1));
            return null;
        }).when(preferences).put(Mockito.anyString(), Mockito.anyString());
        writer = new PreferencesReaderWriter(preferences, temporaryFolder.getRoot().toPath());
        model = newModel();
    }

    private ImageViewModel newModel()
    {
        try (MockedStatic<UsbFinder> usb = Mockito.mockStatic(UsbFinder.class))
        {
            usb.when(UsbFinder::findUsbDevices).thenReturn(ImmutableList.of());
            return new ImageViewModel(writer);
        }
    }

    @After
    public void tearDown() throws Exception
    {
        SwingTree.get().setEventProcessor(previousEventProcessor);
    }

    /* The floating action follows format selection both initially and after changes. */
    @Test
    public void customiseButtonIsVisibleOnlyForBuiltInFormats() throws Exception
    {
        SwingUtilities.invokeAndWait(() -> {
            model.getSelectedFormat().set(From.VIEW, ImageViewModel.CUSTOM_FORMAT);
            FormatEditorPanel panel = new FormatEditorPanel(model);
            JButton customise = button(panel, "Customise");
            assertThat(customise.isVisible()).isFalse();

            model.getSelectedFormat().set(From.VIEW, "ibm");
            assertThat(customise.isVisible()).isTrue();

            model.getSelectedFormat().set(From.VIEW, ImageViewModel.CUSTOM_FORMAT);
            assertThat(customise.isVisible()).isFalse();
        });
    }

    /* Cancelling or closing the discard prompt preserves dirty text; confirming replaces it. */
    @Test
    public void customisePromptsBeforeDiscardingDirtyText() throws Exception
    {
        SwingUtilities.invokeAndWait(() -> {
            model.getSelectedFormat().set(From.VIEW, ImageViewModel.CUSTOM_FORMAT);
            FormatEditorPanel panel = new FormatEditorPanel(model);
            RSyntaxTextArea area = editor(panel);
            String dirtyText = "comment = \"unsaved draft\"\n# edited";
            area.setText(dirtyText);
            model.getSelectedFormat().set(From.VIEW, "ibm");
            String previewText = area.getText();
            String customText = model.getCustomFormatText().get();
            assertThat(customText).isEqualTo(dirtyText);
            try (MockedStatic<JOptionPane> dialogs = Mockito.mockStatic(JOptionPane.class))
            {
                dialogs.when(() -> JOptionPane.showConfirmDialog(
                        panel,
                        "You have unsaved changes. Do you want to discard them?",
                        "Unsaved changes",
                        JOptionPane.YES_NO_OPTION,
                        JOptionPane.WARNING_MESSAGE))
                        .thenReturn(JOptionPane.NO_OPTION, JOptionPane.CLOSED_OPTION, JOptionPane.YES_OPTION);
                for (int attempt = 0; attempt < 2; attempt++)
                {
                    button(panel, "Customise").doClick();
                    assertThat(area.getText()).isEqualTo(previewText);
                    assertThat(model.getSelectedFormat().get()).isEqualTo("ibm");
                    assertThat(model.getCustomFormatText().get()).isEqualTo(customText);
                }
                dialogs.verify(() -> JOptionPane.showConfirmDialog(
                        panel,
                        "You have unsaved changes. Do you want to discard them?",
                        "Unsaved changes",
                        JOptionPane.YES_NO_OPTION,
                        JOptionPane.WARNING_MESSAGE), Mockito.times(2));
                button(panel, "Customise").doClick();
                assertThat(model.getSelectedFormat().get()).isEqualTo(ImageViewModel.CUSTOM_FORMAT);
                assertThat(area.getText()).doesNotContain("# edited");
            }
        });
    }

    /* Previewing built-in formats must not mark the custom draft as saved. */
    @Test
    public void customDirtyStateSurvivesFormatChanges() throws Exception
    {
        SwingUtilities.invokeAndWait(() -> {
            model.getSelectedFormat().set(From.VIEW, ImageViewModel.CUSTOM_FORMAT);
            FormatEditorPanel panel = new FormatEditorPanel(model);
            TextEditorPane area = (TextEditorPane) editor(panel);
            assertThat(area.isDirty()).isFalse();
            String draft = "comment = \"draft\"";
            area.setText(draft);
            assertThat(area.isDirty()).isTrue();
            model.getSelectedFormat().set(From.VIEW, "ibm");
            assertThat(area.isDirty()).isFalse();
            model.getSelectedFormat().set(From.VIEW, "amiga");
            model.getSelectedFormat().set(From.VIEW, ImageViewModel.CUSTOM_FORMAT);
            assertThat(area.getText()).isEqualTo(draft);
            assertThat(area.isDirty()).isTrue();
        });
    }

    /* Built-in format preferences must survive option renames just like custom formats. */
    @Test
    public void ignoresRemovedSavedOptionsWhenBuildingConfig()
    {
        writer.setOptionsForFormat(
                "acornadfs",
                Association.between(String.class, String.class).put("f", "").put("160", ""));
        model.getSelectedFormat().set(From.VIEW, "acornadfs");
        ConfigBuilder builder = model.makeConfigBuilderWithFormat();
        builder.build();
        assertThat(builder.get("layout.tracks")).isEqualTo("40");
    }

    /* Editor text drives custom options and engine configuration without an external file. */
    @Test
    public void editorTextProvidesConfigurationAndOptions() throws Exception
    {
        SwingUtilities.invokeAndWait(() -> {
            model.getCustomFormatText().set(
                    From.VIEW,
                    "shortname = \"ibm\"\ncomment = \"custom data\"\n" +
                            "option = [{ name = \"test\" comment = \"First choice\" }]\n");
            model.getSelectedFormat().set(From.VIEW, ImageViewModel.CUSTOM_FORMAT);
            ConfigurationPanel panel = new ConfigurationPanel(model);
            assertThat(button(panel, "Select file")).isNull();
            assertThat(button(panel, "Reload file")).isNull();
            assertThat(hasLabel(panel, "First choice")).isTrue();
            assertThat(model.makeConfigBuilderWithFormat().get("comment")).isEqualTo("custom data");
            assertThat(model.getFormatConfig("ibm").getComment()).isNotEqualTo("custom data");

            model
                    .getCustomFormatText()
                    .set(From.VIEW, "option = [{ name = \"test\" comment = \"Second choice\" }]\n");
            assertThat(hasLabel(panel, "Second choice")).isTrue();
            assertThat(hasLabel(panel, "First choice")).isFalse();
        });
    }

    /* Invalid drafts remain editable but cannot start an engine operation. */
    @Test
    public void invalidEditorTextDoesNotBreakConfigurationPanel() throws Exception
    {
        SwingUtilities.invokeAndWait(() -> {
            model.getSelectedFormat().set(From.VIEW, ImageViewModel.CUSTOM_FORMAT);
            ConfigurationPanel panel = new ConfigurationPanel(model);
            model.getCustomFormatText().set(From.VIEW, "invalid [");
            assertThrows(FluxEngineException.class, () -> model.makeConfigBuilderWithFormat());
            assertThat(button(panel, "Select file")).isNull();
        });
    }

    /* File-backed editor contents are restored without explicitly loading a format file. */
    @Test
    public void restoresCustomSourceOnStartup()
    {
        model.getCustomFormatText().set(From.VIEW, "shortname: \"legacy\"\n");
        model.getSelectedFormat().set(From.VIEW, ImageViewModel.CUSTOM_FORMAT);
        model = newModel();
        assertThat(model.makeConfigBuilderWithFormat().get("shortname")).isEqualTo("legacy");
    }

    /* Customising preserves the source and selects the editable custom slot. */
    @Test
    public void customiseCopiesOriginalSource() throws Exception
    {
        model.onCopyFromCurrentFormat();
        assertThat(model.getSelectedFormat().get()).isEqualTo(ImageViewModel.CUSTOM_FORMAT);
        assertThat(model.getCustomFormatText().get()).isEqualTo(Formats.loadText("ibm"));
    }

    /* Valid source changes retain compatible choices and discard options that disappeared. */
    @Test
    public void sourceChangesReconcileSavedOptions()
    {
        writer.setOptionsForFormat(
                ImageViewModel.CUSTOM_FORMAT,
                Association
                        .between(String.class, String.class)
                        .put("keep", "")
                        .put("removed", "")
                        .put("density", "hd"));
        model.getCustomFormatText().set(
                From.VIEW,
                "option = [{ name = \"keep\" }]\n" +
                        "option_group = [{ name = \"density\" option = [{ name = \"dd\" }] }]\n");
        assertThat(writer
                .getOptionsForFormat(ImageViewModel.CUSTOM_FORMAT)
                .toMap()).containsExactly("keep", "");
    }
}
