package com.cowlark.fluxengine.gui;

import static com.google.common.truth.Truth.assertThat;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.when;

import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.Mock;
import org.mockito.junit.MockitoJUnitRunner;
import sprouts.Association;
import java.util.prefs.Preferences;

@RunWith(MockitoJUnitRunner.class)
public class PreferencesReaderWriterTest
{
    @Mock private Preferences preferences;
    private PreferencesReaderWriter writer = null;

    private static Association<String, String> assoc(String... pairs)
    {
        Association<String, String> result = Association.betweenLinked(String.class, String.class);
        for (int i = 0; i < pairs.length; i += 2)
            result = result.put(pairs[i], pairs[i + 1]);
        return result;
    }

    @Test
    public void setOptionsForFormatEncodesIntoPreferences()
    {
        writer = new PreferencesReaderWriter(preferences);

        writer.setOptionsForFormat("ibm", assoc("tracks", "c0-80", "side", "0"));

        verify(preferences).put("format_ibm", "tracks=c0-80&side=0");
    }

    @Test
    public void getOptionsForFormatDecodesFromPreferences()
    {
        when(preferences.get("format_ibm", "")).thenReturn("tracks=c0-80&side=0");
        writer = new PreferencesReaderWriter(preferences);

        assertThat(writer.getOptionsForFormat("ibm").toMap()).isEqualTo(assoc(
                "tracks",
                "c0-80",
                "side",
                "0").toMap());
    }

    @Test
    public void roundTripPreservesOptions()
    {
        writer = new PreferencesReaderWriter(preferences);
        Association<String, String> options =
                assoc("density", "hd", "cylinders", "0-79", "rotational-period-ms", "200");

        writer.setOptionsForFormat("ibm", options);

        when(preferences.get("format_ibm", "")).thenReturn(
                "density=hd&cylinders=0-79&rotational-period-ms=200");
        assertThat(writer.getOptionsForFormat("ibm").toMap()).isEqualTo(options.toMap());
    }

    @Test
    public void roundTripEncodesSpecialCharacters()
    {
        writer = new PreferencesReaderWriter(preferences);
        Association<String, String> options =
                assoc("comment", "hello world & goodbye", "path", "a=b%c+d");

        writer.setOptionsForFormat("amiga", options);

        String stored = options
                .entrySet()
                .stream()
                .map(entry -> java.net.URLEncoder.encode(
                        entry.first(),
                        java.nio.charset.StandardCharsets.UTF_8) + "=" + java.net.URLEncoder.encode(
                        entry.second(),
                        java.nio.charset.StandardCharsets.UTF_8))
                .collect(java.util.stream.Collectors.joining("&"));
        when(preferences.get("format_amiga", "")).thenReturn(stored);

        assertThat(writer.getOptionsForFormat("amiga").toMap()).isEqualTo(options.toMap());
    }

    @Test
    public void getOptionsForFormatReturnsEmptyMapWhenNotSet()
    {
        when(preferences.get("format_unknown", "")).thenReturn("");
        writer = new PreferencesReaderWriter(preferences);

        assertThat(writer.getOptionsForFormat("unknown")).isEmpty();
    }

    @Test
    public void setStringPreferenceStoresValue()
    {
        writer = new PreferencesReaderWriter(preferences);

        writer.setStringPreference("last-format", "ibm");

        verify(preferences).put("last-format", "ibm");
    }

    @Test
    public void getStringPreferenceReturnsDefaultWhenNotSet()
    {
        when(preferences.get("missing", "default")).thenReturn("default");
        writer = new PreferencesReaderWriter(preferences);

        assertThat(writer.getStringPreference("missing", "default")).isEqualTo("default");
    }
}
