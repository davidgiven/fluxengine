package com.cowlark.fluxengine.gui;

import org.fife.ui.rsyntaxtextarea.AbstractTokenMaker;
import org.fife.ui.rsyntaxtextarea.Token;
import org.fife.ui.rsyntaxtextarea.TokenMap;
import org.fife.ui.rsyntaxtextarea.TokenTypes;
import org.fife.ui.rsyntaxtextarea.modes.MarkdownTokenMaker;
import org.protowire.pxf.LexerAccess;
import org.protowire.pxf.LexerAccess.Lexeme;
import org.protowire.pxf.TokenKind;
import javax.swing.text.Segment;
import java.util.HashMap;
import java.util.Map;

/* Highlight PXF source, delegating triple-quoted documentation to the Markdown lexer. */
public class PxfTokenMaker extends AbstractTokenMaker implements TokenTypes
{
    public static final String SYNTAX_STYLE = "text/pxf";
    private final MarkdownTokenMaker markdown = new MarkdownTokenMaker();
    private final Map<MarkdownState, Integer> stateTypes = new HashMap<>();
    private final Map<Integer, MarkdownState> states = new HashMap<>();

    private static int indentation(Segment line)
    {
        int count = 0;
        while (count < line.count &&
                (line.array[line.offset + count] == ' ' || line.array[line.offset + count] == '\t'))
            count++;
        return count;
    }

    private static int tokenType(TokenKind kind, TokenKind next, String raw, String error)
    {
        /* Map library token kinds to theme styles without reinterpreting PXF grammar. */
        return switch (kind)
        {
            case COMMENT -> raw.startsWith("/*") ? COMMENT_MULTILINE : COMMENT_EOL;
            case STRING, BYTES -> error == null ? LITERAL_STRING_DOUBLE_QUOTE : ERROR_STRING_DOUBLE;
            case INT -> LITERAL_NUMBER_DECIMAL_INT;
            case FLOAT -> LITERAL_NUMBER_FLOAT;
            case BOOL -> LITERAL_BOOLEAN;
            case NULL -> RESERVED_WORD;
            case TIMESTAMP, DURATION -> LITERAL_NUMBER_FLOAT;
            case IDENT -> next == TokenKind.EQUALS || next == TokenKind.COLON ||
                    next == TokenKind.LBRACE ? VARIABLE : IDENTIFIER;
            case LBRACE, RBRACE, LBRACKET, RBRACKET, LPAREN, RPAREN, COMMA -> SEPARATOR;
            case EQUALS, COLON -> OPERATOR;
            case AT_TYPE, AT_DATASET, AT_PROTO, AT_DIRECTIVE -> PREPROCESSOR;
            case NEWLINE, EOF -> WHITESPACE;
            case ILLEGAL -> raw.startsWith("\"") || raw.startsWith("b\"") ?
                    ERROR_STRING_DOUBLE :
                    ERROR_IDENTIFIER;
        };
    }

    @Override
    public TokenMap getWordsToHighlight()
    {
        return new TokenMap();
    }

    @Override
    public int getClosestStandardTokenTypeForInternalType(int type)
    {
        return type < 0 ? LITERAL_STRING_DOUBLE_QUOTE : type;
    }

    @Override
    public String[] getLineCommentStartAndEnd(int languageIndex)
    {
        return new String[]{"#", null};
    }

    @Override
    public Token getTokenList(Segment line, int initialType, int startOffset)
    {
        /* Reintroduce the opening delimiter for a continued multiline token. ProtoWire then
         * determines where it ends and resumes normal PXF tokenization on the same line. */
        resetTokenList();
        setLanguageIndex(0);
        MarkdownState embedded = states.get(initialType);
        String prefix = embedded != null ? "\"\"\"" : initialType == COMMENT_MULTILINE ? "/*" : "";
        String source = prefix + line.toString();
        LexerAccess stream = new LexerAccess(source);
        int consumed = 0;
        int offset = startOffset - line.offset;
        int finalType = NULL;
        Lexeme lexeme = stream.next();
        while (lexeme.token().kind() != TokenKind.EOF)
        {
            Lexeme next = stream.next();
            int start = Math.max(0, lexeme.start() - prefix.length());
            int end = Math.max(0, lexeme.end() - prefix.length());
            String raw = source.substring(lexeme.start(), lexeme.end());
            TokenKind kind = lexeme.token().kind();
            addSpan(line, consumed, start, WHITESPACE, offset);

            /* Triple-string boundaries come from ProtoWire's token spans. Only the interior
             * is delegated to Markdown, preserving its continuation state and indentation. */
            if (raw.startsWith("\"\"\"") && (kind == TokenKind.STRING || kind == TokenKind.ILLEGAL))
            {
                boolean continued = embedded != null && lexeme.start() == 0;
                boolean closed = lexeme.error() == null;
                int indentation = continued ? embedded.indentation() : indentation(line);
                int contentStart = continued ? start : start + 3;
                int contentEnd = closed ? end - 3 : end;
                addSpan(line, start, contentStart, LITERAL_STRING_DOUBLE_QUOTE, offset);
                int markdownStart = contentStart;
                if (continued)
                    while (markdownStart < contentEnd &&
                            markdownStart - contentStart < indentation &&
                            (line.array[line.offset + markdownStart] == ' ' ||
                                    line.array[line.offset + markdownStart] == '\t'))
                        markdownStart++;
                addSpan(line, contentStart, markdownStart, WHITESPACE, offset);
                int markdownType = addMarkdown(
                        line.array,
                        line.offset + markdownStart,
                        line.offset + contentEnd,
                        offset,
                        continued ? embedded.tokenType() : NULL);
                if (closed)
                {
                    addSpan(line, contentEnd, end, LITERAL_STRING_DOUBLE_QUOTE, offset);
                    embedded = null;
                } else
                    embedded = new MarkdownState(markdownType, indentation);
            } else
            {
                int type = tokenType(kind, next.token().kind(), raw, lexeme.error());
                addSpan(line, start, end, type, offset);
                if (kind == TokenKind.COMMENT && raw.startsWith("/*") && lexeme.error() != null)
                    finalType = COMMENT_MULTILINE;
            }
            consumed = end;
            lexeme = next;
        }
        addSpan(line, consumed, line.count, WHITESPACE, offset);

        /* End tokens retain multiline state, including through empty lines. */
        if (embedded != null)
            finishMarkdown(embedded);
        else
        {
            addNullToken();
            currentToken.setType(finalType);
        }
        return firstToken;
    }

    private void addSpan(Segment line, int start, int end, int type, int offset)
    {
        if (end > start)
            addToken(
                    line.array,
                    line.offset + start,
                    line.offset + end - 1,
                    type,
                    line.offset + start + offset);
    }

    private int addMarkdown(char[] chars, int start, int end, int offset, int initialType)
    {
        /* Copy tokens because the embedded lexer reuses its own token pool. */
        Token token = markdown.getTokenList(
                new Segment(chars, start, end - start),
                initialType,
                start + offset);
        int lastType = NULL;
        setLanguageIndex(1);
        while (token != null)
        {
            lastType = token.getType();
            if (token.isPaintable())
                addToken(
                        token.getTextArray(),
                        token.getTextOffset(),
                        token.getTextOffset() + token.length() - 1,
                        token.getType(),
                        token.getOffset());
            token = token.getNextToken();
        }
        setLanguageIndex(0);
        return lastType;
    }

    private void finishMarkdown(MarkdownState state)
    {
        int type = stateTypes.computeIfAbsent(
                state, key -> {
                    int value = -states.size() - 1;
                    states.put(value, key);
                    return value;
                });
        addNullToken();
        currentToken.setType(type);
    }

    /* Internal negative token types retain both embedded lexer state and source indentation
     * between lines. Interning states keeps repeated lexing deterministic. */
    private record MarkdownState(int tokenType, int indentation)
    {
    }

}
