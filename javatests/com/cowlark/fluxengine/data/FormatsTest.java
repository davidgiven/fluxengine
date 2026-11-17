package com.cowlark.fluxengine.data;

import static com.google.common.truth.Truth.assertThat;
import static org.junit.Assert.assertThrows;

import com.cowlark.fluxengine.config.ConfigProto;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;
import java.io.FileNotFoundException;

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

    /* Source loading reports the missing resource path to callers. */
    @Test
    public void missingSourceThrowsFileNotFoundException()
    {
        FileNotFoundException exception = assertThrows(
                FileNotFoundException.class, () -> Formats.loadText("not a real format"));
        assertThat(exception).hasMessageThat().isEqualTo("/formats/not a real format.pxf");
    }

    @Test
    public void returnsAllConfigNames()
    {
        assertThat(Formats.all()).hasSize(39);
        assertThat(Formats.all()).contains("ibm");
        assertThat(Formats.all()).contains("_global_options");
    }

    /* Every bundled format must be readable, including nested repeated fields. */
    @Test
    public void loadsEveryConfig()
    {
        for (String name : Formats.all())
            assertThat(Formats.get(name)).isNotNull();
    }
}
