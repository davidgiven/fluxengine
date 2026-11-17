package org.protowire.pxf;

import java.nio.charset.StandardCharsets;

/* Bridge to the package-private lexer in the pinned ProtoWire runtime. Keep private API
 * access here so the editor does not duplicate PXF lexical rules or use reflection. */
public final class LexerAccess
{
    public record Lexeme(Token token, int start, int end, String error) {}

    private final Lexer lexer;
    private final int[] characterOffsets;
    private String error;

    public LexerAccess(String source)
    {
        /* ProtoWire uses UTF-8 byte offsets; Swing uses UTF-16 document offsets. */
        byte[] bytes = source.getBytes(StandardCharsets.UTF_8);
        characterOffsets = new int[bytes.length + 1];
        int byteOffset = 0;
        for (int character = 0; character < source.length();)
        {
            int codepoint = source.codePointAt(character);
            int byteCount = codepoint <= 0x7f ? 1 : codepoint <= 0x7ff ? 2 :
                    codepoint <= 0xffff ? 3 : 4;
            /* An isolated surrogate is encoded as a one-byte replacement by Java. */
            if (Character.isSurrogate(source.charAt(character)) &&
                    Character.charCount(codepoint) == 1)
                byteCount = 1;
            for (int i = 0; i < byteCount; i++)
                characterOffsets[byteOffset++] = character;
            character += Character.charCount(codepoint);
        }
        characterOffsets[bytes.length] = source.length();
        lexer = new Lexer(bytes);
        lexer.tolerant = true;
        lexer.onErr = (position, message) -> error = message;
    }

    public Lexeme next()
    {
        /* Include raw delimiters and consumed malformed text, not just decoded token values. */
        error = null;
        Token token = lexer.next();
        return new Lexeme(token, characterOffsets[token.pos().offset()],
                characterOffsets[lexer.pos], error);
    }
}
