package com.cowlark.fluxengine.core;

import static com.google.common.truth.Truth.assertThat;

import com.cowlark.fluxengine.algorithms.BeginReadOperationLogMessage;
import com.cowlark.fluxengine.core.LogMessage.ErrorLogMessage;
import com.cowlark.fluxengine.core.LogMessage.StringMessage;
import java.io.ByteArrayOutputStream;
import java.io.PrintStream;
import java.util.ArrayList;
import java.util.List;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;

@RunWith(JUnit4.class)
public class LoggerTest
{
    @Test
    public void logfStringWrapsInStringMessage()
    {
        List<LogMessage> messages = new ArrayList<>();
        Logger.setLogger(messages::add);

        Logger.logf("hello");

        assertThat(messages).containsExactly(new StringMessage("hello"));
    }

    @Test
    public void logMessagePassesThrough()
    {
        List<LogMessage> messages = new ArrayList<>();
        Logger.setLogger(messages::add);

        Logger.log(new ErrorLogMessage("oops"));

        assertThat(messages).containsExactly(new ErrorLogMessage("oops"));
    }

    @Test
    public void defaultLoggerRendersToStdout()
    {
        Logger.setLogger(message -> LogRenderer.create(System.out).add(message));

        ByteArrayOutputStream buffer = new ByteArrayOutputStream();
        PrintStream stream = new PrintStream(buffer);
        LogRenderer renderer = LogRenderer.create(stream);

        renderer.add(new BeginReadOperationLogMessage(3, 1));

        assertThat(buffer.toString()).isEqualTo("\nR 3.1: ");
    }

    @Test
    public void logUsesSetLogger()
    {
        List<LogMessage> messages = new ArrayList<>();
        Logger.setLogger(messages::add);

        Logger.logf("one");
        Logger.log(new StringMessage("two"));

        assertThat(messages).hasSize(2);
    }
}
