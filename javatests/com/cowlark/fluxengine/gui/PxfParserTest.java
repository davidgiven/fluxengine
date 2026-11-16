package com.cowlark.fluxengine.gui;

import static com.google.common.truth.Truth.assertThat;

import org.fife.ui.rsyntaxtextarea.RSyntaxDocument;
import org.fife.ui.rsyntaxtextarea.SyntaxConstants;
import org.fife.ui.rsyntaxtextarea.parser.ParseResult;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;

@RunWith(JUnit4.class)
public class PxfParserTest
{
    @Test
    public void recoversAndClearsErrorsAfterEditing() throws Exception
    {
        /* Missing values produce diagnostics while preserving subsequent valid entries. */
        RSyntaxDocument document = new RSyntaxDocument(SyntaxConstants.SYNTAX_STYLE_NONE);
        document.insertString(0, "broken =\ngood = 42\n", null);
        PxfParser parser = new PxfParser();
        ParseResult result = parser.parse(document, PxfTokenMaker.SYNTAX_STYLE);
        assertThat(result.getNotices()).isNotEmpty();
        assertThat(parser.getSyntaxTree().document()).isNotNull();

        /* Replacing invalid input updates both the diagnostics and syntax tree. */
        document.remove(0, document.getLength());
        document.insertString(0, "good = 42\n", null);
        assertThat(parser.parse(document, PxfTokenMaker.SYNTAX_STYLE).getNotices()).isEmpty();
        assertThat(parser.getSyntaxTree().document().entries()).hasSize(1);
    }

    @Test
    public void unicodeBeforeErrorUsesDocumentOffsets() throws Exception
    {
        /* A multibyte character and a surrogate pair must not shift an EOF diagnostic. */
        String text = "name = \"é😀\"\nbroken =";
        RSyntaxDocument document = new RSyntaxDocument(SyntaxConstants.SYNTAX_STYLE_NONE);
        document.insertString(0, text, null);
        ParseResult result = new PxfParser().parse(document, PxfTokenMaker.SYNTAX_STYLE);
        assertThat(result.getNotices()).isNotEmpty();
        assertThat(result.getNotices().get(0).getOffset()).isEqualTo(text.length() - 1);
        assertThat(result.getNotices().get(0).getLine()).isEqualTo(1);
    }
}
