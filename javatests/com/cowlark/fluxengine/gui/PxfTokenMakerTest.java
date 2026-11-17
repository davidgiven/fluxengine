package com.cowlark.fluxengine.gui;

import static com.google.common.truth.Truth.assertThat;

import org.fife.ui.rsyntaxtextarea.Token;
import org.fife.ui.rsyntaxtextarea.TokenTypes;
import org.fife.ui.rsyntaxtextarea.modes.MarkdownTokenMaker;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;
import javax.swing.text.Segment;

@RunWith(JUnit4.class)
public class PxfTokenMakerTest
{
    private final PxfTokenMaker lexer = new PxfTokenMaker();

    private static Segment segment(String text)
    {
        return new Segment(text.toCharArray(), 0, text.length());
    }

    private static Token find(Token tokens, String lexeme)
    {
        for (Token token = tokens; token != null; token = token.getNextToken())
            if (token.isPaintable() && token.getLexeme().equals(lexeme))
                return token;
        throw new AssertionError("Missing token: " + lexeme);
    }

    private Token tokens(String text, int state)
    {
        return lexer.getTokenList(segment(text), state, 100);
    }

    /* PXF tokens distinguish source structure from comments, strings, and numeric scalars. */
    @Test
    public void highlightsScalarsAndComments()
    {
        Token tokens = tokens("count = 42 ratio = 1.5 enabled = true data = b\"abc\" # comment", 0);
        assertThat(find(tokens, "count").getType()).isEqualTo(TokenTypes.VARIABLE);
        assertThat(find(tokens, "42").getType()).isEqualTo(TokenTypes.LITERAL_NUMBER_DECIMAL_INT);
        assertThat(find(tokens, "1.5").getType()).isEqualTo(TokenTypes.LITERAL_NUMBER_FLOAT);
        assertThat(find(tokens, "true").getType()).isEqualTo(TokenTypes.LITERAL_BOOLEAN);
        assertThat(find(
                tokens,
                "b\"abc\"").getType()).isEqualTo(TokenTypes.LITERAL_STRING_DOUBLE_QUOTE);
        assertThat(find(tokens, "# comment").getType()).isEqualTo(TokenTypes.COMMENT_EOL);
        assertThat(find(tokens, "count").getOffset()).isEqualTo(100);
    }

    /* Embedded Markdown uses the same tokens as the bundled Markdown lexer after removing
     * PXF indentation, and switches back to PXF immediately after the closing delimiter. */
    @Test
    public void highlightsIndentedMarkdownAndResumesPxf()
    {
        int state = lexer.getLastTokenTypeOnLine(segment("    documentation = \"\"\""), 0);
        assertThat(state).isLessThan(0);
        Token tokens = tokens("    # Heading", state);
        Token heading = new MarkdownTokenMaker().getTokenList(segment("# Heading"), 0, 104);
        assertThat(find(tokens, heading.getLexeme()).getType()).isEqualTo(heading.getType());
        assertThat(find(tokens, heading.getLexeme()).getLanguageIndex()).isEqualTo(1);
        assertThat(find(tokens, heading.getLexeme()).getOffset()).isEqualTo(104);

        tokens = tokens("    \"\"\" enabled = false", state);
        assertThat(find(tokens, "enabled").getType()).isEqualTo(TokenTypes.VARIABLE);
        assertThat(find(tokens, "false").getLanguageIndex()).isEqualTo(0);
        assertThat(lexer.getLastTokenTypeOnLine(segment("\"\"\""), state)).isEqualTo(0);
    }

    /* Fenced code state survives blank lines and is reset when the PXF string closes. */
    @Test
    public void preservesMarkdownStateAcrossLines()
    {
        int state = lexer.getLastTokenTypeOnLine(segment("\"\"\""), 0);
        state = lexer.getLastTokenTypeOnLine(segment("```"), state);
        state = lexer.getLastTokenTypeOnLine(segment(""), state);
        Token code = tokens("value = true", state);
        assertThat(code.getLanguageIndex()).isEqualTo(1);
        state = lexer.getLastTokenTypeOnLine(segment("```"), state);
        assertThat(lexer.getLastTokenTypeOnLine(segment("\"\"\""), state)).isEqualTo(0);
    }

    /* Partial drafts and multiline comments must not corrupt subsequent lexical context. */
    @Test
    public void toleratesIncompleteStringsAndBlockComments()
    {
        assertThat(find(tokens("name = \"unfinished", 0), "\"unfinished").getType()).isEqualTo(
                TokenTypes.ERROR_STRING_DOUBLE);
        int state = lexer.getLastTokenTypeOnLine(segment("/* \"\"\""), 0);
        assertThat(state).isEqualTo(TokenTypes.COMMENT_MULTILINE);
        state = lexer.getLastTokenTypeOnLine(segment(""), state);
        Token tokens = tokens("end */ value = 255", state);
        assertThat(find(tokens, "end */").getType()).isEqualTo(TokenTypes.COMMENT_MULTILINE);
        assertThat(find(tokens, "255").getType()).isEqualTo(TokenTypes.LITERAL_NUMBER_DECIMAL_INT);
    }

    /* ProtoWire byte spans must map back to Swing UTF-16 offsets, including surrogate pairs. */
    @Test
    public void preservesUnicodeSourceOffsets()
    {
        String source = "name = \"ä😀\" enabled = true // comment";
        Token tokens = tokens(source, 0);
        assertThat(find(tokens, "enabled").getOffset()).isEqualTo(100 + source.indexOf("enabled"));
        assertThat(find(tokens, "// comment").getType()).isEqualTo(TokenTypes.COMMENT_EOL);
        assertThat(find(tokens, "\"ä😀\"").getLexeme()).isEqualTo("\"ä😀\"");
    }

    /* Do not invent language constructs that the authoritative lexer does not recognise. */
    @Test
    public void followsProtoWireNumericTokenBoundaries()
    {
        Token tokens = tokens("value = 0xFF", 0);
        assertThat(find(tokens, "0").getType()).isEqualTo(TokenTypes.LITERAL_NUMBER_DECIMAL_INT);
        assertThat(find(tokens, "xFF").getType()).isEqualTo(TokenTypes.IDENTIFIER);
    }
}
