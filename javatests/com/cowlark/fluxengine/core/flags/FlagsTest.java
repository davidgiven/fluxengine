package com.cowlark.fluxengine.core.flags;

import static com.google.common.truth.Truth.assertThat;

import static org.junit.Assert.assertThrows;

import com.cowlark.fluxengine.core.FluxEngineException;
import java.util.List;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;

@RunWith(JUnit4.class)
public class FlagsTest
{
    @Test
    public void parsesFlags()
    {
        FlagGroup group = new FlagGroup();
        StringFlag config = StringFlag.builder()
            .setGroup(group).setNames(List.of("--config", "-c")).setHelpText("config file").build();
        IntFlag count = IntFlag.builder()
            .setGroup(group).setNames(List.of("--count")).setHelpText("count").build();
        BoolFlag verbose = BoolFlag.builder()
            .setGroup(group).setNames(List.of("--verbose")).setHelpText("verbose").build();

        Flags.parse(new String[] {
            "--config=foo", "-c", "bar", "--count", "7", "--verbose=true"}, group);

        assertThat(config.get()).isEqualTo("bar");
        assertThat(count.get()).isEqualTo(7);
        assertThat(verbose.get()).isTrue();
    }

    @Test
    public void parsesParentGroups()
    {
        FlagGroup common = new FlagGroup();
        StringFlag serial = StringFlag.builder()
            .setGroup(common).setNames(List.of("--serial")).setHelpText("serial").build();
        FlagGroup group = new FlagGroup(common);
        StringFlag thing = StringFlag.builder()
            .setGroup(group).setNames(List.of("--thing")).setHelpText("thing").build();

        Flags.parse(new String[] {"--serial=abc", "--thing=xyz"}, group);

        assertThat(serial.get()).isEqualTo("abc");
        assertThat(thing.get()).isEqualTo("xyz");
    }

    @Test
    public void searchesAcrossMultipleRootGroups()
    {
        FlagGroup first = new FlagGroup();
        FlagGroup second = new FlagGroup();
        StringFlag thing = StringFlag.builder()
            .setGroup(second).setNames(List.of("--thing")).setHelpText("thing").build();

        Flags.parse(new String[] {"--thing=xyz"}, first, second);

        assertThat(thing.get()).isEqualTo("xyz");
    }

    @Test
    public void duplicateNamesThrow()
    {
        FlagGroup group = new FlagGroup();
        StringFlag.builder().setGroup(group).setNames(List.of("--foo")).setHelpText("one").build();
        StringFlag.builder().setGroup(group).setNames(List.of("--foo")).setHelpText("two").build();

        assertThrows(IllegalStateException.class,
            () -> Flags.parse(new String[] {"--foo=x"}, group));
    }

    @Test
    public void unknownFlagThrows()
    {
        FlagGroup group = new FlagGroup();
        assertThrows(FluxEngineException.class,
            () -> Flags.parse(new String[] {"--nope=x"}, group));
    }

    @Test
    public void filenames()
    {
        FlagGroup group = new FlagGroup();
        List<String> filenames = Flags.parseWithFilenames(
            new String[] {"one.dsk", "two.dsk"}, name -> name.equals("one.dsk"), group);

        assertThat(filenames).containsExactly("two.dsk");
    }

    @Test
    public void uninitialisedFlagThrows()
    {
        FlagGroup group = new FlagGroup();
        StringFlag flag = StringFlag.builder()
            .setGroup(group).setNames(List.of("--foo")).setHelpText("foo").build();

        assertThrows(IllegalStateException.class, flag::get);
    }

    @Test
    public void findFlagReturnsTheFlag()
    {
        FlagGroup group = new FlagGroup();
        StringFlag foo = StringFlag.builder()
            .setGroup(group).setNames(List.of("--foo", "-f")).setHelpText("foo").build();

        assertThat(group.findFlag("--foo")).isSameInstanceAs(foo);
        assertThat(group.findFlag("-f")).isSameInstanceAs(foo);
        assertThat(group.findFlag("--nope")).isNull();
    }

    @Test
    public void findFlagRecursesToParents()
    {
        FlagGroup common = new FlagGroup();
        StringFlag serial = StringFlag.builder()
            .setGroup(common).setNames(List.of("--serial")).setHelpText("serial").build();
        FlagGroup group = new FlagGroup(common);

        assertThat(group.findFlag("--serial")).isSameInstanceAs(serial);
    }

    @Test
    public void noArgFlagDoesNotConsumeFollowingToken()
    {
        FlagGroup group = new FlagGroup();
        SettableFlag flag = SettableFlag.builder()
            .setGroup(group).setNames(List.of("--read-only")).setHelpText("read only").build();

        List<String> filenames = Flags.parseWithFilenames(
            new String[] {"--read-only", "image.dsk"}, unused -> false, group);

        assertThat(flag.get()).isTrue();
        assertThat(filenames).containsExactly("image.dsk");
    }
}
