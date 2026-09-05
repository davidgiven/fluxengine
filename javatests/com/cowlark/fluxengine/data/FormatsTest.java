package com.cowlark.fluxengine.data;

import static com.google.common.truth.Truth.assertThat;

import com.cowlark.fluxengine.config.ConfigProto;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;

@RunWith(JUnit4.class)
public class FormatsTest
{
    @Test
    public void looksUpConfigByName()
    {
        ConfigProto config = Formats.get("amiga");
        assertThat(config).isNotNull();
        assertThat(config.getShortname()).isEqualTo("Amiga");
    }

    @Test
    public void looksUpGlobalOptions()
    {
        ConfigProto config = Formats.get("_global_options");
        assertThat(config).isNotNull();
        assertThat(config.getIsExtension()).isTrue();
    }

    @Test
    public void returnsNullForUnknownName()
    {
        assertThat(Formats.get("not a real format")).isNull();
    }

    @Test
    public void returnsAllConfigNames()
    {
        assertThat(Formats.all()).hasSize(36);
        assertThat(Formats.all()).contains("ibm");
        assertThat(Formats.all()).contains("_global_options");
    }
}
