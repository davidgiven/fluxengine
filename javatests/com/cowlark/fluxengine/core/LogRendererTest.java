package com.cowlark.fluxengine.core;

import static com.google.common.truth.Truth.assertThat;

import com.cowlark.fluxengine.config.OptionLogMessage;
import com.cowlark.fluxengine.config.OptionProto;
import com.cowlark.fluxengine.core.LogMessage.BeginReadOperationLogMessage;
import com.cowlark.fluxengine.core.LogMessage.BeginWriteOperationLogMessage;
import com.cowlark.fluxengine.core.LogMessage.EmergencyStopMessage;
import com.cowlark.fluxengine.core.LogMessage.EndSpeedOperationLogMessage;
import com.cowlark.fluxengine.core.LogMessage.ErrorLogMessage;
import java.io.ByteArrayOutputStream;
import java.io.PrintStream;
import java.util.function.Consumer;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;

@RunWith(JUnit4.class)
public class LogRendererTest
{
    private static String render(Consumer<LogRenderer> action)
    {
        ByteArrayOutputStream buffer = new ByteArrayOutputStream();
        PrintStream stream = new PrintStream(buffer);
        LogRenderer renderer = LogRenderer.create(stream);
        action.accept(renderer);
        stream.flush();
        return buffer.toString();
    }

    @Test
    public void errorMessage()
    {
        String output = render(
                r -> r.add(new ErrorLogMessage("disk failed")));

        assertThat(output).isEqualTo("\n       Error: disk failed\n");
    }

    @Test
    public void emergencyStop()
    {
        String output = render(
                r -> r.add(new EmergencyStopMessage()));

        assertThat(output).isEqualTo("\n       Stop!\n");
    }

    @Test
    public void endSpeedOperation()
    {
        String output = render(
                r -> r.add(new EndSpeedOperationLogMessage(200e6)));

        assertThat(output).isEqualTo(
                "\n       Rotational period is 200.0ms (300.0rpm)\n");
    }

    @Test
    public void readOperationHeader()
    {
        String output = render(
                r -> r.add(new BeginReadOperationLogMessage(3, 1)));

        assertThat(output).isEqualTo("\nR 3.1: ");
    }

    @Test
    public void writeOperationHeader()
    {
        String output = render(
                r -> r.add(new BeginWriteOperationLogMessage(3, 1)));

        assertThat(output).isEqualTo("\nW 3.1: ");
    }

    @Test
    public void optionMessage()
    {
        OptionProto option = OptionProto.newBuilder()
                .setComment("high density")
                .build();
        String output = render(
                r -> r.add(new OptionLogMessage("user option", option)));

        assertThat(output).isEqualTo("\n       OPTION: user option: high density\n");
    }

    @Test
    public void commaSeparates()
    {
        String output = render(r ->
        {
            r.add("one");
            r.comma();
            r.add("two");
        });

        assertThat(output).isEqualTo(" one; two");
    }

    @Test
    public void addAfterNewlineIndents()
    {
        String output = render(r ->
        {
            r.add("one");
            r.newline();
            r.add("two");
        });

        assertThat(output).isEqualTo(" one\n       two");
    }
}
