package com.cowlark.fluxengine.gui;

import com.cowlark.fluxengine.config.Common;
import com.cowlark.fluxengine.config.ConfigProto;
import com.google.common.html.HtmlEscapers;
import com.google.protobuf.Descriptors.Descriptor;
import com.google.protobuf.Descriptors.EnumValueDescriptor;
import com.google.protobuf.Descriptors.FieldDescriptor;
import org.protowire.pxf.Ast;
import java.nio.charset.StandardCharsets;
import java.util.List;
import java.util.Locale;

/* Resolve source identifiers through the recovered AST and the enclosing protobuf message. */
final class PxfIdentifierToolTips
{
    private PxfIdentifierToolTips()
    {
    }

    static String find(List<Ast.Entry> entries, int offset)
    {
        return find(entries, ConfigProto.getDescriptor(), offset);
    }

    private static String find(List<Ast.Entry> entries, Descriptor message, int offset)
    {
        /* Only AST keys and enum values are eligible, excluding comments and string contents.
         * Descend through recovered blocks even when their closing delimiter is missing. */
        for (Ast.Entry entry : entries)
        {
            if (entry instanceof Ast.Block block)
            {
                String tooltip = key(
                        block.name(),
                        block.nameQuoted(),
                        block.pos().offset(),
                        message,
                        offset);
                if (tooltip != null)
                    return tooltip;
                FieldDescriptor field = message.findFieldByName(block.name());
                if (field != null && field.getJavaType() == FieldDescriptor.JavaType.MESSAGE)
                {
                    tooltip = find(block.entries(), field.getMessageType(), offset);
                    if (tooltip != null)
                        return tooltip;
                }
            } else if (entry instanceof Ast.Assignment assignment)
            {
                String tooltip = key(
                        assignment.key(),
                        assignment.keyQuoted(),
                        assignment.pos().offset(),
                        message,
                        offset);
                if (tooltip != null)
                    return tooltip;
                FieldDescriptor field = message.findFieldByName(assignment.key());
                if (field != null)
                {
                    tooltip = value(assignment.value(), field, offset);
                    if (tooltip != null)
                        return tooltip;
                }
            }
        }
        return null;
    }

    private static String key(String key, boolean quoted, int start, Descriptor message, int offset)
    {
        /* Match the complete literal key against the enclosing message's schema. */
        if (quoted)
            return null;
        FieldDescriptor field = message.findFieldByName(key);
        int end = start + key.getBytes(StandardCharsets.UTF_8).length;
        return field != null && offset >= start && offset < end ? describe(field) : null;
    }

    private static String value(Ast.Value value, FieldDescriptor field, int offset)
    {
        /* Assignment blocks and lists carry the same schema context as ordinary blocks. */
        if (value instanceof Ast.BlockVal block &&
                field.getJavaType() == FieldDescriptor.JavaType.MESSAGE)
            return find(block.entries(), field.getMessageType(), offset);
        if (value instanceof Ast.ListVal list)
        {
            for (Ast.Value element : list.elements())
            {
                String tooltip = value(element, field, offset);
                if (tooltip != null)
                    return tooltip;
            }
        }
        if (value instanceof Ast.IdentVal identifier &&
                field.getJavaType() == FieldDescriptor.JavaType.ENUM &&
                offset >= identifier.pos().offset() && offset < identifier.pos().offset() +
                identifier.name().getBytes(StandardCharsets.UTF_8).length)
        {
            EnumValueDescriptor enumValue = field.getEnumType().findValueByName(identifier.name());
            if (enumValue != null)
                return "<html><b>" + escape(enumValue.getFullName()) + "</b><br>Value: " +
                        enumValue.getNumber() + "</html>";
        }
        return null;
    }

    static String describe(FieldDescriptor field)
    {
        /* Escape all schema text before presenting it as Swing HTML. */
        String type = switch (field.getJavaType())
        {
            case MESSAGE -> field.getMessageType().getFullName();
            case ENUM -> field.getEnumType().getFullName();
            default -> field.getType().name().toLowerCase(Locale.ROOT);
        };
        StringBuilder html = new StringBuilder("<html><b>");
        html.append(escape(field.getFullName())).append("</b><br>");
        if (field.isRepeated())
            html.append("list of ");
        html.append(escape(type));
        String help = field.getOptions().getExtension(Common.help);
        if (!help.isEmpty())
            html.append("<br>").append(escape(help));
        if (field.hasDefaultValue())
            html.append("<br>Default: ").append(escape(String.valueOf(field.getDefaultValue())));
        if (field.getJavaType() == FieldDescriptor.JavaType.ENUM)
        {
            html.append("<br>Choices: ");
            for (EnumValueDescriptor choice : field.getEnumType().getValues())
                html.append(escape(choice.getName())).append(" ");
        }
        return html.append("</html>").toString();
    }

    private static String escape(String text)
    {
        return HtmlEscapers.htmlEscaper().escape(text);
    }
}
