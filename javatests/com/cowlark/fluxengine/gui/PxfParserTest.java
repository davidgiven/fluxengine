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

    @Test
    public void identifierTooltipsUseNestedSchemaAndDocumentation() throws Exception
    {
        /* The same field name resolves differently at the root and inside an option. */
        String text = "comment = \"root\"\noption { comment = \"child\" }\n";
        PxfParser parser = parse(text);
        assertThat(parser.getIdentifierToolTip(text, 1)).contains("ConfigProto.comment");
        String nested = parser.getIdentifierToolTip(text, text.indexOf("comment", 10));
        assertThat(nested).contains("OptionProto.comment");
        assertThat(nested).contains("help text for option");
        assertThat(nested).contains("<br>string<br>");
    }

    @Test
    public void identifierTooltipsHandleNestedFieldsAndEnums() throws Exception
    {
        /* Nested block keys and enum values resolve through their enclosing message. */
        String text = "drive { drive_type = DRIVETYPE_UNKNOWN }\n";
        PxfParser parser = parse(text);
        assertThat(parser.getIdentifierToolTip(text, 0)).contains("ConfigProto.drive");
        String field = parser.getIdentifierToolTip(text, text.indexOf("drive_type"));
        assertThat(field).contains("DriveProto.drive_type");
        assertThat(field).contains("Type of drive");
        assertThat(field).contains("Default: DRIVETYPE_UNKNOWN");
        assertThat(field).contains("Choices:");
        assertThat(parser.getIdentifierToolTip(text, text.indexOf("DRIVETYPE_UNKNOWN")))
                .contains("Value: 0");
    }

    @Test
    public void dottedKeysDoNotResolveAsNestedFields() throws Exception
    {
        /* Neither keys nor nested blocks may treat literal dots as schema traversal. */
        String text = "drive.drive_type = DRIVETYPE_UNKNOWN\noption.config { verify_writes = true }";
        PxfParser parser = parse(text);
        assertThat(parser.getIdentifierToolTip(text, 0)).isNull();
        assertThat(parser.getIdentifierToolTip(text, text.indexOf("drive_type"))).isNull();
        assertThat(parser.getIdentifierToolTip(text, text.indexOf("DRIVETYPE_UNKNOWN"))).isNull();
        assertThat(parser.getIdentifierToolTip(text, text.indexOf("verify_writes"))).isNull();
    }

    @Test
    public void identifierTooltipsExcludeNonIdentifiersAndStaleText() throws Exception
    {
        /* Strings, comments, punctuation and unresolved fields must not inherit nearby help. */
        String text = "comment = \"drive\" # tracks\nunknown { tracks = \"x\" }\n";
        PxfParser parser = parse(text);
        assertThat(parser.getIdentifierToolTip(text, text.indexOf("drive"))).isNull();
        assertThat(parser.getIdentifierToolTip(text, text.indexOf("tracks"))).isNull();
        assertThat(parser.getIdentifierToolTip(text, text.lastIndexOf("tracks"))).isNull();
        assertThat(parser.getIdentifierToolTip(text, text.indexOf('='))).isNull();
        assertThat(parser.getIdentifierToolTip(text + " ", 0)).isNull();
        assertThat(parser.getIdentifierToolTip(text, -1)).isNull();
        assertThat(parser.getIdentifierToolTip(text, text.length())).isNull();
    }

    @Test
    public void identifierTooltipsHandleUnicodeAndIncompleteAssignmentBlocks() throws Exception
    {
        /* Recovery retains the message context even with a missing closing brace. */
        String text = "comment = \"é😀\"\noption = { config = { verify_writes = true\n";
        PxfParser parser = parse(text);
        String tooltip = parser.getIdentifierToolTip(text, text.indexOf("verify_writes"));
        assertThat(tooltip).contains("ConfigProto.verify_writes");
        assertThat(tooltip).contains("verify writes where possible");
    }

    private static PxfParser parse(String text) throws Exception
    {
        /* Exercise the actual tolerant parser rather than constructing synthetic AST nodes. */
        RSyntaxDocument document = new RSyntaxDocument(SyntaxConstants.SYNTAX_STYLE_NONE);
        document.insertString(0, text, null);
        PxfParser parser = new PxfParser();
        parser.parse(document, PxfTokenMaker.SYNTAX_STYLE);
        return parser;
    }

}
