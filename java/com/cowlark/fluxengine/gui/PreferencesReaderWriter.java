package com.cowlark.fluxengine.gui;

import static com.google.common.base.Strings.isNullOrEmpty;

import com.google.common.base.Enums;
import com.google.common.base.Splitter;
import com.google.common.collect.ImmutableSet;
import net.harawata.appdirs.AppDirsFactory;
import sprouts.Association;
import sprouts.From;
import sprouts.Pair;
import sprouts.Var;
import sprouts.Viewable;
import javax.swing.JFileChooser;
import java.io.File;
import java.io.IOException;
import java.io.UncheckedIOException;
import java.net.URLDecoder;
import java.net.URLEncoder;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.nio.file.NoSuchFileException;
import java.nio.file.Path;
import java.util.Map;
import java.util.prefs.Preferences;
import java.util.stream.Collectors;

public class PreferencesReaderWriter
{
    static final String DEVICE_OPTIONS = "device_options";
    static final String FORMAT = "format";
    static final String CUSTOM_FORMAT_FILENAME = "custom_format_filename";
    static final String CUSTOM_FORMAT_TEXT = "custom_format_text";
    static final String DEVICE = "device";
    static final String DRIVE = "drive";
    static final String WORKFLOW = "workflow";
    static final String CURRENT_DIRECTORY = "current_directory";
    static final String ADVANCED_SETTINGS = "advanced_settings";

    static final String DEVICE_FLUXFILE = "fluxfile";
    static final String DEVICE_SERIALPORT = "serialport";

    /* Large text values live in configuration files instead of the size-limited store. */
    private static final ImmutableSet<String> FILE_PREFERENCES = ImmutableSet.of(CUSTOM_FORMAT_TEXT);

    private final Preferences preferences;
    private final Path configDirectory;

    PreferencesReaderWriter(Preferences preferences)
    {
        this(preferences, Path.of(AppDirsFactory.getInstance()
                .getUserConfigDir("FluxEngine", null, "Cowlark")));
    }

    /* Allow callers to select an isolated configuration directory for tests. */
    PreferencesReaderWriter(Preferences preferences, Path configDirectory)
    {
        this.preferences = preferences;
        this.configDirectory = configDirectory;
    }

    private static String encode(String value)
    {
        return URLEncoder.encode(value, StandardCharsets.UTF_8);
    }

    Association<String, String> getOptionsForFormat(String format)
    {
        return getOptions("format_" + format);
    }

    void setOptionsForFormat(String format, Association<String, String> options)
    {
        setOptions("format_" + format, options);
    }

    Association<String, String> getOptions(String key)
    {
        String optionsString = preferences.get(key, "");
        Map<String, String> rawMap = Splitter
                .on('&')
                .omitEmptyStrings()
                .trimResults()
                .withKeyValueSeparator(Splitter.on('=').limit(2))
                .split(optionsString);

        /* Decode URL-encoded keys and values. */
        return rawMap
                .entrySet()
                .stream()
                .map(e -> Pair.of(URLDecoder.decode(e.getKey(), StandardCharsets.UTF_8),
                        URLDecoder.decode(e.getValue(), StandardCharsets.UTF_8)))
                .collect(Association.collectorOf(String.class, String.class));
    }

    void setOptions(String key, Association<String, String> options)
    {
        String optionsString = options
                .entrySet()
                .stream()
                .map(entry -> encode(entry.first()) + "=" + encode(entry.second()))
                .collect(Collectors.joining("&"));
        preferences.put(key, optionsString);
    }

    JFileChooser createFileChooser()
    {
        JFileChooser chooser = new JFileChooser();
        String currentDirectory = getStringPreference(CURRENT_DIRECTORY, "");
        if (!isNullOrEmpty(currentDirectory))
            chooser.setCurrentDirectory(new File(currentDirectory));
        /* Remember navigation even if the user subsequently cancels the dialog. */
        chooser.addPropertyChangeListener(
                JFileChooser.DIRECTORY_CHANGED_PROPERTY, change -> {
                    File directory = (File) change.getNewValue();
                    if (directory != null)
                        setStringPreference(CURRENT_DIRECTORY, directory.getAbsolutePath());
                });
        return chooser;
    }

    <E extends Enum<E>> Var<E> makeEnumPreference(String name, E defaultValue)
    {
        E initial = Enums
                .getIfPresent(defaultValue.getDeclaringClass(),
                        getStringPreference(name, defaultValue.name()))
                .or(defaultValue);
        Var<E> value = Var.of(initial);
        Viewable.cast(value).onChange(
                From.VIEW,
                change -> setStringPreference(
                        name,
                        change.currentValue().orElseThrowUnchecked().name()));
        return value;
    }

    String getStringPreference(String name, String defaultValue)
    {
        /* Missing files have the same default-value semantics as ordinary preferences. */
        if (FILE_PREFERENCES.contains(name))
        {
            Path file = configDirectory.resolve(name + ".dat");
            try
            {
                return Files.readString(file, StandardCharsets.UTF_8);
            } catch (NoSuchFileException e)
            {
                return defaultValue;
            } catch (IOException e)
            {
                throw new UncheckedIOException("Cannot read preference file " + file, e);
            }
        }
        return preferences.get(name, defaultValue);
    }

    void setStringPreference(String name, String value)
    {
        /* Create the application configuration directory only when a file is saved. */
        if (FILE_PREFERENCES.contains(name))
        {
            Path file = configDirectory.resolve(name + ".dat");
            try
            {
                Files.createDirectories(configDirectory);
                Files.writeString(file, value, StandardCharsets.UTF_8);
            } catch (IOException e)
            {
                throw new UncheckedIOException("Cannot write preference file " + file, e);
            }
            return;
        }
        preferences.put(name, value);
    }

    int getIntegerPreference(String name, int defaultValue)
    {
        return preferences.getInt(name, defaultValue);
    }

    void setIntegerPreference(String name, int value)
    {
        preferences.putInt(name, value);
    }
}
