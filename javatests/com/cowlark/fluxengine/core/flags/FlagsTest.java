package com.cowlark.fluxengine.core.flags;

import static com.google.common.truth.Truth.assertThat;
import static org.junit.Assert.assertThrows;

import com.cowlark.fluxengine.core.FluxEngineException;
import com.google.common.collect.ImmutableList;
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
                .setName("--config")
                .setName("-c")
                .setHelpText("config file")
                .build();
        IntFlag count =
                IntFlag.builder().setGroup(group).setName("--count").setHelpText("count").build();
        BoolFlag verbose = BoolFlag.builder()
                .setGroup(group)
                .setName("--verbose")
                .setHelpText("verbose")
                .build();

        Flags.parse(ImmutableList.of("--config=foo", "-c", "bar", "--count", "7", "--verbose=true"),
                group);

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
                .setName("--serial")
                .setHelpText("serial")
                .build();
        FlagGroup group = new FlagGroup(common);
        StringFlag thing = StringFlag.builder()
                .setGroup(group)
                .setName("--thing")
                .setHelpText("thing")
                .build();

        Flags.parse(ImmutableList.of("--serial=abc", "--thing=xyz"), group);

        assertThat(serial.get()).isEqualTo("abc");
        assertThat(thing.get()).isEqualTo("xyz");
    }

    @Test
    public void searchesAcrossMultipleRootGroups()
    {
        FlagGroup first = new FlagGroup();
        FlagGroup second = new FlagGroup();
        StringFlag thing = StringFlag.builder()
                .setGroup(second)
                .setName("--thing")
                .setHelpText("thing")
                .build();

        Flags.parse(ImmutableList.of("--thing=xyz"), first, second);

        assertThat(thing.get()).isEqualTo("xyz");
    }

    @Test
    public void duplicateNamesThrow()
    {
        FlagGroup group = new FlagGroup();
        StringFlag.builder().setGroup(group).setName("--foo").setHelpText("one").build();
        StringFlag.builder().setGroup(group).setName("--foo").setHelpText("two").build();

        assertThrows(IllegalStateException.class,
                () -> Flags.parse(ImmutableList.of("--foo=x"), group));
    }

    @Test
    public void unknownFlagThrows()
    {
        FlagGroup group = new FlagGroup();
        assertThrows(FluxEngineException.class,
                () -> Flags.parse(ImmutableList.of("--nope=x"), group));
    }

    @Test
    public void filenames()
    {
        FlagGroup group = new FlagGroup();
        List<String> filenames = Flags.parseWithFilenames(ImmutableList.of("one.dsk", "two.dsk"),
                name -> name.equals("one.dsk"),
                group);

        assertThat(filenames).containsExactly("two.dsk");
    }

    @Test
    public void uninitialisedFlagThrows()
    {
        FlagGroup group = new FlagGroup();
        StringFlag flag =
                StringFlag.builder().setGroup(group).setName("--foo").setHelpText("foo").build();

        assertThrows(IllegalStateException.class, flag::get);
    }

    @Test
    public void findFlagReturnsTheFlag()
    {
        FlagGroup group = new FlagGroup();
        StringFlag foo = StringFlag.builder()
                .setGroup(group)
                .setName("--foo")
                .setName("-f")
                .setHelpText("foo")
                .build();

        assertThat(group.findFlag("--foo")).isSameInstanceAs(foo);
        assertThat(group.findFlag("-f")).isSameInstanceAs(foo);
        assertThat(group.findFlag("--nope")).isNull();
    }

    @Test
    public void setNameAddsEachName()
    {
        FlagGroup group = new FlagGroup();
        StringFlag flag = StringFlag.builder()
                .setGroup(group)
                .setName("--long")
                .setName("-l")
                .setName("-long")
                .setHelpText("flag")
                .build();

        assertThat(flag.names()).containsExactly("--long", "-l", "-long");
    }

    @Test
    public void setNamesTakesACollection()
    {
        FlagGroup group = new FlagGroup();
        StringFlag flag = StringFlag.builder()
                .setGroup(group)
                .setNames(List.of("--long", "-l"))
                .setHelpText("flag")
                .build();

        assertThat(flag.names()).containsExactly("--long", "-l");
    }

    @Test
    public void findFlagRecursesToParents()
    {
        FlagGroup common = new FlagGroup();
        StringFlag serial = StringFlag.builder()
                .setGroup(common)
                .setName("--serial")
                .setHelpText("serial")
                .build();
        FlagGroup group = new FlagGroup(common);

        assertThat(group.findFlag("--serial")).isSameInstanceAs(serial);
    }

    @Test
    public void noArgFlagDoesNotConsumeFollowingToken()
    {
        FlagGroup group = new FlagGroup();
        SettableFlag flag = SettableFlag.builder()
                .setGroup(group)
                .setName("--read-only")
                .setHelpText("read only")
                .build();

        List<String> filenames =
                Flags.parseWithFilenames(ImmutableList.of("--read-only", "image.dsk"),
                        unused -> false,
                        group);

        assertThat(flag.get()).isTrue();
        assertThat(filenames).containsExactly("image.dsk");
    }
}
