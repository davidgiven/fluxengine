package com.cowlark.fluxengine.gui;

import static com.google.common.truth.Truth.assertThat;

import com.google.common.collect.ImmutableSet;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;

@RunWith(JUnit4.class)
public class ConfigurationPanelTest
{
    @Test
    public void emptyEnabledApplicabilitiesAlwaysApplicable()
    {
        assertThat(ConfigurationPanel.testApplicability(ImmutableSet.of(),
                ImmutableSet.of("a", "b"))).isTrue();
        assertThat(ConfigurationPanel.testApplicability(ImmutableSet.of(),
                ImmutableSet.of())).isTrue();
    }

    @Test
    public void matchingOptionApplicabilitiesIsApplicable()
    {
        assertThat(ConfigurationPanel.testApplicability(ImmutableSet.of("a", "b"),
                ImmutableSet.of("b", "c"))).isTrue();
        assertThat(ConfigurationPanel.testApplicability(ImmutableSet.of("a"),
                ImmutableSet.of("a"))).isTrue();
    }

    @Test
    public void nonMatchingOptionApplicabilitiesIsNotApplicable()
    {
        assertThat(ConfigurationPanel.testApplicability(ImmutableSet.of("a", "b"),
                ImmutableSet.of("c", "d"))).isFalse();
    }

    @Test
    public void emptyOptionApplicabilitiesNotApplicableWhenEnabledSpecified()
    {
        assertThat(ConfigurationPanel.testApplicability(ImmutableSet.of("a"),
                ImmutableSet.of())).isFalse();
    }
}
