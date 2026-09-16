package com.cowlark.fluxengine.gui;

import static com.google.common.truth.Truth.assertThat;

import java.nio.charset.StandardCharsets;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;

@RunWith(JUnit4.class)
public class UiUtilsTest
{
    private static byte[] bs(String s)
    {
        return s.getBytes(StandardCharsets.ISO_8859_1);
    }

    private static byte[] bs(int... vals)
    {
        byte[] out = new byte[vals.length];
        for (int i = 0; i < vals.length; i++)
            out[i] = (byte) vals[i];
        return out;
    }

    @Test
    public void emptyStringIsUnchanged()
    {
        assertThat(UiUtils.buildSanitisedString(new byte[0])).isEqualTo("");
        assertThat(UiUtils.buildSanitisedString(bs(""))).isEqualTo("");
    }

    @Test
    public void printableAsciiIsUnchanged()
    {
        assertThat(UiUtils.buildSanitisedString(bs("Hello, World!"))).isEqualTo("Hello, World!");
        assertThat(UiUtils.buildSanitisedString(bs(" !\"#$%&'()*+,-./0123456789:;<=>?@")))
                .isEqualTo(" !\"#$%&'()*+,-./0123456789:;<=>?@");
        assertThat(UiUtils.buildSanitisedString(bs("ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_`")))
                .isEqualTo("ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_`");
        assertThat(UiUtils.buildSanitisedString(bs("abcdefghijklmnopqrstuvwxyz{|}~")))
                .isEqualTo("abcdefghijklmnopqrstuvwxyz{|}~");
    }

    @Test
    public void tabIsPreserved()
    {
        assertThat(UiUtils.buildSanitisedString(bs("a\tb"))).isEqualTo("a\tb");
        assertThat(UiUtils.buildSanitisedString(bs("\t"))).isEqualTo("\t");
        assertThat(UiUtils.buildSanitisedString(bs("\t\t"))).isEqualTo("\t\t");
        assertThat(UiUtils.buildSanitisedString(bs(9))).isEqualTo("\t");
        assertThat(UiUtils.buildSanitisedString(bs('a', 9, 'b'))).isEqualTo("a\tb");
    }

    @Test
    public void lfIsPreservedAsSingleNewline()
    {
        assertThat(UiUtils.buildSanitisedString(bs("\n"))).isEqualTo("\n");
        assertThat(UiUtils.buildSanitisedString(bs("a\nb"))).isEqualTo("a\nb");
        assertThat(UiUtils.buildSanitisedString(bs("a\n\nb"))).isEqualTo("a\n\nb");
        assertThat(UiUtils.buildSanitisedString(bs("\n\n"))).isEqualTo("\n\n");
        assertThat(UiUtils.buildSanitisedString(bs("a\n"))).isEqualTo("a\n");
        assertThat(UiUtils.buildSanitisedString(bs("\na"))).isEqualTo("\na");
        assertThat(UiUtils.buildSanitisedString(bs(10))).isEqualTo("\n");
    }

    @Test
    public void crIsConvertedToLf()
    {
        assertThat(UiUtils.buildSanitisedString(bs("\r"))).isEqualTo("\n");
        assertThat(UiUtils.buildSanitisedString(bs("a\rb"))).isEqualTo("a\nb");
        assertThat(UiUtils.buildSanitisedString(bs("a\r\rb"))).isEqualTo("a\n\nb");
        assertThat(UiUtils.buildSanitisedString(bs("\r\r"))).isEqualTo("\n\n");
        assertThat(UiUtils.buildSanitisedString(bs("a\r"))).isEqualTo("a\n");
        assertThat(UiUtils.buildSanitisedString(bs("\ra"))).isEqualTo("\na");
        assertThat(UiUtils.buildSanitisedString(bs(13))).isEqualTo("\n");
        assertThat(UiUtils.buildSanitisedString(bs('a', 13, 'b'))).isEqualTo("a\nb");
    }

    @Test
    public void crlfIsConvertedToSingleLf()
    {
        assertThat(UiUtils.buildSanitisedString(bs("\r\n"))).isEqualTo("\n");
        assertThat(UiUtils.buildSanitisedString(bs("a\r\nb"))).isEqualTo("a\nb");
        assertThat(UiUtils.buildSanitisedString(bs("a\r\n\r\nb"))).isEqualTo("a\n\nb");
        assertThat(UiUtils.buildSanitisedString(bs("\r\n\r\n"))).isEqualTo("\n\n");
        assertThat(UiUtils.buildSanitisedString(bs("a\r\n"))).isEqualTo("a\n");
        assertThat(UiUtils.buildSanitisedString(bs("\r\na"))).isEqualTo("\na");
        assertThat(UiUtils.buildSanitisedString(bs(13, 10))).isEqualTo("\n");
        assertThat(UiUtils.buildSanitisedString(bs('a', 13, 10, 'b'))).isEqualTo("a\nb");
    }

    @Test
    public void mixedLineEndingsAreNormalised()
    {
        // CRLF + LF
        assertThat(UiUtils.buildSanitisedString(bs("a\r\n\nb"))).isEqualTo("a\n\nb");
        // LF + CRLF
        assertThat(UiUtils.buildSanitisedString(bs("a\n\r\nb"))).isEqualTo("a\n\nb");
        // CR + LF
        assertThat(UiUtils.buildSanitisedString(bs("a\r\nb\nc\rd"))).isEqualTo("a\nb\nc\nd");
        // All three forms interleaved
        assertThat(UiUtils.buildSanitisedString(bs("x\r\ny\nz\ra"))).isEqualTo("x\ny\nz\na");
        assertThat(UiUtils.buildSanitisedString(bs("a\r\nb\rc\nd"))).isEqualTo("a\nb\nc\nd");
        assertThat(UiUtils.buildSanitisedString(bs("\r\n\n\r"))).isEqualTo("\n\n\n");
        // Raw bytes: CRLF as 13,10
        assertThat(UiUtils.buildSanitisedString(bs(13, 10, 10, 13))).isEqualTo("\n\n\n");
    }

    @Test
    public void nonPrintableCharactersAreEscaped()
    {
        assertThat(UiUtils.buildSanitisedString(bs(0))).isEqualTo("\\x00");
        assertThat(UiUtils.buildSanitisedString(bs(1))).isEqualTo("\\x01");
        assertThat(UiUtils.buildSanitisedString(bs(0x1f))).isEqualTo("\\x1f");
        // \t (0x09) and \n (0x0a) are not escaped; \r (0x0d) becomes \n
        assertThat(UiUtils.buildSanitisedString(bs(0x08))).isEqualTo("\\x08"); // backspace
        assertThat(UiUtils.buildSanitisedString(bs(0x0b))).isEqualTo("\\x0b"); // vertical tab
        assertThat(UiUtils.buildSanitisedString(bs(0x0c))).isEqualTo("\\x0c"); // form feed
        assertThat(UiUtils.buildSanitisedString(bs(0x0e))).isEqualTo("\\x0e");
        // DEL (127) is not printable
        assertThat(UiUtils.buildSanitisedString(bs(0x7f))).isEqualTo("\\x7f");
        // NUL via string conversion
        assertThat(UiUtils.buildSanitisedString(bs("\u0000"))).isEqualTo("\\x00");
        assertThat(UiUtils.buildSanitisedString(bs("\u0001"))).isEqualTo("\\x01");
    }

    @Test
    public void charactersAbove127AreEscaped()
    {
        assertThat(UiUtils.buildSanitisedString(bs(0x80))).isEqualTo("\\x80");
        assertThat(UiUtils.buildSanitisedString(bs(0xff))).isEqualTo("\\xff");
        assertThat(UiUtils.buildSanitisedString(bs(0xa0))).isEqualTo("\\xa0");
        // Check that all bytes 128..255 produce \xXX in lower-case hex
        assertThat(UiUtils.buildSanitisedString(bs(0x80, 0x81, 0xfe)))
                .isEqualTo("\\x80\\x81\\xfe");
        // High bytes via ISO-8859-1 string
        assertThat(UiUtils.buildSanitisedString(bs("\u0080"))).isEqualTo("\\x80");
        assertThat(UiUtils.buildSanitisedString(bs("\u00ff"))).isEqualTo("\\xff");
        // Ensure signed byte handling: (byte)0x80 == -128 should still be \x80
        assertThat(UiUtils.buildSanitisedString(new byte[] {(byte) 0x80})).isEqualTo("\\x80");
        assertThat(UiUtils.buildSanitisedString(new byte[] {(byte) 0xff})).isEqualTo("\\xff");
    }

    @Test
    public void printableBoundaryIsCorrect()
    {
        // 32 (space) and 126 (~) are the boundaries and should be preserved
        assertThat(UiUtils.buildSanitisedString(bs(" "))).isEqualTo(" ");
        assertThat(UiUtils.buildSanitisedString(bs("~"))).isEqualTo("~");
        assertThat(UiUtils.buildSanitisedString(bs(32, 126))).isEqualTo(" ~");
        assertThat(UiUtils.buildSanitisedString(bs(32))).isEqualTo(" ");
        assertThat(UiUtils.buildSanitisedString(bs(126))).isEqualTo("~");
        // 31 and 127 should be escaped
        assertThat(UiUtils.buildSanitisedString(bs(31))).isEqualTo("\\x1f");
        assertThat(UiUtils.buildSanitisedString(bs(127))).isEqualTo("\\x7f");
    }

    @Test
    public void mixedPrintableAndEscaped()
    {
        assertThat(UiUtils.buildSanitisedString(bs("a\u0000b\nc\r\nd\u007fe\u0080f")))
                .isEqualTo("a\\x00b\nc\nd\\x7fe\\x80f");
        assertThat(UiUtils.buildSanitisedString(bs("Hello\u0001World\r\nNext\u007fLine\u00ff")))
                .isEqualTo("Hello\\x01World\nNext\\x7fLine\\xff");
        // Raw bytes mixed
        assertThat(UiUtils.buildSanitisedString(bs('a', 0, 'b', 10, 'c', 13, 10, 'd', 0x7f, 'e', 0x80, 'f')))
                .isEqualTo("a\\x00b\nc\nd\\x7fe\\x80f");
    }

    @Test
    public void hexFormattingIsLowerCaseZeroPadded()
    {
        assertThat(UiUtils.buildSanitisedString(bs(0x01))).isEqualTo("\\x01");
        assertThat(UiUtils.buildSanitisedString(bs("\n"))).isEqualTo("\n"); // LF is preserved, not escaped
        assertThat(UiUtils.buildSanitisedString(bs(0x0f))).isEqualTo("\\x0f");
        assertThat(UiUtils.buildSanitisedString(bs(0x10))).isEqualTo("\\x10");
        assertThat(UiUtils.buildSanitisedString(bs(0x00))).isEqualTo("\\x00");
    }

    @Test
    public void byteArrayHandlesUnsignedCorrectly()
    {
        // 255 via signed byte -1
        assertThat(UiUtils.buildSanitisedString(new byte[] {(byte) 255})).isEqualTo("\\xff");
        // Sequence of high bytes
        assertThat(UiUtils.buildSanitisedString(new byte[] {(byte) 0x80, (byte) 0x90, (byte) 0xa0, (byte) 0xff}))
                .isEqualTo("\\x80\\x90\\xa0\\xff");
        // Printable still works with signed bytes: 'A' is 65
        assertThat(UiUtils.buildSanitisedString(new byte[] {65, 66, 67})).isEqualTo("ABC");
    }
}
