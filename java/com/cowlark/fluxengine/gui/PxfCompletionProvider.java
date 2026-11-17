package com.cowlark.fluxengine.gui;

import com.cowlark.fluxengine.config.ConfigProto;
import com.google.protobuf.Descriptors.Descriptor;
import com.google.protobuf.Descriptors.EnumValueDescriptor;
import com.google.protobuf.Descriptors.FieldDescriptor;
import org.fife.ui.autocomplete.BasicCompletion;
import org.fife.ui.autocomplete.Completion;
import org.fife.ui.autocomplete.DefaultCompletionProvider;
import org.fife.ui.rsyntaxtextarea.RSyntaxTextArea;
import org.fife.ui.rsyntaxtextarea.Token;
import org.protowire.pxf.Ast;
import javax.swing.text.JTextComponent;
import java.nio.charset.StandardCharsets;
import java.util.ArrayList;
import java.util.List;

/* Combine recovered message scopes with existing highlighting tokens around unfinished input. */
public final class PxfCompletionProvider extends DefaultCompletionProvider
{
    private final PxfParser parser;

    public PxfCompletionProvider(PxfParser parser)
    {
        this.parser = parser;
        setAutoActivationRules(true, null);
    }

    private static boolean valuePosition(List<String> statement, String prefix)
    {
        /* Scalar values replace only the current prefix. For lists, previous values must
         * be separated by commas; a closed list is no longer a completion position. */
        if (statement.size() == 2)
            return true;
        if (!statement.get(2).equals("["))
            return statement.size() == 3 && !prefix.isEmpty() && statement.get(2).equals(prefix);
        boolean expectValue = true;
        for (int index = 3; index < statement.size(); index++)
        {
            String token = statement.get(index);
            if (expectValue)
            {
                if (!token.matches("[A-Za-z_][A-Za-z_0-9]*"))
                    return false;
                if (index == statement.size() - 1)
                    return !prefix.isEmpty() && token.equals(prefix);
            } else if (!token.equals(","))
                return false;
            expectValue = !expectValue;
        }
        return expectValue;
    }

    private static List<String> statement(RSyntaxTextArea editor, int start, int caret)
    {
        /* Highlighting already distinguishes embedded Markdown, strings and comments. Keep
         * only significant tokens in the current statement, allowing multiline values. */
        List<String> tokens = new ArrayList<>();
        int brackets = 0;
        int firstLine = editor.getDocument().getDefaultRootElement().getElementIndex(start);
        int lastLine = editor.getDocument().getDefaultRootElement().getElementIndex(caret);
        for (int line = firstLine; line <= lastLine; line++)
        {
            if (line > firstLine && brackets == 0 && (tokens.isEmpty() ||
                    !(tokens.get(tokens.size() - 1).equals("=") ||
                            tokens.get(tokens.size() - 1).equals(":"))))
                tokens.clear();
            Token token = editor.getTokenListForLine(line);
            while (token != null && token.isPaintable())
            {
                int tokenStart = token.getOffset();
                int tokenEnd = tokenStart + token.length();
                if (tokenStart == caret && token.length() == 0 && token.isComment())
                    return null;
                if (tokenStart >= caret)
                    break;
                if (tokenEnd > start)
                {
                    if (token.isComment() || token.getLanguageIndex() != 0 ||
                            token.getType() == Token.LITERAL_STRING_DOUBLE_QUOTE ||
                            token.getType() == Token.ERROR_STRING_DOUBLE)
                    {
                        if (caret <= tokenEnd)
                            return null;
                        if (!token.isComment())
                            tokens.add("\"\"");
                    } else if (!token.isWhitespace())
                    {
                        String raw =
                                token.getLexeme().substring(
                                        Math.max(start, tokenStart) - tokenStart,
                                        Math.min(caret, tokenEnd) - tokenStart);
                        if (raw.equals("["))
                            brackets++;
                        if (raw.equals("]"))
                            brackets--;
                        if (raw.equals("{") || raw.equals("}") ||
                                (raw.equals(",") && brackets == 0))
                            tokens.clear();
                        else
                            tokens.add(raw);
                    }
                }
                token = token.getNextToken();
            }
            /* Continuation tokens also preserve exclusions on empty lines in multiline text. */
            if (line == lastLine && token != null && !token.isPaintable() &&
                    (token.getType() < 0 || token.getType() == Token.COMMENT_MULTILINE))
                return null;
        }
        return tokens;
    }

    private static Scope scope(List<Ast.Entry> entries, Scope parent, int offset, byte[] bytes)
    {
        /* AST positions identify enclosing message bodies even when the current key itself
         * was dropped by error recovery. Unknown blocks deliberately suppress suggestions. */
        for (Ast.Entry entry : entries)
        {
            if (entry instanceof Ast.Block block &&
                    contains(block.bracePos().offset(), block.end().offset(), offset, bytes))
            {
                FieldDescriptor field =
                        parent.message().findFieldByName(block.name());
                Scope child = child(field, block.bracePos().offset() + 1, bytes);
                return child.message() == null ?
                        child :
                        scope(block.entries(), child, offset, bytes);
            }
            if (entry instanceof Ast.Assignment assignment)
            {
                FieldDescriptor field =
                        parent.message().findFieldByName(assignment.key());
                Scope child = valueScope(assignment.value(), field, offset, bytes);
                if (child != null)
                    return child;
            }
        }
        return parent;
    }

    private static Scope valueScope(
            Ast.Value value,
            FieldDescriptor field,
            int offset,
            byte[] bytes)
    {
        if (value instanceof Ast.BlockVal block &&
                contains(block.pos().offset(), block.end().offset(), offset, bytes))
        {
            Scope child = child(field, block.pos().offset() + 1, bytes);
            return child.message() == null ? child : scope(block.entries(), child, offset, bytes);
        }
        if (value instanceof Ast.ListVal list)
            for (Ast.Value element : list.elements())
            {
                Scope child = valueScope(element, field, offset, bytes);
                if (child != null)
                    return child;
            }
        return null;
    }

    private static Scope child(FieldDescriptor field, int byteStart, byte[] bytes)
    {
        int start = new String(bytes, 0, byteStart, StandardCharsets.UTF_8).length();
        Descriptor message =
                field != null && field.getJavaType() == FieldDescriptor.JavaType.MESSAGE ?
                        field.getMessageType() :
                        null;
        return new Scope(message, start);
    }

    private static boolean contains(int start, int end, int offset, byte[] bytes)
    {
        boolean closed = end > 0 && end <= bytes.length && bytes[end - 1] == '}';
        return offset > start && (closed ? offset < end : offset <= end);
    }

    @Override
    protected List<Completion> getCompletionsImpl(JTextComponent component)
    {
        /* Refresh only when a request beats the scheduled parse. Reuse the editor's token
         * lists rather than running another lexer for completion. */
        if (!(component instanceof RSyntaxTextArea editor) || !editor.isEditable())
            return new ArrayList<>();
        String text = editor.getText();
        if (!parser.isCurrent(text))
            editor.forceReparsing(parser);
        if (!parser.isCurrent(text))
            return new ArrayList<>();
        int caret = editor.getCaretPosition();
        byte[] bytes = text.getBytes(StandardCharsets.UTF_8);
        int byteOffset = text.substring(0, caret).getBytes(StandardCharsets.UTF_8).length;
        Scope scope = scope(
                parser.getSyntaxTree().document().entries(),
                new Scope(ConfigProto.getDescriptor(), 0),
                byteOffset,
                bytes);
        if (scope.message() == null)
            return new ArrayList<>();
        List<String> statement = statement(editor, scope.start(), caret);
        if (statement == null)
            return new ArrayList<>();
        String prefix = getAlreadyEnteredText(editor);
        List<Completion> completions = new ArrayList<>();

        /* PXF keys name fields directly within the enclosing message. A dot is part of a
         * literal key, so it cannot select fields from a nested message. */
        if (statement.isEmpty() || statement.size() == 1)
        {
            String key = statement.isEmpty() ? "" : statement.get(0);
            if (!key.matches("[A-Za-z_][A-Za-z_0-9]*|") ||
                    (!key.isEmpty() && prefix.isEmpty()))
                return new ArrayList<>();
            for (FieldDescriptor field : scope.message().getFields())
                add(completions, field.getName(), prefix, PxfIdentifierToolTips.describe(field));
        } else if (statement.size() >= 2 &&
                (statement.get(1).equals("=") || statement.get(1).equals(":")))
        {
            /* Only enum and boolean values have schema-defined literal candidates. A list
             * retains its owning field across commas and line breaks. */
            FieldDescriptor field =
                    scope.message().findFieldByName(statement.get(0));
            if (field == null || !valuePosition(statement, prefix))
                return new ArrayList<>();
            String summary = PxfIdentifierToolTips.describe(field);
            if (field.getJavaType() == FieldDescriptor.JavaType.ENUM)
                for (EnumValueDescriptor choice : field.getEnumType().getValues())
                    add(completions, choice.getName(), prefix, summary);
            else if (field.getJavaType() == FieldDescriptor.JavaType.BOOLEAN)
            {
                add(completions, "false", prefix, summary);
                add(completions, "true", prefix, summary);
            }
        }
        /* AutoComplete sorts this list in place, so its API requires a mutable result. */
        return completions;
    }

    private void add(List<Completion> completions, String name, String prefix, String summary)
    {
        if (name.startsWith(prefix))
            completions.add(new BasicCompletion(this, name, null, summary));
    }

    private record Scope(Descriptor message, int start)
    {
    }
}
