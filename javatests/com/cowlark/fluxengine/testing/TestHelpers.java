package com.cowlark.fluxengine.testing;

import org.junit.rules.TestRule;

/**
 * A convenience wrapper for creating a {@link LoggerRule}.
 */
public final class TestHelpers
{
    private TestHelpers()
    {
    }

    public static TestRule loggerRule()
    {
        return new LoggerRule();
    }
}
