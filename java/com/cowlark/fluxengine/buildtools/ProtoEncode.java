package com.cowlark.fluxengine.buildtools;

import com.cowlark.fluxengine.core.FluxEngineException;
import com.google.protobuf.Message;
import com.google.protobuf.TextFormat;
import java.io.IOException;
import java.lang.reflect.InvocationTargetException;
import java.lang.reflect.Method;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.nio.file.Path;
import java.util.ArrayList;
import java.util.Iterator;
import java.util.List;

/**
 * Reads a textpb file (with the {@code <<<}...{@code >>>} multiline string
 * extension) and writes out the binary representation of the encoded protobuf,
 * ported from scripts/protoencode.cc.
 *
 * Usage: ProtoEncode &lt;input.textpb&gt; &lt;output.bin&gt;
 * [&lt;proto-class-fqn&gt;]
 */
public final class ProtoEncode
{
    private ProtoEncode()
    {
    }

    public static void main(String[] args)
    {
        if (args.length < 2)
        {
            System.err.println(
                    "Usage: ProtoEncode <input.textpb> <output.bin> [<proto-class-fqn>]");
            System.exit(1);
        }

        String protoClass = args.length > 2
                ? args[2]
                : "com.cowlark.fluxengine.config.ConfigProto";

        try
        {
            byte[] data = encodeToBytes(readFile(args[0]), protoClass);
            Files.write(Path.of(args[1]), data);
        } catch (IOException e)
        {
            System.err.println("couldn't open file: " + e.getMessage());
            System.exit(1);
        } catch (FluxEngineException e)
        {
            System.err.println(e.getMessage());
            System.exit(1);
        }
    }

    /* Reads the textpb file, handling the multiline string extension, and
     * returns the serialized protobuf bytes. */
    public static byte[] encodeToBytes(String contents, String protoClass)
    {
        String processed = processMultilineStrings(contents);
        Message.Builder builder = newBuilder(protoClass);
        try
        {
            TextFormat.merge(processed, builder);
        } catch (TextFormat.ParseException e)
        {
            throw new FluxEngineException("cannot parse text proto: " + e.getMessage());
        }
        return builder.build().toByteArray();
    }

    /* Encodes the textpb and writes the serialized protobuf bytes to a file. */
    public static void encodeToFile(String contents, String output, String protoClass)
            throws IOException
    {
        Files.write(Path.of(output), encodeToBytes(contents, protoClass));
    }

    private static String readFile(String filename) throws IOException
    {
        return Files.readString(Path.of(filename), StandardCharsets.UTF_8);
    }

    private static String processMultilineStrings(String contents)
    {
        StringBuilder result = new StringBuilder();
        List<String> lines = new ArrayList<>();
        Iterator<String> it = contents.lines().iterator();
        while (it.hasNext())
            lines.add(it.next());
        int i = 0;
        while (i < lines.size())
        {
            String line = lines.get(i);
            if (line.equals("<<<"))
            {
                i++;
                while (i < lines.size())
                {
                    String s = lines.get(i++);
                    if (s.equals(">>>"))
                        break;

                    result.append('"');
                    int offset = 0;
                    while (offset < s.length())
                    {
                        int codePoint = s.codePointAt(offset);
                        offset += Character.charCount(codePoint);
                        if (codePoint <= 0xffff)
                            result.append(String.format("\\u%04x", codePoint));
                        else
                            result.append(String.format("\\U%08x", codePoint));
                    }
                    result.append("\\n\"\n");
                }
            } else
            {
                result.append(line).append('\n');
                i++;
            }
        }
        return result.toString();
    }

    private static Message.Builder newBuilder(String protoClass)
    {
        try
        {
            Class<?> clazz = Class.forName(protoClass);
            Method method = clazz.getMethod("newBuilder");
            return (Message.Builder) method.invoke(null);
        } catch (ClassNotFoundException | NoSuchMethodException | IllegalAccessException |
                InvocationTargetException | ClassCastException e)
        {
            throw new FluxEngineException("cannot create builder for " + protoClass + ": " + e);
        }
    }
}