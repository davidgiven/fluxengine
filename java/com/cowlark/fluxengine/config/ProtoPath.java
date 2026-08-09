package com.cowlark.fluxengine.config;

import com.google.protobuf.Descriptors.EnumValueDescriptor;
import com.google.protobuf.Descriptors.FieldDescriptor;
import com.google.protobuf.Message;
import java.util.ArrayList;
import java.util.List;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

/**
 * Resolves dotted paths (e.g. "drive.drive_type" or "option[0].comment")
 * against a protobuf builder and sets the leaf value, ported from
 * lib/config/proto.cc's makeProtoPath/ProtoField.
 */
public class ProtoPath
{
    private static final Pattern PATH_COMPONENT = Pattern.compile("^(\\w+)(?:\\[(\\d+)\\])?$");

    private ProtoPath()
    {
    }

    public static void set(Message.Builder builder, String path, String value)
    {
        List<PathComponent> components = parsePath(path);
        setRecursive(builder, components, 0, value, path);
    }

    /* Resolves a dotted path against a message and returns the leaf value as
     * a string, ported from lib/config/proto.cc's findProtoPath/get. */
    public static String get(Message.Builder builder, String path)
    {
        List<PathComponent> components = parsePath(path);
        return getRecursive(builder, components, 0, path);
    }

    private static String getRecursive(Message.Builder builder,
                                       List<PathComponent> path,
                                       int pos,
                                       String originalPath)
    {
        PathComponent component = path.get(pos);
        FieldDescriptor field = findField(builder, component, originalPath);

        if (pos == path.size() - 1)
        {
            return getLeaf(builder, component, field);
        }

        if (field.getJavaType() != FieldDescriptor.JavaType.MESSAGE)
            throw new ProtoPathNotFoundException(
                    "config field '" + component.name() + "' in '" + originalPath +
                            "' is not a message");

        Message.Builder elementBuilder;
        if (field.isRepeated())
        {
            int index = requireIndex(component, field);
            if (builder.getRepeatedFieldCount(field) <= index)
                throw new ProtoPathNotFoundException(
                        "could not find config field '" + field.getName() + "'");
            Message element = (Message) builder.getRepeatedField(field, index);
            elementBuilder = element.toBuilder();
        } else
        {
            if (component.index() >= 0)
                throw new ProtoPathNotFoundException("config field '" + component.name() +
                        "' is not repeated but an index is provided");
            if (!builder.hasField(field))
                throw new ProtoPathNotFoundException(
                        "could not find config field '" + field.getName() + "'");
            elementBuilder = ((Message) builder.getField(field)).toBuilder();
        }
        return getRecursive(elementBuilder, path, pos + 1, originalPath);
    }

    private static String getLeaf(Message.Builder builder,
                                  PathComponent component,
                                  FieldDescriptor field)
    {
        if (field.getJavaType() == FieldDescriptor.JavaType.MESSAGE)
            throw new ConfigException("config field '" + component.name() +
                    "' is a message and can't be directly fetched");

        Object value;
        if (field.isRepeated())
        {
            int index = requireIndex(component, field);
            if (builder.getRepeatedFieldCount(field) <= index)
                throw new ProtoPathNotFoundException(
                        "could not find config field '" + field.getName() + "'");
            value = builder.getRepeatedField(field, index);
        } else
        {
            if (component.index() >= 0)
                throw new ProtoPathNotFoundException("config field '" + component.name() +
                        "' is not repeated but an index is provided");
            value = builder.getField(field);
        }
        return formatValue(field, value);
    }

    private static String formatValue(FieldDescriptor field, Object value)
    {
        switch (field.getType())
        {
            case FLOAT:
            case DOUBLE:
                return String.valueOf(value);
            case BOOL:
                return String.valueOf(value);
            case ENUM:
                return ((EnumValueDescriptor) value).getName();
            default:
                return String.valueOf(value);
        }
    }

    private static List<PathComponent> parsePath(String path)
    {
        List<PathComponent> components = new ArrayList<>();
        for (String token : path.split("\\.", -1))
        {
            Matcher matcher = PATH_COMPONENT.matcher(token);
            if (!matcher.matches())
                throw new ConfigException("invalid config path '" + path + "'");
            String index = matcher.group(2);
            components.add(new PathComponent(
                    matcher.group(1),
                    index == null ? -1 : Integer.parseInt(index)));
        }
        return components;
    }

    private static void setRecursive(Message.Builder builder,
                                     List<PathComponent> path,
                                     int pos,
                                     String value,
                                     String originalPath)
    {
        PathComponent component = path.get(pos);
        FieldDescriptor field = findField(builder, component, originalPath);

        if (pos == path.size() - 1)
        {
            setLeaf(builder, component, field, value);
            return;
        }

        if (field.getJavaType() != FieldDescriptor.JavaType.MESSAGE)
            throw new ProtoPathNotFoundException(
                    "config field '" + component.name() + "' in '" + originalPath +
                            "' is not a message");

        if (field.isRepeated())
        {
            int index = requireIndex(component, field);
            extendTo(builder, field, index);
            Message element = (Message) builder.getRepeatedField(field, index);
            Message.Builder elementBuilder = element.toBuilder();
            setRecursive(elementBuilder, path, pos + 1, value, originalPath);
            builder.setRepeatedField(field, index, elementBuilder.build());
        } else
        {
            if (component.index() >= 0)
                throw new ProtoPathNotFoundException("config field '" + component.name() +
                        "' is not repeated but an index is provided");
            Message.Builder elementBuilder;
            if (builder.hasField(field))
                elementBuilder = ((Message) builder.getField(field)).toBuilder();
            else
                elementBuilder = builder.newBuilderForField(field);
            setRecursive(elementBuilder, path, pos + 1, value, originalPath);
            builder.setField(field, elementBuilder.build());
        }
    }

    private static void setLeaf(Message.Builder builder,
                                PathComponent component,
                                FieldDescriptor field,
                                String value)
    {
        if (field.getJavaType() == FieldDescriptor.JavaType.MESSAGE)
            throw new ConfigException("config field '" + component.name() +
                    "' is a message and can't be directly set");

        Object coerced = coerce(field, value);

        if (field.isRepeated())
        {
            int index = requireIndex(component, field);
            extendScalarTo(builder, field, index);
            builder.setRepeatedField(field, index, coerced);
        } else
        {
            if (component.index() >= 0)
                throw new ProtoPathNotFoundException("config field '" + component.name() +
                        "' is not repeated but an index is provided");
            builder.setField(field, coerced);
        }
    }

    private static FieldDescriptor findField(Message.Builder builder,
                                             PathComponent component,
                                             String path)
    {
        FieldDescriptor field = builder.getDescriptorForType().findFieldByName(component.name());
        if (field == null)
            throw new ProtoPathNotFoundException(
                    "no such config field '" + component.name() + "' in '" + path + "'");
        return field;
    }

    private static int requireIndex(PathComponent component, FieldDescriptor field)
    {
        if (component.index() < 0)
            throw new ProtoPathNotFoundException(
                    "config field '" + component.name() + "' is repeated and must be indexed");
        return component.index();
    }

    private static void extendTo(Message.Builder builder, FieldDescriptor field, int index)
    {
        while (builder.getRepeatedFieldCount(field) <= index)
            builder.addRepeatedField(field, builder.newBuilderForField(field).build());
    }

    private static void extendScalarTo(Message.Builder builder, FieldDescriptor field, int index)
    {
        Object defaultValue = scalarDefault(field);
        while (builder.getRepeatedFieldCount(field) <= index)
            builder.addRepeatedField(field, defaultValue);
    }

    private static Object scalarDefault(FieldDescriptor field)
    {
        switch (field.getType())
        {
            case FLOAT:
                return 0.0f;
            case DOUBLE:
                return 0.0;
            case INT32:
            case SINT32:
            case SFIXED32:
            case UINT32:
            case FIXED32:
                return 0;
            case INT64:
            case SINT64:
            case SFIXED64:
            case UINT64:
            case FIXED64:
                return 0L;
            case STRING:
                return "";
            case BOOL:
                return false;
            case ENUM:
                return field.getEnumType().getValues().get(0);
            default:
                throw new ConfigException("can't set this config value type");
        }
    }

    private static Object coerce(FieldDescriptor field, String value)
    {
        try
        {
            switch (field.getType())
            {
                case FLOAT:
                    return Float.parseFloat(value);
                case DOUBLE:
                    return Double.parseDouble(value);
                case INT32:
                case SINT32:
                case SFIXED32:
                    return Integer.parseInt(value);
                case UINT32:
                case FIXED32:
                    return Integer.parseUnsignedInt(value);
                case INT64:
                case SINT64:
                case SFIXED64:
                    return Long.parseLong(value);
                case UINT64:
                case FIXED64:
                    return Long.parseUnsignedLong(value);
                case STRING:
                    return value;
                case BOOL:
                    return parseBoolean(value);
                case ENUM:
                    EnumValueDescriptor enumValue = field.getEnumType().findValueByName(value);
                    if (enumValue == null)
                        throw new ConfigException("unrecognised enum value '" + value + "'");
                    return enumValue;
                default:
                    throw new ConfigException("can't set this config value type");
            }
        } catch (NumberFormatException e)
        {
            throw new ConfigException("invalid number '" + value + "'");
        }
    }

    private static boolean parseBoolean(String value)
    {
        switch (value)
        {
            case "false":
            case "f":
            case "no":
            case "n":
            case "0":
                return false;
            case "true":
            case "t":
            case "yes":
            case "y":
            case "1":
                return true;
            default:
                throw new ConfigException("invalid boolean value");
        }
    }

    private record PathComponent(String name, int index)
    {
    }
}
