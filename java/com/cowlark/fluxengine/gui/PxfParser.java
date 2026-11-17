package com.cowlark.fluxengine.gui;

import java.nio.charset.StandardCharsets;
import org.fife.ui.rsyntaxtextarea.RSyntaxDocument;
import org.fife.ui.rsyntaxtextarea.parser.AbstractParser;
import org.fife.ui.rsyntaxtextarea.parser.DefaultParseResult;
import org.fife.ui.rsyntaxtextarea.parser.DefaultParserNotice;
import org.fife.ui.rsyntaxtextarea.parser.ParseResult;
import org.protowire.pxf.Parser;
import org.protowire.pxf.PxfException;
import javax.swing.text.BadLocationException;

/* Keep a whole-document syntax tree and diagnostics separate from incremental highlighting. */
public final class PxfParser extends AbstractParser
{
    private Parser.TolerantResult syntaxTree;
    private String parsedText;

    /* Reject stale positions while the editor's scheduled parse is still pending. */
    public String getIdentifierToolTip(String text, int offset)
    {
        if (syntaxTree == null || !text.equals(parsedText) || offset < 0 || offset >= text.length())
            return null;
        int byteOffset = text.substring(0, offset).getBytes(StandardCharsets.UTF_8).length;
        return PxfIdentifierToolTips.find(syntaxTree.document().entries(), byteOffset);
    }

    public Parser.TolerantResult getSyntaxTree()
    {
        return syntaxTree;
    }

    @Override
    public ParseResult parse(RSyntaxDocument document, String style)
    {
        /* Read the current document when the editor schedules syntax diagnostics. */
        String text;
        try
        {
            text = document.getText(0, document.getLength());
        } catch (BadLocationException e)
        {
            throw new IllegalStateException("Cannot read PXF document", e);
        }

        /* Retain the recovered AST for future completion queries. */
        byte[] bytes = text.getBytes(StandardCharsets.UTF_8);
        syntaxTree = Parser.parseTolerant(bytes);
        parsedText = text;
        DefaultParseResult result = new DefaultParseResult(this);
        result.setParsedLines(0, document.getDefaultRootElement().getElementCount() - 1);

        /* Diagnostics carry UTF-8 byte offsets. Convert to Swing's UTF-16 offsets, and mark
         * the preceding character for errors at EOF so an unfinished value remains visible. */
        for (PxfException error : syntaxTree.errors())
        {
            int byteOffset = Math.max(0, Math.min(bytes.length, error.position().offset()));
            int offset = new String(bytes, 0, byteOffset, StandardCharsets.UTF_8).length();
            if (offset == text.length() && offset > 0)
                offset = text.offsetByCodePoints(offset, -1);
            int length = offset < text.length() ? Character.charCount(text.codePointAt(offset)) : 0;
            int line = document.getDefaultRootElement().getElementIndex(offset);
            result.addNotice(new DefaultParserNotice(this, error.getMessage(), line, offset, length));
        }
        return result;
    }
}
