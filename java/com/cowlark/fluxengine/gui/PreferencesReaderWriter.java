package com.cowlark.fluxengine.gui;

import com.google.common.base.Splitter;
import sprouts.Association;
import sprouts.Pair;
import java.net.URLDecoder;
import java.net.URLEncoder;
import java.nio.charset.StandardCharsets;
import java.util.Map;
import java.util.prefs.Preferences;
import java.util.stream.Collectors;

public class PreferencesReaderWriter
{
    static final String FORMAT = "format";

    private final Preferences preferences;

    PreferencesReaderWriter(Preferences preferences)
    {
        this.preferences = preferences;
    }

    Association<String, String> getOptionsForFormat(String format)
    {
        String optionsString = preferences.get("format_" + format, "");
        Map<String, String> rawMap = Splitter.on('&')
                .omitEmptyStrings()
                .trimResults()
                .withKeyValueSeparator(Splitter.on('=').limit(2))
                .split(optionsString);

        // Decode URL-encoded keys and values
        return rawMap.entrySet()
                .stream()
                .map(e -> Pair.of(URLDecoder.decode(e.getKey(), StandardCharsets.UTF_8),
                        URLDecoder.decode(e.getValue(), StandardCharsets.UTF_8)))
                .collect(Association.collectorOf(String.class, String.class));
    }

    void setOptionsForFormat(String format, Association<String, String> options)
    {
        String optionsString = options.entrySet()
                .stream()
                .map(entry -> encode(entry.first()) + "=" + encode(entry.second()))
                .collect(Collectors.joining("&"));
        preferences.put("format_" + format, optionsString);
    }

    String getPreference(String name, String defaultValue)
    {
        return preferences.get(name, defaultValue);
    }

    void setPreference(String name, String value)
    {
        preferences.put(name, value);
    }

    private static String encode(String value)
    {
        return URLEncoder.encode(value, StandardCharsets.UTF_8);
    }
}
