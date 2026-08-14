package com.cowlark.fluxengine.gui;

import static com.google.common.collect.ImmutableMap.toImmutableMap;

import com.google.common.base.Splitter;
import com.google.common.collect.ImmutableMap;
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

    ImmutableMap<String, String> getOptionsForFormat(String format)
    {
        String optionsString = preferences.get("format_" + format, "");
        Map<String, String> rawMap = Splitter.on('&')
                .omitEmptyStrings()
                .trimResults()
                .withKeyValueSeparator(Splitter.on('=').limit(2))
                .split(optionsString);

        // Decode URL-encoded keys and values
        return rawMap.entrySet().stream().collect(toImmutableMap(
                e -> URLDecoder.decode(e.getKey(), StandardCharsets.UTF_8),
                e -> URLDecoder.decode(e.getValue(), StandardCharsets.UTF_8),
                (existing, replacement) -> existing));
    }

    void setOptionsForFormat(String format, ImmutableMap<String, String> options)
    {
        String optionsString = options.entrySet()
                .stream()
                .map(entry -> encode(entry.getKey()) + "=" + encode(entry.getValue()))
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
