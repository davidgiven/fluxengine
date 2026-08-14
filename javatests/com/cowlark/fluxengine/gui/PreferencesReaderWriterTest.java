package com.cowlark.fluxengine.gui;

import static com.google.common.truth.Truth.assertThat;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.when;

import com.google.common.collect.ImmutableMap;
import java.util.prefs.Preferences;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.Mock;
import org.mockito.junit.MockitoJUnitRunner;

@RunWith(MockitoJUnitRunner.class)
public class PreferencesReaderWriterTest
{
    @Mock private Preferences preferences;
    private PreferencesReaderWriter writer = null;

    @Test
    public void setOptionsForFormatEncodesIntoPreferences()
    {
        writer = new PreferencesReaderWriter(preferences);

        writer.setOptionsForFormat("ibm", ImmutableMap.of("tracks", "c0-80", "side", "0"));

        verify(preferences).put("format_ibm", "tracks=c0-80&side=0");
    }

    @Test
    public void getOptionsForFormatDecodesFromPreferences()
    {
        when(preferences.get("format_ibm", "")).thenReturn("tracks=c0-80&side=0");
        writer = new PreferencesReaderWriter(preferences);

        assertThat(writer.getOptionsForFormat("ibm"))
                .isEqualTo(ImmutableMap.of("tracks", "c0-80", "side", "0"));
    }

    @Test
    public void roundTripPreservesOptions()
    {
        writer = new PreferencesReaderWriter(preferences);
        ImmutableMap<String, String> options = ImmutableMap.of(
                "density", "hd",
                "cylinders", "0-79",
                "rotational-period-ms", "200");

        writer.setOptionsForFormat("ibm", options);

        when(preferences.get("format_ibm", "")).thenReturn(
                "density=hd&cylinders=0-79&rotational-period-ms=200");
        assertThat(writer.getOptionsForFormat("ibm")).isEqualTo(options);
    }

    @Test
    public void roundTripEncodesSpecialCharacters()
    {
        writer = new PreferencesReaderWriter(preferences);
        ImmutableMap<String, String> options = ImmutableMap.of(
                "comment", "hello world & goodbye",
                "path", "a=b%c+d");

        writer.setOptionsForFormat("amiga", options);

        String stored = options.entrySet()
                .stream()
                .map(entry -> java.net.URLEncoder.encode(entry.getKey(),
                        java.nio.charset.StandardCharsets.UTF_8) + "=" +
                        java.net.URLEncoder.encode(entry.getValue(),
                                java.nio.charset.StandardCharsets.UTF_8))
                .collect(java.util.stream.Collectors.joining("&"));
        when(preferences.get("format_amiga", "")).thenReturn(stored);

        assertThat(writer.getOptionsForFormat("amiga")).isEqualTo(options);
    }

    @Test
    public void getOptionsForFormatReturnsEmptyMapWhenNotSet()
    {
        when(preferences.get("format_unknown", "")).thenReturn("");
        writer = new PreferencesReaderWriter(preferences);

        assertThat(writer.getOptionsForFormat("unknown")).isEmpty();
    }

    @Test
    public void setPreferenceStoresValue()
    {
        writer = new PreferencesReaderWriter(preferences);

        writer.setPreference("last-format", "ibm");

        verify(preferences).put("last-format", "ibm");
    }

    @Test
    public void getPreferenceReturnsDefaultWhenNotSet()
    {
        when(preferences.get("missing", "default")).thenReturn("default");
        writer = new PreferencesReaderWriter(preferences);

        assertThat(writer.getPreference("missing", "default")).isEqualTo("default");
    }
}
