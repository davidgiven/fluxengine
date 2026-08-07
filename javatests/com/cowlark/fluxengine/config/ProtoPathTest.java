package com.cowlark.fluxengine.config;

import static com.google.common.truth.Truth.assertThat;

import static org.junit.Assert.assertThrows;

import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;

@RunWith(JUnit4.class)
public class ProtoPathTest
{
    private static ConfigProto set(String path, String value)
    {
        ConfigProto.Builder builder = ConfigProto.newBuilder();
        ProtoPath.set(builder, path, value);
        return builder.build();
    }

    @Test
    public void setTopLevelString()
    {
        assertThat(set("tracks", "c=0:2").getTracks()).isEqualTo("c=0:2");
    }

    @Test
    public void setNestedInt()
    {
        assertThat(set("drive.drive", "0").getDrive().getDrive()).isEqualTo(0);
    }

    @Test
    public void setNestedBool()
    {
        assertThat(set("drive.high_density", "y").getDrive().getHighDensity()).isTrue();
    }

    @Test
    public void setNestedEnum()
    {
        assertThat(set("drive.drive_type", "DRIVETYPE_80TRACK").getDrive()
                .getDriveType()
                .name()).isEqualTo("DRIVETYPE_80TRACK");
    }

    @Test
    public void setRepeatedStringWithIndex()
    {
        assertThat(set("documentation[2]", "hello").getDocumentationList()).containsExactly(
                "",
                "",
                "hello");
    }

    @Test
    public void setRepeatedMessageField()
    {
        assertThat(set("option[0].comment", "hello").getOption(0).getComment()).isEqualTo("hello");
    }

    @Test
    public void setMultipleFieldsMerges()
    {
        ConfigProto.Builder builder = ConfigProto.newBuilder();
        ProtoPath.set(builder, "tracks", "c=0:2");
        ProtoPath.set(builder, "drive.drive", "0");
        ProtoPath.set(builder, "drive.high_density", "y");

        ConfigProto proto = builder.build();

        assertThat(proto.getTracks()).isEqualTo("c=0:2");
        assertThat(proto.getDrive().getDrive()).isEqualTo(0);
        assertThat(proto.getDrive().getHighDensity()).isTrue();
    }

    @Test
    public void setRepeatedMessageFieldsMerge()
    {
        ConfigProto.Builder builder = ConfigProto.newBuilder();
        ProtoPath.set(builder, "option[0].comment", "first");
        ProtoPath.set(builder, "option[1].name", "second");

        ConfigProto proto = builder.build();

        assertThat(proto.getOption(0).getComment()).isEqualTo("first");
        assertThat(proto.getOption(1).getName()).isEqualTo("second");
    }

    @Test
    public void setUnknownFieldThrows()
    {
        assertThrows(ConfigException.class, () -> set("bogus", "x"));
    }

    @Test
    public void setUnknownNestedFieldThrows()
    {
        assertThrows(ConfigException.class, () -> set("drive.bogus", "x"));
    }

    @Test
    public void setMessageDirectlyThrows()
    {
        assertThrows(ConfigException.class, () -> set("drive", "x"));
    }

    @Test
    public void setBadNumberThrows()
    {
        assertThrows(ConfigException.class, () -> set("drive.drive", "notanumber"));
    }

    @Test
    public void setBadEnumThrows()
    {
        assertThrows(ConfigException.class, () -> set("drive.drive_type", "BOGUS"));
    }

    @Test
    public void setRepeatedWithoutIndexThrows()
    {
        assertThrows(ConfigException.class, () -> set("documentation", "x"));
    }

    @Test
    public void setIndexOnScalarThrows()
    {
        assertThrows(ConfigException.class, () -> set("tracks[0]", "x"));
    }
}
