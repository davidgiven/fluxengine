package com.cowlark.fluxengine.buildtools;

import static com.google.common.truth.Truth.assertThat;

import com.cowlark.fluxengine.config.ConfigProto;
import com.google.protobuf.TextFormat;
import org.junit.Rule;
import org.junit.Test;
import org.junit.rules.TemporaryFolder;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;
import java.nio.file.Files;
import java.nio.file.Path;

@RunWith(JUnit4.class)
public class ProtoEncodeTest
{
    private static final String PROTO_CLASS = "com.cowlark.fluxengine.config.ConfigProto";
    @Rule public TemporaryFolder tmp = new TemporaryFolder();

    private static ConfigProto parse(String textproto) throws TextFormat.ParseException
    {
        ConfigProto.Builder builder = ConfigProto.newBuilder();
        TextFormat.merge(textproto, builder);
        return builder.build();
    }

    @Test
    public void encodesPlainTextproto() throws Exception
    {
        String textpb = "shortname: 'test'\ncomment: 'a comment'\n";
        ConfigProto expected = parse(textpb);

        byte[] data = ProtoEncode.encodeToBytes(textpb, PROTO_CLASS);
        assertThat(ConfigProto.parseFrom(data)).isEqualTo(expected);
    }

    @Test
    public void encodesMultilineStrings() throws Exception
    {
        String textpb = "shortname: 'test'\n" + "documentation:\n" + "<<<\n" + "The first line\n" +
                "The second line\n" + ">>>\n";
        ConfigProto expected = parse("shortname: 'test'\n" +
                "documentation: \"The first line\\nThe second line\\n\"\n");

        byte[] data = ProtoEncode.encodeToBytes(textpb, PROTO_CLASS);
        assertThat(ConfigProto.parseFrom(data)).isEqualTo(expected);
    }

    @Test
    public void encodesMultilineStringsWithUnicode() throws Exception
    {
        String textpb = "shortname: 'test'\n" + "documentation:\n" + "<<<\n" + "Агат is Russian\n" +
                ">>>\n";
        ConfigProto expected =
                parse("shortname: 'test'\n" + "documentation: \"Агат is Russian\\n\"\n");

        byte[] data = ProtoEncode.encodeToBytes(textpb, PROTO_CLASS);
        assertThat(ConfigProto.parseFrom(data)).isEqualTo(expected);
    }

    @Test
    public void writesBinaryFileThatRoundTrips() throws Exception
    {
        String textpb = "shortname: 'agat'\ncomment: 'a format'\n";
        ConfigProto expected = parse(textpb);

        Path output = tmp.newFile("agat.bin").toPath();
        ProtoEncode.encodeToFile(textpb, output.toString(), PROTO_CLASS);

        byte[] data = Files.readAllBytes(output);
        assertThat(ConfigProto.parseFrom(data)).isEqualTo(expected);
    }
}
