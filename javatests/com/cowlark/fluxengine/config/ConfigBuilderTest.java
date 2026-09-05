package com.cowlark.fluxengine.config;

import static com.cowlark.fluxengine.testing.TestHelpers.loggerRule;
import static com.google.common.truth.Truth.assertThat;
import static org.junit.Assert.assertThrows;

import com.cowlark.fluxengine.core.FluxEngineException;
import com.cowlark.fluxengine.core.flags.FlagGroup;
import com.google.common.collect.ImmutableList;
import org.junit.Rule;
import org.junit.Test;
import org.junit.rules.TestRule;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;
import java.io.IOException;
import java.nio.file.Files;
import java.nio.file.Path;

@RunWith(JUnit4.class)
public class ConfigBuilderTest
{
    @Rule public final TestRule loggerRule = loggerRule();

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
        assertThrows(
                ConfigException.class,
                () -> new ConfigBuilder().loadConfigFile("/nonexistent/config"));
    }

    @Test
    public void loadConfigFileLoadsBuiltInFormatByName()
    {
        ConfigProto proto = builder().loadConfigFile("amiga").build();

        assertThat(proto.getShortname()).isEqualTo("Amiga");
    }

    @Test
    public void findOptionLooksUpTopLevelOption()
    {
        ConfigBuilder builder = builder().loadConfigFile("_global_options");

        ConfigBuilder.OptionInfo info = builder.findOption("hd");

        assertThat(info.option().getName()).isEqualTo("hd");
        assertThat(info.group()).isNull();
        assertThat(info.usesValue()).isFalse();
    }

    @Test
    public void buildAppliesDefaultOptions()
    {
        /* _global_options drivetype group has 80 set by default. */
        ConfigProto proto = builder().loadConfigFile("_global_options").build();

        assertThat(proto.getDrive().getTracks()).isEqualTo("c0-80h0-1");
        assertThat(proto
                .getDrive()
                .getDriveType()).isEqualTo(com.cowlark.fluxengine.external.DriveType.DRIVETYPE_80TRACK);
    }

    @Test
    public void buildDoesNotApplyDefaultForAppliedGroup()
    {
        /* If drivetype=40 is applied explicitly, the default (80) must not
         * also be applied. */
        ConfigBuilder builder = builder().loadConfigFile("_global_options");
        ConfigBuilder.OptionInfo info = builder.findOption("drivetype");
        builder.applyOption(info, "40");

        ConfigProto proto = builder.build();

        assertThat(proto.getDrive().getTracks()).isEqualTo("c0-40h0-1");
    }

    @Test
    public void findOptionLooksUpOptionInUnnamedGroup()
    {
        ConfigBuilder builder = builder().loadConfigFile("amiga");

        ConfigBuilder.OptionInfo info = builder.findOption("without_metadata");

        assertThat(info.option().getName()).isEqualTo("without_metadata");
        assertThat(info.group()).isNotNull();
        assertThat(info.usesValue()).isFalse();
    }

    @Test
    public void findOptionMissingThrows()
    {
        ConfigBuilder builder = builder().loadConfigFile("_global_options");

        assertThrows(ConfigException.class, () -> builder.findOption("no such option"));
    }

    @Test
    public void fromFlagsLooksUpOption()
    {
        /* --hd is a top-level option in _global_options; without a dot it is
         * looked up as an option rather than a config path. */
        ConfigBuilder builder = builder().loadConfigFile("_global_options");

        builder.fromFlags(ImmutableList.of("--hd"), new FlagGroup());

        assertThat(builder.findOption("hd").option().getName()).isEqualTo("hd");
    }

    @Test
    public void fromFlagsUnknownOptionThrows()
    {
        ConfigBuilder builder = builder().loadConfigFile("_global_options");

        assertThrows(
                FluxEngineException.class,
                () -> builder.fromFlags(ImmutableList.of("--no-such-option"), new FlagGroup()));
    }

    @Test
    public void fromFlagsOptionWithoutValue()
    {
        /* --hd is a top-level option with no value in _global_options. */
        ConfigBuilder builder = builder().loadConfigFile("_global_options");

        builder.fromFlags(ImmutableList.of("--hd"), new FlagGroup());

        assertThat(builder.findOption("hd").usesValue()).isFalse();
    }

    @Test
    public void fromFlagsOptionWithValue()
    {
        /* --drivetype is a top-level option with a value in _global_options. */
        ConfigBuilder builder = builder().loadConfigFile("_global_options");

        builder.fromFlags(ImmutableList.of("--drivetype=80"), new FlagGroup());

        assertThat(builder.findOption("drivetype").usesValue()).isTrue();
    }

    @Test
    public void fromFlagsConfigKeySetsValue()
    {
        /* A dotted key is a config path, not an option. */
        ConfigBuilder builder = builder();

        builder.fromFlags(ImmutableList.of("--drive.drive=1"), new FlagGroup());

        assertThat(builder.build().getDrive().getDrive()).isEqualTo(1);
    }

    @Test
    public void fromFlagsConfigKeyWithoutDotSetsValue()
    {
        /* A config key which doesn't have a dot (e.g. --tracks) is also a
         * config path, not an option. */
        ConfigBuilder builder = builder();

        builder.fromFlags(ImmutableList.of("--tracks=c0-80h0-1"), new FlagGroup());

        assertThat(builder.build().getTracks()).isEqualTo("c0-80h0-1");
    }

    @Test
    public void getReturnsConfigValue()
    {
        ConfigBuilder builder = builder().set("tracks", "c0-80h0-1");

        assertThat(builder.get("tracks")).isEqualTo("c0-80h0-1");
    }

    @Test
    public void getOnUnknownKeyThrows()
    {
        ConfigBuilder builder = builder();

        assertThrows(ProtoPathNotFoundException.class, () -> builder.get("nosuchfield"));
    }

    @Test
    public void applyOptionIsCallable()
    {
        ConfigBuilder builder = builder().loadConfigFile("_global_options");

        ConfigBuilder.OptionInfo info = builder.findOption("hd");
        builder.applyOption(info, null);
    }

    @Test
    public void applyOptionGroupSelectsOptionByValue()
    {
        ConfigBuilder builder = builder().loadConfigFile("_global_options");

        ConfigBuilder.OptionInfo info = builder.findOption("drivetype");
        builder.applyOption(info, "80");
    }

    @Test
    public void applyOptionGroupWithInvalidValueThrows()
    {
        ConfigBuilder builder = builder().loadConfigFile("_global_options");

        ConfigBuilder.OptionInfo info = builder.findOption("drivetype");
        assertThrows(ConfigException.class, () -> builder.applyOption(info, "bogus"));
    }

    @Test
    public void checkOptionValidAppliesWhenPrerequisiteMet() throws Exception
    {
        Path file = Files.createTempFile("config", ".textproto");
        Files.writeString(
                file, """
                        option {
                          name: "needs_serial"
                          prerequisite {
                            key: "usb.serial"
                            value: "test-serial"
                          }
                          config {
                            comment: "applied"
                          }
                        }
                        """);

        ConfigBuilder builder =
                builder().loadConfigFile(file.toString()).set("usb.serial", "test-serial");
        ConfigBuilder.OptionInfo info = builder.findOption("needs_serial");
        builder.applyOption(info, null);

        assertThat(builder.build().getComment()).isEqualTo("applied");
    }

    @Test
    public void checkOptionValidThrowsWhenPrerequisiteNotMet() throws Exception
    {
        Path file = Files.createTempFile("config", ".textproto");
        Files.writeString(
                file, """
                        option {
                          name: "needs_serial"
                          prerequisite {
                            key: "usb.serial"
                            value: "test-serial"
                          }
                          config {
                            comment: "applied"
                          }
                        }
                        """);

        ConfigBuilder builder =
                builder().loadConfigFile(file.toString()).set("usb.serial", "other");
        ConfigBuilder.OptionInfo info = builder.findOption("needs_serial");
        assertThrows(InapplicableOptionException.class, () -> builder.applyOption(info, null));
    }

    @Test
    public void findOptionNamedGroupReturnsUsesValue()
    {
        /* Named groups (drivetype, drivespeed, bus) are found, but they select
         * an option by value, so usesValue is true and no option is set. */
        ConfigBuilder builder = builder().loadConfigFile("_global_options");

        ConfigBuilder.OptionInfo info = builder.findOption("drivetype");

        assertThat(info.group()).isNotNull();
        assertThat(info.option()).isNull();
        assertThat(info.usesValue()).isTrue();
    }

    @Test
    public void loadConfigFileLoadsBuiltInFormatBeforeFile()
    {
        /* A file named "amiga" may exist, but the built-in format must take
         * precedence. */
        ConfigProto proto = builder().loadConfigFile("amiga").build();

        assertThat(proto.getShortname()).isEqualTo("Amiga");
    }

    @Test
    public void loadConfigFileBadTextprotoThrows() throws IOException
    {
        Path file = Files.createTempFile("config", ".textproto");
        Files.writeString(file, "this is not a valid textproto\n");

        assertThrows(
                ConfigException.class,
                () -> new ConfigBuilder().loadConfigFile(file.toString()));
    }

    @Test
    public void setMergesWithLoadedConfig() throws IOException
    {
        Path file = Files.createTempFile("config", ".textproto");
        Files.writeString(file, "shortname: \"myconfig\"\n");

        ConfigProto proto =
                builder().loadConfigFile(file.toString()).set("tracks", "c=0:2").build();

        assertThat(proto.getShortname()).isEqualTo("myconfig");
        assertThat(proto.getTracks()).isEqualTo("c=0:2");
    }

    @Test
    public void fromFlagsSetsDottedConfig()
    {
        ConfigProto proto =
                builder().fromFlags(ImmutableList.of("--drive.drive=1"), new FlagGroup()).build();

        assertThat(proto.getDrive().getDrive()).isEqualTo(1);
    }

    @Test
    public void withFluxSource()
    {
        ConfigProto proto = builder().withFluxSource("foo.flux").build();

        assertThat(proto.getFluxSource().getType()).isEqualTo(FluxSourceSinkType.FLUXTYPE_FLUX);
        assertThat(proto.getFluxSource().getFl2().getFilename()).isEqualTo("foo.flux");
    }

    @Test
    public void withFluxSourceDrive()
    {
        ConfigProto proto = builder().withFluxSource("drive:1").build();

        assertThat(proto.getFluxSource().getType()).isEqualTo(FluxSourceSinkType.FLUXTYPE_DRIVE);
        assertThat(proto.getDrive().getDrive()).isEqualTo(1);
    }

    @Test
    public void withImageWriter()
    {
        ConfigProto proto = builder().withImageWriter("out.dsk").build();

        assertThat(proto.getImageWriter().getType()).isEqualTo(ImageReaderWriterType.IMAGETYPE_IMG);
        assertThat(proto.getImageWriter().getFilename()).isEqualTo("out.dsk");
    }

    @Test
    public void withCopyFluxTo()
    {
        ConfigProto proto = builder().withCopyFluxTo("copy.scp").build();

        assertThat(proto
                .getDecoder()
                .getCopyFluxTo()
                .getType()).isEqualTo(FluxSourceSinkType.FLUXTYPE_SCP);
        assertThat(proto.getDecoder().getCopyFluxTo().getScp().getFilename()).isEqualTo("copy.scp");
    }

    @Test
    public void withFluxSink()
    {
        ConfigProto proto = builder().withFluxSink("vcd:vcdfiles").build();

        assertThat(proto.getFluxSink().getType()).isEqualTo(FluxSourceSinkType.FLUXTYPE_VCD);
        assertThat(proto.getFluxSink().getVcd().getDirectory()).isEqualTo("vcdfiles");
    }

    @Test
    public void withImageReader()
    {
        ConfigProto proto = builder().withImageReader("in.dim").build();

        assertThat(proto.getImageReader().getType()).isEqualTo(ImageReaderWriterType.IMAGETYPE_DIM);
        assertThat(proto.getImageReader().getFilename()).isEqualTo("in.dim");
    }

    @Test
    public void withImageWriterReadOnlyThrows()
    {
        assertThrows(ConfigException.class, () -> builder().withImageWriter("out.dim"));
    }

    @Test
    public void withImageReaderUnrecognisedThrows()
    {
        assertThrows(ConfigException.class, () -> builder().withImageReader("bogus"));
    }

    @Test
    public void withFluxSourceUnrecognisedThrows()
    {
        assertThrows(ConfigException.class, () -> builder().withFluxSource("bogus"));
    }

    @Test
    public void applyOptionsParsesSingleOption()
    {
        ConfigBuilder builder = builder().loadConfigFile("_global_options");
        builder.applyOptions("drivetype=40");
        assertThat(builder.build().getDrive().getTracks()).isEqualTo("c0-40h0-1");
    }

    @Test
    public void applyOptionsHandlesWhitespaceAroundEquals()
    {
        ConfigBuilder builder = builder().loadConfigFile("_global_options");
        builder.applyOptions("  drivetype  =  40  ");
        assertThat(builder.build().getDrive().getTracks()).isEqualTo("c0-40h0-1");
    }

    @Test
    public void applyOptionsIgnoresCommentsAndBlankLines()
    {
        ConfigBuilder builder = builder().loadConfigFile("_global_options");
        builder.applyOptions("# comment\n\n   # another comment\n drivetype=40 \n\n");
        assertThat(builder.build().getDrive().getTracks()).isEqualTo("c0-40h0-1");
    }

    @Test
    public void applyOptionsParsesMultipleOptions()
    {
        ConfigBuilder builder = builder().loadConfigFile("_global_options");
        builder.applyOptions("drivetype=40\ndrivespeed=300");
        assertThat(builder.build().getDrive().getTracks()).isEqualTo("c0-40h0-1");
        assertThat(builder.build().getDrive().getRotationalPeriodMs()).isEqualTo(200);
    }

    @Test
    public void applyOptionsThrowsOnMissingEquals()
    {
        ConfigBuilder builder = builder().loadConfigFile("_global_options");
        assertThrows(ConfigException.class, () -> builder.applyOptions("badoption"));
        assertThrows(ConfigException.class, () -> builder.applyOptions("drivetype 40"));
        assertThrows(ConfigException.class, () -> builder.applyOptions("hd"));
    }

    @Test
    public void applyOptionsThrowsOnEmptyKey()
    {
        ConfigBuilder builder = builder().loadConfigFile("_global_options");
        assertThrows(ConfigException.class, () -> builder.applyOptions("=40"));
        assertThrows(ConfigException.class, () -> builder.applyOptions("   =40"));
        assertThrows(ConfigException.class, () -> builder.applyOptions("= value"));
    }

    @Test
    public void applyOptionsHandlesCommentWithLeadingWhitespace()
    {
        ConfigBuilder builder = builder().loadConfigFile("_global_options");
        builder.applyOptions("   # comment with leading space\ndrivetype=40");
        assertThat(builder.build().getDrive().getTracks()).isEqualTo("c0-40h0-1");
    }

    @Test
    public void applyOptionsEmptyAndNullIsNoOp()
    {
        ConfigBuilder builder = builder().loadConfigFile("_global_options");
        builder.applyOptions("");
        builder.applyOptions("   \n\n");
        builder.applyOptions(null);
        // default drivetype 80 should still apply
        assertThat(builder.build().getDrive().getTracks()).isEqualTo("c0-80h0-1");
    }

    @Test
    public void applyOptionsValueWhitespaceIsTrimmed()
    {
        ConfigBuilder builder = builder().loadConfigFile("_global_options");
        builder.applyOptions("drivetype=  40   ");
        assertThat(builder.build().getDrive().getTracks()).isEqualTo("c0-40h0-1");
    }

    @Test
    public void applyOptionsHandlesConfigKey()
    {
        ConfigBuilder builder = builder();
        builder.applyOptions("tracks=c0-40h0-1");
        assertThat(builder.build().getTracks()).isEqualTo("c0-40h0-1");
    }

    @Test
    public void applyOptionsHandlesDottedConfigKey()
    {
        ConfigBuilder builder = builder();
        builder.applyOptions("drive.drive=1");
        assertThat(builder.build().getDrive().getDrive()).isEqualTo(1);
    }

    @Test
    public void applyOptionsConfigKeyWithWhitespace()
    {
        ConfigBuilder builder = builder();
        builder.applyOptions("  drive.drive  =  1  ");
        assertThat(builder.build().getDrive().getDrive()).isEqualTo(1);
    }

    @Test
    public void applyOptionsMixedOptionAndConfig()
    {
        ConfigBuilder builder = builder().loadConfigFile("_global_options");
        builder.applyOptions("drivetype=40\ndrive.drive=1");
        ConfigProto proto = builder.build();
        assertThat(proto.getDrive().getTracks()).isEqualTo("c0-40h0-1");
        assertThat(proto.getDrive().getDrive()).isEqualTo(1);
    }

    @Test
    public void applyOptionsThrowsOnUnknownKey()
    {
        ConfigBuilder builder = builder().loadConfigFile("_global_options");
        assertThrows(ConfigException.class, () -> builder.applyOptions("nosuchkey=value"));
        assertThrows(ConfigException.class, () -> builder.applyOptions("nosuch.config.key=value"));
    }

    @Test
    public void applyOptionsConfigKeyFallsBackFromOption()
    {
        // 'decoder.retries' is a config key, not an option – ensure fallback works
        ConfigBuilder builder = builder();
        builder.applyOptions("decoder.retries=5");
        assertThat(builder.build().getDecoder().getRetries()).isEqualTo(5);
    }
}
