package com.cowlark.fluxengine.core;

import static com.google.common.truth.Truth.assertThat;
import static org.junit.Assert.assertThrows;

import com.cowlark.fluxengine.core.Flags.BoolFlag;
import com.cowlark.fluxengine.core.Flags.FlagGroup;
import com.cowlark.fluxengine.core.Flags.IntFlag;
import com.cowlark.fluxengine.core.Flags.StringFlag;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;
import java.util.List;

@RunWith(JUnit4.class)
public class FlagsTest
{
    @Test
    public void parsesFlags()
    {
        FlagGroup group = new FlagGroup();
        StringFlag config = StringFlag.builder()
                .setGroup(group)
                .setNames(List.of("--config", "-c"))
                .setHelpText("config file")
                .build();
        IntFlag count = IntFlag.builder()
                .setGroup(group)
                .setNames(List.of("--count"))
                .setHelpText("count")
                .build();
        BoolFlag verbose = BoolFlag.builder()
                .setGroup(group)
                .setNames(List.of("--verbose"))
                .setHelpText("verbose")
                .build();

        group.parse(new String[]{"--config=foo", "-c", "bar", "--count", "7", "--verbose=true"});

        assertThat(config.get()).isEqualTo("bar");
        assertThat(count.get()).isEqualTo(7);
        assertThat(verbose.get()).isTrue();
    }

    @Test
    public void parsesParentGroups()
    {
        FlagGroup common = new FlagGroup();
        StringFlag serial = StringFlag.builder()
                .setGroup(common)
                .setNames(List.of("--serial"))
                .setHelpText("serial")
                .build();
        FlagGroup group = new FlagGroup(common);
        StringFlag thing = StringFlag.builder()
                .setGroup(group)
                .setNames(List.of("--thing"))
                .setHelpText("thing")
                .build();

        group.parse(new String[]{"--serial=abc", "--thing=xyz"});

        assertThat(serial.get()).isEqualTo("abc");
        assertThat(thing.get()).isEqualTo("xyz");
    }

    @Test
    public void duplicateNamesThrow()
    {
        FlagGroup group = new FlagGroup();
        StringFlag.builder().setGroup(group).setNames(List.of("--foo")).setHelpText("one").build();
        StringFlag.builder().setGroup(group).setNames(List.of("--foo")).setHelpText("two").build();

        assertThrows(IllegalStateException.class, () -> group.parse(new String[]{"--foo=x"}));
    }

    @Test
    public void unknownFlagThrows()
    {
        FlagGroup group = new FlagGroup();
        assertThrows(FluxEngineException.class, () -> group.parse(new String[]{"--nope=x"}));
    }

    @Test
    public void filenames()
    {
        FlagGroup group = new FlagGroup();
        List<String> filenames = group.parseWithFilenames(
                new String[]{"one.dsk", "two.dsk"},
                name -> name.equals("one.dsk"));

        assertThat(filenames).containsExactly("two.dsk");
    }

    @Test
    public void uninitialisedFlagThrows()
    {
        FlagGroup group = new FlagGroup();
        StringFlag flag = StringFlag.builder()
                .setGroup(group)
                .setNames(List.of("--foo"))
                .setHelpText("foo")
                .build();

        assertThrows(IllegalStateException.class, flag::get);
    }
}
