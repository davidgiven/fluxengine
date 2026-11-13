package com.cowlark.fluxengine.gui;

import static com.google.common.truth.Truth.assertThat;
import static org.junit.Assert.assertThrows;

import com.cowlark.fluxengine.core.FluxEngineException;
import com.cowlark.fluxengine.usb.UsbFinder;
import com.google.common.collect.ImmutableList;
import java.awt.Component;
import java.nio.file.Files;
import java.nio.file.Path;
import java.util.concurrent.atomic.AtomicInteger;
import java.util.HashMap;
import java.util.Map;
import java.util.prefs.Preferences;
import javax.swing.JButton;
import javax.swing.JFileChooser;
import javax.swing.JLabel;
import javax.swing.JTextField;
import javax.swing.SwingUtilities;
import org.junit.After;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;
import org.mockito.MockedStatic;
import org.mockito.Mockito;
import sprouts.Association;
import sprouts.From;
import sprouts.Viewable;
import swingtree.SwingTree;
import swingtree.threading.EventProcessor;

@RunWith(JUnit4.class)
public class CustomFormatTest
{
    private Preferences preferences;
    private PreferencesReaderWriter writer;
    private ImageViewModel model;
    private Path file;
    private EventProcessor previousEventProcessor;

    @Before
    public void setUp() throws Exception
    {
        /* Isolate preferences, disk files, and USB discovery from the user's environment. */
        System.setProperty("java.awt.headless", "true");
        previousEventProcessor = SwingTree.get().getEventProcessor();
        SwingTree.get().setEventProcessor(EventProcessor.COUPLED);
        Map<String, String> savedPreferences = new HashMap<>();
        preferences = Mockito.mock(Preferences.class);
        Mockito.when(preferences.get(Mockito.anyString(), Mockito.anyString())).thenAnswer(
                call -> savedPreferences.getOrDefault(call.getArgument(0), call.getArgument(1)));
        Mockito.doAnswer(call -> {
            savedPreferences.put(call.getArgument(0), call.getArgument(1));
            return null;
        }).when(preferences).put(Mockito.anyString(), Mockito.anyString());
        writer = new PreferencesReaderWriter(preferences);
        model = newModel();
        file = Files.createTempFile("custom-format", ".pxf");
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
        Files.deleteIfExists(file);
        SwingTree.get().setEventProcessor(previousEventProcessor);
    }

    private void load() throws Exception
    {
        SwingUtilities.invokeAndWait(() -> model.onCustomFormatChanged(file.toString()));
    }

    @Test
    public void customSelectionUsesLoadedSnapshotRegardlessOfMetadata() throws Exception
    {
        Files.writeString(file, "shortname = \"ibm\"\ncomment = \"custom data\"\n");
        load();
        model.getSelectedFormat().set(From.VIEW, ImageViewModel.CUSTOM_FORMAT);

        /* Editing the file does not affect operations until an explicit reload. */
        Files.writeString(file, "comment = \"not reloaded\"\n");
        assertThat(model.makeConfigBuilderWithFormat().get("comment")).isEqualTo("custom data");
        assertThat(model.getFormatConfig("ibm").getComment()).isNotEqualTo("custom data");
        assertThat(model.getFormatNames()).contains(ImageViewModel.CUSTOM_FORMAT);
        assertThat(preferences.get("custom_format_filename", "")).isEqualTo(file.toString());
    }

    @Test
    public void sameFilenameAndIdenticalContentsStillNotify() throws Exception
    {
        Files.writeString(file, "comment = \"first\"\n");
        load();
        AtomicInteger notifications = new AtomicInteger();
        Viewable.cast(model.getCustomFormatFilename()).onChange(From.ALL, change -> {
            assertThat(model.getFormatConfig(ImageViewModel.CUSTOM_FORMAT).getComment()).isNotEmpty();
            notifications.incrementAndGet();
        });
        Files.writeString(file, "comment = \"second\"\n");
        load();
        load();

        assertThat(notifications.get()).isEqualTo(2);
        assertThat(model.getFormatConfig(ImageViewModel.CUSTOM_FORMAT).getComment()).isEqualTo("second");
    }

    @Test
    public void failedReloadPreservesWorkingSnapshotAndFilename() throws Exception
    {
        Files.writeString(file, "comment = \"working\"\n");
        load();
        Files.writeString(file, "invalid [");
        assertThrows(FluxEngineException.class, () -> model.onCustomFormatChanged(file.toString()));
        assertThrows(FluxEngineException.class, () -> model.onCustomFormatChanged(file + ".missing"));

        assertThat(model.getFormatConfig(ImageViewModel.CUSTOM_FORMAT).getComment()).isEqualTo("working");
        assertThat(model.getCustomFormatFilename().get()).isEqualTo(file.toString());
    }

    @Test
    public void loadingAnotherFileReplacesOnlyTheCustomSlot() throws Exception
    {
        Files.writeString(file, "shortname = \"first\"\n");
        load();
        Path replacement = Files.createTempFile("replacement-format", ".pxf");
        try
        {
            Files.writeString(replacement, "shortname = \"second\"\n");
            SwingUtilities.invokeAndWait(() -> model.onCustomFormatChanged(replacement.toString()));
            assertThat(model.getFormatConfig(ImageViewModel.CUSTOM_FORMAT).getShortname()).isEqualTo("second");
            assertThat(model.getCustomFormatFilename().get()).isEqualTo(replacement.toString());
            assertThat(model.getFormatNames().stream().filter(ImageViewModel.CUSTOM_FORMAT::equals).count())
                    .isEqualTo(1);
        } finally
        {
            Files.deleteIfExists(replacement);
        }
    }

    @Test
    public void keepsCompatibleChoicesAndDropsRemovedOptions() throws Exception
    {
        Files.writeString(file, """
                option = [{ name = "keep" }]
                option_group = [{ name = "density" option = [{ name = "dd" }] }]
                """);
        writer.setOptionsForFormat(ImageViewModel.CUSTOM_FORMAT,
                Association.between(String.class, String.class)
                        .put("keep", "").put("removed", "").put("density", "hd"));
        load();
        assertThat(writer.getOptionsForFormat(ImageViewModel.CUSTOM_FORMAT).toMap())
                .containsExactly("keep", "");

        writer.setOptionsForFormat(ImageViewModel.CUSTOM_FORMAT,
                Association.between(String.class, String.class).put("density", "dd"));
        load();
        assertThat(writer.getOptionsForFormat(ImageViewModel.CUSTOM_FORMAT).toMap())
                .containsExactly("density", "dd");
    }

    @Test
    public void restoresPreferencesAndSupportsLegacyTextproto() throws Exception
    {
        Files.writeString(file, "shortname: \"legacy\"\n");
        load();
        model.getSelectedFormat().set(From.VIEW, ImageViewModel.CUSTOM_FORMAT);
        model = newModel();
        assertThat(model.getSelectedFormat().get()).isEqualTo(ImageViewModel.CUSTOM_FORMAT);
        assertThat(model.getCustomFormatFilename().get()).isEqualTo(file.toString());
        assertThrows(FluxEngineException.class, () -> model.makeConfigBuilderWithFormat());
        load();
        assertThat(model.makeConfigBuilderWithFormat().get("shortname")).isEqualTo("legacy");
    }

    @Test
    public void controlsAppearOnlyInCustomModeAndReloadRebuildsOptions() throws Exception
    {
        SwingUtilities.invokeAndWait(() -> {
            ConfigurationPanel panel = new ConfigurationPanel(model);
            assertThat(button(panel, "Select file")).isNull();
            model.getSelectedFormat().set(From.VIEW, ImageViewModel.CUSTOM_FORMAT);
            assertThat(button(panel, "Select file")).isNotNull();
            assertThat(button(panel, "Reload file").isEnabled()).isFalse();
        });

        Files.writeString(file, "option = [{ name = \"test\" comment = \"First choice\" }]\n");
        load();
        SwingUtilities.invokeAndWait(() -> {
            ConfigurationPanel panel = new ConfigurationPanel(model);
            assertThat(button(panel, "Reload file").isEnabled()).isTrue();
            assertThat(hasLabel(panel, "First choice")).isTrue();
            try
            {
                Files.writeString(file, "option = [{ name = \"test\" comment = \"Second choice\" }]\n");
            } catch (java.io.IOException e)
            {
                throw new AssertionError(e);
            }
            button(panel, "Reload file").doClick();
            assertThat(hasLabel(panel, "Second choice")).isTrue();
            assertThat(hasLabel(panel, "First choice")).isFalse();
            model.getSelectedFormat().set(From.VIEW, "ibm");
            assertThat(button(panel, "Select file")).isNull();
        });
    }

    @Test
    public void selectingAndCancellingFileChooser() throws Exception
    {
        Files.writeString(file, "comment = \"chosen\"\n");
        SwingUtilities.invokeAndWait(() -> {
            model.getSelectedFormat().set(From.VIEW, ImageViewModel.CUSTOM_FORMAT);
            ConfigurationPanel panel = new ConfigurationPanel(model);
            try (org.mockito.MockedConstruction<JFileChooser> choosers = Mockito.mockConstruction(
                    JFileChooser.class, (chooser, context) -> {
                        Mockito.when(chooser.showOpenDialog(panel)).thenReturn(JFileChooser.APPROVE_OPTION);
                        Mockito.when(chooser.getSelectedFile()).thenReturn(file.toFile());
                    }))
            {
                button(panel, "Select file").doClick();
                assertThat(model.getFormatConfig(ImageViewModel.CUSTOM_FORMAT).getComment()).isEqualTo("chosen");
                JFileChooser chooser = choosers.constructed().getFirst();
                Mockito.verify(chooser).setFileFilter(Mockito.any(javax.swing.filechooser.FileNameExtensionFilter.class));
            }

            /* Canceling leaves the working configuration, filename, and notifications alone. */
            AtomicInteger notifications = new AtomicInteger();
            Viewable.cast(model.getCustomFormatFilename()).onChange(From.ALL, change -> notifications.incrementAndGet());
            try (org.mockito.MockedConstruction<JFileChooser> choosers = Mockito.mockConstruction(
                    JFileChooser.class, (chooser, context) -> Mockito.when(chooser.showOpenDialog(panel))
                            .thenReturn(JFileChooser.CANCEL_OPTION)))
            {
                button(panel, "Select file").doClick();
                Mockito.verify(choosers.constructed().getFirst()).setSelectedFile(file.toFile());
                assertThat(notifications.get()).isEqualTo(0);
                assertThat(model.getCustomFormatFilename().get()).isEqualTo(file.toString());
                assertThat(model.getFormatConfig(ImageViewModel.CUSTOM_FORMAT).getComment()).isEqualTo("chosen");
            }
        });
    }

    @Test
    public void pathFieldClipsLongPathsWithoutWideningThePanel() throws Exception
    {
        SwingUtilities.invokeAndWait(() -> {
            model.getSelectedFormat().set(From.VIEW, ImageViewModel.CUSTOM_FORMAT);
            ConfigurationPanel panel = new ConfigurationPanel(model);
            panel.setSize(340, 1000);
            panel.doLayout();
            int initialWidth = panel.getPreferredSize().width;

            /* A remembered path can be displayed even when its file is unavailable. */
            String filename = "/very-long-directory".repeat(30) + "/custom-format.pxf";
            model.getCustomFormatFilename().set(From.VIEW_MODEL, filename);
            panel.doLayout();
            JTextField field = null;
            for (Component component : panel.getComponents())
                if (component instanceof JTextField candidate
                        && "customFormatPath".equals(candidate.getName()))
                    field = candidate;
            assertThat(field).isNotNull();
            assertThat(hasLabel(panel, "Path:")).isTrue();
            assertThat(field.isEditable()).isFalse();
            assertThat(field.getText()).isEqualTo(filename);
            assertThat(field.getToolTipText()).isEqualTo(filename);
            assertThat(field.getHorizontalAlignment()).isEqualTo(JTextField.RIGHT);
            assertThat(field.getCaretPosition()).isEqualTo(filename.length());
            assertThat(panel.getPreferredSize().width).isEqualTo(initialWidth);
            assertThat(field.getWidth()).isAtMost(200);
            assertThat(field.getX() + field.getWidth()).isAtMost(panel.getWidth());
            assertThat(buttonBounds(panel, "Select file").y).isGreaterThan(field.getY());
            assertThat(buttonBounds(panel, "Reload file").y)
                    .isEqualTo(buttonBounds(panel, "Select file").y);
            assertThat(buttonBounds(panel, "Select file").x).isEqualTo(field.getX());
            assertThat(Math.abs(button(panel, "Select file").getWidth()
                    - button(panel, "Reload file").getWidth())).isAtMost(1);
            assertThat(buttonBounds(panel, "Reload file").x
                    + button(panel, "Reload file").getWidth())
                    .isEqualTo(field.getX() + field.getWidth());

            /* Resizing preserves the right-hand end of the path. */
            panel.setSize(300, 1000);
            panel.doLayout();
            assertThat(field.getX() + field.getWidth()).isAtMost(panel.getWidth());
            assertThat(buttonBounds(panel, "Select file").x).isEqualTo(field.getX());
            assertThat(Math.abs(button(panel, "Select file").getWidth()
                    - button(panel, "Reload file").getWidth())).isAtMost(1);
            assertThat(buttonBounds(panel, "Reload file").x
                    + button(panel, "Reload file").getWidth())
                    .isEqualTo(field.getX() + field.getWidth());
            assertThat(field.getCaretPosition()).isEqualTo(filename.length());
            model.getSelectedFormat().set(From.VIEW, "ibm");
            assertThat(hasLabel(panel, "Path:")).isFalse();
        });
    }

    private static java.awt.Rectangle buttonBounds(ConfigurationPanel panel, String text)
    {
        JButton button = button(panel, text);
        button.getParent().doLayout();
        return SwingUtilities.convertRectangle(button.getParent(), button.getBounds(), panel);
    }

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
}
