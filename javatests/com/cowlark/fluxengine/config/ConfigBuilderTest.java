package com.cowlark.fluxengine.config;

import static com.google.common.truth.Truth.assertThat;

import static org.junit.Assert.assertThrows;

import com.cowlark.fluxengine.core.flags.FlagGroup;
import com.google.common.collect.ImmutableList;
import java.io.IOException;
import java.nio.file.Files;
import java.nio.file.Path;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;

@RunWith(JUnit4.class)
public class ConfigBuilderTest
{
    /* ConfigBuilder defaults to a drive flux source, which makes build()
     * select a USB device; stub the serial so no hardware is needed. */
    private static ConfigBuilder builder()
    {
        return new ConfigBuilder().set("usb.serial", "test-serial");
    }

    @Test
    public void loadConfigFileMergesTextproto() throws IOException
    {
        Path file = Files.createTempFile("config", ".textproto");
        Files.writeString(file, "shortname: \"myconfig\"\ntracks: \"c=0:2\"\n");

        ConfigProto proto = builder().loadConfigFile(file.toString()).build();

        assertThat(proto.getShortname()).isEqualTo("myconfig");
        assertThat(proto.getTracks()).isEqualTo("c=0:2");
    }

    @Test
    public void loadConfigFileMergesAcrossFiles() throws IOException
    {
        Path first = Files.createTempFile("config", ".textproto");
        Path second = Files.createTempFile("config", ".textproto");
        Files.writeString(first, "shortname: \"first\"\n");
        Files.writeString(second, "tracks: \"c=0:2\"\n");

        ConfigProto proto = builder()
                .loadConfigFile(first.toString())
                .loadConfigFile(second.toString())
                .build();

        assertThat(proto.getShortname()).isEqualTo("first");
        assertThat(proto.getTracks()).isEqualTo("c=0:2");
    }

    @Test
    public void loadConfigFileMissingFileThrows()
    {
        assertThrows(ConfigException.class,
            () -> new ConfigBuilder().loadConfigFile("/nonexistent/config"));
    }

    @Test
    public void loadConfigFileBadTextprotoThrows() throws IOException
    {
        Path file = Files.createTempFile("config", ".textproto");
        Files.writeString(file, "this is not a valid textproto\n");

        assertThrows(ConfigException.class,
            () -> new ConfigBuilder().loadConfigFile(file.toString()));
    }

    @Test
    public void setMergesWithLoadedConfig() throws IOException
    {
        Path file = Files.createTempFile("config", ".textproto");
        Files.writeString(file, "shortname: \"myconfig\"\n");

        ConfigProto proto = builder()
                .loadConfigFile(file.toString())
                .set("tracks", "c=0:2")
                .build();

        assertThat(proto.getShortname()).isEqualTo("myconfig");
        assertThat(proto.getTracks()).isEqualTo("c=0:2");
    }

    @Test
    public void fromFlagsSetsDottedConfig()
    {
        ConfigProto proto = builder()
                .fromFlags(ImmutableList.of("--drive.drive=1"), new FlagGroup())
                .build();

        assertThat(proto.getDrive().getDrive()).isEqualTo(1);
    }
}
