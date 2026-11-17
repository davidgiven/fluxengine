package com.cowlark.fluxengine.gui;

import static com.google.common.truth.Truth.assertThat;

import com.google.common.collect.ImmutableList;
import java.util.List;
import javax.swing.SwingUtilities;
import org.fife.ui.autocomplete.AutoCompletion;
import org.fife.ui.autocomplete.Completion;
import org.fife.ui.rsyntaxtextarea.RSyntaxTextArea;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;

@RunWith(JUnit4.class)
public class PxfCompletionProviderTest
{
    @Test
    public void completesRootKeys() throws Exception
    {
        /* Top-level suggestions come directly from ConfigProto fields. */
        assertThat(names("ver|")).containsExactly("verify_writes");
        assertThat(names("|")).containsAtLeast("drive", "tracks", "option");
        assertThat(names("unknown.|")).isEmpty();
        assertThat(names("tracks.|")).isEmpty();
    }

    @Test
    public void dottedKeysDoNotTraverseMessages() throws Exception
    {
        /* PXF parses dots as part of a literal key, not as config parameter traversal. */
        assertThat(names("drive.dr|")).isEmpty();
        assertThat(names("drive.|")).isEmpty();
        assertThat(names("drive.drive_type = DRIVETYPE_|")).isEmpty();
        assertThat(names("option.config { ver| }")).isEmpty();
        assertThat(names("option.config = { ver| }")).isEmpty();
    }

    @Test
    public void completesNestedAndIncompleteBlocks() throws Exception
    {
        /* The recovered AST preserves nested schemas around missing keys and delimiters. */
        assertThat(names("option { na| }")).containsExactly("name");
        assertThat(names("option { config { ver|")).containsExactly("verify_writes");
        assertThat(names("option = { config = { ver| } }")).containsExactly("verify_writes");
        assertThat(names("option = [{ na| }]")).containsExactly("name");
        assertThat(names("drive { drive_type = DRIVETYPE_UNKNOWN }\nver|"))
                .containsExactly("verify_writes");
        assertThat(names("drive {\n\n |\n}")).contains("drive_type");
        assertThat(names("unknown { ver| }")).isEmpty();
    }

    @Test
    public void completesEnumAndBooleanValues() throws Exception
    {
        /* Values resolve through their owning field, including lists and line breaks. */
        assertThat(names("read_support_status = UNI|")).containsExactly("UNICORN");
        assertThat(names("read_support_status =\n UNI|")).containsExactly("UNICORN");
        assertThat(names("verify_writes = |")).containsExactly("false", "true");
        assertThat(names("option { applicability = [FORMAT,\n HAR|] }"))
                .containsExactly("HARDWARE_SOURCESINK");
        assertThat(names("option { applicability = [|] }"))
                .containsAtLeast("FORMAT", "HARDWARE_SOURCESINK");
        assertThat(names("tracks = |")).isEmpty();
        assertThat(names("verify_writes = true |")).isEmpty();
        assertThat(names("option { applicability = [FORMAT] | }")).isEmpty();
    }

    @Test
    public void excludesStringsCommentsAndReadOnlyEditors() throws Exception
    {
        /* Token classification must also exclude embedded Markdown and multiline comments. */
        assertThat(names("comment = \"ver|\"")).isEmpty();
        assertThat(names("comment = \"\"\"\n\nver|\n\"\"\"")).isEmpty();
        assertThat(names("comment = \"\"\"\n|\n\"\"\"")).isEmpty();
        assertThat(names("# ver|")).isEmpty();
        assertThat(names("/*\n|\n*/")).isEmpty();
        assertThat(names("comment = \"é😀\"\nver|")).containsExactly("verify_writes");
        SwingUtilities.invokeAndWait(() -> {
            RSyntaxTextArea editor = editor("ver");
            PxfParser parser = new PxfParser();
            editor.addParser(parser);
            editor.setEditable(false);
            assertThat(new PxfCompletionProvider(parser).getCompletions(editor)).isEmpty();
        });
    }

    @Test
    public void refreshesPendingParseAndSuppliesDocumentation() throws Exception
    {
        /* A completion request must see edits that have not reached the parser timer yet. */
        SwingUtilities.invokeAndWait(() -> {
            RSyntaxTextArea editor = editor("ver");
            PxfParser parser = new PxfParser();
            editor.addParser(parser);
            PxfCompletionProvider provider = new PxfCompletionProvider(parser);
            List<Completion> completions = provider.getCompletions(editor);
            assertThat(completions).hasSize(1);
            assertThat(completions.get(0).getSummary()).contains("verify writes where possible");
            editor.setText("option { na }");
            editor.setCaretPosition("option { na".length());
            assertThat(provider.getCompletions(editor).get(0).getReplacementText()).isEqualTo("name");
        });
    }

    @Test
    public void selectingCompletionReplacesTheTypedPrefix() throws Exception
    {
        /* Exercise AutoComplete's insertion path for nested fields and enum values. */
        SwingUtilities.invokeAndWait(() -> {
            for (String[] example : new String[][] {
                    {"ver", "verify_writes"},
                    {"drive { drive_ty", "drive { drive_type"},
                    {"read_support_status = UNI", "read_support_status = UNICORN"}})
            {
                RSyntaxTextArea editor = editor(example[0]);
                PxfParser parser = new PxfParser();
                editor.addParser(parser);
                PxfCompletionProvider provider = new PxfCompletionProvider(parser);
                InsertingCompletion completion = new InsertingCompletion(provider);
                completion.install(editor);
                completion.insert(provider.getCompletions(editor).get(0));
                assertThat(editor.getText()).isEqualTo(example[1]);
                completion.uninstall();
            }
        });
    }

    /* Expose the library's normal insertion action without opening a popup in headless tests. */
    private static final class InsertingCompletion extends AutoCompletion
    {
        InsertingCompletion(PxfCompletionProvider provider)
        {
            super(provider);
        }

        void insert(Completion completion)
        {
            insertCompletion(completion);
        }
    }

    private static ImmutableList<String> names(String markedText) throws Exception
    {
        /* Run real document tokenization and completion synchronously on Swing's event thread. */
        ImmutableList.Builder<String> result = ImmutableList.builder();
        SwingUtilities.invokeAndWait(() -> {
            int caret = markedText.indexOf('|');
            RSyntaxTextArea editor = editor(markedText.replace("|", ""));
            editor.setCaretPosition(caret);
            PxfParser parser = new PxfParser();
            editor.addParser(parser);
            PxfCompletionProvider provider = new PxfCompletionProvider(parser);
            assertThat(provider.getAlreadyEnteredText(editor)).doesNotContain(".");
            for (Completion completion : provider.getCompletions(editor))
                result.add(completion.getReplacementText());
        });
        return result.build();
    }

    private static RSyntaxTextArea editor(String text)
    {
        /* Register the production token maker without constructing the full format panel. */
        org.fife.ui.rsyntaxtextarea.AbstractTokenMakerFactory factory =
                (org.fife.ui.rsyntaxtextarea.AbstractTokenMakerFactory)
                        org.fife.ui.rsyntaxtextarea.TokenMakerFactory.getDefaultInstance();
        factory.putMapping(PxfTokenMaker.SYNTAX_STYLE, PxfTokenMaker.class.getName());
        RSyntaxTextArea editor = new RSyntaxTextArea();
        editor.setSyntaxEditingStyle(PxfTokenMaker.SYNTAX_STYLE);
        editor.setText(text);
        editor.setCaretPosition(text.length());
        return editor;
    }
}
