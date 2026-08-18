package com.cowlark.fluxengine.testing;

import com.cowlark.fluxengine.core.LogMessage;
import com.cowlark.fluxengine.core.LogRenderer;
import com.cowlark.fluxengine.core.Logger;
import org.junit.rules.TestRule;
import org.junit.runner.Description;
import org.junit.runners.model.Statement;
import java.util.function.Consumer;

/**
 * A JUnit rule which installs a stdout-rendering logger for the thread running
 * the test, and restores the previous logger afterwards. Attach it with:
 *
 * <pre>
 * &#64;Rule public final TestRule loggerRule = new LoggerRule();
 * </pre>
 */
public final class LoggerRule implements TestRule
{
    @Override
    public Statement apply(Statement base, Description description)
    {
        return new Statement()
        {
            @Override
            public void evaluate() throws Throwable
            {
                Consumer<? super LogMessage> oldLogger = Logger.getLogger();
                Logger.setLogger(LogRenderer.create(System.out)::add);
                try
                {
                    base.evaluate();
                } finally
                {
                    Logger.setLogger(oldLogger);
                }
            }
        };
    }
}
